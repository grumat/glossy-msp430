#include "stdproj.h"
#include "TapMcu.h"
#include "util/bytes.h"
#include "drivers/JtagDev.h"

#include "TapDev430X.h"
#include "TapDev430Xv2_1377.h"

#include "eem_defs.h"


using namespace ChipInfoDB;

JtagDev jtag_device;
#if OPT_JTAG_SPEED_SEL
JtagDev_2 jtag_device_2;
JtagDev_3 jtag_device_3;
JtagDev_4 jtag_device_4;
JtagDev_5 jtag_device_5;
#endif
TapMcu g_TapMcu;

TapDev430 msp430legacy_;
TapDev430X msp430X_;
TapDev430Xv2 msp430Xv2_;
TapDev430Xv2_1377 msp430Xv2_1377_;


bool TapMcu::Open()
{
	attached_ = false;
	chip_info_.DefaultMcu();
	breakpoints_.ctor();

#if OPT_JTAG_SPEED_SEL
	g_Player.itf_ = &jtag_device_5;
#else
	g_Player.itf_ = &jtag_device;
#endif
	traits_ = &msp430legacy_;
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
	SetLedState(LedState::red);
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


bool TapMcu::StartMcu()
{
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
		WATCHPOINT();
		// Apply again 4wire/SBW entry Sequence.
		g_Player.itf_->OnEnterTap();
		// reset TAP state machine -> Run-Test/Idle
		g_Player.itf_->OnResetTap();
		// shift out JTAG ID
		core_id_.jtag_id_ = (JtagId)g_Player.IR_Shift(IR_CNTRL_SIG_CAPTURE);
		// break if a valid JTAG ID is being returned
		if (core_id_.IsMSP430())
			break;
		WATCHPOINT();
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
	// Hint: <core_id_.coreip_id_ == 0> is equivalent to <!core_id_.IsXv2()> in this context
	if (core_id_.coreip_id_ == 0 && ChipProfile::IsCpuX_ID(core_id_.device_id_))
		traits_ = &msp430X_;

	cpu_ctx_.pc_ = 0xFFFE;
	//traits_->SyncJtag();
	cpu_ctx_.jtag_id_ = core_id_.jtag_id_;
	// Empty CPU profile will set a default part for initialization
	chip_info_.Init();
	traits_->InitDefaultChip(chip_info_);
	traits_->SyncJtagAssertPorSaveContext(cpu_ctx_, chip_info_);
	// Check watchdog key
	if (cpu_ctx_.wdt_ & 0xff00 != ETKEY)
	{
		Error() << "jtag_init: invalid WDT key: 0x" << f::X<4>(cpu_ctx_.wdt_) << '\n';
		core_id_.Init();
		return false;
	}

	Trace() << "JTAG ID: 0x" << f::X<2>(core_id_.jtag_id_) << '\n';

	if (ProbeId() == false
		|| StartMcu() == false)
	{
		Close();
		return false;
	}
	__NOP();
	return true;
}


void TapMcu::Close()
{
	ClearError();

	if(attached_)
	{
		traits_->ReleaseDevice(cpu_ctx_, chip_info_, false);
		attached_ = false;
	}
	SetLedState(LedState::green);
	g_Player.itf_->OnClose();
	traits_ = &msp430legacy_;
	ClearBrk();
}


uint32_t TapMcu::OnGetReg(int reg)
{
	ClearError();

	// Cached registers
	if (reg == 0)
		return cpu_ctx_.pc_;
	if (reg == 2)
		return cpu_ctx_.sr_;
	// All others
	uint32_t v = UINT32_MAX;
	if (traits_->GetRegs_Begin())
	{
		// read register
		v = traits_->GetReg(reg);
		traits_->GetRegs_End();
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
	{
		cpu_ctx_.pc_ = value;
		return traits_->SetPC(value);
	}
	else if (reg == 2)
		cpu_ctx_.sr_ = value;
	return traits_->SetReg(reg, value);
}


bool TapMcu::OnGetRegs(address_t *regs)
{
	int i;

	ClearError();

	if(traits_->GetRegs_Begin())
	{
		for (i = 0; i < DEVICE_NUM_REGS; i++)
		{
			uint32_t r;
			if (i == 0)
				r = cpu_ctx_.pc_;
			else if (i == 2)
				r = cpu_ctx_.sr_;
			else
				r = traits_->GetReg(i);
			regs[i] = r;
		}
	}
	traits_->GetRegs_End();
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


void TapMcu::OnReadBytes(address_t address, void *data, address_t byte_count)
{
	ClearError();

	if (byte_count == 1)
		*(uint16_t *)data = traits_->ReadByte(address);
	else 
		traits_->ReadBytes(address, (uint8_t *)data, byte_count);
}


/*!
Read a word-aligned block from any kind of memory
returns the number of bytes read or UINT32_MAX on failure
*/
void TapMcu::OnReadWords(address_t address, void *data, address_t word_count)
{
	ClearError();

	// 16-bit aligned address required
	assert((address & 1) == 0);
	// At least one word is required
	assert(word_count > 0);

	if (word_count == 1)
		*(uint16_t *)data = traits_->ReadWord(address);
	else 
		traits_->ReadWords(address, (uint16_t *)data, word_count);
}


/*!
Given an address range, specified by a start and a size (in bytes),
return a size which is trimmed so as to not overrun a region boundary
in the chip's memory map.

The single region occupied is optionally returned in m_ret. If the
range doesn't start in a valid region, it's trimmed to the start of
the next valid region, and m_ret is NULL.
*/
address_t TapMcu::CheckRange(address_t addr, address_t size, const MemInfo **mem)
{
	const MemInfo *m = chip_info_.FindMemByAddress(addr);

	if (m)
	{
		if (addr + size > m->start_ + m->size_)
		{
			size = m->start_ + m->size_ - addr;
		}
	}
	// A 0-sized block has no meaning
	if (size == 0)
		m = NULL;
	if(mem)
		*mem = m;
	return size;
}


bool TapMcu::ReadMem(address_t addr, void *mem_, address_t len)
{
	if (!attached_)
		return false;

	OnClearState();

	uint8_t *mem = (uint8_t *)mem_;

	const MemInfo *m;

	if (!len)
		return true;

	// Handle unaligned start
	if (addr & 1)
	{
		address_t rlen = CheckRange(addr, 1, &m);
		if (rlen == 0 || m == NULL)
			mem[0] = 0x55;
		else 
			OnReadBytes(addr, &mem[0], 1);
		++addr;
		++mem;
		--len;
	}

	// Read aligned blocks
	while (len >= 2)
	{
		address_t rlen = CheckRange(addr, len & ~1, &m);
		if (rlen == 0 || m == NULL)
			memset(mem, 0x55, rlen);
		else if (m->bit_size_ > 8)
			OnReadWords(addr, mem, rlen >> 1);
		else
			OnReadBytes(addr, mem, rlen);

		addr += rlen;
		mem += rlen;
		len -= rlen;
	}

	// Handle unaligned end
	if (len)
	{
		address_t rlen = CheckRange(addr, 1, &m);
		if (rlen == 0 || m == NULL)
			mem[0] = 0x55;
		else
			OnReadBytes(addr, &mem[0], 1);
	}
	return true;
}


/*!
Write a word-aligned block to any kind of memory.
returns the number of bytes written or -1 on failure
*/
void TapMcu::OnWriteWords(const MemInfo *m, address_t addr, const void *data_, int wordcount)
{
	if (m->type_ != ChipInfoDB::kMtypFlash)
		traits_->WriteWords(addr, (const unaligned_u16 *)data_, wordcount);
	else
		traits_->WriteFlash(addr, (const unaligned_u16 *)data_, wordcount);
}


bool TapMcu::WriteMem(address_t addr, const void *mem_, address_t len)
{
	if (!attached_)
		return false;

	const MemInfo *m;
	const uint8_t *mem = (const uint8_t *)mem_;

	if (!len)
		return true;

	// Handle unaligned start
	if (addr & 1)
	{
		uint8_t data[2];
		--addr;
		address_t blklen = CheckRange(addr, 2, &m);
		if (blklen == 0 || m == NULL)
			goto fail; // fail on unmapped regions
		// Read-Modify-Write
		OnReadWords(addr, data, 1);
		data[1] = mem[0];
		OnWriteWords(m, addr, data, 1);
		// Update pointers and counter
		addr += 2;
		mem++;
		len--;
	}

	while (len >= 2)
	{
		// Search block on memory map
		address_t blklen = CheckRange(addr, len & ~1, &m);
		if (blklen == 0 || m == NULL)
			goto fail; // fail on unmapped regions
		OnWriteWords(m, addr, mem, blklen>>1);
		// Next word
		addr += blklen;
		mem += blklen;
		len -= blklen;
	}

	// Handle unaligned end
	if (len)
	{
		uint8_t data[2];
		address_t blklen = CheckRange(addr, 2, &m);
		if (blklen == 0 || m == NULL)
			goto fail; // fail on unmapped regions
		// Read-Modify-Write
		OnReadWords(addr, data, 1);
		data[0] = mem[0];
		OnWriteWords(m, addr, data, 1);
		}
	return true;

fail:
	Error() << "TapMcu::WriteMem failed at 0x" << f::X<4>(addr) << '\n';
	return false;
}


bool TapMcu::EraseMain()
{
	Trace() << "Erasing Main memory";
	ClearError();

	const MemInfo &flash = chip_info_.GetMainMem();
	if (flash.type_ != kMtypFlash)
	{
		Trace() << "Main memory is not erasable!\n";
		return true;	// silent acceptance
	}

	FlashEraseFlags flags(chip_info_.has_locka_, false);
	flags.MainErase(chip_info_.has_gmeras_);
	return EraseFlash(flash.start_, flags, kMassErase);
}


bool TapMcu::EraseAll()
{
	Trace() << "Erasing Main+INFO memories";
	ClearError();

	const MemInfo &flash = chip_info_.GetMainMem();
	if (flash.type_ != kMtypFlash)
	{
		Trace() << "Main memory is not erasable!\n";
		return true;	// silent acceptance
	}
	
	FlashEraseFlags flags(chip_info_.has_locka_, false);
	FlashEraseFlags seg(flags);
	
	flags.MassErase(chip_info_.has_gmeras_);

	// Do erase flash memory
	if (!EraseFlash(flash.start_, flags, kMassErase))
		return false;
	// Newer families require explicit INFO memory erase
	if (!chip_info_.has_1p_mass_erase_)
	{
		seg.EraseSegment();
		// INFO Memory needs to be cleared separately
		const MemInfo &info = chip_info_.GetInfoMem();
		// Protect INFOA in SLAU144 as it contains factory default calibration values (TLV record)
		const int banks = info.banks_ - chip_info_.tlv_clash_;
		uint32_t addr = info.start_;
		for (int i = 0; i < banks; ++i)
		{
			if (!EraseFlash(addr, seg))
				return false;
			addr += info.segsize_;
		}
	}
	return true;
}


bool TapMcu::EraseInfoA()
{
	Trace() << "Erasing INFOA memory";
	ClearError();

	// Chip does not locks INFOA segment
	if (chip_info_.has_locka_ == false)
		return false;
	// Info memory
	const MemInfo &info = chip_info_.GetInfoMem();
	// Last segment is INFO A
	address_t addr = info.start_ + info.size_ - info.segsize_;
	// Option to unlock Info A
	FlashEraseFlags flags(true, true);
	flags.EraseSegment();
	// Clear INFO A
	return EraseFlash(addr, flags);
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
	if (pFlash->type_ != kMtypFlash)
	{
		Trace() << "Address 0x" << f::X<4>(addr) << " is not erasable!\n";
		return true;	// silent acceptance
	}
	
	Debug() << "Erasing 0x" << f::X<4>(addr) << "...\n";
	FlashEraseFlags flags(chip_info_.has_locka_, false);
	flags.EraseSegment();
	return EraseFlash(addr, flags);
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
	if (pFlash->type_ != kMtypFlash)
	{
		Trace() << "Address 0x" << f::X<4>(addr) << " is not erasable!\n";
		return true;	// silent acceptance
	}
	uint32_t memtop = pFlash->start_ + pFlash->size_;
	if (memtop < addr + size)
		Trace() << "Size of 0x" << f::X<4>(size) << " overflows memory segment!\n";
	FlashEraseFlags flags(chip_info_.has_locka_, false);
	flags.EraseSegment();
	while (addr < memtop)
	{
		Debug() << "Erasing 0x" << f::X<4>(addr) << "...\n";
		if (!EraseFlash(addr, flags))
			return false;
		addr += pFlash->segsize_;
	}
	return true;
}


/* Is there a more reliable way of doing this? */
int TapMcu::device_is_fram()
{
	return chip_info_.is_fram_;
}


void TapMcu::ShowDeviceType()
{
#if DEBUG
	static constexpr const char *clas[] =
	{
		"Boot:",
		"Boot2:",
		"BSL:",
		"BSL2:",
		"CPU:",
		"EEM:",
		"Info:",
		"IV:",
		"LCD:",
		"LeaPer:",
		"LeaRAM:",
		"ClasLib:",
		"Main:",
		"MidROM:",
		"Periph16:",
		"Periph16(1):",
		"Periph16(2):",
		"Periph16(3):",
		"Periph8:",
		"RAM:",
		"RAM2:",
		"TinyRAM:",
		"USBRAM:",
		"UssPer:",
	};
	static constexpr const char *mem_type[] =
	{
		"Flash",
		"RAM",
		"Regs",
		"ROM",
	};
	static constexpr const char *slau[] =
	{
		"SLAU049",
		"SLAU056",
		"SLAU144",
		"SLAU208",
		"SLAU259",
		"SLAU321",
		"SLAU335",
		"SLAU367",
		"SLAU378",
		"SLAU445",
		"SLAU506",
	};
	static constexpr const char *eem[] =
	{
		"LOW",
		"MEDIUM",
		"HIGH",
		"EXTRA_SMALL_5XX",
		"SMALL_5XX",
		"MEDIUM_5XX",
		"LARGE_5XX",
	};
	static_assert(_countof(clas) == ChipInfoDB::kMkeyLast_+1, "Array size does not match enumeration");
	static_assert(_countof(mem_type) == ChipInfoDB::kMtypLast_+1, "Array size does not match enumeration");
	static_assert(_countof(slau) == ChipInfoDB::kSlau_Last_+1, "Array size does not match enumeration");
	static_assert(_countof(eem) == ChipInfoDB::kEemUpper_ +1, "Array size does not match enumeration");
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
			<< "-0x" << f::X<4>(mem.start_ + mem.size_ - 1)
			<< '\t' << f::S<-8>(tmp)
			<< " [" << mem_type[mem.type_]
			;
		if (mem.banks_ > 1)
			msg << " - " << mem.banks_ << " banks";
		msg << "]\n";
	}
	msg << "Hardware breakpoints: " << chip_info_.num_breakpoints_ << '\n';
#else
	msg << "\nHardware breakpoints: " << chip_info_.num_breakpoints_ << '\n';
#endif
}


bool TapMcu::ProbeId()
{
	/* proceed with identification */
	uint8_t data[16];
	uint8_t tlv_data[1024];

	DieInfo id;
	memset(&id, 0xff, sizeof(id));

	if (!traits_->GetDeviceSignature(id, cpu_ctx_, core_id_))
	{
		Debug() << "Failed to identify chip\n";
		return false;
	}

	if(!chip_info_.Load(id))
	{
		Error() << "Unknown chip. Loading default profile for platform";
		traits_->InitDefaultChip(chip_info_);
	}

	Debug() << "Chip ID data:\n"
		"  mcu_ver:  " << f::X<4>(id.mcu_ver_) << "\n"
		"  mcu_sub:  " << f::X<4>(id.mcu_sub_) << "\n"
		"  mcu_rev:  " << f::X<2>(id.mcu_rev_) << "\n"
		"  mcu_fab:  " << f::X<2>(id.mcu_fab_) << "\n"
		"  mcu_self: " << f::X<4>(id.mcu_self_) << "\n"
		"  mcu_cfg:  " << f::X<2>(id.mcu_cfg_) << "\n"
		"  mcu_fuse: " << f::X<2>(id.mcu_fuse_) << "\n";

	ShowDeviceType();
	return true;
}


#if 0
int TapMcu::refresh_bps()
{
	int i;
	int ret;
	device_breakpoint *bp;
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
#endif


int TapMcu::OnSoftReset()
{
	ClearError();
	// perform soft reset
	return ExecutePOR();
}


int TapMcu::OnRun()
{
	ClearError();
	UpdateEemBreakpoints();
	traits_->ReleaseDevice(cpu_ctx_, chip_info_, true);
	return 0;
}


int TapMcu::OnSingleStep()
{
	ClearError();
	// execute next instruction at current Gpio::PC
	McuCore::Abort();	// TODO
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
	StopWatch().Delay<100>();

	if (GetCpuState() != 0)
		return DEVICE_STATUS_HALTED;
	return DEVICE_STATUS_RUNNING;
}

