/*!
\file target.bluepill.g431kb/platform.h
\brief Definitions specific for the BluePill-G431 board (STM32G431, LQFP48)

This target runs on the **same jiga board as `target.bluepill`** but populated
with a G431-in-BluePill-package daughter board instead of the STM32F103. The
authoritative hardware description is **Hardware/BluePill-G431/README.md** — the
older Hardware/L432KC CubeMX reference is OBSOLETE and was the source of the wrong
pin map this file used to carry.

Pin map (from Hardware/BluePill-G431/README.md):
  PA0  TEST (bit-bang)      PA5  TCK / SPI1_SCK    PA10 SBW_RD (SBW dir)
  PA1  RST  (bit-bang)      PA6  TDO / SPI1_MISO   PA11 USB DM
  PA2  GDB_TX / USART2_TX   PA7  TDI / SPI1_MOSI   PA12 USB DP
  PA3  GDB_RX / USART2_RX   PA8  TCK (bridged)     PA13 SWDIO
  PA4  GEN_VT / DAC1_OUT1   PA9  TMS / TIM1_CH2    PA14 SWCLK / PA15 LEDS
  PB0  V2REF (ADC1_IN15)    PB6  h.TXD/USART1_TX   PB10 DIS_RST
  PB3  TRACESWO             PB7  h.RXD/USART1_RX   PB11 DIS_TCK
  PB4/PB5 breakout (unused) PB8  BOOT0 (float!)    PB12 DIS_JTAG
                            PB9  GEN_VT/TIM4_CH4   PB13 DIS_COM

⚠️  DTRIG port is NOT bench-validated on this target. Expect to re-trim
    s_cnt_offset[1..5] in JtagDev.dtrig.cpp for 160 MHz G431 timing (F1 values
    were calibrated at 72 MHz) and confirm the DMAMUX request mapping.
*/
#pragma once

using namespace Bmt;
using namespace Bmt::Gpio;

#include "platform-defs.h"
#include "drivers/BusStates.h"
#include "drivers/LedStates.h"
#include <adc.h>				// target-voltage sense (ADC1_IN15 on PB0)

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
/// This jiga has independent buffer-enable lines (DIS_RST/DIS_TCK/DIS_JTAG, see
/// the bus-state table below) — unlike target.bluepill's ganged ENA1N — so it
/// CAN selectively tri-state TEST while driving SBWCLK. SBW is nonetheless kept
/// OFF until the SbwDev bit-bang entry (currently STLinkV2-specific) is ported to
/// this board's SBW_RD direction control.
#define OPT_SBW_IMPLEMENTATION			OPT_SBW_IMPL_OFF

/// JTCLK generation strategy.  SPI variant is the natural pair with DTRIG.
#define OPT_JTAG_TCLK_IMPLEMENTATION	OPT_JTCLK_IMPL_SPI
/// Implementation for "GDB serial port" (USART2; provisory until USB VCP is added)
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


/// DTRIG drives TIM1 from its internal clock, so PA8 (bridged to the TCK net) is freed.
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

/// Pin for JRST output (PA1)
using JRST = AnyOut<Port::PA, 1>;
using JRST_Init = AnyInPu<Port::PA, 1>;

/// Pin for JTEST output (PA0)
using JTEST = AnyOut<Port::PA, 0>;
using JTEST_Init = AnyInPd<Port::PA, 0>;

/// SBW aliases (SBW reuses the JTAG host pins: SBWCLK on TCK, SBWDIO on TDO/TDI)
using SBWDIO_In = JTDO;
using SBWDIO = JTDI;
using SBWCLK = JTCK;

/// SBW direction control (PA10 = SBW_RD, GPIO bit-bang). Selects which physical
/// pin drives the bidirectional SBWDIO trace. Pin is also TIM1_CH3-capable —
/// reserved for a hardware-timed SBWO direction toggle in the planned SPI-stream
/// SBW driver. Dormant while OPT_SBW_IMPLEMENTATION == OPT_SBW_IMPL_OFF.
using SBW_RD = AnyOut<Port::PA, 10, Speed::kSlow, Level::kHigh>;

