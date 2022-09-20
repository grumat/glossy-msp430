#include "stdproj.h"

#include "TapDev430.h"
#include "eem_defs.h"
#include "TapMcu.h"


/**************************************************************************************/
/* MCU VERSION-RELATED POWER ON RESET                                                 */
/**************************************************************************************/

void TapDev430::InitDefaultChip(ChipProfile &prof)
{
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
	if ((g_Player.GetCtrlSigReg()
		 & (CNTRL_SIG_INSTRLOAD | CNTRL_SIG_READ)) != (CNTRL_SIG_INSTRLOAD | CNTRL_SIG_READ))
	{
		return false;
	}
	return true;
}


// Source UIF
bool TapDev430::SyncJtagAssertPorSaveContext(CpuContext &ctx, const ChipProfile &prof)
{
	constexpr uint16_t address = WDT_ADDR_CPU;
	uint16_t ctl_sync = 0;

	ctx.is_running_ = false;

	// Sync the JTAG
	if (g_Player.SetJtagRunReadLegacy() != kMspStd)
		return false;

	uint16_t lOut = g_Player.GetCtrlSigReg();    // read control register once

	g_Player.SetTCLK();

	if (!(lOut & CNTRL_SIG_TCE))
	{
		// If the JTAG and CPU are not already synchronized ...
		// Initiate Jtag and CPU synchronization. Read/Write is under CPU control. Source TCLK via TDI.
		// Do not effect bits used by DTC (CPU_HALT, MCLKON).
		static constexpr TapStep steps_01[] =
		{
			// initiate CPU synchronization but release low byte of CNTRL sig register to CPU control
			kIrDr8(IR_CNTRL_SIG_HIGH_BYTE, (CNTRL_SIG_TAGFUNCSAT | CNTRL_SIG_TCE1 | CNTRL_SIG_CPU) >> 8),
			// Address Force Sync special handling
			// read access to EEM General Clock Control Register (GCLKCTRL)
			kIrDr16(IR_EMEX_DATA_EXCHANGE, MX_GCLKCTRL + MX_READ),
			// read the content of GCLKCNTRL into lOut
			kDr16_ret(0),
		};

		g_Player.Play(steps_01, _countof(steps_01), &lOut);
		// Set Force Jtag Synchronization bit in Emex General Clock Control register.
		lOut |= 0x0040;							// 0x0040 = FORCE_SYN

		// Stability improvement: should be possible to remove this, required only once at the beginning
		// write access to EEM General Clock Control Register (GCLKCTRL)
		g_Player.Play(kIrDr16(IR_EMEX_DATA_EXCHANGE, MX_GCLKCTRL + MX_WRITE));
		lOut = g_Player.DR_Shift16(lOut);		// write into GCLKCNTRL

		// Reset Force Jtag Synchronization bit in Emex General Clock Control register.
		lOut &= ~0x0040;
		// Stability improvement: should be possible to remove this, required only once at the beginning
		// write access to EEM General Clock Control Register (GCLKCTRL)
		g_Player.Play(kIrDr16(IR_EMEX_DATA_EXCHANGE, MX_GCLKCTRL + MX_WRITE));
		lOut = g_Player.DR_Shift16(lOut);		// write into GCLKCNTRL

		ctl_sync = SyncJtag();

		if (!(ctl_sync & CNTRL_SIG_CPU_HALT))
		{ // Synchronization failed!
			return false;
		}
		g_Player.ClrTCLK();
		g_Player.Play(kIrDr16(IR_CNTRL_SIG_16BIT, 0x2401));	// JTAG + WORD + RD
		g_Player.SetTCLK();
	}
	else
	{
		g_Player.Play(kIrDr16(IR_CNTRL_SIG_16BIT, 0x2401));	// JTAG + WORD + RD
	}// end of if(!(lOut & CNTRL_SIG_TCE))

	// TODO: This has something to do with (activationKey == 0x20404020)
	if (prof.slau_ == ChipInfoDB::kSLAU335)
	{
		// here we add bit de assert bit 7 in JTAG test reg to enable clocks again
		lOut = g_Player.Play(kIrDr8(IR_TEST_REG, 0));
		lOut |= 0x80; //DE_ASSERT_BSL_VALID;
		g_Player.Play(kIrDr8(IR_TEST_REG, lOut));	// Bit 7 is de asserted now
	}

	// execute a dummy instruction here
	static constexpr TapStep steps_02[] =
	{
		// Stability improvement: should be possible to remove this TCLK is already 1
		kIrData16(kdTclk1, 0x4303, kdTclk0),	// kIr(IR_DATA_16BIT) + kTclk1 + kDr16(0x4303 = NOP) + kTclk
		kIr(IR_DATA_CAPTURE, kdTclk1),			// kIr(IR_DATA_CAPTURE) + kTclk1
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
			kIrDr16(IR_EMEX_DATA_EXCHANGE, MX_GENCNTRL + MX_WRITE),		// write access to EEM General Control Register (MX_GENCNTRL)
			kDr16(EMU_FEAT_EN | EMU_CLK_EN | CLEAR_STOP | EEM_EN),		// write into MX_GENCNTRL
			// Stability improvement: should be possible to remove this, required only once at the beginning
			kIrDr16(IR_EMEX_DATA_EXCHANGE, MX_GENCNTRL + MX_WRITE),		// write access to EEM General Control Register (MX_GENCNTRL)
			kDr16(EMU_FEAT_EN | EMU_CLK_EN),							// write into MX_GENCNTRL
		};
		g_Player.Play(steps, _countof(steps));
	}
	else if (prof.clk_ctrl_ == ChipInfoDB::kGccStandardI)
	{
		static constexpr TapStep steps[] =
		{
			kIrDr16(IR_EMEX_DATA_EXCHANGE, MX_GENCNTRL + MX_WRITE),		// write access to EEM General Control Register (MX_GENCNTRL)
			kDr16(EMU_FEAT_EN),											// write into MX_GENCNTRL

		};
		g_Player.Play(steps, _countof(steps));
	}

	static constexpr TapStep steps_03[] =
	{
		kTclk0,
		// Assert PUC
		kIrDr16(IR_CNTRL_SIG_16BIT, CNTRL_SIG_READ | CNTRL_SIG_TCE1 | CNTRL_SIG_PUC | CNTRL_SIG_TAGFUNCSAT),
		kTclk1,
		// Negate PUC
		kIrDr16(IR_CNTRL_SIG_16BIT, CNTRL_SIG_READ | CNTRL_SIG_TCE1 | CNTRL_SIG_TAGFUNCSAT),

		kTclk0,
		// Assert PUC
		kIrDr16(IR_CNTRL_SIG_16BIT, CNTRL_SIG_READ | CNTRL_SIG_TCE1 | CNTRL_SIG_PUC | CNTRL_SIG_TAGFUNCSAT),
		kTclk1,
		// Negate PUC
		kIrDr16(IR_CNTRL_SIG_16BIT, CNTRL_SIG_READ | CNTRL_SIG_TCE1 | CNTRL_SIG_TAGFUNCSAT),

		// Explicitly set TMR
		kDr16(CNTRL_SIG_READ | CNTRL_SIG_TCE1),			// Enable access to Flash registers

		kIrDr16(IR_FLASH_16BIT_UPDATE, FLASH_SESEL1),	// Disable flash test mode
		kDr16(FLASH_SESEL1 | FLASH_TMR),				// Pulse TMR
		kDr16(FLASH_SESEL1),
		kDr16(FLASH_SESEL1 | FLASH_TMR),				// Set TMR to user mode

		// Disable access to Flash register
		kIrDr8(IR_CNTRL_SIG_HIGH_BYTE, (CNTRL_SIG_TAGFUNCSAT | CNTRL_SIG_TCE1) >> 8),
	};
	g_Player.Play(steps_03, _countof(steps_03));

	// step until an appropriate instruction load boundary
	uint32_t i = 10;
	while (true)
	{
		lOut = g_Player.Play(kIrDr16(IR_ADDR_CAPTURE, 0x0000));
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
		kIrDr16(IR_ADDR_CAPTURE, 0x0000),
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

#if 1
	// read MAB = PC here
	ctx.pc_ = g_Player.Play(kIrDr16(IR_ADDR_CAPTURE, 0));
#else
	// UIF: Read reset vector!
	ctx.pc_ = ReadWord(0xFFFE);
#endif

	// set PC to a save address pointing to ROM to avoid RAM corruption on certain devices
	SetPC(ROM_ADDR);

	// read status register
	ctx.sr_ = GetReg(2);

	return true;
}


