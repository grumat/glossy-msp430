#include "stdproj.h"
#include "TapMcu.h"
#include "util/bytes.h"
#include "drivers/JtagDev.h"

#include "TapDev430X.h"
#include "TapDev430Xv2_1377.h"

#include "eem_defs.h"


using namespace ChipInfoDB;

JtagDev jtag_device;
TapMcu g_TapMcu;

TapDev430 msp430legacy_;
TapDev430X msp430X_;
TapDev430Xv2 msp430Xv2_;
TapDev430Xv2_1377 msp430Xv2_1377_;


bool TapMcu::Open()
{
	attached_ = false;
	chip_info_.DefaultMcu();
	max_breakpoints = 2; //supported by all devices

	g_Player.itf_ = &jtag_device;
	traits_ = &msp430legacy_;
	cpu_ctx_.eem_mask_ = 0x0417;
	failed_ = !g_Player.itf_->OnOpen();

	if (failed_)
	{
		Error() << "can't open port\n";
		return false;
	}

	if (InitDevice() == false)
	{
		Error() << "initialization failed\n";
		return false;
	}
	attached_ = true;
	RedLedOn();
	GreenLedOff();
	return true;
}


/*!
This function checks if the JTAG access security fuse is blown.

\return: true - fuse is blown; false - otherwise
*/
bool TapMcu::IsFuseBlown()
{
	// First trial could be wrong
	for (uint32_t loop_counter = 3; loop_counter > 0; loop_counter--)
	{
		if (g_Player.Play(kIrDr16(IR_CNTRL_SIG_CAPTURE, 0xAAAA)) == 0x5555)
			return true;	// Fuse is blown
	}
	return false;			// Fuse is not blown
}

/*!
Release the target device from JTAG control.

\param address: 0xFFFE - perform Reset, load Reset Vector into PC
	0xFFFF - start execution at current PC position
	other  - load Address into PC
*/
void TapMcu::ReleaseDevice(address_t address)
{
	/* delete all breakpoints */
	if (address == V_RESET)
		SetBreakpoint(-1, 0);

	traits_->ReleaseDevice(address);
}


bool TapMcu::StartMcu()
{
	g_Player.itf_ = &g_Player.itf_->OnStetupArchitecture(chip_info_.arch_);

	switch (chip_info_.arch_)
	{
	case ChipInfoDB::kCpuXv2:
		traits_ = HasIssue1377() ? (ITapDev *)&msp430Xv2_1377_ : (ITapDev *)&msp430Xv2_;
		break;
	case ChipInfoDB::kCpuX:
		traits_ = &msp430X_;
		break;
	default:
		traits_ = &msp430legacy_;
		break;
	}

	/* Perform PUC, includes target watchdog disable */
	if (ExecutePOR() == false)
	{
		Error() << "jtag_init: PUC failed\n";
		return false;
	}

	failed_ = false;
	return true;
}


