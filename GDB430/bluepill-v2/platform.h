/*!
\file bluepill/platform.h
\brief Definitions specific for the Blue/Black Pill combined Development board
*/
#pragma once

#include "drivers/BusStates.h"
#include "drivers/LedStates.h"

/// Platform supports SPI
#define OPT_JTAG_USING_SPI	1
/// Transfer SPI bytes using DMA
#define OPT_JTAG_USING_DMA	0
/// ISR handler for "DMA Transfer Complete"
#define OPT_JTAG_DMA_ISR "DMA1_Channel4_IRQHandler"
/// ISR handler for "GDB serial port" (provisory until USB UART is added to firmware)
#define OPT_USART_ISR "USART1_IRQHandler"

/// TMS pulse automatic shape generator is used by SPI JTAG mode
#define OPT_TMS_AUTO_SHAPER OPT_JTAG_USING_SPI

/// Option controlling SPI peripheral for JTAG communication
#if OPT_JTAG_USING_SPI

/// Timer used for TMS generation
static constexpr TimInstance kJtmsShapeTimer = kTim1;		// Timer 1
/// External clock source from SPI
static constexpr ExtClockSource kJtmsTimerClk = kTI1FP1Clk;	// Timer Input 1 (PA8)
//! PA9 (TIM1:TIM1_CH3) is used as output pin
static constexpr TimChannel kTmsOutChannel = kTimCh3;


/* SPI interface grades */
/// Constant for 4.5 MHz communication grade
static constexpr uint32_t JTCK_Speed_5 = 9000000UL;
/// Constant for 4.5 MHz communication grade
static constexpr uint32_t JTCK_Speed_4 = 4500000UL;
/// Constant for 2.25 MHz communication grade
static constexpr uint32_t JTCK_Speed_3 = 2250000UL;
/// Constant for 1.125 MHz communication grade
static constexpr uint32_t JTCK_Speed_2 = 1125000UL;
/// Constant for 0.563 MHz communication grade
static constexpr uint32_t JTCK_Speed_1 = 562500UL;

//! GPIO settings for the timer input pin
typedef TIM1_CH1_PA8_IN TmsShapeGpioIn;
//! GPIO settings for the timer output pin
typedef TIM1_CH3_PA10_OUT TmsShapeGpioOut;

#else 

/// Bitbanging pin
typedef PinUnchanged<8> TmsShapeGpioIn;

#endif

/// Dedicated pin for write JTMS
typedef GpioTemplate<Gpio::PA, 10, GpioSpeed::kOutput50MHz, GpioMode::kPushPull, Level::kLow> JTMS;
/// Logic state for JTMS pin initialization
typedef InputPullDownPin<Gpio::PA, 10> JTMS_Init;
/// Special setting for JTMS using SPI
typedef TIM1_CH3_PA10_OUT JTMS_SPI;

/// Pin for JTCK output
typedef GpioTemplate<Gpio::PA, 5, GpioSpeed::kOutput50MHz, GpioMode::kPushPull, Level::kHigh> JTCK;
/// Logic state for JTCK pin initialization
typedef InputPullUpPin<Gpio::PA, 5> JTCK_Init;
/// Special setting for JTCK using SPI
typedef SPI1_SCK_PA5 JTCK_SPI;

/// Pin for JTDO input (output on MCU)
typedef InputPullUpPin<Gpio::PA, 6> JTDO;
/// Logic state for JTDO pin initialization
typedef InputPullUpPin<Gpio::PA, 6> JTDO_Init;
/// Special setting for JTDO using SPI
typedef SPI1_MISO_PA6 JTDO_SPI;

/// Pin for JTDI output (input on MCU)
typedef GpioTemplate<Gpio::PA, 7, GpioSpeed::kOutput50MHz, GpioMode::kPushPull, Level::kHigh> JTDI;
/// Logic state for JTDI pin initialization
typedef InputPullUpPin<Gpio::PA, 7> JTDI_Init;

/// JTDI during run/idle state produces JTCLK
typedef JTDI JTCLK;
/// Special setting for JTCLK using SPI
typedef SPI1_MOSI_PA7 JTCLK_SPI;
/// Special setting for JTDI using SPI
typedef SPI1_MOSI_PA7 JTDI_SPI;

