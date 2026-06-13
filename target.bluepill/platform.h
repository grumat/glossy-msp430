/*!
\file bluepill-v2/platform.h
\brief Definitions specific for the Blue/Black Pill combined Development board

Pins:
	PA0:	VREF / 2
	PA1:	/RST: reset target MSP430
	PA2:	TXD of RS232 (to pin 12 of JTAG connector)
	PA3:	RXD of RS232 (to pin 14 of JTAG connector)
	PA4:	TEST on MSP430 debug port (pin 8 of JTAG connector)
	PA5:	TCK on MSP430 debug port (pin 7 of JTAG connector) / join with PA8
	PA6:	TDO input (from pin 1 of JTAG connector)
	PA7:	TDI output (to pin 3 of JTAG connector)
	PA8:	TCK on MSP430 debug port (pin 7 of JTAG connector) / join with PA5
			(SPI mode: TIM1_CH1 input, drives TIM1 from the SCK trace.
			 DTRIG mode: unused — TIM1 uses its internal clock.)
	PA9:	Spi-Bi_wire output control (0: out; 1: in)
	PA10:	TMS on MSP430 debug port (pin 5 of JTAG connector)
			Always TIM1_CH3 alt-function while a JTAG frame is in flight; the
			pulse-shape (SPI mode) or toggle waveform (DTRIG mode) is generated
			by the same timer channel.
	PA11:	USB-
	PA12:	USB+
	PA13:	TMS DEBUG Host 
	PA14:	TCK DEBUG Host 
	PA15:	TDI DEBUG Host 
	
	PB3:	TRACESWO Host (firmware uses SWO trace function!)
	PB6:	TXD GDB (until USB support is implemented)
	PB7:	RXD GDB (until USB support is implemented)
	PB8:	Target Voltage PWM regulator
	PB12:	Enables TEST, TCK and RST outputs (0: on, 1: tri-state)
	PB13:	Enables TMS and TDI outputs (0: on, 1: tri-state)
	PB14:	Enables TXD output on JTAG connector (0: on, 1: tri-state)

	PC13:	Leds: 0: green; 1: red; tri-state: off

SBW specific bus pins:
	PA4:	TEST on MSP430 debug port, pin 8 of JTAG connector. Target boards have this pin directly 
			connected to the SBWTCK/TEST pin. This is a dual function pin. TEST pin to issue "enter SBW" 
			sequence and TCK (PA5/PA8) to use as clock input when SBW was acquired. A 220R~330R resistor 
			joins them together, so TEST always wins. For the TCK function, TEST needs to enter 
			tri-state.
	PA5:	TCK on MSP430 debug port (pin 7 of JTAG connector) / join with PA8 / see PA4
	PA8:	TCK on MSP430 debug port (pin 7 of JTAG connector) / join with PA5 / see PA4
	PA6:	TDO input (from pin 1 of JTAG connector) / Joins to /RST/NMI/SBWTDIO of target / see PA9
	PA7:	TDI output (to pin 3 of JTAG connector) / Joins to TDO depending on PA9
	PA9:	Spi-Bi_wire output control: 0: joins PA7 to TDO on target; 1: Transfers TDO target into PA6

JTAG14 connector Variations:

| Pin | Old JTAG | MSP-FET  | Glossy JTAG |  TI SBW  | Glossy SBW | Olimex SBW |
|:---:|:--------:|:--------:|:-----------:|:--------:|:------  --:|:----------:|
|  1  |   TDO    |   TDO    |     TDO     |  SBWDIO  |   SBWDIO   |            |
|  2  |   VCC    |   VCC    |     VCC     |    VCC   |     VCC    |     VCC    |
|  3  |   TDI    |   TDI    |     TDI     |          |            |            |
|  4  |   Vref   |   Vref   |     Vref    |   Vref   |    Vref    |    Vref    |
|  5  |   TMS    |   TMS    |     TMS     |          |            |            |
|  6  |          |          |             |          |            |            |
|  7  |   TCK    |   TCK    |     TCK     | SBTCK(*) |  SBTCK(*)  |            |
|  8  | TEST/VPP | TEST/VPP |     TEST    | TEST/VPP |    TEST    |   SBTCK    |
|  9  |   GND    |   GND    |     GND     |   GND    |    GND     |    GND     |
| 10  |          | UART_CTS |             | UART_CTS |            |            |
| 11  |   TRST   |   TRST   |     TRST    |   TRST   |    TRST    |   SBWDIO   |
| 12  |          | UART_TXD |   UART_TXD  | UART_TXD |  UART_TXD  |            |
| 13  |          | UART_RTS |     GND     | UART_RTS |    GND     |            |
| 14  |          | UART_RXD |   UART_RXD  | UART_RXD |  UART_RXD  |            |

(*) Joins with 330R to TEST pin
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
/// **HARDWARE LIMITATION — SBW is BROKEN on this board.** The output-buffer
/// enable ENA1N (PB12) gangs TEST, TCK, and RST together: a single enable
/// turns all three on, with no way to selectively tri-state TEST. On the
/// JTAG connector, the probe's TEST line (PA4) and TCK line (PA5/PA8) are
/// joined through a 220-330R resistor at the target — "TEST always wins."
/// For SBW the probe must drive SBWTCK out of the TCK pad while leaving the
/// TEST pad tri-stated, which the ganged ENA1N forbids. Bench evidence:
/// SBW entry pulses are visible on TEST/TRST, then no SBWCLK reaches the
/// target. A PCB cut/jumper would be needed to separate TEST's enable from
/// TCK/RST's enable. See .claude/docs/drivers/TIM_SBW_DRIVER.md
/// "Board hardware requirements for SBW".
///
/// SBW is therefore compiled OUT here. (It never actually built anyway: the
/// shared bit-bang entry in SbwDev.cpp uses the STLinkV2-only SBWTEST_Bb/
/// SBWRST_Bb pins and assumes no direction mux, whereas this board routes
/// SBWDIO through the PA9 mux — so SBW needs real bring-up, not just aliases,
/// before it can be re-enabled.)
#define OPT_SBW_IMPLEMENTATION		OPT_SBW_IMPL_OFF

/// FROZEN: do not hard-select SBW on this board — see the hardware-limitation
/// note above. SBW work moved to STLinkV2 (target.stlinv2/platform.h) where
/// TEST (PB1) and TCK (PA5/PB13) have independent driver enables.
//#define OPT_HARD_SELECT_SBW_TMP		1

/// JTCLK generation strategy. SPI is the only supported generator — natural pair
/// with DTRIG (the same SPI MOSI carries the burst, avoiding a fight over the F1
/// alt-function mux on PA7).
#define OPT_JTAG_TCLK_IMPLEMENTATION	OPT_JTCLK_IMPL_SPI

/// Implementation for "GDB serial port" (USART used provisory until USB VCP is added to firmware)
#define OPT_GDB_IMPLEMENTATION			OPT_GDB_IMPL_USART1


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

/* SBW wire-rate grades. Ceiling is target-RC-bound (RST/SBWDIO reset cap), not
// the MSP430 5 MHz spec — see the matching note in target.stlinv2/platform.h.
// Floor is the flash timing generator: f_FTG must stay in 257-476 kHz, so the
// slowest grade can't drop below ~275 kHz (the future flash-write path derives
// its clock from the link). Frequencies are nominal; integer-PSC rounding is
// irrelevant for SBW. Values mirror target.stlinv2 for now — fine-tune per board. */
static constexpr uint32_t SBW_Speed_5 = 1300000UL;	///< top — good/short cabling only
static constexpr uint32_t SBW_Speed_4 = 1100000UL;	///< safe default top speed
static constexpr uint32_t SBW_Speed_3 = 900000UL;
static constexpr uint32_t SBW_Speed_2 = 600000UL;
static constexpr uint32_t SBW_Speed_1 = 300000UL;	///< slowest: can't go below 275 kHz (min flash clock!)

