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
	PB12:	SWD_IN: passive level-translated echo of PB14/TMS (= JTAG-20 pin 7).
			Unused for JTAG, but it is the SBW read-back path: it always reads the
			true bus level whether the probe drives or releases PB14. See SBW notes
			below and Hardware/STLinkV2/README.md "Bus direction (read-back)".
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

/// JTAG transport selection. DTRIG is the only supported variant — see
/// .claude/docs/drivers/SPI_VARIANT_REMOVED.md and TIM_VARIANT_REMOVED.md.
#define OPT_JTAG_IMPLEMENTATION			OPT_JTAG_IMPL_DTRIG

/// SBW (Spy-Bi-Wire) transport selection. Independent of OPT_JTAG_IMPLEMENTATION
/// — both can be compiled in, but only one can be active at runtime (they share
/// TIM1 + GPIO + DMA channels). TapMcu::Open() picks one and calls exactly one
/// driver's Init(), which then claims every shared resource it needs. Set to
/// OPT_SBW_IMPL_OFF to compile SBW out entirely. See
/// .claude/docs/drivers/TIM_SBW_DRIVER.md.
#define OPT_SBW_IMPLEMENTATION			OPT_SBW_IMPL_TIM

/// Bench bring-up: TapMcu keeps JtagDev as the active backend until this is 1.
/// Set to 1 for a TimSbw bench session (forces SBW as the live driver); the
/// build asserts OPT_SBW_IMPLEMENTATION is an active backend. Leave 0 for
/// normal JTAG operation. See .claude/docs/drivers/TIM_SBW_DRIVER.md.
#define OPT_HARD_SELECT_SBW_TMP		0

/// Bench diagnostic: when 1, SbwDev::DumpReadPhase() dumps the read-phase IDR
/// sample buffer (bus / clk / rd, one char per SBW cycle) over TRACESWO on every
/// DR/IR scan — an internal logic-analyzer view for verifying TDO sample phase
/// and the level-translator path. Verbose and slow; leave 0 except during an
/// SBW read bring-up session. See Firmware.shared/util/TimSbw.h.
#define OPT_SBWDEV_DUMP_READ_PHASE	0

/// JTCLK generation strategy.  SPI variant is the natural pair with DTRIG —
/// same SPI MOSI carries the burst, no F1 alt-function mux fight on PA7.
#define OPT_JTAG_TCLK_IMPLEMENTATION	OPT_JTCLK_IMPL_SPI

/// Implementation for "GDB serial port" (USART used provisory until USB VCP is added to firmware)
#define OPT_GDB_IMPLEMENTATION			OPT_GDB_IMPL_USART2
// Use this for Geehy APM32F103CB. It has issues with the SWOTRACE
#define OPT_GEEGY_APM32F103CB			1




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
	Timer::TimerDescriptor<Timer::kTim1>,	// JTAG wave generator (advanced timer; CH2N=JTMS)
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

/* SBW wire-rate grades. The ceiling is NOT the MSP430's 5 MHz spec max but the
// target's RST/SBWDIO reset RC: the SBWTDIO line is tied to RST/NMI, whose reset
// cap (e.g. F5418A: 3k3 series + 47k/2.2nF) dominates the bus and limits how fast
// the input→output turnaround can settle. Above ~1.2 MHz it does not settle in the
// slot even with the series resistor → read errors. So 1.2 MHz is the top grade
// (good/short cabling only), 1.0 MHz the safe default, stepping down to 200 kHz
// for long/marginal cabling. Frequencies are nominal — integer-PSC rounding is
// irrelevant for SBW timing (the MSP430 spec gives only a max, not a tolerance). */
static constexpr uint32_t SBW_Speed_5 = 1200000UL;	///< top — good/short cabling only
static constexpr uint32_t SBW_Speed_4 = 1000000UL;	///< safe default top speed
static constexpr uint32_t SBW_Speed_3 = 800000UL;
static constexpr uint32_t SBW_Speed_2 = 400000UL;
static constexpr uint32_t SBW_Speed_1 = 200000UL;	///< slowest — long/marginal cabling

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

