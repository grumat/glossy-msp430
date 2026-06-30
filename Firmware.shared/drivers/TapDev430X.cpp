#include "stdproj.h"

#include "TapDev430X.h"
#include "eem_defs.h"
#include "TapMcu.h"


/**************************************************************************************/
/* MCU VERSION-RELATED POWER ON RESET                                                 */
/**************************************************************************************/

void TapDev430X::InitDefaultChip(ChipProfile &prof, JtagId jtag_id)
{
	(void)jtag_id;	// CPUX parts have a single default profile
	prof.DefaultMcuX();
}


// Source UIF
bool TapDev430X::SyncJtagAssertPorSaveContext(CpuContext &ctx, const ChipProfile &prof)
{
	constexpr uint16_t address = kWdtAddrCpu;
	CtrlSigReg ctl_sync = CtrlSigReg::kNone;

	ctx.is_running_ = false;

	// Sync the JTAG
	if (gPlayer.SetJtagRunReadLegacy() != kMspStd)
		return false;

	uint32_t lOut = gPlayer.GetCtrlSigReg();    // read control register once

	gPlayer.SetTCLK();

	if (IsReset(static_cast<CtrlSigReg>(lOut), CtrlSigReg::kTce))
	{
		// If the JTAG and CPU are not already synchronized ...
		// Initiate Jtag and CPU synchronization. Read/Write is under CPU control. Source TCLK via TDI.
		static constexpr TapStep steps_01[] =
		{
			// Do not effect bits used by DTC (CPU_HALT, MCLKON).
			kIrDr8(Ir::kCntrlSigHighByte, E2I(CtrlSigReg::kTagFuncSat | CtrlSigReg::kTce1 | CtrlSigReg::kCpu) >> 8),
			// Address Force Sync special handling
			// read access to EEM General Clock Control Register (GCLKCTRL)
			kIrDr32(Ir::kEmexDataExchange32, kGenClkCtrl + kMxRead),
			// read the content of GCLKCNTRL into lOut
			kDr32_ret(0),
		};

		gPlayer.Play(steps_01, _countof(steps_01), &lOut);
		// Set Force Jtag Synchronization bit in Emex General Clock Control register.
		lOut |= 0x0040;							// 0x0040 = FORCE_SYN

		// Stability improvement: should be possible to remove this, required only once at the beginning
		// write access to EEM General Clock Control Register (GCLKCTRL)
		gPlayer.PlayAsync(kIrDr32(Ir::kEmexDataExchange32, kGenClkCtrl + kMxWrite));	// write-only; next DR_Shift32 drains
		lOut = gPlayer.DR_Shift32(lOut);		// write into GCLKCNTRL

		// Reset Force Jtag Synchronization bit in Emex General Clock Control register.
		lOut &= ~0x0040;
		// Stability improvement: should be possible to remove this, required only once at the beginning
		// write access to EEM General Clock Control Register (GCLKCTRL)
		gPlayer.PlayAsync(kIrDr32(Ir::kEmexDataExchange32, kGenClkCtrl + kMxWrite));	// write-only; next DR_Shift32 drains
		lOut = gPlayer.DR_Shift32(lOut);		// write into GCLKCNTRL

		ctl_sync = SyncJtag();

		if (!IsSet(ctl_sync, CtrlSigReg::kCpuHalt))
		{
			return false;	// Synchronization failed!
		}
		gPlayer.ClrTCLK();
		gPlayer.PlayAsync(kIrDr16(Ir::kCntrlSig16Bit, 0x2401));	// write-only; SetTCLK drains
		gPlayer.SetTCLK();
	}
	else
	{
		gPlayer.PlayAsync(kIrDr16(Ir::kCntrlSig16Bit, 0x2401));	// write-only; next shift drains
	}

	// execute a dummy instruction here
	static constexpr TapStep steps_02[] =
	{
		kIrData16(kdTclk1, 0x4303, kdTclk0),	// kIr(Ir::kData16Bit) + kTclk1 + kDr16(0x4303 = NOP) + kTclk
		kIr(Ir::kDataCapture, kdTclk1),			// kIr(Ir::kDataCapture) + kTclk1
	};
	gPlayer.Play(steps_02, _countof(steps_02));

	// step until next instruction load boundary if not being already there
	if (!IsInstrLoad())
		return false;

	if (prof.clk_ctrl_ == ChipInfoDB::kGccExtended)
	{
		static constexpr TapStep steps[] =
		{
			// Perform the POR
			kIrDr32(Ir::kEmexDataExchange32, kGenCtrl + kMxWrite),	// write access to EEM General Control Register (kGenCtrl)
			kDr32(kEmuFeatEn | kEmuClkEn | kClearStop | kEemEn),		// write into kGenCtrl
			// Stability improvement: should be possible to remove this, required only once at the beginning
			kIrDr32(Ir::kEmexDataExchange32, kGenCtrl + kMxWrite),	// write access to EEM General Control Register (kGenCtrl)
			kDr32(kEmuFeatEn | kEmuClkEn),							// write into kGenCtrl
		};
		gPlayer.Play(steps, _countof(steps));
	}
	
	constexpr CtrlSigReg common = CtrlSigReg::kRead | CtrlSigReg::kTce1 | CtrlSigReg::kTagFuncSat;

	static constexpr TapStep steps_03[] =
	{
		kTclk0,
		// Assert PUC
		kIrDr16(Ir::kCntrlSig16Bit, E2I(common | CtrlSigReg::kPOR)),
		kTclk1,
		// Negate PUC
		kIrDr16(Ir::kCntrlSig16Bit, E2I(common)),

		kTclk0,
		// Assert PUC
		kIrDr16(Ir::kCntrlSig16Bit, E2I(common | CtrlSigReg::kPOR)),
		kTclk1,
		// Negate PUC
		kIrDr16(Ir::kCntrlSig16Bit, E2I(common)),

		// Explicitly set TMR
		kDr16(E2I(CtrlSigReg::kRead | CtrlSigReg::kTce1)), // Enable access to Flash registers

		kIrDr16(Ir::kFlash16BitUpdate, kFlashSesel1),	// Disable flash test mode
		kDr16(kFlashSesel1 | kFlashTmr),				// Pulse TMR
		kDr16(kFlashSesel1),
		kDr16(kFlashSesel1 | kFlashTmr),				// Set TMR to user mode

		// Disable access to Flash register
		kIrDr8(Ir::kCntrlSigHighByte, E2I(CtrlSigReg::kTagFuncSat | CtrlSigReg::kTce1) >> 8),
	};
	gPlayer.Play(steps_03, _countof(steps_03));

	// step until an appropriate instruction load boundary
	uint32_t i = 10;
	while (true)
	{
		lOut = gPlayer.Play(kIrDr20(Ir::kAddrCapture, 0x0000)) & 0xffff;
		if (lOut == 0xFFFE || lOut == 0x0F00)
			break;
		if (i == 0)
			return false;
		--i;
		gPlayer.PulseTCLKN();
	}

	static constexpr TapStep steps_04[] =
	{
		kPulseTclkN,
		kPulseTclkN,

		kTclk0,
		kIrDr16(Ir::kAddrCapture, 0x0000),
		kTclk1,
	};
	gPlayer.Play(steps_04, _countof(steps_04));

	// step until next instruction load boundary if not being already there
	if (!IsInstrLoad())
		return false;

	// Hold Watchdog
	ctx.wdt_ = ReadWord(address);	// safe WDT value
	uint16_t wdtval = kWdtHold | ctx.wdt_;	// set original bits in addition to stop bit
	WriteWord(address, wdtval);

	// read MAB = PC here
	ctx.pc_ = gPlayer.Play(kIrDr20(Ir::kAddrCapture, 0));

	// set PC to a save address pointing to ROM to avoid RAM corruption on certain devices
	SetPC(kRomAddr);

	// read status register
	ctx.sr_ = GetReg(2);

	return true;
}