/// SBW direction-flip policy for BluePill-G431 — buffered/optimized variant.
/// The direction-script DMA writes one BSRR word per phase boundary to
/// GPIOA->BSRR; only the PA10 (SBW_RD) bit toggles. See "DirPolicy contract" in
/// .claude/docs/drivers/TIM_SBW_DRIVER.md.
struct DirPolicy_PA10_BsrrMux
{
	static constexpr unsigned kWordsPerFlip = 1;
	static void Init() {}			///< no-op — both arrays are constexpr
	static const uint32_t* DriveOutput()
	{
		static constexpr uint32_t v[1] = { 1u << 10 };			///< BSRR set PA10 → dir→OUT
		return v;
	}
	static const uint32_t* DriveInput()
	{
		static constexpr uint32_t v[1] = { 1u << (10 + 16) };	///< BSRR reset PA10 → dir→IN
		return v;
	}
	static volatile uint32_t* DirRegister() { return &GPIOA->BSRR; }
};

/// === Bus buffer-enable lines (negative logic: DISxxx HIGH ⇒ buffer tri-stated).
/// Grouped per the "Bus buffer control" table in Hardware/BluePill-G431/README.md.
///   DIS_RST  (PB10) → RST + TEST buffers
///   DIS_TCK  (PB11) → TCK buffer
///   DIS_JTAG (PB12) → TDI + TMS buffers
///   DIS_COM  (PB13) → TXD (target UART) buffer
/// All init HIGH (Hi-Z) for a safe cold-boot Standby.
using DIS_RST  = AnyOut<Port::PB, 10, Speed::kSlow, Level::kHigh>;
using DIS_TCK  = AnyOut<Port::PB, 11, Speed::kSlow, Level::kHigh>;
using DIS_JTAG = AnyOut<Port::PB, 12, Speed::kSlow, Level::kHigh>;
using DIS_COM  = AnyOut<Port::PB, 13, Speed::kSlow, Level::kHigh>;

/// LED driver (PA15). Dual LED: tri-state = off, high = Red, low = Green.
using LEDS_Init = AnyIn<Port::PA, 15, PuPd::kFloating>;
using LEDS = AnyOut<Port::PA, 15, Speed::kSlow, Level::kHigh>;

/// Target voltage (GEN_VT node). DAC1_OUT1 on PA4 — the generated gpio-types name
/// DAC output pins as analog (AnyAnalog), so the analog pin config is Analog_PA4
/// (f1xx does the same; there is no DAC1_OUT1_PA4 alias). DAC_VT and PWM_VT share
/// the same GEN_VT net — drive ONLY ONE at a time (README "Two target-voltage paths").
using DAC_VT_0V = AnyOut<Port::PA, 4, Speed::kSlow, Level::kLow>;
using DAC_VT_3V3 = AnyOut<Port::PA, 4, Speed::kSlow, Level::kHigh>;
using DAC_VT = Analog_PA4;

/// Target-voltage PWM regulator on PB9 / TIM4_CH4. Deliberately NOT PB8/TIM4_CH3:
/// PB8 = BOOT0 on the G431, and the regulator-input pull-up would force the chip
/// into the system bootloader (DFU) at reset. Pin-consistent with target.bluepill.
/// TIM4 is free here (JTCLK is OPT_JTCLK_IMPL_SPI; JTAG/SBW use TIM1).
using PWM_VT_0V = AnyOut<Port::PB, 9, Speed::kSlow, Level::kLow>;
using PWM_VT_3V3 = AnyOut<Port::PB, 9, Speed::kSlow, Level::kHigh>;
using PWM_VT = TIM4_CH4_PB9_OUT;

// ── Target voltage (#46 PASS 2) ──────────────────────────────────────────────
// SENSE: PB0 = V2REF carries the target VCC through a /2 divider into ADC1_IN15.
// The G4 ADC kernel clock comes from the default AnyConfig (CkMode::kHclkDiv4),
// so no clock-tree ADC mux setup is needed. DRIVE (PWM_VT/DAC_VT) lands in the
// TIM4 PWM regulator bring-up pass — OPT_TARGET_HAS_VDRIVE stays 0 for now.
#define OPT_TARGET_HAS_VSENSE	1
#define OPT_TARGET_HAS_VDRIVE	0
static constexpr uint32_t kVtgMax_mV    = 3300;		///< feasible drive ceiling (PWM_VT/DAC, next pass)
static constexpr uint32_t kVtgAdcRef_mV = 3300;		///< ADC full-scale reference (VDDA)
static constexpr uint32_t kVtgSenseMul  = 2;		///< PB0 = VCC/2 → multiply reading by 2
using VSenseAdc = Adc::AnySetup<Adc::AnyConfig,
	Adc::AnySequence<Adc::Chan<Adc::Unit::k1, 15>>>;