/// Pin for JTCK output (GPIO bit-bang mode for OnResetTap and manual control)
using JTCK = AnyOut<Port::PA, 5, Speed::kMedium, Level::kHigh>;
/// Logic state for JTCK pin initialization
using JTCK_Init = AnyInPu<Port::PA, 5>;
/// SPI1 SCK on PA5 carries JTCK in dtrig mode (PA5 and PB13 are shorted on the STLinkV2 PCB)
using JTCK_SPI = SPI1_SCK_PA5;

/// SPI1 MISO on PA6 carries JTDO in dtrig mode
using JTDO_SPI = SPI1_MISO_PA6;

/// SPI1 MOSI on PA7 carries JTDI in dtrig mode
using JTDI_SPI = SPI1_MOSI_PA7;

/// SPI1 MOSI doubles as JTCLK (TDI = TCLK during Run-Test/Idle)
using JTCLK_SPI = JTDI_SPI;
/// PB14 as TIM1_CH2N alternate function (drives JTMS during JTAG frame generation)
using JTMS_PWM = TIM1_CH2N_PB14_OUT;

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

// ---------------------------------------------------------------------------
// Spy-Bi-Wire (SBW) pin plan — read-back direction handling on this hardware
//
// SBW is half-duplex: the single SBWDIO line (mapped to TMS/PB14 per the
// STLink-Adapter wiring: DIO=TMS, CLK=TCK) is driven by the probe and then
// released mid-frame so the target answers. This board has NO PA9 direction
// mux (that is a planned optimization for the G431/BluePill targets, where the
// direction bit is folded into the DMA word). Here the turnaround is software
// paced.
//
// The original STM design hands us a passive read-back instead: PB14 (TMS_SWDO)
// is echoed to PB12 (SWD_IN) through the voltage level converter. PB12 always
// reflects the real bus level — the 3.3 V driven level while PB14 drives, and
// the level-translated Target-VCC level once PB14 tri-states (held by the SBW
// pull-up). So SBWDIO is WRITTEN on PB14 and READ on PB12.
//
// Two turnaround strategies are open (see TIM_SBW_DRIVER.md):
//   (1) tri-state PB14 and sample PB12 during the target's turn, or
//   (2) keep everything on PB14, flipping its GPIO direction in place.
// Strategy (1) is the reason SBWDIO_In points at PB12 rather than PB14.
//
// Over-voltage caveat: translation helps inputs only — PB14 always drives 3.3 V,
// so a sub-3 V MSP430 is over-driven regardless (variable Vcc unusable < 3 V).
// ---------------------------------------------------------------------------

/// Pin for SBWDIO input — read-back via the SWD_IN echo of TMS (PB12), NOT PB14.
using SBWDIO_In = AnyInPu<Port::PB, 12>;

/// Pin for SBWDIO output (drives the bus on TMS/PB14)
using SBWDIO = JTMS;

/// Pin for SBWCLK output
using SBWCLK = JTCK;

// ── SBW entry-sequence pin roles ─────────────────────────────────────────────
// SBW is a two-wire interface and on this repurposed-SWD hardware it lives
// ENTIRELY on the SWD pins (see Hardware/STLink-Adapter/README.md SBW table:
// DIO→TMS, CLK→TCK; the JTAG-14 connector is explicitly NOT used for SBW):
//   • SBWTCK — the TEST-role pin (entry sequence + fuse-sense, then clock) = PB13
//   • SBWTDIO — the ~RST/NMI/SBWDIO pin (entry data, then I/O)             = PB14
// The dedicated JTAG nRST/TEST lines (JRST=PB0, JTEST=PB1) are NOT routed to the
// SBW connector, so the activation sequence MUST be bit-banged on PB13/PB14, not
// on PB0/PB1. PB13 is driven as a GPIO output during the handshake, then handed
// to TIM1_CH1N for DMA-clocked frames (SbwClkToAf touches PB13 only, so it does
// not disturb the ~RST level already established on PB14).
using SBWTEST_Bb = AnyOut<Port::PB, 13, Speed::kFast>;	///< entry TEST role (=SBWTCK pin) as GPIO out
using SBWRST_Bb  = SBWDIO;								///< entry ~RST role (=SBWTDIO pin, PB14)
using SbwClkToAf = AnyPinGroup<Port::PB
	, TIM1_CH1N_PB13<Mode::kAlternate, Speed::kFast>	///< hand PB13 back to TIM1_CH1N for frame clocking
