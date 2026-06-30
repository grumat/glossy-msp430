#include "stdproj.h"

#include "TapDev430Xv2.h"
#include "TapMcu.h"
#include "eem_defs.h"
#include "res/EmbeddedResources.h"
#include "../Funclets/Interface/Interface.h"

namespace
{
ALWAYS_INLINE bool UsesFr2xxFr4xxXv2Map(JtagId jtag_id)
{
	return jtag_id == kMsp_98;
}

ALWAYS_INLINE uint16_t GetXv2WatchdogAddress(JtagId jtag_id)
{
	return UsesFr2xxFr4xxXv2Map(jtag_id) ? kWdtAddrFr41xx : kWdtAddrXv2;
}

ALWAYS_INLINE uint16_t GetXv2SysJmbO0Address(JtagId jtag_id)
{
	return UsesFr2xxFr4xxXv2Map(jtag_id) ? 0x014C : 0x018C;
}
}


/**************************************************************************************/
/* MCU VERSION-RELATED POWER ON RESET                                                 */
/**************************************************************************************/

void TapDev430Xv2::InitDefaultChip(ChipProfile &prof, JtagId jtag_id)
{
	// When the device descriptor can't be matched in the DB, fall back to a
	// representative ("big brother") of the SAME jtag_id family rather than a
	// single hardcoded part. The families differ in memory technology (Flash vs
	// FRAM), EEM size and the 1377 erratum, so picking the wrong family mis-routes
	// register access and memory reads — issue #19, where a 0x99 FRAM part was
	// wrongly defaulted to the 0x91 Flash F5418A.
	prof.DefaultMcuXv2(jtag_id);
}


bool TapDev430Xv2::GetDevice(CoreId &core_id)
{
	core_id.idDataAddr = 0x0FF0;
	assert(core_id.IsXv2());
	// Get Core identification info
	// |  MCU   |  IR  | Out  |  Data  | coreipId |
	// |--------|------|------|--------|----------|
	// | F5418A | 0xE8 | 0x91 | 0x0000 |  0x0103  |
	core_id.coreipId = gPlayer.Play(kIrDr16(Ir::kCoreIpId, 0));
	if (core_id.coreipId == 0)
	{
		Error() << "TapDev::GetDeviceXv2: invalid CoreIP ID\n";
		gTapMcu.fFailed = true;
		/* timeout reached */
		return false;
	}
	// Get device identification pointer
	if (core_id.jtagId == kMsp_95)
		StopWatch().Delay<Timer::Msec(1500)>();
	/*
	** Asks DEVICE ID: Most recent cores (slau208) will return the chip description are called TLV.
	** The start is typically:
	**		- uint8_t: Info length
	**		- uint8_t: CRC length
	**		- uint16_t: CRC16 of block
	**		- uint16_t: Device ID
	**		- uint8_t: Firmware revision
	**		- uint8_t: Hardware revision
	** The slau144 also has a TLV, but not really useful. Besides, it could be erased...
	*/
	// |  MCU   |  IR  | Out  |
	// |--------|------|------|
	// | F5418A | 0xE1 | 0x91 |
	gPlayer.IR_Shift(Ir::kDeviceId);
	uint32_t tmp = gPlayer.SetReg_20Bits(0);
	// The ID pointer is an un-scrambled 20bit value (makes the inverse of the transport layer 
	// to obtain a real mem address. Probably some historic desease)
	core_id.ipPointer = ((tmp & 0xFFFF) << 4) | (tmp >> 16);
	// MSP430F5418A:
	// |  MCU   |  Data   | ipPointer |
	// |--------|---------|-----------|
	// | F5418A | 0x00000 |  0x01A00  |
	if (core_id.ipPointer && (core_id.ipPointer & 1) == 0)
		core_id.idDataAddr = core_id.ipPointer;
	return true;
}


bool TapDev430Xv2::WaitForSynch()
{
	//             IR     DR16
	// |  MCU   | 0x28 | 0x0000 |
	// |--------|------|--------|
	// | F5418A | 0x01 | 0xD301 |
	gPlayer.IR_Shift(Ir::kCntrlSigCapture);
	uint16_t i = 50;
	do
	{
		if ((gPlayer.DR_Shift16(0) & 0x0200) != 0)
			return true;
	} while (--i > 0);
	return false;
}


