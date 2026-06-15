#pragma once

#define STDPROJ_H__INCLUDED__

#ifndef __cplusplus
	#error Project requires a C++ compiler
#endif
#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
	#error Project requires a little endian MCU
#endif


#include <bmt.h>

//! Firmware version string, reported by the GDB monitor "version" command and
//! the startup banner. Bump on release.
#define GLOSSY_FW_VERSION		"0.2.0-dev"

//#define OPT_IMPLEMENT_TEST_DB

// Project settings should select the correct `platform.h` file path
#include <platform.h>

#ifndef OPT_JTAG_IMPLEMENTATION
	#error OPT_JTAG_IMPLEMENTATION definition is required for every platform
#endif

/*
DTRIG is the only JTAG transport variant. The legacy SPI (OPT_JTAG_IMPL_SPI /
SPI_DMA) and TIM_DMA (OPT_JTAG_IMPL_TIM_DMA / TIM_DMA_SLOW) backends were
removed — see .claude/docs/drivers/SPI_VARIANT_REMOVED.md and
.claude/docs/drivers/TIM_VARIANT_REMOVED.md for the last-working git refs and
the architectural reasons.

Default values controlled by this block:
  OPT_INCLUDE_JTAG_DTRIG_  enables JtagDev.dtrig.cpp (mandatory; only choice)
  OPT_BUFFER_LAYOUT_       picks ping-pong layout (PAIR is the only one needed
                           by DTRIG; TRIPLE was used by the removed TIM_DMA_SLOW)
  OPT_BUFFER_CNT_          element count shared by every ping-pong sub-buffer
  OPT_JTAG_SPEED_SEL_      enables runtime speed-grade selection
*/


// ── Ping-pong buffer layout selectors ────────────────────────────────────────
// All sub-buffers share the same element count (OPT_BUFFER_CNT_) and advance
// in lockstep via a single Step() — see util/PingPongBuffer.h.
#define OPT_BUFFER_LAYOUT_PAIR		2	///< TX + RX (DTRIG)
#define OPT_BUFFER_LAYOUT_TRIPLE	3	///< TX + RX + AUX (kept for future ports)


#if OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_DTRIG
	#define OPT_INCLUDE_JTAG_DTRIG_		1
	// TMS is driven by TIM1 hardware toggle, not per-bit DMA → no AUX needed
	#define OPT_BUFFER_LAYOUT_			OPT_BUFFER_LAYOUT_PAIR
	#define OPT_BUFFER_CNT_				8
	#define OPT_JTAG_SPEED_SEL_			1
	using FrameBufEleType = uint8_t;
#else
	#error OPT_JTAG_IMPLEMENTATION must be OPT_JTAG_IMPL_DTRIG (the legacy SPI / TIM_DMA variants were removed - see .claude/docs/drivers/)
#endif

#ifndef OPT_INCLUDE_JTAG_DTRIG_
	#define OPT_INCLUDE_JTAG_DTRIG_		0
#endif

// SBW transport selection (independent of JTAG; only one of the two is brought
// up at runtime — see "Init() is sovereign" in TIM_SBW_DRIVER.md).
#ifndef OPT_SBW_IMPLEMENTATION
	#define OPT_SBW_IMPLEMENTATION		OPT_SBW_IMPL_OFF
#endif

#if OPT_SBW_IMPLEMENTATION == OPT_SBW_IMPL_TIM
	#define OPT_INCLUDE_SBW_TIM_		1
	// SBW renders BSRR + IDR scripts as uint32_t words; ping-pong holds
	// 3 × kJtagBitsMax = 3 × (5 + 32 + 1) ≈ 120 cycles for a 32-bit DR scan.
	#define OPT_SBW_BUFFER_CNT_			128
#endif

#ifndef OPT_INCLUDE_SBW_TIM_
	#define OPT_INCLUDE_SBW_TIM_		0
#endif

#if OPT_INCLUDE_SBW_TIM_ && !defined(OPT_SBW_BUFFER_CNT_)
	#error OPT_SBW_BUFFER_CNT_ must be set when the SBW timdma backend is enabled
#endif

