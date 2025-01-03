
#include "stdproj.h"

using namespace Bmt::Timer;
using namespace Bmt::Dma;

#if OPT_JTAG_USING_SPI
#include "JtagDev.h"
#include "WaveSet.h"
#include "util/TimDmaWave.h"
#if OPT_JTAG_USING_SPI
#	include "util/SpiJtagDataShift.h"
#endif


// A template for all different SPI configuration. Same device but different BAUD rate.
template<
	const uint32_t SPEED
>
struct SpiJtagDevType 
	: Spi::SpiTemplate
		<
		kSpiForJtag
		, SysClk
		, SPEED
		, Spi::Role::kMaster
		, Spi::ClkPol::kMode3
		, Spi::Bits::k8
		, Spi::Props::kDefault
		, Spi::BiDi::kFullDuplex
		>
{
	typedef Spi::SpiTemplate<
		kSpiForJtag
		, SysClk
		, SPEED
		, Spi::Role::kMaster
		, Spi::ClkPol::kMode3
		, Spi::Bits::k8
		, Spi::Props::kDefault
		, Spi::BiDi::kFullDuplex
	> BASE;
	static_assert(BASE::kInputClock_/8 >= SPEED, "Clock");
};


// Forward declaration for templates to work
struct DmaMode_;

// A data-type to transmit 8-bits to the IR register
typedef SpiJtagDataShift
<
	DmaMode_
	, kSelectIR_Scan	// Select IR-Scan JTAG register
	, 8					// 8 bits data
	, uint8_t			// 8 bits data-type fits perfectly
> XMitIr8Type;
// A data-type to transmit 8-bits to the IR register for very high clock rates
typedef SpiJtagDataShift
<
	DmaMode_
	, kSelectIR_Scan	// Select IR-Scan JTAG register
	, 8					// 8 bits data
	, uint8_t			// 8 bits data-type fits perfectly
	, kAnticipateClock	// High clock requires internal delays compensation
> XMitIr8VhcType;
// A data-type to transmit 8-bits to the DR register
typedef SpiJtagDataShift
<
	DmaMode_
	, kSelectDR_Scan	// Select IR-Scan JTAG register
	, 8					// 8 bits data
	, uint8_t			// 8 bits data-type fits perfectly
> XMitDr8Type;
// A data-type to transmit 8-bits to the DR register for very high clock rates
typedef SpiJtagDataShift
<
	DmaMode_
	, kSelectDR_Scan	// Select IR-Scan JTAG register
	, 8					// 8 bits data
	, uint8_t			// 8 bits data-type fits perfectly
	, kAnticipateClock	// High clock requires internal delays compensation
> XMitDr8VhcType;
// A data-type to transmit 16-bits to the DR register
typedef SpiJtagDataShift
<
	DmaMode_
	, kSelectDR_Scan	// Select IR-Scan JTAG register
	, 16				// 16 bits data
	, uint16_t			// 16 bits data-type fits perfectly
> XMitDr16Type;
// A data-type to transmit 16-bits to the DR register for very high clock rates
typedef SpiJtagDataShift
<
	DmaMode_
	, kSelectDR_Scan	// Select IR-Scan JTAG register
	, 16				// 16 bits data
	, uint16_t			// 16 bits data-type fits perfectly
	, kAnticipateClock	// High clock requires internal delays compensation
> XMitDr16VhcType;
typedef SpiJtagDataShift
<
	DmaMode_
	, kSelectDR_Scan	// Select IR-Scan JTAG register
	, 20				// 20 bits data
	, uint32_t			// 32 bits data-type is required
> XMitDr20Type;
typedef SpiJtagDataShift
<
	DmaMode_
	, kSelectDR_Scan	// Select IR-Scan JTAG register
	, 20				// 20 bits data
	, uint32_t			// 32 bits data-type is required
	, kAnticipateClock	// High clock requires internal delays compensation
> XMitDr20VhcType;
typedef SpiJtagDataShift
<
	DmaMode_
	, kSelectDR_Scan	// Select IR-Scan JTAG register
	, 32				// 32 bits data
	, uint32_t			// 32 bits data-type fits perfectly
	, kNormalSpeed
	, uint64_t