// Source UIF
bool TapDev430Xv2::SyncJtagAssertPorSaveContext(CpuContext &ctx, const ChipProfile &prof)
{
	const uint16_t address = GetXv2WatchdogAddress(ctx.jtagId);

	if (ctx.jtagId == kMsp_99)
	{
		gPlayer.PlayAsync(kIrDr16(Ir::kTest3VReg, 0x40A0));	// write-only; next shift drains
		gPlayer.IR_Shift(Ir::kTestReg);
		gPlayer.DR_Shift32(0x00010000);
	}
	// -------------------------Power mode handling end ------------------------

	static constexpr TapStep steps_01[] =
	{
		// enable clock control before sync
		// switch all functional clocks to JCK = 1 and stop them
		kIrDr32(Ir::kEmexDataExchange32, kGenClkCtrl + kMxWrite),
		kDr32(kMclkSel3 + kSmclkSel3 + kAclkSel3 + kStopMclk + kStopSmclk + kStopAclk),
		// enable Emulation Clocks
		kIrDr16(Ir::kEmexWriteControl, kEmuClkEn + kEemEn),

		// release RW and BYTE control signals in low byte, set TCE1 & CPUSUSP(!!) & RW
		kIrDr16(Ir::kCntrlSig16Bit, 0x1501),
	};
	//                    IR       DR32         DR32       IR     DR16     IR     DR16
	// |  MCU   | TCLK | 0xB0 | 0x00000088 | 0x00006D8E | 0x30 | 0x0005 | 0xC8 | 0x1501 |
	// |--------|------|------|------------|------------|------|--------|------|--------|
	// | F5418A | (H)  | 0x91 | 0x00000000 | 0x0000249E | 0x91 | 0x0000 | 0x91 | 0x???? |
	gPlayer.Play(steps_01, _countof(steps_01));

	//                    IR        DR16
	// |  MCU   | TCLK | 0x28 |    0x0000     |
	// |--------|------|------|---------------|
	// | F5418A | (H)  | 0x91 | 0xD301/0xD201 |
	if (!WaitForSynch())
		return false;

	static constexpr TapStep steps_02[] =
	{
		// provide one more clock to empty the pipe
		kPulseTclkN,
		// release CPUFLUSH(=CPUSUSP) signal and apply POR signal
		kIrDr16(Ir::kCntrlSig16Bit, 0x0C01),
		kDelay1ms(40),	// a **very** conservative POR reset
		// release POR signal again
		kDr16(0x0401),	// disable fetch of CPU
	};
	//                     IR     DR16            DR16
	// |  MCU   | TCLK  | 0xC8 | 0x0C01 | 40ms | 0x0401 |
	// |--------|-------|------|--------|------|--------|
	// | F5418A | (H)LH | 0x91 | 0xD301 |      | 0xCB01 |
	gPlayer.Play(steps_02, _countof(steps_02));
	if (ctx.jtagId == kMsp_91)
	{
		static constexpr TapStep steps[] =
		{
			// Force PC so safe value memory location, JMP $
			kIrData16(kdTclk2N, SAFE_PC_ADDRESS, kdTclk2N),	// kIr(Ir::kData16Bit) + 2*kPulseTclkN + kDr16(SAFE_PC_ADDRESS) + 2*kPulseTclkN
			kIr(Ir::kDataCapture)
		};
		//                    IR                 DR16            IR
		// |  MCU   | TCLK | 0x82 |   TCLK    | 0x0004 | TCLK | 0x42 |
		// |--------|------|------|-----------|--------|------|------|
		// | F5418A | (H)  | 0x91 | L/H - L/H | 0x0000 | HLH  | 0x91 |
		gPlayer.Play(steps, _countof(steps));
	}
	else if (ctx.jtagId == kMsp_98
				|| ctx.jtagId == kMsp_99)
	{
		static constexpr TapStep steps[] =
		{
			// Force PC so safe value memory location, JMP $
			kIrData16(kdTclk2N, SAFE_PC_ADDRESS, kdTclkN),	// kIr(Ir::kData16Bit) + 2*kPulseTclkN + kDr16(SAFE_PC_ADDRESS) + kPulseTclkN
			kIr(Ir::kDataCapture)
		};
		gPlayer.Play(steps, _countof(steps));
	}
	else
	{
		static constexpr TapStep steps[] =
		{
			kPulseTclkN,
			kPulseTclkN,
			kPulseTclkN,
		};
		gPlayer.Play(steps, _countof(steps));
	}

	static constexpr TapStep steps_03[] =
	{
		// TWO more to release CPU internal POR delay signals
		kPulseTclkN,
		kPulseTclkN,
		// set CPUFLUSH signal
		kIrDr16(Ir::kCntrlSig16Bit, 0x0501),
		kPulseTclkN,
		// set EEM FEATURE enable now!!!
		kIrDr16(Ir::kEmexWriteControl, kEmuFeatEn + kEmuClkEn + kClearStop),
		// Check that sequence exits on Init State
		kIrDr16(Ir::kCntrlSigCapture, 0x0000),
	};
	//                         IR     DR16            IR     DR16     IR     DR16
	// |  MCU   |  TCLK  | 0xC8 | 0x0501 | TCLK | 0x30 | 0x000E | 0x28 | 0x0000 |
	// |--------|--------|------|--------|------|------|--------|------|--------|
	// | F5418A | (L)HLH | 0x91 | 0x4201 |  LH  | 0x91 | 0x0005 | 0x91 | 0x4201 |
	gPlayer.Play(steps_03, _countof(steps_03));

	// hold Watchdog Timer
	//                    IR     DR16     IR     DR20      IR            DR16
	// |  MCU   | TCLK | 0xC8 | 0x0501 | 0xC1 | 0x0015C | 0xA1 | TCLK | 0x0000 | TCLK |
	// |--------|------|------|------- |------|---------|------|------|--------|------|
	// | F5418A | (H)L | 0x91 | 0x4201 | 0x91 | 0x00000 | 0x91 |  HL  | 0x6904 | HLH  |
	ctx.wdt = ReadWord(address);
	uint16_t wdtval = kWdtHold | ctx.wdt;		// set original bits in addition to stop bit
	
	//                    IR     DR16     IR     DR20             IR     DR16            IR     DR16
	// |  MCU   | TCLK | 0xC8 | 0x0500 | 0xC1 | 0x0015C | TCLK | 0xA1 | 0x5A84 | TCLK | 0xC8 | 0x0501 | TCLK |
	// |--------|------|------|------- |------|---------|------|------|--------|------|------|--------|------|
	// | F5418A | (H)L | 0x91 | 0xC301 | 0x91 | 0x015C0 |  H   | 0x91 | 0x0000 |  L   | 0x91 | 0xC301 | HLH  |
	WriteWord(address, wdtval);

	// Initialize Test Memory to keep the prefetched PC/MAB consistent (slau320aj
	// §2.3.2.2.3, FRAM path): the POR parked the PC at SAFE_PC_ADDRESS (0x0004) and the
	// CPU prefetches the next words at 0x06/0x08 (MAB is +2 after sync). Writing 0x3FFF
	// (an Xv2 "JMP $" / no-access word) to both keeps the CPU cleanly self-looping at the
	// safe address, so a later SetPC + quick read lands where it should. Applies only to
	// jtag_id 0x91/0x99 (the FRAM groups TI calls out); skipped elsewhere.
	if (ctx.jtagId == kMsp_91 || ctx.jtagId == kMsp_99)
	{
		WriteWord(0x0006, 0x3FFF);
		WriteWord(0x0008, 0x3FFF);
	}

	// Capture MAB - the actual PC value is (MAB - 4)
	//                    IR      DR20
	// |  MCU   | TCLK | 0x21 | 0x00000 |
	// |--------|------|------|---------|
	// | F5418A | (H)  | 0x91 | 0x????? |
	ctx.pc = gPlayer.Play(kIrDr20(Ir::kAddrCapture, 0));

	/*****************************************/
	/* Note 1495, 1637 special handling      */
	/*****************************************/
	if (((ctx.pc & 0xFFFF) == 0xFFFE) 
		|| (ctx.jtagId == kMsp_91)
		|| (ctx.jtagId == kMsp_98)
		|| (ctx.jtagId == kMsp_99))
	{
		// Load Reset vector
		ctx.pc = ReadWord(0xFFFE) & 0x000FFFFE;
	}
	else
	{
		ctx.pc -= 4;
	}
	// Status Register should be always 0 after a POR
	ctx.sr = 0;

	//                    IR     DR16
	// |  MCU   | TCLK | 0x28 | 0x0000 |
	// |--------|------|------|--------|
	// | F5418A | (H)  | 0x91 | 0xC301 |
	if (WaitForSynch())
	{
		const ChipInfoDB::EtwCodes &eem = prof.eemTimers;
		/*
		** Per-module emulation clock control (CPUXv2). Code refactored from the
		** MSP-FET firmware (SyncJtag_AssertPor_SaveContextXv2.c + modules.h).
		** kEtKeySel/kEtClkSel are NOT EEM-exchange registers: they are written via
		** ordinary memory writes. For each module slot i, select the peripheral
		** via kEtKeySel = kEtKey | PID, then kEtClkSel = 1/0 keeps/stops its clock on
		** halt, driven by bit i of eemClkCtrl (cached kModClkCtrl0, def kModClkCtrl0Default).
		** Documented in wiki 'The-Missing-EEM-Documentation.md' (kEtKeySel/kEtClkSel).
		*/
		for (size_t i = 0; i < _countof(eem.etwCodes); ++i)
		{
			// skip empty module slots (ETWPID_EMPTY == 0); matches the UIF
			// guard and avoids selecting the null PID for unpopulated entries
			if (eem.etwCodes[i] == 0)
				continue;
			// check if module clock control is enabled for corresponding module
			uint16_t v = (ctx.eemClkCtrl & (1UL << i)) != 0;
			WriteWord(kEtKeySel, kEtKey | eem.etwCodes[i]);
			WriteWord(kEtClkSel, v);
		}
	}
	static constexpr TapStep steps_04[] =
	{
		// switch back system clocks to original clock source but keep them stopped
		kIrDr32(Ir::kEmexDataExchange32, kGenClkCtrl + kMxWrite),
		kDr32(kMclkSel0 + kSmclkSel0 + kAclkSel0 + kStopMclk + kStopSmclk + kStopAclk),
	};
	gPlayer.Play(steps_04, _countof(steps_04));

	// reset Vacant Memory Interrupt Flag inside SFRIFG1
	if (ctx.jtagId == kMsp_91)
	{
		uint16_t specialFunc = ReadWord(0x0102);
		if (specialFunc & 0x8)
		{
			SetPC(SAFE_PC_ADDRESS);

			static constexpr TapStep steps_05[] =
			{
				kIrDr16(Ir::kCntrlSig16Bit, 0x0501),
				kIr(kdTclk1, Ir::kAddrCapture),
			};
			gPlayer.Play(steps_05, _countof(steps_05));

			specialFunc &= ~0x8;
			WriteWord(0x0102, specialFunc);
		}
	}
	return true; // return status OK
}


