/*!
\file bluepill-v2/platform.h
\brief Definitions specific for the Blue/Black Pill combined Development board
*/
#pragma once

using namespace Bmt;
using namespace Bmt::Gpio;

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
static constexpr Tim kJtmsShapeTimer = Tim::k1;		// Timer 1
/// External clock source from SPI
static constexpr ExtClockSource kJtmsTimerClk = ExtClockSource::kTI1FP1Clk; // Timer Input 1 (PA8)
//! PA9 (TIM1:TIM1_CH3) is used as output pin
static constexpr TimChannel kTmsOutChannel = TimChannel::k3;


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
typedef Unchanged<8> TmsShapeGpioIn;

#endif

/// Dedicated pin for write JTMS
typedef AnyOut<GpioPortId::PA, 10> JTMS;
/// Logic state for JTMS pin initialization
typedef AnyInPd<GpioPortId::PA, 10> JTMS_Init;
/// Special setting for JTMS using SPI
typedef TIM1_CH3_PA10_OUT JTMS_SPI;

/// Pin for JTCK output
typedef AnyOut<GpioPortId::PA, 5, Speed::kFast, Level::kHigh> JTCK;
/// Logic state for JTCK pin initialization
typedef AnyInPu<GpioPortId::PA, 5> JTCK_Init;
/// Special setting for JTCK using SPI
typedef SPI1_SCK_PA5 JTCK_SPI;

/// Pin for JTDO input (output on MCU)
typedef AnyInPu<GpioPortId::PA, 6> JTDO;
/// Logic state for JTDO pin initialization
typedef AnyInPu<GpioPortId::PA, 6> JTDO_Init;
/// Special setting for JTDO using SPI
typedef SPI1_MISO_PA6 JTDO_SPI;

/// Pin for JTDI output (input on MCU)
typedef AnyOut<GpioPortId::PA, 7, Speed::kFast, Level::kHigh> JTDI;
/// Logic state for JTDI pin initialization
typedef AnyInPu<GpioPortId::PA, 7> JTDI_Init;

/// JTDI during run/idle state produces JTCLK
typedef JTDI JTCLK;
/// Special setting for JTCLK using SPI
typedef SPI1_MOSI_PA7 JTCLK_SPI;
/// Special setting for JTDI using SPI
typedef SPI1_MOSI_PA7 JTDI_SPI;

/// Pin for JRST output
typedef AnyOut<GpioPortId::PA, 1> JRST;
/// Logic state for JRST pin initialization
typedef AnyInPu<GpioPortId::PA, 1> JRST_Init;

/// Pin for JTEST output
typedef AnyOut<GpioPortId::PA, 4> JTEST;
/// Logic state for JTEST pin initialization
typedef AnyInPd<GpioPortId::PA, 4> JTEST_Init;

/// Pin for SBWDIO input
typedef JTDO SBWDIO_In;

/// Pin for SBWDIO output
typedef JTDI SBWDIO;

/// Pin for SBWCLK output
typedef JTCK SBWCLK;

/// Pin for SBWO Enable control
typedef AnyOut<GpioPortId::PA, 9, Speed::kLow, Level::kHigh> SBWO;

/// Pin for ENA1N control
typedef AnyOut<GpioPortId::PB, 12, Speed::kLow, Level::kHigh> ENA1N;

/// Pin for ENA2N control
typedef AnyOut<GpioPortId::PB, 13, Speed::kLow, Level::kHigh> ENA2N;

/// Pin for ENA3N control
typedef AnyOut<GpioPortId::PB, 14, Speed::kLow, Level::kHigh> ENA3N;

/// LED driver activation (LEDS connected in Series will not light, if not driven)
typedef AnyIn<GpioPortId::PC, 13, PuPd::kFloating> LEDS_Init;
/// Pin for LED output
typedef AnyOut<GpioPortId::PC, 13, Speed::kLow, Level::kHigh> LEDS;

