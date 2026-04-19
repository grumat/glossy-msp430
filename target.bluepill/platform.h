/*!
\file bluepill-v2/platform.h
\brief Definitions specific for the Blue/Black Pill combined Development board
*/
#pragma once

using namespace Bmt;
using namespace Bmt::Gpio;

#include "platform-defs.h"
#include "drivers/BusStates.h"
#include "drivers/LedStates.h"

/// Platform uses SPI optimized hardware
#define OPT_JTAG_IMPLEMENTATION		OPT_JTAG_IMPL_SPI
/// If SPI clock is SYSCLK/8 internal delays breaks TMS signal.
/// Pulse Anticipation is required. Specifies the speed level (2-5). Use 9 to disable.
#define OPT_TMS_VERY_HIGH_CLOCK	5
/// TIM/DMA/GPIO wave generation required for JTCLK generation
#define OPT_JTAG_TCLK_IMPLEMENTATION	OPT_JTCLK_IMPL_TIM_DMA
/// Implementation for "GDB serial port" (USART used provisory until USB VCP is added to firmware)
#define OPT_GDB_IMPLEMENTATION			OPT_GDB_IMPL_USART1
/// ISR handler for "DMA Transfer Complete"
#define OPT_JTAG_DMA_ISR "DMA1_Channel4_IRQHandler"


/// Crystal on external clock for this project
using HSE = Clocks::AnyHse<8000000UL>;
/// 72 MHz is Max freq
using PLL = Clocks::AnyPll<HSE, 72000000UL>;
/// Set the clock tree
using SysClk = Clocks::AnySycClk<
	PLL
	, Clocks::AhbPrscl::k1
	, Clocks::ApbPrscl::k2
	, Clocks::ApbPrscl::k1
	, Clocks::AdcPrscl::k8
	, Clocks::SysClkOpts::kUsbClock
	>;


/// Option controlling SPI peripheral for JTAG communication
#if OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_SPI || OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_SPI_DMA

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
using TmsShapeGpioIn = TIM1_CH1_PA8_IN;
//! GPIO settings for the timer output pin
using TmsShapeGpioOut = TIM1_CH3_PA10_OUT;

#else 

/// Bitbanging pin
using TmsShapeGpioIn = Unchanged<8>;

#endif

/// Dedicated pin for write JTMS
using JTMS = AnyOut<Port::PA, 10>;
/// Logic state for JTMS pin initialization
using JTMS_Init = AnyInPd<Port::PA, 10>;
#if OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_SPI || OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_SPI_DMA
/// Special setting for JTMS using SPI
using JTMS_SPI = TIM1_CH3_PA10_OUT;
#endif

/// Pin for JTCK output
using JTCK = AnyOut<Port::PA, 5, Speed::kFast, Level::kHigh>;
/// Logic state for JTCK pin initialization
using JTCK_Init = AnyInPu<Port::PA, 5>;
#if OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_SPI || OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_SPI_DMA
/// Special setting for JTCK using SPI
using JTCK_SPI = SPI1_SCK_PA5;
#endif

/// Pin for JTDO input (output on MCU)
using JTDO = AnyInPu<Port::PA, 6>;
/// Logic state for JTDO pin initialization
using JTDO_Init = AnyInPu<Port::PA, 6>;
#if OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_SPI || OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_SPI_DMA
/// Special setting for JTDO using SPI
using JTDO_SPI = SPI1_MISO_PA6;
#endif

/// Pin for JTDI output (input on MCU)
using JTDI = AnyOut<Port::PA, 7, Speed::kFast, Level::kHigh>;
/// Logic state for JTDI pin initialization
using JTDI_Init = AnyInPd<Port::PA, 7>;

/// JTDI during run/idle state produces JTCLK
using JTCLK = JTDI;
#if OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_SPI || OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_SPI_DMA
/// Special setting for JTCLK using SPI
using JTCLK_SPI = SPI1_MOSI_PA7;
/// Special setting for JTDI using SPI
using JTDI_SPI = SPI1_MOSI_PA7;
#endif

/// Pin for JRST output
using JRST = AnyOut<Port::PA, 1>;
/// Logic state for JRST pin initialization
using JRST_Init = AnyInPu<Port::PA, 1>;

/// Pin for JTEST output
using JTEST = AnyOut<Port::PA, 4>;
/// Logic state for JTEST pin initialization
using JTEST_Init = AnyInPd<Port::PA, 4>;

/// Pin for SBWDIO input
using SBWDIO_In = JTDO;

/// Pin for SBWDIO output
using SBWDIO = JTDI;

/// Pin for SBWCLK output
using SBWCLK = JTCK;

