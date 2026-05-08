#include "stdproj.h"

#if OPT_INCLUDE_JTAG_TIM_DMA_

#include "JtagDev.h"
#include "util/TimDmaWave.h"
#include "util/WaveJtag.h"


// Initial values for generator timer frequency (max is 1.8MHz)
#define INIT_TIME_FREQ		1800000


using namespace Bmt::Dma;
using namespace Bmt::Timer;
using namespace WaveJtag;

uint32_t rise_buffer;


#if OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_TIM_DMA
// JTMS and JTDI shall be on the same port, for performance reason
static_assert(JTCK::kPortBase_ == JTDI::kPortBase_, "Same port required for performance reason");
 // JTMS and JTCK shall be on the same port, for performance reason
static_assert(
	JTCK::kPortBase_ == JTDI::kPortBase_
	&& JTCK::kPortBase_ == JTMS::kPortBase_
	&& JTMS::kPortBase_ == JTDI::kPortBase_
	, "Use OPT_JTAG_IMPL_TIM_DMA_SLOW for hardware platform with splitted JTAG bus"
	);
#elif OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_TIM_DMA_SLOW
// JTMS and JTDI shall be on the same port, for performance reason
static_assert(JTCK::kPortBase_ != JTDI::kPortBase_, "Slow variant expects STLinkV2 hardware constraints");
// Unoptimized hardware works on distinct ports
static_assert(
	JTCK::kPortBase_ != JTDI::kPortBase_
	|| JTCK::kPortBase_ != JTMS::kPortBase_
	|| JTMS::kPortBase_ != JTDI::kPortBase_
	, "Use OPT_JTAG_IMPL_TIM_DMA if all pins share the same port, will improve performance"
	);
#else
#	error Unknown configuration found!
#endif


// Validates setup
static_assert(JTMS::kBitValue_ != 0, "JTMS::kBitValue_ is zero!");
static_assert(JTDI::kBitValue_ != 0, "JTDI::kBitValue_ is zero!");
static_assert(JTCK::kBitValue_ != 0, "JTCK::kBitValue_ is zero!");
static_assert(JTCLK::kBitValue_ != 0, "JTCLK::kBitValue_ is zero!");
static_assert(JTCLK::kBitValue_ == JTDI::kBitValue_, "JTCLK should be same as JTDI");


 // Maps set/reset bit on port BSRR (Port bit set/reset register)
static constexpr uint32_t tms0 = JTMS::kBitValue_ << 16;
static constexpr uint32_t tms1 = JTMS::kBitValue_;
static constexpr uint32_t tdi0 = JTDI::kBitValue_ << 16;
static constexpr uint32_t tdi1 = JTDI::kBitValue_;
static constexpr uint32_t tck0 = JTCK::kBitValue_ << 16;
static constexpr uint32_t tck1 = JTCK::kBitValue_;
static constexpr uint32_t tclk0 = JTCLK::kBitValue_ << 16;
static constexpr uint32_t tclk1 = JTCLK::kBitValue_;
static constexpr uint32_t tms0tck0 = tms0 | tck0;
static constexpr uint32_t tms0tck1 = tms0 | tck1;
static constexpr uint32_t tms1tck0 = tms1 | tck0;
static constexpr uint32_t tms1tck1 = tms1 | tck1;

// Slow speed constants for better shaped waves
static volatile const uint32_t tclk0_s = tclk0;
static volatile const uint32_t tclk1_s = tclk1;


// JTCLK pulse generator
/*!
TIM3_UP: DMA1_CH3 - Produces the wave (two edges in circular mode)
TIM2 CLK: each TIM3 update and counts TIM3 cycles
TIM2_UP: DMA1_CH2 - Single DMA shot to disabe TIM3
*/
typedef TimDmaWav<
	SysClk
	, kTimDmaWavBeat
	, kTimForJtclkCnt
	, kTimDmaWavFreq
	, 2						// each pulse has two borders
	> JtclkWaveGen;

