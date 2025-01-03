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
#define OPT_JTAG_USING_SPI	0
/// Transfer SPI bytes using DMA
#define OPT_JTAG_USING_DMA	0
/// Currently JTAG speed selection depends on JTAG-over-SPI feature
#define OPT_JTAG_SPEED_SEL		OPT_JTAG_USING_SPI
/// If SPI clock is SYSCLK/8 internal delays breaks TMS signal.
/// Pulse Anticipation is required. Specifies the speed level (2-5). Use 9 to disable.
#define OPT_TMS_VERY_HIGH_CLOCK	5
/// TIM/DMA/GPIO wave generation required for JTCLK generation
#define OPT_TIMER_DMA_WAVE_GEN	1
/// Generates JTCLK using the SPI port and synthetic waves
#define OPT_USE_SPI_WAVE_GEN	0
/// ISR handler for "DMA Transfer Complete"
#define OPT_JTAG_DMA_ISR "DMA1_Channel4_IRQHandler"
/// ISR handler for "GDB serial port" (provisory until USB UART is added to firmware)
#define OPT_USART_ISR "USART1_IRQHandler"


/// Crystal on external clock for this project
typedef Clocks::AnyHse<8000000UL> HSE;
/// 72 MHz is Max freq
typedef Clocks::AnyPll<HSE, 72000000UL> PLL;
/// Set the clock tree
typedef Clocks::AnySycClk<
	PLL
	, Clocks::AhbPrscl::k1
	, Clocks::ApbPrscl::k2
	, Clocks::ApbPrscl::k1
	, Clocks::AdcPrscl::k8
	, Clocks::SysClkOpts::kUsbClock
	> SysClk;


/// Option controlling SPI peripheral for JTAG communication
#if OPT_JTAG_USING_SPI

/// Timer used for TMS generation
static constexpr Timer::Unit kJtmsShapeTimer = Timer::kTim1;		// Timer 1
//! PA10 (TIM1:TIM1_CH3) is used as output pin
static constexpr Timer::Channel kTmsOutChannel = Timer::Channel::k3;


/* SPI interface grades */
/// Constant for 9 MHz communication grade
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
typedef AnyOut<Port::PA, 10> JTMS;
/// Logic state for JTMS pin initialization
typedef AnyInPd<Port::PA, 10> JTMS_Init;
#if OPT_JTAG_USING_SPI
/// Special setting for JTMS using SPI
typedef TIM1_CH3_PA10_OUT JTMS_SPI;
#endif

/// Pin for JTCK output
typedef AnyOut<Port::PA, 5, Speed::kFast, Level::kHigh> JTCK;
/// Logic state for JTCK pin initialization
typedef AnyInPu<Port::PA, 5> JTCK_Init;
#if OPT_JTAG_USING_SPI
/// Special setting for JTCK using SPI
typedef SPI1_SCK_PA5 JTCK_SPI;
#endif

/// Pin for JTDO input (output on MCU)
typedef AnyInPu<Port::PA, 6> JTDO;
/// Logic state for JTDO pin initialization
typedef AnyInPu<Port::PA, 6> JTDO_Init;
#if OPT_JTAG_USING_SPI
/// Special setting for JTDO using SPI
typedef SPI1_MISO_PA6 JTDO_SPI;
#endif

/// Pin for JTDI output (input on MCU)
typedef AnyOut<Port::PA, 7, Speed::kFast, Level::kHigh> JTDI;
/// Logic state for JTDI pin initialization
typedef AnyInPu<Port::PA, 7> JTDI_Init;

/// JTDI during run/idle state produces JTCLK
typedef JTDI JTCLK;
#if OPT_JTAG_USING_SPI
/// Special setting for JTCLK using SPI
typedef SPI1_MOSI_PA7 JTCLK_SPI;
/// Special setting for JTDI using SPI
typedef SPI1_MOSI_PA7 JTDI_SPI;
#endif