/// Initial configuration for PORTA
using PORTA = AnyPortSetup <Port::PA
	, JTEST_Init			///< PA0  TEST (bit-bang)
	, JRST_Init				///< PA1  RST  (bit-bang)
	, USART2_TX_PA2			///< PA2  GDB serial TX
	, USART2_RX_PA3			///< PA3  GDB serial RX
	, DAC_VT_3V3			///< PA4  GEN_VT (target power on)
	, JTCK_Init				///< PA5  TCK / SPI1_SCK
	, JTDO_Init				///< PA6  TDO / SPI1_MISO
	, JTDI_Init				///< PA7  TDI / SPI1_MOSI
	, TmsShapeGpioIn		///< PA8  TCK bridged (TIM1 internal clock in DTRIG)
	, JTMS_Init				///< PA9  TMS / TIM1_CH2
	, SBW_RD				///< PA10 SBW direction (GPIO)
	, Unused<11>			///< PA11 USB DM
	, Unused<12>			///< PA12 USB DP
	, Unchanged<13>			///< PA13 SWDIO (STM32 debug — leave alone)
	, Unchanged<14>			///< PA14 SWCLK (STM32 debug — leave alone)
	, LEDS_Init				///< PA15 LEDS (Red/Green, init Hi-Z = off)
>;

/// Initial configuration for PORTB
using PORTB = AnyPortSetup <Port::PB
	, Analog_PB0			///< PB0  V2REF = target VCC/2 sense (ADC1_IN15)
	, Unused<1>				///< PB1  breakout (unused)
	, Unused<2>				///< PB2  BOOT1
	, TRACESWO_PB3			///< PB3  ARM SWO trace
	, Unused<4>				///< PB4  breakout (unused)
	, Unused<5>				///< PB5  breakout (unused)
	, USART1_TX_PB6			///< PB6  h.TXD → target UART (USART1, VCP builds)
	, USART1_RX_PB7			///< PB7  h.RXD ← target UART (USART1, VCP builds)
	, Unused<8>				///< PB8  BOOT0 — keep floating (no pull-up → DFU)
	, PWM_VT_3V3			///< PB9  GEN_VT PWM (TIM4_CH4)
	, DIS_RST				///< PB10 DIS_RST  (buffer enable, init Hi-Z)
	, DIS_TCK				///< PB11 DIS_TCK  (buffer enable, init Hi-Z)
	, DIS_JTAG				///< PB12 DIS_JTAG (buffer enable, init Hi-Z)
	, DIS_COM				///< PB13 DIS_COM  (UART buffer enable, init Hi-Z)
	, Unused<14>			///< PB14 breakout (unused)
	, Unused<15>			///< PB15 breakout (unused)
>;


using PORTC = Bmt::DummyInit;
using PORTD = Bmt::DummyInit;

using AllGpioStartup = PortMerge<PORTA, PORTB, PORTC, PORTD>;

/// JRST driven HIGH while activating JTAG. UIF holds RST high before the entry
/// sequence (no RST-low pre-pulse), so JtagOn must bring RST up — the bare `JRST`
/// (AnyOut default Level::kLow) would drive it low. See
/// I2031_ACQUISITION_GOLDEN_REFERENCE.md.
using JRST_On = AnyOut<Port::PA, 1, Speed::kFast, Level::kHigh>;
/// JTAG bus active for DTRIG
using JtagOn = AnyPinGroup <Port::PA
	, JTEST
	, JRST_On
	, JTCK
	, JTDO
	, JTDI
	, Unchanged<8>			///< PA8 unused (TIM1 internal clock)
	, JTMS_PWM				///< PA9 → TIM1_CH2 alt-function for JTMS
>;

/// === Init state (DriverOff): release the MCU pins to Hi-Z. Applied only by
/// OnReleaseDriver() (from OnClose()), which also drops the buffer enables
/// (BusState::off). This is the old "JtagOff" (Hi-Z) set.
using DriverOff = AnyPinGroup <Port::PA
	, JTEST_Init
	, JRST_Init
	, JTCK_Init
	, JTDO_Init
	, JTDI_Init
	, TmsShapeGpioIn
	, JTMS_Init