/// Pin for SBWO Enable control
using SBWO = AnyOut<Port::PA, 9, Speed::kSlow, Level::kHigh>;

/// Pin for ENA1N control
using ENA1N = AnyOut<Port::PB, 12, Speed::kSlow, Level::kHigh>;

/// Pin for ENA2N control
using ENA2N = AnyOut<Port::PB, 13, Speed::kSlow, Level::kHigh>;

/// Pin for ENA3N control
using ENA3N = AnyOut<Port::PB, 14, Speed::kSlow, Level::kHigh>;

/// LED driver activation (LEDS connected in Series will not light, if not driven)
using LEDS_Init = AnyIn<Port::PC, 13, PuPd::kFloating>;
/// Pin for LED output
using LEDS = AnyOut<Port::PC, 13, Speed::kSlow, Level::kHigh>;

/// PWM 3.3V target voltage
using PWM_VT_0V = AnyOut<Port::PB, 8, Speed::kSlow, Level::kLow>;
/// PWM 3.3V target voltage
using PWM_VT_3V3 = AnyOut<Port::PB, 8, Speed::kSlow, Level::kHigh>;
/// PWM target voltage modulation
using PWM_VT = TIM4_CH3_PB8_OUT;

/// Initial configuration for PORTA
using PORTA = AnyPortSetup <Port::PA
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
>;

/// Initial configuration for PORTB
using PORTB = AnyPortSetup <Port::PB
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
>;

/// Initial configuration for PORTC
using PORTC = AnyPortSetup <Port::PC
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
>;

/// Initial configuration for PORTC
using PORTD = AnyPortSetup <Port::PD
	, Unchanged<0>			///< OSC_IN
	, Unchanged<1>			///< OSC_OUT
>;


/// This configuration activates JTAG bus using bit-banging
using JtagOn = AnyPinGroup <Port::PA
	, JRST					///< JRST pin for bit bang access
	, JTEST					///< JTEST pin for bit bang access
	, JTCK					///< JTCK pin for bit bang access
	, JTDO					///< JTDO pin for bit bang access
	, JTDI					///< JTDI pin for bit bang access
	, TmsShapeGpioIn		///< Input for TMS shape active
	, JTMS					///< JTMS pin for bit bang access
>;

/// This configuration deactivates JTAG bus
using JtagOff = AnyPinGroup <Port::PA
	, JRST_Init				///< JRST in Hi-Z
	, JTEST_Init			///< JTEST in Hi-Z
	, JTCK_Init				///< JTCK in Hi-Z
	, JTDO_Init				///< JTDO in Hi-Z
	, JTDI_Init				///< JTDI in Hi-Z
	, TmsShapeGpioIn		///< Keep as input
	, JTMS_Init				///< JTMS in Hi-Z
>;

#if OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_SPI || OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_SPI_DMA
/// This configuration activates SPI mode for JTAG, after it was activated in bit-bang mode
using JtagSpiOn = AnyPinGroup <Port::PA
	, JRST					///< JRST is still used in bit bang mode
	, JTEST					///< JTEST is still used in bit bang mode
	, JTCK_SPI				///< setup JTCK pin for SPI mode
	, JTDO_SPI				///< setup JTDO pin for SPI mode
	, JTDI_SPI				///< setup JTDI pin for SPI mode
	, TmsShapeGpioIn		///< input for pulse shaper
	, JTMS_SPI				///< setup JTMS pin for SPI mode
>;
#endif

#if OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_SPI || OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_SPI_DMA
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

#if OPT_JTAG_TCLK_IMPLEMENTATION == OPT_JTCLK_IMPL_TIM_DMA
/// Frequency for generation (MSP430 flash max freq is 476kHz; two cycles per pulse)
static constexpr uint32_t kTimDmaWavFreq = 2 * 450000; // slightly lower because of inherent jitter
/// Timer for JTCLK wave generation
static constexpr Timer::Unit kTimDmaWavBeat = Timer::kTim3;
/// Timer for JTCLK wave count
static constexpr Timer::Unit kTimForJtclkCnt = Timer::kTim2;
#endif

#if OPT_JTAG_TCLK_IMPLEMENTATION == OPT_JTCLK_IMPL_SPI
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


using TickTimer = Timer::SysTickCounter<SysClk>;


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
using DEBUG_BUS_CTRL = Gpio::AnyCounter <
	ENA1N						///< Controls lower debug bus
	, ENA2N						///< Controls upper debug bus
>;

ALWAYS_INLINE void SetBusState(const BusState st)
{
	// Hardware uses negative logic control lines
	DEBUG_BUS_CTRL::WriteComplement((uint32_t)st);
}
