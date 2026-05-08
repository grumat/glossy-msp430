/*!
\file target.stlinkv2/platform.h
\brief Definitions specific for the ST-Link V2 hardware

Pins:
	PA0:	VREF / 2
	PA2:	TXD of RS232 (borrowed to GDB until USN support)
	PA3:	RXD of RS232 (borrowed to GDB until USN support)
	PA5:	TCK joined with PB13 (option for SPI CLK)
	PA6:	TDO input (from level converter)
	PA7:	TDI output
	PA9:	LED: High:red; Low:green; floating:off
	PA10:	SWO: not used for MSP430
	PA11:	USB-
	PA12:	USB+
	PA13:	TMS DEBUG Host 
	PA14:	TCK DEBUG Host 
	PA15:	TDI DEBUG Host 
	
	PB0:	/RST: reset target MSP430
	PB1:	TRST: retarget to TEST on MSP430 (adapter board needs a strong pull down for functionality)
	PB3:	TDO DEBUG Host (firmware uses SWO trace function!)
	PB12:	SWD_IN: not used for MSP430
	PB13:	TCK joined with PA5 (option for timer PWM)
	PB14:	TMS output
*/
#pragma once

using namespace Bmt;
using namespace Bmt::Gpio;

#include "platform-defs.h"
#include "drivers/BusStates.h"
#include "drivers/LedStates.h"

/// Uncomment to compile in the bench-only DoLogicAnalyzerTest() routine and
/// invoke it from JtagDev::OnOpen() so a logic analyzer can capture the
/// reference IR/DR/TCLK waveform sequence. Leave undefined for normal builds.
//#define OPT_TEST_WITH_LOGIC_ANALYZER	1

/// Platform uses STLinkV2 hardware.
/// Switch to OPT_JTAG_IMPL_DTRIG to enable the double-trigger SPI+TIM1 driver.
#define OPT_JTAG_IMPLEMENTATION			OPT_JTAG_IMPL_DTRIG
//#define OPT_JTAG_IMPLEMENTATION			OPT_JTAG_IMPL_TIM_DMA_SLOW

/// TIM/DMA/GPIO wave generation required for JTCLK generation
#define OPT_JTAG_TCLK_IMPLEMENTATION	OPT_JTCLK_IMPL_SPI
//#define OPT_JTAG_TCLK_IMPLEMENTATION	OPT_JTCLK_IMPL_TIM_DMA

/// Implementation for "GDB serial port" (USART used provisory until USB VCP is added to firmware)
#define OPT_GDB_IMPLEMENTATION			OPT_GDB_IMPL_USART2
#if 0
/// ISR handler for "DMA Transfer Complete"
#define OPT_JTAG_DMA_ISR "DMA1_Channel4_IRQHandler"
#endif
// Use this for Geehy APM32F103CB. It has issues with the SWOTRACE
#define OPT_GEEGY_APM32F103CB			1




#if OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_TIM_DMA_SLOW

/*!
\brief Single source of truth for which MCU peripherals are owned by this firmware.

`SystemInit()` calls `PeripheralEnabler::Init()` once at boot — this enables
clocks for every listed peripheral and pulses APBxRSTR / AHBRSTR for the ones
that have a reset bit. After that, all per-class `Init()` calls are unnecessary
(and deprecated): callers just `Setup()` the peripheral they want to use.

Add a peripheral here when introducing it to the firmware; pick a free resource
(see CLAUDE.md "Allocate MCU resources wisely") rather than time-multiplexing.
*/
using PeripheralEnabler = Clocks::Enabler<
	// GPIO ports — every port we configure must be listed
	PortClock<Port::PA>,
	PortClock<Port::PB>,
	PortClock<Port::PC>,
	PortClock<Port::PD>,
	// AFIO — required for SWO trace pin remap and any EXTI line
	Afio,
	// DMA1 — covers all channels (CH1 for TIM2/JTCLK count, CH3 SPI, CH4 SPI, CH6 TIM1_CH3 / TIM4_CH1)
	Dma::Controller<Dma::Itf::k1>,
	// Timers used by the firmware
	Timer::TimerDescriptor<Timer::kTim1>,	// JTAG wave generator (advanced timer; CH1N=JTCK, CH2N=JTMS)
	Timer::TimerDescriptor<Timer::kTim2>,
	Timer::TimerDescriptor<Timer::kTim3>,
	Timer::TimerDescriptor<Timer::kTim4>,
	// USART2 — provisional GDB serial (until USB CDC is added)
	UsartHardware<Usart::k2>