// Source UIF
bool TapDev430Xv2::SyncJtagConditionalSaveContext(CpuContext &ctx, const ChipProfile &prof)
{
	const uint16_t address = GetXv2WatchdogAddress(ctx.jtagId);
	static constexpr uint32_t MaxCyclesForSync = 10000;	// must be defined, dependent on DMA (burst transfer)!!!

	// syncWithRunVarAddress
	ctx.fIsRunning = false;
	// -------------------------Power mode handling start ----------------------
	DisableLpmx5(prof);
	// -------------------------Power mode handling end ------------------------

	// read out EEM control register...
	// ... and check if device got already stopped by the EEM
	if (gPlayer.Play(kIrDr16(Ir::kEmexReadControl, 0)) & kEemStopped)
	{
		// do this only if the device is NOT already stopped.
		// read out control signal register first

		// check if CPUOFF bit is set
		if (IsReset(static_cast<CtrlSigReg>(gPlayer.Play(kIrDr16(Ir::kCntrlSigCapture, 0))), CtrlSigReg::kCpuOff))
		{
			// do the following only if the device is NOT in Low Power Mode
			uint32_t tbValue;
			uint32_t tbCntrl;
			uint32_t tbMask;
			uint32_t tbComb;
			uint32_t tbBreak;

			static constexpr TapStep steps_01[] =
			{
				kIr(Ir::kEmexDataExchange32),
				// Trigger Block 0 value register (val)
				kDr32(kMbTrigxVal + kMxRead + kTb0), // load val address
				// Trigger Block 0 control register
				kDr32_ret(0),								// shift in dummy 0

				// Trigger Block 0 value register (ctl)
				kDr32(kMbTrigxCtl + kMxRead + kTb0),			// load ctl address
				// Trigger Block 0 control register
				kDr32_ret(0),								// shift in dummy 0

				// Trigger Block 0 value register (msk)
				kDr32(kMbTrigxMsk + kMxRead + kTb0),			// load msk address
				// Trigger Block 0 control register
				kDr32_ret(0),								// shift in dummy 0

				// Trigger Block 0 value register (cmb)
				kDr32(kMbTrigxCmb + kMxRead + kTb0),			// load cmb address
				// Trigger Block 0 control register
				kDr32_ret(0),								// shift in dummy 0

				// Trigger Block 0 value register (break)
				kDr32(kBreakReact + kMxRead),					// load break address
				// Trigger Block 0 control register
				kDr32_ret(0),								// shift in dummy 0

				// now configure a trigger on the next instruction fetch
				kDr32(kMbTrigxCtl + kMxWrite + kTb0),
				kDr32(kCmpEqual + kTrig0 + kMab),
				kDr32(kMbTrigxMsk + kMxWrite + kTb0),
				kDr32(kMaskAll),
				kDr32(kMbTrigxCmb + kMxWrite + kTb0),
				kDr32(kEn0),
				kDr32(kBreakReact + kMxWrite),
				kDr32(kEn0),

				// enable EEM to stop the device
				kDr32(kGenClkCtrl + kMxWrite),
				kDr32(kMclkSel0 + kSmclkSel0 + kAclkSel0 + kStopMclk + kStopSmclk + kStopAclk),
				kIrDr16(Ir::kEmexWriteControl, kEmuClkEn + kClearStop + kEemEn),
			};
			gPlayer.Play(steps_01, _countof(steps_01),
						  &tbValue,
						  &tbCntrl,
						  &tbMask,
						  &tbComb,
						  &tbBreak
						  );
			{
				int lTimeout = 3000;
				do
				{
					if(!(gPlayer.Play(kIrDr16(Ir::kEmexReadControl, 0)) & kEemStopped))
						break;
				}
				while (--lTimeout > 0);
			}

			// restore the setting of Trigger Block 0 previously stored
			// Trigger Block 0 value register
			static constexpr TapStep steps_02[] =
			{
				kIr(Ir::kEmexDataExchange32),
				kDr32(kMbTrigxVal + kMxWrite + kTb0),
				kDr32Argv,							// tbValue
				kDr32(kMbTrigxCtl + kMxWrite + kTb0),
				kDr32Argv,							// tbCntrl
				kDr32(kMbTrigxMsk + kMxWrite + kTb0),
				kDr32Argv,							// tbMask
				kDr32(kMbTrigxCmb + kMxWrite + kTb0),
				kDr32Argv,							// tbComb
				kDr32(kBreakReact + kMxWrite),
				kDr32Argv,							// tbBreak
			};
			gPlayer.Play(steps_02, _countof(steps_02),
						  tbValue,
						  tbCntrl,
						  tbMask,
						  tbComb,
						  tbBreak
			);
		}
	}
	// End: special handling note 822
	//------------------------------------------------------------------------------

	// enable clock control before sync
	gPlayer.PlayAsync(kIrDr16(Ir::kEmexWriteControl, kEmuClkEn + kEemEn));	// write-only; SyncJtagXv2's first shift drains

	// sync device to JTAG
	SyncJtagXv2();

	// reset CPU stop reaction - CPU is now under JTAG control
	// Note: does not work on F5438 due to note 772, State storage triggers twice on single stepping
	static constexpr TapStep steps_03[] =
	{
		kIrDr16(Ir::kEmexWriteControl, kEmuClkEn + kClearStop + kEemEn),
		kDr16(kEmuClkEn + kClearStop),
		kIrDr16(Ir::kCntrlSig16Bit, 0x1501),
		// clock system into Full Emulation State now...
		// while checking control signals CPUSUSP (pipe_empty), CPUOFF and HALT
		kIr(Ir::kCntrlSigCapture),
		kDr16_ret(0),
	};
	uint16_t lOut;
	gPlayer.Play(steps_03, _countof(steps_03), &lOut);

	///////////////////////////////////////////
	// Note 805: check if wait is set
	if ((lOut & 0x8) == 0x8)
	{
		uint32_t TimeOut = 0;
		// wait until wait is end
		while ((lOut & 0x8) == 0x8 && TimeOut++ < 30000)
		{
			static constexpr TapStep steps[] =
			{
				// falling + rising edge clock
				kIr(kdTclkN, Ir::kCntrlSigCapture),
				kDr16_ret(0),
			};
			gPlayer.Play(steps, _countof(steps), &lOut);
		}
	}
	// Note 805 end: Florian, 21 Dec 2010
	///////////////////////////////////////////

	bool pipe_empty = false;
	gPlayer.IR_Shift(Ir::kCntrlSigCapture);
	uint32_t i = 0;
	do 
	{
		gPlayer.ClrTCLK();		// provide falling clock edge
		// shift out current control signal register value
		pipe_empty = IsSet(ShiftCtrlSigReg(), CtrlSigReg::kCpuSusp);
		gPlayer.SetTCLK();		// provide rising clock edge
	}
	while (!pipe_empty && i < MaxCyclesForSync);

	//! \todo check error condition
	if (i >= MaxCyclesForSync)
	{
		;
	}

	static constexpr TapStep steps_04[] =
	{
		// the interrupts will be disabled now - JTAG takes over control of the control signals
		kIrDr16(Ir::kCntrlSig16Bit, 0x0501),

		// provide 1 clock in order to have the data ready in the first transaction slot
		kIr(kdTclkN, Ir::kAddrCapture),
		kDr20_ret(0),
		// shift out current control signal register value
		kIr(Ir::kCntrlSigCapture),
		kDr16_ret(0),
	};
	uint32_t lOut_long;
	gPlayer.Play(steps_04, _countof(steps_04)
				  , &lOut_long
				  , &lOut
	);

	bool cpuoff = IsSet(static_cast<CtrlSigReg>(lOut), CtrlSigReg::kCpuOff);

	ctx.fInInterrupt = (lOut & 0x4) != 0;	// undocumented!?!

	// adjust program counter according to control signals
	lOut_long -= cpuoff ? 2 : 4;

	/********************************************************/
	/* Note 1495, 1637 special handling for program counter */
	/********************************************************/
	if ((lOut_long & 0xFFFF) == 0xFFFE)
		ctx.pc = TapDev430Xv2::ReadWord(0xFFFE);
	else
		ctx.pc = lOut_long;

	static constexpr TapStep steps_05[] =
	{
		// set EEM FEATURE enable now!!!
		kIrDr16(Ir::kEmexWriteControl, kEmuFeatEn + kEmuClkEn + kClearStop),
		// check for Init State
		kIrDr16(Ir::kCntrlSigCapture, 0),
	};
	gPlayer.Play(steps_05, _countof(steps_05));

	// Hold Watchdog
	uint16_t wdtval = ctx.wdt | kWdtPasswd;
	ctx.wdt = (uint8_t)TapDev430Xv2::ReadWord(address);	// save WDT value
	wdtval |= ctx.wdt;										// adds the WDT stop bit
	TapDev430Xv2::WriteWord(address, wdtval);

	ctx.sr = GetReg(2);
	SetReg(2, ctx.sr & 0xFFE7);	// clear CPUOFF/GIE bit

	return true;
}


