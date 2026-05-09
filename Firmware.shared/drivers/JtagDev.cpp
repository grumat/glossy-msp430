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
	Debug() << f::Xw(OnIrShift(IR_CNTRL_SIG_RELEASE), 2)
		<< '\n'
		;
	//			0xA8		0x91			0x89
	Debug() << f::Xw(OnIrShift(IR_CNTRL_SIG_RELEASE), 2)
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
	Debug() << f::Xw(OnDrShift8(IR_CNTRL_SIG_RELEASE), 2)
		<< '\n'
		;
#endif

	// Hardware buffers in tri-state...
	SetBusState(BusState::off);
	JtagOff::SetupPinMode();
	WATCHPOINT();
	for (int i = 0; i < 100; ++i)
		__NOP();
	// Hardware buffers in tri-state
	SetBusState(BusState::off);
	JtagOff::SetupPinMode();
	assert(false);
	while (1)
		__WFI();
}
#endif // OPT_TEST_WITH_LOGIC_ANALYZER


bool JtagDev::IsInstrLoad()
{
	OnIrShift(IR_CNTRL_SIG_CAPTURE);
	CtrlSigReg reg = static_cast<CtrlSigReg>(OnDrShift16(0));
	constexpr CtrlSigReg mask = CtrlSigReg::kRead | CtrlSigReg::kInstrLoad;
	return IsSet(reg, mask, mask);
}


bool JtagDev::OnInstrLoad()
{
	OnIrShift(IR_CNTRL_SIG_LOW_BYTE);
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


uint16_t JtagDev::OnData16(DataClk clk0, uint16_t data, DataClk clk1)
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


uint32_t JtagDev::OnReadJmbOut()
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
//! \brief Write a 16bit value into the JTAG mailbox system.
//! The function timeouts if the mailbox is not empty after a certain number
//! of retries.
//! \param[in] word dataX (data to be shifted into mailbox)
bool JtagDev::OnWriteJmbIn16(uint16_t dataX)
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

