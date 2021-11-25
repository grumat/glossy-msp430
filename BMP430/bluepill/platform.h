#pragma once

//! Platform supports SPI
#define JTAG_USING_SPI	0


#if JTAG_USING_SPI
//! Tied to JCLK is TIM2:CH1 input, used as timer external clock
typedef ExtTimeBase
<
	kTim2					// Timer 2
	, kTI2FP2				// Timer Input 2 (PA1)
	, 9000000UL				// 9 Mhz (72 Mhz / 16)
	, 1						// No prescaler for JCLK clock
	, 0						// Input filter selection (fastest produces ~60ns delay)
	, kSlaveModeExternal	// Enable external clock using rising edge
> ExternJClk;

//! TIM2 peripheral instance
typedef TimerTemplate
<
	ExternJClk				// Associate external clock to a timer handler template class
	, kSingleShot			// Single shot timer
	, 65535					// Don't care, so use max value
	, false					// No buffer as DMA will modify on the fly
> TmsShapeTimer;

//! PA0 (TIM2:TIM2_CH4) is used as output pin
typedef TimerOutputChannel
<
	TmsShapeTimer			// Associate timer class to the output
	, kTimCh4				// Channel 4 is out output (PA3)
	, kTimOutLow			// TMS level defaults to low
	, kTimOutActiveHigh
	, kTimOutInactive		// No negative output
	, false					// No preload
	, false					// Fast mode has no effect in timer pulse mode
> TmsShapeOutTimerChannel;

//! GPIO settings for the timer input pin
typedef TIM2_CH2_PA1_IN TmsShapeGpioIn;
//! GPIO settings for the timer output pin
typedef TIM2_CH4_PA3_OUT TmsShapeGpioOut;

#else 

typedef PinUnchanged<1> TmsShapeGpioIn;

#endif

//! Dedicated pin for write TMS
typedef GpioTemplate<PA, 3, kOutput50MHz, kPushPull, kLow> JTMS;
typedef InputPullDownPin<PA, 3> JTMS_Init;

//! Pin for TCK output
typedef GpioTemplate<PA, 5, kOutput50MHz, kPushPull, kHigh> JTCK;
typedef InputPullUpPin<PA, 5> JTCK_Init;
//! Special setting for JTCK using SPI
typedef SPI1_SCK_PA5 JTCK_SPI;

//! Pin for TDO input (output on MCU)
typedef FloatingPin<PA, 6> JTDO;
typedef InputPullUpPin<PA, 6> JTDO_Init;
//! Special setting for JTDO using SPI
typedef SPI1_MISO_PA6 JTDO_SPI;

//! Pin for TDI output (input on MCU)
typedef GpioTemplate<PA, 7, kOutput50MHz, kPushPull, kLow> JTDI;
typedef InputPullDownPin<PA, 7> JTDI_Init;
typedef JTDI	JTCLK;
//! Special setting for JTCLK using SPI
typedef SPI1_MOSI_PA7 JTCLK_Out_SPI;
//! Special setting for JTDI using SPI
typedef SPI1_MOSI_PA7 JTDI_SPI;

//! Pin for RST output
typedef GpioTemplate<PA, 2, kOutput50MHz, kOpenDrain, kHigh> JRST;
typedef InputPullUpPin<PA, 2> JRST_Init;

//! Pin for TEST output
typedef GpioTemplate<PA, 4, kOutput50MHz, kPushPull, kLow> JTEST;
typedef InputPullDownPin<PA, 4> JTEST_Init;

//! Pin for SBWDIO input (TODO)
typedef GpioTemplate<PB, 4, kOutput50MHz, kOpenDrain, kHigh> SBWDIO_In;
typedef InputPullUpPin<PB, 4> SBWDIO_In_Init;

//! Pin for SBWDIO output (TODO)
typedef GpioTemplate<PB, 5, kOutput50MHz, kOpenDrain, kHigh> SBWDIO;
typedef InputPullUpPin<PB, 5> SBWDIO_Init;

//! Pin for SBWCLK output
typedef JTCK	SBWCLK;

//! Pin for Jtag Enable control
typedef GpioTemplate<PB, 1, kOutput50MHz, kPushPull, kLow> JENA;

//! Pin for LED output
typedef GpioTemplate<PC, 13, kOutput50MHz, kPushPull, kHigh> RED_LED;

//! No green LED available
typedef GpioTemplate<PB, 0, kOutput50MHz, kPushPull, kLow> GREEN_LED;

