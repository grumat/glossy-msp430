/*!
\file target.bluepill.g431kb/platform.h
\brief Definitions specific for the BluePill-G431 board (STM32G431, LQFP48)

================================================================================
⚠️  UNTESTED DTRIG PORT — never bench-validated
================================================================================
This platform.h was ported from OPT_JTAG_IMPL_SPI_DMA to OPT_JTAG_IMPL_DTRIG
when the legacy SPI/TIM_DMA backends were retired (see
.claude/docs/drivers/SPI_VARIANT_REMOVED.md). The port mirrors bluepill's
"regular CH" DTRIG path because PA9 (JTMS) sits on TIM1_CH2 — a main channel,
not a CHN. No hardware bring-up has happened on this target since the port.

If you're picking this board up, expect to:
  - Verify the PeripheralEnabler list against actual G4 peripheral RCC bits.
  - Re-trim s_cnt_offset[1..5] in JtagDev.dtrig.cpp for G4 timing (F1 values
    were calibrated on a 72 MHz STM32F103, not 160 MHz STM32G431).
  - Confirm DMAMUX request mapping matches DtrigJtag's expectations.
  - Validate that TIM1_CH3 (compare-only DMA trigger) doesn't clash with
    PA10 = SBWO — PA10 stays GPIO-driven, CH3 has Output::kDisabled.

Pin map (from Hardware/L432KC/CubeMX/G431K6.txt):
  PA0  JTEST          PA5  JTCK / SPI1_SCK   PA10 SBWO
  PA1  JRST           PA6  JTDO / SPI1_MISO  PA15 LEDS
  PA2  GDB_TX         PA7  JTDI / SPI1_MOSI  PB4/5 ENA1/2
  PA4  JVCC (DAC1)    PA9  JTMS / TIM1_CH2
================================================================================
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

/// JTAG transport selection. DTRIG is the only supported variant.
#define OPT_JTAG_IMPLEMENTATION			OPT_JTAG_IMPL_DTRIG

/// SBW (Spy-Bi-Wire) transport selection. Independent of OPT_JTAG_IMPLEMENTATION
/// — both can be compiled in, but only one can be active at runtime (they share
/// TIM1 + GPIO + DMA channels). TapMcu::Open() picks one and calls exactly one
/// driver's Init(), which then claims every shared resource it needs. Set to
/// OPT_SBW_IMPL_OFF to compile SBW out entirely. See
/// .claude/docs/drivers/TIM_SBW_DRIVER.md.
///
/// **PREREQUISITE BEFORE ENABLING:** verify the ENA1N/ENA2N/ENA3N buffer-enable
/// grouping does NOT gang TEST with TCK/RST — see "Board hardware requirements
/// for SBW" in the design doc. Bluepill's PB12 gangs all three, which breaks
/// SBW; check whether this board has the same flaw before flipping the switch.
#define OPT_SBW_IMPLEMENTATION			OPT_SBW_IMPL_OFF

/// JTCLK generation strategy.  SPI variant is the natural pair with DTRIG.
#define OPT_JTAG_TCLK_IMPLEMENTATION	OPT_JTCLK_IMPL_SPI
/// Implementation for "GDB serial port" (USART used provisory until USB VCP is added to firmware)
#define OPT_GDB_IMPLEMENTATION			OPT_GDB_IMPL_USART2


/// HSE 24 MHz → PLL → 160 MHz SYSCLK
using HSE = Clocks::AnyHse<24000000UL>;
using PLL = Clocks::AnyPll<HSE, 160000000UL, Clocks::AutoRange1>;
using SysClk = Clocks::AnySycClk<
	PLL
	, Power::Mode::kRange1
	, Clocks::AhbPrscl::k1
	, Clocks::ApbPrscl::k2
	, Clocks::ApbPrscl::k1
	>;


/* SPI interface grades (untested on G4 — APB2 = 80 MHz at 160 MHz SYSCLK; SPI baud = APB2 / 2^N) */
static constexpr uint32_t JTCK_Speed_5 = 10000000UL;
static constexpr uint32_t JTCK_Speed_4 = 5000000UL;
static constexpr uint32_t JTCK_Speed_3 = 2500000UL;
static constexpr uint32_t JTCK_Speed_2 = 1250000UL;
static constexpr uint32_t JTCK_Speed_1 = 625000UL;