bool TapMcu::InitDevice()
{
	Debug() << "Starting JTAG\n";
	failed_ = true;
	core_id_.Init();
	traits_ = &msp430legacy_;

	// see GetCoreID()
	size_t tries = 0;
	for (;;)
	{
		// release JTAG/TEST signals to safely reset the test logic
		g_Player.itf_->OnReleaseJtag();
		// establish the physical connection to the JTAG interface
		g_Player.itf_->OnConnectJtag();
		__NOP();
		// Apply again 4wire/SBW entry Sequence.
		g_Player.itf_->OnEnterTap();
		// reset TAP state machine -> Run-Test/Idle
		g_Player.itf_->OnResetTap();
		// shift out JTAG ID
		core_id_.jtag_id_ = (JtagId)g_Player.IR_Shift(IR_CNTRL_SIG_CAPTURE);
		__NOP();
		// break if a valid JTAG ID is being returned
		if (core_id_.IsMSP430())
			break;
		// Stop on errors
		if (++tries == kMaxEntryTry)
		{
			Error() << "jtag_init: no device found\n";
			core_id_.jtag_id_ = kInvalid;
			return false;
		}
	}

	// Check fuse
	if (IsFuseBlown())
	{
		Error() << "jtag_init: fuse is blown\n";
		return false;
	}

	/*
	** Before a database lookup we cannot be more specific, so load a general
	** compatible function set.
	*/
	traits_ = core_id_.IsXv2() ? (ITapDev *)&msp430Xv2_ : (ITapDev *)&msp430legacy_;
	// Capture device into JTAG mode
	if (!traits_->GetDevice(core_id_))
	{
		Error() << "jtag_init: invalid jtag_id: 0x" << f::X<2>(core_id_.jtag_id_) << '\n';
		core_id_.Init();
		return false;
	}
	// Forward detect CPUX devices, before database lookup
	if (core_id_.coreip_id_ == 0 && ChipProfile::IsCpuX_ID(core_id_.device_id_))
		traits_ = &msp430X_;

	//traits_->SyncJtag();
	cpu_ctx_.jtag_id_ = core_id_.jtag_id_;
	__NOP();
	// TODO: Use a all case valid ChipProfile
	ChipProfile tmp;
	tmp.DefaultMcuXv2();
	traits_->SyncJtagAssertPorSaveContext(cpu_ctx_, tmp);
	// Check watchdog key
	if (cpu_ctx_.wdt_ & 0xff00 != ETKEY)
	{
		Error() << "jtag_init: invalid WDT key: 0x" << f::X<4>(cpu_ctx_.wdt_) << '\n';
		core_id_.Init();
		return false;
	}

	if (!IsMSP430())
	{
		Error() << "pif: unexpected JTAG ID: 0x" << f::X<2>(core_id_.jtag_id_) << '\n';
		ReleaseDevice(V_RESET);
		return false;
	}
	Trace() << "JTAG ID: 0x" << f::X<2>(core_id_.jtag_id_) << '\n';

	if (ProbeId() == false
		|| StartMcu() == false)
	{
		Close();
		return false;
	}
	return true;
}


void TapMcu::Close()
{
	ClearError();

	if(attached_)
	{
		ReleaseDevice(V_RUNNING);
		attached_ = false;
		RedLedOff();
		GreenLedOn();
	}
	g_Player.itf_->OnClose();
	traits_ = &msp430legacy_;
}


uint32_t TapMcu::OnGetReg(int reg)
{
	ClearError();

	uint32_t v = UINT32_MAX;
	if (traits_->StartGetRegs())
	{
		// read register
		v = traits_->GetReg(reg);
		traits_->StopGetRegs();
	}
	return v;
}


void TapMcu::OnClearState()
{
	ClearError();
}


bool TapMcu::OnSetReg(int reg, uint32_t value)
{
	ClearError();
	if (reg == 0)
		return traits_->SetPC(value);
	return traits_->SetReg(reg, value);
}


bool TapMcu::OnGetRegs(address_t *regs)
{
	int i;

	ClearError();

	if(traits_->StartGetRegs())
	{
		for (i = 0; i < DEVICE_NUM_REGS; i++)
			regs[i] = traits_->GetReg(i);
	}
	traits_->StopGetRegs();
	return true;
}


int TapMcu::OnSetRegs(address_t *regs)
{
	ClearError();

	if (!traits_->SetPC(regs[0]))
		return UINT32_MAX;
	for (int i = 1; i < DEVICE_NUM_REGS; i++)
	{
		if(!traits_->SetReg(i, regs[i]))
			return UINT32_MAX;
	}
	return 0;
}


/*!
Read a word-aligned block from any kind of memory
returns the number of bytes read or UINT32_MAX on failure
*/
address_t TapMcu::OnReadWords(address_t address, void *data, address_t word_count)
{
	ClearError();

	// 16-bit aligned address required
	assert((address & 1) == 0);
	// At least one word is required
	assert(word_count > 0);

	if (word_count == 1)
		*(uint16_t *)data = traits_->ReadWord(address);
	else if (!traits_->ReadWords(address, (uint16_t *)data, word_count))
		return UINT32_MAX;
	return word_count;
}


/*!
Given an address range, specified by a start and a size (in bytes),
return a size which is trimmed so as to not overrun a region boundary
in the chip's memory map.

The single region occupied is optionally returned in m_ret. If the
range doesn't start in a valid region, it's trimmed to the start of
the next valid region, and m_ret is NULL.
*/
address_t TapMcu::check_range(address_t addr, address_t size, const MemInfo **mem)
{
	const MemInfo *m = chip_info_.FindMemByAddress(addr);

	if (m)
	{
		if (m->start_ > addr)
		{
			address_t n = m->start_ - addr;

			if (size > n)
				size = n;

			m = NULL;
		}
		else if (addr + size > m->start_ + m->size_)
		{
			size = m->start_ + m->size_ - addr;
		}
	}

	if(mem)
		*mem = m;

	return size;
}