/// PWM 3.3V target voltage
typedef AnyOut<GpioPortId::PB, 8, Speed::kLow, Level::kLow> PWM_VT_0V;
/// PWM 3.3V target voltage
typedef AnyOut<GpioPortId::PB, 8, Speed::kLow, Level::kHigh> PWM_VT_3V3;
/// PWM target voltage modulation
typedef TIM4_CH3_PB8_OUT PWM_VT;

/// Initial configuration for PORTA
typedef AnyPortSetup <GpioPortId::PA
	, Unused<0>				///< Vref (pending)
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
	, Unused<11>				///< USB-
	, Unused<12>				///< USB+
	, Unused<13>				///< STM32 TMS/SWDIO
	, Unused<14>				///< STM32 TCK/SWCLK
	, Unused<15>				///< STM32 TDI
> PORTA;

/// Initial configuration for PORTB
typedef AnyPortSetup <GpioPortId::PB
	, Unused<0>				///< not used
	, Unused<1>				///< not used
	, Unused<2>				///< STM32 BOOT1
	, TRACESWO					///< ARM trace pin
	, Unused<4>				///< STM32 JNTRST
	, Unused<5>				///< not used
	, USART1_TX_PB6				///< GDB UART port
	, USART1_RX_PB7				///< GDB UART port
	, PWM_VT_3V3				///< Target power on
	, Unused<9>				///< not used
	, Unused<10>				///< not used
	, Unused<11>				///< not used
	, ENA1N						///< ENA1N in Hi-Z
	, ENA2N						///< ENA2N in Hi-Z
	, ENA3N						///< ENA3N in Hi-Z
	, Unused<15>				///< not used
> PORTB;

/// Initial configuration for PORTC
typedef AnyPortSetup <GpioPortId::PC
	, Unused<0>				///< not used
	, Unused<1>				///< not used
	, Unused<2>				///< not used
	, Unused<3>				///< not used
	, Unused<4>				///< not used
	, Unused<5>				///< not used
	, Unused<6>				///< not used
	, Unused<7>				///< not used
	, Unused<8>				///< not used
	, Unused<9>				///< not used
	, Unused<10>				///< not used
	, Unused<11>				///< not used
	, Unused<12>				///< not used
	, LEDS_Init					///< Inactive LED
	, Unused<14>				///< not used
	, Unused<15>				///< not used
> PORTC;

/// Initial configuration for PORTC
typedef AnyPortSetup <GpioPortId::PD
	, Unchanged<0>			///< OSC_IN
	, Unchanged<1>			///< OSC_OUT
> PORTD;


/// This configuration activates JTAG bus using bit-banging
typedef AnyPortSetup <GpioPortId::PA
	, Unchanged<0>			///< state of pin unchanged
	, JRST						///< JRST pin for bit bang access
	, Unchanged<2>			///< UART2 state of pin unchanged
	, Unchanged<3>			///< UART2 state of pin unchanged
	, JTEST						///< JTEST pin for bit bang access
	, JTCK						///< JTCK pin for bit bang access
	, JTDO						///< JTDO pin for bit bang access
	, JTDI						///< JTDI pin for bit bang access
	, TmsShapeGpioIn			///< Input for TMS shape active
	, Unchanged<9>			///< SBWO is always left unchanged
	, JTMS						///< JTMS pin for bit bang access
	, Unchanged<11>			///< state of pin unchanged
	, Unchanged<12>			///< state of pin unchanged
	, Unchanged<13>			///< state of pin unchanged
	, Unchanged<14>			///< state of pin unchanged
	, Unchanged<15>			///< state of pin unchanged
> JtagOn;

