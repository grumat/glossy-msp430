#include "stdproj.h"
#include "TapMcu.h"
#include "drivers/JtagDev.h"
#include "drivers/SbwDev.h"
#include "drivers/TargetPower.h"

#include "TapDev430X.h"
#include "TapDev430Xv2_1377.h"

#include "eem_defs.h"


using namespace ChipInfoDB;

JtagDev gJtagDevice;
#if OPT_INCLUDE_SBW_TIM_
SbwDev gSbwDevice;
#endif
TapMcu gTapMcu;

TapDev430 gMsp430Legacy;
TapDev430X gMsp430X;
TapDev430Xv2 gMsp430Xv2;
TapDev430Xv2_1377 gMsp430Xv2_1377;


bool TapMcu::SetTransport(Transport t)
{
#if !OPT_INCLUDE_SBW_TIM_
	if (t == Transport::kSbw)
		return false;		// SBW driver not compiled into this build
#endif
	transport_ = t;
	return true;
}


bool TapMcu::SelectActiveDriver()
{
	switch (transport_)
	{
#if OPT_INCLUDE_SBW_TIM_
	case Transport::kSbw:
		gPlayer.pItf = &gSbwDevice;
		return true;
#endif
	case Transport::kJtag:
		gPlayer.pItf = &gJtagDevice;
		return true;
	default:
		return false;		// requested transport not available
	}
}


bool TapMcu::Open()
{
	fAttached_ = false;
	chipInfo_.DefaultMcu();
	breakpoints.Clear();

	// UIF-style: a connect powers the target. If nothing is driving it and it
	// isn't externally powered, bring up the default supply and wait for the rail
	// to settle before energizing the bus. No-op on sense-only / fixed probes or
	// when power was already commanded (explicit `power`, prior scan).
	TargetPower::EnsurePowered();

	// Runtime transport pick (#4/#15/#46): transport_ is set by the GDB monitor
	// jtag_scan / sbw_scan commands, defaulting from the legacy compile-time lever
	// OPT_HARD_SELECT_SBW_TMP. SelectActiveDriver points gPlayer.pItf at the
	// matching driver, or fails if that transport is not compiled in.
	if (!SelectActiveDriver())
	{
		Error() << "transport not available in this build\n";
		return false;
	}
	pTraits_ = &gMsp430Legacy;
	fFailed = !gPlayer.pItf->OnOpen();

	if (fFailed)
	{
		Error() << "can't open port\n";
		return false;
	}

	if (InitDevice() == false)
	{
		Error() << "initialization failed\n";
		return false;
	}
	fAttached_ = true;
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
		if (gPlayer.Play(kIrDr16(Ir::kCntrlSigCapture, 0xAAAA)) == 0x5555)
			return true;	// Fuse is blown
	}
	return false;			// Fuse is not blown
}


bool TapMcu::StartMcu()
{
	switch (chipInfo_.arch)
	{
	case ChipInfoDB::kCpuXv2:
		pTraits_ = HasIssue1377() ? (ITapDev *)&gMsp430Xv2_1377 : (ITapDev *)&gMsp430Xv2;
		break;
	case ChipInfoDB::kCpuX:
		pTraits_ = &gMsp430X;
		break;
	default:
		pTraits_ = &gMsp430Legacy;
		break;
	}

	/* Perform PUC, includes target watchdog disable */
	if (ExecutePOR() == false)
	{
		Error() << "jtag_init: PUC failed\n";
		return false;
	}

	fFailed = false;
	return true;
}