//----------------------------------------------------------------------------
//! \brief Function to execute a Power-On Reset (POR) using JTAG CNTRL SIG 
//! register
//! \return word (STATUS_OK if target is in Full-Emulation-State afterwards,
//! STATUS_ERROR otherwise)
//! Source: slau320aj
bool TapDev430Xv2::ExecutePOR()
{
	const uint16_t address = gTapMcu.IsFr41xx() ? kWdtAddrFr41xx : kWdtAddrXv2;
	static constexpr TapStep steps[] =
	{
		kIr(Ir::kCntrlSigCapture, kdTclkN),
		// provide one clock cycle to empty the pipe

		// prepare access to the JTAG CNTRL SIG register
		// release CPUSUSP signal and apply POR signal
		kIrDr16(Ir::kCntrlSig16Bit, 0x0C01),
		// release POR signal again
		kDr16(0x0401),
		kPulseTclkN,
		kPulseTclkN,
		kPulseTclkN,
		// two more to release CPU internal POR delay signals
		kPulseTclkN,
		kPulseTclkN,
		// now set CPUSUSP signal again
		kIrDr16(Ir::kCntrlSig16Bit, 0x0501),
		// and provide one more clock
		kPulseTclkN,
	};
	gPlayer.Play(steps, _countof(steps));
	// the CPU is now in 'Full-Emulation-State'

	// disable Watchdog Timer on target device now by setting the HOLD signal
	// in the WDT_CNTRL register
	TapDev430Xv2::WriteWord(address, kWdtHold);

	// Check if device is in Full-Emulation-State again and return status
	if (gPlayer.GetCtrlSigReg() & 0x0301)
		return true;

	return false;
}





/**************************************************************************************/
/* DEVICE IDENTIFICATION METHODS                                                      */
/**************************************************************************************/

static uint16_t get_subid_(const uint8_t *tlv, uint32_t tlv_size)
{
	constexpr uint8_t SUBVERSION_TAG = 0x14;
	constexpr uint8_t UNDEFINED_FF = 0xFF;
	constexpr uint8_t UNDEFINED_00 = 0x00;

	uint32_t pos = 8;
	// Must have at least 2 byte data left
	while (pos + 3 < tlv_size)
	{
		const uint8_t tag = tlv[pos++];
		const uint8_t len = tlv[pos++];
		const uint8_t *value = &tlv[pos];
		pos += len;

		if (tag == SUBVERSION_TAG)
			return ((uint16_t)value[1] << 8) + value[0];
		if ((tag == UNDEFINED_FF) || (tag == UNDEFINED_00) || (tag == SUBVERSION_TAG))
			break;
	}
	return UNDEFINED_00;
}


// Source: UIF
bool TapDev430Xv2::GetDeviceSignature(DieInfo &id, CpuContext &ctx, const CoreId &coreid)
{
	if (coreid.idDataAddr == 0)
	{
		return false;
	}
	union
	{
		uint16_t d16[4];
		uint8_t d8[8];
	} data;
	bool status = false;

	ReadWords(coreid.idDataAddr, data.d16, _countof(data.d16));
	Debug() << "Xv2 raw signature:\n"
		"  id_data_addr: 0x" << f::X<4>(coreid.idDataAddr) << "\n"
		"  raw[0]:       0x" << f::X<4>(data.d16[0]) << "\n"
		"  raw[1]:       0x" << f::X<4>(data.d16[1]) << "\n"
		"  raw[2]:       0x" << f::X<4>(data.d16[2]) << "\n"
		"  raw[3]:       0x" << f::X<4>(data.d16[3]) << "\n";
	
	id.mcuVer = data.d16[2];
	id.mcuSub = 0x0000; // init with zero = no sub id
	id.mcuRev = data.d8[6]; // HW Revision
	id.mcuCfg = data.d8[7]; // SW Revision
	id.mcuFab = 0x55;
	id.mcuSelf = 0x5555;
	id.mcuFuse = 0x55;

	uint8_t info_len = data.d8[0];
	if ((info_len > 1) && (info_len < 11))
	{
		uint32_t tlv_size = 4 * (1 << data.d8[0]) - 2;
		uint8_t tlv[tlv_size];

		ReadWords(coreid.idDataAddr, (uint16_t *)(void*)tlv, tlv_size / 2);
		id.mcuSub = get_subid_(tlv, tlv_size);
	}

	ctx.eemVersion = EemDataExchangeXv2(0x87, ctx);

	status = true;
error_exit:
	if (!SetPC(ctx.pc))
		return false;
	return status;
}