> XMitDr32Type;
typedef SpiJtagDataShift
<
	DmaMode_
	, kSelectDR_Scan	// Select IR-Scan JTAG register
	, 32				// 32 bits data
	, uint32_t			// 32 bits data-type fits perfectly
	, kAnticipateClock	// High clock requires internal delays compensation
	, uint64_t
> XMitDr32VhcType;

// Lower speed grade SPI connection (or all default operation)
typedef SpiJtagDevType<JTCK_Speed_1> SpiJtagDevice;

// SPI for speed grade 2
typedef SpiJtagDevType<JTCK_Speed_2> SpiJtagDevice_2;
// SPI for speed grade 3
typedef SpiJtagDevType<JTCK_Speed_3> SpiJtagDevice_3;
// SPI for speed grade 4
typedef SpiJtagDevType<JTCK_Speed_4> SpiJtagDevice_4;
// SPI for speed grade 5
typedef SpiJtagDevType<JTCK_Speed_5> SpiJtagDevice_5;

#if OPT_JTAG_USING_DMA
typedef AnyChannel
<
	SpiJtagDevice::DmaInstance_
	, SpiJtagDevice::DmaTxCh_
	, Dir::kMemToPer
	, PtrPolicy::kBytePtrInc
	, PtrPolicy::kBytePtr
	, Prio::kMedium
> SpiTxDma;

typedef AnyChannel
<
	SpiJtagDevice::DmaInstance_
	, SpiJtagDevice::DmaRxCh_
	, Dir::kPerToMem
	, PtrPolicy::kBytePtr
	, PtrPolicy::kBytePtrInc
	, Prio::kHigh
	, true
> SpiRxDma;

// DMA channels are shared in STM32. Check conflicts.
static_assert(GenericJtagType::TmsGen::TmsOutCh_::DmaCh_ != SpiTxDma::kChan_, "DMA Channels are conflicting. This hardware setup is not compatible when JTAG_USING_DMA==1.");
static_assert(GenericJtagType::TmsGen::TmsOutCh_::DmaCh_ != SpiRxDma::kChan_, "DMA Channels are conflicting. This hardware setup is not compatible when JTAG_USING_DMA==1.");
// Just in case...
static_assert(SpiTxDma::kChan_ != SpiRxDma::kChan_, "The timer does not have independent DMA channels. This hardware setup is not compatible when JTAG_USING_DMA==1.");

struct DmaMode_
{
	static ALWAYS_INLINE void OnOpen()
	{
		SpiTxDma::Init();
		SpiTxDma::SetDestAddress(&SpiJtagDevice::GetDevice()->DR);
		SpiTxDma::SetSourceAddress(&JtagDev::tx_buf_);
		SpiRxDma::Init();
		SpiRxDma::EnableTransferCompleteInt();
		SpiRxDma::SetSourceAddress(&SpiJtagDevice::GetDevice()->DR);
		SpiRxDma::SetDestAddress(&JtagDev::rx_buf_);
	}
	static ALWAYS_INLINE void OnSpiInit()
	{
		SpiJtagDevice::Disable();
		SpiJtagDevice::EnableDma();
		SpiJtagDevice::Enable();
	}
	static ALWAYS_INLINE void OnClose()
	{
		SpiTxDma::Stop();
		SpiRxDma::Stop();
	}
	static OPTIMIZED void SendStream(uint16_t cnt)
	{
		SpiRxDma::SetTransferCount(cnt);
		SpiRxDma::Enable();
		SpiTxDma::SetTransferCount(cnt);
		McuCore::SleepOnExit();
		SpiTxDma::Enable();		// this will start the transmission
		__WFI();
		SpiTxDma::Disable();
		SpiRxDma::Disable();
	}
};

#else	// JTAG_USING_DMA

struct DmaMode_
{
	static ALWAYS_INLINE void OnOpen() {}
	static ALWAYS_INLINE void OnSpiInit() {}
	static ALWAYS_INLINE void OnClose() {}
	static ALWAYS_INLINE void SendStream(uint16_t cnt)
	{
		SpiJtagDevice::PutStream(&JtagDev::tx_buf_, &JtagDev::rx_buf_, cnt);
	}
};