/// Pin for JRST output
typedef GpioTemplate<Gpio::PA, 1, GpioSpeed::kOutput50MHz, GpioMode::kPushPull, Level::kLow> JRST;
/// Logic state for JRST pin initialization
typedef InputPullUpPin<Gpio::PA, 1> JRST_Init;

/// Pin for JTEST output
typedef GpioTemplate<Gpio::PA, 4, GpioSpeed::kOutput50MHz, GpioMode::kPushPull, Level::kLow> JTEST;
/// Logic state for JTEST pin initialization
typedef InputPullDownPin<Gpio::PA, 4> JTEST_Init;

/// Pin for SBWDIO input
typedef JTDO SBWDIO_In;

/// Pin for SBWDIO output
typedef JTDI SBWDIO;

/// Pin for SBWCLK output
typedef JTCK SBWCLK;

/// Pin for SBWO Enable control
typedef GpioTemplate<Gpio::PA, 9, GpioSpeed::kOutput2MHz, GpioMode::kPushPull, Level::kHigh> SBWO;

/// Pin for ENA1N control
typedef GpioTemplate<Gpio::PB, 12, GpioSpeed::kOutput2MHz, GpioMode::kPushPull, Level::kHigh> ENA1N;

/// Pin for ENA2N control
typedef GpioTemplate<Gpio::PB, 13, GpioSpeed::kOutput2MHz, GpioMode::kPushPull, Level::kHigh> ENA2N;

/// Pin for ENA3N control
typedef GpioTemplate<Gpio::PB, 14, GpioSpeed::kOutput2MHz, GpioMode::kPushPull, Level::kHigh> ENA3N;

/// LED driver activation (LEDS connected in Series will not light, if not driven)
typedef GpioTemplate<Gpio::PC, 13, GpioSpeed::kInput, GpioMode::kFloating, Level::kHigh> LEDS_Init;
/// Pin for LED output
typedef GpioTemplate<Gpio::PC, 13, GpioSpeed::kOutput2MHz, GpioMode::kPushPull, Level::kHigh> LEDS;

/// PWM 3.3V target voltage
typedef GpioTemplate<Gpio::PB, 8, GpioSpeed::kOutput2MHz, GpioMode::kPushPull, Level::kLow> PWM_VT_0V;
/// PWM 3.3V target voltage
typedef GpioTemplate<Gpio::PB, 8, GpioSpeed::kOutput2MHz, GpioMode::kPushPull, Level::kHigh> PWM_VT_3V3;
/// PWM target voltage modulation
typedef TIM4_CH3_PB8_OUT PWM_VT;

/// Initial configuration for PORTA
typedef GpioPortTemplate <Gpio::PA
	, PinUnused<0>				///< Vref (pending)
	, JRST_Init					///< bit bang
	, USART2_TX_PA2				///< UART2 TX --> JRXD
	, USART2_RX_PA3				///< UART2 RX -- > JTXD
	, JTEST_Init				///< bit bang
	, JTCK_Init					///< bit bang / SPI1_SCK
	, JTDO_Init					///< bit bang / SPI1_MISO
	, JTDI_Init					///< bit bang / SPI1_MOSI
	, TmsShapeGpioIn			///< TIM1 external clock input
	, SBWO						///< bit bang
	, JTMS_Init					///< TIM1 CH3 output / bit bang
	, PinUnused<11>				///< USB-
	, PinUnused<12>				///< USB+
	, PinUnused<13>				///< STM32 TMS/SWDIO
	, PinUnused<14>				///< STM32 TCK/SWCLK
	, PinUnused<15>				///< STM32 TDI
> PORTA;

/// Initial configuration for PORTB
typedef GpioPortTemplate <Gpio::PB
	, PinUnused<0>				///< not used
	, PinUnused<1>				///< not used
	, PinUnused<2>				///< STM32 BOOT1
	, TRACESWO					///< ARM trace pin
	, PinUnused<4>				///< STM32 JNTRST
	, PinUnused<5>				///< not used
	, USART1_TX_PB6				///< GDB UART port
	, USART1_RX_PB7				///< GDB UART port
	, PWM_VT_3V3				///< Target power on
	, PinUnused<9>				///< not used
	, PinUnused<10>				///< not used
	, PinUnused<11>				///< not used
	, ENA1N						///< ENA1N in Hi-Z
	, ENA2N						///< ENA2N in Hi-Z
	, ENA3N						///< ENA3N in Hi-Z
	, PinUnused<15>				///< not used