>;

// ── Bit-bang half-duplex turnaround (TCLK strobes) ───────────────────────────
// The bit-banged Run-Test/Idle TCLK strobes must release SBWTDIO (PB14) to the
// target every TDO slot, exactly like the DMA frame engine's per-cycle CRH flip
// (SbwCrhDrive/SbwCrhRelease). But during a strobe PB13 is a GPIO output (SBWTCK
// is driven by hand), so the full-CRH SbwDirPolicy cannot be reused — it would
// hand PB13 back to TIM1_CH1N AF mid-strobe. These two single-pin types flip
// ONLY the PB14 nibble (SetupPinMode does a masked CRH RMW), leaving PB13 alone.
using SbwDioDrive_Bb   = SBWDIO;						///< PB14 push-pull output (host drives TMS/TDI)
using SbwDioRelease_Bb = Floating<Port::PB, 14>;		///< PB14 floating input (target drives TDO)

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


/// Initial configuration for PORTA
using PORTA = AnyPortSetup <Port::PA
	, Unused<0>				///< Vref (pending)
	, Unused<1>				///< not used
	, USART2_TX_PA2			///< UART2 TX --> JRXD
	, USART2_RX_PA3			///< UART2 RX -- > JTXD
	, Unused<4>				///< not used
	, JTCK_Init				///< bit bang/SPI1_SCK (DTRIG)
	, JTDO_Init				///< bit bang/SPI1_MISO (DTRIG)
	, JTDI_Init				///< bit bang/SPI1_MOSI (DTRIG)
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
	, Unused<12>			///< SWD_IN: SBW read-back echo of PB14/TMS (configured by SBW Init when active)
	, Unused<13>			///< not used (PA5 drives JTCK in DTRIG mode)
	, JTMS_Init				///< bit bang/TIM1_CH2N (DTRIG PWM)
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

// Operate both ports for JtagOn
using JtagOn = PortMerge<JtagOn1, JtagOn2>;


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

// Operate both ports for JtagOff
using JtagOff = PortMerge<JtagOff1, JtagOff2>;

/// Port A group: PA5/PA6/PA7 → SPI AF mode for dtrig operation
using JtagSpiOn = AnyPinGroup <Port::PA
	, JTCK_SPI				///< PA5 → SPI1_SCK (drives JTCK; wired to PB13 on PCB)
	, JTDO_SPI				///< PA6 → SPI1_MISO (= JTDO)
	, JTDI_SPI				///< PA7 → SPI1_MOSI (= JTDI / JTCLK)
>;
/// Port A group: PA5/PA6/PA7 → GPIO mode for bit-bang windows
using JtagGpioOn = AnyPinGroup <Port::PA
	, JTCK					///< PA5 → JTCK GPIO out
	, JTDO					///< PA6 → JTDO GPIO in
	, JTDI					///< PA7 → JTDI GPIO out
>;

/// Main JTAG signal generation (Must be an advanced timer - TIM1)
static constexpr Timer::Unit kWaveJtagTimer = Timer::kTim1;
/// SPI1 carries JTCK (SCK/PA5), JTDI (MOSI/PA7), JTDO (MISO/PA6); TIM1 drives TMS only
static constexpr Spi::Iface kSpiForJtag = Spi::Iface::k1;
/// TIM1_CH2 toggle output; complementary CH2N drives PB14 (JTMS) — no per-bit DMA
static constexpr Timer::Channel kWaveJtagTms    = Timer::Channel::k2;		// toggle → CH2N → PB14
/// TIM1_CH3 compare-only: CC3 DMA (DMA1_CH6) reloads CCR2 at end of entry pulse
static constexpr Timer::Channel kWaveJtagTmsRld1 = Timer::Channel::k3;		// entry→shift CCR2 reload
/// JTMS sits on TIM1_CH2N (PB14) — the complementary output, which inverts OCREF
/// naturally. Tells DtrigJtag to enable CHN with default polarity; CHN_pin = NOT_OCREF.
static constexpr bool kWaveJtagTmsCmpComplementary = true;