#endif	// JTAG_USING_DMA


#if OPT_TIMER_DMA_WAVE_GEN

/*
** JTCLK generation
*/
typedef TimDmaWav<
	SysClk
	, kTimDmaWavBeat
	, kTimForJtclkCnt
	, kTimDmaWavFreq
	, 2						// each pulse has two borders
	> JtclkWaveGen;
#endif

class MuteSpiClk
{
public:
	ALWAYS_INLINE MuteSpiClk()
	{
		JTCK::SetHigh();	// SPI clk defaults; this is used while muting JTCK_SPI
		JTCK::SetupPinMode();
	}
	ALWAYS_INLINE ~MuteSpiClk()
	{
		JTCK_SPI::SetupPinMode();
	}
};


#if OPT_JTAG_USING_SPI
JtagPacketBuffer JtagDev::tx_buf_;
JtagPacketBuffer JtagDev::rx_buf_;
#endif

#if OPT_JTAG_USING_DMA
void JtagDev::IRQHandler(void)
{
	// Put CPU back from sleep
	McuCore::WakeOnExit();
	SpiRxDma::ClearAllFlags();	// clear all flags as device may set others
}
#endif	// JTAG_USING_DMA


void JtagDev::OpenCommon_1()
{
#if OPT_TIMER_DMA_WAVE_GEN
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
	static const uint32_t bsrr_table[] =
	{
		JTCLK::kBitValue_ << 16, // reset bit
		JTCLK::kBitValue_,		 // set bit,
	};
#endif
	
	// TMS uses GPIO on reset state
	XMitDr8Type::TmsGen::InitOutput();
#if OPT_TIMER_DMA_WAVE_GEN
	WATCHPOINT();
	JtclkWaveGen::Init();
	JtclkWaveGen::SetStopper();
	JtclkWaveGen::SetTarget(&JTCLK::Io().BSRR, bsrr_table, _countof(bsrr_table));
#endif
	DmaMode_::OnOpen();
}


void JtagDev::OpenCommon_2()
{
	DmaMode_::OnSpiInit();
	// Initialize DMA timer (do not add multiple for shared timer channel!)
#define TEST_WITH_LOGIC_ANALYZER 0
#if TEST_WITH_LOGIC_ANALYZER
	WATCHPOINT();
	OnConnectJtag();
	OnEnterTap();
	OnResetTap();
	
	OnIrShift(IR_CNTRL_SIG_RELEASE);	// 0xA8
	OnFlashTclk(6);
	OnDrShift8(IR_CNTRL_SIG_RELEASE);	// 0xA8
	OnFlashTclk(7);
	OnDrShift16(0x1234);
	OnFlashTclk(8);
	OnDrShift20(0x12345);
	OnFlashTclk(9);
	OnDrShift32(0x12345789);
	WATCHPOINT();
	for (int i = 0; i < 100; ++i)
		__NOP();
	SetBusState(BusState::off);
	JtagOff::Enable();
	assert(false);
#endif
}


bool JtagDev::OnAnticipateTms() const
{
	return false;
}
bool JtagDevVhc::OnAnticipateTms() const
{
	return true;
}


bool JtagDev::OnOpen()
{
	XMitDr8Type::TmsGen::Config::Init();
	// TMS uses GPIO on reset state
	OpenCommon_1();
	SpiJtagDevice::Init();
	OpenCommon_2();
	return true;
}
bool JtagDev_2::OnOpen()
{
	XMitDr8Type::TmsGen::Config::Init();
	// TMS uses GPIO on reset state
	OpenCommon_1();
	SpiJtagDevice_2::Init();
	OpenCommon_2();
	return true;
}
bool JtagDev_3::OnOpen()
{
	XMitDr8Type::TmsGen::Config::Init();
	// TMS uses GPIO on reset state
	OpenCommon_1();
	SpiJtagDevice_3::Init();
	OpenCommon_2();
	return true;
}
bool JtagDev_4::OnOpen()
{
	XMitDr8Type::TmsGen::Config::Init();
	// TMS uses GPIO on reset state
	OpenCommon_1();
	SpiJtagDevice_4::Init();
	OpenCommon_2();
	return true;
}
bool JtagDev_5::OnOpen()
{
	WATCHPOINT();
	XMitDr8Type::TmsGen::Config::Init();
	// TMS uses GPIO on reset state
	OpenCommon_1();
	SpiJtagDevice_5::Init();
	OpenCommon_2();
	return true;
}