int TapMcu::ReadMem(address_t addr, void *mem_, address_t len)
{
	if (!attached_)
		return -1;

	OnClearState();

	uint8_t *mem = (uint8_t *)mem_;

	const MemInfo *m;

	if (!len)
		return 0;

	/* Handle unaligned start */
	if (addr & 1)
	{
		uint8_t data[2];
		check_range(addr - 1, 2, &m);
		if (!m)
			data[1] = 0x55;
		else if (OnReadWords(addr - 1, data, 1) < 0)
			return -1;

		mem[0] = data[1];
		addr++;
		mem++;
		len--;
	}

	/* Read aligned blocks */
	while (len >= 2)
	{
		int rlen = check_range(addr, len & ~1, &m);
		if (!m)
			memset(mem, 0x55, rlen);
		else
		{
			rlen = OnReadWords(addr, mem, rlen >> 1);
			if (rlen < 0)
				return -1;
			rlen <<= 1;
		}

		addr += rlen;
		mem += rlen;
		len -= rlen;
	}

	/* Handle unaligned end */
	if (len)
	{
		uint8_t data[2];
		check_range(addr, 2, &m);
		if (!m)
			data[0] = 0x55;
		else if (OnReadWords(addr, data, 1) < 0)
			return -1;

		mem[0] = data[0];
	}

	return 0;
}


/*!
Write a word-aligned flash block.
The starting address must be within the flash memory range.
*/
int TapMcu::write_flash_block(address_t addr, address_t len, const uint8_t *data)
{
	unsigned int i;
	uint16_t *word;

	// TODO: GET RID OF MALLOC!!!

	word = (uint16_t *)malloc(len / 2 * sizeof(*word));
	if (!word)
	{
		Error() << "pif: failed to allocate memory\n";
		return -1;
	}

	for (i = 0; i < len / 2; i++)
	{
		word[i] = data[2 * i] + (((uint16_t)data[2 * i + 1]) << 8);
	}
	bool res = traits_->WriteFlash(addr, word, len / 2);

	free(word);

	return res ? 0 : -1;
}


/*!
Write a word-aligned block to any kind of memory.
returns the number of bytes written or -1 on failure
*/
int TapMcu::OnWriteWords(const MemInfo *m, address_t addr, const void *data_, address_t len)
{
	int r;
	const uint8_t *data = (const uint8_t *)data_;

	if (m->type_ != ChipInfoDB::kFlash)
	{
		len = 2;
		if(!traits_->WriteWord(addr, r16le(data)))
			goto failure;
	}
	else
	{
		if (write_flash_block(addr, len, data) < 0)
		{
failure:
			Error() << "pif: write_words at address 0x" << f::X<4>(addr) << " failed\n";
			return -1;
		}
	}
	return len;
}


int TapMcu::WriteMem(address_t addr, const void *mem_, address_t len)
{
	if (!attached_)
		return -1;

	const MemInfo *m;
	const uint8_t *mem = (const uint8_t *)mem_;

	if (!len)
		return 0;

	/* Handle unaligned start */
	if (addr & 1)
	{
		uint8_t data[2];
		check_range(addr - 1, 2, &m);
		if (!m)
			goto fail; // fail on unmapped regions

		if (OnReadWords(addr - 1, data, 1) < 0)
			return -1;

		data[1] = mem[0];

		if (OnWriteWords(m, addr - 1, data, 1) < 0)
			return -1;

		addr++;
		mem++;
		len--;
	}

	while (len >= 2)
	{
		int wlen = check_range(addr, len & ~1, &m);
		if (!m)
			goto fail; // fail on unmapped regions

		wlen = OnWriteWords(m, addr, mem, wlen);
		if (wlen < 0)
			return -1;

		addr += wlen;
		mem += wlen;
		len -= wlen;
	}

	/* Handle unaligned end */
	if (len)
	{
		uint8_t data[2];
		check_range(addr, 2, &m);
		if (!m)
			goto fail; // fail on unmapped regions

		if (OnReadWords(addr, data, 1) < 0)
			return -1;

		data[0] = mem[0];

		if (OnWriteWords(m, addr, data, 1) < 0)
			return -1;
	}
	return 0;

fail:
	Error() << "TapMcu::WriteMem failed at 0x" << f::X<4>(addr) << '\n';
	return -1;
}


