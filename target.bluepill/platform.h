/*!
\file target.bluepill/platform.h
\brief Definitions specific for the F103 BluePill on the BluePill-G431 jiga board

This target runs the **STM32F103** daughterboard on the **same jiga board as
`target.bluepill.g431kb`** (which populates a G431-in-BluePill-package instead).
The authoritative hardware description is **Hardware/BluePill-G431/README.md**; the
older Hardware/BluePill-BMP board (LED on PC13, TEST on PA4, ganged ENA1N/2N/3N)
is OBSOLETE — this file was migrated onto the new jiga pinout.

Pin map (from Hardware/BluePill-G431/README.md; F1 deltas noted):
  PA0  TEST (bit-bang)      PA5  TCK / SPI1_SCK    PA10 SBW_RD (SBW dir)
  PA1  RST  (bit-bang)      PA6  TDO / SPI1_MISO   PA11 USB DM
  PA2  GDB_TX / USART2_TX   PA7  TDI / SPI1_MOSI   PA12 USB DP
  PA3  GDB_RX / USART2_RX   PA8  TCK (bridged)     PA13 SWDIO
  PA4  GEN_VT (Hi-Z; F103   PA9  TMS / TIM1_CH2    PA14 SWCLK / PA15 LEDS
       has no DAC — PB9 PWM drives the GEN_VT net)
  PB0  V2REF (ADC1_IN8)     PB6  h.TXD/USART1_TX   PB10 DIS_RST
  PB3  TRACESWO             PB7  h.RXD/USART1_RX   PB11 DIS_TCK
  PB4/PB5 breakout (unused) PB8  BOOT0 (float!)    PB12 DIS_JTAG
                            PB9  GEN_VT/TIM4_CH4   PB13 DIS_COM

F1 specifics vs the G431 target:
  - 8 MHz HSE → 72 MHz PLL; ADC prescaler /8; USB clock enabled.
  - V2REF sense is **ADC1_IN8** (PB0), not IN15 (the F103 ADC channel map differs).
  - AFIO is required (USART1 remap to PB6/PB7, TRACESWO); G4 has no AFIO.
  - DMA1 has fixed channel↔request mapping (no DMAMUX). JTMS is on TIM1_CH2 (PA9);
    the TMS-CCR-reload DMA trigger is TIM1_CH4 → DMA1_CH4, picked because SPI1_RX =
    DMA1_CH2 and SPI1_TX = DMA1_CH3 are taken (see kWaveJtagTmsRld1).
*/
#pragma once

using namespace Bmt;
using namespace Bmt::Gpio;

#include "platform-defs.h"
#include "drivers/BusStates.h"
#include "drivers/LedStates.h"

// Startup mode — what the firmware does at power-up. Default OPT_STARTUP_GDB;
// see the OPT_STARTUP_* enum in stdproj.h for the bench modes (LA_WAVEFORM,
// TIM_DMA_TIMING, DETECT_JTAG/SBW, SBW_TDO_SETTLE).
//#define OPT_STARTUP					OPT_STARTUP_LA_WAVEFORM

/// JTAG transport selection. DTRIG is the only supported variant — see
/// .claude/docs/drivers/SPI_VARIANT_REMOVED.md and TIM_VARIANT_REMOVED.md.
#define OPT_JTAG_IMPLEMENTATION		OPT_JTAG_IMPL_DTRIG