>;

#elif OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_DTRIG

/*!
\brief Single source of truth for which MCU peripherals are owned by this firmware.

`SystemInit()` calls `PeripheralEnabler::Init()` once at boot — this enables
clocks for every listed peripheral and pulses APBxRSTR / AHBRSTR for the ones
that have a reset bit. After that, all per-class `Init()` calls are unnecessary
(and deprecated): callers just `Setup()` the peripheral they want to use.

Add a peripheral here when introducing it to the firmware; pick a free resource
(see CLAUDE.md "Allocate MCU resources wisely") rather than time-multiplexing.
*/
using PeripheralEnabler = Clocks::Enabler<
	// GPIO ports — every port we configure must be listed
	PortClock<Port::PA>,
	PortClock<Port::PB>,
	PortClock<Port::PC>,
	PortClock<Port::PD>,
	// AFIO — required for SWO trace pin remap and any EXTI line
	Afio,
	// DMA1 — covers all channels (CH1 for TIM2/JTCLK count, CH3 SPI, CH4 SPI, CH6 TIM1_CH3 / TIM4_CH1)
	Dma::Controller<Dma::Itf::k1>,
	// Timers used by the firmware
	Timer::TimerDescriptor<Timer::kTim1>,	// JTAG wave generator (advanced timer; CH1N=JTCK, CH2N=JTMS)
	Timer::TimerDescriptor<Timer::kTim2>,	// (reserved — see comment below)
	// TIM3 + TIM4 (JtclkWaveGen master/slave) are NOT listed here on purpose.
	// JtclkWaveGen::Acquire()/Release() power-cycles them per OnFlashTclk() call,
	// because on STM32F1 the PA7 alt-function mux only releases back to SPI1_MOSI
	// when TIM3's RCC clock is gated.
	// SPI1 carries JTCK/JTDI/JTDO in dtrig mode
	Spi::Hardware<Spi::Iface::k1>,
	// USART2 — provisional GDB serial (until USB CDC is added)
	UsartHardware<Usart::k2>
>;

#else

#error the selected OPT_JTAG_IMPLEMENTATION is not supported by platform

#endif


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
using JTMS = AnyOut<Port::PB, 14, Speed::kMedium, Level::kLow>;
/// Logic state for JTMS pin initialization
using JTMS_Init = AnyInPd<Port::PB, 14>;

/// Pin for JTDO input (output on MCU)
using JTDO = AnyInPu<Port::PA, 6>;
/// Logic state for JTDO pin initialization
using JTDO_Init = AnyInPu<Port::PA, 6>;

/// Pin for JTDI output (input on MCU)
using JTDI = AnyOut<Port::PA, 7, Speed::kMedium, Level::kHigh>;
/// Logic state for JTDI pin initialization
using JTDI_Init = AnyInPu<Port::PA, 7>;

#if OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_TIM_DMA_SLOW

/// Pin for JTCK output (GPIO bit-bang mode for OnResetTap and manual control)
using JTCK = AnyOut<Port::PB, 13, Speed::kMedium, Level::kHigh>;
/// Logic state for JTCK pin initialization
using JTCK_Init = AnyInPu<Port::PB, 13>;
/// Pin for JTCK output (PWM alternate function via TIM1_CH1N for 50% duty cycle frame generation)
/// Used by GeneratorSTLinkPWM and DtrigJtag for automatic clock generation.
/// Context switches with JTCK: GPIO mode for bit-bang, PWM/AF mode for frame generation.
using JTCK_PWM = TIM1_CH1N_PB13_OUT;

#elif OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_DTRIG

