#include "stdproj.h"

#if OPT_INCLUDE_SBW_DTRIG_

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

Pin reuse: the same PB0/PB1 physical pads carry RST/TEST during the entry
handshake and then become SBWTCK/SBWTDIO once SBW is active. We bit-bang
both via the existing `JRST` / `JTEST` aliases. After entry the dtrig
backend reconfigures the same pins for TIM1 AF / DMA-driven SBW frames.

Sequence:
		   ________            _________________
	TEST  |        |__________|                  (final high → keep SBW selected)
		   _____         __________________
	RST   |     |_______|                        (TEST=1,RST=0 window activates SBW)
	SBWTCK ________________________________ (held low ≥7μs to ensure clean entry)
*/
void SbwDev::OnEnterTap()
{
	/*
	Workflow: Open -> ConnectJtag -> EnterTap -> ResetTap -> SBW mode ready
									 \______/
	*/

	// 1. Force the entry pins under bit-bang control. SBWTCK held low for
	//    >7 µs deactivates any previous SBW session before we re-arm it.
	JRST::SetLow();				// RST low
	JTEST::SetLow();			// TEST low
	StopWatch().Delay<Msec(4)>();

	// 2. Bring the target out of POR briefly so it can sample TEST cleanly.
	JRST::SetHigh();
	JTEST::SetHigh();
	StopWatch().Delay<Msec(20)>();

	// 3. Re-assert RST low; TEST follows briefly so the next TEST rising
	//    edge happens while RST=0 (this is the SBW activation trigger per
	//    slau320 Table 2-2: TEST↑ while RST=0 selects SBW).
	JRST::SetLow();
	StopWatch().Delay<Usec(50)>();

	JTEST::SetLow();
	StopWatch().Delay<Usec(1)>();

	// 4. TEST high while RST is still low → SBW mode latched.
	JTEST::SetHigh();
	StopWatch().Delay<Usec(60)>();

	// 5. Release RST high. From now on PB0/PB1 carry SBWTCK/SBWTDIO; the
	//    dtrig backend is responsible for reconfiguring them to AF/DMA.
	JRST::SetHigh();
	SetBusState(BusState::sbw);
	StopWatch().Delay<Msec(5)>();
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

#endif // OPT_INCLUDE_SBW_DTRIG_