// Source UIF
bool TapDev430X::SyncJtagConditionalSaveContext(CpuContext &ctx, const ChipProfile &prof)
{
	constexpr uint16_t address = kWdtAddrCpu;
	CtrlSigReg ctl_sync = CtrlSigReg::kNone;
	uint16_t statusReg = 0;

	// syncWithRunVarAddress
	ctx.is_running_ = false;

	// Stability improvement: should be possible to remove this here, default state of TCLK should 
	// be one
	gPlayer.SetTCLK();

	if (IsReset(GetCtrlSigReg(), CtrlSigReg::kTce))
	{
		// If the JTAG and CPU are not already synchronized ...
		// Initiate JTAG and CPU synchronization. Read/Write is under CPU control. Source TCLK 
		// via TDI.
		// Do not effect bits used by DTC (CPU_HALT, MCLKON).
		gPlayer.PlayAsync(kIrDr8(Ir::kCntrlSigHighByte, E2I(CtrlSigReg::kTagFuncSat | CtrlSigReg::kTce1 | CtrlSigReg::kCpu) >> 8));	// write-only; SyncJtag's shift drains

		// TCE eventually set indicates synchronization (and clocking via TCLK).
		ctl_sync = SyncJtag();
		if (ctl_sync == CtrlSigReg::kNone)
			return false;	// Synchronization failed!
	}// end of if(!(lOut & CNTRL_SIG_TCE))

	const bool cpu_halted = IsSet(ctl_sync, CtrlSigReg::kCpuHalt);
	if (cpu_halted)
		gPlayer.ClrTCLK();
	// Clear HALT. Read/Write is under CPU control. As a precaution, disable interrupts.
	gPlayer.PlayAsync(kIrDr16(Ir::kCntrlSig16Bit, E2I(CtrlSigReg::kTagFuncSat | CtrlSigReg::kTce1 | CtrlSigReg::kCpu)));	// write-only; SetTCLK drains
	if (cpu_halted)
		gPlayer.SetTCLK();

	// step until next instruction load boundary if not being already there
	if (!IsInstrLoad())
		return false;

	// read MAB = PC here
	ctx.pc_ = gPlayer.Play(kIrDr20(Ir::kAddrCapture, 0));

	// DLLv2: Check if a breakpoint was hit
	// \todo Determine if this is needed

	if (prof.clk_ctrl_ == ChipInfoDB::kGccExtended)
	{
		static constexpr TapStep steps[] =
		{
			// disable EEM and clear stop reaction
			kIrDr16(Ir::kEmexWriteControl, kClearStop | kEemEn),
			kDr16(0x0000),

			// write access to EEM General Control Register (kGenCtrl)
			kIrDr32(Ir::kEmexDataExchange32, kGenCtrl + kMxWrite),
			// write into kGenCtrl
			kDr32(kEmuFeatEn | kEmuClkEn | kClearStop | kEemEn),

			// Stability improvement: should be possible to remove this, required only once at the beginning
			// write access to EEM General Control Register (kGenCtrl)
			kIrDr32(Ir::kEmexDataExchange32, kGenCtrl + kMxWrite),
			// write into kGenCtrl
			kDr32(kEmuFeatEn | kEmuClkEn),

			kIrData16(kdTclk1, 0x4303, kdTclk0),	// kIr(Ir::kData16Bit) + kTclk1 + kDr16(0x4303) + kTclk
			kIr(Ir::kDataCapture, kdTclk1),			// kIr(Ir::kDataCapture) + kTclk1
		};
		gPlayer.Play(steps, _countof(steps));
	}
	else
	{
		static constexpr TapStep steps[] =
		{
			// disable EEM and clear stop reaction
			kIrDr16(Ir::kEmexWriteControl, kClearStop | kEemEn),
			kDr16(0x0000),

			kIrData16(kdTclk1, 0x4303, kdTclk0),	// kIr(Ir::kData16Bit) + kTclk1 + kDr16(0x4303) + kTclk
			kIr(Ir::kDataCapture, kdTclk1),			// kIr(Ir::kDataCapture) + kTclk1
		};
		gPlayer.Play(steps, _countof(steps));
	}

	// Advance to an instruction load boundary if an interrupt is detected.
	// The CPUOff bit will be cleared.
	// Basically, if there is an interrupt pending, the above dummy instruction
	// will have initiated its processing, and the CPU will not be on an
	// instruction load boundary following the dummy instruction.

	uint16_t i = 0;
	gPlayer.IR_Shift(Ir::kCntrlSigCapture);
	while (IsReset(ShiftCtrlSigReg(), CtrlSigReg::kInstrLoad))
	{
		if (!ClkTclkAndCheckDTC())
			return false;
		gPlayer.DR_Shift16(0);
		// give up depending on retry counter
		if (++i == MAX_TCE1)
			return false;
		gPlayer.IR_Shift(Ir::kCntrlSigCapture);
	}

	// Read PC now!!! Only the NOP or BIS #0,R4 instruction above was clocked into the device
	// The PC value should now be (OriginalValue + 2)
	// read MAB = PC here
	ctx.pc_ = (gPlayer.Play(kIrDr20(Ir::kAddrCapture, 0)) - 2) & 0xFFFFF;

	if (i == 0)
	{
		// An Interrupt was not detected
		//		lOut does not contain the content of the CNTRL_SIG register anymore at this point
		//		need to capture it again...different to DLLv3 sequence but don't expect any negative 
		//		effect due to recapturing
		ctx.in_interrupt_ = false;

		if (IsSet(GetCtrlSigReg(), CtrlSigReg::kCpuOff))
		{
			ctx.pc_ = (ctx.pc_ + 2) & 0xFFFFF;
			constexpr uint16_t kBicImm0R2 = 0xC032;
			gPlayer.Play(kIrData16(kdTclk1, kBicImm0R2));
			if (!ClkTclkAndCheckDTC())
				return false;
			constexpr uint16_t kBraPcP = 0x0010;
			// clear carry flag
			gPlayer.Play(kIrData16(kdTclk1, kBraPcP));
			if (!ClkTclkAndCheckDTC())
				return false;
			// DLLv2 preserve the CPUOff bit
			statusReg |= kStatusRegCpuOff;
		}
	}
	else
	{
		ctx.pc_ = gPlayer.Play(kIrDr20(Ir::kAddrCapture, 0));
		ctx.in_interrupt_ = true;
	}

	// DLL v2: deviceHasDTCBug
	//     Configure the DTC so that it transfers after the present CPU instruction is complete (ADC10FETCH).
	//     Save and clear ADC10CTL0 and ADC10CTL1 to switch off DTC (Note: Order matters!).

	// Regain control of the CPU. Read/Write will be set, and source TCLK via TDI.
	gPlayer.PlayAsync(kIrDr16(Ir::kCntrlSig16Bit, E2I(CtrlSigReg::kTce1 | CtrlSigReg::kRead | CtrlSigReg::kTagFuncSat)));	// write-only; InstrLoad's shift drains

	// Test if we are on an instruction load boundary
	if (!InstrLoad())
		return false;

	// Hold Watchdog
	uint16_t wdtval = ctx.wdt_ | kWdtPasswd;
	ctx.wdt_ = (uint8_t)TapDev430X::ReadWord(address);	// save WDT value
	wdtval |= ctx.wdt_;									// adds the WDT stop bit
	TapDev430X::WriteWord(address, wdtval);

	// set PC to a save address pointing to ROM to avoid RAM corruption on certain devices
	SetPC(kRomAddr);

	// read status register
	ctx.sr_ = GetReg(2) | statusReg;	// combine with preserved CPUOFF bit setting

	return true;
}