// One alias template selects the per-implementation Generator family. All
// concrete frames below are then a single line — the OPT_JTAG_IMPLEMENTATION
// selector fires exactly once, not once per frame width.
//
// TIM_DMA:      BSRR-driven generator (CH1 falling edge / CH2 rising edge / CH4 read).
// TIM_DMA_SLOW: PWM-driven STLink generator (CH1 JTCK 50%, CH2 TMS, CH3 JTDI write,
//               CH4 TDO read implicit via PWM phase).
#if OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_TIM_DMA
template <JtagFrame::Scan S, JtagFrame::NumBits N>
using JtagFor = Generator<
	SysClk
	, kWaveJtagTimer
	, kWaveJtagWriteCh
	, kWaveJtagRise
	, kWaveJtagReadCh
	, INIT_TIME_FREQ			// Max frequency is 1.8 MHz before bus saturates
	, S, N>;
#else
template <JtagFrame::Scan S, JtagFrame::NumBits N>
using JtagFor = GeneratorSTLinkPWM<
	SysClk
	, kWaveJtagTimer
	, kWaveJtagTck		// TIM1_CH1: JTCK
	, kWaveJtagTms		// TIM1_CH2: JTMS control
	, kWaveJtagWrite	// TIM1_CH3: JTDI write
	, kWaveJtagReadCh	// TIM1_CH4: TDO read (implicit via PWM phase)
	, INIT_TIME_FREQ
	, S, N>;
#endif

using JtagGoIdle = JtagFor<JtagFrame::Scan::kGoIdle, JtagFrame::NumBits::kGoIdle>;
using JtagIr8    = JtagFor<JtagFrame::Scan::kIR,     JtagFrame::NumBits::k8>;
using JtagDr8    = JtagFor<JtagFrame::Scan::kDR,     JtagFrame::NumBits::k8>;
using JtagDr16   = JtagFor<JtagFrame::Scan::kDR,     JtagFrame::NumBits::k16>;
using JtagDr20   = JtagFor<JtagFrame::Scan::kDR,     JtagFrame::NumBits::k20>;
using JtagDr32   = JtagFor<JtagFrame::Scan::kDR,     JtagFrame::NumBits::k32>;

static_assert(JtagDr32::GetCount() + 1 < JtagDev::kBufSize_, "Shared buffer is not big enough");


// Per-call wrappers that hide the AUX-buffer argument needed by the
// TIM_DMA_SLOW (TRIPLE) path but absent from TIM_DMA (PAIR). The layout
// selector now fires once inside each helper instead of at every call site.
template <typename F, typename Data>
ALWAYS_INLINE static void RenderShift(uint32_t* tx_buf, uint32_t tclk, Data data)
{
#if OPT_BUFFER_LAYOUT_ == OPT_BUFFER_LAYOUT_TRIPLE
	F::RenderTransaction(tx_buf, JtagDev::buf_.GetNext3(), tclk, data);
#else
	F::RenderTransaction(tx_buf, tclk, data);
#endif
}

template <typename F>
ALWAYS_INLINE static void StartShift(uint32_t* rd_buf, uint32_t* tx_buf)
{
#if OPT_BUFFER_LAYOUT_ == OPT_BUFFER_LAYOUT_TRIPLE
	F::Start(rd_buf, tx_buf, JtagDev::buf_.GetNext3());
#else
	F::Start(rd_buf, tx_buf);
#endif
}

ALWAYS_INLINE static void RunGoIdle()
{
#if OPT_BUFFER_LAYOUT_ == OPT_BUFFER_LAYOUT_TRIPLE
	JtagGoIdle::DoGoIdle(JtagDev::buf_.GetNext2(), JtagDev::buf_.GetNext1(), JtagDev::buf_.GetNext3());
#else
	JtagGoIdle::DoGoIdle(JtagDev::buf_.GetNext2());
#endif
}

/*
** This table has up/down bits for the GPIOx_BSRR register that will be sourced
** into the DMA to generate a 470 kHz frequency for the Flash memory. This clock is 
** required during flash erase and writes for the Gen1 and Gen2 MSP430 devices.
** The table consists of 8 pulses, so polling will always have a chance to track DMA 
** state also when CPU is moderately interrupted (a complete table scan will require 
** about 16.9 us)
** IMPORTANT: For all MSP430 JTCLK is shared with JTDI. The interface redirects pulses 
** in the JTDI line when JTAG state machine is in "Run-Test/Idle" state.
*/
static const uint32_t s_bsrr_table[] =
{
	tclk0, // reset bit
	tclk1, // set bit,
};


JtagDev::JtagDev()
{
}


bool JtagDev::OnOpen()
{
	JtclkWaveGen::Init();
	
	JtagDr32::Init();
	WATCHPOINT();

#if OPT_TEST_WITH_LOGIC_ANALYZER
	DoLogicAnalyzerTest();
#endif
	return true;
}


