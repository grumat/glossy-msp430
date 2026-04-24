/*!
\file target.stlinkv2/platform.h
\brief Definitions specific for the ST-Link V2 hardware
*/
#pragma once

using namespace Bmt;
using namespace Bmt::Gpio;

#include "platform-defs.h"
#include "drivers/BusStates.h"
#include "drivers/LedStates.h"

/// Platform uses STLinkV2 hardware
#define OPT_JTAG_IMPLEMENTATION			OPT_JTAG_IMPL_TIM_DMA_SLOW
/// TIM/DMA/GPIO wave generation required for JTCLK generation
#define OPT_JTAG_TCLK_IMPLEMENTATION	OPT_JTCLK_IMPL_TIM_DMA
/// Implementation for "GDB serial port" (USART used provisory until USB VCP is added to firmware)
#define OPT_GDB_IMPLEMENTATION			OPT_GDB_IMPL_USART2
#if 0
/// ISR handler for "DMA Transfer Complete"
#define OPT_JTAG_DMA_ISR "DMA1_Channel4_IRQHandler"
#endif
// Use this for Geehy APM32F103CB. It has issues with the SWOTRACE
#define OPT_GEEGY_APM32F103CB			1

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

/// Dedicated pin for write JTMS
typedef AnyOut<Port::PB, 14, Speed::kMedium> JTMS;
/// Logic state for JTMS pin initialization
typedef AnyInPd<Port::PB, 14> JTMS_Init;

/// Pin for JTCK output (GPIO bit-bang mode for OnResetTap and manual control)
typedef AnyOut<Port::PB, 13, Speed::kMedium, Level::kHigh> JTCK;
/// Logic state for JTCK pin initialization
typedef AnyInPu<Port::PB, 13> JTCK_Init;

/// Pin for JTCK output (PWM alternate function via TIM1_CH1N for 50% duty cycle frame generation)
/// Used by GeneratorSTLinkPWM for automatic clock with correct timing
/// Context switches with JTCK: GPIO mode for bit-bang, PWM mode for frame generation
typedef TIM1_CH1N_PB13_OUT JTCK_PWM;

/// Pin for JTDO input (output on MCU)
typedef AnyInPu<Port::PA, 6> JTDO;
/// Logic state for JTDO pin initialization
typedef AnyInPu<Port::PA, 6> JTDO_Init;

/// Pin for JTDI output (input on MCU)
typedef AnyOut<Port::PA, 7, Speed::kMedium, Level::kHigh> JTDI;
/// Logic state for JTDI pin initialization
typedef AnyInPu<Port::PA, 7> JTDI_Init;

/// JTDI during run/idle state produces JTCLK
typedef JTDI JTCLK;

/// Pin for JRST output
typedef AnyOut<Port::PB, 0, Speed::kMedium> JRST;
/// Logic state for JRST pin initialization
typedef AnyInPu<Port::PB, 0> JRST_Init;

/// Pin for JTEST output (TRST)
typedef AnyOut<Port::PB, 1, Speed::kMedium> JTEST;
/// Logic state for JTEST is forced low, since this hardware has a pull up and we have to 
// release the TEST pins of the MSP430
typedef AnyOut<Port::PB, 1, Speed::kMedium> JTEST_Init;

/// Pin for SBWDIO input
typedef JTMS SBWDIO_In;

/// Pin for SBWDIO output
typedef JTMS SBWDIO;

/// Pin for SBWCLK output
typedef JTCK SBWCLK;

/// LED driver activation (LEDS connected in Series will not light, if not driven)
typedef AnyIn<Port::PA, 9, PuPd::kFloating> LEDS_Init;
/// Pin for LED output
typedef AnyOut<Port::PA, 9, Speed::kSlow, Level::kHigh> LEDS;

#if OPT_GEEGY_APM32F103CB
// TRACESWO does not work reliably on Geehy MCU, without configuring the GPIO as output
using TRACESWO = AnyOut<Port::PB, 3, Speed::kFastest, Level::kHigh>;
#else
// Debugger of STM32F103 simply takes full control of GPIO, so this pin is passive
using TRACESWO = TRACESWO_PB3;
#endif