// ── Temporary protocol selector ─────────────────────────────────────────────
// TODO: remove once the GDB monitor / qRcmd interface lets the host pick the
// transport at runtime. Today TapMcu::Open() needs to commit to exactly one
// ITapInterface instance at build time; this macro lets a target's platform.h
// pin that choice when both JTAG and SBW backends are compiled in. Has no
// effect when only one backend is enabled.
//
//   0 (default) → use JtagDev if compiled in, otherwise SbwDev
//   1           → force SbwDev (requires OPT_INCLUDE_SBW_TIM_)
#ifndef OPT_HARD_SELECT_SBW_TMP
	#define OPT_HARD_SELECT_SBW_TMP		0
#endif

#if OPT_HARD_SELECT_SBW_TMP && !OPT_INCLUDE_SBW_TIM_
	#error OPT_HARD_SELECT_SBW_TMP=1 requires OPT_SBW_IMPLEMENTATION to be set to an active SBW backend
#endif

// Bench diagnostic: dump the SBW read-phase IDR sample buffer over TRACESWO on
// every scan (SbwDev::DumpReadPhase). Off by default; only meaningful when the
// SBW timdma backend is compiled in.
#ifndef OPT_SBWDEV_DUMP_READ_PHASE
	#define OPT_SBWDEV_DUMP_READ_PHASE	0
#endif

// TDO read-settle sweep parameters (mode = OPT_STARTUP_SBW_TDO_SETTLE; selected
// via OPT_STARTUP in the Startup section below). The probe sweeps the TDO sample
// compare across the SBWCLK fall while reading the JTAG ID to measure T_settle —
// see .claude/docs/drivers/SBW_SPEED_TIMING_MODEL.md "Measuring T_settle". Tuning:
// Ticks per wire-cycle for the sweep instance — large for fine resolution
// (tick = 1/(mult*freq); 64@300kHz ~ 52 ns/step). timer clock = mult*freq.
#ifndef OPT_SBW_TDO_SETTLE_MULT
	#define OPT_SBW_TDO_SETTLE_MULT		64
#endif
// Low wire frequency for the sweep → a long low phase to walk across.
#ifndef OPT_SBW_TDO_SETTLE_FREQ
	#define OPT_SBW_TDO_SETTLE_FREQ		300000UL
#endif
// Reps per phase (consistency = stable-settled; the spread ~ the latency jitter).
#ifndef OPT_SBW_TDO_SETTLE_REPS
	#define OPT_SBW_TDO_SETTLE_REPS		32
#endif
// Measured compare->DMA->IDR latency band (ns) from OPT_TEST_TIM_DMA_TIMING.
// The sweep adds these to the compare offset to report the effective sample
// instant; re-measure per MCU. (GD32F103: 135..180 ns.)
#ifndef OPT_SBW_DMA_LAT_MIN_NS
	#define OPT_SBW_DMA_LAT_MIN_NS		135
#endif
#ifndef OPT_SBW_DMA_LAT_MAX_NS
	#define OPT_SBW_DMA_LAT_MAX_NS		180
#endif

// MagicPattern (0xA55A -> JTAG mailbox) acquisition fallback in
// TapMcu::InitDevice. Reached only when the normal RST-high entry returns an
// invalid JTAG-ID; it holds the device in reset and feeds the mailbox so a
// blank/running part stays halted under JTAG instead of dropping into LPM4
// (issues #19/#20). UNPROVEN: there is no confirmed bench case that exercises
// this path successfully — FR5994 takes the fast path and never arms it, and
// the RST-low re-entry de-latches the in-bring-up single-pin SBW transport.
// Off by default until a real acquisition case validates it.
#ifndef OPT_MSP430_MAGIC_PATTERN_ACQ
	#define OPT_MSP430_MAGIC_PATTERN_ACQ	0
#endif