// Source: slau320aj
bool TapDev430X::ExecutePOR()
{
	// Perform Reset
	uint8_t jtag_ver;
	static constexpr TapStep steps[] =
	{
		kIrDr16(Ir::kCntrlSig16Bit, 0x2C01), // Apply Reset
		kDr16(0x2401), // Remove Reset
		kPulseTclkN, // F2xxx
		kPulseTclkN,
		kTclk0,
		kIrRet(Ir::kAddrCapture), // returns the jtag ID
		kTclk1,
	};
	gPlayer.Play(steps,
		_countof(steps),
		&jtag_ver);

	TapDev430X::WriteWord(0x0120, 0x5A80); // Disable Watchdog on target device

	if (jtag_ver != kMspStd)
		return false;
	return true;
}


void TapDev430X::ReleaseDevice(CpuContext &ctx, const ChipProfile &prof, bool run_to_bkpt, uint16_t mdbval)
{
	// Restore status register
	SetReg(2, ctx.sr_);
	// Restore Watchdog Control register
	WriteWord(kWdtAddrCpu, ctx.wdt_);
	// Restore Program Counter
	SetPC(ctx.pc_);
	
	if (prof.clk_ctrl_ == ChipInfoDB::kGccExtended)
	{
		static constexpr TapStep steps[] =
		{
			kIrDr32(Ir::kEmexDataExchange32, kGenCtrl + kMxWrite),
			kDr32(kEmuFeatEn | kEmuClkEn | kClearStop | kEemEn),
		};
		gPlayer.Play(steps,
			_countof(steps));
	}
	// Activate EEM
	gPlayer.Play(run_to_bkpt 
		? kIrDr16(Ir::kEmexWriteControl, 0x0007)
		: kIrDr16(Ir::kEmexWriteControl, 0x0006)
		);
	// Pre-initialize MDB before release if
	if (mdbval != kSwBkpInstr)
	{
		static constexpr TapStep steps[] =
		{
			kIrDr16Argv(Ir::kData16Bit, kdTclk0),
			kIr(Ir::kAddrCapture, kdTclk1),
		};
		gPlayer.Play(steps,
			_countof(steps),
			mdbval);
	}
	else
		gPlayer.IR_Shift(Ir::kAddrCapture);
	// Release target device from JTAG control
	gPlayer.IR_Shift(Ir::kCntrlSigRelease);
	ctx.is_running_ = true;
}