/// SBW (Spy-Bi-Wire) transport selection. Independent of OPT_JTAG_IMPLEMENTATION
/// — both can be compiled in, but only one can be active at runtime (they share
/// TIM1 + GPIO + DMA channels). TapMcu::Open() picks one and calls exactly one
/// driver's Init(), which then claims every shared resource it needs. Set to
/// OPT_SBW_IMPL_OFF to compile SBW out entirely. See
/// .claude/docs/drivers/TIM_SBW_DRIVER.md.
///
/// This jiga has independent buffer-enable lines (DIS_RST/DIS_TCK/DIS_JTAG, see
/// the bus-state table below) — unlike the legacy BluePill-BMP's ganged ENA1N — so
/// it CAN selectively tri-state TEST while driving SBWCLK. SBW is nonetheless kept
/// OFF until the buffered TimSbw driver and the SBW_RD/PA0-TEST entry are brought
/// up on this board (Phase 2 of the jiga migration).
///
/// STAGED FOR BRING-UP: the buffered TimSbw transport, the SBW channel/speed block,
/// and the bit-bang entry/clock aliases (TEST=PA0, SBWTCK=PA8, SBW_RD=PA10) are all
/// in place and compile with SBW on. Flip this to OPT_SBW_IMPL_TIM to bench-test SBW
/// (LA + identify) — JTAG stays the default transport, so this can be enabled safely
/// and SBW exercised via the `sbw_scan` monitor command. UNVALIDATED on hardware: the
/// 330Ω TEST/SBWTCK handoff (SbwClkToAf releasing PA0) and the buffer states need an
/// LA check. README marks SBW data/clock "t.b.d.".
#define OPT_SBW_IMPLEMENTATION		OPT_SBW_IMPL_OFF

/// JTCLK generation strategy. SPI is the natural pair with DTRIG (the same SPI MOSI
/// carries the burst, avoiding a fight over the F1 alt-function mux on PA7).
#define OPT_JTAG_TCLK_IMPLEMENTATION	OPT_JTCLK_IMPL_SPI

/// GDB serial port: USART2 (PA2/PA3) — the jiga routes the GDB host UART here and
/// the target-UART passthrough to USART1 (PB6/PB7). (The legacy BMP board had GDB
/// on USART1; the jiga swapped them.) Provisional until the USB VCP is added.
#define OPT_GDB_IMPLEMENTATION			OPT_GDB_IMPL_USART2




/*!
\brief Single source of truth for which MCU peripherals this firmware owns.

`SystemInit()` calls `PeripheralEnabler::Init()` once at boot — this enables
clocks for every listed peripheral and pulses APBxRSTR / AHBRSTR for the ones
that have a reset bit. Per-class `Init()` calls are unnecessary (and deprecated).
*/
using PeripheralEnabler = Clocks::Enabler<
	// GPIO ports (PC/PD carry nothing the firmware configures)
	PortClock<Port::PA>,
	PortClock<Port::PB>,
	// AFIO — required for the USART1 PB6/PB7 remap and the SWO trace pin
	Afio,
	// DMA1 — covers all channels (CH2 SPI RX, CH3 SPI TX, CH4 TIM1_CH4 TMS reload)
	Dma::Controller<Dma::Itf::k1>,
	// Timers used by the firmware
	Timer::TimerDescriptor<Timer::kTim1>,	// DTRIG TMS (CH2 → PA9) + CC4/DMA1_CH4 reload
	Timer::TimerDescriptor<Timer::kTim4>,	// target-voltage PWM regulator (CH4 → PB9)
	// SPI1 carries JTCK/JTDO/JTDI (and the JTCLK burst) in DTRIG mode
	Spi::Hardware<Spi::Iface::k1>,
	// USART2 = GDB host serial; USART1 = target UART passthrough (VCP builds)
	UsartHardware<Usart::k2>,
	UsartHardware<Usart::k1>
>;


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


/* SPI interface grades — used by both SPI and DTRIG modes */
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

/* SBW wire-rate grades. Floor is the flash timing generator (f_FTG must stay in
// 257-476 kHz), so the slowest grade can't drop below ~275 kHz. Values mirror
// target.stlinv2 — fine-tune per board at the bench. */
static constexpr uint32_t SBW_Speed_5 = 1300000UL;	///< top — good/short cabling only
static constexpr uint32_t SBW_Speed_4 = 1100000UL;	///< safe default top speed
static constexpr uint32_t SBW_Speed_3 = 900000UL;
static constexpr uint32_t SBW_Speed_2 = 600000UL;
static constexpr uint32_t SBW_Speed_1 = 300000UL;	///< slowest: can't go below 275 kHz (min flash clock)