>;

/// === Close state (JtagOff): DRIVE the bus idle level instead of Hi-Z while the
/// buffers stay enabled, so the resting level is held by the MCU/buffer outputs,
/// not the target's pull-ups/-downs, between acquisition attempts. Applied by
/// OnReleaseJtag(). Idle mapping: RST=high, TEST=low, TMS=low, TCK=high, TDI=high;
/// TDO=input. (This jiga DOES have output buffers — see SetBusState.)
using JRST_Close  = AnyOut<Port::PA, 1, Speed::kFast, Level::kHigh>;		///< RST driven high
using JTEST_Close = AnyOut<Port::PA, 0, Speed::kFast, Level::kLow>;		///< TEST driven low
using JTMS_Close  = AnyOut<Port::PA, 9, Speed::kFast, Level::kLow>;		///< TMS driven low
using JtagOff = AnyPinGroup <Port::PA
	, JTEST_Close			///< JTEST driven low (TEST=L)
	, JRST_Close			///< JRST driven high (RST=H)
	, JTCK					///< JTCK driven high (TCK=H)
	, JTDO_Init				///< JTDO input (target drives TDO)
	, JTDI					///< JTDI driven high (TDI=H)
	, TmsShapeGpioIn
	, JTMS_Close			///< JTMS driven low (TMS=L)
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


/// Enables MSP430 UART interface buffers (DIS_COM = PB13, negative logic)
ALWAYS_INLINE void UartBusOn() { DIS_COM::SetLow(); }
/// Disables MSP430 UART interface buffers
ALWAYS_INLINE void UartBusOff() { DIS_COM::SetHigh(); }


/// Drives the JTAG/SBW buffer-enable lines from the firmware's BusState. Negative
/// logic (DISxxx HIGH ⇒ buffer tri-stated). Mirrors the states table in
/// Hardware/BluePill-G431/README.md. The DTRIG JTAG driver uses only `off` (Hi-Z)
/// and `sbw` (enable the bus for the active protocol — JTAG on this board); `jtag`
/// maps to the same full-bus enable. DIS_COM (UART) and SBW_RD (SBW direction) are
/// managed separately (UartBusOn/Off and the SBW driver).
/// NOTE: the README's finer phases (Acquiring JTAG/SBW, distinct SBW-active column)
/// would need a richer BusState enum than the current off/sbw/jtag — a driver-level
/// change left for SBW bring-up on this board.
ALWAYS_INLINE void SetBusState(const BusState st)
{
	switch (st)
	{
	case BusState::off:		// Standby — whole bus Hi-Z
		DIS_RST::SetHigh();
		DIS_TCK::SetHigh();
		DIS_JTAG::SetHigh();
		break;
	case BusState::sbw:		// active bus (JTAG on this board) — buffers driving
	case BusState::jtag:
		DIS_RST::SetLow();
		DIS_TCK::SetLow();
		DIS_JTAG::SetLow();
		break;
	}
}


/*!
\brief Single source of truth for which MCU peripherals this firmware owns.

`SystemInit()` calls `PeripheralEnabler::Init()` once at boot — this enables
clocks for every listed peripheral and pulses the matching reset bits. Per-class
`Init()` calls are unnecessary (and deprecated).

G4 differences vs the F1 bluepill list: there is no AFIO (per-pin AFR replaces it),
and every DMA request is routed through DMAMUX1 — so Dma::Dmamux MUST be listed.
*/
using PeripheralEnabler = Clocks::Enabler<
	// GPIO ports
	PortClock<Port::PA>,
	PortClock<Port::PB>,
	// DMA1 + DMAMUX (G4 routes every DMA request through DMAMUX1)
	Dma::Controller<Dma::Itf::k1>,
	Dma::Dmamux,
	// Timers used by the firmware
	Timer::TimerDescriptor<Timer::kTim1>,	// DTRIG TMS (CH2 → PA9) + CC3 DMA trigger
	Timer::TimerDescriptor<Timer::kTim4>,	// target-voltage PWM (CH4 → PB9)
	// SPI1 carries JTCK/JTDO/JTDI (and the JTCLK burst) in DTRIG mode
	Spi::Hardware<Spi::Iface::k1>,
	// USART2 = GDB host serial; USART1 = target UART passthrough (VCP builds)
	UsartHardware<Usart::k2>,
	UsartHardware<Usart::k1>
>;