/**************************************************************************************/
/* MCU VERSION-RELATED REGISTER GET/SET METHODS                                       */
/**************************************************************************************/

//----------------------------------------------------------------------------
//! \brief Load a given address into the target CPU's program counter (PC).
//! \param[in] uint32_t address (destination address)
//! 
//!  Source: slau320aj
bool TapDev430Xv2::SetPC(address_t address)
{
	const uint16_t Mova = 0x0080
		| (uint16_t)((address >> 8) & 0x00000F00);
	const uint16_t Pc_l = (uint16_t)address;

	// Check Full-Emulation-State at the beginning
	if (gPlayer.GetCtrlSigReg() & 0x0301)
	{
		static constexpr TapStep steps[] =
		{
			// MOVA #imm20, PC
			kTclk0,
			// take over bus control during clock LOW phase
			kIrData16Argv(kdTclk1, kdTclk0),	// kIr(Ir::kData16Bit) + kTclk1 + kDr16(Mova) + kTclk0
			// above is just for delay
			kIrDr16(Ir::kCntrlSig16Bit, 0x1400),
			kIrData16Argv(kdTclkN, kdTclkN),	// kIr(Ir::kData16Bit) + kPulseTclkN + kDr16(Pc_l) + kPulseTclkN
			kDr16(0x4303),						// NOP
			kTclk0,
			kIrDr20(Ir::kAddrCapture, 0),
		};
		gPlayer.Play(steps, _countof(steps),
			 Mova,
			 Pc_l
		);
	}
	return true;
}


// Source:  uif
bool TapDev430Xv2::SetReg(uint8_t reg, uint32_t value)
{
	uint16_t mova = 0x0080;
	mova += (uint16_t)((value >> 8) & 0x00000F00);
	mova += (reg & 0x000F);
	uint16_t rx_l = (uint16_t)value;

	static constexpr TapStep steps[] =
	{
		kTclk0,
		kIrData16Argv(kdTclk1),				// kIr(Ir::kData16Bit) + kTclk1 + kDr16(mova)
		kIrDr16(Ir::kCntrlSig16Bit, 0x1401),
		kIrData16Argv(kdTclkN, kdTclkN),	// kIr(Ir::kData16Bit) + kPulseTclkN + kDr16(rx_l) + kPulseTclkN
		kDr16(0x3ffd),						// jmp $-4
		kPulseTclkN,
		kTclk0,
		kIrDr20(Ir::kDataCapture, 0x00000),	// kIr(Ir::kDataCapture) + kDr16(0x00000)
		kTclk1,
		kIrDr16(Ir::kCntrlSig16Bit, 0x0501),
		kPulseTclkN,
		kIr(kdTclk0, Ir::kDataCapture, kdTclk1),
	};
	gPlayer.Play(steps, _countof(steps),
		mova,
		rx_l
	);
	return true;
}


// Source: uif
uint32_t TapDev430Xv2::GetRegInternal(uint8_t reg)
{
	const uint16_t Mova = 0x0060
		| ((uint16_t)reg << 8) & 0x0F00;

	JtagId jtagId = (JtagId)(uint8_t)gPlayer.itf_->OnIrShift(Ir::kCntrlSigCapture);
	const uint16_t jmbAddr = GetXv2SysJmbO0Address(jtagId);

	uint16_t Rx_l = 0xFFFF;
	uint16_t Rx_h = 0xFFFF;
	static constexpr TapStep steps[] =
	{
		kTclk0,
		kIrData16Argv(kdTclk1),					// kIr(Ir::kData16Bit) + kTclk1 + kDr16(Mova)
		kIrDr16(Ir::kCntrlSig16Bit, 0x1401),	// RD + JTAGCTRL + RELEASE_LBYTE:01
		kIrData16Argv(kdTclkN, kdTclkN),		// kIr(Ir::kData16Bit) + kPulseTclkN + kDr16(jmbAddr) + kPulseTclkN
		kDr16(0x3ffd),							// jmp $-4
		kIr(kdTclk0, Ir::kDataCapture, kdTclk1),
		kDr16_ret(0),							// Rx_l = dr16(0)
		kPulseTclkN,
		kDr16_ret(0),							// Rx_h = dr16(0)
		kPulseTclkN,
		kPulseTclkN,
		kPulseTclkN,
		kIrDr16(Ir::kCntrlSig16Bit, 0x0501),	// Set Word read CpuXv2
		kIr(kdTclk0, Ir::kDataCapture, kdTclk1),
	};
	gPlayer.Play(steps, _countof(steps),
		 Mova,
		 jmbAddr,
		 &Rx_l,
		 &Rx_h
	);
	gPlayer.itf_->OnReadJmbOut();

	return (((uint32_t)Rx_h << 16) | Rx_l) & 0xfffff;
}


bool TapDev430Xv2::GetRegs_Begin()
{
	back_r0_ = GetRegInternal(0);
	return true;
}


uint32_t TapDev430Xv2::GetReg(uint8_t reg)
{
	if (reg == 0)
		return back_r0_;
	return GetRegInternal(reg);
}


void TapDev430Xv2::GetRegs_End()
{
	SetPC(back_r0_ - 4);
}



/**************************************************************************************/
/* MCU VERSION-RELATED READ MEMORY METHODS                                            */
/**************************************************************************************/

#if 0
uint8_t TapDev430Xv2::ReadByte(address_t address)
{
	static constexpr TapStep steps[] =
	{
		kTclk0,
		// set word read
		kIrDr16(Ir::kCntrlSig16Bit, 0x0511),
		// Set address
		kIrDr20Argv(Ir::kAddr16Bit),		// dr16(address)
		kIr(Ir::kDataToAddr, kdTclkP),
		// shift out 16 bits
		kDr16_ret(0x0000),				// content = dr16(0x0000)
		kPulseTclk,
		kTclk1,							// is also the first instruction in ReleaseCpu()
	};
	uint16_t content = 0xFFFF;
	gPlayer.Play(steps, _countof(steps),
		address,
		&content
	);
	return content;
}
#else
uint8_t TapDev430Xv2::ReadByte(address_t address)
{
	bool unaligned = (address & 1);
	uint16_t data = ReadWord(address & 0x000FFFFE);
	if (unaligned)
		data >>= 8;
	return (uint8_t)data;
}
#endif


void TapDev430Xv2::ReadBytes(address_t address, uint8_t *buf, uint32_t byte_count)
{
	while (byte_count--)
		*buf++ = TapDev430Xv2::ReadByte(address++);
}