void JtagDev::OnClose()
{
	SetBusState(BusState::off);
	JtagOff::Enable();
	XMitDr8Type::TmsGen::Close();
	DmaMode_::OnClose();
	SpiJtagDevice::Stop();
}


void JtagDev::OnConnectJtag()
{
	// slau320: ConnectJTAG / DrvSignals

	// BLOCK: Auto TMS output temporary suspension
	{
		// Suspend TMS output as we want to clock the char without changing TMS
		XMitDr8Type::TmsGen::SuspendAutoOutput suspend;
		// Drive TDI high, while pins are disabled
		{
			SpiJtagDevice::PutChar(0xFF);
		}
		// Drive MCU outputs on
		// Switch to SPI
		JtagSpiOn::Enable();
	}
	//TmsShapeGpioOut::Setup();
	// Enable voltage level converter
	SetBusState(BusState::sbw);
	// Wait to settle
	StopWatch().Delay<Msec(10)>();
}


void JtagDev::OnReleaseJtag()
{
	{
		MuteSpiClk scope;
		SpiJtagDevice::PutChar(0xFF);
	}
	// slau320: StopJtag
	JTEST::SetLow();
	JTCK::SetHigh();
	// Disable Voltage level converter
	SetBusState(BusState::off);
	// Put MCU pins in 3-state
	JtagOff::Enable();
	StopWatch().Delay<Msec(10)>();
}


void JtagDev::OnEnterTap()
{
	JRST::SetLow();
	JTEST::SetLow();		//1
	StopWatch().Delay<Msec(2)>(); // reset TEST logic

	JRST::SetHigh();		//2
	__NOP();
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
	SetBusState(BusState::jtag);
	StopWatch().Delay<Msec(5)>();
}


/*!
Reset target JTAG interface and perform fuse-HW check

This is the slau320aj sequence:
	¯¯¯¯¯¯¯| |¯| |¯| |¯| |¯| |¯| |¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
TCK	       | | | | | | | | | | | |
	       |_| |_| |_| |_| |_| |_|
	         ^   ^   ^   ^   ^   ^
	¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯|   |¯| |¯| |¯|
TMS                                |   | | | | | |
								   |___| |_| |_| |_____

The sequence enters the Test-Logic-Reset, where fuse check happens
(see slau320aj @2.1.2). More pulses may happen, as it will stay locked
in the Test-Logic-Reset state.
A 10µs pause is required for the Run-Test/Idle. TDI needs to clock once
during the Run-Test/Idle state and kept high. This may draw up to 2 mA
during the fuse check.
Note that TCLK (target clock) are always generated for TDI transients 
during the Run-Test/Idle state.

Note that this sequence will be solved using SPI transfer, which may 
generate more pulses than required. Bit banging is also done for the
TMS pulses.
*/
void JtagDev::OnResetTap()
{
	WATCHPOINT();
	//initialize it to high level
	XMitDr8Type::TmsGen::SuspendAutoOutput suspend;
	XMitDr8Type::TmsGen::Set();
	XMitDr8Type::TmsGen::ConfigForResetTap(OnAnticipateTms());
	SpiJtagDevice::PutChar(0xFF); // Keep TDI up
	XMitDr8Type::TmsGen::Stop();

	StopWatch().Delay<Usec(10)>();

	XMitDr8Type::TmsGen::Pulse(false);
	StopWatch().Delay<Usec(5)>();
	XMitDr8Type::TmsGen::Pulse(false);
	StopWatch().Delay<Usec(5)>();
	XMitDr8Type::TmsGen::Pulse(true);	// Restore toggle mode
}