bool TapMcu::EraseMain()
{
	ClearError();

	const MemInfo &flash = chip_info_.GetMainMem();
	if (flash.type_ != kFlash)
	{
		Trace() << "Main memory is not erasable!\n";
		return true;	// silent acceptance
	}

	// Ensures EraseModeFctl values are valid because logic is hard coded
	static_assert(kMainEraseSlau049 == kMainEraseSlau335, "EraseModeFctl value ranges are hard coded here. Changes will cause malfunction.");
	static_assert(kMainEraseSlau208 == kMainEraseSlau259, "EraseModeFctl value ranges are hard coded here. Changes will cause malfunction.");

	EraseModeFctl ctrl = (chip_info_.slau_ == kSLAU049 || chip_info_.slau_ == kSLAU335)
		? kMainEraseSlau049 : (chip_info_.slau_ == kSLAU056)
		? kMainEraseSlau056 : (chip_info_.slau_ == kSLAU144)
		? kMainEraseSlau144
		: kMainEraseSlau259
		;
	return EraseFlash(flash.start_, ctrl);
}


bool TapMcu::EraseAll()
{
	ClearError();

	const MemInfo &flash = chip_info_.GetMainMem();
	if (flash.type_ != kFlash)
	{
		Trace() << "Main memory is not erasable!\n";
		return true;	// silent acceptance
	}

	// Ensures EraseModeFctl values are valid because logic is hard coded
	static_assert(kMassEraseSlau259 == kMassEraseSlau049, "EraseModeFctl value ranges are hard code here. Changes will cause malfunction.");
	static_assert(kMassEraseSlau259 == kMassEraseSlau208, "EraseModeFctl value ranges are hard code here. Changes will cause malfunction.");
	static_assert(kMassEraseSlau259 == kMassEraseSlau335, "EraseModeFctl value ranges are hard code here. Changes will cause malfunction.");

	// Select appropriate register control values
	EraseModeFctl ctrl = (chip_info_.slau_ == kSLAU056)
		? kMassEraseSlau056 : (chip_info_.slau_ == kSLAU144)
		? kMassEraseSlau144
		: kMassEraseSlau259
		;
	// Do erase flash memory
	if(!EraseFlash(flash.start_, ctrl))
		return false;
	// Newer families require explicit INFO memory erase
	if (chip_info_.slau_ >= kSLAU144
		&& chip_info_.slau_ != kSLAU335)
	{
		// INFO Memory needs to be cleared separately
		const MemInfo &info = chip_info_.GetInfoMem();
		// Protect INFOA in SLAU144 as it contains factory default calibration values (TLV record)
		const int banks = info.banks_ - (chip_info_.slau_ == kSLAU144);
		uint32_t addr = info.start_;
		for (int i = 0; i < banks; ++i)
		{
			if(!EraseFlash(addr, kSegmentEraseGeneral))
				return false;
			addr += info.segsize_;
		}
	}
	return true;
}


bool TapMcu::EraseSegment(address_t addr)
{
	ClearError();

	const MemInfo *pFlash = chip_info_.FindMemByAddress(addr);
	if (pFlash == NULL)
	{
		Error() << "Address 0x" << f::X<4>(addr) << " not found in device memory map!\n";
		return false;
	}
	if (pFlash->type_ != kFlash)
	{
		Trace() << "Address 0x" << f::X<4>(addr) << " is not erasable!\n";
		return true;	// silent acceptance
	}

	// Ensures EraseModeFctl values are valid because logic is hard coded
	static_assert(kSegmentEraseGeneral == kSegmentEraseSlau049, "EraseModeFctl value ranges are hard code here. Changes will cause malfunction.");
	static_assert(kSegmentEraseGeneral == kSegmentEraseSlau056, "EraseModeFctl value ranges are hard code here. Changes will cause malfunction.");
	static_assert(kSegmentEraseGeneral == kSegmentEraseSlau144, "EraseModeFctl value ranges are hard code here. Changes will cause malfunction.");
	static_assert(kSegmentEraseGeneral == kSegmentEraseSlau208, "EraseModeFctl value ranges are hard code here. Changes will cause malfunction.");
	static_assert(kSegmentEraseGeneral == kSegmentEraseSlau259, "EraseModeFctl value ranges are hard code here. Changes will cause malfunction.");
	static_assert(kSegmentEraseGeneral == kSegmentEraseSlau335, "EraseModeFctl value ranges are hard code here. Changes will cause malfunction.");

	Debug() << "Erasing 0x" << f::X<4>(addr) << "...\n";
	return EraseFlash(addr, kSegmentEraseGeneral);
}


