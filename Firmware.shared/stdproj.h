#pragma once

#define STDPROJ_H__INCLUDED__

#ifndef __cplusplus
	#error Project requires a C++ compiler
#endif
#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
	#error Project requires a little endian MCU
#endif


#include <bmt.h>

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

/// Bench-only logic-analyzer probe; off by default. Define to non-zero in a
/// target's platform.h to compile in JtagDev::DoLogicAnalyzerTest() and have
/// OnOpen() invoke it.
#ifndef OPT_TEST_WITH_LOGIC_ANALYZER
	#define OPT_TEST_WITH_LOGIC_ANALYZER	0
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