/**************************************************************************************/
/* MCU VERSION-RELATED REGISTER GET/SET METHODS                                       */
/**************************************************************************************/

//----------------------------------------------------------------------------
//! \brief Load a given address into the target CPU's program counter (PC).
//! \param[in] word address (destination address)
//! Source: slau320aj
bool TapDev430X::SetPC(address_t address)
{
	static constexpr TapStep steps[] =
	{
		// Load PC with address
		kIrDr16(Ir::kCntrlSigHighByte, 0x34), // CPU has control of RW & BYTE.
		kIrDr16Argv(Ir::kData16Bit,				// "mova #addr20,PC" instruction
			kdTclkN),		// F2xxx
		kDr16Argv,								// second word of "mova #addr20,PC" instruction
		kIr(kdTclkN,		// F2xxx 
			Ir::kAddrCapture, kdTclk0),			// Now the PC should be on address
		kIrDr16(Ir::kCntrlSigHighByte, 0x24),	// JTAG has control; WORD + RD.
		kTclk1,
	};
	gPlayer.Play(steps, _countof(steps),
		(uint16_t)(0x0080 | (((address) >> 8) & 0x0F00)),
		(uint16_t)address
	);
	return true;
}


// Source: uif
bool TapDev430X::SetReg(uint8_t reg, uint32_t value)
{
	uint16_t op = 0x0080 | (uint16_t)reg | ((value >> 8) & 0x0F00);
	static constexpr TapStep steps[] =
	{
		kIrDr8(Ir::kCntrlSigHighByte, 0x34),
		kIrData16Argv(kdTclk1, kdTclk0),		// kIr(Ir::kData16Bit) + kTclk1 + kDr16(op) + kTclk0
		kIr(Ir::kDataCapture, kdTclk1),
		kIrData16Argv(kdTclk1, kdTclk0),		// kIr(Ir::kData16Bit) + kTclk1 + kDr16(value) + kTclk0
		kIr(Ir::kDataCapture, kdTclk1),
		kIrData16(kdTclk1, 0x3ffd, kdTclk0),	// kIr(Ir::kData16Bit) + kTclk1 + kDr16('jmp $-4') + kTclk0
		kIr(Ir::kDataCapture, kdTclkP),			// kIr(Ir::kDataCapture) + kPulseTclk
		kIrDr8(Ir::kCntrlSigHighByte, 0x24),
		kTclk1,
	};
	gPlayer.Play(steps, _countof(steps),
		op,
		value
	);
	return true;
}