/// DTRIG drives TIM1 from its internal clock, so PA8 is freed.
using TmsShapeGpioIn = Unchanged<8>;

/// Dedicated pin for write JTMS (PA9 = TIM1_CH2)
using JTMS = AnyOut<Port::PA, 9>;
using JTMS_Init = AnyInPd<Port::PA, 9>;
/// PA9 → TIM1_CH2 alt-function during a JTAG frame (regular CH, not CHN)
using JTMS_PWM = TIM1_CH2_PA9_OUT;

/// Pin for JTCK output
using JTCK = AnyOut<Port::PA, 5, Speed::kFast, Level::kHigh>;
using JTCK_Init = AnyInPu<Port::PA, 5>;
using JTCK_SPI = SPI1_SCK_PA5;

/// Pin for JTDO input (output on MCU)
using JTDO = AnyInPu<Port::PA, 6>;
using JTDO_Init = AnyInPu<Port::PA, 6>;
using JTDO_SPI = SPI1_MISO_PA6;

/// Pin for JTDI output (input on MCU)
using JTDI = AnyOut<Port::PA, 7, Speed::kFast, Level::kHigh>;
using JTDI_Init = AnyInPu<Port::PA, 7>;

/// JTDI during run/idle state produces JTCLK
using JTCLK = JTDI;
using JTDI_SPI = SPI1_MOSI_PA7;
using JTCLK_SPI = JTDI_SPI;

/// Pin for JRST output
using JRST = AnyOut<Port::PA, 1>;
using JRST_Init = AnyInPu<Port::PA, 1>;

/// Pin for JTEST output
using JTEST = AnyOut<Port::PA, 0>;
using JTEST_Init = AnyInPd<Port::PA, 0>;

/// SBW aliases
using SBWDIO_In = JTDO;
using SBWDIO = JTDI;
using SBWCLK = JTCK;

/// Pin for SBWO Enable control (PA10, GPIO output). PA10 is the hardware mux
/// that selects which physical pin drives the SBWDIO trace: high = output
/// path, low = input path.
using SBWO = AnyOut<Port::PA, 10, Speed::kSlow, Level::kHigh>;

/// SBW direction-flip policy for Nucleo-G431 — buffered/optimized variant.
/// The direction-script DMA writes one BSRR word per phase boundary to
/// GPIOA->BSRR; only the PA10 bit toggles. See "DirPolicy contract" in
/// .claude/docs/drivers/TIM_SBW_DRIVER.md.
struct DirPolicy_PA10_BsrrMux
{
	static constexpr unsigned kWordsPerFlip = 1;
	static void Init() {}			///< no-op — both arrays are constexpr
	static const uint32_t* DriveOutput()
	{
		static constexpr uint32_t v[1] = { 1u << 10 };			///< BSRR set PA10 → mux→OUT
		return v;
	}
	static const uint32_t* DriveInput()
	{
		static constexpr uint32_t v[1] = { 1u << (10 + 16) };	///< BSRR reset PA10 → mux→IN
		return v;
	}
	static volatile uint32_t* DirRegister() { return &GPIOA->BSRR; }
};

/// Pin for ENA1N / ENA2N / ENA3N control
using ENA1N = AnyOut<Port::PB, 4, Speed::kSlow, Level::kHigh>;
using ENA2N = AnyOut<Port::PB, 5, Speed::kSlow, Level::kHigh>;
using ENA3N = AnyOut<Port::PA, 2, Speed::kSlow, Level::kHigh>;

/// LED driver
using LEDS_Init = AnyIn<Port::PA, 15, PuPd::kFloating>;
using LEDS = AnyOut<Port::PA, 15, Speed::kSlow, Level::kHigh>;

/// Target voltage (DAC1 OUT1)
using DAC_VT_0V = AnyOut<Port::PA, 4, Speed::kSlow, Level::kLow>;
using DAC_VT_3V3 = AnyOut<Port::PA, 4, Speed::kSlow, Level::kHigh>;
using DAC_VT = DAC1_OUT1_PA4;