#if 1
// Source: slau320aj
uint16_t TapDev430Xv2::ReadWord(address_t address)
{
	/*
	Important: This routine fails to read SFR registers
	*/
	static constexpr TapStep steps[] =
	{
		kTclk0,
		// set word read
		kIrDr16(Ir::kCntrlSig16Bit, 0x0501),	// removed as capture is done before
		// Set address
		kIrDr20Argv(Ir::kAddr16Bit),		// dr16(address)
		kIr(Ir::kDataToAddr, kdTclkP),
		// shift out 16 bits
		kDr16_ret(0x0000),				// content = dr16(0x0000)
		kPulseTclk,
		kTclk1,							// is also the first instruction in ReleaseCpu()
	};
	uint16_t content = 0xFFFF;
	//                    IR     DR16     IR     DR20      IR            DR16
	// |  MCU   | TCLK | 0xC8 | 0x0501 | 0xC1 | 0x????? | 0xA1 | TCLK | 0x0000 | TCLK |
	// |--------|------|------|------- |------|---------|------|------|--------|------|
	// | F5418A | (H)L | 0x91 | 0x4201 | 0x91 | 0x00000 | 0x91 |  HL  | 0x???? | HLH  |
	gPlayer.Play(steps, _countof(steps),
		address,
		&content
	);
	return content;
}
#elif 0
// Source: uif
uint16_t TapDev430Xv2::ReadWord(address_t address)
{
	/*
	This routine fails even when detecting the chip
	*/
	static constexpr TapStep steps[] =
	{
		kTclk0,
		// set word read
		kIrDr16(Ir::kCntrlSig16Bit, 0x0501),
		// Set address
		kIrDr20Argv(Ir::kAddr16Bit), // dr16(address)
		kIr(kdTclkP, Ir::kDataCapture),
		// shift out 16 bits
		kDr16_ret(0x0000), // content = dr16(0x0000)
		kTclk1,
		kPulseTclkN,
	};
	uint16_t content = 0xFFFF;
	gPlayer.Play(steps,
		_countof(steps),
		address,
		&content);
	return content;
}
#else
uint16_t TapDev430Xv2::ReadWord(address_t address)
{
	uint16_t data;
	ReadWords(address, &data, 1);
	return data;
}
#endif

//! Source: slau320aj "Data Quick" auto-increment read — ONE strategy for all Xv2.
//!
//! Previously FRAM parts (jtag_id 0x98/0x99) were special-cased to re-issue a full
//! SetPC + Data Quick per word, on the theory that auto-increment "derails" at the
//! 0x1A00 device descriptor (raw[0] real, raw[1..3] = 0x3fff). The FR5994 eZ-FET
//! golden reference (.claude/docs/msp430/FR5994_SBW_GOLDEN_REFERENCE.md, seq 59–78)
//! disproves that: the stock MSP-FET reads the 0x1A00 descriptor on jtag_id 0x99 with
//! a SINGLE SetPC(0x1A00) then plain auto-increment Data Quick, one TCLK edge per word
//! (TCLK phase H · L · HL · HL · HL). The per-word SetPC was the misdiagnosis behind
//! issue #19 — each re-navigation re-strobes TCLK and re-fetches, corrupting the read.
//! Collapsing onto the proven auto-increment path makes our wire identical to the
//! reference. (If a bench still shows raw[1..3] derail, the residual cause is the SBW
//! transport's OnPulseTclk falling edge not advancing the CPU fetch — verify against
//! the golden-reference TCLK string with supp/docs-ai/decode_sbw_fsm.py.)
void TapDev430Xv2::ReadWords(address_t address, unaligned_u16 *buf, uint32_t word_count)
{
	// proven slau320aj "Data Quick" auto-increment read (all Xv2 jtag_ids)
	TapDev430Xv2::SetPC(address);

	static constexpr TapStep steps[] =
	{
		kTclk1,
		// set word read
		kIrDr16(Ir::kCntrlSig16Bit, 0x0501),
		// Set address
		kIr(Ir::kAddrCapture),
		kIr(Ir::kDataQuick),
	};
	gPlayer.Play(steps, _countof(steps));

	for (uint32_t i = 0; i < word_count; ++i)
	{
		gPlayer.itf_->OnPulseTclk();
		*buf++ = gPlayer.DR_Shift16(0);  // Read data from memory.
	}

	gPlayer.SetTCLK();
}


/**************************************************************************************/
/* MCU VERSION-RELATED WRITE MEMORY METHODS                                           */
/**************************************************************************************/

//----------------------------------------------------------------------------
//! \brief This function writes one byte/word at a given address ( <0xA00)
//! \param[in] word address (Address of data to be written)
//! \param[in] word data (shifted data)
//! Source: slau320aj
void TapDev430Xv2::WriteWord(address_t address, uint16_t data)
{
	// Check Init State at the beginning
	static constexpr TapStep steps[] =
	{
		kTclk0,
		// set word read
		kIrDr16(Ir::kCntrlSig16Bit, 0x0500),
		// Set address
		kIrDr20Argv(Ir::kAddr16Bit),			// dr16(address)
		kIrDr16Argv(kdTclk1,
			// New style: Only apply data during clock high phase
			Ir::kDataToAddr,				// dr16(data)
			kdTclk0),
		kIrDr16(Ir::kCntrlSig16Bit, 0x0501),
		kTclk1,
		// one or more cycle, so CPU is driving correct MAB
		kPulseTclkN,
	};
	gPlayer.Play(steps, _countof(steps),
		address,
		data
	);
	// Processor is now again in Init State
}


//----------------------------------------------------------------------------
//! \brief This function writes an array of words into the target memory.
//! \param[in] word address (Start address of target memory)
//! \param[in] word word_count (Number of words to be programmed)
//! \param[in] word *buf (Pointer to array with the data)
//! Source: slau320aj
void TapDev430Xv2::WriteWords(address_t address, const unaligned_u16 *buf, uint32_t word_count)
{
	for (uint32_t i = 0; i < word_count; i++)
	{
		TapDev430Xv2::WriteWord(address, *buf++);
		address += 2;
	}
}



/**************************************************************************************/
/* MCU VERSION-RELATED WRITE MEMORY METHODS                                           */
/**************************************************************************************/