// ── Target voltage (#46 PASS 2) ──────────────────────────────────────────────
// SENSE: PA0 carries the target VCC through a /2 divider (board "VREF/2") into
// ADC1_IN0. DRIVE: PWM_VT on TIM4_CH4/PB9 into a 47k/10uF RC regulator.
#define OPT_TARGET_HAS_VSENSE	1
#define OPT_TARGET_HAS_VDRIVE	1
static constexpr uint32_t kVtgMax_mV    = 3300;		///< output at 100% PWM duty
static constexpr uint32_t kVtgAdcRef_mV = 3300;		///< ADC full-scale reference (VDDA)
static constexpr uint32_t kVtgSenseMul  = 2;		///< PA0 = VCC/2 → multiply reading by 2
static constexpr uint32_t kVtgDefault_mV = 3000;	///< UIF-style default supply on connect
using VSenseAdc = Adc::AnySetup<Adc::AnyConfig,
	Adc::AnySequence<Adc::Chan<Adc::Unit::k1, 0>>>;
// PWM regulator: TIM4_CH4 free-running, period = kVtgMax_mV so CCR = millivolts
// (1:1). Carrier ~22 kHz (≫ the 47k/10uF filter fc ~0.34 Hz → ripple negligible).
using VtgPwmTimer = Timer::Any<
	Timer::InternalClock_Hz<Timer::kTim4, SysClk, 20000u * kVtgMax_mV>,
	Timer::Mode::kUpCounter, kVtgMax_mV - 1, /*buffered=*/true>;