// Source: uif
uint32_t TapDev430X::GetReg(uint8_t reg)
{
	static constexpr TapStep steps[] =
	{
		kIrDr8(Ir::kCntrlSigHighByte, 0x34),
		kIrData16Argv(kdTclk1, kdTclk0),		// kIr(Ir::kData16Bit) + kTclk1 + kDr16(op) + kTclk0
		kIr(Ir::kDataCapture, kdTclk1),			// kIr(Ir::kDataCapture) + kTclk1
		// address part of "mova rX, &00fc"
		kIrData16(kdTclk1, 0x00fc, kdTclk0),	// kIr(Ir::kData16Bit) + kTclk1 + kDr16(0x00fc) + kTclk0
		kIr(Ir::kDataCapture, kdTclk1),			// kIr(Ir::kDataCapture) + kTclk1
		kDr16_ret(0),			// rx_l = dr16(0);
		kPulseTclkN,
		kDr16_ret(0),			// rx_h = dr16(0);
		kTclk0,
		kIrDr8(Ir::kCntrlSigHighByte, 0x24),
		kTclk1,
	};
	uint16_t rx_l = 0xFFFF;
	uint16_t rx_h = 0xFFFF;
	gPlayer.Play(steps, _countof(steps),
		(((reg << 8) & 0x0F00) | 0x60),			// equivalent to "mova rX, &00fc", 
		&rx_l,
		&rx_h
	);
	return ((uint32_t)rx_h << 16) | rx_l;
}



/**************************************************************************************/
/* MCU VERSION-RELATED READ MEMORY METHODS                                            */
/**************************************************************************************/

// Source: slau320aj
uint8_t TapDev430X::ReadByte(address_t address)
{
	HaltCpu();
	static constexpr TapStep ReadWordX_steps[] =
	{
		kTclk0,
		kIrDr16(Ir::kCntrlSigLowByte, 0x19),
		// Set address
		kIrDr20Argv(Ir::kAddr16Bit),			// dr20(address)
		kIr(Ir::kDataToAddr, kdTclkP),
		kDr16_ret(0x0000),					// content = dr16(0x0000)
		kReleaseCpu,
	};
	uint16_t content;
	gPlayer.Play(ReadWordX_steps,
		_countof(ReadWordX_steps),
		address,
		&content);
	return (uint8_t)content;
}