bool TapMcu::EraseRange(address_t addr, address_t size)
{
	ClearError();

	const MemInfo *pFlash = chip_info_.FindMemByAddress(addr);
	if (pFlash == NULL)
	{
		Error() << "Address 0x" << f::X<4>(addr) << " not found in device memory map!\n";
		return false;
	}
	if (pFlash->type_ != kFlash)
	{
		Trace() << "Address 0x" << f::X<4>(addr) << " is not erasable!\n";
		return true;	// silent acceptance
	}
	uint32_t memtop = pFlash->start_ + pFlash->size_;
	if (memtop < addr + size)
		Trace() << "Size of 0x" << f::X<4>(size) << " overflows memory segment!\n";
	while (addr < memtop)
	{
		Debug() << "Erasing 0x" << f::X<4>(addr) << "...\n";
		if(!EraseFlash(addr, kSegmentEraseGeneral))
			return false;
		addr += pFlash->segsize_;
	}
	return true;
}


int TapMcu::addbrk(address_t addr, device_bptype_t type)
{
	int which = -1;
	struct device_breakpoint *bp;

	for (int i = 0; i < max_breakpoints; i++)
	{
		bp = &breakpoints[i];

		if (bp->flags & DEVICE_BP_ENABLED)
		{
			if (bp->addr == addr && bp->type == type)
				return i;
		}
		else if (which < 0)
		{
			which = i;
		}
	}

	if (which < 0)
		return -1;

	bp = &breakpoints[which];
	bp->flags = DEVICE_BP_ENABLED | DEVICE_BP_DIRTY;
	bp->addr = addr;
	bp->type = type;

	return which;
}


void TapMcu::delbrk(address_t addr, device_bptype_t type)
{
	for (int i = 0; i < max_breakpoints; i++)
	{
		device_breakpoint *bp = &breakpoints[i];

		if ((bp->flags & DEVICE_BP_ENABLED) &&
			bp->addr == addr && bp->type == type)
		{
			bp->flags = DEVICE_BP_DIRTY;
			bp->addr = 0;
		}
	}
}


int TapMcu::SetBrk(int which, int enabled, address_t addr, device_bptype_t type)
{
	if (which < 0)
	{
		if (enabled)
			return addbrk(addr, type);

		delbrk(addr, type);
	}
	else
	{
		struct device_breakpoint *bp = &breakpoints[which];
		int new_flags = enabled ? DEVICE_BP_ENABLED : 0;

		if (!enabled)
			addr = 0;

		if (bp->addr != addr ||
			(bp->flags & DEVICE_BP_ENABLED) != new_flags)
		{
			bp->flags = new_flags | DEVICE_BP_DIRTY;
			bp->addr = addr;
			bp->type = type;
		}
	}
	return 0;
}


void TapMcu::ClearBrk()
{
	for (int i = 0; i < max_breakpoints; ++i)
		SetBrk(i, 0, 0, DEVICE_BPTYPE_BREAK);
}


int TapMcu::tlv_read(uint8_t *tlv_data)
{
	if (ReadMem(0x1a00, tlv_data, 8) < 0)
		return -1;

	uint8_t info_len = tlv_data[0];
	if (info_len < 1 || info_len > 8)
		return -1;

	int tlv_size = 4 * (1 << info_len);
	if (ReadMem(0x1a00 + 8, tlv_data + 8, tlv_size - 8) < 0)
		return -1;
	return 0;
}


int TapMcu::tlv_find(const uint8_t *tlv_data, const uint8_t type, uint8_t *const size, const uint8_t **const ptr)
{
	const int tlv_size = 4 * (1 << tlv_data[0]);
	int i = 8;
	*ptr = NULL;
	*size = 0;
	while (i + 3 < tlv_size)
	{
		uint8_t tag = tlv_data[i++];
		uint8_t len = tlv_data[i++];

		if (tag == 0xff)
			break;

		if (tag == type)
		{
			*ptr = tlv_data + i;
			*size = len;
			break;
		}
		i += len;
	}
	return *ptr != NULL;
}


