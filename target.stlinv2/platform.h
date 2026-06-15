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

// ── Startup mode ─────────────────────────────────────────────────────────────
// The single switch for what the firmware does at power-up. See the OPT_STARTUP_*
// enum in platform-defs.h for the full list (GDB / DETECT_JTAG / DETECT_SBW /
// LA_WAVEFORM / TIM_DMA_TIMING / SBW_TDO_SETTLE / SBW_LA_WAVEFORM). OPT_STARTUP_GDB
// = normal. The SBW modes auto-connect with no GDB host; mode params in stdproj.h.
// SBW_LA_WAVEFORM = one-shot SBW IR/DR/flash-TCLK reference waveform for an LA.
#define OPT_STARTUP						OPT_STARTUP_GDB

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
	// TIM3 + TIM4 are unused (JTCLK is SPI-generated, OPT_JTCLK_IMPL_SPI), so they
	// are not clock-enabled here.
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
// the input→output turnaround can settle. Above ~1.3 MHz it does not settle in the
// slot even with the series resistor → read errors. So 1.3 MHz is the top grade
// (good/short cabling only, F5418A w/ R+R/C), 1.1 MHz the recommended top, stepping
// down to 300 kHz. The floor is the flash timing generator: f_FTG must stay in
// 257-476 kHz, so the slowest grade can't drop below ~275 kHz (the future flash-write
// path derives its clock from the link). Frequencies are nominal — integer-PSC
// rounding is irrelevant for SBW timing (the MSP430 spec gives only a max). */
static constexpr uint32_t SBW_Speed_5 = 1500000UL;	///< works on MSP430F5418A, with R + R/C
static constexpr uint32_t SBW_Speed_4 = 1000000UL;	///< recommended for R + R/C
static constexpr uint32_t SBW_Speed_3 = 750000UL;
static constexpr uint32_t SBW_Speed_2 = 600000UL;
static constexpr uint32_t SBW_Speed_1 = 300000UL;	///< slowest: Can't allow slower than 275kHz (min flash clock!)

// ── Target voltage (#46 PASS 2) ──────────────────────────────────────────────
// This ST-Link clone has a FIXED 3.3 V target supply: it can SENSE the target
// voltage but cannot DRIVE a variable one. PA0 carries the target VCC through a
// /2 divider (board "VREF/2" node) into ADC1_IN0.
#define OPT_TARGET_HAS_VSENSE	1
#define OPT_TARGET_HAS_VDRIVE	0
static constexpr uint32_t kVtgMax_mV    = 3300;		///< feasible drive ceiling (n/a: sense-only here)
static constexpr uint32_t kVtgAdcRef_mV = 3300;		///< ADC full-scale reference (VDDA)
static constexpr uint32_t kVtgSenseMul  = 2;		///< PA0 = VCC/2 → multiply reading by 2
/// One-shot ADC1 setup for the single VREF/2 channel (software-triggered).
using VSenseAdc = Adc::AnySetup<Adc::AnyConfig,
	Adc::AnySequence<Adc::Chan<Adc::Unit::k1, 0>>>;

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
/// JTEST_Init drives PB1 LOW (AnyOut default Level::kLow), not Hi-Z. TEST idles low
/// passively here anyway — schematic review (2026-06-07) found NO pull-up; the adapter
/// has a 10K pull-down and the MSP430 TEST pin has a weak internal PD. We still drive
/// it low actively so TEST is only ever high during the OnEnterTap activation glitch.
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
// Read-back: PB14 CANNOT be read directly (the host drive buffer is in the way —
// tried, does not work). The board provides a passive read-back instead: PB14
// (TMS_SWDO) and PB12 (SWD_IN) are the SAME connector node, with PB12 a voltage-
// level-translated COPY of it. PB12 faithfully follows PB14 while the probe drives,
// and follows whatever the TARGET drives once PB14 tri-states. So the turnaround is:
// tri-state PB14 (SbwCrhRelease) and sample PB12 — that is the only readable image
// of the bus. kSbwIdrBit = 12 (= SBWDIO_In::kPin_, PB12).
//
// Historical corruption (FR5739 coreip 0xc121≠0x1106; G2553 jtag_id 0x4d≠0x89) was
// NOT a pin or translator problem — the LA on PB14 showed the target driving the
// correct, identical waveform on good and bad runs. Root cause (2026-06-11): the
// JtagPending resolver called TapWaitTransfer(), which #if'd on OPT_HARD_SELECT_SBW_TMP
// (0 here) to JtagWaitTransfer() — a no-op when SBW is the RUNTIME-active transport —
// so GetResult read the IDR sample buffer while the SBW DMA was still in flight. Fixed
// in JtagPending.h (TapWaitTransfer now drains both idempotent waits). Enabling
// OPT_SBWDEV_DUMP_READ_PHASE masked it because the SWO dump let the DMA finish before
// the decode loop. See project_stlinkv2_sbw_readback memory.
//
// Over-voltage caveat: PB14 always drives 3.3 V and the translator is 3.3 V-
// referenced, so a sub-3 V MSP430 is unusable regardless (STLinkV2 is 3.3 V-only —
// see Hardware/STLinkV2 README).
// ---------------------------------------------------------------------------