/// Pin for JTCK output (GPIO bit-bang mode for OnResetTap and manual control)
using JTCK = AnyOut<Port::PA, 5, Speed::kMedium, Level::kHigh>;
/// Logic state for JTCK pin initialization
using JTCK_Init = AnyInPu<Port::PA, 5>;
/// SPI1 SCK on PA5 carries JTCK in dtrig mode (PA5 and PB13 are shorted on the STLinkV2 PCB)
using JTCK_SPI = SPI1_SCK_PA5;

/// SPI1 MISO on PA6 carries JTDO in dtrig mode
using JTDO_SPI = SPI1_MISO_PA6;

/// SPI1 MISO on PA6 carries JTDO in dtrig mode
using JTDI_SPI = SPI1_MOSI_PA7;

/// SPI1 MOSI doubles as JTCLK (TDI = TCLK during Run-Test/Idle)
using JTCLK_SPI = JTDI_SPI;
/// PB14 as TIM1_CH2N alternate function (drives JTMS during JTAG frame generation)
using JTMS_PWM = TIM1_CH2N_PB14_OUT;

#else

#error the selected OPT_JTAG_IMPLEMENTATION is not supported by platform

#endif

/// JTDI during run/idle state produces JTCLK
using JTCLK = TIM3_CH2_PA7_OUT;

/// Pin for JRST output
using JRST = AnyOut<Port::PB, 0, Speed::kMedium>;
/// Logic state for JRST pin initialization
using JRST_Init = AnyInPu<Port::PB, 0>;

/// Pin for JTEST output (TRST)
using JTEST = AnyOut<Port::PB, 1, Speed::kMedium>;
/// Logic state for JTEST is forced low, since this hardware has a pull up and we have to
// release the TEST pins of the MSP430
using JTEST_Init = AnyOut<Port::PB, 1, Speed::kMedium>;

/// Pin for SBWDIO input
using SBWDIO_In = JTMS;

/// Pin for SBWDIO output
using SBWDIO = JTMS;

/// Pin for SBWCLK output
using SBWCLK = JTCK;

/// LED driver activation (LEDS connected in Series will not light, if not driven)
using LEDS_Init = AnyIn<Port::PA, 9, PuPd::kFloating>;
/// Pin for LED output
using LEDS = AnyOut<Port::PA, 9, Speed::kSlow, Level::kHigh>;

#if OPT_GEEGY_APM32F103CB
// TRACESWO does not work reliably on Geehy MCU, without configuring the GPIO as output
using TRACESWO = AnyOut<Port::PB, 3, Speed::kFastest, Level::kHigh>;
#else
// Debugger of STM32F103 simply takes full control of GPIO, so this pin is passive
using TRACESWO = TRACESWO_PB3;
#endif

#if OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_TIM_DMA_SLOW

/// Initial configuration for PORTA
using PORTA = AnyPortSetup <Port::PA
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
>;


/// Initial configuration for PORTB
using PORTB = AnyPortSetup <Port::PB
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
>;

#elif OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_DTRIG

/// Initial configuration for PORTA
using PORTA = AnyPortSetup <Port::PA
	, Unused<0>				///< Vref (pending)
	, Unused<1>				///< not used
	, USART2_TX_PA2			///< UART2 TX --> JRXD
	, USART2_RX_PA3			///< UART2 RX -- > JTXD
	, Unused<4>				///< not used
	, JTCK_Init				///< bit bang/TIM+DMA
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
>;

/// Initial configuration for PORTB
using PORTB = AnyPortSetup <Port::PB
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
	, Unused<13>			///< not used (PA5 drives JTCK in DTRIG mode)
	, JTMS_Init				///< bit bang/TIM+DMA
	, Unused<15>			///< not used
>;

#endif

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
	, Unused<13>			///< not used
	, Unused<14>			///< not used
	, Unused<15>			///< not used
>;

/// Initial configuration for PORTD
using PORTD = AnyPortSetup <Port::PD
	, Unchanged<0>			///< OSC_IN
	, Unchanged<1>			///< OSC_OUT
>;

/// All GPIO ports collected for one-shot initialization at startup
using AllGpioStartup = PortMerge<PORTA, PORTB, PORTC, PORTD>;