using VtgPwm = Timer::AnyOutputChannel<VtgPwmTimer, Timer::Channel::k4,
	Timer::OutMode::kPWM1, Timer::Output::kEnabled, Timer::Output::kDisabled,
	/*preload=*/true>;


/// DTRIG drives TIM1 from its internal clock (APB2 × multiplier), so PA8 is freed.
using TmsShapeGpioIn = Unchanged<8>;

/// Dedicated pin for write JTMS
using JTMS = AnyOut<Port::PA, 10>;
/// Logic state for JTMS pin initialization
using JTMS_Init = AnyInPd<Port::PA, 10>;
/// PA10 as TIM1_CH3 alt-function (drives JTMS during JTAG frame generation in DTRIG mode)
using JTMS_PWM = TIM1_CH3_PA10_OUT;

/// Pin for JTCK output
using JTCK = AnyOut<Port::PA, 5, Speed::kFast, Level::kHigh>;
/// Logic state for JTCK pin initialization
using JTCK_Init = AnyInPu<Port::PA, 5>;
/// JTCK driven by SPI1_SCK during frames (PA5)
using JTCK_SPI = SPI1_SCK_PA5;

/// Pin for JTDO input (output on MCU)
using JTDO = AnyInPu<Port::PA, 6>;
/// Logic state for JTDO pin initialization
using JTDO_Init = AnyInPu<Port::PA, 6>;
/// JTDO captured by SPI1_MISO during frames (PA6)
using JTDO_SPI = SPI1_MISO_PA6;

/// Pin for JTDI output (input on MCU)
using JTDI = AnyOut<Port::PA, 7, Speed::kFast, Level::kHigh>;
/// Logic state for JTDI pin initialization
using JTDI_Init = AnyInPd<Port::PA, 7>;

/// JTDI during run/idle state produces JTCLK
using JTCLK = JTDI;
/// JTDI driven by SPI1_MOSI during frames (PA7); same pin doubles as JTCLK in RTI
using JTDI_SPI = SPI1_MOSI_PA7;
using JTCLK_SPI = JTDI_SPI;

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

/// Pin for SBWO Enable control. PA9 is the hardware mux that selects which
/// physical pin drives the SBWDIO trace: high = output path (SPI1_MOSI/PA7),
/// low = input path (SPI1_MISO/PA6).
using SBWO = AnyOut<Port::PA, 9, Speed::kSlow, Level::kHigh>;

/// SBW direction-flip policy for Bluepill — buffered/optimized variant.
/// The direction-script DMA writes one BSRR word per phase boundary to
/// GPIOA->BSRR; only the PA9 bit toggles. See "DirPolicy contract" in
/// .claude/docs/drivers/TIM_SBW_DRIVER.md.
struct DirPolicy_PA9_BsrrMux
{
	static constexpr unsigned kWordsPerFlip = 1;
	static void Init() {}			///< no-op — both arrays are constexpr
	static const uint32_t* DriveOutput()
	{
		static constexpr uint32_t v[1] = { 1u << 9 };			///< BSRR set PA9 → mux→OUT
		return v;
	}
	static const uint32_t* DriveInput()
	{
		static constexpr uint32_t v[1] = { 1u << (9 + 16) };	///< BSRR reset PA9 → mux→IN
		return v;
	}
	static volatile uint32_t* DirRegister() { return &GPIOA->BSRR; }
};

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

/// Target-voltage PWM regulator on PB9 / TIM4_CH4 (not PB8/TIM4_CH3) to stay
/// pin-consistent with the G431 target, where PB8 = BOOT0 — a pull-up at the
/// regulator input there would force the chip into the system bootloader (DFU)
/// at every reset. Same TIM4 unit, channel 4.
using PWM_VT_0V = AnyOut<Port::PB, 9, Speed::kSlow, Level::kLow>;
using PWM_VT_3V3 = AnyOut<Port::PB, 9, Speed::kSlow, Level::kHigh>;
using PWM_VT = TIM4_CH4_PB9_OUT;

/// Initial configuration for PORTA
using PORTA = AnyPortSetup <Port::PA
	, ADC12_IN0				///< PA0 = target VCC/2 sense (ADC1_IN0)
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
	, Unused<8>				///< not used (PWM_VT moved to PB9 for G431 BOOT0 parity)
	, PWM_VT_0V				///< Target power OFF at boot (UIF-style); TargetPower drives TIM4_CH4
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