// ════════════════════════════════════════════════════════════════════════════
// Startup mode — the SINGLE switch for what the firmware does at power-up.
//
// Set OPT_STARTUP in the target's platform.h to exactly one value below. Every
// mutually-exclusive boot behaviour lives here; the per-feature activation flags
// (OPT_BARE_RUN / OPT_TEST_WITH_LOGIC_ANALYZER / OPT_TEST_TIM_DMA_TIMING /
// OPT_SBW_TDO_SETTLE_SWEEP) are DERIVED from it below — do NOT set them directly.
// Mode parameters (mult / freq / reps / channel-swap) stay as separate tuning
// knobs (above for the settle sweep, below for the TIM→DMA probe). The
// OPT_STARTUP_* values are defined in platform-defs.h (so platform.h's own probe
// bundles can guard on them); this section supplies the default + derivations.
// ════════════════════════════════════════════════════════════════════════════
#ifndef OPT_STARTUP
	#define OPT_STARTUP				OPT_STARTUP_GDB
#endif

// ── Derived activation flags (do not set these directly — pick OPT_STARTUP) ──
#define OPT_BARE_RUN_GDB	0
#define OPT_BARE_RUN_JTAG	1
#define OPT_BARE_RUN_SBW	2
#if   (OPT_STARTUP == OPT_STARTUP_DETECT_JTAG) || (OPT_STARTUP == OPT_STARTUP_LA_WAVEFORM)
	#define OPT_BARE_RUN	OPT_BARE_RUN_JTAG	// LA waveform fires from JtagDev::OnOpen on the autonomous open
#elif (OPT_STARTUP == OPT_STARTUP_DETECT_SBW) || (OPT_STARTUP == OPT_STARTUP_SBW_TDO_SETTLE) || (OPT_STARTUP == OPT_STARTUP_SBW_LA_WAVEFORM)
	#define OPT_BARE_RUN	OPT_BARE_RUN_SBW	// the settle sweep + SBW LA waveform need the autonomous SBW connect
#else
	// GDB (normal) and TIM_DMA_TIMING (its probe runs from main() before the serve loop)
	#define OPT_BARE_RUN	OPT_BARE_RUN_GDB
#endif

#define OPT_TEST_WITH_LOGIC_ANALYZER	(OPT_STARTUP == OPT_STARTUP_LA_WAVEFORM)
#define OPT_SBW_TDO_SETTLE_SWEEP		(OPT_STARTUP == OPT_STARTUP_SBW_TDO_SETTLE)
#define OPT_SBW_TEST_WITH_LOGIC_ANALYZER	(OPT_STARTUP == OPT_STARTUP_SBW_LA_WAVEFORM)
#if OPT_STARTUP == OPT_STARTUP_TIM_DMA_TIMING
	#ifndef OPT_TIM_DMA_SWAP
		#define OPT_TIM_DMA_SWAP	0	///< 0 = normal DMA channel order, 1 = swapped (the priority experiment)
	#endif
	#define OPT_TEST_TIM_DMA_TIMING		(OPT_TIM_DMA_SWAP ? 2 : 1)
#else
	#define OPT_TEST_TIM_DMA_TIMING		0
#endif

// Guard: the SBW boot modes need an SBW driver compiled in.
#if ((OPT_STARTUP == OPT_STARTUP_DETECT_SBW) || (OPT_STARTUP == OPT_STARTUP_SBW_TDO_SETTLE) || (OPT_STARTUP == OPT_STARTUP_SBW_LA_WAVEFORM)) && !OPT_INCLUDE_SBW_TIM_
	#error OPT_STARTUP SBW mode requires an active SBW backend (OPT_SBW_IMPLEMENTATION)
#endif

// SBW LA-waveform (mode OPT_STARTUP_SBW_LA_WAVEFORM) flash TCLK-strobe pulse count.
// ≤127 stays a single one-shot chunk; set >127 to also exercise the chunk-boundary
// path. 35 mirrors a real Gen1/Gen2 word-write strobe count (TapDev430.cpp).
#ifndef OPT_SBW_LA_FLASH_PULSES
	#define OPT_SBW_LA_FLASH_PULSES		35
#endif

// Target-voltage capabilities (#46 PASS 2). A target's platform.h sets these to
// 1 and provides the ADC/PWM wiring; default off so other targets build with the
// "power" command reporting no sense / fixed supply.
#ifndef OPT_TARGET_HAS_VSENSE
	#define OPT_TARGET_HAS_VSENSE	0	///< can measure target voltage (ADC)
