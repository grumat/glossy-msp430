#include "stdproj.h"
#include "JtagDev.h"

using namespace Bmt::Timer;


// Combined ping-pong buffer for autonomous read/write — one Step() per frame.
#if OPT_BUFFER_LAYOUT_ == OPT_BUFFER_LAYOUT_PAIR
AnyPingPongBuffer2<FrameBufEleType, JtagDev::kBufSize_, FrameBufEleType, JtagDev::kBufSize_> JtagDev::buf_;
#elif OPT_BUFFER_LAYOUT_ == OPT_BUFFER_LAYOUT_TRIPLE
AnyPingPongBuffer3<FrameBufEleType, JtagDev::kBufSize_, FrameBufEleType, JtagDev::kBufSize_, uint32_t, JtagDev::kBufSize_> JtagDev::buf_;
#endif


#if OPT_TEST_WITH_LOGIC_ANALYZER
void JtagDev::DoLogicAnalyzerTest()
{
	WATCHPOINT();
	OnConnectJtag(BusSpeed::kSlowest);
	OnEnterTap();
	OnResetTap();

	//					MSP430F5418A	MSP430F1611
	//			0xA8		0x91			0x89
	Debug() << f::Xw(OnIrShift(Ir::kCntrlSigRelease), 2)
		<< '\n'
		;
	//			0xA8		0x91			0x89
	Debug() << f::Xw(OnIrShift(Ir::kCntrlSigRelease), 2)
		<< '\n'
		;
	//			0xAAAA		0x5555			0x5555
	Debug() << f::Xw(OnDrShift16(0xAAAA), 4)
		<< '\n'
		;
	//			0x12345		0x091A2			0x091A2
	Debug() << f::Xw(OnDrShift20(0x12345), 5)
		<< '\n'
		;
	//			0x12345789	0x091a2bc4		0x091a2bc4
	Debug() << f::Xw(OnDrShift32(0x12345789), 8)
		<< '\n'
		;
#if 0
	OnFlashTclk(9);
	//			0xA8		0x??			0x54
	Debug() << f::Xw(OnDrShift8(E2I(Ir::kCntrlSigRelease)), 2)
		<< '\n'
		;
#endif

	// Hardware buffers in tri-state...
	SetBusState(BusState::kStandby);
	JtagOff::SetupPinMode();
	WATCHPOINT();
	for (int i = 0; i < 100; ++i)
		__NOP();
	// Hardware buffers in tri-state
	SetBusState(BusState::kStandby);
	JtagOff::SetupPinMode();
	assert(false);
	while (1)
		__WFI();
}
#endif // OPT_TEST_WITH_LOGIC_ANALYZER