// ── SBW channel assignments (TimSbw — single-pin / full-CRH direction) ───────
// STLinkV2 is the un-buffered single-pin case: SBWDIO drives on PB14 and is
// released to Hi-Z each TDO slot so the target answers; the bus is read back on
// PB12 (SWD_IN echo). Direction is a *separate* full-CRH DMA — the whole GPIOB
// high half (PB8..PB15) is ours (only the ex-ARM-SWD pins live there), so we
// just DMA the new CRH word per cycle, no read-modify-write. See
// .claude/docs/drivers/TIM_SBW_DRIVER.md.
//
// TIM1 channel → DMA1 channel (F103): CH2→CH3 (data BSRR), CH3→CH6 (dir CRH),
// CH4→CH4 (IDR sample); all distinct. SBWCLK PWM rides CH1N (no DMA). JTAG and
// SBW are mutually exclusive, so reusing JTAG's DMA channels is fine.
static constexpr Timer::Unit    kWaveSbwTimer        = Timer::kTim1;
/// SBWCLK on TIM1_CH1N → PB13 (complementary CH). PB13 is shorted to PA5 on the
/// PCB, so driving CH1N produces SBWCLK on the trace (PA5 must be released).
static constexpr Timer::Channel kWaveSbwClk          = Timer::Channel::k1;
/// CH2 frozen-compare → DMA1_CH3: data BSRR DMA for PB14 (TMS/TDI set/reset).
static constexpr Timer::Channel kWaveSbwDataTrig     = Timer::Channel::k2;
/// CH3 frozen-compare → DMA1_CH6: direction DMA, full GPIOB->CRH word per cycle.
static constexpr Timer::Channel kWaveSbwDirTrig      = Timer::Channel::k3;
/// CH4 frozen-compare → DMA1_CH4: IDR sample DMA (read PB12 on TDO cycles).
static constexpr Timer::Channel kWaveSbwSampleTrig   = Timer::Channel::k4;
/// SBWCLK rides the complementary output (CH1N on PB13).
static constexpr bool kWaveSbwCmpComplementary       = true;
/// Single-pin board: direction is a separate full-register (CRH) DMA.
static constexpr bool kWaveSbwSeparateDirDma         = true;

/// GPIO port helper for IDR sample DMA. SBWDIO_In (PB12, SWD_IN echo) is on GPIOB.
struct SbwIdrPort_B
{
	static GPIO_TypeDef* Get() { return GPIOB; }
};
using SbwIdrPort = SbwIdrPort_B;
/// Bit position of SBWDIO_In (PB12) inside GPIOB->IDR.
static constexpr uint8_t kSbwIdrBit = 12;

/// SBW direction via full-CRH DMA — no hand-computed constants. Two pin groups
/// describe the "drive" and "release" states of GPIOB's high half; bmt folds
/// each into a constexpr CRH word (AnyPinGroup::kCRH_). AnyPinGroup (unlike
/// AnyPortSetup) only constrains the pins we name:
///   PB12  read-back echo, input pull-up        (fixed both states)
///   PB13  SBWCLK = TIM1_CH1N, AF push-pull      (fixed both states)
///   PB14  half-duplex data: PP-output when driving, floating-input when released
/// Unnamed PB8..11/PB15 resolve to analog-in (0x0) in the full-word write — that
/// is harmless here (they are unused and the whole CRH is ours on this board).
using SbwCrhDrive = AnyPinGroup<Port::PB
	, SBWDIO_In									///< PB12 input pull-up
	, TIM1_CH1N_PB13<Mode::kAlternate, Speed::kFast>	///< PB13 SBWCLK AF push-pull
	, AnyOut<Port::PB, 14, Speed::kFast>		///< PB14 push-pull output (driving)
>;
using SbwCrhRelease = AnyPinGroup<Port::PB
	, SBWDIO_In									///< PB12 input pull-up
	, TIM1_CH1N_PB13<Mode::kAlternate, Speed::kFast>	///< PB13 SBWCLK AF push-pull
	, Floating<Port::PB, 14>					///< PB14 floating input (released to target)
>;