// Source: slau320aj
void TapDev430Xv2::WriteFlash(address_t address, const unaligned_u16 *data, uint32_t word_count)
{
	const uint32_t total_size = EmbeddedResources::___Firmware_shared_res_WriteFlashXv2_bin.size() / sizeof(uint16_t);
	const MemInfo &mem = gTapMcu.GetChipProfile().GetRamMem();
#if 0
	const address_t ctrlAddr = mem.start + EmbeddedResources::res_WriteFlashXv2_bin.size();
#endif

	// Save a backup of a register range (check funclet ASM to fine tune this)
	constexpr int kStartReg = 12;
	constexpr int kNumRegs = 3;
	uint32_t regs[kNumRegs];
	GetRegs_Begin();
	for (int i = 0; i < kNumRegs; ++i)
		regs[i] = GetRegInternal(kStartReg + i);
	GetRegs_End();

	// Backup RAM here
	uint16_t backup[total_size];
	TapDev430Xv2::ReadWords(mem.start, backup, total_size);
	
	// Install funclet
	TapDev430Xv2::WriteWords(mem.start
		, (const uint16_t *)EmbeddedResources::___Firmware_shared_res_WriteFlashXv2_bin.data()
		, EmbeddedResources::___Firmware_shared_res_WriteFlashXv2_bin.size()/sizeof(uint16_t));
	// Pass parameters
	SetReg(12, (uint32_t)address);
	SetReg(13, word_count);
	// Run funclet
	TapDev430Xv2::ReleaseDevice(mem.start);

	bool success = true;
	StopWatch stopwatch(TickTimer::M2T<Timer::Msec(100)>::kTicks);
	// Wait until funclet signals startup
	do
	{
		if (gPlayer.i_ReadJmbOut() == 0xABADBABE)
			break;
		success = (stopwatch.IsNotElapsed());
	} while (success);

	if (success)
	{
		// Transfer words while funclet writes them
		for (uint32_t i = 0; i < word_count; ++i)
			gPlayer.i_WriteJmbIn16(data[i]);
		// Wait for termination
		stopwatch.Start<Timer::Msec(100)>();
		do
		{
			if (gPlayer.i_ReadJmbOut() == 0xCAFEBABE)
				break;
			success = (stopwatch.IsNotElapsed());
		} while (success);
	}

	TapDev430Xv2::SyncJtagXv2();

	// Restore RAM contents
	TapDev430Xv2::WriteWords(mem.start, backup, total_size);

	// Restore register backup
	for (int i = 0; i < kNumRegs; ++i)
		SetReg(kStartReg + i, regs[i]);
	
	//return success;
}



/**************************************************************************************/
/* MCU VERSION-RELATED FLASH ERASE                                                    */
/**************************************************************************************/


// Source: slau320aj
bool TapDev430Xv2::EraseFlash(address_t address, const FlashEraseFlags flags, EraseMode mass_erase)
{
	(void)mass_erase;
	
	EraseCtrlXv2 ctrlData;
	const uint32_t total_size = (EmbeddedResources::___Firmware_shared_res_EraseXv2_bin.size() + sizeof(ctrlData)) / sizeof(uint16_t);

	const MemInfo &mem = gTapMcu.GetChipProfile().GetRamMem();
	address_t ctrlAddr = mem.start + EmbeddedResources::___Firmware_shared_res_EraseXv2_bin.size();

	// Save a backup of a register range (check funclet ASM to fine tune this)
	constexpr int kStartReg = 11;
	constexpr int kNumRegs = 5;
	uint32_t regs[kNumRegs];
	GetRegs_Begin();
	for (int i = 0; i < kNumRegs; ++i)
		regs[i] = GetRegInternal(kStartReg + i);
	GetRegs_End();
	
	// backup RAM here
	uint16_t backup[total_size];
	TapDev430Xv2::ReadWords(mem.start, backup, total_size);

	ctrlData.addr_ = address;			// set dummy write address
	ctrlData.fctl1 = flags.w.fctl1;		// set erase mode
	ctrlData.fctl3 = flags.w.fctl3;		// FCTL3: lock/unlock INFO Segment A

	TapDev430Xv2::WriteWords(mem.start, (uint16_t *)EmbeddedResources::___Firmware_shared_res_EraseXv2_bin.data(), EmbeddedResources::___Firmware_shared_res_EraseXv2_bin.size() / sizeof(uint16_t));
	TapDev430Xv2::WriteWords(ctrlAddr, (uint16_t *)&ctrlData, sizeof(ctrlData) / sizeof(uint16_t));
	// R12 points to the control data
	TapDev430Xv2::SetReg(12, ctrlAddr);
	// Release device and wait for JMB signal
	TapDev430Xv2::ReleaseDevice(mem.start);

	// Wait until funclet erases flash
	bool success = true;
	StopWatch stopwatch(TickTimer::M2T<Timer::Msec(300)>::kTicks);
	// repeat while not a timeout
	do
	{
		// Expected message received?
		if (gPlayer.i_ReadJmbOut() == 0xCAFEBABE)
			break;
		success = (stopwatch.IsNotElapsed());
	}
	while(success);

	// Capture MCU
	TapDev430Xv2::SyncJtagXv2();

	// Restore RAM
	TapDev430Xv2::WriteWords(mem.start, backup, total_size);

	// Restore register backup
	for (int i = 0; i < kNumRegs; ++i)
		SetReg(kStartReg + i, regs[i]);
	
	return success;
}




#if 0
//----------------------------------------------------------------------------
//! \brief Function to resync the JTAG connection and execute a Power-On-Reset
//! \return true if operation was successful, false otherwise)
bool TapDev430Xv2::SyncJtag()
{
	uint32_t i = 0;

	gPlayer.PlayAsync(kIrDr16(Ir::kCntrlSig16Bit, 0x1501));  // Set device into JTAG mode + read (write-only; next shift drains)

	uint8_t jtag_id = gPlayer.IR_Shift(Ir::kCntrlSigCapture);

	if ((jtag_id != JTAG_ID91) && (jtag_id != JTAG_ID99))
	{
		return false;
	}
	// wait for sync
	while (!(gPlayer.DR_Shift16(0) & 0x0200) && i < 50)
	{
		i++;
	};
	// continues if sync was successful
	if (i >= 50)
		return false;

	// execute a Power-On-Reset
	if (TapDev430Xv2::ExecutePOR() == false)
		return false;

	return true;
}
#endif

void TapDev430Xv2::DisableLpmx5(const ChipProfile &prof)
{
	if (prof.pwr_settings_ == NULL)
		return;
	if (prof.pwr_settings_->testReg3vMask)
	{
		uint16_t reg_3V = gPlayer.Play(kIrDr16(Ir::kTest3VReg, prof.pwr_settings_->testReg3vDefault));

		gPlayer.DR_Shift16(reg_3V & ~prof.pwr_settings_->testReg3vMask
							| prof.pwr_settings_->testReg3vDisableLpm5);
		StopWatch().Delay<Timer::Msec(20)>();
	}

	if (prof.pwr_settings_->testRegMask)
	{
		gPlayer.IR_Shift(Ir::kTestReg);
		uint32_t reg_test = gPlayer.DR_Shift32(prof.pwr_settings_->testRegDefault);
		gPlayer.DR_Shift32(reg_test & ~prof.pwr_settings_->testRegMask
							| prof.pwr_settings_->testRegDisableLpm5);
		StopWatch().Delay<Timer::Msec(20)>();
	}
}


void TapDev430Xv2::SyncJtagXv2()
{
	gPlayer.PlayAsync(kIrDr16(Ir::kCntrlSig16Bit, 0x1501));	// write-only; next shift drains
	gPlayer.IR_Shift(Ir::kCntrlSigCapture);
	int i = 50;
	do
	{
		uint16_t lOut = gPlayer.DR_Shift16(0);
		if (lOut != 0xFFFF && (lOut & 0x0200) != 0)
			break;
	}
	while (--i >= 0);
}



