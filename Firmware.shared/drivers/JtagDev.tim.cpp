#include "stdproj.h"

#if ! OPT_JTAG_USING_SPI

#include "JtagDev.h"
#include "util/TimDmaWave.h"
#include "util/WaveJtag.h"


// Initial values for generator timer frequency (max is 1.8MHz)
#define INIT_TIME_FREQ		1800000


using namespace Bmt::Dma;
using namespace Bmt::Timer;
using namespace WaveJtag;

// A double buffer to perform autonomous read and write operations
AnyPingPongBuffer<uint32_t, JtagDev::kPingPongBufSize_> JtagDev::pingpongbuf_;
bool JtagDev::tclk_;



 // JTMS and JTCK shall be on the same port, for performance reason
static_assert(JTMS::kPortBase_ == JTCK::kPortBase_, "Same port required for performance reason");
// JTMS and JTDI shall be on the same port, for performance reason
static_assert(JTMS::kPortBase_ == JTDI::kPortBase_, "Same port required for performance reason");


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

// JTAG transaction generator for IR 8-bits
typedef Generator<
	SysClk
	, kWaveJtagTimer	// TIM1
	, kWaveJtagWriteCh	// TIM1_CH1: Used for clock falling edge and set any output data
	, kWaveJtagRise		// TIM1_CH2: Used for clock rising edge (no other signal shall change here!)
	, kWaveJtagReadCh	// TIM1_CH4: Reads TDO line imediately after rising edge
	, 800000
	, Scan::kGoIdle
	, NumBits::kGoIdle
	> JtagGoIdle;

// JTAG transaction generator for IR 8-bits
/*!
TIM1_CH1: DMA1_CH2 - Write buffer to BSSR GPIO register
TIM1_CH2: DMA1_CH3 - Fixed address to BSSR GPIO register
TIM1_CH4: DMA1_CH4 - Reads IDR to a buffer (reuses write buffer)
*/
typedef Generator<
	SysClk
	, kWaveJtagTimer
	, kWaveJtagWriteCh
	, kWaveJtagRise
	, kWaveJtagReadCh
	, INIT_TIME_FREQ			// Max frequency is 1.8 MHz before bus saturates
	, Scan::kIR
	, NumBits::k8
	> JtagIr8;

// JTAG transaction generator for DR 8-bits
typedef Generator<
	SysClk
	, kWaveJtagTimer
	, kWaveJtagWriteCh
	, kWaveJtagRise
	, kWaveJtagReadCh
	, INIT_TIME_FREQ			// Max frequency is 1.8 MHz before bus saturates
	, Scan::kDR
	, NumBits::k8
	> JtagDr8;

// JTAG transaction generator for DR 16-bits
typedef Generator<
	SysClk
	, kWaveJtagTimer
	, kWaveJtagWriteCh
	, kWaveJtagRise
	, kWaveJtagReadCh
	, INIT_TIME_FREQ			// Max frequency is 1.8 MHz before bus saturates
	, Scan::kDR
	, NumBits::k16
	> JtagDr16;

// JTAG transaction generator for DR 20-bits
typedef Generator<
	SysClk
	, kWaveJtagTimer
	, kWaveJtagWriteCh
	, kWaveJtagRise
	, kWaveJtagReadCh
	, INIT_TIME_FREQ			// Max frequency is 1.8 MHz before bus saturates
	, Scan::kDR
	, NumBits::k20
	> JtagDr20;

// JTAG transaction generator for DR 32-bits
typedef Generator<
	SysClk
	, kWaveJtagTimer
	, kWaveJtagWriteCh
	, kWaveJtagRise
	, kWaveJtagReadCh
	, INIT_TIME_FREQ			// Max frequency is 1.8 MHz before bus saturates
	, Scan::kDR
	, NumBits::k32
	> JtagDr32;

static_assert(JtagDr32::GetCount() <= JtagDev::kPingPongBufSize_, "Shared buffer is not big enough");

static const Recipe s_recipe =
{
	&JTDO::Io(),
	JTDO::kBitValue_,
	tdi0,
	tdi1,
	tms0tck0,
	tms1tck0,
	tms1tck1,
	tck1
};


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


bool JtagDev::OnAnticipateTms() const
{
	return false;
}