// ── Target voltage (#46 PASS 2) ──────────────────────────────────────────────
// SENSE: PB0 = V2REF carries the target VCC through a /2 divider into ADC1_IN8.
// DRIVE: PWM_VT on TIM4_CH4/PB9 into a 47k/10uF RC regulator. PA4 (GEN_VT/DAC on
// G431) shares the same net but the F103 has no DAC, so it is held Hi-Z (analog)
// and only the PB9 PWM drives the rail.
#define OPT_TARGET_HAS_VSENSE	1
#define OPT_TARGET_HAS_VDRIVE	1
static constexpr uint32_t kVtgMax_mV    = 3300;		///< output at 100% PWM duty
static constexpr uint32_t kVtgAdcRef_mV = 3300;		///< ADC full-scale reference (VDDA)
static constexpr uint32_t kVtgSenseMul  = 2;		///< PB0 = VCC/2 → multiply reading by 2
static constexpr uint32_t kVtgMin_mV    = 1800;		///< "power auto" copy floor (output buffers' min rail)
static constexpr uint32_t kVtgValid_mV  = 1600;		///< below this a sensed V2REF reading is invalid
static constexpr uint32_t kVtgDefault_mV = 3300;	///< UIF-style default supply on connect
static constexpr uint32_t kVtgFallback_mV = 3300;	///< "power auto" fallback on an invalid V2REF reading
using VSenseAdc = Adc::AnySetup<Adc::AnyConfig,
	Adc::AnySequence<Adc::Chan<Adc::Unit::k1, 8>>>;
// PWM regulator: TIM4_CH4 free-running, period = kVtgMax_mV so CCR = millivolts
// (1:1). Carrier ~20 kHz (≫ the 47k/10uF filter fc ~0.34 Hz → ripple negligible).
using VtgPwmTimer = Timer::Any<
	Timer::InternalClock_Hz<Timer::kTim4, SysClk, 20000u * kVtgMax_mV>,
	Timer::Mode::kUpCounter, kVtgMax_mV - 1, /*buffered=*/true>;
using VtgPwm = Timer::AnyOutputChannel<VtgPwmTimer, Timer::Channel::k4,
	Timer::OutMode::kPWM1, Timer::Output::kEnabled, Timer::Output::kDisabled,
	/*preload=*/true>;


/// DTRIG drives TIM1 from its internal clock (APB2 × multiplier), so PA8 (bridged
/// to the TCK net) is freed.
using TmsShapeGpioIn = Unchanged<8>;

/// Dedicated pin for write JTMS (PA9 = TIM1_CH2)
using JTMS = AnyOut<Port::PA, 9, Speed::kMedium, Level::kLow>;
/// Logic state for JTMS pin initialization
using JTMS_Init = AnyInPd<Port::PA, 9>;
/// PA9 as TIM1_CH2 alt-function (drives JTMS during JTAG frame generation in DTRIG mode)
using JTMS_PWM = TIM1_CH2_PA9_OUT;

/// Pin for JTCK output
using JTCK = AnyOut<Port::PA, 5, Speed::kMedium, Level::kHigh>;
/// Logic state for JTCK pin initialization
using JTCK_Init = AnyInPu<Port::PA, 5>;
/// JTCK driven by SPI1_SCK during frames (PA5)
using JTCK_SPI = SPI1_SCK_PA5;

/// Pin for JTDI output (input on MCU)
using JTDI = AnyOut<Port::PA, 7, Speed::kMedium, Level::kHigh>;
/// Logic state for JTDI pin initialization
using JTDI_Init = AnyInPu<Port::PA, 7>;
/// JTDI driven by SPI1_MOSI during frames (PA7); same pin doubles as JTCLK in RTI
using JTDI_SPI = SPI1_MOSI_PA7;

/// Pin for JTDO input (output on MCU)
using JTDO = AnyInPu<Port::PA, 6>;
/// Logic state for JTDO pin initialization
using JTDO_Init = AnyInPu<Port::PA, 6>;
/// JTDO captured by SPI1_MISO during frames (PA6)
using JTDO_SPI = SPI1_MISO_PA6;

/// JTDI during run/idle state produces JTCLK
using JTCLK = JTDI;
using JTCLK_SPI = JTDI_SPI;

/// Pin for JRST output (PA1)
using JRST = AnyOut<Port::PA, 1, Speed::kMedium, Level::kLow>;
/// Logic state for JRST pin initialization
using JRST_Init = AnyInPu<Port::PA, 1>;