//Source: slau320aj
void TapDev430X::ReadBytes(address_t address, uint8_t *buf, uint32_t word_count)
{
	HaltCpu();
	gPlayer.itf_->OnClearTclk();
	gPlayer.PlayAsync(kIrDr16(Ir::kCntrlSig16Bit, 0x2409)); // Set RW to read (write-only; loop's OnIrShift drains)
	for (uint32_t i = 0; i < word_count; ++i)
	{
		// Set address
		gPlayer.itf_->OnIrShift(Ir::kAddr16Bit);
		gPlayer.itf_->OnDrShift20(address);
		gPlayer.itf_->OnIrShift(Ir::kDataToAddr);
		gPlayer.itf_->OnPulseTclk();
		// Fetch 16-bit data
		*buf = (uint8_t)gPlayer.itf_->OnDrShift16(0x0000);
		++buf;
		address += 1;
	}
	gPlayer.ReleaseCpu();
}


// Source: slau320aj
uint16_t TapDev430X::ReadWord(address_t address)
{
	HaltCpu();
	static constexpr TapStep ReadWordX_steps[] =
	{
		kTclk0,
		kIrDr16(Ir::kCntrlSigLowByte, 0x09),
		// Set address
		kIrDr20Argv(Ir::kAddr16Bit),			// dr20(address)
		kIr(Ir::kDataToAddr, kdTclkP),
		kDr16_ret(0x0000),					// content = dr16(0x0000)
		kReleaseCpu,
	};
	uint16_t content;
	gPlayer.Play(ReadWordX_steps, _countof(ReadWordX_steps),
		address,
		&content
	);
	return content;
}


//Source: slau320aj
void TapDev430X::ReadWords(address_t address, unaligned_u16 *buf, uint32_t word_count)
{
	HaltCpu();
	gPlayer.itf_->OnClearTclk();
	gPlayer.PlayAsync(kIrDr16(Ir::kCntrlSig16Bit, 0x2409)); // Set RW to read (write-only; loop's OnIrShift drains)
	for (uint32_t i = 0; i < word_count; ++i)
	{
		// Set address
		gPlayer.itf_->OnIrShift(Ir::kAddr16Bit);
		gPlayer.itf_->OnDrShift20(address);
		gPlayer.itf_->OnIrShift(Ir::kDataToAddr);
		gPlayer.itf_->OnPulseTclk();
		// Fetch 16-bit data
		*buf = gPlayer.itf_->OnDrShift16(0x0000);
		++buf;
		address += 2;
	}
	gPlayer.ReleaseCpu();
}



/**************************************************************************************/
/* MCU VERSION-RELATED WRITE MEMORY METHODS                                           */
/**************************************************************************************/

//----------------------------------------------------------------------------
//! \brief This function writes one byte/word at a given address ( <0xA00)
//! \param[in] word address (Address of data to be written)
//! \param[in] word data (shifted data)
//! Source: uif
void TapDev430X::WriteWord(address_t address, uint16_t data)
{
	HaltCpu();

	static constexpr TapStep steps[] =
	{
		kTclk0,
		kIrDr8(Ir::kCntrlSigLowByte, 0x08),
		kIrDr20Argv(Ir::kAddr16Bit),
		kIrDr16Argv(Ir::kDataToAddr,
			kdTclk1),
		kReleaseCpu,
	};
	gPlayer.Play(steps, _countof(steps),
		address,
		data
	);
}


//----------------------------------------------------------------------------
//! \brief This function writes an array of words into the target memory.
//! \param[in] word address (Start address of target memory)
//! \param[in] word *buf (Pointer to array with the data)
//! \param[in] word word_count (Number of words to be programmed)
//! Source: uif
void TapDev430X::WriteWords(address_t address, const unaligned_u16 *buf, uint32_t word_count)
{
	HaltCpu();

	gPlayer.itf_->OnClearTclk();
	gPlayer.PlayAsync(kIrDr8(Ir::kCntrlSigLowByte, 0x08));	// write-only; loop's OnIrShift drains
	for (uint32_t i = 0; i < word_count; i++)
	{
		static constexpr TapStep steps_01[] =
		{
			kIrDr20Argv(Ir::kAddr16Bit),
			kIrDr16Argv(Ir::kDataToAddr,
				kdTclkP),
		};
		gPlayer.Play(steps_01, _countof(steps_01)
			, address
			, buf[i]);
		address += 2;
	}
	gPlayer.ReleaseCpu();
}


