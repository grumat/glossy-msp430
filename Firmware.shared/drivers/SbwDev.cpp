#include "stdproj.h"

#if OPT_INCLUDE_SBW_TIM_

#include "SbwDev.h"

using namespace Bmt::Timer;


// Combined ping-pong buffer for autonomous SBW read/write — one Step() per frame.
AnyPingPongBuffer2<uint32_t, SbwDev::kBufSize_, uint32_t, SbwDev::kBufSize_> SbwDev::buf_;


/*!
Enter TAP via the SBW TEST/RST activation sequence (slau320AJ §7.3).

The SBW activation table:
	TEST=0, RST=0 : SBW deactivated (target in POR)
	TEST=1, RST=0 : Activate Spy-Bi-Wire
	TEST=0, RST=1 : Activate 4-wire JTAG on an SBW-capable device

Pin mapping (this repurposed-SWD hardware): SBW is a two-wire interface that
lives entirely on the SWD pins — the TEST role (which becomes SBWTCK once
active) is PB13, and the ~RST/NMI role (which becomes SBWTDIO) is PB14. The
JTAG nRST/TEST lines (JRST=PB0 / JTEST=PB1) are NOT on the SBW connector (see
Hardware/STLink-Adapter/README.md), so the handshake is bit-banged on the SBW
pins via SBWTEST_Bb (PB13) / SBWRST_Bb (PB14). After the handshake PB13 is
handed to TIM1_CH1N (SbwClkToAf) for DMA-clocked frames; PB14 stays a driven
output until the per-frame turnaround DMA takes over.

Sequence (EntrySequences_RstHigh_SBW — RST high throughout):
		   ___________________      ___________
	TEST  |                   |____|             (1µs low glitch latches SBW)
		   ______________________________________
	RST   |                                       (stays HIGH the whole time)
*/
void SbwDev::OnEnterTap()
{
	/*
	Workflow: Open -> ConnectJtag -> EnterTap -> ResetTap -> SBW mode ready
									 \______/
	*/

	// 0. Take the SBW pins under bit-bang control. SbwBusOn() (run in
	//    OnConnectJtag) left PB13 in TIM1 AF; flip it to a GPIO output for the
	//    handshake. PB14 is already a driven output.
	SBWTEST_Bb::SetupPinMode();		// PB13: AF → GPIO push-pull output (TEST)
	SBWRST_Bb::SetupPinMode();		// PB14: ensure push-pull output (~RST)

	// Faithful port of Replicator430Xv2 EntrySequences_RstHigh_SBW()
	// (slau320aj, JTAGfunc430Xv2.c). The decisive point: RST stays HIGH through
	// the activating TEST 1→0→1 glitch — the device latches SBW on that glitch
	// while RST is high. (Earlier this raised RST only AFTER the glitch, citing
	// the slau320 static "TEST↑ while RST=0" truth table; but TI's working entry
	// code keeps RST high, and an RST-low glitch left the target not driving TDO
	// → IR_Shift returned 0xFF.) Prior-session teardown is OnReleaseJtag()'s job.

	// 1. Reset TEST logic: TEST low, RST high, 4 ms (TI //1 ClrTST + //2 SetRST).
	SBWRST_Bb::SetHigh();			// RST high — stays high for the whole sequence
	SBWTEST_Bb::SetLow();			// TEST low
	StopWatch().Delay<Msec(4)>();

	// 2. Activate TEST logic: TEST high, 20 ms (TI //3 SetTST).
	SBWTEST_Bb::SetHigh();
	StopWatch().Delay<Msec(20)>();

	// 3. Phase 1: RST already high, settle 60 µs.
	StopWatch().Delay<Usec(60)>();

	// 4. Phases 2-4: the activating TEST 1→0→1 glitch WITH RST HIGH. The 1 µs
	//    TEST-low pulse is timing-critical, so disable interrupts across it
	//    (TI brackets it with _DINT()/_EINT()).
	{
		CriticalSection lock;
		SBWTEST_Bb::SetLow();		// phase 2: TEST 1→0
		StopWatch().Delay<Usec(1)>();
		SBWTEST_Bb::SetHigh();		// phase 4: TEST 0→1 → SBW latched
	}
	StopWatch().Delay<Usec(60)>();

	// 5. Phase 5 settle, then hand SBWTCK (PB13) to TIM1_CH1N for DMA-clocked
	//    frames. SbwClkToAf touches PB13 only, leaving the RST/PB14 level intact.
	StopWatch().Delay<Msec(5)>();
	SbwClkToAf::Setup();
	SetBusState(BusState::sbw);
}