/// Generic single-pin direction policy: DMAs a whole mode register (CRH on F1)
/// per cycle from two pin-group-derived words. Reusable across F1 single-pin SBW
/// boards (promote to a shared F1 header if a second board needs it).
template <typename DriveGroup, typename ReleaseGroup>
struct DirPolicy_FullCrh
{
	static constexpr unsigned kWordsPerFlip = 1;
	static void Init() {}
	static const uint32_t* DriveOutput()
	{
		static constexpr uint32_t v[1] = { DriveGroup::kCRH_ };
		return v;
	}
	static const uint32_t* DriveInput()
	{
		static constexpr uint32_t v[1] = { ReleaseGroup::kCRH_ };
		return v;
	}
	static volatile uint32_t* DirRegister()
	{
		return &reinterpret_cast<GPIO_TypeDef*>(DriveGroup::kPortBase_)->CRH;
	}
};
using SbwDirPolicy = DirPolicy_FullCrh<SbwCrhDrive, SbwCrhRelease>;

/// Turnaround must touch only PB14 (CRH nibble 6 = bits 24..27). If a future pin
/// edit makes the two states differ elsewhere, the per-cycle CRH write would
/// disturb SBWCLK/read-back — catch that at compile time.
static_assert(((SbwCrhDrive::kCRH_ ^ SbwCrhRelease::kCRH_) & ~0x0F000000u) == 0,
	"SBW drive/release CRH words must differ only in the PB14 nibble");

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


// ── SBW bus activation (single-pin board) ────────────────────────────────────
// NOTE: SetBusState(BusState::sbw) is NOT an SBW-vs-JTAG discriminator —
// JtagDev::OnConnectJtag also sets BusState::sbw for the buffered-board buffer
// enables. So SBW-only pin bring-up lives in these dedicated hooks, called only
// from SbwDev.
//
// PB13 (TIM1_CH1N) drives the SBWCLK trace, which is shorted to PA5 on the PCB —
// release PA5 to input so it does not fight the timer output. PB12 is the
// read-back echo (input pull-up); PB14 is the half-duplex data pin, parked as a
// push-pull output (the per-cycle CRH DMA then owns its direction during
// frames). PB13's AF mode is configured by TimSbw::Init() (SbwClkOut::Setup()).
/// SBW-active pin state. PA5 released to input (PB13/TIM1_CH1N owns the shorted
/// SBWCLK trace); PB13 → TIM1_CH1N alternate-function push-pull (SBWCLK output);
/// PB12 input pull-up (read-back echo); PB14 push-pull output (idle drive — the
/// per-cycle CRH DMA then owns its direction during frames). One masked CRL/CRH
/// write per port.
///
/// NOTE: PB13 MUST be set to AF here. TimSbw::Init()/SbwClkOut::Setup() only
/// program the TIM1 compare registers (CCMR/CCER/BDTR) — bmt's AnyOutputChannel
/// does NOT configure the GPIO pin mode. The SbwCrhDrive/SbwCrhRelease words
/// also encode PB13=AF, but those are only written by the per-cycle direction
/// DMA during a frame; the SBWCLK output would be dead before the first frame
/// (and entirely, if the dir DMA is disabled) without setting it here.
using SbwBusOnPins = PortMerge<
	  AnyPinGroup<Port::PA, JTCK_Init>		///< PA5 → input pull-up
	, AnyPinGroup<Port::PB
		, SBWDIO_In							///< PB12 → in-PU (read-back echo)
		, TIM1_CH1N_PB13<Mode::kAlternate, Speed::kFast>	///< PB13 → SBWCLK AF
		, SBWDIO>							///< PB14 → push-pull output
>;
/// SBW-idle/teardown pin state: PB14 → Hi-Z, PA5 → input pull-up.
using SbwBusOffPins = PortMerge<
	  AnyPinGroup<Port::PA, JTCK_Init>		///< PA5 → input pull-up
	, AnyPinGroup<Port::PB, JTMS_Init>		///< PB14 → Hi-Z
>;

ALWAYS_INLINE void SbwBusOn()  { SbwBusOnPins::Setup(); }
ALWAYS_INLINE void SbwBusOff() { SbwBusOffPins::Setup(); }