/// Initial configuration for PORTA
typedef AnyPortSetup <Port::PA
	, Unused<0>				///< Vref (pending)
	, Unused<1>				///< not used
	, USART2_TX_PA2			///< UART2 TX --> JRXD
	, USART2_RX_PA3			///< UART2 RX -- > JTXD
	, Unused<4>				///< not used
	, Unused<5>				///< bit bang/TIM+DMA
	, JTDO_Init				///< bit bang/TIM+DMA
	, JTDI_Init				///< bit bang/TIM+DMA
	, Unused<8>				///< not used
	, LEDS_Init				///< Inactive LED
	, Unused<10>			///< not used
	, Unused<11>			///< USB-
	, Unused<12>			///< USB+
	, Unchanged<13>			///< STM32 TMS/SWDIO
	, Unchanged<14>			///< STM32 TCK/SWCLK
	, Unused<15>			///< STM32 TDI
> PORTA;

/// Initial configuration for PORTB
typedef AnyPortSetup <Port::PB
	, JRST_Init				///< bit bang
	, JTEST_Init			///< bit bang
	, Unused<2>				///< STM32 BOOT1
	, TRACESWO				///< ARM trace pin
	, Unused<4>				///< STM32 JNTRST
	, Unused<5>				///< not used
	, Unused<6>				///< not used
	, Unused<7>				///< not used
	, Unused<8>				///< not used
	, Unused<9>				///< not used
	, Unused<10>			///< not used
	, Unused<11>			///< not used
	, Unused<12>			///< not used
	, JTCK_Init				///< bit bang/TIM+DMA+PWM
	, JTMS_Init				///< bit bang/TIM+DMA
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
	, Unused<13>			///< not used
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
	, JTDO					///< JTDO pin for bit bang access
	, JTDI					///< JTDI pin for bit bang access
> JtagOn1;
/// This configuration activates JTAG bus using bit-banging
typedef AnyPinGroup <Port::PB
	, JRST					///< JRST pin for bit bang access
	, JTEST					///< JTEST pin for bit bang access (old TRST)
	, JTCK					///< JTCK pin for bit bang access
	, JTMS					///< JTMS pin for bit bang access
> JtagOn2;

// Operate both ports for JtagOn
typedef PortMerge<JtagOn1, JtagOn2> JtagOn;


/// This configuration deactivates JTAG bus
typedef AnyPinGroup <Port::PA
	, JTDO_Init				///< JTDO in Hi-Z
	, JTDI_Init				///< JTDI in Hi-Z
> JtagOff1;
/// This configuration deactivates JTAG bus
typedef AnyPinGroup <Port::PB
	, JRST_Init				///< JRST in Hi-Z
	, JTEST_Init			///< JTEST in Hi-Z
	, JTCK_Init				///< JTCK in Hi-Z
	, JTMS_Init				///< JTMS in Hi-Z
> JtagOff2;

// Operate both ports for JtagOn
typedef PortMerge<JtagOff1, JtagOff2> JtagOff;

/// Main JTAG signal generation (Must be an advanced timer - TIM1)
static constexpr Timer::Unit kWaveJtagTimer = Timer::kTim1;
/// Timer channels for GeneratorSTLinkPWM (3 DMA + 1 PWM):
/// CH1N PWM for JTCK (50% duty), CH2 for JTMS, CH3 for JTDI, CH4 for JTDO read
static constexpr Timer::Channel kWaveJtagTms = Timer::Channel::k2;		// JTMS control (DMA)
static constexpr Timer::Channel kWaveJtagWrite = Timer::Channel::k3;	// JTDI data (DMA)
static constexpr Timer::Channel kWaveJtagReadCh = Timer::Channel::k4;	// JTDO read (DMA)
static constexpr Timer::Channel kWaveJtagTck = Timer::Channel::k1;		// Channel to generate PWM

#if OPT_JTAG_TCLK_IMPLEMENTATION == OPT_JTCLK_IMPL_TIM_DMA
/// Frequency for generation (MSP430 flash max freq is 476kHz; two cycles per pulse)
static constexpr uint32_t kTimDmaWavFreq = 2 * 450000; // slightly lower because of inherent jitter
/// Timer for JTCLK wave generation
static constexpr Timer::Unit kTimDmaWavBeat = Timer::kTim2;
/// Timer for JTCLK wave count
static constexpr Timer::Unit kTimForJtclkCnt = Timer::kTim3;
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
ALWAYS_INLINE void UartBusOn() { }
/// Disables MSP430 UART interface buffers
ALWAYS_INLINE void UartBusOff() { }


ALWAYS_INLINE void SetBusState(const BusState) { }
