/*!
\file nucleo-g431kb/platform.h
\brief Definitions specific for the Blue/Black Pill combined Development board
*/
#pragma once

using namespace Bmt;
using namespace Bmt::Gpio;

#include "platform-defs.h"
#include "drivers/BusStates.h"
#include "drivers/LedStates.h"

/// Platform uses SPI optimized hardware
#define OPT_JTAG_IMPLEMENTATION OPT_JTAG_IMPL_SPI_DMA
/// If SPI clock is SYSCLK/8 internal delays breaks TMS signal.
/// Pulse Anticipation is required. Specifies the speed level (2-5). Use 9 to disable.
#define OPT_TMS_VERY_HIGH_CLOCK	9
/// TIM/DMA/GPIO wave generation required for JTCLK generation
#define OPT_JTAG_TCLK_IMPLEMENTATION OPT_JTCLK_IMPL_TIM_DMA
/// Implementation for "GDB serial port" (USART used provisory until USB VCP is added to firmware)
#define OPT_GDB_IMPLEMENTATION OPT_GDB_IMPL_USART2
/// ISR handler for "DMA Transfer Complete"
#define OPT_JTAG_DMA_ISR "DMA1_Channel4_IRQHandler"


/// Crystal on external clock for this project
using HSE = Clocks::AnyHse<24000000UL>;
/// 72 MHz is Max freq
using PLL = Clocks::AnyPll<HSE, 160000000UL, Clocks::AutoRange1>;
/// Set the clock tree
using SysClk = Clocks::AnySycClk<
	PLL
	, Power::Mode::kRange1
	, Clocks::AhbPrscl::k1
	, Clocks::ApbPrscl::k2
	, Clocks::ApbPrscl::k1
	>;


/// Option controlling SPI peripheral for JTAG communication
#if OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_SPI || OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_SPI_DMA

/// Timer used for TMS generation
static constexpr Timer::Unit kJtmsShapeTimer = Timer::kTim1;		// Timer 1
//! PA9 (TIM1:TIM1_CH2) is used as output pin
static constexpr Timer::Channel kTmsOutChannel = Timer::Channel::k2;


/* SPI interface grades */
/// Constant for 10 MHz communication grade
static constexpr uint32_t JTCK_Speed_5 = 10000000UL;
/// Constant for 5 MHz communication grade
static constexpr uint32_t JTCK_Speed_4 = 5000000UL;
/// Constant for 2.5 MHz communication grade
static constexpr uint32_t JTCK_Speed_3 = 2500000UL;
/// Constant for 1.25 MHz communication grade
static constexpr uint32_t JTCK_Speed_2 = 1250000UL;
/// Constant for 0.625 MHz communication grade
static constexpr uint32_t JTCK_Speed_1 = 625000UL;

//! GPIO settings for the timer input pin
using TmsShapeGpioIn = TIM1_CH1_PA8;
//! GPIO settings for the timer output pin
using TmsShapeGpioOut = TIM1_CH2_PA9;

#else 

/// Bitbanging pin
using TmsShapeGpioIn = Unchanged<8>;

#endif

/// Dedicated pin for write JTMS
using JTMS = AnyOut<Port::PA, 9>;
/// Logic state for JTMS pin initialization
using JTMS_Init = AnyInPd<Port::PA, 9>;
#if OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_SPI || OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_SPI_DMA
/// Special setting for JTMS using SPI
using JTMS_SPI = TIM1_CH2_PA9;
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
using JTDI_Init = AnyInPu<Port::PA, 7>;

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
using JTEST = AnyOut<Port::PA, 0>;
/// Logic state for JTEST pin initialization
using JTEST_Init = AnyInPd<Port::PA, 0>;

/// Pin for SBWDIO input
using SBWDIO_In = JTDO;

/// Pin for SBWDIO output
using SBWDIO = JTDI;

/// Pin for SBWCLK output
using SBWCLK = JTCK;

/// Pin for SBWO Enable control
using SBWO = AnyOut<Port::PA, 10, Speed::kSlow, Level::kHigh>;

/// Pin for ENA1N control
using ENA1N = AnyOut<Port::PB, 4, Speed::kSlow, Level::kHigh>;

/// Pin for ENA2N control
using ENA2N = AnyOut<Port::PB, 5, Speed::kSlow, Level::kHigh>;