/* Is there a more reliable way of doing this? */
int TapMcu::device_is_fram()
{
	return chip_info_.is_fram_;
}


void TapMcu::show_device_type()
{
#if DEBUG
	static constexpr const char *clas[] =
	{
		"Main:"
		, "RAM:"
		, "RAM2:"
		, "TinyRAM:"
		, "LeaRAM:"
		, "Info:"
		, "CPU:"
		, "IV:"
		, "LCD:"
		, "EEM:"
		, "Boot:"
		, "Boot2:"
		, "BSL:"
		, "BSL2:"
		, "MidROM:"
		, "USBRAM:"
		, "Periph8:"
		, "Periph16:"
		, "Periph16(1):"
		, "Periph16(2):"
		, "Periph16(3):"
		, "UssPer:"
		, "LeaPer:"
		, "ClasLib:"
	};
	static constexpr const char *mem_type[] =
	{
		"Unkn"
		, "Regs"
		, "Flash"
		, "RAM"
		, "ROM"
	};
	static constexpr const char *slau[] =
	{
		"SLAU012"
		, "SLAU049"
		, "SLAU056"
		, "SLAU144"
		, "SLAU208"
		, "SLAU259"
		, "SLAU321"
		, "SLAU335"
		, "SLAU367"
		, "SLAU378"
		, "SLAU445"
		, "SLAU506"
	};
	static constexpr const char *eem[] =
	{
	"NONE"
	, "LOW"
	, "MEDIUM"
	, "HIGH"
	, "EXTRA_SMALL_5XX"
	, "SMALL_5XX"
	, "MEDIUM_5XX"
	, "LARGE_5XX"
	};
	static_assert(_countof(clas) == ChipInfoDB::kClasMax_, "Array size does not match enumeration");
	static_assert(_countof(mem_type) == ChipInfoDB::kMemTypeMax, "Array size does not match enumeration");
	static_assert(_countof(slau) == ChipInfoDB::kSlauMax_, "Array size does not match enumeration");
#endif
	Trace msg;
	msg << "Device: " << chip_info_.name_;
	// Architecture
	if(chip_info_.arch_ == ChipInfoDB::kCpuXv2)
		msg << " [CPUXv2]";
	else if (chip_info_.arch_ == ChipInfoDB::kCpuX)
		msg << " [CPUX]";
	// Fram
	if (device_is_fram())
		msg << " [FRAM]";
#if DEBUG
	msg << " [EMEX_" << eem[chip_info_.eem_type_]
		<< "] [" << slau[chip_info_.slau_]
		<< "]";
	if (chip_info_.issue_1377_)
		msg << " [1377]";
	if (chip_info_.quick_mem_read_)
		msg << " [QUICK]";
	msg << '\n';
	for (int i = 0; i < _countof(chip_info_.mem_); ++i)
	{
		const MemInfo &mem = chip_info_.mem_[i];
		if(mem.valid_ == false)
			break;
		// Left align size of memory
		StringBuf<18> tmp;
		tmp << f::K(mem.size_);
		// Put line
		msg << '\t' << f::S<12>(clas[mem.class_]) << " 0x" << f::X<4>(mem.start_)
			<< "-0x" << f::X<4>(mem.start_ + mem.size_)
			<< '\t' << f::S<-8>(tmp)
			<< " [" << mem_type[mem.type_]
			;
		if (mem.banks_ > 1)
			msg << " - " << mem.banks_ << " banks";
		msg << "]\n";
	}
#else
	msg << '\n';
#endif
}


bool TapMcu::OnReadChipId(void *buf, uint32_t size)
{
	uint32_t words = size >> 1;
	// function table is not ready yet, so bypass it
	if (core_id_.IsXv2())
	{
		/*
		** MSP430F5418A does not like any read on this area without the use of the PC, so
		** we need to use the IR_DATA_QUICK to read this area.
		** This is nowhere described and costs me many wasted hours...
		*/
		//ReadWordsXv2_slau320aj(id_data_addr_, (uint16_t *)buf, words < 8 ? words : 8);
		// Now we should get a valid read
		return msp430Xv2_.ReadWords(core_id_.id_data_addr_, (uint16_t *)buf, words);
		//return ReadWordsXv2_slau320aj(id_data_addr_, (uint16_t *)buf, words);
	}
	else
	{
		//msp430legacy_.ReadWords(id_data_addr_, (uint16_t *)buf, words < 8 ? words : 8);
		//ReadWords_slau320aj(id_data_addr_, (uint16_t *)buf, words < 8 ? words : 8);
		// Now we should get a valid read
		return msp430legacy_.ReadWords(core_id_.id_data_addr_, (uint16_t *)buf, words);
		//return ReadWords_slau320aj(id_data_addr_, (uint16_t *)buf, words);
	}
}