bool SbwDev::IsInstrLoad()
{
	OnIrShift(IR_CNTRL_SIG_CAPTURE);
	CtrlSigReg reg = static_cast<CtrlSigReg>((uint16_t)OnDrShift16(0));
	constexpr CtrlSigReg mask = CtrlSigReg::kRead | CtrlSigReg::kInstrLoad;
	return IsSet(reg, mask, mask);
}


bool SbwDev::OnInstrLoad()
{
	OnIrShift(IR_CNTRL_SIG_LOW_BYTE);
	OnDrShift8(E2I(CtrlSigReg::kRead));
	SbwDev::OnSetTclk();

	for (int i = 0; i < 10; i++)
	{
		if (IsInstrLoad())
			return true;
		SbwDev::OnPulseTclk();
	}
	return false;
}


void SbwDev::OnTclk(DataClk tclk)
{
	switch (tclk)
	{
	case kdTclk0:
		OnClearTclk();
		break;
	case kdTclk1:
		OnSetTclk();
		break;
	case kdTclk2P:
		OnPulseTclk();
		// FALL THROUGH
	case kdTclkP:
		OnPulseTclk();
		break;
	case kdTclk2N:
		OnPulseTclkN();
		// FALL THROUGH
	case kdTclkN:
		OnPulseTclkN();
		break;
	default:
		break;
	}
}


uint16_t SbwDev::OnData16(DataClk clk0, uint16_t data, DataClk clk1)
{
	OnIrShift(IR_DATA_16BIT);
	// Send clock before data
	OnTclk(clk0);
	// Send data
	data = OnDrShift16(data);
	// Send clock after data
	OnTclk(clk1);
	return data;
}



#define OUT1RDY 0x0008
#define OUT0RDY 0x0004
#define IN1RDY  0x0002
#define IN0RDY  0x0001

#define JMB32B  0x0010
#define OUTREQ  0x0004
#define INREQ   0x0001


uint32_t SbwDev::OnReadJmbOut()
{
	uint16_t sJMBOUT0 = 0, sJMBOUT1 = 0, sJMBINCTL = 0;
	OnIrShift(IR_JMB_EXCHANGE);
	if (OnDrShift16(sJMBINCTL) & OUT1RDY)
	{
		sJMBINCTL |= JMB32B + OUTREQ;
		OnDrShift16(sJMBINCTL);
		sJMBOUT0 = OnDrShift16(0);
		sJMBOUT1 = OnDrShift16(0);
		return ((uint32_t)sJMBOUT1 << 16) + sJMBOUT0;
	}
	return 0;
}



//----------------------------------------------------------------------------
//! \brief Write a 16bit value into the JTAG mailbox system over SBW.
//! The function timeouts if the mailbox is not empty after a certain number
//! of retries.
//! \param[in] word dataX (data to be shifted into mailbox)
bool SbwDev::OnWriteJmbIn16(uint16_t dataX)
{
	constexpr uint16_t sJMBINCTL = INREQ;
	const uint16_t sJMBIN0 = dataX;

	StopWatch stopwatch(TickTimer::M2T<Msec(25)>::kTicks);

	OnIrShift(IR_JMB_EXCHANGE);
	do
	{
		// Timeout
		if (stopwatch.IsNotElapsed() == false)
		{
#if DEBUG
			McuCore::Abort();
#endif // DEBUG
			return false;
		}
	} while (!(OnDrShift16(0x0000) & IN0RDY));
	OnDrShift16(sJMBINCTL);
	OnDrShift16(sJMBIN0);
	return true;
}

#endif // OPT_INCLUDE_SBW_TIM_
