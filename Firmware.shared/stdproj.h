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
Default values for the flags manipulated next:

#define OPT_INCLUDE_JTAG_SPI_		0	// Enables/Disables JtagDev.spi.cpp file
#define OPT_INCLUDE_JTAG_TIM_DMA_	0	// Enables/Disables JtagDev.tim.cpp file
#define OPT_INCLUDE_JTAG_DTRIG_		0	// Enables/Disables JtagDev.dtrig.cpp file
#define OPT_BUFFER_LAYOUT_			OPT_BUFFER_LAYOUT_PAIR
#define OPT_BUFFER_CNT_				8	// Element count shared by every ping-pong sub-buffer
#define OPT_JTAG_SPEED_SEL_			0	// JTAG runs fixed speed only
using FrameBufEleType = uint8_t;
*/


// ── Ping-pong buffer layout selectors ────────────────────────────────────────
// All sub-buffers share the same element count (OPT_BUFFER_CNT_) and advance
// in lockstep via a single Step() — see util/PingPongBuffer.h.
#define OPT_BUFFER_LAYOUT_PAIR		2	///< TX + RX (most JTAG implementations)
#define OPT_BUFFER_LAYOUT_TRIPLE	3	///< TX + RX + AUX (TIM_DMA_SLOW: extra TMS-bit channel)


#if OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_SPI || OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_SPI_DMA
	#define OPT_INCLUDE_JTAG_SPI_		1
	#define OPT_BUFFER_LAYOUT_			OPT_BUFFER_LAYOUT_PAIR
	#define OPT_BUFFER_CNT_				2
	#define OPT_JTAG_SPEED_SEL_			1
	using FrameBufEleType = uint32_t;
#elif OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_TIM_DMA
	#define OPT_INCLUDE_JTAG_TIM_DMA_	1
	#define OPT_BUFFER_LAYOUT_			OPT_BUFFER_LAYOUT_PAIR
	#define OPT_BUFFER_CNT_				40
	#define OPT_JTAG_SPEED_SEL_			1
	using FrameBufEleType = uint32_t;
#elif OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_TIM_DMA_SLOW
	#define OPT_INCLUDE_JTAG_TIM_DMA_	1
	#define OPT_BUFFER_LAYOUT_			OPT_BUFFER_LAYOUT_TRIPLE
	#define OPT_BUFFER_CNT_				40
	#define OPT_JTAG_SPEED_SEL_			1
	using FrameBufEleType = uint32_t;
#elif OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_DTRIG
	#define OPT_INCLUDE_JTAG_DTRIG_		1
	// TMS is driven by TIM1_CH2N hardware toggle, not per-bit DMA → no AUX needed
	#define OPT_BUFFER_LAYOUT_			OPT_BUFFER_LAYOUT_PAIR
	#define OPT_BUFFER_CNT_				8
	#define OPT_JTAG_SPEED_SEL_			1
	using FrameBufEleType = uint8_t;
#endif

// Enables JtagDev.spi.cpp file
#ifndef OPT_INCLUDE_JTAG_SPI_
	#define OPT_INCLUDE_JTAG_SPI_		0
#endif // OPT_INCLUDE_JTAG_SPI_
// Enables JtagDev.tim.cpp file
#ifndef OPT_INCLUDE_JTAG_TIM_DMA_
	#define OPT_INCLUDE_JTAG_TIM_DMA_	0
#endif // OPT_INCLUDE_JTAG_TIM_DMA_
// Enables JtagDev.dtrig.cpp file
#ifndef OPT_INCLUDE_JTAG_DTRIG_
	#define OPT_INCLUDE_JTAG_DTRIG_		0
#endif // OPT_INCLUDE_JTAG_DTRIG_

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
	
#if (OPT_INCLUDE_JTAG_SPI_ + OPT_INCLUDE_JTAG_TIM_DMA_ + OPT_INCLUDE_JTAG_DTRIG_) == 0
	#error Missing JTAG implementation configuration
#elif (OPT_INCLUDE_JTAG_SPI_ + OPT_INCLUDE_JTAG_TIM_DMA_ + OPT_INCLUDE_JTAG_DTRIG_) != 1
	#error Conflicting JTAG implementation configuration
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