/// PB12 (SWD_IN) — the passive level-translated copy of the PB14 bus node, and the
/// SBW read-back pin (PB14 itself is not directly readable). TimSbw derives the
/// sample port + bit from this alias (kSbwIdrBit = SBWDIO_In::kPin_ = 12).
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
	, ADC12_IN0				///< PA0 = target VCC/2 sense (ADC1_IN0)
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
/// JRST level while activating JTAG: RST driven HIGH. UIF holds RST high before
/// the entry sequence (no RST-low pre-pulse), so JtagOn must bring RST up — the
/// bare `JRST` (AnyOut default Level::kLow) would drive it low and force the
/// caller to fix it up after Setup(). See I2031_ACQUISITION_GOLDEN_REFERENCE.md.
using JRST_On = AnyOut<Port::PB, 0, Speed::kMedium, Level::kHigh>;
/// This configuration activates JTAG bus using bit-banging
using JtagOn2 = AnyPinGroup <Port::PB
	, JRST_On				///< JRST driven HIGH (no RST-low pre-pulse before entry)
	, JTEST					///< JTEST pin for bit bang access (old TRST)
	, JTMS_PWM				///< JTMS pin for PWM
>;

// Operate both ports for JtagOn
using JtagOn = PortMerge<JtagOn1, JtagOn2>;


/// === Init state (DriverOff): release the hardware buffers to Hi-Z so the
/// target's own pull-ups/-downs own the bus. Applied only by OnReleaseDriver()
/// (from OnClose()), never inside the acquisition retry loop. This is the old
/// "JtagOff" (Hi-Z) content; "JtagOff" now means the driven Close state below.
using DriverOff1 = AnyPinGroup <Port::PA
	, JTCK_Init				///< JTCK in Hi-Z
	, JTDO_Init				///< JTDO in Hi-Z
	, JTDI_Init				///< JTDI in Hi-Z
>;
using DriverOff2 = AnyPinGroup <Port::PB
	, JRST_Init				///< JRST in Hi-Z
	, JTEST_Init			///< JTEST in Hi-Z
	, JTMS_Init				///< JTMS in Hi-Z
>;
// Operate both ports for DriverOff
using DriverOff = PortMerge<DriverOff1, DriverOff2>;

/// === Close state (JtagOff): the buffers DRIVE the bus idle level (no Hi-Z,
/// buffers stay enabled) so weak/asymmetric target pull-ups/-downs cannot pulse
/// the lines between acquisition attempts. Applied by OnReleaseJtag().
/// Idle mapping: RST=high, TEST=low, TMS=low, TCK=high, TDI=high; TDO=input
/// (target-driven). NOTE: levels are bench-tunable for this PU/PD-marginal board.
using JRST_Close  = AnyOut<Port::PB, 0, Speed::kMedium, Level::kHigh>;	///< RST driven high
using JTEST_Close = AnyOut<Port::PB, 1, Speed::kMedium, Level::kLow>;	///< TEST driven low
using JtagOff1 = AnyPinGroup <Port::PA
	, JTCK					///< JTCK driven high (TCK=H)
	, JTDO_Init				///< JTDO input (target drives TDO)
	, JTDI					///< JTDI driven high (TDI=H)