/// All GPIO ports collected for one-shot initialization at startup
using AllGpioStartup = PortMerge<PORTA, PORTB, PORTC, PORTD>;


/// JRST driven HIGH while activating JTAG. UIF holds RST high before the entry
/// sequence (no RST-low pre-pulse), so JtagOn must bring RST up — the bare `JRST`
/// (AnyOut default Level::kLow) would drive it low. See
/// I2031_ACQUISITION_GOLDEN_REFERENCE.md.
using JRST_On = AnyOut<Port::PA, 1, Speed::kFast, Level::kHigh>;
/// JTAG-bus active config for DTRIG: JTMS goes to TIM1_CH3 alt-function so the
/// timer can drive the TMS waveform; JTCK/JTDO/JTDI stay as GPIO until
/// JtagSpiOn flips them to SPI mode.
using JtagOn = AnyPinGroup <Port::PA
	, JRST_On				///< JRST driven HIGH (no RST-low pre-pulse before entry)
	, JTEST					///< JTEST pin for bit bang access
	, JTCK					///< JTCK pin for bit bang access
	, JTDO					///< JTDO pin for bit bang access
	, JTDI					///< JTDI pin for bit bang access
	, Unchanged<8>			///< PA8 unused (TIM1 internal clock)
	, JTMS_PWM				///< PA10 → TIM1_CH3 alt-function for JTMS
>;

/// === Init state (DriverOff): release pins to Hi-Z; the target's pull-ups/-downs
/// own the bus. Applied only by OnReleaseDriver() (from OnClose()). This BluePill
/// has no external buffers, so Init and Close are nearly identical electrically;
/// the concept is applied for portability. This is the old "JtagOff" (Hi-Z) set.
using DriverOff = AnyPinGroup <Port::PA
	, JRST_Init				///< JRST in Hi-Z
	, JTEST_Init			///< JTEST in Hi-Z
	, JTCK_Init				///< JTCK in Hi-Z
	, JTDO_Init				///< JTDO in Hi-Z
	, JTDI_Init				///< JTDI in Hi-Z
	, TmsShapeGpioIn		///< Keep as input
	, JTMS_Init				///< JTMS in Hi-Z
>;

/// === Close state (JtagOff): DRIVE the bus idle level instead of Hi-Z, so the
/// resting level is held by the MCU outputs, not the target's pull-ups/-downs,
/// between acquisition attempts. Applied by OnReleaseJtag().
/// Idle mapping: RST=high, TEST=low, TMS=low, TCK=high, TDI=high; TDO=input.
using JRST_Close  = AnyOut<Port::PA, 1, Speed::kFast, Level::kHigh>;		///< RST driven high
using JTEST_Close = AnyOut<Port::PA, 4, Speed::kFast, Level::kLow>;		///< TEST driven low
using JTMS_Close  = AnyOut<Port::PA, 10, Speed::kFast, Level::kLow>;		///< TMS driven low
using JtagOff = AnyPinGroup <Port::PA
	, JRST_Close			///< JRST driven high (RST=H)
	, JTEST_Close			///< JTEST driven low (TEST=L)
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
/// TIM1_CH3 → PA10 (JTMS). On bluepill the TMS pin sits on a regular CH (not CHN),
/// unlike STLinkV2's CH2N → PB14; kWaveJtagTmsCmpComplementary below selects the
/// non-complementary path inside DtrigJtag.
static constexpr Timer::Channel kWaveJtagTms     = Timer::Channel::k3;	// PWM → CH3 → PA10
/// CH4 compare-only: CC4 DMA (DMA1_CH4) reloads the TMS CCR at end of entry
/// pulse. CH4/DMA1_CH4 is chosen because SPI1_RX uses DMA1_CH2 and SPI1_TX
/// uses DMA1_CH3; CH1 is reserved for the TIM1 internal clock and CH2's
/// DMA (DMA1_CH3) collides with SPI1_TX.
static constexpr Timer::Channel kWaveJtagTmsRld1 = Timer::Channel::k4;	// entry→shift CCR reload
/// JTMS sits on TIM1_CH3 (PA10) — the regular output, not CHN. Tells DtrigJtag to
/// drive the main CH output via Output::kInverted (CCxP=1) + PWM1; on this F103
/// silicon that combination empirically produces TMS=HIGH for the entry pulse and
/// LOW for the shift portion. See the TmsOut comment in DtrigJtag.h.
static constexpr bool kWaveJtagTmsCmpComplementary = false;

