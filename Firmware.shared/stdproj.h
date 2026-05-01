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
#define OPT_TX_BUFFER_CNT_			8	// Transmit ping pong buffer size
#define OPT_RX_BUFFER_CNT_			8	// Receive ping pong buffer size
#define OPT_AUX_BUFFER_CNT_		0	// Auxiliary ping pong buffer size (fixed uint32_t elements)
#define OPT_JTAG_SPEED_SEL_			0	// JTAG runs fixed speed only
using FrameBufEleType = uint8_t;
*/


#if OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_SPI || OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_SPI_DMA
	#define OPT_INCLUDE_JTAG_SPI_		1
	#define OPT_JTAG_SPEED_SEL_			1
#elif OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_TIM_DMA
	#define OPT_INCLUDE_JTAG_TIM_DMA_	1
	#define OPT_TX_BUFFER_CNT_			40
	#define OPT_RX_BUFFER_CNT_			40
	#define OPT_JTAG_SPEED_SEL_			1
	using FrameBufEleType = uint32_t;
#elif OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_TIM_DMA_SLOW
	#define OPT_INCLUDE_JTAG_TIM_DMA_	1
	#define OPT_TX_BUFFER_CNT_			40
	#define OPT_RX_BUFFER_CNT_			40
	#define OPT_AUX_BUFFER_CNT_			40
	#define OPT_JTAG_SPEED_SEL_			1
	using FrameBufEleType = uint32_t;
#elif OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_DTRIG
	#define OPT_INCLUDE_JTAG_DTRIG_		1
	#define OPT_TX_BUFFER_CNT_			8
	#define OPT_RX_BUFFER_CNT_			8
	#define OPT_AUX_BUFFER_CNT_			44
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
// Size of TX buffer
#ifndef OPT_TX_BUFFER_CNT_
	#define OPT_TX_BUFFER_CNT_			2
	using FrameBufEleType = uint32_t;
#endif // OPT_TX_BUFFER_CNT_
// Size of RX buffer
#ifndef OPT_RX_BUFFER_CNT_
	#define OPT_RX_BUFFER_CNT_			2
#endif // OPT_RX_BUFFER_CNT_
// Auxiliary flag buffer used during frame transmission
#ifndef OPT_AUX_BUFFER_CNT_
	#define OPT_AUX_BUFFER_CNT_		0
#endif // OPT_AUX_BUFFER_CNT_
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