// Source: slau320aj
void TapDev430X::WriteFlash(address_t address, const unaligned_u16 *buf, uint32_t word_count)
{
	uint32_t addr = address;				// Address counter
	HaltCpu();

	const ChipProfile &prof = gTapMcu.GetChipProfile();
	uint32_t strobes = 30;
	if (prof.flash_timings_ != NULL)
		strobes = prof.flash_timings_->word_wr_;
	
	// Writes are always allowed on INFOA, if program requires to
	uint16_t fctl3 = prof.has_locka_ ? kFctl3UnlockA : kFctl3Unlock;

	static constexpr TapStep steps_01[] =
	{
		kTclk0,
		kIrDr16(Ir::kCntrlSig16Bit, 0x2408),		// Set RW to write
		kIrDr20(Ir::kAddr16Bit, kFctl1Addr),			// FCTL1 register
		kIrDr16(Ir::kDataToAddr, kFctl1Wrt),		// Enable FLASH write
			
		kPulseTclk,
			
		kIrDr20(Ir::kAddr16Bit, kFctl2Addr),			// FCTL2 register
		kIrDr16(Ir::kDataToAddr, kFctl2MclkDiv0),	// Select MCLK as source, DIV=1
			
		kPulseTclk,
			
		kIrDr20(Ir::kAddr16Bit, kFctl3Addr),			// FCTL3 register
		kIrDr16Argv(Ir::kDataToAddr),				// Clear FCTL3; F2xxx: Unlock Info-Seg.
													// A by toggling LOCKA-Bit if required,
		kIr(kdTclkP, Ir::kCntrlSig16Bit),
	};
	gPlayer.Play(steps_01, _countof(steps_01),
		fctl3);
	
	for (uint32_t i = 0; i < word_count; i++, addr += 2)
	{
		// Skip unnecessary writes
		if (buf[i] != 0xFFFF)
		{
			static constexpr TapStep steps_02[] =
			{
				kDr16(0x2408),							// Set RW to write
				kIrDr20Argv(Ir::kAddr16Bit),				// Set address
				kIrDr16Argv(Ir::kDataToAddr,			// Set data

					kdTclkP),

				kIrDr16(Ir::kCntrlSig16Bit, 0x2409),	// Set RW to read
				kStrobeTclkArgv,						// Provide TCLKs
														// F2xxx: 29 are ok
			};
			gPlayer.Play(steps_02, _countof(steps_02), 
				addr, 
				buf[i],
				strobes
			);
		}
	}

	fctl3 |= Fctl3Flags::kLock;
	static constexpr TapStep steps_03[] =
	{
		kIrDr16(Ir::kCntrlSig16Bit, 0x2408),		// Set RW to write
		kIrDr20(Ir::kAddr16Bit, kFctl1Addr),			// FCTL1 register
		kIrDr16(Ir::kDataToAddr, kFctl1Lock),		// Enable FLASH write

		kPulseTclk,

		kIrDr20(Ir::kAddr16Bit, kFctl3Addr),			// FCTL3 register
		// Lock Inf-Seg. A by toggling LOCKA and set LOCK again
		kIrDr16Argv(Ir::kDataToAddr,
			kdTclk1),
		kReleaseCpu,
	};
	gPlayer.Play(steps_03, _countof(steps_03),
		fctl3);
}



/**************************************************************************************/
/* MCU VERSION-RELATED FLASH ERASE                                                    */
/**************************************************************************************/