// ── SBW channel assignments (TimSbw) ────────────────────────────────────────
/// TIM1 is the master timer for SBW too. JTAG and SBW are mutually exclusive.
static constexpr Timer::Unit kWaveSbwTimer = Timer::kTim1;
/// SBWCLK on TIM1_CH1 → PA8 (regular CH, not CHN). PA8 is bridged to PA5 on
/// the bluepill PCB, so driving TIM1_CH1 produces SBWCLK on the SBWDIO trace.
static constexpr Timer::Channel kWaveSbwClk = Timer::Channel::k1;
/// CH2 frozen-compare → TIM1_CH2 DMA request → DMA1_CH3 fires the composite
/// data+direction BSRR DMA. (PA9 stays GPIO; CH2 has no pin output in SBW mode.)
static constexpr Timer::Channel kWaveSbwDataTrig = Timer::Channel::k2;
/// CH4 frozen-compare → TIM1_CH4 DMA request → DMA1_CH4 fires the IDR sample DMA.
static constexpr Timer::Channel kWaveSbwSampleTrig = Timer::Channel::k4;
/// PA8 is a regular CH, not CHN, so TimSbw drives it via the non-complementary
/// (PWM1) path — same convention as kWaveJtagTmsCmpComplementary on this board.
static constexpr bool kWaveSbwCmpComplementary = false;

// ── SBW platform contract (consumed by SbwDev.tim.cpp) ───────────────────────
// The read-back GPIO port and bit are derived by TimSbw straight from SBWDIO_In
// (= JTDO = AnyInPu<PA,6>): kSbwIdrBit = SBWDIO_In::kPin_ and the sampled IDR is
// SBWDIO_In::Io().IDR. No separate SbwIdrPort/kSbwIdrBit aliases — a single
// source of truth for the read pin (mirrors target.stlinv2).
/// Buffered board: direction is folded into the data BSRR DMA via the PA9 mux.
using SbwDirPolicy = DirPolicy_PA9_BsrrMux;
static constexpr bool kWaveSbwSeparateDirDma = false;
/// Unused on the buffered path (no separate direction DMA); any free channel.
static constexpr Timer::Channel kWaveSbwDirTrig = Timer::Channel::k3;

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

// Controls if bus controls the MCU (tri-state or one of the controlled states)
ALWAYS_INLINE void SetBusState(const BusState st)
{
	// Hardware uses negative logic control lines
	DEBUG_BUS_CTRL::WriteComplement((uint32_t)st);
}


// SBW pin bring-up hooks (called only by SbwDev). The buffered board drives the
// SBWDIO direction through the PA9 mux folded into the data BSRR DMA and has no
// pin shorted to the SBWCLK trace to release, so there is nothing extra to do
// here — the PA-side SBW pins are configured by TimSbw::Init().
ALWAYS_INLINE void SbwBusOn() { }
ALWAYS_INLINE void SbwBusOff() { }
// No external buffers on this board, so the driven Close state has nothing extra
// to manage (same as SbwBusOn/Off) — kept for the three-state interface contract.
ALWAYS_INLINE void SbwBusClose() { }


/*!
\brief Single source of truth for which MCU peripherals this firmware owns.

`SystemInit()` calls `PeripheralEnabler::Init()` once at boot — this enables
clocks for every listed peripheral and pulses APBxRSTR / AHBRSTR for the ones
that have a reset bit. Per-class `Init()` calls are unnecessary (and deprecated).
*/
using PeripheralEnabler = Clocks::Enabler<
	// GPIO ports
	PortClock<Port::PA>,
	PortClock<Port::PB>,
	PortClock<Port::PC>,
	PortClock<Port::PD>,
	// AFIO — required for SWO trace pin remap
	Afio,
	// DMA1 — covers all channels (CH2 SPI RX, CH3 SPI TX, CH4 TIM1_CH4 TMS reload)
	Dma::Controller<Dma::Itf::k1>,
	// Timers used by the firmware
	Timer::TimerDescriptor<Timer::kTim1>,	// DTRIG TMS toggle (advanced timer; CH3 → PA10 = JTMS)
	Timer::TimerDescriptor<Timer::kTim2>,	// (reserved — see comment below)
	Timer::TimerDescriptor<Timer::kTim4>,	// target-voltage PWM regulator (CH4 → PB9)
	// TIM3 is unused (JTCLK is SPI-generated, OPT_JTCLK_IMPL_SPI), so it is not
	// clock-enabled here.
	// SPI1 carries JTCK/JTDO/JTDI in DTRIG mode
	Spi::Hardware<Spi::Iface::k1>,
	// USART1 — provisional GDB serial
	UsartHardware<Usart::k1>
>;