uint8_t JtagDev::OnIrShift(uint8_t instruction)
{
	XMitIr8Type::Transmit(instruction);
	return XMitIr8Type::Decode();
	// JTAG state = Run-Test/Idle
}
uint8_t JtagDevVhc::OnIrShift(uint8_t instruction)
{
	XMitIr8VhcType::Transmit(instruction);
	return XMitIr8VhcType::Decode();
	// JTAG state = Run-Test/Idle
}


uint8_t JtagDev::OnDrShift8(uint8_t data)
{
	XMitDr8Type::Transmit(data);
	return XMitDr8Type::Decode();
	/* JTAG state = Run-Test/Idle */
}
uint8_t JtagDevVhc::OnDrShift8(uint8_t data)
{
	XMitDr8VhcType::Transmit(data);
	return XMitDr8VhcType::Decode();
	/* JTAG state = Run-Test/Idle */
}


uint16_t JtagDev::OnDrShift16(uint16_t data)
{
	XMitDr16Type::Transmit(data);
	return XMitDr16Type::Decode();
	/* JTAG state = Run-Test/Idle */
}
uint16_t JtagDevVhc::OnDrShift16(uint16_t data)
{
	XMitDr16VhcType::Transmit(data);
	return XMitDr16VhcType::Decode();
	/* JTAG state = Run-Test/Idle */
}


uint32_t JtagDev::OnDrShift20(uint32_t data)
{
	XMitDr20Type::Transmit(data);
	data = XMitDr20Type::Decode();

	return ((data << 16) + (data >> 4)) & 0x000FFFFF;
	/* JTAG state = Run-Test/Idle */
}
uint32_t JtagDevVhc::OnDrShift20(uint32_t data)
{
	XMitDr20VhcType::Transmit(data);
	data = XMitDr20VhcType::Decode();

	return ((data << 16) + (data >> 4)) & 0x000FFFFF;
	/* JTAG state = Run-Test/Idle */
}


uint32_t JtagDev::OnDrShift32(uint32_t data)
{
	XMitDr32Type::Transmit(data);
	return XMitDr32Type::Decode();
}
uint32_t JtagDevVhc::OnDrShift32(uint32_t data)
{
	XMitDr32VhcType::Transmit(data);
	return XMitDr32VhcType::Decode();
}


void JtagDev::OnSetTclk()
{
	MuteSpiClk mute_clk;
	SpiJtagDevice::PutChar(0xff);
}


void JtagDev::OnClearTclk()
{
	MuteSpiClk mute_clk;
	SpiJtagDevice::PutChar(0x00);
}


void JtagDev::OnPulseTclk()
{
	MuteSpiClk mute_clk;
	SpiJtagDevice::PutChar(0xf0);
}


void JtagDev::OnPulseTclkN()
{
	MuteSpiClk mute_clk;
	SpiJtagDevice::PutChar(0x0f);
}


void JtagDev::OnPulseTclk(int count)
{
	MuteSpiClk mute_clk;
	SpiJtagDevice::Repeat(0xf0, count);
}


void JtagDev::OnFlashTclk(uint32_t min_pulses)
{
#if OPT_USE_SPI_WAVE_GEN

	// Mute JCLK
	MuteSpiClk mute;
	// Sets the SPI to the speed required for JTCLK generation
	SpiJtmsWave::Disable();
	Spi::RawSpiSpeed oldspeed = SpiJtmsWave::SetupSpeed();
	SpiJtmsWave::Enable();
	// Send pulses up to the minimal required
	for (uint32_t pulses = 0; pulses < min_pulses; pulses += kNumPeriods)
		SpiJtmsWave::PutStream(g_JtmsWave, _countof(g_JtmsWave));
	// Restore SPI to previous speed
	SpiJtmsWave::DisableSafe();
	SpiJtmsWave::RestoreSpeed(oldspeed);
	SpiJtmsWave::Enable();
	
#elif OPT_TIMER_DMA_WAVE_GEN

	// Enable GPIO mode for TDI pin	
	JTCLK::SetupPinMode();
	// Run and wait
	JtclkWaveGen::RunEx(min_pulses);
	// Let SPI take control again
	JTCLK_SPI::SetupPinMode();
	// Regardless of state where it stopped, keep GPIO always high
	JTCLK::SetHigh();
	
#endif
}


#endif