typedef GpioPortTemplate <PA
	, PinUnused<0>
	, TmsShapeGpioIn	// TIM2 external clock input
	, JRST_Init			// bit bang
	, JTMS_Init			// bit bang
	, JTEST_Init		// bit bang
	, JTCK_Init			// bit bang / SPI1_SCK
	, JTDO_Init			// bit bang / SPI1_MISO
	, JTDI_Init			// bit bang / SPI1_MOSI
	, PinUnused<8>
	, USART1_TX_PA9		// GDB UART port
	, USART1_RX_PA10	// GDB UART port
	, PinUnused<11>		// USB-
	, PinUnused<12>		// USB+
	, PinUnused<13>
	, PinUnused<14>
	, PinUnused<15>
> PORTA;

// This group is used only for initialization of the GPIOB
typedef GpioPortTemplate <PB
	, GREEN_LED
	, JENA
	, PinUnused<2>
	, TRACESWO			// ARM trace pin
	, SBWDIO_In_Init	// SPI or bit bang (tied to SBW connector)
	, SBWDIO_Init		// SPI or bit bang (with resistor to SBWDIO_In)
	, PinUnused<6>
	, PinUnused<7>
	, PinUnused<8>
	, PinUnused<9>
	, PinUnused<10>
	, PinUnused<11>
	, PinUnused<12>
	, PinUnused<13>
	, PinUnused<14>
	, PinUnused<15>
> PORTB;

typedef GpioPortTemplate <PC
	, PinUnused<0>
	, PinUnused<1>
	, PinUnused<2>
	, PinUnused<3>
	, PinUnused<4>
	, PinUnused<5>
	, PinUnused<6>
	, PinUnused<7>
	, PinUnused<8>
	, PinUnused<9>
	, PinUnused<10>
	, PinUnused<11>
	, PinUnused<12>
	, RED_LED
	, PinUnused<14>
	, PinUnused<15>
> PORTC;

// This group activates JTAG bus using bit-banging
typedef GpioPortTemplate <PA
	, PinUnchanged<0>
	, TmsShapeGpioIn
	, JRST
	, JTMS
	, JTEST
	, JTCK
	, JTDO
	, JTDI
	, PinUnchanged<8>
	, PinUnchanged<9>
	, PinUnchanged<10>
	, PinUnchanged<11>
	, PinUnchanged<12>
	, PinUnchanged<13>
	, PinUnchanged<14>
	, PinUnchanged<15>
> JtagOn;

// This group deactivates JTAG bus
typedef GpioPortTemplate <PA
	, PinUnchanged<0>
	, TmsShapeGpioIn
	, JRST_Init
	, JTMS_Init
	, JTEST_Init
	, JTCK_Init
	, JTDO_Init
	, JTDI_Init
	, PinUnchanged<8>
	, PinUnchanged<9>
	, PinUnchanged<10>
	, PinUnchanged<11>
	, PinUnchanged<12>
	, PinUnchanged<13>
	, PinUnchanged<14>
	, PinUnchanged<15>
> JtagOff;

// This group activates SPI mode for JTAG, after it was activated in bit-bang mode
typedef GpioPortTemplate <PA
	, PinUnchanged<0>
	, TmsShapeGpioIn
	, PinUnchanged<2>
	, TIM2_CH4_PA3_OUT
	, PinUnchanged<4>
	, JTCK_SPI
	, JTDO_SPI
	, JTDI_SPI
	, PinUnchanged<8>
	, PinUnchanged<9>
	, PinUnchanged<10>
	, PinUnchanged<11>
	, PinUnchanged<12>
	, PinUnchanged<13>
	, PinUnchanged<14>
	, PinUnchanged<15>
> JtagSpiOn;


// Crystal on external clock for this project
typedef HseTemplate<8000000UL> HSE;
// 72 MHz is Max freq
typedef PllTemplate<HSE, 72000000UL> PLL;
// Set the clock tree
typedef SysClkTemplate<PLL, 1, 2, 1> SysClk;

// USART2 for GDB port
typedef UsartTemplate<kUsart1, SysClk, 115200> UsartGdbSettings;

// SPI channel for JTAG
static constexpr SpiInstance kSpiForJtag = SpiInstance::kSpi1;
//static constexpr uint32_t kSpiClock = ExternJClk::kFrequency_;
// Timer for JTAG wave generation
static constexpr TimInstance kTimForJtag = TimInstance::kTim1;
static constexpr TimChannel kTimChForJtag = TimChannel::kTimCh1;

ALWAYS_INLINE void RedLedOn() { RED_LED::SetLow(); }
ALWAYS_INLINE void RedLedOff() { RED_LED::SetHigh(); }
ALWAYS_INLINE void GreenLedOn() { GREEN_LED::SetLow(); }
ALWAYS_INLINE void GreenLedOff() { GREEN_LED::SetHigh(); }

ALWAYS_INLINE void InterfaceOn() { JENA::SetHigh(); }
ALWAYS_INLINE void InterfaceOff() { JENA::SetLow(); }