/// This configuration deactivates JTAG bus
typedef AnyPortSetup <GpioPortId::PA
	, Unchanged<0>			///< state of pin unchanged
	, JRST_Init					///< JRST in Hi-Z
	, Unchanged<2>			///< UART2 state of pin unchanged
	, Unchanged<3>			///< UART2 state of pin unchanged
	, JTEST_Init				///< JTEST in Hi-Z
	, JTCK_Init					///< JTCK in Hi-Z
	, JTDO_Init					///< JTDO in Hi-Z
	, JTDI_Init					///< JTDI in Hi-Z
	, TmsShapeGpioIn			///< Keep as input
	, Unchanged<9>			///< SBWO is always left unchanged
	, JTMS_Init					///< JTMS in Hi-Z
	, Unchanged<11>			///< state of pin unchanged
	, Unchanged<12>			///< state of pin unchanged
	, Unchanged<13>			///< state of pin unchanged
	, Unchanged<14>			///< state of pin unchanged
	, Unchanged<15>			///< state of pin unchanged
> JtagOff;

/// This configuration activates SPI mode for JTAG, after it was activated in bit-bang mode
typedef AnyPortSetup <GpioPortId::PA
	, Unchanged<0>			///< state of pin unchanged
	, JRST						///< JRST is still used in bit bang mode
	, Unchanged<2>			///< UART2 state of pin unchanged
	, Unchanged<3>			///< UART2 state of pin unchanged
	, JTEST						///< JTEST is still used in bit bang mode
	, JTCK_SPI					///< setup JTCK pin for SPI mode
	, JTDO_SPI					///< setup JTDO pin for SPI mode
	, JTDI_SPI					///< setup JTDI pin for SPI mode
	, TmsShapeGpioIn			///< input for pulse shaper
	, Unchanged<9>			///< SBWO is always left unchanged
	, JTMS_SPI					///< setup JTMS pin for SPI mode
	, Unchanged<11>			///< state of pin unchanged
	, Unchanged<12>			///< state of pin unchanged
	, Unchanged<13>			///< state of pin unchanged
	, Unchanged<14>			///< state of pin unchanged
	, Unchanged<15>			///< state of pin unchanged
> JtagSpiOn;


/// Crystal on external clock for this project
typedef HseTemplate<8000000UL> HSE;
/// 72 MHz is Max freq
typedef PllTemplate<HSE, 72000000UL> PLL;
/// Set the clock tree
typedef SysClkTemplate<PLL, kAhbPres_1, kApbPres_2, kApbPres_1> SysClk;

#ifdef OPT_USART_ISR
/// USART1 for GDB port
typedef UsartTemplate<Usart::k1, SysClk, 115200> UsartGdbSettings;
#endif

#if OPT_JTAG_USING_SPI
/// SPI channel for JTAG
static constexpr Spi kSpiForJtag = Spi::k1;
/// Timer for JTAG TMS generation
static constexpr Tim kTimForTms = Tim::k1;
/// Timer channel for JTAG TMS generation
static constexpr TimChannel kTimChForTms = TimChannel::k3;
#endif
/// TIM/DMA/GPIO wave generation required for JTCLK generation
#define OPT_TIMER_DMA_WAVE_GEN	1
#if OPT_TIMER_DMA_WAVE_GEN
/// Frequency for generation (MSP430 flash max freq is 476kHz; two cycles per pulse)
static constexpr uint32_t kTimDmaWavFreq = 2 * 450000; // slightly lower because of inherent jitter
/// Timer for JTCLK wave generation
static constexpr Tim kTimDmaWavBeat = Tim::k3;
/// Timer for JTCLK wave count
static constexpr Tim kTimForJtclkCnt = Tim::k2;
/// The TIM2 CH3 triggers DMA on every beat
static constexpr TimChannel kTimChOnBeatDma = TimChannel::k3;
/// 
static constexpr TimChannel kTimChOnStopTimers = TimChannel::k4;
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
typedef GpioEnum <GpioPortId::PB
	, ENA1N						///< Controls lower debug bus
	, ENA2N						///< Controls upper debug bus
> DEBUG_BUS_CTRL;

ALWAYS_INLINE void SetBusState(const BusState st)
{
	DEBUG_BUS_CTRL::SetComplement((uint32_t)st);
}