#if OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_TIM_DMA_SLOW

/// This configuration activates JTAG bus using bit-banging
using JtagOn1 = AnyPinGroup <Port::PA
	, JTDO					///< JTDO pin for bit bang access
	, JTDI					///< JTDI pin for bit bang access
>;
/// This configuration activates JTAG bus using bit-banging
using JtagOn2 = AnyPinGroup <Port::PB
	, JRST					///< JRST pin for bit bang access
	, JTEST					///< JTEST pin for bit bang access (old TRST)
	, JTCK					///< JTCK pin for bit bang access
	, JTMS					///< JTMS pin for bit bang access
>;

#elif OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_DTRIG

/// This configuration activates JTAG bus using bit-banging
using JtagOn1 = AnyPinGroup <Port::PA
	, JTCK					///< JTCK pin for bit bang access
	, JTDO					///< JTDO pin for bit bang access
	, JTDI					///< JTDI pin for bit bang access
>;
/// This configuration activates JTAG bus using bit-banging
using JtagOn2 = AnyPinGroup <Port::PB
	, JRST					///< JRST pin for bit bang access
	, JTEST					///< JTEST pin for bit bang access (old TRST)
	, JTMS_PWM				///< JTMS pin for PWM
>;

#endif

// Operate both ports for JtagOn
using JtagOn = PortMerge<JtagOn1, JtagOn2>;


#if OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_TIM_DMA_SLOW

/// This configuration deactivates JTAG bus
using JtagOff1 = AnyPinGroup <Port::PA
	, JTDO_Init				///< JTDO in Hi-Z
	, JTDI_Init				///< JTDI in Hi-Z
>;
/// This configuration deactivates JTAG bus
using JtagOff2 = AnyPinGroup <Port::PB
	, JRST_Init				///< JRST in Hi-Z
	, JTEST_Init			///< JTEST in Hi-Z
	, JTCK_Init				///< JTCK in Hi-Z
	, JTMS_Init				///< JTMS in Hi-Z
>;

#elif OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_DTRIG

/// This configuration deactivates JTAG bus
using JtagOff1 = AnyPinGroup <Port::PA
	, JTCK_Init				///< JTCK in Hi-Z
	, JTDO_Init				///< JTDO in Hi-Z
	, JTDI_Init				///< JTDI in Hi-Z
>;
/// This configuration deactivates JTAG bus
using JtagOff2 = AnyPinGroup <Port::PB
	, JRST_Init				///< JRST in Hi-Z
	, JTEST_Init			///< JTEST in Hi-Z
	, JTMS_Init				///< JTMS in Hi-Z
>;

#endif

// Operate both ports for JtagOff
using JtagOff = PortMerge<JtagOff1, JtagOff2>;

#if OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_DTRIG
/// Port A group: PA5/PA6/PA7 → SPI AF mode for dtrig operation
using JtagSpiOn = AnyPinGroup <Port::PA
	, JTCK_SPI				///< PA5 → SPI1_SCK (drives JTCK; wired to PB13 on PCB)
	, JTDO_SPI				///< PA6 → SPI1_MISO (= JTDO)
	, JTDI_SPI				///< PA7 → SPI1_MOSI (= JTDI / JTCLK)
>;
/// Port A group: PA5/PA6/PA7 → SPI AF mode for dtrig operation
using JtagGpioOn = AnyPinGroup <Port::PA
	, JTCK					///< PA5 → SPI1_SCK (drives JTCK; wired to PB13 on PCB)
	, JTDO					///< PA6 → SPI1_MISO (= JTDO)
	, JTDI					///< PA7 → SPI1_MOSI (= JTDI / JTCLK)
>;
#endif

/// Main JTAG signal generation (Must be an advanced timer - TIM1)
static constexpr Timer::Unit kWaveJtagTimer = Timer::kTim1;
/// TIM1 CH1 drives JTCK via CH1N complementary output in all TIM-based modes
static constexpr Timer::Channel kWaveJtagTck = Timer::Channel::k1;