int TapMcu::OnGetConfigFuses()
{
	g_Player.itf_->OnIrShift(IR_CONFIG_FUSES);
	return g_Player.itf_->OnDrShift8(0);
}


bool TapMcu::ProbeId()
{
	/* proceed with identification */
	uint8_t data[16];
	uint8_t tlv_data[1024];

	DieInfo id;
	memset(&id, 0xff, sizeof(id));

	int retries = 5;
	do
	{
		OnReadChipId(data, sizeof(data));

		if (data[0] == 0x80)
		{
			if (tlv_read(tlv_data) < 0)
			{
				Error() << "device_probe_id: tlv_read failed\n";
				return false;
			}
			id.mcu_ver_ = r16le(tlv_data + 4);
			id.mcu_rev_ = tlv_data[6];
			id.mcu_cfg_ = tlv_data[7];
			id.mcu_fab_ = 0x55;
			id.mcu_self_ = 0x5555;
			id.mcu_fuse_ = 0x55;

			/* Search TLV for sub-ID */
			uint8_t len;
			const uint8_t *p;
			if (tlv_find(tlv_data, 0x14, &len, &p))
			{
				if (len >= 2)
					id.mcu_sub_ = r16le(p);
			}
		}
		else
		{
			id.mcu_ver_ = r16le(data);
			id.mcu_sub_ = 0;
			id.mcu_rev_ = data[2];
			id.mcu_fab_ = data[3];
			id.mcu_self_ = r16le(data + 8);
			id.mcu_cfg_ = data[13] & 0x7f;
			id.mcu_fuse_ = OnGetConfigFuses();
		}


		if (chip_info_.Load(id))
			break;
		Debug() << "Failed to identify chip\n";
	}
	while (--retries);

	Debug() << "Chip ID data:\n"
		"  mcu_ver:  " << f::X<4>(id.mcu_ver_) << "\n"
		"  mcu_sub:  " << f::X<4>(id.mcu_sub_) << "\n"
		"  mcu_rev:  " << f::X<2>(id.mcu_rev_) << "\n"
		"  mcu_fab:  " << f::X<2>(id.mcu_fab_) << "\n"
		"  mcu_self: " << f::X<4>(id.mcu_self_) << "\n"
		"  mcu_cfg:  " << f::X<2>(id.mcu_cfg_) << "\n"
		"  mcu_fuse: " << f::X<2>(id.mcu_fuse_) << "\n";
	// All attempts failed
	if(retries == 0)
	{
		if (IsXv2())
			chip_info_.DefaultMcuXv2();
		else
			chip_info_.DefaultMcu();
		Error() << "warning: unknown chip\n";
		return true;
	}

	show_device_type();
	return true;
}


int TapMcu::refresh_bps()
{
	int i;
	int ret;
	struct device_breakpoint *bp;
	address_t addr;
	ret = 0;

	for (i = 0; i < max_breakpoints; i++)
	{
		bp = &breakpoints[i];

		Debug() << "refresh breakpoint " << i << ": type=" << bp->type <<
			" addr=" << f::X<4>(bp->addr) << " flags=" << f::X<4>(bp->flags) << '\n';

		if ((bp->flags & DEVICE_BP_DIRTY) &&
			(bp->type == DEVICE_BPTYPE_BREAK))
		{
			addr = bp->addr;

			if (!(bp->flags & DEVICE_BP_ENABLED))
			{
				addr = 0;
			}

			if (SetBreakpoint(i, addr) == 0)
			{
				Error() << "pif: failed to refresh breakpoint #" << i << '\n';
				ret = -1;
			}
			else
			{
				bp->flags &= ~DEVICE_BP_DIRTY;
			}
		}
	}
	return ret;
}


int TapMcu::OnSoftReset()
{
	ClearError();
	// perform soft reset
	return ExecutePOR();
}