void JtagDev::OnClose()
{
	// Hardware buffers in tri state
	SetBusState(BusState::off);
	JtagOff::SetupPinMode();
}


void JtagDev::OnConnectJtag(BusSpeed speed)
{
	speed;
	// slau320: ConnectJTAG / DrvSignals
	// This puts the MCU in reset state
	JtagOn::SetupPinMode();
	// Hardware buffers driving SBW lines
	SetBusState(BusState::sbw);
	// This requests control for the test pins
	JTEST::SetHigh();
	StopWatch().Delay<Msec(10)>();
}


void JtagDev::OnReleaseJtag()
{
	// slau320: StopJtag
	JTEST::SetLow();
	// Hardware buffers in tri state
	SetBusState(BusState::off);
	JtagOff::SetupPinMode();
	StopWatch().Delay<Msec(10)>();
}


void JtagDev::OnEnterTap()
{
#if 1			// slau320
	/*
			________             ____
	RST  __|        |___________|
				  _____    __________
	TEST ________|     |__|
	*/
	JTEST::SetLow();		//1
	StopWatch().Delay<Msec(4)>(); // reset TEST logic

	JRST::SetHigh();		//2

	JTEST::SetHigh();		//3
	StopWatch().Delay<Msec(20)>(); // activate TEST logic

	// phase 1
	JRST::SetLow();			//4
	StopWatch().Delay<Usec(40)>();

	// phase 2 -> TEST pin to 0, no change on RST pin
	// for 4-wire JTAG clear Test pin
	JTEST::SetLow();		//5

	// phase 3
	StopWatch().Delay<Usec(1)>();

	// phase 4 -> TEST pin to 1, no change on RST pin
	// for 4-wire JTAG
	JTEST::SetHigh();		//7
	StopWatch().Delay<Msec(40)>();

	// phase 5
	JRST::SetHigh();
	// Hardware buffers driving JTAG lines
	SetBusState(BusState::jtag);
	StopWatch().Delay<Msec(5)>();
#else
	/*-------------RstLow_JTAG----------------
				________           __________
	Test ______|        |_________|
							  _______________
	Rst_____________________|
	----------------------------------------*/
	CriticalSection lock;
	{
		JTEST::SetLow();
		StopWatch().DelayUS<5>();
		JTEST::SetHigh();
		StopWatch().DelayUS<5>();
		JRST::SetLow();
		StopWatch().DelayUS<5>();
		JTEST::SetLow();		// Enter JTAG 4w
		StopWatch().DelayUS<2>();
		JTEST::SetHigh();
		StopWatch().DelayUS<5>();
		JRST::SetHigh();
		StopWatch().DelayUS<100>();
#if 0
	else
	{
		WATCHPOINT();
		JTEST::SetLow();			//1
		StopWatch().Delay<4>();

		JRST::SetHigh();			//2
		JTEST::SetHigh();			//3
		StopWatch().Delay<20>();

		JRST::SetLow();			//4
		StopWatch().Delay<60>();

		// for 4-wire JTAG clear Test pin Test(0)
		JTEST::SetLow();			//5
		StopWatch().DelayUS<1>();

		// for 4-wire JTAG - Test (1)
		JTEST::SetHigh();
		StopWatch().DelayUS<60>();

		// phase 5 Reset(1)
		JRST::SetHigh();
		StopWatch().DelayUS<500>();
		}
#endif
	}
#endif
}


/*!
Reset target JTAG interface and perform fuse-HW check
*/
void JtagDev::OnResetTap()
{
	WATCHPOINT();
	JTMS::SetHigh();
	JTCK::SetHigh();

	/* Reset JTAG state machine */
	RunGoIdle();

	JTMS::SetHigh();
	__NOP();
	JTMS::SetLow();
	StopWatch().Delay<Usec(5)>();
	JTMS::SetHigh();
	__NOP();
	JTMS::SetLow();
	StopWatch().Delay<Usec(5)>();
	JTMS::SetHigh();
	__NOP();
	JTMS::SetLow();
}