> PORTB;

/// Initial configuration for PORTC
typedef GpioPortTemplate <Gpio::PC
	, PinUnused<0>				///< not used
	, PinUnused<1>				///< not used
	, PinUnused<2>				///< not used
	, PinUnused<3>				///< not used
	, PinUnused<4>				///< not used
	, PinUnused<5>				///< not used
	, PinUnused<6>				///< not used
	, PinUnused<7>				///< not used
	, PinUnused<8>				///< not used
	, PinUnused<9>				///< not used
	, PinUnused<10>				///< not used
	, PinUnused<11>				///< not used
	, PinUnused<12>				///< not used
	, LEDS_Init					///< Inactive LED
	, PinUnused<14>				///< not used
	, PinUnused<15>				///< not used
> PORTC;

/// Initial configuration for PORTC
typedef GpioPortTemplate <Gpio::PD
	, PinUnchanged<0>			///< OSC_IN
	, PinUnchanged<1>			///< OSC_OUT
> PORTD;


/// This configuration activates JTAG bus using bit-banging
typedef GpioPortTemplate <Gpio::PA
	, PinUnchanged<0>			///< state of pin unchanged
	, JRST						///< JRST pin for bit bang access
	, PinUnchanged<2>			///< UART2 state of pin unchanged
	, PinUnchanged<3>			///< UART2 state of pin unchanged
	, JTEST						///< JTEST pin for bit bang access
	, JTCK						///< JTCK pin for bit bang access
	, JTDO						///< JTDO pin for bit bang access
	, JTDI						///< JTDI pin for bit bang access
	, TmsShapeGpioIn			///< Input for TMS shape active
	, PinUnchanged<9>			///< SBWO is always left unchanged
	, JTMS						///< JTMS pin for bit bang access
	, PinUnchanged<11>			///< state of pin unchanged
	, PinUnchanged<12>			///< state of pin unchanged
	, PinUnchanged<13>			///< state of pin unchanged
	, PinUnchanged<14>			///< state of pin unchanged
	, PinUnchanged<15>			///< state of pin unchanged
> JtagOn;

/// This configuration deactivates JTAG bus
typedef GpioPortTemplate <Gpio::PA
	, PinUnchanged<0>			///< state of pin unchanged
	, JRST_Init					///< JRST in Hi-Z
	, PinUnchanged<2>			///< UART2 state of pin unchanged
	, PinUnchanged<3>			///< UART2 state of pin unchanged
	, JTEST_Init				///< JTEST in Hi-Z
	, JTCK_Init					///< JTCK in Hi-Z
	, JTDO_Init					///< JTDO in Hi-Z
	, JTDI_Init					///< JTDI in Hi-Z
	, TmsShapeGpioIn			///< Keep as input
	, PinUnchanged<9>			///< SBWO is always left unchanged
	, JTMS_Init					///< JTMS in Hi-Z
	, PinUnchanged<11>			///< state of pin unchanged
	, PinUnchanged<12>			///< state of pin unchanged
	, PinUnchanged<13>			///< state of pin unchanged
	, PinUnchanged<14>			///< state of pin unchanged
	, PinUnchanged<15>			///< state of pin unchanged
> JtagOff;

/// This configuration activates SPI mode for JTAG, after it was activated in bit-bang mode
typedef GpioPortTemplate <Gpio::PA
	, PinUnchanged<0>			///< state of pin unchanged
	, JRST						///< JRST is still used in bit bang mode
	, PinUnchanged<2>			///< UART2 state of pin unchanged
	, PinUnchanged<3>			///< UART2 state of pin unchanged
	, JTEST						///< JTEST is still used in bit bang mode
	, JTCK_SPI					///< setup JTCK pin for SPI mode
	, JTDO_SPI					///< setup JTDO pin for SPI mode
	, JTDI_SPI					///< setup JTDI pin for SPI mode
	, TmsShapeGpioIn			///< input for pulse shaper
	, PinUnchanged<9>			///< SBWO is always left unchanged
	, JTMS_SPI					///< setup JTMS pin for SPI mode
	, PinUnchanged<11>			///< state of pin unchanged
	, PinUnchanged<12>			///< state of pin unchanged
	, PinUnchanged<13>			///< state of pin unchanged
	, PinUnchanged<14>			///< state of pin unchanged
	, PinUnchanged<15>			///< state of pin unchanged