/// Target-voltage PWM regulator on PB9 / TIM4_CH4. Deliberately NOT PB8/TIM4_CH3:
/// PB8 = BOOT0 on the G431, and the regulator input pull-up would force the chip
/// into the system bootloader (DFU) at reset. Pin-consistent with target.bluepill.
/// TIM4 is free here (JTCLK is OPT_JTCLK_IMPL_SPI; JTAG/SBW use TIM1).
using PWM_VT_0V = AnyOut<Port::PB, 9, Speed::kSlow, Level::kLow>;
using PWM_VT_3V3 = AnyOut<Port::PB, 9, Speed::kSlow, Level::kHigh>;
using PWM_VT = TIM4_CH4_PB9_OUT;

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
	, TmsShapeGpioIn		///< PA8 unused (TIM1 internal clock in DTRIG)
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
	, Unused<8>				///< PB8 = BOOT0, keep floating
	, PWM_VT_3V3			///< Target power on (TIM4_CH4)
	, Unused<10>, Unused<11>
	, Unused<12>, Unused<13>, Unused<14>, Unused<15>
>;


using PORTC = Bmt::DummyInit;
using PORTD = Bmt::DummyInit;

using AllGpioStartup = PortMerge<PORTA, PORTB, PORTC, PORTD>;

/// JTAG bus active for DTRIG
using JtagOn = AnyPinGroup <Port::PA
	, JTEST
	, JRST
	, JTCK
	, JTDO
	, JTDI
	, Unchanged<8>			///< PA8 unused (TIM1 internal clock)
	, JTMS_PWM				///< PA9 → TIM1_CH2 alt-function for JTMS
>;

/// Tri-state config when JTAG is released
using JtagOff = AnyPinGroup <Port::PA
	, JTEST_Init
	, JRST_Init
	, JTCK_Init
	, JTDO_Init
	, JTDI_Init
	, TmsShapeGpioIn
	, JTMS_Init
>;

/// PA5/PA6/PA7 → SPI AF mode for DTRIG operation
using JtagSpiOn = AnyPinGroup <Port::PA
	, JTCK_SPI
	, JTDO_SPI
	, JTDI_SPI
>;
/// PA5/PA6/PA7 → GPIO mode for the brief bit-bang windows
using JtagGpioOn = AnyPinGroup <Port::PA
	, JTCK
	, JTDO
	, JTDI
>;

/// SPI1 carries JTCK/JTDO/JTDI; TIM1 drives TMS only
static constexpr Spi::Iface kSpiForJtag = Spi::Iface::k1;
/// Main JTAG signal generation (advanced timer — TIM1)
static constexpr Timer::Unit kWaveJtagTimer = Timer::kTim1;
/// JTMS sits on TIM1_CH2 (PA9) — regular output, not CHN.
static constexpr Timer::Channel kWaveJtagTms     = Timer::Channel::k2;
/// CH3 compare-only: CC3 DMA reloads CCR2 at end of entry pulse.
static constexpr Timer::Channel kWaveJtagTmsRld1 = Timer::Channel::k3;
/// JTMS is on TIM1_CH2 (regular CH) — same path as bluepill (kCmpComplementary=false).
static constexpr bool kWaveJtagTmsCmpComplementary = false;


#if OPT_JTAG_TCLK_IMPLEMENTATION == OPT_JTCLK_IMPL_SPI
//#define WAVESET_1_4th	1
#define WAVESET_1_5th	1
//#define WAVESET_2_11	1
//#define WAVESET_1_6	1
//#define WAVESET_1_7	1
//#define WAVESET_1_8	1
static constexpr uint32_t kJtclkSpiClock = 5000000UL;
#endif


using TickTimer = Timer::SysTickCounter<SysClk>;


ALWAYS_INLINE void SetLedState(const LedState st)
{
	switch (st)
	{
	case LedState::on:		LEDS::SetupPinMode();	break;
	case LedState::red:		LEDS::SetHigh();		break;
	case LedState::green:	LEDS::SetLow();			break;
	default:				LEDS_Init::SetupPinMode(); break;
	}
}


/// Enables MSP430 UART interface buffers
ALWAYS_INLINE void UartBusOn() { ENA3N::SetLow(); }
/// Disables MSP430 UART interface buffers
ALWAYS_INLINE void UartBusOff() { ENA3N::SetHigh(); }


using DEBUG_BUS_CTRL = Gpio::AnyCounter <
	ENA1N,
	ENA2N
>;

ALWAYS_INLINE void SetBusState(const BusState st)
{
	DEBUG_BUS_CTRL::WriteComplement((uint32_t)st);
}
