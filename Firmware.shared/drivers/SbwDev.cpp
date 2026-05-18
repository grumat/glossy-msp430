#include "stdproj.h"

#if OPT_INCLUDE_SBW_DTRIG_

#include "SbwDev.h"

using namespace Bmt::Timer;


// Combined ping-pong buffer for autonomous SBW read/write — one Step() per frame.
AnyPingPongBuffer2<uint32_t, SbwDev::kBufSize_, uint32_t, SbwDev::kBufSize_> SbwDev::buf_;


/*!
Enter TAP via SBW TEST/RST entry sequence (slau320 SBW section).

Pure bit-bang on SBWTCK (PB0) and JTEST (PB1) — independent of the active
SBW transport variant, so it lives here rather than being duplicated per
backend file. The pulse pattern reshapes the 4-wire JTAG TEST/RST handshake
into the form required to put the MSP430 into SBW mode.
*/
void SbwDev::OnEnterTap()
{
	// TODO: SBW entry sequence (slau320 "RstLow_SBW" / Section 2.3)
	//
	// Workflow: Open -> ConnectJtag -> EnterTap -> ResetTap -> SBW mode ready
	//                                  \______/
	//                        ____________________
	// RST/SBWTCK ___________|
	//                _____    __    __    __    __
	// TEST/SBWTDIO _|     |__|  |__|  |__|  |__|
	//                                 SBW entry pulses
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