// Source UIF
bool TapDev430::SyncJtagConditionalSaveContext(CpuContext &ctx, const ChipProfile &prof)
{
	constexpr uint16_t address = WDT_ADDR_CPU;
	uint16_t ctl_sync = 0;
	uint16_t lOut = 0;
	uint16_t statusReg = 0;

	// syncWithRunVarAddress
	ctx.is_running_ = false;

	// Stability improvement: should be possible to remove this here, default state of TCLK should 
	// be one
	g_Player.SetTCLK();

	if (!(g_Player.GetCtrlSigReg() & CNTRL_SIG_TCE))
	{
		// If the JTAG and CPU are not already synchronized ...
		// Initiate JTAG and CPU synchronization. Read/Write is under CPU control. Source TCLK 
		// via TDI.
		// Do not effect bits used by DTC (CPU_HALT, MCLKON).
		g_Player.Play(kIrDr8(IR_CNTRL_SIG_HIGH_BYTE, (CNTRL_SIG_TAGFUNCSAT | CNTRL_SIG_TCE1 | CNTRL_SIG_CPU) >> 8));

		// A bug in first F43x and F44x silicon requires that JTAG synchronization be forced (when 
		// the CPU is Off).
		// Since the JTAG and CPU are not yet synchronized, the CPUOFF bit in the lower byte of the 
		// cntrlSig register is not valid.
		// TCE eventually set indicates synchronization (and clocking via TCLK).
		ctl_sync = SyncJtag();
		if (ctl_sync == 0)
			return false;	// Synchronization failed!
	}// end of if(!(lOut & CNTRL_SIG_TCE))

	const bool cpu_halted = (ctl_sync & CNTRL_SIG_CPU_HALT) != 0;
	if (cpu_halted)
		g_Player.ClrTCLK();
	// Clear HALT. Read/Write is under CPU control. As a precaution, disable interrupts.
	g_Player.Play(kIrDr16(IR_CNTRL_SIG_16BIT, CNTRL_SIG_TCE1 | CNTRL_SIG_CPU | CNTRL_SIG_TAGFUNCSAT));
	if (cpu_halted)
		g_Player.SetTCLK();

	// TODO: This has something to do with (activationKey == 0x20404020)
	if (prof.slau_ == ChipInfoDB::kSLAU335)
	{
		// here we add bit de assert bit 7 in JTAG test reg to enable clocks again
		lOut = g_Player.Play(kIrDr8(IR_TEST_REG, 0));
		lOut |= 0x80; //DE_ASSERT_BSL_VALID;
		g_Player.Play(kIrDr8(IR_TEST_REG, lOut));	// Bit 7 is de asserted now
	}

	// step until next instruction load boundary if not being already there
	if (!IsInstrLoad())
		return false;

	// read MAB = PC here
	ctx.pc_ = g_Player.Play(kIrDr16(IR_ADDR_CAPTURE, 0));

	if (prof.clk_ctrl_ != ChipInfoDB::kGccNone)
	{
		if (prof.clk_ctrl_ == ChipInfoDB::kGccExtended)
		{
			static constexpr TapStep steps[] =
			{
				// disable EEM and clear stop reaction
				kIrDr16(IR_EMEX_WRITE_CONTROL, CLEAR_STOP | EEM_EN),
				kDr16(0x0000),

				// write access to EEM General Control Register (MX_GENCNTRL)
				kIrDr16(IR_EMEX_DATA_EXCHANGE, MX_GENCNTRL + MX_WRITE),
				// write into MX_GENCNTRL
				kDr16(EMU_FEAT_EN | EMU_CLK_EN | CLEAR_STOP | EEM_EN),

				// Stability improvement: should be possible to remove this, required only once at the beginning
				// write access to EEM General Control Register (MX_GENCNTRL)
				kIrDr16(IR_EMEX_DATA_EXCHANGE, MX_GENCNTRL + MX_WRITE),
				// write into MX_GENCNTRL
				kDr16(EMU_FEAT_EN | EMU_CLK_EN),
			};
			g_Player.Play(steps, _countof(steps));
		}
		else if (prof.clk_ctrl_ == ChipInfoDB::kGccStandardI)
		{
			static constexpr TapStep steps[] =
			{
				// disable EEM and clear stop reaction
				kIrDr16(IR_EMEX_WRITE_CONTROL, CLEAR_STOP | EEM_EN),
				kDr16(0x0000),

				// write access to EEM General Control Register (MX_GENCNTRL)
				kIrDr16(IR_EMEX_DATA_EXCHANGE, MX_GENCNTRL + MX_WRITE),
				// write into MX_GENCNTRL
				kDr16(EMU_FEAT_EN),
			};
			g_Player.Play(steps, _countof(steps));
		}

		if (prof.stop_fll_)
		{
			// read access to EEM General Clock Control Register (GCLKCTRL)
			g_Player.Play(kIrDr16(IR_EMEX_DATA_EXCHANGE, MX_GCLKCTRL + MX_READ));
			uint16_t clkCntrl = g_Player.DR_Shift16(0);
			// added UPSF: FE427 does regulate the FLL to the upper boarder
			// added the switch off and release of FLL (JTFLLO)
			clkCntrl |= 0x10;
			g_Player.Play(kIrDr16(IR_EMEX_DATA_EXCHANGE, MX_GCLKCTRL + MX_WRITE));
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
	if (g_Player.GetCtrlSigReg() & CNTRL_SIG_INSTRLOAD)
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
	g_Player.IR_Shift(IR_CNTRL_SIG_CAPTURE);
	while (!(g_Player.DR_Shift16(0) & CNTRL_SIG_INSTRLOAD))
	{
		if (!ClkTclkAndCheckDTC())
			return false;
		// give up depending on retry counter
		if (++i == MAX_TCE1)
			return false;
		g_Player.IR_Shift(IR_CNTRL_SIG_CAPTURE);
	}

	// Read PC now!!! Only the NOP or BIS #0,R4 instruction above was clocked into the device
	// The PC value should now be (OriginalValue + 2)
	// read MAB = PC here
	ctx.pc_ = (g_Player.Play(kIrDr16(IR_ADDR_CAPTURE, 0)) - 4) & 0xFFFF;

	if (i == 0)
	{
		// An Interrupt was not detected
		//		lOut does not contain the content of the CNTRL_SIG register anymore at this point
		//		need to capture it again...different to DLLv3 sequence but don't expect any negative 
		//		effect due to recapturing
		ctx.in_interrupt_ = false;

		if (g_Player.GetCtrlSigReg() & CNTRL_SIG_CPU_OFF)
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
			statusReg |= STATUS_REG_CPUOFF;
		}
	}
	else
	{
		ctx.pc_ = g_Player.Play(kIrDr16(IR_ADDR_CAPTURE, 0));
		ctx.in_interrupt_ = true;
	}

	// DLL v2: deviceHasDTCBug
	//     Configure the DTC so that it transfers after the present CPU instruction is complete (ADC10FETCH).
	//     Save and clear ADC10CTL0 and ADC10CTL1 to switch off DTC (Note: Order matters!).

	// Regain control of the CPU. Read/Write will be set, and source TCLK via TDI.
	g_Player.Play(kIrDr16(IR_CNTRL_SIG_16BIT, CNTRL_SIG_TCE1 | CNTRL_SIG_READ | CNTRL_SIG_TAGFUNCSAT));

	// Test if we are on an instruction load boundary
	if (!InstrLoad())
		return false;

	// Hold Watchdog
	uint16_t wdtval = ctx.wdt_ | WDT_PASSWD;
	ctx.wdt_ = (uint8_t)TapDev430::ReadWord(address);	// save WDT value
	wdtval |= ctx.wdt_;									// adds the WDT stop bit
	TapDev430::WriteWord(address, wdtval);

	// set PC to a save address pointing to ROM to avoid RAM corruption on certain devices
	SetPC(ROM_ADDR);

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
		kIrDr16(IR_CNTRL_SIG_16BIT, 0x2C01),	// Apply Reset
		kDr16(0x2401),							// Remove Reset
		kPulseTclkN,							// F2xxx
		kPulseTclkN,
		kTclk0,
		kIrRet(IR_ADDR_CAPTURE),				// returns the jtag ID
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
		g_Player.Play(kIrDr16(IR_CNTRL_SIG_16BIT, 0x2C01));
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
		kIrDr16(IR_EMEX_DATA_EXCHANGE, BREAKREACT + MX_READ),
		kDr16(0x0000),
		kIrDr16(IR_EMEX_WRITE_CONTROL, EMU_FEAT_EN + EMU_CLK_EN + CLEAR_STOP + EEM_EN),
		kIr(IR_CNTRL_SIG_RELEASE),
	};
	g_Player.Play(steps, _countof(steps));
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

	ReadWords(idDataAddr, data.d16, _countof(data.d16));

	id.mcu_ver_ = data.d16[0];
	id.mcu_sub_ = 0x0000;
	id.mcu_rev_ = data.d8[2];
	id.mcu_fab_ = data.d8[3];
	id.mcu_self_ = data.d16[2];
	id.mcu_cfg_ = data.d8[13] & 0x7f;
	// read fuses
	g_Player.itf_->OnIrShift(IR_CONFIG_FUSES);
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
		kIrDr16(IR_CNTRL_SIG_16BIT, 0x3401),		// CPU has control of RW & BYTE.
		kIrData16(kdNone, 0x4030, kdTclkN),			// "mov #addr,PC" instruction
		// kPulseTclkN		// F2xxx
		kDr16Argv,									// "mov #addr,PC" instruction
		// kPulseTclkN		// F2xxx
		kIr(kdTclkN, IR_ADDR_CAPTURE, kdTclk0),		// Now the PC should be on address
		kIrDr16(IR_CNTRL_SIG_16BIT, 0x2401),		// JTAG has control of RW & BYTE.
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
		kIrDr16(IR_CNTRL_SIG_16BIT, 0x3401),
		kIrData16Argv(kdNone, kdTclk0),			// kIr(IR_DATA_16BIT) + kDr16(0x4030 | reg) + kTclk0
		kIr(IR_DATA_CAPTURE),
		kIrData16Argv(kdTclk1, kdTclk0),		// kIr(IR_DATA_16BIT) + kTclk1 + kDr16(value) + kTclk0
		kIr(IR_DATA_CAPTURE, kdTclk1),			// kIr(IR_DATA_CAPTURE) + kTclk1
		kIrData16(kdNone, 0x3ffd, kdTclk0),		// kIr(IR_DATA_16BIT) + kTclk1 + kDr16('jmp $-4') + kTclk0
		kIr(IR_DATA_CAPTURE, kdTclkP),			// kIr(IR_DATA_CAPTURE) + kPulseTclk
		kIrDr16(IR_CNTRL_SIG_16BIT, 0x2401),	// kIr(IR_CNTRL_SIG_16BIT) + kDr16(0x2401); JTAG + WORD + RD
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
		kIrDr16(IR_CNTRL_SIG_16BIT, 0x3401),
		kIrDr16Argv(IR_DATA_16BIT),
		kIr(kdTclk0, IR_DATA_CAPTURE, kdTclk1),	// kTclk0 + kIr(IR_DATA_CAPTURE) + kTclk1
		kIrData16(kdNone, 0x00fe, kdTclk0),		// address part of "mov rX, &00fe"
		kIr(IR_DATA_CAPTURE, kdTclkP),			// kIr(IR_DATA_CAPTURE) + kPulseTclk
		kTclk1,
		kDr16_ret(0),							// *data = dr16(0)
		kTclk0,
		kIrDr16(IR_CNTRL_SIG_16BIT, 0x2401),	// kIr(IR_CNTRL_SIG_16BIT) + kDr16(0x2401); JTAG + WORD + RD
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
uint16_t TapDev430::ReadWord(address_t address)
{
	HaltCpu();
	static constexpr TapStep steps[] =
	{
		kTclk0,
		kIrDr16(IR_CNTRL_SIG_16BIT, 0x2409),
		// Set address
		kIrDr16Argv(IR_ADDR_16BIT),			// dr16(address)
		kIr(IR_DATA_TO_ADDR, kdTclkP),		// kIr(IR_DATA_TO_ADDR) + kPulseTclk
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
	g_Player.Play(kIrDr16(IR_CNTRL_SIG_16BIT, 0x2409)); // Set RW to read
	for (uint32_t i = 0; i < word_count; ++i)
	{
		// Set address
		g_Player.itf_->OnIrShift(IR_ADDR_16BIT);
		g_Player.itf_->OnDrShift16(address);
		g_Player.itf_->OnIrShift(IR_DATA_TO_ADDR);
		g_Player.itf_->OnPulseTclk();
		// Fetch 16-bit data
		*buf++ = g_Player.itf_->OnDrShift16(0x0000);
		address += 2;
	}
	//g_Player.itf_->OnSetTclk(); // is also the first instruction in ReleaseCpu()
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
		kIrDr8(IR_CNTRL_SIG_LOW_BYTE, 0x08), // Set word write
		kIrDr16Argv(IR_ADDR_16BIT),				// Set address
		kIrDr16Argv(IR_DATA_TO_ADDR),			// Shift in 16 bits
		kTclk1,
		kReleaseCpu,
	};
	g_Player.Play(steps, _countof(steps),
		address,
		data
	);
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
	g_Player.Play(kIrDr16(IR_CNTRL_SIG_16BIT, 0x2408));
	for (uint32_t i = 0; i < word_count; i++)
	{
		static constexpr TapStep steps_01[] =
		{
			kIrDr16Argv(IR_ADDR_16BIT),
			kIrDr16Argv(IR_DATA_TO_ADDR),
			kPulseTclk,
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

	static constexpr TapStep steps_01[] =
	{
		kTclk0,
		kIrDr16(IR_CNTRL_SIG_16BIT, 0x2408),	// Set RW to write
		kIrDr16(IR_ADDR_16BIT, kFctl1Addr),		// FCTL1 register
		kIrDr16(IR_DATA_TO_ADDR, 0xA540),		// Enable FLASH write
			
		kPulseTclk,								// kTclk1 + kTclk0

		kIrDr16(IR_ADDR_16BIT, kFctl2Addr),		// FCTL2 register
		kIrDr16(IR_DATA_TO_ADDR, 0xA540),		// Select MCLK as source, DIV=1

		kPulseTclk,								// kTclk1 + kTclk0

		kIrDr16(IR_ADDR_16BIT, kFctl3Addr),		// FCTL3 register
		kIrDr16(IR_DATA_TO_ADDR, kFctl3Unlock),	// Clear FCTL3; F2xxx: Unlock Info-Seg.
												// A by toggling LOCKA-Bit if required,
		kIr(kdTclkP, IR_CNTRL_SIG_16BIT),		// Pulse + kIr(IR_CNTRL_SIG_16BIT)
	};
	g_Player.Play(steps_01, _countof(steps_01));

	for (uint32_t i = 0; i < word_count; i++, addr += 2)
	{
		static constexpr TapStep steps_02[] =
		{
			kDr16(0x2408),							// Set RW to write
			kIrDr16Argv(IR_ADDR_16BIT),				// Set address
			kIrDr16Argv(IR_DATA_TO_ADDR),			// Set data

			kPulseTclk,

			kIrDr16(IR_CNTRL_SIG_16BIT, 0x2409),	// Set RW to read
			kStrobeTclkArgv,						// Provide TCLKs
													// F2xxx: 29 are ok
		};
		g_Player.Play(steps_02, _countof(steps_02),
			addr,
			buf[i],
			strobes
		);
	}

	static constexpr TapStep steps_03[] =
	{
		kIrDr16(IR_CNTRL_SIG_16BIT, 0x2408),	// Set RW to write
		kIrDr16(IR_ADDR_16BIT, kFctl1Addr),		// FCTL1 register
		kIrDr16(IR_DATA_TO_ADDR, kFctl1Lock),	// Disable FLASH write

		kPulseTclk,								// kTclk1 + kTclk0

		kIrDr16(IR_ADDR_16BIT, kFctl3Addr),		// FCTL3 register
		// Lock Inf-Seg. A by toggling LOCKA and set LOCK again
		kIrDr16(IR_DATA_TO_ADDR, kFctl3Lock),
		kTclk1,
		kReleaseCpu,
	};
	g_Player.Play(steps_03, _countof(steps_03));
}



/**************************************************************************************/
/* MCU VERSION-RELATED FLASH ERASE                                                    */
/**************************************************************************************/

// Source: slau320aj
bool TapDev430::EraseFlash(address_t address, const FlashFlags flags, EraseMode mass_erase)
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
		strobe_amount = 5298;
		run_cnt = 19;	// default for this CPU generation is slow flash
	}

	HaltCpu();
	
	// LOCKA bit is always 0 after reset; setting 1 will toggle it
	uint16_t fctl3n = (prof.has_locka_) ? flags.w.fctl3_ ^ FlashFlags::LOCKA : flags.w.fctl3_;
	// Restore LOCKA and LOCK flash at the end
	uint16_t fctl3l = fctl3n | FlashFlags::LOCK;

	// Repeat operation for slow flash devices, until cumulative time has reached
	do
	{
		static constexpr TapStep steps_01[] =
		{
			kTclk0,
			kIrDr16(IR_CNTRL_SIG_16BIT, 0x2408),
			kIrDr16(IR_ADDR_16BIT, kFctl1Addr),		// FCTL1 address
			kIrDr16Argv(IR_DATA_TO_ADDR),			// Enable erase "fctl1"

			kPulseTclk,								// kTclk1 + kTclk0

			kIrDr16(IR_ADDR_16BIT, kFctl2Addr),		// FCTL2 address
			kIrDr16(IR_DATA_TO_ADDR, 0xA540),		// MCLK is source, DIV=1

			kPulseTclk,								// kTclk1 + kTclk0

			kIrDr16(IR_ADDR_16BIT, kFctl3Addr),		// FCTL3 address
			kIrDr16Argv(IR_DATA_TO_ADDR),			// Clear FCTL3; F2xxx: Unlock Info-Seg. A
													// by toggling LOCKA-Bit if required,
			kPulseTclk,

			kIrDr16Argv(IR_ADDR_16BIT),				// Set erase "address"
			kIrDr16(IR_DATA_TO_ADDR, 0x55AA),		// Dummy write to start erase
			
			kPulseTclk,

			kIrDr16(IR_CNTRL_SIG_16BIT, 0x2409),	// Set RW to read
			kStrobeTclkArgv,						// Provide 'strobe_amount' TCLKs
			kIrDr16(IR_CNTRL_SIG_16BIT, 0x2408),	// Set RW to write
			kIrDr16(IR_ADDR_16BIT, kFctl1Addr),		// FCTL1 address
			kIrDr16(IR_DATA_TO_ADDR, kFctl1Lock),	// Disable erase
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
		kIrDr16(IR_ADDR_16BIT, kFctl3Addr),		// FCTL3 address
		kIrDr16Argv(IR_DATA_TO_ADDR),			// Lock Inf-Seg. A by toggling LOCKA (F2xxx) and set LOCK again
		kTclk1,
		kReleaseCpu,
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

	g_Player.Play(kIrDr8(IR_CNTRL_SIG_LOW_BYTE, CNTRL_SIG_READ));
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
uint16_t TapDev430::SyncJtag()
{
	unsigned short lOut = 0, i = 50;
	g_Player.SetJtagRunReadLegacy();
	do
	{
		lOut = g_Player.DR_Shift16(0x0000);
		if (!--i)
			return 0;
	}
	while (((lOut == 0xFFFF) || !(lOut & 0x0200)));
	return lOut;
}


bool TapDev430::ClkTclkAndCheckDTC()
{
#define MAX_DTC_CYCLE 10
	uint16_t cntrlSig;
	uint16_t dtc_cycle_cnt = 0;
	long timeOut = 0;
	do
	{
		g_Player.ClrTCLK();
		cntrlSig = g_Player.Play(kIrDr16(IR_CNTRL_SIG_CAPTURE, 0));

		if ((dtc_cycle_cnt != 0) 
			&& ((cntrlSig & CNTRL_SIG_CPU_HALT) == 0))
		{
			// DTC cycle completed, take over control again...
			g_Player.Play(kIrDr16(IR_CNTRL_SIG_16BIT, CNTRL_SIG_TCE1 | CNTRL_SIG_CPU | CNTRL_SIG_TAGFUNCSAT));
			++dtc_cycle_cnt;	// grumat: added this line, or original logic is broken
		}
		if ((dtc_cycle_cnt == 0) 
			&& ((cntrlSig & CNTRL_SIG_CPU_HALT) == CNTRL_SIG_CPU_HALT))
		{
			// DTC cycle requested, grant it...
			g_Player.Play(kIrDr16(IR_CNTRL_SIG_16BIT, CNTRL_SIG_CPU_HALT | CNTRL_SIG_TCE1 
								  | CNTRL_SIG_CPU | CNTRL_SIG_TAGFUNCSAT));
			++dtc_cycle_cnt;
		}
		g_Player.SetTCLK();
		++timeOut;
	}
	while ((dtc_cycle_cnt < MAX_DTC_CYCLE) 
			&& ((cntrlSig & CNTRL_SIG_CPU_HALT) == CNTRL_SIG_CPU_HALT) 
			&& timeOut < 5000);

	return (dtc_cycle_cnt < MAX_DTC_CYCLE);
}






/**************************************************************************************/
/* CPU FLOW CONTROL                                                                   */
/**************************************************************************************/

bool TapDev430::SingleStep()
{
	// CPU controls RW & BYTE
	g_Player.Play(kIrDr16(IR_CNTRL_SIG_16BIT, 0x3401));

	/*
	Clock CPU until next instruction fetch cycle.
	Failure after 10 clock cycles this is more than for the longest instruction.
	*/
	g_Player.IR_Shift(IR_CNTRL_SIG_CAPTURE);
	size_t cnt = 10;
	do
	{
		g_Player.PulseTCLKN();
		if ((g_Player.DR_Shift16(0x0000) & 0x8000) == 0x8000)
			break;
	} while (--cnt > 0);
	
	// JTAG controls RW & BYTE
	g_Player.Play(kIrDr16(IR_CNTRL_SIG_16BIT, 0x2401));
	
	return (cnt > 0);
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
	g_Player.Play(kIrDr8(IR_CNTRL_SIG_LOW_BYTE, CNTRL_SIG_READ));
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
		kIrDr16(IR_CNTRL_SIG_16BIT, 0x2409),
		kTclk1,
	};
	g_Player.Play(steps, _countof(steps));
}


void TapDev430::UpdateEemBreakpoints(Breakpoints &bkpts, const ChipProfile &prof)
{
	/* The breakpoint logic is explained in 'SLAU414c EEM.pdf' */
	/* A good overview is given with Figure 1-1                */
	/* MBx           is TBx         in EEM_defs.h              */
	/* CPU Stop      is BREAKREACT  in EEM_defs.h              */
	/* State Storage is STOR_REACT  in EEM_defs.h              */
	/* Cycle Counter is EVENT_REACT in EEM_defs.h              */

	uint16_t breakreact = bkpts.PrepareEemSetup(prof);
	
	if (breakreact == 0)
	{
		// disable all breakpoints by deleting the BREAKREACT register
		g_Player.Play(kIrDr16(IR_EMEX_DATA_EXCHANGE, BREAKREACT + MX_WRITE));
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
				kIrDr16(IR_EMEX_DATA_EXCHANGE, GENCTRL + MX_WRITE),
				kDr16(EEM_EN + CLEAR_STOP + EMU_CLK_EN + EMU_FEAT_EN),
				// Value register
				kDr16Argv,
				kDr16Argv,
				// Control register
				kDr16Argv,
				kDr16(MAB + TRIG_0 + CMP_EQUAL),	// instruction fetch
				// Mask register
				kDr16Argv,
				kDr16(NO_MASK),
				// Combination register
				kDr16Argv,
				kDr16Argv,
			};
			g_Player.Play(steps, _countof(steps),
				bvBP + MBTRIGxVAL + MX_WRITE,		// value register
				bp.addr_,
				bvBP + MBTRIGxCTL + MX_WRITE,		// control register
				bvBP + MBTRIGxMSK + MX_WRITE,		// mask register
				bvBP + MBTRIGxCMB + MX_WRITE,		// combination register
				EN0 << bp_num
				);
		}
	}
	// This mask activates enabled breakpoints
	g_Player.itf_->OnDrShift16(BREAKREACT + MX_WRITE);
	g_Player.itf_->OnDrShift16(breakreact);
}