#endif
#ifndef OPT_TARGET_HAS_VDRIVE
	#define OPT_TARGET_HAS_VDRIVE	0	///< can drive a variable target supply
#endif

#ifndef OPT_BUFFER_LAYOUT_
	#error OPT_BUFFER_LAYOUT_ must be set by the active JTAG implementation block
#endif
#ifndef OPT_BUFFER_CNT_
	#error OPT_BUFFER_CNT_ must be set by the active JTAG implementation block
#endif

// Defaults to fixed JTAG speed
#ifndef OPT_JTAG_SPEED_SEL_
	#define OPT_JTAG_SPEED_SEL_			0
#endif

#ifndef OPT_JTAG_TCLK_IMPLEMENTATION
	#error Platform.h has to specify the OPT_JTAG_TCLK_IMPLEMENTATION option value
#endif

#ifndef OPT_GDB_IMPLEMENTATION
	#error Platform.h has to specify the OPT_GDB_IMPLEMENTATION option value
#endif

// OPT_TEST_WITH_LOGIC_ANALYZER (mode OPT_STARTUP_LA_WAVEFORM → JtagDev::
// DoLogicAnalyzerTest from OnOpen) and OPT_TEST_TIM_DMA_TIMING (mode
// OPT_STARTUP_TIM_DMA_TIMING → the driver-decoupled timer→DMA latency probe in
// main(), 1=normal/2=swapped via OPT_TIM_DMA_SWAP) are DERIVED in the Startup
// section above. Refs: Firmware.shared/util/TimDmaTiming.h,
// .claude/docs/drivers/{TIM_DMA_TIMING_PROBE,SBW_SPEED_TIMING_MODEL}.md.
//
/// Timer ticks per SBWCLK wire-cycle for the probe (intra-cycle phase resolution).
/// 8 mirrors TimSbw::kTimerMultiplier_; raise to 16/32 for a finer time slice.
/// Must be even (the PWM 50 % duty / falling-edge anchor sits at mult/2).
#ifndef OPT_TEST_TIM_DMA_MULT
	#define OPT_TEST_TIM_DMA_MULT			8
#endif


/// A stop watch object
using StopWatch = Timer::MicroStopWatch<TickTimer>;

#if OPT_GDB_IMPLEMENTATION != OPT_GDB_IMPL_VCP

#if OPT_GDB_IMPLEMENTATION == OPT_GDB_IMPL_USART1
/// USART1 for GDB port
using UsartGdbSettings = UsartTemplate<Usart::k1, SysClk, 115200>;
#elif OPT_GDB_IMPLEMENTATION == OPT_GDB_IMPL_USART2
/// USART2 for GDB port
using UsartGdbSettings = UsartTemplate<Usart::k2, SysClk, 115200>;
#elif OPT_GDB_IMPLEMENTATION == OPT_GDB_IMPL_USART3
/// USART3 for GDB port
using UsartGdbSettings = UsartTemplate<Usart::k3, SysClk, 115200>;
#endif

/// Defines a dual FIFO buffer for GDB UART port
using UsartGdbBuffer = UartFifo<UsartGdbSettings, 256, 64>;
/// The UART driver using interrupts
using UsartGdbDriver = UsartIntDriverModel<UsartGdbBuffer>;
/// Singleton for the GDB UART
extern UsartGdbDriver gUartGdb;
#endif // OPT_GDB_IMPLEMENTATION != OPT_GDB_IMPL_VCP

using Trace_ = SwoChannel<0>;
using Error_ = SwoChannel<1>;
#if DEBUG
using Debug_ = SwoChannel<2>;
#else
using Debug_ = SwoDummyChannel;
#endif
using SwoTrace = SwoTraceSetup <SysClk, SwoProtocol::kAsynchronous, 720000, Trace_, Error_, Debug_>;
// A stream object for the trace output
typedef OutStream<Trace_> Trace;
typedef OutStream<Error_> Error;
typedef OutStream<Debug_> Debug;

#ifdef DEBUG
#	define WATCHPOINT()		__NOP()
#else
#	define WATCHPOINT()
#endif

//! XML is currently disabled on MSP430 GDB
#define OPT_MEMORY_MAP	0