bool JtagDev::OnOpen()
{
	tclk_ = true;	// TCLK clock always starts with level high
	WATCHPOINT();
	JtclkWaveGen::Init();
	
	JtagDr32::Init();

	// JUST FOR A CASUAL TEST USING LOGIC ANALYZER
#define TEST_WITH_LOGIC_ANALYZER 0
#if TEST_WITH_LOGIC_ANALYZER
	WATCHPOINT();
	OnConnectJtag();
	OnEnterTap();
	OnResetTap();
	
#if 0
	JtagDr8::RenderTransaction(pingpongbuf_.GetNext(), s_recipe, tdi0, 0xA8);
	WATCHPOINT();
	JtagDr8::Start((uint32_t *)pingpongbuf_.GetNext(), s_recipe);
	JtagDr8::Wait();
#else
	OnIrShift(IR_CNTRL_SIG_RELEASE); // 0xA8
	OnFlashTclk(6);
	OnDrShift8(IR_CNTRL_SIG_RELEASE); // 0xA8
	OnFlashTclk(7);
	OnDrShift16(0x1234);
	OnFlashTclk(8);
	OnDrShift20(0x12345);
	OnFlashTclk(9);
	OnDrShift32(0x12345789);
#endif
	WATCHPOINT();
	for (int i = 0; i < 100; ++i)
		__NOP();
	SetBusState(BusState::off);
	JtagOff::Enable();
	assert(false);
#endif
	return true;
}


void JtagDev::OnClose()
{
	SetBusState(BusState::off);
	JtagOff::Enable();
}


void JtagDev::OnConnectJtag()
{
	// slau320: ConnectJTAG / DrvSignals
	JtagOn::Enable();
	SetBusState(BusState::sbw);
	//JENABLE::SetHigh();
	JTEST::SetHigh();
	StopWatch().Delay<Msec(10)>();
}


void JtagDev::OnReleaseJtag()
{
	// slau320: StopJtag
	JTEST::SetLow();
	SetBusState(BusState::off);
	JtagOff::Enable();
	//JENABLE::SetLow();
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
	JtagGoIdle::DoGoIdle(
		pingpongbuf_.GetNext(), 
		s_recipe
		);

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

	uint32_t tclk = tclk_ ? tclk1 : tclk0;
	R::RenderTransaction(
		pingpongbuf_.GetNext(), 
		s_recipe, 
		tclk, 
		instruction
		);
	R::Start(
		pingpongbuf_.GetNext(), 
		s_recipe
		);
	pingpongbuf_.Step();
	
	R::Wait();
	return (P)R::GetResult(pingpongbuf_.GetCurrent(), s_recipe);
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

	uint32_t tclk = tclk_ ? tclk1 : tclk0;
	WATCHPOINT();
	R::RenderTransaction(
		pingpongbuf_.GetNext(), 
		s_recipe, 
		tclk, 
		data);
	R::Start(
		pingpongbuf_.GetNext(), 
		s_recipe);
	pingpongbuf_.Step();
	
	R::Wait();
	return (P)R::GetResult(pingpongbuf_.GetCurrent(), s_recipe);
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

	uint32_t tclk = tclk_ ? tclk1 : tclk0;
	R::RenderTransaction(
		pingpongbuf_.GetNext(), 
		s_recipe, 
		tclk, 
		data);
	R::Start(
		pingpongbuf_.GetNext(), 
		s_recipe);
	pingpongbuf_.Step();
	
	R::Wait();
	return (P)R::GetResult(pingpongbuf_.GetCurrent(), s_recipe);
}


uint32_t JtagDev::OnDrShift20(uint32_t data)
{
	typedef JtagDr20 R;

	uint32_t tclk = tclk_ ? tclk1 : tclk0;
	R::RenderTransaction(
		pingpongbuf_.GetNext(), 
		s_recipe, 
		tclk, 
		data);
	R::Start(
		pingpongbuf_.GetNext(), 
		s_recipe);
	pingpongbuf_.Step();
	
	R::Wait();
	data = R::GetResult(pingpongbuf_.GetCurrent(), s_recipe);

	/* JTAG state = Run-Test/Idle */
	data = ((data << 16) + (data >> 4)) & 0x000FFFFF;
	return data;
}


uint32_t JtagDev::OnDrShift32(uint32_t data)
{
	typedef JtagDr32 R;
	typedef uint32_t P;

	uint32_t tclk = tclk_ ? tclk1 : tclk0;
	R::RenderTransaction(
		pingpongbuf_.GetNext(), 
		s_recipe, 
		tclk, 
		data);
	R::Start(
		pingpongbuf_.GetNext(), 
		s_recipe);
	pingpongbuf_.Step();
	
	R::Wait();
	return (P)R::GetResult(pingpongbuf_.GetCurrent(), s_recipe);
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
	volatile GPIO_TypeDef &port = JTMS::Io();
	port.BSRR = tclk1;
	port.BSRR = tclk0_s;
}


void JtagDev::OnPulseTclk(int count)
{
	volatile GPIO_TypeDef &port = JTMS::Io();
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
	volatile GPIO_TypeDef &port = JTMS::Io();
	port.BSRR = tclk0;
	port.BSRR = tclk1_s;
}


#endif // JTAG_USING_SPI