// Source: slau320aj
bool TapDev430X::EraseFlash(address_t address, const FlashEraseFlags flags, EraseMode mass_erase)
{
	// Default values
	uint32_t strobe_amount = 4820;
	// Run erase just once, except for mass erase (see below)
	int run_cnt = 1;

	const ChipProfile &prof = gTapMcu.GetChipProfile();
	if (prof.flash_timings_ != NULL)
	{
		if (mass_erase)
		{
			strobe_amount = prof.flash_timings_->mass_erase_;
			// Mass erase may repeat as to obtain required cumulative time
			run_cnt = prof.flash_timings_->mass_erase_rep_;
		}
		else
			strobe_amount =prof.flash_timings_->seg_erase_;
	}
	else if (mass_erase)
	{
		// This branch can only happen if device cannot be identified
		strobe_amount = 10600;
		run_cnt = 1;	// default for this CPU generation is fast flash
	}

	HaltCpu();
	
	// LOCKA bit is always 1 after reset; setting 1 will toggle it; ignore on parts that don't have it
	uint16_t fctl3n = (prof.has_locka_) ? flags.w.fctl3_ ^ Fctl3Flags::kLockA : flags.w.fctl3_;
	// Restore LOCKA and LOCK flash at the end
	uint16_t fctl3l = fctl3n | Fctl3Flags::kLock;

	// Repeat operation for slow flash devices, until cumulative time has reached
	do
	{
		static constexpr TapStep steps_01[] =
		{
			kTclk0,
			kIrDr16(Ir::kCntrlSig16Bit, 0x2408),
			kIrDr20(Ir::kAddr16Bit, kFctl1Addr),		// FCTL1 address
			kIrDr16Argv(Ir::kDataToAddr,			// Enable erase "fctl1"

				kdTclkP),

			kIrDr20(Ir::kAddr16Bit, kFctl2Addr),		// FCTL2 address
			kIrDr16(Ir::kDataToAddr, 0xA540),		// MCLK is source, DIV=1

			kPulseTclk,

			kIrDr20(Ir::kAddr16Bit, kFctl3Addr),		// FCTL3 address
			kIrDr16Argv(Ir::kDataToAddr,			// Clear FCTL3; F2xxx: Unlock Info-Seg. A 

				kdTclkP),							// by toggling LOCKA-Bit if required,

			kIrDr20Argv(Ir::kAddr16Bit),				// Set erase "address"
			kIrDr16(Ir::kDataToAddr, 0x55AA),		// Dummy write to start erase

			kPulseTclk,								// by toggling LOCKA-Bit if required,

			kIrDr16(Ir::kCntrlSig16Bit, 0x2409),	// Set RW to read
			kStrobeTclkArgv,						// Provide 'strobe_amount' TCLKs
			kIrDr16(Ir::kCntrlSig16Bit, 0x2408),	// Set RW to write
			kIrDr20(Ir::kAddr16Bit, kFctl1Addr),		// FCTL1 address
			kIrDr16(Ir::kDataToAddr, kFctl1Lock),	// Disable erase
			kTclk1,
		};
		gPlayer.Play(steps_01, _countof(steps_01),
			flags.w.fctl1_,
			fctl3n,
			address,
			strobe_amount
		);
	}
	while (--run_cnt > 0);

	// set LOCK-Bits again
	static constexpr TapStep steps_02[] =
	{
		kTclk0,
		kIrDr20(Ir::kAddr16Bit, kFctl3Addr),		// FCTL3 address
		// Lock Inf-Seg. A by toggling LOCKA (F2xxx) and set LOCK again
		kIrDr16Argv(Ir::kDataToAddr,
			kdTclk1),
		//kReleaseCpu,
	};
	gPlayer.Play(steps_02, _countof(steps_02),
		fctl3l);
	return true;
}


void TapDev430X::UpdateEemBreakpoints(Breakpoints &bkpts, const ChipProfile &prof)
{
	uint16_t breakreact = bkpts.PrepareEemSetup(prof);
	
	if (breakreact == 0)
	{
		// disable all breakpoints by deleting the kBreakReact register
		gPlayer.PlayAsync(kIrDr32(Ir::kEmexDataExchange32, kBreakReact + kMxWrite));	// write-only; next OnDrShift32 drains
		gPlayer.itf_->OnDrShift32(0x0000);
		return;
	}
	
	for (uint8_t bp_num = 0; bp_num < prof.num_breakpoints_; ++bp_num)
	{
		DeviceBreakpoint &bp = bkpts[BkptId(bp_num)];
		if (bp.enabled_ && bp.dirty_)
		{
			// clear dirty flag
			bp.dirty_ = false;
			// Base register for breakpoint number
			const uint16_t bvBP = kTriggerBlockSize * bp_num;
			static constexpr TapStep steps[] =
			{
				// set breakpoint
				kIrDr32(Ir::kEmexDataExchange32, kGenCtrl + kMxWrite),
				kDr32(kEemEn + kClearStop + kEmuClkEn + kEmuFeatEn),
				// Value register
				kDr32Argv,
				kDr32Argv,
				// Control register
				kDr32Argv,
				kDr32(kMab + kTrig0 + kCmpEqual), // instruction fetch
				// Mask register
				kDr32Argv,
				kDr32(kNoMask),
				// Combination register
				kDr32Argv,
				kDr32Argv
			};
			gPlayer.Play(steps,
				_countof(steps),
				bvBP + kMbTrigxVal + kMxWrite,		// value register
				bp.addr_,
				bvBP + kMbTrigxCtl + kMxWrite,		// control register
				bvBP + kMbTrigxMsk + kMxWrite,		// mask register
				bvBP + kMbTrigxCmb + kMxWrite,		// combination register
				kEn0 << bp_num);
		}
	}
	// This mask activates enabled breakpoints
	gPlayer.itf_->OnDrShift32(kBreakReact + kMxWrite);
	gPlayer.itf_->OnDrShift32(breakreact);
}




/**************************************************************************************/
/* CPU FLOW CONTROL                                                                   */
/**************************************************************************************/