/// Pin for JRST output
typedef AnyOut<Port::PA, 1> JRST;
/// Logic state for JRST pin initialization
typedef AnyInPu<Port::PA, 1> JRST_Init;

/// Pin for JTEST output
typedef AnyOut<Port::PA, 4> JTEST;
/// Logic state for JTEST pin initialization
typedef AnyInPd<Port::PA, 4> JTEST_Init;

/// Pin for SBWDIO input
typedef JTDO SBWDIO_In;

/// Pin for SBWDIO output
typedef JTDI SBWDIO;

/// Pin for SBWCLK output
typedef JTCK SBWCLK;

/// Pin for SBWO Enable control
typedef AnyOut<Port::PA, 9, Speed::kSlow, Level::kHigh> SBWO;

/// Pin for ENA1N control
typedef AnyOut<Port::PB, 12, Speed::kSlow, Level::kHigh> ENA1N;

/// Pin for ENA2N control
typedef AnyOut<Port::PB, 13, Speed::kSlow, Level::kHigh> ENA2N;

/// Pin for ENA3N control
typedef AnyOut<Port::PB, 14, Speed::kSlow, Level::kHigh> ENA3N;

/// LED driver activation (LEDS connected in Series will not light, if not driven)
typedef AnyIn<Port::PC, 13, PuPd::kFloating> LEDS_Init;
/// Pin for LED output
typedef AnyOut<Port::PC, 13, Speed::kSlow, Level::kHigh> LEDS;

/// PWM 3.3V target voltage
typedef AnyOut<Port::PB, 8, Speed::kSlow, Level::kLow> PWM_VT_0V;
/// PWM 3.3V target voltage
typedef AnyOut<Port::PB, 8, Speed::kSlow, Level::kHigh> PWM_VT_3V3;
/// PWM target voltage modulation
typedef TIM4_CH3_PB8_OUT PWM_VT;

/// Initial configuration for PORTA
typedef AnyPortSetup <Port::PA
	, Unused<0>				///< Vref (pending)
	, JRST_Init				///< bit bang
	, USART2_TX_PA2			///< UART2 TX --> JRXD
	, USART2_RX_PA3			///< UART2 RX -- > JTXD
	, JTEST_Init			///< bit bang
	, JTCK_Init				///< bit bang / SPI1_SCK
	, JTDO_Init				///< bit bang / SPI1_MISO
	, JTDI_Init				///< bit bang / SPI1_MOSI
	, TmsShapeGpioIn		///< TIM1 external clock input
	, SBWO					///< bit bang
	, JTMS_Init				///< TIM1 CH3 output / bit bang
	, Unused<11>			///< USB-
	, Unused<12>			///< USB+
	, Unchanged<13>			///< STM32 TMS/SWDIO
	, Unchanged<14>			///< STM32 TCK/SWCLK
	, Unused<15>			///< STM32 TDI
> PORTA;

/// Initial configuration for PORTB
typedef AnyPortSetup <Port::PB
	, Unused<0>				///< not used
	, Unused<1>				///< not used
	, Unused<2>				///< STM32 BOOT1
	, TRACESWO_PB3			///< ARM trace pin
	, Unused<4>				///< STM32 JNTRST
	, Unused<5>				///< not used
	, USART1_TX_PB6			///< GDB UART port
	, USART1_RX_PB7			///< GDB UART port
	, PWM_VT_3V3			///< Target power on
	, Unused<9>				///< not used
	, Unused<10>			///< not used
	, Unused<11>			///< not used
	, ENA1N					///< ENA1N in Hi-Z
	, ENA2N					///< ENA2N in Hi-Z
	, ENA3N					///< ENA3N in Hi-Z
	, Unused<15>			///< not used
> PORTB;

/// Initial configuration for PORTC
typedef AnyPortSetup <Port::PC
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
	, Unused<10>			///< not used
	, Unused<11>			///< not used
	, Unused<12>			///< not used
	, LEDS_Init				///< Inactive LED
	, Unused<14>			///< not used
	, Unused<15>			///< not used
> PORTC;