>;
using JtagOff2 = AnyPinGroup <Port::PB
	, JRST_Close			///< JRST driven high (RST=H)
	, JTEST_Close			///< JTEST driven low (TEST=L)
	, JTMS					///< JTMS driven low (TMS=L)
>;

// Operate both ports for JtagOff (driven Close idle)
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
// (Single-pin / separate-CRH direction is now implicit in TimSbwSTLink — the old
//  kWaveSbwSeparateDirDma bool was dropped when the template split into
//  TimSbwSTLink (single-pin) + TimSbw (buffered placeholder).)

#if OPT_STARTUP == OPT_STARTUP_TIM_DMA_TIMING
// ── Timer→DMA latency probe resource bundle (util/TimDmaTiming.h) ──
// Reuses the SBW assignments above; the probe instantiation lives in main.cpp
// (needs WATCHPOINT/Trace, defined after platform.h). SBWCLK = TIM1_CH1N/PB13,
// SBWDIO pulsed on PB14, two frozen-compare triggers on CH2→DMA1_CH3 (A) and
// CH4→DMA1_CH4 (B) — A is the lower-numbered DMA channel, so it wins arbitration.
using TimDmaTimingClkPin = TIM1_CH1N_PB13<Mode::kAlternate, Speed::kFast>;
using TimDmaTimingDio    = SBWDIO;									///< PB14 (AnyOut, idle low)
static constexpr Timer::Unit    kTimDmaTimer       = kWaveSbwTimer;		///< TIM1
static constexpr Timer::Channel kTimDmaClkCh       = kWaveSbwClk;		///< CH1N (SBWCLK PWM)
static constexpr Timer::Channel kTimDmaTrigACh     = kWaveSbwDataTrig;	///< CH2 → DMA1_CH3
static constexpr Timer::Channel kTimDmaTrigBCh     = kWaveSbwSampleTrig;	///< CH4 → DMA1_CH4
static constexpr bool kTimDmaClkCmpComplementary   = kWaveSbwCmpComplementary;	///< CHN
#endif

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

#if OPT_JTAG_TCLK_IMPLEMENTATION == OPT_JTCLK_IMPL_SPI
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
/// SBW Init/teardown pin state (DriverOff equivalent): PB14 → Hi-Z, PA5 → input
/// pull-up. Applied by OnReleaseDriver() (from OnClose()) — the only Hi-Z release.
using SbwBusOffPins = PortMerge<
	  AnyPinGroup<Port::PA, JTCK_Init>		///< PA5 → input pull-up
	, AnyPinGroup<Port::PB, JTMS_Init>		///< PB14 → Hi-Z
>;
/// SBW Close pin state (driven idle): keep TEST/SBWCLK (PB13) low and DRIVE
/// SBWDIO (PB14) push-pull instead of Hi-Z, so the buffers hold the bus — not the
/// target's marginal pull-ups/-downs — between acquisition attempts. PA5 stays
/// input (shorted to the PB13 SBWCLK trace). Applied by OnReleaseJtag().
/// NOTE: driven levels are bench-tunable for this PU/PD-marginal board.
using SbwBusClosePins = PortMerge<
	  AnyPinGroup<Port::PA, JTCK_Init>		///< PA5 → input pull-up
	, AnyPinGroup<Port::PB
		, AnyOut<Port::PB, 13, Speed::kFast, Level::kLow>	///< PB13 → SBWTCK/TEST driven low
		, SBWDIO>							///< PB14 → push-pull output (driven idle)
>;

ALWAYS_INLINE void SbwBusOn()    { SbwBusOnPins::Setup(); }
ALWAYS_INLINE void SbwBusOff()   { SbwBusOffPins::Setup(); }
ALWAYS_INLINE void SbwBusClose() { SbwBusClosePins::Setup(); }
