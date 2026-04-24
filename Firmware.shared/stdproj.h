#pragma once

#define STDPROJ_H__INCLUDED__

#include <bmt.h>

//#define OPT_IMPLEMENT_TEST_DB

#ifdef __cplusplus

// Project settings should select the correct `platform.h` file path
#include <platform.h>

#ifndef OPT_JTAG_IMPLEMENTATION
#	error OPT_JTAG_IMPLEMENTATION definition is required for every platform
#endif

#if OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_SPI || OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_SPI_DMA
#	define OPT_INCLUDE_JTAG_SPI_	1
#elif defined (OPT_INCLUDE_JTAG_SPI_)
#	undef OPT_INCLUDE_JTAG_SPI_
#endif

#if OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_TIM_DMA || OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_TIM_DMA_SLOW
#	define OPT_INCLUDE_JTAG_TIM_DMA_	1
#elif defined(OPT_INCLUDE_JTAG_TIM_DMA_)
#	undef OPT_INCLUDE_JTAG_TIM_DMA_
#endif

#ifndef OPT_JTAG_SPEED_SEL
#	if OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_TIM_DMA \
		|| OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_TIM_DMA_SLOW \
		|| OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_SPI \
		|| OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_SPI_DMA
#			define OPT_JTAG_SPEED_SEL	1
#	else
#			define OPT_JTAG_SPEED_SEL	0
#	endif
#endif

#ifndef OPT_JTAG_TCLK_IMPLEMENTATION
#	error Platform.h has to specify the OPT_JTAG_TCLK_IMPLEMENTATION option value
#endif

#ifndef OPT_GDB_IMPLEMENTATION
#	error Platform.h has to specify the OPT_GDB_IMPLEMENTATION option value
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

#ifndef OPT_TMS_VERY_HIGH_CLOCK
/// If SPI clock is SYSCLK/8 internal delays breaks TMS signal.
/// Pulse Anticipation is required. Specifies the speed level (2-5). Use 9 to disable.
#	define OPT_TMS_VERY_HIGH_CLOCK	9
#endif


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

#endif		// __cplusplus