/// Initial configuration for PORTC
typedef AnyPortSetup <Port::PD
	, Unchanged<0>			///< OSC_IN
	, Unchanged<1>			///< OSC_OUT
> PORTD;


/// This configuration activates JTAG bus using bit-banging
typedef AnyPinGroup <Port::PA
	, JRST					///< JRST pin for bit bang access
	, JTEST					///< JTEST pin for bit bang access
	, JTCK					///< JTCK pin for bit bang access
	, JTDO					///< JTDO pin for bit bang access
	, JTDI					///< JTDI pin for bit bang access
	, TmsShapeGpioIn		///< Input for TMS shape active
	, JTMS					///< JTMS pin for bit bang access
> JtagOn;

/// This configuration deactivates JTAG bus
typedef AnyPinGroup <Port::PA
	, JRST_Init				///< JRST in Hi-Z
	, JTEST_Init			///< JTEST in Hi-Z
	, JTCK_Init				///< JTCK in Hi-Z
	, JTDO_Init				///< JTDO in Hi-Z
	, JTDI_Init				///< JTDI in Hi-Z
	, TmsShapeGpioIn		///< Keep as input
	, JTMS_Init				///< JTMS in Hi-Z
> JtagOff;

#if OPT_JTAG_USING_SPI
/// This configuration activates SPI mode for JTAG, after it was activated in bit-bang mode
typedef AnyPinGroup <Port::PA
	, JRST					///< JRST is still used in bit bang mode
	, JTEST					///< JTEST is still used in bit bang mode
	, JTCK_SPI				///< setup JTCK pin for SPI mode
	, JTDO_SPI				///< setup JTDO pin for SPI mode
	, JTDI_SPI				///< setup JTDI pin for SPI mode
	, TmsShapeGpioIn		///< input for pulse shaper
	, JTMS_SPI				///< setup JTMS pin for SPI mode
> JtagSpiOn;
#endif

#ifdef OPT_USART_ISR
/// USART1 for GDB port
typedef UsartTemplate<Usart::k1, SysClk, 115200> UsartGdbSettings;
#endif

#if OPT_JTAG_USING_SPI
/// SPI channel for JTAG
static constexpr Spi::Iface kSpiForJtag = Spi::Iface::k1;
/// Timer for JTAG TMS generation
static constexpr Timer::Unit kTimForTms = Timer::kTim1;
/// Timer channel for JTAG TMS generation
static constexpr Timer::Channel kTimChForTms = Timer::Channel::k3;
#else
/// Main JTAG signal generation (Must be an advanced timer)
static constexpr Timer::Unit kWaveJtagTimer = Timer::kTim1;
static constexpr Timer::Channel kWaveJtagWriteCh = Timer::Channel::k1;
static constexpr Timer::Channel kWaveJtagRise = Timer::Channel::k2;
static constexpr Timer::Channel kWaveJtagReadCh = Timer::Channel::k4;
#endif

#if OPT_TIMER_DMA_WAVE_GEN
/// Frequency for generation (MSP430 flash max freq is 476kHz; two cycles per pulse)
static constexpr uint32_t kTimDmaWavFreq = 2 * 450000; // slightly lower because of inherent jitter
/// Timer for JTCLK wave generation
static constexpr Timer::Unit kTimDmaWavBeat = Timer::kTim3;
/// Timer for JTCLK wave count
static constexpr Timer::Unit kTimForJtclkCnt = Timer::kTim2;
#endif

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
static constexpr uint32_t kJtclkSpiClock = 4500000UL;
#endif


typedef Timer::SysTickCounter<SysClk> TickTimer;


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
typedef Gpio::AnyCounter <
	ENA1N						///< Controls lower debug bus
	, ENA2N						///< Controls upper debug bus
> DEBUG_BUS_CTRL;

ALWAYS_INLINE void SetBusState(const BusState st)
{
	// Hardware uses negative logic control lines
	DEBUG_BUS_CTRL::WriteComplement((uint32_t)st);
}