/**************************************************************************************/
/* MCU VERSION-RELATED DEVICE RELEASE                                                 */
/**************************************************************************************/

// Source: slau320aj
void TapDev430Xv2::ReleaseDevice(address_t address)
{
	switch (address)
	{
	case V_BOR:
		// perform a BOR via JTAG - we loose control of the device then...
		gPlayer.Play(kIrDr16(Ir::kTestReg, 0x0200));
		StopWatch().Delay<Timer::Msec(1500)>(); // wait some time before doing any other action
		// JTAG control is lost now - GetDevice() needs to be called again to gain control.
		break;

	case V_RESET:
		gPlayer.PlayAsync(kIrDr16(Ir::kCntrlSig16Bit, 0x0C01));	// Perform a reset (write-only; next shift drains)
		gPlayer.DR_Shift16(0x0401);
		gPlayer.IR_Shift(Ir::kCntrlSigRelease);
		break;

	case V_RUNNING:
		gPlayer.IR_Shift(Ir::kCntrlSigRelease);
		break;

	default:
		TapDev430Xv2::SetPC(address);	// Set target CPU's PC
		// prepare release & release
		static constexpr TapStep steps[] =
		{
			kTclk1,
			kIrDr16(Ir::kCntrlSig16Bit, 0x0401),
			kIr(Ir::kAddrCapture),
			kIr(Ir::kCntrlSigRelease),
		};
		gPlayer.Play(steps, _countof(steps));
		break;
	}
}


// Source: uif
void TapDev430Xv2::ReleaseDevice(CpuContext &ctx, const ChipProfile &prof, bool run_to_bkpt, uint16_t mdbval)
{
	// Restore status register
	SetReg(2, ctx.sr);
	// Restore watchdog timer
	WriteWord(GetXv2WatchdogAddress(ctx.jtagId), ctx.wdt);
	
	address_t pc = ctx.pc;
	// check if CPU is OFF, and decrement PC = PC-2
	if (ctx.sr & kCPUOFF)
		pc -= 2;
	SetPC(pc);
	
	uint16_t sig;	// return value of JTAG SIG register
	if (mdbval != kSwBkpInstr)
	{
		static constexpr TapStep steps[] =
		{
			kIrDr16(Ir::kCntrlSig16Bit, 0x0401),
			kIrData16Argv(kdTclk1, kdTclk0),
			kIr(Ir::kAddrCapture),
			kIr(Ir::kCntrlSigCapture),
			kDr16_ret(0),
		};
		gPlayer.Play(steps,
			_countof(steps),
			mdbval,
			&sig);
	}
	else
	{
		static constexpr TapStep steps[] =
		{
			kTclk1,
			kIrDr16(Ir::kCntrlSig16Bit, 0x0401),
			kIr(Ir::kAddrCapture),
			kIr(Ir::kCntrlSigCapture, kdTclk0),
			kDr16_ret(0),
		};
		gPlayer.Play(steps,
			_countof(steps),
			&sig);
	}
	// Toggle HALT bit (undocumented)
	if (IsSet(static_cast<CtrlSigReg>(sig), CtrlSigReg::kHalt))
		gPlayer.PlayAsync(kIrDr16(Ir::kCntrlSig16Bit, 0x0403));	// write-only; next Play(steps) shift drains
	static constexpr TapStep steps[] =
	{
		// here one more clock cycle is required to advance the CPU
		// otherwise enabling the EEM can stop the device again
		kIrDr16Argv(kdTclk1,
			Ir::kEmexWriteControl),
	};
	gPlayer.Play(steps,
		_countof(steps),
		run_to_bkpt ? 0x0007 : 0x0006);	// eem control bits
	// TODO: LPM5 stuff
	{
		DisableLpmx5(prof);
		if (ctx.jtagId == kMsp_99)
		{
			// Manually disable DBGJTAGON bit
			uint16_t tmp = (uint16_t)gPlayer.Play(kIrDr16(Ir::kTest3VReg, 0));
			gPlayer.DR_Shift16(tmp & ~kDBGJTAGON);
		}
	}
	gPlayer.IR_Shift(Ir::kCntrlSigRelease);
	ctx.fIsRunning = true;
}



// Single step
bool TapDev430Xv2::SingleStep(CpuContext &ctx, const ChipProfile &prof, uint16_t mdbval)
{
	bool normal = ((ctx.sr & kStatusRegCpuOff) == 0) || (ctx.fInInterrupt & 0x04);
	// Stores BKPT 0 information
	BkptSetting bkpt0;

	// Preserve breakpoint block 0
	ReadBkptSettings(bkpt0, 0, k32_bits);

	BkptSetting bkpt_step =
	{
		.cntrl_ = (kCmpEqual | kTrig0 | kMab),
		.mask_ = kMaskAll,
		.combi_ = 0x0001,
		.value_ = bkpt0.value_,
		.cpustop_ = 0x0001,
	};
	// Configure "Single Step Trigger" using Trigger Block 0
	WriteBkptSettings(bkpt_step, 0, k32_bits);

	ReleaseDevice(ctx, prof, true, mdbval);
	
	bool running = true;
	if (normal)
	{
		StopWatch stopwatch(TickTimer::M2T<Timer::Msec(2)>::kTicks);
		// Wait for EEM stop reaction
		gPlayer.IR_Shift(Ir::kEmexReadControl);
		while (((gPlayer.DR_Shift16(0) & 0x0080) == 0) && running)
			running = (stopwatch.IsNotElapsed());
		// Capture again
		if (running)
			running &= SyncJtagConditionalSaveContext(ctx, prof);
		// Configure "Single Step Trigger" using Trigger Block 0
		WriteBkptSettings(bkpt0, 0, k32_bits);
	}
	else
	{
		if (gPlayer.IR_Shift(Ir::kCntrlSigCapture) == ctx.jtagId)
			SyncJtagConditionalSaveContext(ctx, prof);
		else
			running = false;
	}
	return running;
}


uint32_t TapDev430Xv2::EemDataExchangeXv2(uint8_t xchange, const CpuContext &ctx)
{
	// read access
	assert((xchange & kMxRead) != 0);
	if ((xchange & 0xfe) == kModClkCtrl0)
		return ctx.eemClkCtrl;
	else
	{
		gPlayer.IR_Shift(Ir::kEmexDataExchange32);
		gPlayer.SetReg_32Bits(xchange);	// load address
		return gPlayer.SetReg_32Bits(0);
	}
}


void TapDev430Xv2::EemDataExchangeXv2(uint8_t xchange, uint32_t data, CpuContext &ctx)
{
	// write access
	assert((xchange & kMxRead) == 0);

	if ((xchange & 0xfe) == kModClkCtrl0)
	{
		ctx.eemClkCtrl = data;
	}
	else
	{
#if 0
		if ((xchange == 0x9E) && (data & 0x40))
		{
			lastTraceWritePos = 0;
		}
#endif

		gPlayer.IR_Shift(Ir::kEmexDataExchange32);
		gPlayer.SetReg_32Bits(xchange);	// load address
		gPlayer.SetReg_32Bits(data);		// shift in value
	}
}