#if OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_TIM_DMA_SLOW
/// Timer channels for GeneratorSTLinkPWM (3 DMA + 1 PWM):
/// CH1N PWM for JTCK (50% duty), CH2 for JTMS, CH3 for JTDI, CH4 for JTDO read
static constexpr Timer::Channel kWaveJtagTms = Timer::Channel::k2;		// JTMS control (DMA1_CH3)
static constexpr Timer::Channel kWaveJtagWrite = Timer::Channel::k3;	// JTDI data (DMA1_CH6)
static constexpr Timer::Channel kWaveJtagReadCh = Timer::Channel::k4;	// JTDO read (DMA1_CH4)
#elif OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_DTRIG
/// SPI1 carries JTCK (SCK/PA5), JTDI (MOSI/PA7), JTDO (MISO/PA6); TIM1 drives TMS only
static constexpr Spi::Iface kSpiForJtag = Spi::Iface::k1;
/// TIM1_CH2 toggle output; complementary CH2N drives PB14 (JTMS) — no per-bit DMA
static constexpr Timer::Channel kWaveJtagTms    = Timer::Channel::k2;		// toggle → CH2N → PB14
/// TIM1_CH3 compare-only: CC3 DMA (DMA1_CH6) reloads CCR2 at end of entry pulse
static constexpr Timer::Channel kWaveJtagTmsRld1 = Timer::Channel::k3;		// entry→shift CCR2 reload
/// JTMS sits on TIM1_CH2N (PB14) — the complementary output, which inverts OCREF
/// naturally. Tells DtrigJtag to enable CHN with default polarity; CHN_pin = NOT_OCREF.
static constexpr bool kWaveJtagTmsCmpComplementary = true;
#endif

#if OPT_JTAG_TCLK_IMPLEMENTATION == OPT_JTCLK_IMPL_TIM_DMA
/// Frequency for generation (MSP430 flash max freq is 476kHz; two cycles per pulse)
static constexpr uint32_t kTimDmaWavFreq = 2 * 450000; // slightly lower because of inherent jitter
/// Timer for JTCLK wave generation (TIM4_UP → DMA1_CH7; no conflict with DtrigJtag SPI DMA)
static constexpr Timer::Unit kTimDmaWavBeat = Timer::kTim2;
/// Timer for JTCLK wave count
static constexpr Timer::Unit kTimForJtclkCnt = Timer::kTim3;
#elif OPT_JTAG_TCLK_IMPLEMENTATION == OPT_JTCLK_IMPL_TIM_DMA_2
/// Frequency for generation (MSP430 flash max freq is 476kHz; two cycles per pulse)
static constexpr uint32_t kTimDmaWavFreq = 2 * 450000; // slightly lower because of inherent jitter
/// Timer for JTCLK wave generation (TIM3 → TIM4)
static constexpr Timer::Unit kTimDmaWavBeat = Timer::kTim3;
/// Timer channel for JTCLK wave count (TIM3_CCR2 → PWM → PA7/TDI)
static constexpr Timer::Channel kTimChForJtclk = Timer::Channel::k2;
/// Timer for JTCLK cycle count (TIM4_UP → DMA1_CH7 → stop TIM3)
static constexpr Timer::Unit kTimForJtclkCnt = Timer::kTim4;
#elif OPT_JTAG_TCLK_IMPLEMENTATION == OPT_JTCLK_IMPL_SPI
/// JTCLK is produced by clocking a precomputed bit pattern out of MOSI at a
/// reduced SPI baud. Avoids fighting the F1 alt-function mux on PA7.
//#define WAVESET_1_4th	1
//#define WAVESET_2_9th	1
#define WAVESET_1_5th	1
//#define WAVESET_2_11	1
//#define WAVESET_1_6	1
//#define WAVESET_1_7	1
//#define WAVESET_1_8	1
/// Target SPI baud during the TCLK burst (≈2 SPI clocks per JTCLK edge)
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
ALWAYS_INLINE void UartBusOn() { }
/// Disables MSP430 UART interface buffers
ALWAYS_INLINE void UartBusOff() { }


// Sets hardware buffers in tri-state or driving
ALWAYS_INLINE void SetBusState(const BusState)
{
	// This hardware does not have output buffers to be managed
}

