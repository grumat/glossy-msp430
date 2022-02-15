#pragma once

//! Platform supports SPI
#define OPT_JTAG_USING_SPI	1
//! Transfer SPI bytes using DMA
#define OPT_JTAG_USING_DMA	0
//! ISR handler for "DMA Transfer Complete"
#define OPT_JTAG_DMA_ISR "DMA1_Channel4_IRQHandler"


#if OPT_JTAG_USING_SPI
//! Tied to JCLK is TIM4:CH1 input, used as timer external clock
typedef ExtTimeBase
<
	kTim4					// Timer 4
	, kTI2FP2				// Timer Input 2 (PB7)
	, 9000000UL				// 9 Mhz (72 Mhz / 16)
	, 1						// No prescaler for JCLK clock
	, 0						// Input filter selection (fastest produces ~60ns delay)
	, kSlaveModeExternal	// Enable external clock using rising edge
> ExternJClk;

//! TIM4 peripheral instance
typedef TimerTemplate
<
	ExternJClk				// Associate external clock to a timer handler template class
	, kSingleShot			// Single shot timer
	, 65535					// Don't care, so use max value
	, false					// No buffer as DMA will modify on the fly
> TmsShapeTimer;

//! PB6 (TIM4:TIM4_CH1) is used as output pin
typedef TimerOutputChannel
<
	TmsShapeTimer			// Associate timer class to the output
	, kTimCh1				// Channel 2 is out output (PB6)
	, kTimOutLow			// TMS level defaults to low
	, kTimOutActiveHigh
	, kTimOutInactive		// No negative output
	, false					// No preload
	, false					// Fast mode has no effect in timer pulse mode
> TmsShapeOutTimerChannel;

//! GPIO settings for the timer input pin
typedef TIM4_CH2_PB7_IN TmsShapeGpioIn;
//! GPIO settings for the timer output pin
typedef TIM4_CH1_PB6_OUT TmsShapeGpioOut;

#else 

typedef PinUnchanged<7> TmsShapeGpioIn;

#endif

//! Dedicated pin for write TMS
typedef GpioTemplate<PB, 6, kOutput50MHz, kPushPull, kLow> JTMS;
typedef InputPullDownPin<PB, 6> JTMS_Init;
//! Special setting for JTMS using SPI
typedef TIM4_CH1_PB6_OUT JTMS_SPI;

//! Pin for TCK output
typedef GpioTemplate<PB, 13, kOutput50MHz, kPushPull, kHigh> JTCK;
typedef InputPullUpPin<PB, 13> JTCK_Init;
//! Special setting for JTCK using SPI
typedef SPI2_SCK_PB13 JTCK_SPI;

//! Pin for TDO input (output on MCU)
typedef FloatingPin<PB, 14> JTDO;
typedef InputPullUpPin<PB, 14> JTDO_Init;
//! Special setting for JTDO using SPI
typedef SPI2_MISO_PB14 JTDO_SPI;

//! Pin for TDI output (input on MCU)
typedef GpioTemplate<PB, 15, kOutput50MHz, kPushPull, kLow> JTDI;
typedef InputPullDownPin<PB, 15> JTDI_Init;

//! JTDI during run/idle state produces JTCLK
typedef JTDI	JTCLK;
//! Special setting for JTCLK using SPI
typedef SPI2_MOSI_PB15 JTCLK_Out_SPI;
//! Special setting for JTDI using SPI
typedef SPI2_MOSI_PB15 JTDI_SPI;

//! Pin for RST output
typedef GpioTemplate<PB, 12, kOutput50MHz, kOpenDrain, kHigh> JRST;
typedef InputPullUpPin<PB, 12> JRST_Init;

//! Pin for TEST output
typedef GpioTemplate<PB, 9, kOutput50MHz, kPushPull, kLow> JTEST;
typedef InputPullDownPin<PB, 9> JTEST_Init;

//! Pin for SBWDIO input
typedef JTDO SBWDIO_In;

//! Pin for SBWDIO output
typedef JTDI SBWDIO;

//! Pin for SBWCLK output
typedef JTCK	SBWCLK;

//! Pin for Jtag Enable control
typedef GpioTemplate<PB, 8, kOutput50MHz, kPushPull, kLow> JENA;

//! Pin for LED output
typedef GpioTemplate<PC, 13, kOutput50MHz, kPushPull, kHigh> RED_LED;

//! Pin for green LED
typedef GpioTemplate<PB, 0, kOutput50MHz, kPushPull, kHigh> GREEN_LED;

