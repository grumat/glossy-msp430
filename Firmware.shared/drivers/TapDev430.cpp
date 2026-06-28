#include "stdproj.h"

#include "TapDev430.h"
#include "eem_defs.h"
#include "TapMcu.h"


/**************************************************************************************/
/* MCU VERSION-RELATED POWER ON RESET                                                 */
/**************************************************************************************/

void TapDev430::InitDefaultChip(ChipProfile &prof, JtagId jtag_id)
{
	(void)jtag_id;	// legacy parts have a single default profile
	prof.DefaultMcu();
}


bool TapDev430::GetDevice(CoreId &coreid)
{
	coreid.id_data_addr_ = 0x0FF0;
	coreid.coreip_id_ = 0;
	coreid.ip_pointer_ = 0;
	// JTAG mode + CPU run + read
	if (g_Player.SetJtagRunReadLegacy() != kMspStd)
		goto error_exit;

	unsigned int loop_counter;
	for (loop_counter = 50; loop_counter > 0; loop_counter--)
	{
		if ((g_Player.itf_->OnDrShift16(0x0000) & 0x0200) == 0x0200)
			break;
	}

	if (loop_counter == 0)
	{
		Error() << "TapDev430::GetDevice: timed out\n";
error_exit:
		g_TapMcu.failed_ = true;
		/* timeout reached */
		return kInvalid;
	}
	coreid.device_id_ = TapDev430::ReadWord(0x0FF0);
	return true;
}


// Source UIF
bool TapDev430::IsInstrLoad()
{
	if (!IsSet(GetCtrlSigReg(), CtrlSigReg::kInstrLoad | CtrlSigReg::kRead))
	{
		return false;
	}
	return true;
}