int TapMcu::OnRun()
{
	ClearError();
	// transfer changed breakpoints to device
	if (refresh_bps() < 0)
		return -1;
	// start program execution at current PC
	ReleaseDevice(V_RUNNING);
	return 0;
}


int TapMcu::OnSingleStep()
{
	ClearError();
	// execute next instruction at current PC
	SingleStep();
	return 0;
}


bool TapMcu::OnHalt()
{
	ClearError();
	// take device under JTAG control
	return traits_->GetDevice(core_id_);
}


bool TapMcu::GetCpuState()
{
	g_Player.itf_->OnIrShift(IR_EMEX_READ_CONTROL);

	if ((g_Player.itf_->OnDrShift16(0x0000) & 0x0080) == 0x0080)
	{
		return true; /* halted */
	}
	else
	{
		return false; /* running */
	}
}


device_status_t TapMcu::OnPoll()
{
	StopWatch().Delay(100);

	if (GetCpuState() != 0)
		return DEVICE_STATUS_HALTED;
	return DEVICE_STATUS_RUNNING;
}


bool TapMcu::SetBreakpoint(int bp_num, address_t bp_addr)
{
	/* The breakpoint logic is explained in 'SLAU414c EEM.pdf' */
	/* A good overview is given with Figure 1-1                */
	/* MBx           is TBx         in EEM_defs.h              */
	/* CPU Stop      is BREAKREACT  in EEM_defs.h              */
	/* State Storage is STOR_REACT  in EEM_defs.h              */
	/* Cycle Counter is EVENT_REACT in EEM_defs.h              */

	unsigned int breakreact;

	if (bp_num >= 8)
	{
		/* there are no more than 8 breakpoints in EEM */
		Error() << "jtag_set_breakpoint: failed setting "
			"breakpoint " << bp_num << " at " << f::X<4>(bp_addr) << '\n';
		failed_ = true;
		return false;
	}

	if (bp_num < 0)
	{
		/* disable all breakpoints by deleting the BREAKREACT
		 * register */
		g_Player.Play(kIrDr16(IR_EMEX_DATA_EXCHANGE, BREAKREACT + WRITE));
		g_Player.itf_->OnDrShift16(0x0000);
		return true;
	}

	/* set breakpoint */
	g_Player.Play(kIrDr16(IR_EMEX_DATA_EXCHANGE, GENCTRL + WRITE));
	g_Player.itf_->OnDrShift16(EEM_EN + CLEAR_STOP + EMU_CLK_EN + EMU_FEAT_EN);

	g_Player.itf_->OnIrShift(IR_EMEX_DATA_EXCHANGE); //repeating may not needed
	g_Player.itf_->OnDrShift16(8 * bp_num + MBTRIGxVAL + WRITE);
	g_Player.itf_->OnDrShift16(bp_addr);

	g_Player.itf_->OnIrShift(IR_EMEX_DATA_EXCHANGE); //repeating may not needed
	g_Player.itf_->OnDrShift16(8 * bp_num + MBTRIGxCTL + WRITE);
	g_Player.itf_->OnDrShift16(MAB + TRIG_0 + CMP_EQUAL);

	g_Player.itf_->OnIrShift(IR_EMEX_DATA_EXCHANGE); //repeating may not needed
	g_Player.itf_->OnDrShift16(8 * bp_num + MBTRIGxMSK + WRITE);
	g_Player.itf_->OnDrShift16(NO_MASK);

	g_Player.itf_->OnIrShift(IR_EMEX_DATA_EXCHANGE); //repeating may not needed
	g_Player.itf_->OnDrShift16(8 * bp_num + MBTRIGxCMB + WRITE);
	g_Player.itf_->OnDrShift16(1 << bp_num);

	/* read the actual setting of the BREAKREACT register         */
	/* while reading a 1 is automatically shifted into LSB        */
	/* this will be undone and the bit for the new breakpoint set */
	/* then the updated value is stored back                      */
	g_Player.itf_->OnIrShift(IR_EMEX_DATA_EXCHANGE); //repeating may not needed
	breakreact = g_Player.itf_->OnDrShift16(BREAKREACT + READ);
	breakreact += g_Player.itf_->OnDrShift16(0x000);
	breakreact = (breakreact >> 1) | (1 << bp_num);
	g_Player.itf_->OnDrShift16(BREAKREACT + WRITE);
	g_Player.itf_->OnDrShift16(breakreact);
	return true;
}