typedef GpioPortTemplate <PA
	, PinUnused<0>		//!< Vref
	, PinUnused<1>
	, PinUnused<2>
	, PinUnused<3>
	, PinUnused<4>
	, PinUnused<5>
	, PinUnused<6>
	, PinUnused<7>
	, PinUnused<8>
	, USART1_TX_PA9		//!< GDB UART port
	, USART1_RX_PA10	//!< GDB UART port
	, PinUnused<11>		//!< USB-
	, PinUnused<12>		//!< USB+
	, PinUnused<13>		//!< STM32 TMS/SWDIO
	, PinUnused<14>		//!< STM32 TCK/SWCLK
	, PinUnused<15>		//!< STM32 TDI
> PORTA;

// This group is used only for initialization of the GPIOB
typedef GpioPortTemplate <PB
	, GREEN_LED			//!< bit bang
	, PinUnused<1>
	, PinUnused<2>		//!< STM32 BOOT1
	, TRACESWO			//!< ARM trace pin
	, PinUnused<4>		//!< STM32 JNTRST
	, PinUnused<5>
	, JTMS_Init			//!< TIM4 CH1 output / bit bang
	, TmsShapeGpioIn	//!< TIM4 external clock input
	, JENA				//!< bit bang (not used on platform)
	, JTEST_Init		//!< bit bang
	, USART3_TX_PB10	//!< UART3 RX --> JTXD
	, USART3_RX_PB11	//!< UART3 TX -- > JRXD
	, JRST_Init			//!< bit bang
	, JTCK_Init			//!< bit bang / SPI2_SCK
	, JTDO_Init			//!< bit bang / SPI2_MISO
	, JTDI_Init			//!< bit bang / SPI2_MOSI
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
typedef GpioPortTemplate <PB
	, PinUnchanged<0>
	, PinUnchanged<1>
	, PinUnchanged<2>
	, PinUnchanged<3>
	, PinUnchanged<4>
	, PinUnchanged<5>
	, JTMS
	, TmsShapeGpioIn
	, PinUnchanged<8>
	, JTEST
	, PinUnchanged<10>
	, PinUnchanged<11>
	, JRST
	, JTCK
	, JTDO
	, JTDI
> JtagOn;

// This group deactivates JTAG bus
typedef GpioPortTemplate <PB
	, PinUnchanged<0>
	, PinUnchanged<1>
	, PinUnchanged<2>
	, PinUnchanged<3>
	, PinUnchanged<4>
	, PinUnchanged<5>
	, JTMS_Init
	, TmsShapeGpioIn
	, PinUnchanged<8>
	, JTEST_Init
	, PinUnchanged<10>
	, PinUnchanged<11>
	, JRST_Init
	, JTCK_Init
	, JTDO_Init
	, JTDI_Init
> JtagOff;

// This group activates SPI mode for JTAG, after it was activated in bit-bang mode
typedef GpioPortTemplate <PB
	, PinUnchanged<0>
	, PinUnchanged<1>
	, PinUnchanged<2>
	, PinUnchanged<3>
	, PinUnchanged<4>
	, PinUnchanged<5>
	, JTMS_SPI
	, TmsShapeGpioIn
	, PinUnchanged<8>
	, PinUnchanged<9>
	, PinUnchanged<10>
	, PinUnchanged<11>
	, PinUnchanged<12>
	, JTCK_SPI
	, JTDO_SPI
	, JTDI_SPI
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
static constexpr SpiInstance kSpiForJtag = SpiInstance::kSpi2;
// Timer for JTAG wave generation
static constexpr TimInstance kTimForJtag = TimInstance::kTim1;
static constexpr TimChannel kTimChForJtag = TimChannel::kTimCh1;

// Base timer for microsecond delay
typedef TimeBase_us<kTim3, SysClk, 1U> MicroDelayTimeBase;
// Base timer for HW tick counter
typedef TimeBase_us<kTim2, SysClk, 500U> TickTimeBase;

ALWAYS_INLINE void RedLedOn() { RED_LED::SetLow(); }
ALWAYS_INLINE void RedLedOff() { RED_LED::SetHigh(); }
ALWAYS_INLINE void GreenLedOn() { GREEN_LED::SetLow(); }
ALWAYS_INLINE void GreenLedOff() { GREEN_LED::SetHigh(); }

ALWAYS_INLINE void InterfaceOn() { JENA::SetHigh(); }
ALWAYS_INLINE void InterfaceOff() { JENA::SetLow(); }