/// Pin for JTEST output (PA0)
using JTEST = AnyOut<Port::PA, 0, Speed::kMedium, Level::kLow>;
/// Logic state for JTEST pin initialization
using JTEST_Init = AnyInPd<Port::PA, 0>;

/// SBW aliases (SBW reuses the JTAG host pins: SBWCLK on TCK, SBWDIO on TDO/TDI).
/// Dormant while OPT_SBW_IMPLEMENTATION == OPT_SBW_IMPL_OFF; the kWaveSbw* timer
/// channels and SBW_Speed_* grades are added in Phase 2 with the buffered driver.
using SBWDIO_In = JTDO;
using SBWDIO = JTDI;

using SBWCLK = JTCK;

/// SBW direction control (PA10 = SBW_RD, GPIO bit-bang). SBW_RD=0 → SBWDIO drives
/// out (PA7); SBW_RD=1 → read the bus on PA6. PA7 (data) and PA10 (SBW_RD) share
/// GPIOA, so the buffered TimSbw can fold both into one BSRR write. Pin is also
/// TIM1_CH3-capable — reserved for a hardware-timed direction toggle later.
using SBW_RD = AnyOut<Port::PA, 10, Speed::kSlow, Level::kHigh>;

/// SBW direction-flip policy for this jiga — buffered/optimized variant. The
/// direction-script DMA writes one BSRR word per phase boundary to GPIOA->BSRR;
/// only the PA10 (SBW_RD) bit toggles. See "DirPolicy contract" in
/// .claude/docs/drivers/TIM_SBW_DRIVER.md.
struct DirPolicy_PA10_BsrrMux
{
	static constexpr unsigned kWordsPerFlip = 1;
	static void Init() {}			///< no-op — both arrays are constexpr
	// Board wiring (hardware spec): SBW_RD (PA10) = 0 → SBWDIO drives OUT (PA7);
	// SBW_RD = 1 → read the bus on PA6. So "drive output" pulls PA10 LOW and
	// "release to read" drives PA10 HIGH — see SetBusState / README States table.
	static const uint32_t* DriveOutput()
	{
		static constexpr uint32_t v[1] = { 1u << (10 + 16) };	///< BSRR reset PA10 → 0 → dir→OUT
		return v;
	}
	static const uint32_t* DriveInput()
	{
		static constexpr uint32_t v[1] = { 1u << 10 };			///< BSRR set PA10 → 1 → dir→READ
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

/// LED driver (PA15). Dual LED: tri-state = off, high = Red, low = Green
/// (same logic as STLinkV2 / the G431 target).
using LEDS_Init = AnyIn<Port::PA, 15, PuPd::kFloating>;
using LEDS = AnyOut<Port::PA, 15, Speed::kSlow, Level::kHigh>;

/// GEN_VT on PA4 held Hi-Z (analog) — the F103 has no DAC, so the rail is driven
/// only by the PB9 PWM. (On the G431 target PA4 = DAC1_OUT1 also drives GEN_VT.)
using GEN_VT_HiZ = ADC12_IN4;

/// Target-voltage PWM regulator on PB9 / TIM4_CH4 (not PB8/TIM4_CH3): PB8 = BOOT0,
/// and a regulator-input pull-up there would force the chip into DFU at reset.
/// Pin-consistent with the G431 target. TIM4 is free (JTCLK is SPI-generated).
using PWM_VT_0V = AnyOut<Port::PB, 9, Speed::kSlow, Level::kLow>;
using PWM_VT_3V3 = AnyOut<Port::PB, 9, Speed::kSlow, Level::kHigh>;
using PWM_VT = TIM4_CH4_PB9_OUT;

/// Initial configuration for PORTA
using PORTA = AnyPortSetup <Port::PA
	, JTEST_Init			///< PA0  TEST (bit-bang)
	, JRST_Init				///< PA1  RST  (bit-bang)
	, USART2_TX_PA2			///< PA2  GDB serial TX
	, USART2_RX_PA3			///< PA3  GDB serial RX
	, GEN_VT_HiZ			///< PA4  GEN_VT held Hi-Z (analog); PB9 PWM drives the net
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
	, ADC12_IN8				///< PB0  V2REF = target VCC/2 sense (ADC1_IN8)
	, Unused<1>				///< PB1  breakout (unused)
	, Unused<2>				///< PB2  BOOT1
	, TRACESWO_PB3			///< PB3  ARM SWO trace
	, Unused<4>				///< PB4  breakout (unused)
	, Unused<5>				///< PB5  breakout (unused)
	, USART1_TX_PB6			///< PB6  h.TXD → target UART (USART1, VCP builds)
	, USART1_RX_PB7			///< PB7  h.RXD ← target UART (USART1, VCP builds)
	, Unused<8>				///< PB8  BOOT0 — keep floating (no pull-up → DFU)
	, PWM_VT_0V				///< PB9  GEN_VT PWM OFF at boot (UIF-style); TargetPower drives TIM4_CH4
	, DIS_RST				///< PB10 DIS_RST  (buffer enable, init Hi-Z)
	, DIS_TCK				///< PB11 DIS_TCK  (buffer enable, init Hi-Z)
	, DIS_JTAG				///< PB12 DIS_JTAG (buffer enable, init Hi-Z)
	, DIS_COM				///< PB13 DIS_COM  (UART buffer enable, init Hi-Z)
	, Unused<14>			///< PB14 breakout (unused)
	, Unused<15>			///< PB15 breakout (unused)
>;

/// PORTC unused (LED moved to PA15; no PC13). PORTD carries only the HSE OSC pins,
/// which the RCC oscillator owns — no GPIO config needed.
using PORTC = Bmt::DummyInit;
using PORTD = Bmt::DummyInit;

/// All GPIO ports collected for one-shot initialization at startup
using AllGpioStartup = PortMerge<PORTA, PORTB, PORTC, PORTD>;


/// JRST driven HIGH while activating JTAG. UIF holds RST high before the entry
/// sequence (no RST-low pre-pulse), so JtagOn must bring RST up — the bare `JRST`
/// (AnyOut default Level::kLow) would drive it low. See
/// I2031_ACQUISITION_GOLDEN_REFERENCE.md.
using JRST_On = AnyOut<Port::PA, 1, Speed::kMedium, Level::kHigh>;
/// JTAG-bus active config for DTRIG: JTMS goes to TIM1_CH2 alt-function so the
/// timer can drive the TMS waveform; JTCK/JTDO/JTDI stay as GPIO until JtagSpiOn
/// flips them to SPI mode.
using JtagOn = AnyPinGroup <Port::PA
	, JTEST					///< JTEST pin for bit bang access
	, JRST_On				///< JRST driven HIGH (no RST-low pre-pulse before entry)
	, JTCK					///< JTCK pin for bit bang access
	, JTDO					///< JTDO pin for bit bang access
	, JTDI					///< JTDI pin for bit bang access
	, Unchanged<8>			///< PA8 unused (TIM1 internal clock)
	, JTMS_PWM				///< PA9 → TIM1_CH2 alt-function for JTMS
>;

/// === Init state (DriverOff): release pins to Hi-Z; the target's pull-ups/-downs
/// own the bus. Applied only by OnReleaseDriver() (from OnClose()), which also drops
/// the buffer enables (BusState::kStandby). This is the old "JtagOff" (Hi-Z) set.
using DriverOff = AnyPinGroup <Port::PA
	, JTEST_Init			///< JTEST in Hi-Z
	, JRST_Init				///< JRST in Hi-Z
	, JTCK_Init				///< JTCK in Hi-Z
	, JTDO_Init				///< JTDO in Hi-Z
	, JTDI_Init				///< JTDI in Hi-Z
	, TmsShapeGpioIn		///< Keep as input
	, JTMS_Init				///< JTMS in Hi-Z
>;

/// === Close state (JtagOff): DRIVE the bus idle level instead of Hi-Z while the
/// buffers stay enabled, so the resting level is held by the MCU/buffer outputs,
/// not the target's pull-ups/-downs, between acquisition attempts. Applied by
/// OnReleaseJtag(). Idle mapping: RST=high, TEST=low, TMS=low, TCK=high, TDI=high;
/// TDO=input.
using JRST_Close  = AnyOut<Port::PA, 1, Speed::kMedium, Level::kHigh>;		///< RST driven high
using JTEST_Close = AnyOut<Port::PA, 0, Speed::kMedium, Level::kLow>;		///< TEST driven low
using JTMS_Close  = AnyOut<Port::PA, 9, Speed::kMedium, Level::kLow>;		///< TMS driven low
using JtagOff = AnyPinGroup <Port::PA
	, JTEST_Close			///< JTEST driven low (TEST=L)
	, JRST_Close			///< JRST driven high (RST=H)
	, JTCK					///< JTCK driven high (TCK=H)
	, JTDO_Init				///< JTDO input (target drives TDO)
	, JTDI					///< JTDI driven high (TDI=H)
	, TmsShapeGpioIn		///< Keep as input
	, JTMS_Close			///< JTMS driven low (TMS=L)
>;

/// PA5/PA6/PA7 → SPI AF mode for DTRIG operation (JTCK/JTDO/JTDI on SPI1)
using JtagSpiOn = AnyPinGroup <Port::PA
	, JTCK_SPI				///< PA5 → SPI1_SCK (= JTCK)
	, JTDO_SPI				///< PA6 → SPI1_MISO (= JTDO)
	, JTDI_SPI				///< PA7 → SPI1_MOSI (= JTDI / JTCLK)
>;
/// PA5/PA6/PA7 → GPIO mode for the brief windows where bit-bang access to
/// JTCK/JTDI is needed (e.g. JTCLK muting via PA5 = high).
using JtagGpioOn = AnyPinGroup <Port::PA
	, JTCK					///< PA5 → GPIO output (JTCK manual control)
	, JTDO					///< PA6 → GPIO input (JTDO read)
	, JTDI					///< PA7 → GPIO output (JTDI manual control)
>;

/// SPI1 carries JTCK (SCK/PA5), JTDI (MOSI/PA7), JTDO (MISO/PA6); TIM1 drives TMS only
static constexpr Spi::Iface kSpiForJtag = Spi::Iface::k1;
/// Main JTAG signal generation (Must be an advanced timer — TIM1)
static constexpr Timer::Unit kWaveJtagTimer = Timer::kTim1;
/// JTMS sits on TIM1_CH2 → PA9 (regular CH, not CHN), matching the G431 target.
static constexpr Timer::Channel kWaveJtagTms     = Timer::Channel::k2;	// PWM → CH2 → PA9
/// CH4 compare-only: CC4 DMA reloads the TMS CCR (CCR2) at end of entry pulse.
/// On F1 the channel↔DMA map is fixed: TIM1_CH4 → DMA1_CH4 — free, because SPI1_RX
/// uses DMA1_CH2 and SPI1_TX uses DMA1_CH3 (CH1→DMA1_CH2 would collide with RX, and
/// CH2's DMA1_CH3 with TX). The reload destination tracks kWaveJtagTms automatically
/// (DtrigJtag uses TmsOut::GetCcrAddress()).
static constexpr Timer::Channel kWaveJtagTmsRld1 = Timer::Channel::k4;	// entry→shift CCR reload
/// JTMS sits on TIM1_CH2 (PA9) — the regular output, not CHN. Tells DtrigJtag to
/// drive the main CH output via Output::kInverted (CCxP=1) + PWM1; on this F103
/// silicon that combination empirically produces TMS=HIGH for the entry pulse and
/// LOW for the shift portion. See the TmsOut comment in DtrigJtag.h.
static constexpr bool kWaveJtagTmsCmpComplementary = false;


// ── SBW channel assignments (buffered TimSbw) ────────────────────────────────
// TIM1 is the master timer for SBW too (JTAG and SBW are mutually exclusive — only
// one driver's Init() runs). SBWCLK is on TIM1_CH1 → PA8, which is PCB-bridged to
// PA5 (the TCK net), so the timer output reaches the SBWTCK trace. SBW uses the same
// TIM1 channels JTAG does (free because the two never run together): CH2→DMA1_CH3
// (composite data|dir BSRR), CH4→DMA1_CH4 (IDR sample).
static constexpr Timer::Unit    kWaveSbwTimer        = Timer::kTim1;
static constexpr Timer::Channel kWaveSbwClk          = Timer::Channel::k1;	///< TIM1_CH1 → PA8 (bridged → PA5/TCK)
static constexpr Timer::Channel kWaveSbwDataTrig     = Timer::Channel::k2;	///< → DMA1_CH3 (composite data|dir BSRR)
static constexpr Timer::Channel kWaveSbwSampleTrig   = Timer::Channel::k4;	///< → DMA1_CH4 (IDR sample)
static constexpr Timer::Channel kWaveSbwDirTrig      = Timer::Channel::k3;	///< UNUSED on the buffered path (dir folded into data BSRR)
static constexpr bool kWaveSbwCmpComplementary       = false;				///< CH1 is a regular output
/// Buffered board → TimSbw (data+dir folded into one BSRR write, read on a separate
/// pin). See the transport selector in SbwDev.tim.cpp.
static constexpr bool kWaveSbwSeparateDirDma         = false;
/// Direction is the SBW_RD (PA10) bit folded into the data BSRR — same GPIO port as
/// the SBWDIO drive pin (PA7). See DirPolicy_PA10_BsrrMux above.
using SbwDirPolicy = DirPolicy_PA10_BsrrMux;

// ── SBW bit-bang entry / clock pins (consumed by SbwDev.cpp / SbwDev.tim.cpp) ──
// On this buffered board TEST and SBWTCK are SEPARATE pins (330Ω-joined at the
// connector, "TEST always wins"), unlike STLinkV2 where they are one pin.
/// Entry TEST glitch pin (PA0) — bit-banged by SbwDev::OnEnterTap.
using SBWTEST_Bb = JTEST;								///< AnyOut<PA,0>
/// Bit-bang frame-clock pin (PA8, bridged → PA5/TCK) — toggled by the RTI TCLK
/// strobes and parked static-high for flash TCLK (SbwDev.tim.cpp).
using SBWCLK_Bb  = AnyOut<Port::PA, 8, Speed::kFast, Level::kLow>;
/// Entry ~RST/SBWDIO data pin = the SBWDIO drive pin (PA7).
using SBWRST_Bb  = SBWDIO;								///< = JTDI = PA7
/// Half-duplex turnaround is the SBW_RD direction bit (PA10), NOT a data-pin mode
/// flip: drive = SBW_RD low (output), release = SBW_RD high (read). SetupPinMode()
/// sets PA10's mode + level in one masked RMW.
using SbwDioDrive_Bb   = AnyOut<Port::PA, 10, Speed::kSlow, Level::kLow>;	///< SBW_RD=0 → drive
using SbwDioRelease_Bb = AnyOut<Port::PA, 10, Speed::kSlow, Level::kHigh>;	///< SBW_RD=1 → read
/// Post-entry handoff: hand the clock pin (PA8) to TIM1_CH1 AF for DMA frames AND
/// release the TEST pin (PA0) to Hi-Z so the 330Ω-joined SBWTCK line is owned by the
/// PA8 clock (a still-driven TEST would fight the clock's low pulses through 330Ω).
/// The PWM idles HIGH, so the joined line stays high (SBW stays enabled).
/// BENCH-TUNABLE: PA0 release polarity / pull may need adjustment on hardware.
using SbwClkToAf = AnyPinGroup<Port::PA
	, TIM1_CH1_PA8<Mode::kAlternate, Speed::kFast>	///< PA8 → TIM1_CH1 (SBWCLK)
	, AnyIn<Port::PA, 0, PuPd::kFloating>			///< PA0 (TEST) → Hi-Z; clock owns the joined line
>;

// SBW pin bring-up (called only by SbwDev). Sets the MCU-side SBW pin MODES for the
// entry sequence: release the clock pin (PA8) and its bridged twin (PA5) to Hi-Z so
// they do not fight the PA0 TEST glitch on the 330Ω-joined line; drive SBWDIO (PA7)
// output; read-back (PA6) input. SBW_RD (PA10) and the DIS_* buffers are driven by
// SetBusState; the clock pin is handed to TIM1 AF post-entry by SbwClkToAf.
using SbwBusOnPins = AnyPinGroup<Port::PA
	, AnyIn<Port::PA, 5, PuPd::kFloating>	///< PA5 released (bridged to the PA8 clock trace)
	, JTDO_Init								///< PA6 read-back input (pull-up)
	, JTDI									///< PA7 SBWDIO drive output
	, AnyIn<Port::PA, 8, PuPd::kFloating>	///< PA8 clock pin released for the TEST-glitch entry
>;
/// SBW teardown: SBW PA pins back to Hi-Z (target pull-ups/-downs own the bus).
using SbwBusOffPins = AnyPinGroup<Port::PA
	, AnyIn<Port::PA, 5, PuPd::kFloating>	///< PA5 Hi-Z
	, JTDO_Init								///< PA6 Hi-Z (input PU)
	, JTDI_Init								///< PA7 Hi-Z
	, AnyIn<Port::PA, 8, PuPd::kFloating>	///< PA8 Hi-Z
>;
ALWAYS_INLINE void SbwBusOn()    { SbwBusOnPins::Setup(); }
ALWAYS_INLINE void SbwBusOff()   { SbwBusOffPins::Setup(); }
// No separate driven-idle SBW state on this board (same as SbwBusOff for now —
// bench-tunable once SBW is validated).
ALWAYS_INLINE void SbwBusClose() { SbwBusOffPins::Setup(); }


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
	case LedState::on:		LEDS::SetupPinMode();		break;
	case LedState::red:		LEDS::SetHigh();			break;
	case LedState::green:	LEDS::SetLow();				break;
	default:				LEDS_Init::SetupPinMode();	break;
	}
}