/*!
Enter TAP via TEST/RST reset sequence (slau320 4.2.1).

Pure bit-bang on JRST (PB0) and JTEST (PB1) — identical across all JTAG
backend variants (dtrig/spi/tim), so it lives here rather than being
duplicated per file. The only peripheral interaction is the leading
OnSetTclk() to seed TDI=high, which each variant maps to its native
"drive TDI high" primitive.
*/
void JtagDev::OnEnterTap(bool rst_low)
{
	/*
	Workflow: Open -> ConnectJtag -> EnterTap -> ResetTap -> JTAG mode ready
									 \______/

	Faithful port of TI _hil_EntrySequences_Rst{High,Low}_JTAG (uifv1/hil430.c).
	Unlike the old slau320 fuse-check shape, RST is NOT pulsed low at the start:
	for the default RstHigh entry RST stays HIGH through the TEST-low "reset TEST
	logic" window AND the TEST-high "activate TEST logic" 100 ms dwell, dipping
	only ~40 us at //4. A leading RST-low-while-activating window leaves a legacy
	MSP430i20xx (i2031, jtag_id 0x89) TAP dark — see
	.claude/docs/msp430/I2031_ACQUISITION_GOLDEN_REFERENCE.md.

	      RstHigh (default)              RstLow (rst_low, MagicPattern)
	          ____      ____                 ____      ____
	TEST ____|    |____|              TEST _|    |____|
	     _________  ________               _____________
	RST            ||                  RST _|

	rst_low (EntrySequences_RstLow_JTAG) keeps the device in reset across the
	activation; used by the MagicPattern acquisition. The two paths differ only at
	//1 dwell and //2 RST level.
	*/

	// Acquiring phase: the entry glitch must run with only the RST buffer live (see
	// the BusState table). Set it HERE — at the start of the entry sequence — not just
	// in OnConnectJtag, because TapMcu re-enters OnEnterTap directly for the
	// RstLow→RstHigh retry (TapMcu::InitDevice) with no intervening OnConnectJtag;
	// without this that second glitch would run in the active kJtag buffer state.
	SetBusState(BusState::kAcquiringJtag);

	OnSetTclk();					// TDI high

	JTEST::SetLow();				//1 reset TEST logic
	if (rst_low)
	{
		StopWatch().Delay<Msec(4)>();
		JRST::SetLow();				//2 RstLow: hold device in reset
	}
	else
	{
		StopWatch().Delay<Msec(1)>();
		JRST::SetHigh();			//2 RstHigh: RST stays high
	}

	JTEST::SetHigh();				//3 activate TEST logic
	// TI uses 100 ms (RstHigh) / 50 ms (RstLow), but the i2030 datasheet spec
	// t_SBW,En ("TEST high to acceptance of first clock edge") is only 1 us max,
	// so 100 ms is heavy overkill. Trying 25 ms (1/4 of TI's value) as a bench
	// step; reduce further toward the 1 us spec if acquisition still succeeds.
	StopWatch().Delay<Msec(25)>();

	JRST::SetLow();					//4 brief RST low
	StopWatch().Delay<Usec(40)>();

	JTEST::SetLow();				//5 (4-wire: clear TEST)
	StopWatch().Delay<Usec(1)>();

	JTEST::SetHigh();				//7 TEST high (JTAG enabled)
	StopWatch().Delay<Usec(40)>();

	JRST::SetHigh();
	// Entry sequence done — JTAG enabled. Full JTAG bus driving.
	SetBusState(BusState::kJtag);
	StopWatch().Delay<Msec(5)>();
}


bool JtagDev::IsInstrLoad()
{
	OnIrShift(Ir::kCntrlSigCapture);
	CtrlSigReg reg = static_cast<CtrlSigReg>((uint16_t)OnDrShift16(0));
	constexpr CtrlSigReg mask = CtrlSigReg::kRead | CtrlSigReg::kInstrLoad;
	return IsSet(reg, mask, mask);
}


bool JtagDev::OnInstrLoad()
{
	OnIrShift(Ir::kCntrlSigLowByte);
	OnDrShift8(E2I(CtrlSigReg::kRead));
	JtagDev::OnSetTclk();

	for (int i = 0; i < 10; i++)
	{
		if (IsInstrLoad())
			return true;
		JtagDev::OnPulseTclk();
	}
	return false;
}


void JtagDev::OnTclk(DataClk tclk)
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
		[[fallthrough]];
	case kdTclkP:
		OnPulseTclk();
		break;
	case kdTclk2N:
		OnPulseTclkN();
		[[fallthrough]];
	case kdTclkN:
		OnPulseTclkN();
		break;
	default:
		break;
	}
}


uint16_t JtagDev::OnData16(DataClk clk0, uint16_t data, DataClk clk1)
{
	OnIrShift(Ir::kData16Bit);
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


uint32_t JtagDev::OnReadJmbOut()
{
	uint16_t sJMBOUT0 = 0, sJMBOUT1 = 0, sJMBINCTL = 0;
	OnIrShift(Ir::kJmbExchange);
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
//! \brief Write a 16bit value into the JTAG mailbox system.
//! The function timeouts if the mailbox is not empty after a certain number
//! of retries.
//! \param[in] word dataX (data to be shifted into mailbox)
bool JtagDev::OnWriteJmbIn16(uint16_t dataX)
{
	constexpr uint16_t sJMBINCTL = INREQ;
	const uint16_t sJMBIN0 = dataX;

	StopWatch stopwatch(TickTimer::M2T<Msec(25)>::kTicks);

	OnIrShift(Ir::kJmbExchange);
	do
	{
		// Timeout: a mailbox-not-ready is a handled outcome (best-effort MagicPattern
		// write, or a device whose JMB never reports ready) — return false; do not abort.
		if (stopwatch.IsNotElapsed() == false)
			return false;
	} while (!(OnDrShift16(0x0000) & IN0RDY));
	OnDrShift16(sJMBINCTL);
	OnDrShift16(sJMBIN0);
	return true;
}