/*!
Shifts a new instruction into the JTAG instruction register through TDI
MSB first, with interchanged MSB/LSB, to use the shifting function

\param instruction: 8 bit instruction
\return: scanned TDO value
*/
uint8_t JtagDev::OnIrShift(uint8_t instruction)
{
	typedef JtagIr8 R;
	typedef uint8_t P;

	uint32_t tclk = JTCLK::IsHigh() ? tclk1 : tclk0;
	uint32_t* tx = buf_.GetNext1();
	RenderShift<R>(tx, tclk, instruction);
	StartShift<R>(buf_.GetNext2(), tx);
	WATCHPOINT();
	buf_.Step();
	R::Wait();
	return (P)R::GetResult(buf_.GetCurrent2());
}


/*!
Shifts a given 8-bit byte into the JTAG data register through TDI.

\param data  : 8 bit data
\return: scanned TDO value
*/
uint8_t JtagDev::OnDrShift8(uint8_t data)
{
	typedef JtagDr8 R;
	typedef uint8_t P;

	uint32_t tclk = JTCLK::IsHigh() ? tclk1 : tclk0;
	uint32_t* tx = buf_.GetNext1();
	RenderShift<R>(tx, tclk, data);
	StartShift<R>(buf_.GetNext2(), tx);
	buf_.Step();
	R::Wait();
	return (P)R::GetResult(buf_.GetCurrent2());
}


/*!
Shifts a given 16-bit word into the JTAG data register through TDI.

\param data  : 16 bit data
\return: scanned TDO value
*/
uint16_t JtagDev::OnDrShift16(uint16_t data)
{
	typedef JtagDr16 R;
	typedef uint16_t P;

	uint32_t tclk = JTCLK::IsHigh() ? tclk1 : tclk0;
	uint32_t* tx = buf_.GetNext1();
	RenderShift<R>(tx, tclk, data);
	StartShift<R>(buf_.GetNext2(), tx);
	buf_.Step();
	R::Wait();
	return (P)R::GetResult(buf_.GetCurrent2());
}


uint32_t JtagDev::OnDrShift20(uint32_t data)
{
	typedef JtagDr20 R;

	uint32_t tclk = JTCLK::IsHigh() ? tclk1 : tclk0;
	uint32_t* tx = buf_.GetNext1();
	RenderShift<R>(tx, tclk, data);
	StartShift<R>(buf_.GetNext2(), tx);
	buf_.Step();
	R::Wait();
	data = R::GetResult(buf_.GetCurrent2());

	/* JTAG state = Run-Test/Idle */
	data = ((data << 16) + (data >> 4)) & 0x000FFFFF;
	return data;
}


uint32_t JtagDev::OnDrShift32(uint32_t data)
{
	typedef JtagDr32 R;
	typedef uint32_t P;

	uint32_t tclk = JTCLK::IsHigh() ? tclk1 : tclk0;
	uint32_t* tx = buf_.GetNext1();
	RenderShift<R>(tx, tclk, data);
	StartShift<R>(buf_.GetNext2(), tx);
	buf_.Step();
	R::Wait();
	return (P)R::GetResult(buf_.GetCurrent2());
}


void JtagDev::OnSetTclk()
{
	JTCLK::SetHigh();
}


void JtagDev::OnClearTclk()
{
	JTCLK::SetLow();
}


void JtagDev::OnPulseTclk()
{
	volatile GPIO_TypeDef &port = JTCLK::Io();
	port.BSRR = tclk1;
	port.BSRR = tclk0_s;
}


void JtagDev::OnPulseTclk(int count)
{
	volatile GPIO_TypeDef &port = JTCLK::Io();
	for(int i = 0 ; i < count; ++i)
	{
		port.BSRR = tclk1;
		port.BSRR = tclk0_s;
	}
}


void JtagDev::OnFlashTclk(uint32_t min_pulses)
{
	// Release DMA channels for repurposing
	JtagDr32::ReleaseDma();
	JtclkWaveGen::SetStopper();
	JtclkWaveGen::SetTarget(&JTCLK::Io().BSRR, s_bsrr_table, _countof(s_bsrr_table));
	// Run and wait
	JtclkWaveGen::RunEx(min_pulses);
	// Regardless of state where it stopped, keep GPIO always high
	JTCLK::SetHigh();
	// Default DMA setup is for JTAG frames
	JtagDr32::SetupDma();
}


void JtagDev::OnPulseTclkN()
{
	volatile GPIO_TypeDef &port = JTCLK::Io();
	port.BSRR = tclk0;
	port.BSRR = tclk1_s;
}

#endif // OPT_INCLUDE_JTAG_TIM_DMA_