/// Enables MSP430 UART interface buffers (DIS_COM = PB13, negative logic)
ALWAYS_INLINE void UartBusOn() { DIS_COM::SetLow(); }
/// Disables MSP430 UART interface buffers
ALWAYS_INLINE void UartBusOff() { DIS_COM::SetHigh(); }


/// The three DIS_* buffer-enable lines grouped for a single BSRR write (bmt
/// AnyCounter). All on PB and sequential (PB10/11/12), so Write() folds them into one
/// store. Bit order: 0=DIS_RST, 1=DIS_TCK, 2=DIS_JTAG. The table below holds the raw
/// pin levels (1 = HIGH = buffer tri-stated), so we use Write(), not WriteComplement().
using BusBufCtrl = AnyCounter<DIS_RST, DIS_TCK, DIS_JTAG>;

/// Drives the bus buffer-enable lines from the firmware's BusState, per the "States"
/// table in Hardware/BluePill-G431/README.md. Table-indexed (the enum is 0..4
/// contiguous): one grouped BSRR write for DIS_*, plus the SBW_RD bit (PA10, a
/// different port, so it can't join the PB counter).
///
///   bit  3        2         1        0
///        SBW_RD   DIS_JTAG  DIS_TCK  DIS_RST     (1 = pin HIGH)
///
///   line     | Standby | AcqJTAG | AcqSBW | JTAG | SBW
///   DIS_RST  |    1    |    0    |   0    |  0   |  1
///   DIS_TCK  |    1    |    1    |   1    |  0   |  0
///   DIS_JTAG |    1    |    1    |   1    |  0   |  1
///   SBW_RD   |    1    |    1    |   0    |  1   | idle high (driver owns it R/W per-cycle)
///
/// DIS_COM (target UART) is managed separately (UartBusOn/Off).
ALWAYS_INLINE void SetBusState(const BusState st)
{
	static constexpr uint8_t kPattern[] = {
		0b1111,		///< kStandby       — whole bus Hi-Z
		0b1110,		///< kAcquiringJtag — only RST buffer live for the entry glitch
		0b0110,		///< kAcquiringSbw  — RST buffer live, SBW_RD low (drive entry)
		0b1000,		///< kJtag          — full JTAG bus driving
		0b1101,		///< kSbw           — RST buffer OFF (role moves to SBWTDIO), SBW_RD idle high
	};
	const uint8_t p = kPattern[(unsigned)st];
	BusBufCtrl::Write(p & 0b0111);					// DIS_RST/TCK/JTAG in one BSRR write
	if (p & 0b1000) SBW_RD::SetHigh(); else SBW_RD::SetLow();
}