> JtagSpiOn;


/// Crystal on external clock for this project
typedef HseTemplate<8000000UL> HSE;
/// 72 MHz is Max freq
typedef PllTemplate<HSE, 72000000UL> PLL;
/// Set the clock tree
typedef SysClkTemplate<PLL, kAhbPres_1, kApbPres_2, kApbPres_1> SysClk;

#ifdef OPT_USART_ISR
/// USART1 for GDB port
typedef UsartTemplate<kUsart1, SysClk, 115200> UsartGdbSettings;
#endif

#if OPT_JTAG_USING_SPI
/// SPI channel for JTAG
static constexpr SpiInstance kSpiForJtag = SpiInstance::kSpi1;
/// Timer for JTAG TMS generation
static constexpr TimInstance kTimForTms = TimInstance::kTim1;
/// Timer channel for JTAG TMS generation
static constexpr TimChannel kTimChForTms = TimChannel::kTimCh3;
#endif
/// TIM/DMA/GPIO wave generation required for JTCLK generation
#define OPT_TIMER_DMA_WAVE_GEN	1
#if OPT_TIMER_DMA_WAVE_GEN
/// Frequency for generation (MSP430 flash max freq is 476kHz; two cycles per pulse)
static constexpr uint32_t kTimDmaWavFreq = 2 * 450000; // slightly lower because of inherent jitter
/// Timer for JTCLK wave generation
static constexpr TimInstance kTimDmaWavBeat = TimInstance::kTim3;
/// Timer for JTCLK wave count
static constexpr TimInstance kTimForJtclkCnt = TimInstance::kTim2;
/// The TIM2 CH3 triggers DMA on every beat
static constexpr TimChannel kTimChOnBeatDma = TimChannel::kTimCh3;
/// 
static constexpr TimChannel kTimChOnStopTimers = TimChannel::kTimCh4;
#endif

/// Generates JTCLK using the SPI port and synthetic waves
#define OPT_USE_SPI_WAVE_GEN	0
#if OPT_USE_SPI_WAVE_GEN
//#define WAVESET_1_4th	1
//#define WAVESET_2_9th	1
//! JTCLK generated by a wave generator at 1/5th of CLK frequency
#define WAVESET_1_5th	1
//#define WAVESET_2_11	1
//#define WAVESET_1_6	1
//#define WAVESET_1_7	1
//#define WAVESET_1_8	1
//! SPI clock frequency to generate JTCLK (2 cycles per bit: this combo generates 450 kHz)
//static constexpr uint32_t kJtclkSpiClock = 4500000UL;
#endif


typedef SysTickCounter<SysClk> TickTimer;


ALWAYS_INLINE void SetLedState(const LedState st)
{
	switch (st)
	{
	case LedState::on:
		LEDS::SetupPinMode();
		break;
	case LedState::red:
		LEDS::SetHigh();
		break;
	case LedState::green:
		LEDS::SetLow();
		break;
	default:
		LEDS_Init::SetupPinMode();
		break;
	}
}


/// Enables MSP430 UART interface buffers
ALWAYS_INLINE void UartBusOn() { ENA3N::SetLow(); }
/// Disables MSP430 UART interface buffers
ALWAYS_INLINE void UartBusOff() { ENA3N::SetHigh(); }


/// Initial configuration for PORTB
typedef GpioEnum <Gpio::PB
	, ENA1N						///< Controls lower debug bus
	, ENA2N						///< Controls upper debug bus
> DEBUG_BUS_CTRL;

ALWAYS_INLINE void SetBusState(const BusState st)
{
	DEBUG_BUS_CTRL::SetComplement((uint32_t)st);
}