// Source UIF
bool TapDev430::SyncJtagAssertPorSaveContext(CpuContext &ctx, const ChipProfile &prof)
{
	constexpr uint16_t address = WDT_ADDR_CPU;
	CtrlSigReg ctl_sync = CtrlSigReg::kNone;

	ctx.is_running_ = false;

	// Sync the JTAG
	if (g_Player.SetJtagRunReadLegacy() != kMspStd)
		return false;

	uint16_t lOut = g_Player.GetCtrlSigReg();    // read control register once

	g_Player.SetTCLK();

	if (!IsSet(CtrlSigReg(lOut), CtrlSigReg::kTce))
	{
		// If the JTAG and CPU are not already synchronized ...
		// Initiate Jtag and CPU synchronization. Read/Write is under CPU control. Source TCLK via TDI.
		// Do not effect bits used by DTC (CPU_HALT, MCLKON).
		static constexpr TapStep steps_01[] =
		{
			// initiate CPU synchronization but release low byte of CNTRL sig register to CPU control
			kIrDr8(
				Ir::kCntrlSigHighByte
				, E2I(CtrlSigReg::kTagFuncSat | CtrlSigReg::kTce1 | CtrlSigReg::kCpu) >> 8
				),
			// Address Force Sync special handling
			// read access to EEM General Clock Control Register (GCLKCTRL)
			kIrDr16(Ir::kEmexDataExchange, kMxGClkCtrl + kMxRead),
			// read the content of GCLKCNTRL into lOut
			kDr16_ret(0),
		};

		g_Player.Play(steps_01, _countof(steps_01), &lOut);
		// Set Force Jtag Synchronization bit in Emex General Clock Control register.
		lOut |= 0x0040;							// 0x0040 = FORCE_SYN

		// Stability improvement: should be possible to remove this, required only once at the beginning
		// write access to EEM General Clock Control Register (GCLKCTRL)
		g_Player.PlayAsync(kIrDr16(Ir::kEmexDataExchange, kMxGClkCtrl + kMxWrite));	// write-only; next DR_Shift16 drains
		lOut = g_Player.DR_Shift16(lOut);		// write into GCLKCNTRL

		// Reset Force Jtag Synchronization bit in Emex General Clock Control register.
		lOut &= ~0x0040;
		// Stability improvement: should be possible to remove this, required only once at the beginning
		// write access to EEM General Clock Control Register (GCLKCTRL)
		g_Player.PlayAsync(kIrDr16(Ir::kEmexDataExchange, kMxGClkCtrl + kMxWrite));	// write-only; next DR_Shift16 drains
		lOut = g_Player.DR_Shift16(lOut);		// write into GCLKCNTRL

		ctl_sync = SyncJtag();

		if (!IsSet(ctl_sync, CtrlSigReg::kCpuHalt))
		{ // Synchronization failed!
			return false;
		}
		g_Player.ClrTCLK();
		g_Player.PlayAsync(kIrDr16(Ir::kCntrlSig16Bit, 0x2401));	// JTAG + WORD + RD
		g_Player.SetTCLK();
	}
	else
	{
		g_Player.PlayAsync(kIrDr16(Ir::kCntrlSig16Bit, 0x2401));	// JTAG + WORD + RD (write-only; next shift drains)
	}// end of if(!(lOut & CNTRL_SIG_TCE))

	// SLAU335 (i20xx) JTAGONSBW activation (== TI SPYBIWIREJTAG_IF, the
	// activationKey 0x20404020 path): after the TEST/RST entry the device clocks
	// stay gated until bit 7 (BSL_VALID) of the JTAG test register is de-asserted.
	// See .claude/docs/msp430/I2031_ACQUISITION_GOLDEN_REFERENCE.md.
	if (prof.slau_ == ChipInfoDB::kSLAU335)
	{
		// here we add bit de assert bit 7 in JTAG test reg to enable clocks again
		lOut = g_Player.Play(kIrDr8(Ir::kTestReg, 0));
		lOut |= 0x80; //DE_ASSERT_BSL_VALID;
		g_Player.PlayAsync(kIrDr8(Ir::kTestReg, lOut));	// Bit 7 is de asserted now (write-only; next shift drains)
	}

	// execute a dummy instruction here
	static constexpr TapStep steps_02[] =
	{
		// Stability improvement: should be possible to remove this TCLK is already 1
		kIrData16(kdTclk1, 0x4303, kdTclk0),	// kIr(Ir::kData16Bit) + kTclk1 + kDr16(0x4303 = NOP) + kTclk
		kIr(Ir::kDataCapture, kdTclk1),			// kIr(Ir::kDataCapture) + kTclk1
	};
	g_Player.Play(steps_02, _countof(steps_02));

	// step until next instruction load boundary if not being already there
	if (!IsInstrLoad())
		return false;

	if (prof.clk_ctrl_ == ChipInfoDB::kGccExtended)
	{
		static constexpr TapStep steps[] =
		{
			// Perform the POR
			kIrDr16(Ir::kEmexDataExchange, kMxGenCntrl + kMxWrite),		// write access to EEM General Control Register (kMxGenCntrl)
			kDr16(kEmuFeatEn | kEmuClkEn | kClearStop | kEemEn),		// write into kMxGenCntrl
			// Stability improvement: should be possible to remove this, required only once at the beginning
			kIrDr16(Ir::kEmexDataExchange, kMxGenCntrl + kMxWrite),		// write access to EEM General Control Register (kMxGenCntrl)
			kDr16(kEmuFeatEn | kEmuClkEn),							// write into kMxGenCntrl
		};
		g_Player.Play(steps, _countof(steps));
	}
	else if (prof.clk_ctrl_ == ChipInfoDB::kGccStandardI)
	{
		static constexpr TapStep steps[] =
		{
			kIrDr16(Ir::kEmexDataExchange, kMxGenCntrl + kMxWrite),		// write access to EEM General Control Register (kMxGenCntrl)
			kDr16(kEmuFeatEn),											// write into kMxGenCntrl

		};
		g_Player.Play(steps, _countof(steps));
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
	g_Player.Play(steps_03, _countof(steps_03));

	// step until an appropriate instruction load boundary
	uint32_t i = 10;
	while (true)
	{
		lOut = g_Player.Play(kIrDr16(Ir::kAddrCapture, 0x0000));
		if (lOut == 0xFFFE || lOut == 0x0F00)
			break;
		if (i == 0)
			return false;
		--i;
		g_Player.PulseTCLKN();
	}

	static constexpr TapStep steps_04[] =
	{
		kPulseTclkN,
		kPulseTclkN,

		kTclk0,
		kIrDr16(Ir::kAddrCapture, 0x0000),
		kTclk1,
	};
	g_Player.Play(steps_04, _countof(steps_04));

	// step until next instruction load boundary if not being already there
	if (!IsInstrLoad())
		return false;

	// Hold Watchdog
	ctx.wdt_ = ReadWord(address);	// safe WDT value
	uint16_t wdtval = WDT_HOLD | ctx.wdt_;	// set original bits in addition to stop bit
	WriteWord(address, wdtval);

#if 0
	// read MAB = PC here
	ctx.pc_ = g_Player.Play(kIrDr16(Ir::kAddrCapture, 0));
#else
	// UIF: Read reset vector!
	ctx.pc_ = ReadWord(0xFFFE) & 0x0FFFE;
#endif

	// set PC to a save address pointing to ROM to avoid RAM corruption on certain devices
	SetPC(kRomAddr);

	// read status register
	ctx.sr_ = GetReg(2);

	return true;
}


// Source UIF
bool TapDev430::SyncJtagConditionalSaveContext(CpuContext &ctx, const ChipProfile &prof)
{
	constexpr uint16_t address = WDT_ADDR_CPU;
	CtrlSigReg ctl_sync = CtrlSigReg::kNone;
	uint16_t lOut = 0;
	uint16_t statusReg = 0;

	// syncWithRunVarAddress
	ctx.is_running_ = false;

	// Stability improvement: should be possible to remove this here, default state of TCLK should 
	// be one
	g_Player.SetTCLK();

	// Check Test Clock Enable bit
	if (!IsSet(GetCtrlSigReg(), CtrlSigReg::kTce))
	{
		// If the JTAG and CPU are not already synchronized ...
		// Initiate JTAG and CPU synchronization. Read/Write is under CPU control. Source TCLK 
		// via TDI.
		// Do not effect bits used by DTC (CPU_HALT, MCLKON).
		g_Player.PlayAsync(kIrDr8(Ir::kCntrlSigHighByte, E2I(CtrlSigReg::kTagFuncSat | CtrlSigReg::kTce1 | CtrlSigReg::kCpu) >> 8));	// write-only; SyncJtag's shift drains

		// A bug in first F43x and F44x silicon requires that JTAG synchronization be forced (when 
		// the CPU is Off).
		// Since the JTAG and CPU are not yet synchronized, the CPUOFF bit in the lower byte of the 
		// cntrlSig register is not valid.
		// TCE eventually set indicates synchronization (and clocking via TCLK).
		ctl_sync = SyncJtag();
		if (ctl_sync == CtrlSigReg::kNone)
			return false;	// Synchronization failed!
	}// end of if(!(lOut & CNTRL_SIG_TCE))

	const bool cpu_halted = IsSet(ctl_sync, CtrlSigReg::kCpuHalt);
	if (cpu_halted)
		g_Player.ClrTCLK();
	// Clear HALT. Read/Write is under CPU control. As a precaution, disable interrupts.
	g_Player.PlayAsync(kIrDr16(Ir::kCntrlSig16Bit, E2I(CtrlSigReg::kTagFuncSat | CtrlSigReg::kTce1 | CtrlSigReg::kCpu)));	// write-only; SetTCLK drains
	if (cpu_halted)
		g_Player.SetTCLK();

	// SLAU335 (i20xx) JTAGONSBW activation (== TI SPYBIWIREJTAG_IF, the
	// activationKey 0x20404020 path): after the TEST/RST entry the device clocks
	// stay gated until bit 7 (BSL_VALID) of the JTAG test register is de-asserted.
	// See .claude/docs/msp430/I2031_ACQUISITION_GOLDEN_REFERENCE.md.
	if (prof.slau_ == ChipInfoDB::kSLAU335)
	{
		// here we add bit de assert bit 7 in JTAG test reg to enable clocks again
		lOut = g_Player.Play(kIrDr8(Ir::kTestReg, 0));
		lOut |= 0x80; //DE_ASSERT_BSL_VALID;
		g_Player.PlayAsync(kIrDr8(Ir::kTestReg, lOut));	// Bit 7 is de asserted now (write-only; next shift drains)
	}

	// step until next instruction load boundary if not being already there
	if (!IsInstrLoad())
		return false;

	// read MAB = PC here
	ctx.pc_ = g_Player.Play(kIrDr16(Ir::kAddrCapture, 0));

	if (prof.clk_ctrl_ != ChipInfoDB::kGccNone)
	{
		if (prof.clk_ctrl_ == ChipInfoDB::kGccExtended)
		{
			static constexpr TapStep steps[] =
			{
				// disable EEM and clear stop reaction
				kIrDr16(Ir::kEmexWriteControl, kClearStop | kEemEn),
				kDr16(0x0000),

				// write access to EEM General Control Register (kMxGenCntrl)
				kIrDr16(Ir::kEmexDataExchange, kMxGenCntrl + kMxWrite),
				// write into kMxGenCntrl
				kDr16(kEmuFeatEn | kEmuClkEn | kClearStop | kEemEn),

				// Stability improvement: should be possible to remove this, required only once at the beginning
				// write access to EEM General Control Register (kMxGenCntrl)
				kIrDr16(Ir::kEmexDataExchange, kMxGenCntrl + kMxWrite),
				// write into kMxGenCntrl
				kDr16(kEmuFeatEn | kEmuClkEn),
			};
			g_Player.Play(steps, _countof(steps));
		}
		else if (prof.clk_ctrl_ == ChipInfoDB::kGccStandardI)
		{
			static constexpr TapStep steps[] =
			{
				// disable EEM and clear stop reaction
				kIrDr16(Ir::kEmexWriteControl, kClearStop | kEemEn),
				kDr16(0x0000),

				// write access to EEM General Control Register (kMxGenCntrl)
				kIrDr16(Ir::kEmexDataExchange, kMxGenCntrl + kMxWrite),
				// write into kMxGenCntrl
				kDr16(kEmuFeatEn),
			};
			g_Player.Play(steps, _countof(steps));
		}

		if (prof.stop_fll_)
		{
			// read access to EEM General Clock Control Register (GCLKCTRL)
			g_Player.PlayAsync(kIrDr16(Ir::kEmexDataExchange, kMxGClkCtrl + kMxRead));	// write-only; next shift drains
			uint16_t clkCntrl = g_Player.DR_Shift16(0);
			// added UPSF: FE427 does regulate the FLL to the upper boarder
			// added the switch off and release of FLL (JTFLLO)
			clkCntrl |= 0x10;
			g_Player.PlayAsync(kIrDr16(Ir::kEmexDataExchange, kMxGClkCtrl + kMxWrite));	// write-only; next DR_Shift16 drains
			g_Player.DR_Shift16(clkCntrl);
		}
	}

	//-----------------------------------------------------------------------
	// Execute a dummy instruction (BIS #0,R4) to work around a problem in the F12x 
	// that can sometimes cause the first TCLK after synchronization to be lost. If 
	// the TCLK is lost, try the cycle again.
	// Note: It is critical that the dummy instruction require exactly two cycles to 
	// execute. The version of BIS # used does not use the constant generator, and so 
	// it requires two cycles.
	// The dummy instruction also provides a needed extra clock when the device is 
	// stopped by the Emex module and it is OFF.
	// The dummy instruction also possibly initiates processing of a pending interrupt.
#define BIS_IMM_0_R4 0xd034
	g_Player.Play(kIrData16(kdTclk1, BIS_IMM_0_R4));

	if (!ClkTclkAndCheckDTC())
		return false;

	// Still on an instruction load boundary?
	if (IsSet(GetCtrlSigReg(), CtrlSigReg::kInstrLoad))
	{
		// Repeat the previous step a second time
		g_Player.Play(kIrData16(kdTclk1, BIS_IMM_0_R4));
		if (!ClkTclkAndCheckDTC())
			return false;
	}

	g_Player.Play(kIrData16(kdTclk1, 0));
	if (!ClkTclkAndCheckDTC())
		return false;

	// Advance to an instruction load boundary if an interrupt is detected.
	// The CPUOff bit will be cleared.
	// Basically, if there is an interrupt pending, the above dummy instruction
	// will have initiated its processing, and the CPU will not be on an
	// instruction load boundary following the dummy instruction.

	uint16_t i = 0;
	g_Player.IR_Shift(Ir::kCntrlSigCapture);
	while (IsReset(ShiftCtrlSigReg(), CtrlSigReg::kInstrLoad))
	{
		if (!ClkTclkAndCheckDTC())
			return false;
		// give up depending on retry counter
		if (++i == MAX_TCE1)
			return false;
		g_Player.IR_Shift(Ir::kCntrlSigCapture);
	}

	// Read PC now!!! Only the NOP or BIS #0,R4 instruction above was clocked into the device
	// The PC value should now be (OriginalValue + 2)
	// read MAB = PC here
	ctx.pc_ = (g_Player.Play(kIrDr16(Ir::kAddrCapture, 0)) - 4) & 0xFFFF;

	if (i == 0)
	{
		// An Interrupt was not detected
		//		lOut does not contain the content of the CNTRL_SIG register anymore at this point
		//		need to capture it again...different to DLLv3 sequence but don't expect any negative 
		//		effect due to recapturing
		ctx.in_interrupt_ = false;

		if (IsSet(GetCtrlSigReg(), CtrlSigReg::kPOR))
		{
			ctx.pc_ = (ctx.pc_ + 2) & 0xFFFF;
#define BIC_IMM_0_R2 0xC032
			g_Player.Play(kIrData16(kdTclk1, BIC_IMM_0_R2));
			if (!ClkTclkAndCheckDTC())
				return false;
#define BRA_PC_P 0x0010
			// clear carry flag
			g_Player.Play(kIrData16(kdTclk1, BRA_PC_P));
			if (!ClkTclkAndCheckDTC())
				return false;
			// DLLv2 preserve the CPUOff bit
			statusReg |= kStatusRegCpuOff;
		}
	}
	else
	{
		ctx.pc_ = g_Player.Play(kIrDr16(Ir::kAddrCapture, 0));
		ctx.in_interrupt_ = true;
	}

	// DLL v2: deviceHasDTCBug
	//     Configure the DTC so that it transfers after the present CPU instruction is complete (ADC10FETCH).
	//     Save and clear ADC10CTL0 and ADC10CTL1 to switch off DTC (Note: Order matters!).

	// Regain control of the CPU. Read/Write will be set, and source TCLK via TDI.
	g_Player.PlayAsync(kIrDr16(Ir::kCntrlSig16Bit, E2I(CtrlSigReg::kTce1 | CtrlSigReg::kRead | CtrlSigReg::kTagFuncSat)));	// write-only; InstrLoad's shift drains

	// Test if we are on an instruction load boundary
	if (!InstrLoad())
		return false;

	// Hold Watchdog
	uint16_t wdtval = ctx.wdt_ | WDT_PASSWD;
	ctx.wdt_ = (uint8_t)TapDev430::ReadWord(address);	// save WDT value
	wdtval |= ctx.wdt_;									// adds the WDT stop bit
	TapDev430::WriteWord(address, wdtval);

	// set PC to a save address pointing to ROM to avoid RAM corruption on certain devices
	SetPC(kRomAddr);

	// read status register
	ctx.sr_ = GetReg(2) | statusReg;	// combine with preserved CPUOFF bit setting

	return true;
}


// Source: slau320aj
bool TapDev430::ExecutePOR()
{
	// Perform Reset
	uint8_t jtag_ver;
	static constexpr TapStep steps[] =
	{
		kIrDr16(Ir::kCntrlSig16Bit, 0x2C01),	// Apply Reset
		kDr16(0x2401),							// Remove Reset
		kPulseTclkN,							// F2xxx
		kPulseTclkN,
		kTclk0,
		kIrRet(Ir::kAddrCapture),				// returns the jtag ID
		kTclk1,
	};
	g_Player.Play(steps, _countof(steps),
		 &jtag_ver
	);

	TapDev430::WriteWord(0x0120, 0x5A80);	// Disable Watchdog on target device

	if (jtag_ver != kMspStd)
		return false;
	return true;
}


// Source: slau320aj
void TapDev430::ReleaseDevice(address_t address)
{
	switch (address)
	{
	case V_RUNNING: /* Nothing to do */
		break;
	case V_BOR:
	case V_RESET: /* Perform reset */
		/* issue reset */
		g_Player.PlayAsync(kIrDr16(Ir::kCntrlSig16Bit, 0x2C01));	// write-only; next OnDrShift16 drains
		g_Player.itf_->OnDrShift16(0x2401);
		break;
	default: /* Set target CPU's PC */
		SetPC(address);
		break;
	}

#warning "Review this method"
	SetInstructionFetch();

	// Reads breakpoint reaction register
	static constexpr TapStep steps[] =
	{
		kIrDr16(Ir::kEmexDataExchange, kBreakReact + kMxRead),
		kDr16(0x0000),
		kIrDr16(Ir::kEmexWriteControl, kEmuFeatEn + kEmuClkEn + kClearStop + kEemEn),
		kIr(Ir::kCntrlSigRelease),
	};
	g_Player.Play(steps, _countof(steps));
}


// Source: uif
void TapDev430::ReleaseDevice(CpuContext &ctx, const ChipProfile &prof, bool run_to_bkpt, uint16_t mdbval)
{
	SetReg(2, ctx.sr_);
	WriteWord(WDT_ADDR_CPU, ctx.wdt_);
	SetPC(ctx.pc_);
	
	// BLOCK: Workaround for MSP430F149 derivatives
	{
		/*** This is highly undocumented stuff ***/
		uint16_t backup[3];
		static constexpr TapStep steps[] =
		{
			kIr(Ir::kEmexDataExchange),
			
			kDr16(0x03), kDr16_ret(0),
			kDr16(0x0B), kDr16_ret(0),
			kDr16(0x13), kDr16_ret(0),
			
			kDr16(0x02), kDr16(0),
			kDr16(0x0A), kDr16(0),
			kDr16(0x12), kDr16(0),
			
			kDr16(0x02), kDr16ArgvI,
			kDr16(0x0A), kDr16ArgvI,
			kDr16(0x12), kDr16ArgvI,
		};
		g_Player.Play(steps, _countof(steps),
			&backup[0],
			&backup[1],
			&backup[2],
			&backup[0],
			&backup[1],
			&backup[2]
			);
	}
	//
	if (prof.clk_ctrl_ != ChipInfoDB::kGccNone
		&& prof.stop_fll_)
	{
		uint16_t clk_ctrl;
		static constexpr TapStep steps_0[] =
		{ 
			kIrDr16(Ir::kEmexDataExchange, kMxGClkCtrl + kMxRead),
			kDr16_ret(0),
		};
		g_Player.Play(steps_0,
			_countof(steps_0),
			&clk_ctrl);
		// added UPSF: FE427 does regulate the FLL to the upper border
		// added the switch off and release of FLL (JTFLLO)
		clk_ctrl &= ~0x10;
		static constexpr TapStep steps_1[] =
		{ 
			kIrDr16(Ir::kEmexDataExchange, kMxGClkCtrl + kMxWrite),
			kDr16Argv,
		};
		g_Player.Play(steps_1,
			_countof(steps_1),
			clk_ctrl);
	}
	
	uint16_t eemwr = 0;
	//
	if (prof.clk_ctrl_ == ChipInfoDB::kGccExtended)
		eemwr = kEmuFeatEn | kEmuClkEn | kClearStop | kEemEn;
	else if (prof.clk_ctrl_ == ChipInfoDB::kGccStandardI)
		eemwr = kEmuFeatEn;
	// Additional EEM setup
	if (eemwr)
	{
		static constexpr TapStep steps[] =
		{ 
			// write access to EEM General Control Register (kMxGenCntrl)
			kIrDr16(Ir::kEmexDataExchange, kMxGClkCtrl + kMxWrite),
			// write into kMxGenCntrl
			kDr16Argv,
		};
		g_Player.Play(steps,
			_countof(steps),
			eemwr);
	}
	// Activate EEM
	g_Player.Play(run_to_bkpt 
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
		g_Player.Play(steps,
			_countof(steps),
			mdbval);
	}
	else
		g_Player.IR_Shift(Ir::kAddrCapture);
	// Release target device from JTAG control
	g_Player.IR_Shift(Ir::kCntrlSigRelease);
	ctx.is_running_ = true;
}




/**************************************************************************************/
/* DEVICE IDENTIFICATION METHODS                                                      */
/**************************************************************************************/

// Source: UIF
bool TapDev430::GetDeviceSignature(DieInfo &id, CpuContext &ctx, const CoreId &coreid)
{
	constexpr uint32_t idDataAddr = 0x0FF0;
	union
	{
		uint16_t d16[8];
		uint8_t d8[16];
	} data;
	
	(void)ctx; (void)coreid;

	ReadWords(idDataAddr, data.d16, _countof(data.d16));

	id.mcu_ver_ = data.d16[0];
	id.mcu_sub_ = 0x0000;
	id.mcu_rev_ = data.d8[2];
	id.mcu_fab_ = data.d8[3];
	id.mcu_self_ = data.d16[2];
	id.mcu_cfg_ = data.d8[13] & 0x7f;
	// read fuses
	g_Player.itf_->OnIrShift(Ir::kConfigFuses);
	id.mcu_fuse_ = g_Player.itf_->OnDrShift8(0);
	return true;
}




/**************************************************************************************/
/* MCU VERSION-RELATED REGISTER GET/SET METHODS                                       */
/**************************************************************************************/

//----------------------------------------------------------------------------
//! \brief Load a given address into the target CPU's program counter (PC).
//! \param[in] word address (destination address)
//! slau320aj
bool TapDev430::SetPC(address_t address)
{
	static constexpr TapStep steps[] =
	{
		// Load PC with address
		kIrDr16(Ir::kCntrlSig16Bit, 0x3401),		// CPU has control of RW & BYTE.
		kIrData16(kdNone, 0x4030, kdTclkN),			// "mov #addr,PC" instruction
		// kPulseTclkN		// F2xxx
		kDr16Argv,									// "mov #addr,PC" instruction
		// kPulseTclkN		// F2xxx
		kIr(kdTclkN, Ir::kAddrCapture, kdTclk0),		// Now the PC should be on address
		kIrDr16(Ir::kCntrlSig16Bit, 0x2401),		// JTAG has control of RW & BYTE.
		kTclk1,
	};
	g_Player.Play(steps, _countof(steps),
				address
	);
	return true;
}


// Source: uif
bool TapDev430::SetReg(uint8_t reg, uint32_t value)
{
	static constexpr TapStep steps[] =
	{
		kIrDr16(Ir::kCntrlSig16Bit, 0x3401),
		kIrData16Argv(kdNone, kdTclk0),			// kIr(Ir::kData16Bit) + kDr16(0x4030 | reg) + kTclk0
		kIr(Ir::kDataCapture),
		kIrData16Argv(kdTclk1, kdTclk0),		// kIr(Ir::kData16Bit) + kTclk1 + kDr16(value) + kTclk0
		kIr(Ir::kDataCapture, kdTclk1),			// kIr(Ir::kDataCapture) + kTclk1
		kIrData16(kdNone, 0x3ffd, kdTclk0),		// kIr(Ir::kData16Bit) + kTclk1 + kDr16('jmp $-4') + kTclk0
		kIr(Ir::kDataCapture, kdTclkP),			// kIr(Ir::kDataCapture) + kPulseTclk
		kIrDr16(Ir::kCntrlSig16Bit, 0x2401),	// kIr(Ir::kCntrlSig16Bit) + kDr16(0x2401); JTAG + WORD + RD
		kTclk1,
	};
	g_Player.Play(steps, _countof(steps),
		(0x4030 | reg),
		value
	);
	return true;
}


// Source: uif
uint32_t TapDev430::GetReg(uint8_t reg)
{
	static constexpr TapStep steps[] =
	{
		kIrDr16(Ir::kCntrlSig16Bit, 0x3401),
		kIrDr16Argv(Ir::kData16Bit),
		kIr(kdTclk0, Ir::kDataCapture, kdTclk1),	// kTclk0 + kIr(Ir::kDataCapture) + kTclk1
		kIrData16(kdNone, 0x00fe, kdTclk0),		// address part of "mov rX, &00fe"
		kIr(Ir::kDataCapture, kdTclkP),			// kIr(Ir::kDataCapture) + kPulseTclk
		kTclk1,
		kDr16_ret(0),							// *data = dr16(0)
		kTclk0,
		kIrDr16(Ir::kCntrlSig16Bit, 0x2401),	// kIr(Ir::kCntrlSig16Bit) + kDr16(0x2401); JTAG + WORD + RD
		kTclk1,
	};

	uint16_t data = 0xFFFF;
	g_Player.Play(steps, _countof(steps),
		 ((reg << 8) & 0x0F00) | 0x4082,		// equivalent to "mov rX, &00fe"
		 &data
	);
	return data;
}



/**************************************************************************************/
/* MCU VERSION-RELATED READ MEMORY METHODS                                            */
/**************************************************************************************/

// Source: slau320aj
uint8_t TapDev430::ReadByte(address_t address)
{
	HaltCpu();
	static constexpr TapStep steps[] =
	{
		kTclk0,
		kIrDr16(Ir::kCntrlSig16Bit, 0x2419),
		// Set address
		kIrDr16Argv(Ir::kAddr16Bit),			// dr16(address)
		kIr(Ir::kDataToAddr, kdTclkP),		// kIr(Ir::kDataToAddr) + kPulseTclk
		kDr16_ret(0x0000),					// content = dr16(0x0000)
		kReleaseCpu,
	};
	uint16_t content = 0xFFFF;
	g_Player.Play(steps,
		_countof(steps),
		address,
		&content);
	return (uint8_t)content;
}


// Source: slau320aj
void TapDev430::ReadBytes(address_t address, uint8_t *buf, uint32_t byte_count)
{
	HaltCpu();
	g_Player.itf_->OnClearTclk();
	g_Player.PlayAsync(kIrDr16(Ir::kCntrlSig16Bit, 0x2419)); // Set RW to read (write-only; loop's OnIrShift drains)
	for (uint32_t i = 0; i < byte_count; ++i)
	{
		// Set address
		g_Player.itf_->OnIrShift(Ir::kAddr16Bit);
		g_Player.itf_->OnDrShift16(address);
		g_Player.itf_->OnIrShift(Ir::kDataToAddr);
		g_Player.itf_->OnPulseTclk();
		// Fetch 16-bit data
		*buf++ = (uint8_t)g_Player.itf_->OnDrShift16(0x0000);
		address += 1;
	}
	g_Player.ReleaseCpu();
}


// Source: slau320aj
uint16_t TapDev430::ReadWord(address_t address)
{
	HaltCpu();
	static constexpr TapStep steps[] =
	{
		kTclk0,
		kIrDr16(Ir::kCntrlSig16Bit, 0x2409),
		// Set address
		kIrDr16Argv(Ir::kAddr16Bit),			// dr16(address)
		kIr(Ir::kDataToAddr, kdTclkP),		// kIr(Ir::kDataToAddr) + kPulseTclk
		kDr16_ret(0x0000),					// content = dr16(0x0000)
		kReleaseCpu,
	};
	uint16_t content = 0xFFFF;
	g_Player.Play(steps, _countof(steps),
		 address,
		 &content
	);
	return content;
}


// Source: slau320aj
void TapDev430::ReadWords(address_t address, unaligned_u16 *buf, uint32_t word_count)
{
	HaltCpu();
	g_Player.itf_->OnClearTclk();
	g_Player.PlayAsync(kIrDr16(Ir::kCntrlSig16Bit, 0x2409)); // Set RW to read (write-only; loop's OnIrShift drains)
	for (uint32_t i = 0; i < word_count; ++i)
	{
		// Set address
		g_Player.itf_->OnIrShift(Ir::kAddr16Bit);
		g_Player.itf_->OnDrShift16(address);
		g_Player.itf_->OnIrShift(Ir::kDataToAddr);
		g_Player.itf_->OnPulseTclk();
		// Fetch 16-bit data
		*buf++ = g_Player.itf_->OnDrShift16(0x0000);
		address += 2;
	}
	g_Player.ReleaseCpu();
}


//----------------------------------------------------------------------------
//! \brief This function writes one byte/word at a given address ( <0xA00)
//! \param[in] word address (Address of data to be written)
//! \param[in] word data (shifted data)
//! Source: uif
void TapDev430::WriteWord(address_t address, uint16_t data)
{
	HaltCpu();

	static constexpr TapStep steps[] =
	{
		kTclk0,
		kIrDr8(Ir::kCntrlSigLowByte, 0x08), // Set word write
		kIrDr16Argv(Ir::kAddr16Bit),				// Set address
		kIrDr16Argv(Ir::kDataToAddr,			// Shift in 16 bits
			kdTclk1),
		kReleaseCpu,
	};
	g_Player.Play(steps, _countof(steps),
		address,
		data
	);
	//g_Player.itf_->OnSetTclk(); // is also the first instruction in ReleaseCpu()
	g_Player.ReleaseCpu();
}


//----------------------------------------------------------------------------
//! \brief This function writes an array of words into the target memory.
//! \param[in] word address (Start address of target memory)
//! \param[in] word word_count (Number of words to be programmed)
//! \param[in] word *buf (Pointer to array with the data)
//! Source: uif
void TapDev430::WriteWords(address_t address, const unaligned_u16 *buf, uint32_t word_count)
{
	HaltCpu();

	g_Player.ClrTCLK();
	g_Player.PlayAsync(kIrDr16(Ir::kCntrlSig16Bit, 0x2408));	// write-only; loop's OnIrShift drains
	for (uint32_t i = 0; i < word_count; i++)
	{
		static constexpr TapStep steps_01[] =
		{
			kIrDr16Argv(Ir::kAddr16Bit),
			kIrDr16Argv(Ir::kDataToAddr,
				kdTclkP),
		};
		g_Player.Play(steps_01,
			_countof(steps_01),
			address,
			buf[i]);
		address += 2;
	}
	g_Player.ReleaseCpu();
}


// Source: slau320aj
void TapDev430::WriteFlash(address_t address, const unaligned_u16 *buf, uint32_t word_count)
{
	address_t addr = address;				// Address counter
	HaltCpu();

	const ChipProfile &prof = g_TapMcu.GetChipProfile();
	uint32_t strobes = 35;
	if (prof.flash_timings_ != NULL)
		strobes = prof.flash_timings_->word_wr_;
	
	// Writes are always allowed on INFOA, if program requires to
	uint16_t fctl3 = prof.has_locka_ ? kFctl3UnlockA : kFctl3Unlock;

	static constexpr TapStep steps_01[] =
	{
		kTclk0,
		kIrDr16(Ir::kCntrlSig16Bit, 0x2408),		// Set RW to write
		kIrDr16(Ir::kAddr16Bit, kFctl1Addr),			// FCTL1 register
		kIrDr16(Ir::kDataToAddr, kFctl1Wrt),		// Enable FLASH write
			
		kPulseTclk,									// kTclk1 + kTclk0

		kIrDr16(Ir::kAddr16Bit, kFctl2Addr),			// FCTL2 register
		kIrDr16(Ir::kDataToAddr, kFctl2MclkDiv0),	// Select MCLK as source, DIV=1

		kPulseTclk,									// kTclk1 + kTclk0

		kIrDr16(Ir::kAddr16Bit, kFctl3Addr),			// FCTL3 register
		kIrDr16Argv(Ir::kDataToAddr),				// Clear FCTL3; F2xxx: Unlock Info-Seg.
													// A by toggling LOCKA-Bit if required,
		kIr(kdTclkP, Ir::kCntrlSig16Bit),			// Pulse + kIr(Ir::kCntrlSig16Bit)
	};
	g_Player.Play(steps_01, _countof(steps_01),
		fctl3);

	for (uint32_t i = 0; i < word_count; i++, addr += 2)
	{
		// Skip unnecessary writes
		if(buf[i] != 0xFFFF)
		{
			static constexpr TapStep steps_02[] =
			{
				kDr16(0x2408),							// Set RW to write
				kIrDr16Argv(Ir::kAddr16Bit),				// Set address
				kIrDr16Argv(Ir::kDataToAddr,			// Set data
					kdTclkP),

				kIrDr16(Ir::kCntrlSig16Bit, 0x2409),	// Set RW to read
				kStrobeTclkArgv,						// Provide TCLKs
														// F2xxx: 29 are ok
			};
			g_Player.Play(steps_02, _countof(steps_02),
				addr,
				buf[i],
				strobes
			);
		}
	}

	fctl3 |= Fctl3Flags::LOCK;
	static constexpr TapStep steps_03[] =
	{
		kIrDr16(Ir::kCntrlSig16Bit, 0x2408),	// Set RW to write
		kIrDr16(Ir::kAddr16Bit, kFctl1Addr),		// FCTL1 register
		kIrDr16(Ir::kDataToAddr, kFctl1Lock),	// Disable FLASH write

		kPulseTclk,								// kTclk1 + kTclk0

		kIrDr16(Ir::kAddr16Bit, kFctl3Addr),		// FCTL3 register
		// Lock Inf-Seg. A by toggling LOCKA and set LOCK again
		kIrDr16Argv(Ir::kDataToAddr,
			kdTclk1),
		kReleaseCpu,
	};
	g_Player.Play(steps_03, _countof(steps_03),
		fctl3);
}



/**************************************************************************************/
/* MCU VERSION-RELATED FLASH ERASE                                                    */
/**************************************************************************************/

// Source: slau320aj
bool TapDev430::EraseFlash(address_t address, const FlashEraseFlags flags, EraseMode mass_erase)
{
	// Default values
	uint32_t strobe_amount = 4820;
	// Run erase just once, except for mass erase (see below)
	int run_cnt = 1;

	const ChipProfile &prof = g_TapMcu.GetChipProfile();
	if (prof.flash_timings_ != NULL)
	{
		if (mass_erase)
		{
			strobe_amount = prof.flash_timings_->mass_erase_;
			// Mass erase may repeat as to obtain required cumulative time
			run_cnt = prof.flash_timings_->mass_erase_rep_;
		}
		else
			strobe_amount = prof.flash_timings_->seg_erase_;
	}
	else if (mass_erase)
	{
		// This branch can only happen if device cannot be identified
		strobe_amount = 5300;
		run_cnt = 19;	// default for this CPU generation is slow flash
	}

	HaltCpu();
	
	// LOCKA bit is always 1 after reset; setting 1 will toggle it; ignore on parts that don't have it
	uint16_t fctl3n = (prof.has_locka_) ? flags.w.fctl3_ ^ Fctl3Flags::LOCKA : flags.w.fctl3_;
	// Restore LOCKA and LOCK flash at the end
	uint16_t fctl3l = fctl3n | Fctl3Flags::LOCK;

	// Repeat operation for slow flash devices, until cumulative time has reached
	do
	{
		static constexpr TapStep steps_01[] =
		{
			kTclk0,
			kIrDr16(Ir::kCntrlSig16Bit, 0x2408),
			kIrDr16(Ir::kAddr16Bit, kFctl1Addr),		// FCTL1 address
			kIrDr16Argv(Ir::kDataToAddr,			// Enable erase "fctl1"

				kdTclkP),							// kTclk1 + kTclk0

			kIrDr16(Ir::kAddr16Bit, kFctl2Addr),		// FCTL2 address
			kIrDr16(Ir::kDataToAddr, 0xA540),		// MCLK is source, DIV=1

			kPulseTclk,								// kTclk1 + kTclk0

			kIrDr16(Ir::kAddr16Bit, kFctl3Addr),		// FCTL3 address
			kIrDr16Argv(Ir::kDataToAddr,			// Clear FCTL3; F2xxx: Unlock Info-Seg. A
													// by toggling LOCKA-Bit if required,
				kdTclkP),

			kIrDr16Argv(Ir::kAddr16Bit),				// Set erase "address"
			kIrDr16(Ir::kDataToAddr, 0x55AA),		// Dummy write to start erase
			
			kPulseTclk,

			kIrDr16(Ir::kCntrlSig16Bit, 0x2409),	// Set RW to read
			kStrobeTclkArgv,						// Provide 'strobe_amount' TCLKs
			kIrDr16(Ir::kCntrlSig16Bit, 0x2408),	// Set RW to write
			kIrDr16(Ir::kAddr16Bit, kFctl1Addr),		// FCTL1 address
			kIrDr16(Ir::kDataToAddr, kFctl1Lock),	// Disable erase
			kTclk1,
		};
		g_Player.Play(steps_01, _countof(steps_01),
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
		kIrDr16(Ir::kAddr16Bit, kFctl3Addr),		// FCTL3 address
		kIrDr16Argv(Ir::kDataToAddr,			// Lock Inf-Seg. A by toggling LOCKA (F2xxx) and set LOCK again
			kdTclk1),
		//kReleaseCpu,
	};
	g_Player.Play(steps_02, _countof(steps_02),
		fctl3l
	);
	return true;
}

// Source UIF
bool TapDev430::InstrLoad()
{
	unsigned short i = 0;

	g_Player.PlayAsync(kIrDr8(Ir::kCntrlSigLowByte, E2I(CtrlSigReg::kRead)));	// write-only; SetTCLK drains
	g_Player.SetTCLK();

	for (i = 0; i < 10; i++)
	{
		if (IsInstrLoad())
			return true;
		g_Player.PulseTCLKN();
	}
	return false;
}

// Source UIF
CtrlSigReg TapDev430::SyncJtag()
{
	CtrlSigReg lOut = CtrlSigReg::kNone;
	uint16_t i = 50;
	g_Player.SetJtagRunReadLegacy();
	do
	{
		lOut = static_cast<CtrlSigReg>((uint16_t)g_Player.DR_Shift16(0x0000));
		if (!--i)
			return CtrlSigReg::kNone;
	}
	while (IsAllSet(lOut) || IsReset(lOut, CtrlSigReg::kTce));
	return lOut;
}


bool TapDev430::ClkTclkAndCheckDTC()
{
#define MAX_DTC_CYCLE 10
	CtrlSigReg cntrl_sig;
	uint16_t dtc_cycle_cnt = 0;
	long timeOut = 0;
	do
	{
		g_Player.ClrTCLK();
		cntrl_sig = static_cast<CtrlSigReg>(g_Player.Play(kIrDr16(Ir::kCntrlSigCapture, 0)));

		if ((dtc_cycle_cnt != 0) 
			&& IsReset(cntrl_sig, CtrlSigReg::kCpuHalt))
		{
			// DTC cycle completed, take over control again...
			g_Player.PlayAsync(kIrDr16(Ir::kCntrlSig16Bit, E2I(CtrlSigReg::kTagFuncSat | CtrlSigReg::kTce1 | CtrlSigReg::kCpu)));	// write-only; SetTCLK drains
			++dtc_cycle_cnt;	// grumat: added this line, or original logic is broken
		}
		if ((dtc_cycle_cnt == 0) 
			&& IsSet(cntrl_sig, CtrlSigReg::kCpuHalt))
		{
			// DTC cycle requested, grant it...
			g_Player.PlayAsync(
				kIrDr16(
					Ir::kCntrlSig16Bit
					, E2I(CtrlSigReg::kCpuHalt | CtrlSigReg::kTce1 | CtrlSigReg::kCpu | CtrlSigReg::kTagFuncSat)
					)
				);
			++dtc_cycle_cnt;
		}
		g_Player.SetTCLK();
		++timeOut;
	}
	while ((dtc_cycle_cnt < MAX_DTC_CYCLE) 
			&& IsSet(cntrl_sig, CtrlSigReg::kCpuHalt) 
			&& timeOut < 5000);

	return (dtc_cycle_cnt < MAX_DTC_CYCLE);
}






/**************************************************************************************/
/* CPU FLOW CONTROL                                                                   */
/**************************************************************************************/

// Source: uif
bool TapDev430::SingleStep(CpuContext &ctx, const ChipProfile &prof, uint16_t mdbval)
{
	uint32_t ctrl_type = kBpCntrlIf;
	bool extra_step = 0;
	const BusWidth bus_width = prof.arch_ == ChipInfoDB::kCpu 
		? k16_bits : k32_bits;

	if (ctx.sr_ & kStatusRegCpuOff)
	{
		// If the CPU is OFF, only step after the CPU has been awakened by an interrupt.
		// This permits single step to work when the CPU is in LPM0 and 1 (as well as 2-4).
		ctrl_type = kBpCntrlNif;
		// Emulation logic requires an additional step when the CPU is OFF (but only if there is not a pending interrupt).
		if (IsSet(static_cast<CtrlSigReg>(g_Player.Play(kIrDr16(Ir::kCntrlSigCapture, 0))), CtrlSigReg::kIntrReq))
			extra_step = 1;
	}

	// Stores BKPT 0 information
	BkptSetting bkpt0;
	// Preserve breakpoint block 0
	ReadBkptSettings(bkpt0, 0, bus_width);

	BkptSetting bkpt_step =
	{
		.cntrl_ = kBpCntrlEq | kBpCntrlRwDisable | ctrl_type | kBpCntrlMab,
		.mask_ = kBpMaskDontCare,
		.combi_ = 0x0001,
		.value_ = bkpt0.value_,
		.cpustop_ = 0x0001,
	};
	// Configure "Single Step Trigger" using Trigger Block 0
	WriteBkptSettings(bkpt_step, 0, bus_width);

	ReleaseDevice(ctx, prof, true, mdbval);

	bool running = true;
	StopWatch stopwatch(TickTimer::M2T<Timer::Msec(2)>::kTicks);
	do
	{
		// Wait for EEM stop reaction
		g_Player.IR_Shift(Ir::kEmexReadControl);
		while (((g_Player.DR_Shift16(0) & 0x0080) == 0) && running)
			running = (stopwatch.IsNotElapsed());
		// Check if an extra step was required
		if (running && extra_step)
		{
			extra_step = false;
			g_Player.IR_Shift(Ir::kAddrCapture);
			g_Player.IR_Shift(Ir::kCntrlSigRelease);
		}
		else break;	// Ok, keep flag value for return status
	} while (running);

	// Restore Trigger Block 0
	WriteBkptSettings(bkpt0, 0, bus_width);
	running &= SyncJtagConditionalSaveContext(ctx, prof);

	return running;
}






/**************************************************************************************/
/* SUPPORT METHODS                                                                    */
/**************************************************************************************/


/*!
Set target CPU JTAG state machine into the instruction fetch state

\return: 1 - instruction fetch was set; 0 - otherwise
*/
bool TapDev430::SetInstructionFetch()
{
	g_Player.PlayAsync(kIrDr8(Ir::kCntrlSigLowByte, E2I(CtrlSigReg::kRead)));	// write-only; SetTCLK drains
	g_Player.SetTCLK();

	for (int i = 0; i < 10; ++i)
	{
		if (IsInstrLoad() == 0)
			return true;
		g_Player.PulseTCLKN();
	}
	return false;
}


/*!
Set the CPU into a controlled stop state
*/
void TapDev430::HaltCpu()
{
	g_TapMcu.failed_ = false;
	static constexpr TapStep steps[] =
	{
		/* Send JMP $ instruction to keep CPU from changing the state */
		kIrData16(kdNone, 0x3FFF, kdTclk0),
		// kTclk0,
		/* Set JTAG_HALT bit */
		kIrDr16(Ir::kCntrlSig16Bit, 0x2409),
		kTclk1,
	};
	g_Player.Play(steps, _countof(steps));
}


void TapDev430::ReadBkptSettings(TapDev430::BkptSetting &buf,
	const uint8_t trig_block,
	BusWidth use_32bits)
{
	static constexpr uint16_t regs[] =
	{ 
		kMxCntrl + kMxRead,
		kMxMask + kMxRead,
		kMxComb + kMxRead,
		kMxBp + kMxRead,
	};
	uint16_t offs = trig_block * kTriggerBlockSize;
	// Enter mode
	g_Player.IR_Shift(use_32bits
					? Ir::kEmexDataExchange32
					: Ir::kEmexDataExchange
					);
	// Transfer pointers
	uint32_t *pV = &buf.cntrl_;
	const uint16_t *pK = regs;
	// Use 32-bits mode?
	if (use_32bits != k16_bits)
	{
		// Copy Keys and values
		for (int i = 0; i < _countof(regs); ++i)
		{
			g_Player.DR_Shift16(offs + *pK++);
			*pV++ = g_Player.DR_Shift32(0);
		}
		// And the CPU stop mask
		g_Player.DR_Shift16(kMxCpuStop + kMxRead);
		g_Player.DR_Shift32(*pV);
	}
	else
	{
		// Copy Keys and values
		for (int i = 0; i < _countof(regs); ++i)
		{
			g_Player.DR_Shift16(offs + *pK++);
			*pV++ = g_Player.DR_Shift16(0);
		}
		// And the CPU stop mask
		g_Player.DR_Shift16(kMxCpuStop + kMxRead);
		g_Player.DR_Shift16(*pV);
	}
}


void TapDev430::WriteBkptSettings(TapDev430::BkptSetting &buf,
	const uint8_t trig_block,
	BusWidth use_32bits)
{
	static constexpr uint16_t regs[] =
	{ 
		kMxCntrl + kMxWrite,
		kMxMask + kMxWrite,
		kMxComb + kMxWrite,
		kMxBp + kMxWrite,
	};
	uint16_t offs = trig_block * kTriggerBlockSize;
	// Enter mode
	g_Player.IR_Shift(use_32bits
					? Ir::kEmexDataExchange32
					: Ir::kEmexDataExchange
					);
	// Transfer pointers
	const uint32_t *pV = &buf.cntrl_;
	const uint16_t *pK = regs;
	// Use 32-bits mode?
	if (use_32bits)
	{
		// Copy Keys and values
		for (int i = 0; i < _countof(regs); ++i)
		{
			g_Player.DR_Shift16(offs + *pK++);
			g_Player.DR_Shift32(*pV++);
		}
		// And the CPU stop mask
		g_Player.DR_Shift16(kMxCpuStop + kMxWrite);
		g_Player.DR_Shift32(*pV);
	}
	else
	{
		// Copy Keys and values
		for (int i = 0; i < _countof(regs); ++i)
		{
			g_Player.DR_Shift16(offs + *pK++);
			g_Player.DR_Shift16((uint16_t)*pV++);
		}
		// And the CPU stop mask
		g_Player.DR_Shift16(kMxCpuStop + kMxWrite);
		g_Player.DR_Shift16(*pV);
	}
}


void TapDev430::UpdateEemBreakpoints(Breakpoints &bkpts, const ChipProfile &prof)
{
	/* The breakpoint logic is explained in 'SLAU414c EEM.pdf' */
	/* A good overview is given with Figure 1-1                */
	/* MBx           is TBx         in EEM_defs.h              */
	/* CPU Stop      is kBreakReact  in EEM_defs.h              */
	/* State Storage is kStorReact  in EEM_defs.h              */
	/* Cycle Counter is kEventReact in EEM_defs.h              */

	uint16_t breakreact = bkpts.PrepareEemSetup(prof);
	
	if (breakreact == 0)
	{
		// disable all breakpoints by deleting the kBreakReact register
		g_Player.PlayAsync(kIrDr16(Ir::kEmexDataExchange, kBreakReact + kMxWrite));	// write-only; next OnDrShift16 drains
		g_Player.itf_->OnDrShift16(0x0000);
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
				kIrDr16(Ir::kEmexDataExchange, kGenCtrl + kMxWrite),
				kDr16(kEemEn + kClearStop + kEmuClkEn + kEmuFeatEn),
				// Value register
				kDr16Argv,
				kDr16Argv,
				// Control register
				kDr16Argv,
				kDr16(kMab + kTrig0 + kCmpEqual),	// instruction fetch
				// Mask register
				kDr16Argv,
				kDr16(kNoMask),
				// Combination register
				kDr16Argv,
				kDr16Argv,
			};
			g_Player.Play(steps, _countof(steps),
				bvBP + kMbTrigxVal + kMxWrite,		// value register
				bp.addr_,
				bvBP + kMbTrigxCtl + kMxWrite,		// control register
				bvBP + kMbTrigxMsk + kMxWrite,		// mask register
				bvBP + kMbTrigxCmb + kMxWrite,		// combination register
				kEn0 << bp_num
				);
		}
	}
	// This mask activates enabled breakpoints
	g_Player.itf_->OnDrShift16(kBreakReact + kMxWrite);
	g_Player.itf_->OnDrShift16(breakreact);
}