/// Pin for ENA3N control
using ENA3N = AnyOut<Port::PA, 2, Speed::kSlow, Level::kHigh>;

/// LED driver activation (LEDS connected in Series will not light, if not driven)
using LEDS_Init = AnyIn<Port::PA, 15, PuPd::kFloating>;
/// Pin for LED output
using LEDS = AnyOut<Port::PA, 15, Speed::kSlow, Level::kHigh>;

/// PWM 3.3V target voltage
using DAC_VT_0V = AnyOut<Port::PA, 4, Speed::kSlow, Level::kLow>;
/// PWM 3.3V target voltage
using DAC_VT_3V3 = AnyOut<Port::PA, 4, Speed::kSlow, Level::kHigh>;
/// PWM target voltage modulation
using DAC_VT = DAC1_OUT1_PA4;

/// Initial configuration for PORTA
using PORTA = AnyPortSetup <Port::PA
	, JTEST_Init			///< bit bang
	, JRST_Init				///< bit bang
	, USART2_TX_PA2			///< GDB UART port
	, USART2_RX_PA3			///< GDB UART port
	, DAC_VT_3V3			///< Target power on
	, JTCK_Init				///< bit bang / SPI1_SCK
	, JTDO_Init				///< bit bang / SPI1_MISO
	, JTDI_Init				///< bit bang / SPI1_MOSI
	, TmsShapeGpioIn		///< TIM1 external clock input
	, JTMS_Init				///< TIM1 CH2 output / bit bang
	, SBWO					///< bit bang
	, Unused<11>			///< USB-
	, Unused<12>			///< USB+
	, Unchanged<13>			///< STM32 TMS/SWDIO
	, Unchanged<14>			///< STM32 TCK/SWCLK
	, LEDS_Init				///< LED Red/Green
>;

/// Initial configuration for PORTB
using PORTB = AnyPortSetup <Port::PB
	, Unused<0>				///< Vref (pending)
	, Unused<1>				///< not used
	, Unused<2>				///< STM32 BOOT1
	, TRACESWO_PB3			///< ARM trace pin
	, ENA1N					///< ENA1N in Hi-Z
	, ENA2N					///< ENA2N in Hi-Z
	, USART1_TX_PB6			///< UART2 TX --> JRXD
	, USART1_RX_PB7			///< UART2 RX -- > JTXD
	, Unused<8>				///< not used
	, Unused<9>				///< not used
	, Unused<10>			///< not used
	, Unused<11>			///< not used
	, Unused<12>			///< not used
	, Unused<13>			///< not used
	, Unused<14>			///< not used
	, Unused<15>			///< not used
>;


using PORTC = Bmt::DummyInit;
using PORTD = Bmt::DummyInit;

/// This configuration activates JTAG bus using bit-banging
using JtagOn = AnyPinGroup <Port::PA
	, JTEST					///< JTEST pin for bit bang access
	, JRST					///< JRST pin for bit bang access
	, JTCK					///< JTCK pin for bit bang access
	, JTDO					///< JTDO pin for bit bang access
	, JTDI					///< JTDI pin for bit bang access
	, TmsShapeGpioIn		///< Input for TMS shape active
	, JTMS					///< JTMS pin for bit bang access
>;

/// This configuration deactivates JTAG bus
using JtagOff = AnyPinGroup <Port::PA
	, JTEST_Init			///< JTEST in Hi-Z
	, JRST_Init				///< JRST in Hi-Z
	, JTCK_Init				///< JTCK in Hi-Z
	, JTDO_Init				///< JTDO in Hi-Z
	, JTDI_Init				///< JTDI in Hi-Z
	, TmsShapeGpioIn		///< Keep as input
	, JTMS_Init				///< JTMS in Hi-Z
>;

#if OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_SPI || OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_SPI_DMA
/// This configuration activates SPI mode for JTAG, after it was activated in bit-bang mode
using JtagSpiOn = AnyPinGroup <Port::PA
	, JTEST					///< JTEST is still used in bit bang mode
	, JRST					///< JRST is still used in bit bang mode
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
static constexpr Timer::Channel kTimChForTms = Timer::Channel::k2;
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
static constexpr uint32_t kJtclkSpiClock = 5000000UL;
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
	DEBUG_BUS_CTRL::WriteComplement((uint32_t)st);
}