bool TapMcu::InitDevice()
{
	// Bus speed for this connect: the grade selected via the monitor "speed"
	// command (defaults to kSlowest — safest for acquisition / long cables).
	const BusSpeed speed = speed_;
	Debug() << "Starting JTAG\n";
	fFailed = true;
	coreId_.Init();
	pTraits_ = &gMsp430Legacy;

	// Transport state cycle: Init -> (Open -> Close) -> Init.
	//   Open  = OnConnectJtag (bus actively driven for the entry attempt)
	//   Close = OnReleaseJtag  (buffers DRIVE the idle level — NOT Hi-Z)
	//   Init  = OnReleaseDriver (full Hi-Z release; only at OnClose(), see TapMcu::Close)
	// The retry loop stays in Open/Close and never tri-states between attempts, so
	// the STLinkV2's marginal target pull-ups/-downs cannot pulse the bus mid-loop.
	// see GetCoreID()
	size_t tries = 0;
	for (;;)
	{
		// === JTAG entry sequence for 430Xv2 (slau320aj §2.3.2.2.1, Figure 2-16) ===
		// TI's algorithm tries a NORMAL (RST-high) entry first and only falls back to the
		// "magic pattern" acquisition if that returns an INVALID JTAG-ID. The magic pattern
		// is NOT applied to a device that already answers with a valid ID — doing so was the
		// bug that broke CoreIP/hung the transport (issues #19/#20). FR5994 returns a valid
		// 0x99 on the normal entry, so it takes the fast path and never arms the mailbox.
		// The fallback itself is UNPROVEN and compiled out by default
		// (OPT_MSP430_MAGIC_PATTERN_ACQ, see stdproj.h).

		// --- Attempt 1: normal entry (RST high). "Connect → reset high → entry → reset TAP
		//     → instruction shift to get JTAG-ID" (Figure 2-16, first half). ---
		gPlayer.pItf->OnReleaseJtag();				// Stop JTAG - release JTAG/TEST signals
		// TODO: expose speed selection through the debug session configuration
		gPlayer.pItf->OnConnectJtag(speed);
		gPlayer.pItf->OnEnterTap(false);				// RST high through entry
		gPlayer.pItf->OnResetTap();					// TAP -> Run-Test/Idle
		coreId_.jtagId = (JtagId)(uint8_t)gPlayer.IR_Shift(Ir::kCntrlSigCapture);
		/*
		|  MCU   | jtagId |
		|--------|--------|
		| F1611  |  0x89  |
		| F5418A |  0x91  |
		*/
		if (coreId_.IsMSP430())
			break;								// valid ID on the normal entry -> done

#if OPT_MSP430_MAGIC_PATTERN_ACQ
		// UNPROVEN acquisition path: no validated bench case exercises this fallback
		// successfully. Gated behind OPT_MSP430_MAGIC_PATTERN_ACQ (default 0, see stdproj.h).
		//
		// MagicPattern (TI MagicPattern.c): the value written to the JTAG mailbox during
		// acquisition so the device's boot code stays halted under JTAG instead of running
		// on into LPM4. A blank device auto-enters LPM4 on reset (slau272d), which leaves
		// the device-descriptor / boot-ROM at 0x1A00 reading back as vacant 0x3FFF — issue #19.
		static constexpr uint16_t MAGIC_PATTERN = 0xA55A;

		// --- Attempt 2: MagicPattern fallback (Figure 2-16, RST-low branch). Reached only
		//     when the normal entry returned an invalid ID — a running/blank part that
		//     dropped into LPM4. Hold the device in reset, feed 0xA55A to the JTAG mailbox so
		//     the BootCode sends it to LPM4 (no user code runs), then re-enter with RST high.
		//     NOTE (#20): the RSTHIGH second open currently de-latches the single-pin SBW DR
		//     path on the in-bring-up TimSbw transport — it fails GRACEFULLY here (invalid ID
		//     -> retry/abort, no hang, because there is no mid-sequence OnConnectJtag/ApplySpeed
		//     UG), and will be revisited when the transport matures. ---
		gPlayer.pItf->OnReleaseJtag();
		gPlayer.pItf->OnConnectJtag(speed);
		gPlayer.pItf->OnEnterTap(true);			// RST low - device held in reset
		gPlayer.pItf->OnResetTap();
		gPlayer.i_WriteJmbIn16(MAGIC_PATTERN);		// best-effort (returns false on timeout)
		gPlayer.pItf->OnEnterTap(false);			// re-enter, RST high
		gPlayer.pItf->OnResetTap();
		coreId_.jtagId = (JtagId)(uint8_t)gPlayer.IR_Shift(Ir::kCntrlSigCapture);
		// break if a valid JTAG ID is being returned
		if (coreId_.IsMSP430())
			break;
#endif // OPT_MSP430_MAGIC_PATTERN_ACQ

		// --- MSP430i20xx (jtag_id 0x89) acquisition is NOT handled here yet (#43).
		//     That family needs a BSL Entry Sequence (RST/TEST pulse train -> LPM4)
		//     + ~500 ms settle + a SPYBIWIREJTAG/SBW open BEFORE the TAP answers —
		//     an additive preamble, not a tweak of OnEnterTap. See
		//     .claude/docs/msp430/I2031_ACQUISITION_GOLDEN_REFERENCE.md. The earlier
		//     "slow_settle" attempt was the wrong fix and was reverted. ---

		// Stop on errors
		if (++tries == kMaxEntryTry)
		{
			Error() << "jtag_init: no device found\n";
			coreId_.jtagId = kInvalid;
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
	pTraits_ = coreId_.IsXv2() ? (ITapDev *)&gMsp430Xv2 : (ITapDev *)&gMsp430Legacy;
	// Capture device into JTAG mode
	if (!pTraits_->GetDevice(coreId_))
	{
		Error() << "jtag_init: invalid jtag_id: 0x" << f::X<2>(coreId_.jtagId) << '\n';
		coreId_.Init();
		return false;
	}
	// note: coreId_.deviceId is the **BSL device ID** and is not touched by Xv2 GetDevice()!
	Debug() << "JTAG identify path:\n"
		"  jtag_id:     0x" << f::X<2>(coreId_.jtagId) << "\n"
		"  coreip_id:   0x" << f::X<4>(coreId_.coreipId) << "\n"
		"  device_id:   0x" << f::X<4>(coreId_.deviceId) << "\n"
		"  id_data_addr:0x" << f::X<4>(coreId_.idDataAddr) << "\n";
	// Forward detect CPUX devices, before database lookup
	// Hint: <coreId_.coreipId == 0> is equivalent to <!coreId_.IsXv2()> in this context
	if (coreId_.coreipId == 0 && ChipProfile::IsCpuX_ID(coreId_.deviceId))
		pTraits_ = &gMsp430X;

	cpuCtx_.pc = 0xFFFE;	// reset vector
	cpuCtx_.jtagId = coreId_.jtagId;
	// Empty CPU profile will set a default part for initialization
	chipInfo_.Init();
	pTraits_->InitDefaultChip(chipInfo_, coreId_.jtagId);
	// Take the CPU into Full-Emulation-State: assert the control-bit POR and park the PC at
	// SAFE_PC_ADDRESS (JMP $). For FRAM parts (jtag_id 0x91/0x99) this also initializes the
	// test-memory at 0x06/0x08 so the prefetched PC/MAB stays consistent (slau320aj
	// §2.3.2.2.3). The device ID is then read afterwards (Figure 2-15: Read device ID).
	pTraits_->SyncJtagAssertPorSaveContext(cpuCtx_, chipInfo_);

	if (ProbeId() == false
		|| StartMcu() == false)
	{
		Close();
		return false;
	}
	// Deferred until after the PC-park -> descriptor-read gap: SwoChannel::PutChar busy-waits
	// on ITM FIFO backpressure (bmt/include/trace.h), so a trace emitted between
	// SyncJtagAssertPorSaveContext (parks the PC at SAFE_PC_ADDRESS) and ProbeId() (reads the
	// device descriptor off that parked state) can stall long enough to desync the read and
	// misidentify the chip as vacant/DefaultChip — a probe effect reproduced under Debug builds.
	Debug() << "Saved WDTCTL low byte: 0x" << f::X<2>(cpuCtx_.wdt) << '\n';
	Trace() << "JTAG ID: 0x" << f::X<2>(coreId_.jtagId) << '\n';
	__NOP();
	return true;
}


void TapMcu::Close()
{
	ClearError();

	if(fAttached_)
	{
		pTraits_->ReleaseDevice(cpuCtx_, chipInfo_, false);
		fAttached_ = false;
	}
	SetLedState(LedState::green);
	gPlayer.pItf->OnClose();
	pTraits_ = &gMsp430Legacy;
	ClearBrk();
}


uint32_t TapMcu::OnGetReg(int reg)
{
	ClearError();

	// Cached registers
	if (reg == 0)
		return cpuCtx_.pc;
	if (reg == 2)
		return cpuCtx_.sr;
	// All others
	uint32_t v = UINT32_MAX;
	if (pTraits_->GetRegs_Begin())
	{
		// read register
		v = pTraits_->GetReg(reg);
		pTraits_->GetRegs_End();
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
		cpuCtx_.pc = value;
		return pTraits_->SetPC(value);
	}
	else if (reg == 2)
		cpuCtx_.sr = value;
	return pTraits_->SetReg(reg, value);
}


bool TapMcu::OnGetRegs(address_t *regs)
{
	int i;

	ClearError();

	if(pTraits_->GetRegs_Begin())
	{
		for (i = 0; i < DEVICE_NUM_REGS; i++)
		{
			uint32_t r;
			if (i == 0)
				r = cpuCtx_.pc;
			else if (i == 2)
				r = cpuCtx_.sr;
			else
				r = pTraits_->GetReg(i);
			regs[i] = r;
		}
	}
	pTraits_->GetRegs_End();
	return true;
}


int TapMcu::OnSetRegs(address_t *regs)
{
	ClearError();

	if (!pTraits_->SetPC(regs[0]))
		return UINT32_MAX;
	for (int i = 1; i < DEVICE_NUM_REGS; i++)
	{
		if(!pTraits_->SetReg(i, regs[i]))
			return UINT32_MAX;
	}
	return 0;
}


void TapMcu::OnReadBytes(address_t address, void *data, address_t byte_count)
{
	ClearError();

	if (byte_count == 1)
		*(uint16_t *)data = pTraits_->ReadByte(address);
	else 
		pTraits_->ReadBytes(address, (uint8_t *)data, byte_count);
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
		*(uint16_t *)data = pTraits_->ReadWord(address);
	else
		pTraits_->ReadWords(address, (uint16_t *)data, word_count);
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
	const MemInfo *m = chipInfo_.FindMemByAddress(addr);

	if (m)
	{
		if (addr + size > m->start + m->size)
		{
			size = m->start + m->size - addr;
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
	if (!fAttached_)
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
		else if (m->bitSize > 8)
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
	if (m->type != ChipInfoDB::kMtypFlash)
		pTraits_->WriteWords(addr, (const unaligned_u16 *)data_, wordcount);
	else
		pTraits_->WriteFlash(addr, (const unaligned_u16 *)data_, wordcount);
}


bool TapMcu::WriteMem(address_t addr, const void *mem_, address_t len)
{
	if (!fAttached_)
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

	const MemInfo &flash = chipInfo_.GetMainMem();
	if (flash.type != kMtypFlash)
	{
		Trace() << "Main memory is not erasable!\n";
		return true;	// silent acceptance
	}

	FlashEraseFlags flags(chipInfo_.fHasLocka, false);
	flags.MainErase(chipInfo_.fHasGmeras);
	return EraseFlash(flash.start, flags, kMassErase);
}


bool TapMcu::EraseAll()
{
	Trace() << "Erasing Main+INFO memories";
	ClearError();

	const MemInfo &flash = chipInfo_.GetMainMem();
	if (flash.type != kMtypFlash)
	{
		Trace() << "Main memory is not erasable!\n";
		return true;	// silent acceptance
	}
	
	FlashEraseFlags flags(chipInfo_.fHasLocka, false);
	FlashEraseFlags seg(flags);
	
	flags.MassErase(chipInfo_.fHasGmeras);

	// Do erase flash memory
	if (!EraseFlash(flash.start, flags, kMassErase))
		return false;
	// Newer families require explicit INFO memory erase
	if (!chipInfo_.fHas1pMassErase)
	{
		seg.EraseSegment();
		// INFO Memory needs to be cleared separately
		const MemInfo &info = chipInfo_.GetInfoMem();
		// Protect INFOA in SLAU144 as it contains factory default calibration values (TLV record)
		const int banks = info.banks - chipInfo_.fTlvClash;
		uint32_t addr = info.start;
		for (int i = 0; i < banks; ++i)
		{
			if (!EraseFlash(addr, seg))
				return false;
			addr += info.segsize;
		}
	}
	return true;
}


bool TapMcu::EraseInfoA()
{
	Trace() << "Erasing INFOA memory";
	ClearError();

	// Chip does not locks INFOA segment
	if (chipInfo_.fHasLocka == false)
		return false;
	// Info memory
	const MemInfo &info = chipInfo_.GetInfoMem();
	// Last segment is INFO A
	address_t addr = info.start + info.size - info.segsize;
	// Option to unlock Info A
	FlashEraseFlags flags(true, true);
	flags.EraseSegment();
	// Clear INFO A
	return EraseFlash(addr, flags);
}


bool TapMcu::EraseSegment(address_t addr)
{
	ClearError();

	const MemInfo *pFlash = chipInfo_.FindMemByAddress(addr);
	if (pFlash == NULL)
	{
		Error() << "Address 0x" << f::X<4>(addr) << " not found in device memory map!\n";
		return false;
	}
	if (pFlash->type != kMtypFlash)
	{
		Trace() << "Address 0x" << f::X<4>(addr) << " is not erasable!\n";
		return true;	// silent acceptance
	}
	
	Debug() << "Erasing 0x" << f::X<4>(addr) << "...\n";
	FlashEraseFlags flags(chipInfo_.fHasLocka, false);
	flags.EraseSegment();
	return EraseFlash(addr, flags);
}


bool TapMcu::EraseRange(address_t addr, address_t size)
{
	ClearError();

	const MemInfo *pFlash = chipInfo_.FindMemByAddress(addr);
	if (pFlash == NULL)
	{
		Error() << "Address 0x" << f::X<4>(addr) << " not found in device memory map!\n";
		return false;
	}
	if (pFlash->type != kMtypFlash)
	{
		Trace() << "Address 0x" << f::X<4>(addr) << " is not erasable!\n";
		return true;	// silent acceptance
	}
	uint32_t memtop = pFlash->start + pFlash->size;
	if (memtop < addr + size)
		Trace() << "Size of 0x" << f::X<4>(size) << " overflows memory segment!\n";
	FlashEraseFlags flags(chipInfo_.fHasLocka, false);
	flags.EraseSegment();
	while (addr < memtop)
	{
		Debug() << "Erasing 0x" << f::X<4>(addr) << "...\n";
		if (!EraseFlash(addr, flags))
			return false;
		addr += pFlash->segsize;
	}
	return true;
}


// Enum-to-name lookup tables for PrintChipInfo. These live here (not in the
// header template) as the single source of truth, validated against the DB
// enumerations by the adjacent static_asserts. The DB enums are unscoped, so the
// values index the arrays directly (as the original inline code did).
const char *TapMcu::MemClassName(ChipInfoDB::EnumMemoryKey v)
{
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
	static_assert(_countof(clas) == ChipInfoDB::kMkeyLast_+1, "Array size does not match enumeration");
	return ((size_t)v < _countof(clas)) ? clas[v] : "?:";
}

const char *TapMcu::MemTypeName(ChipInfoDB::EnumMemoryType v)
{
	static constexpr const char *mem_type[] =
	{
		"Flash",
		"RAM",
		"Regs",
		"ROM",
	};
	static_assert(_countof(mem_type) == ChipInfoDB::kMtypLast_+1, "Array size does not match enumeration");
	return ((size_t)v < _countof(mem_type)) ? mem_type[v] : "?";
}

const char *TapMcu::SlauName(ChipInfoDB::EnumSlau v)
{
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
	static_assert(_countof(slau) == ChipInfoDB::kSlau_Last_+1, "Array size does not match enumeration");
	return ((size_t)v < _countof(slau)) ? slau[v] : "SLAU???";
}

const char *TapMcu::EemName(ChipInfoDB::EnumEemType v)
{
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
	static_assert(_countof(eem) == ChipInfoDB::kEemUpper_ +1, "Array size does not match enumeration");
	return ((size_t)v < _countof(eem)) ? eem[v] : "?";
}


void TapMcu::ShowDeviceType()
{
	// SWO trace dump. Release builds stay terse (summary line); debug builds add
	// the full memory map. The GDB monitor path (chipinfo) requests the full map
	// regardless of build via PrintChipInfo(..., true).
	Trace msg;
#if DEBUG
	PrintChipInfo(msg, /*full=*/true);
#else
	PrintChipInfo(msg, /*full=*/false);
#endif
}


/* Proceed with identification */
bool TapMcu::ProbeId()
{
	DieInfo id;
	memset(&id, 0xff, sizeof(id));

	if (!pTraits_->GetDeviceSignature(id, cpuCtx_, coreId_))
	{
		Debug() << "Failed to identify chip\n";
		return false;
	}

	if(!chipInfo_.Load(id))
	{
		Error() << "Unknown chip. Loading default profile for platform\n";
		pTraits_->InitDefaultChip(chipInfo_, coreId_.jtagId);
	}
	Debug() << "Selected chip/profile: " << chipInfo_.szName << '\n';

	Debug() << "Chip ID data:\n"
		"  mcu_ver:  " << f::X<4>(id.mcuVer) << "\n"
		"  mcu_sub:  " << f::X<4>(id.mcuSub) << "\n"
		"  mcu_rev:  " << f::X<2>(id.mcuRev) << "\n"
		"  mcu_fab:  " << f::X<2>(id.mcuFab) << "\n"
		"  mcu_self: " << f::X<4>(id.mcuSelf) << "\n"
		"  mcu_cfg:  " << f::X<2>(id.mcuCfg) << "\n"
		"  mcu_fuse: " << f::X<2>(id.mcuFuse) << "\n";

	ShowDeviceType();
	return true;
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
	UpdateEemBreakpoints();
	pTraits_->ReleaseDevice(cpuCtx_, chipInfo_, true);
	return 0;
}


int TapMcu::OnSingleStep()
{
	ClearError();
	// execute next instruction at current PC
	return pTraits_->SingleStep(cpuCtx_, chipInfo_) ? 0 : -1;
}


bool TapMcu::OnHalt()
{
	ClearError();
	// take device under JTAG control
	return pTraits_->GetDevice(coreId_)
		&& pTraits_->SyncJtagConditionalSaveContext(cpuCtx_, chipInfo_);
}


bool TapMcu::GetCpuState()
{
	gPlayer.pItf->OnIrShift(Ir::kEmexReadControl);

	if ((gPlayer.pItf->OnDrShift16(0x0000) & 0x0080) == 0x0080)
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
	StopWatch().Delay<Timer::Msec(100)>();

	if (GetCpuState() != 0)
		return DEVICE_STATUS_HALTED;
	return DEVICE_STATUS_RUNNING;
}
