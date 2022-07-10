#include <bmt.h>

//#define OPT_IMPLEMENT_TEST_DB

#ifdef __cplusplus

#if defined(PROTO_V1)
#	include "proto-v1/platform.h"
#elif defined(BLUEPILL)
#	include "bluepill/platform.h"
#else
#	error Please define the platform for debugging
#endif

#ifndef OPT_JTAG_USING_SPI
#error OPT_JTAG_USING_SPI definition is required for every platform
#endif

/// Currently JTAG speed selection depends on JTAG-over-SPI feature
#define OPT_JTAG_SPEED_SEL		OPT_JTAG_USING_SPI


/// Single shot down-counter
typedef DelayTimerTemplate<MicroDelayTimeBase> MicroDelay;
/// Continuous up-counter
typedef TimerTemplate<TickTimeBase> TickTimer;
/// A stop watch object
typedef StopWatchTemplate<TickTimer> StopWatch;

#ifdef OPT_USART_ISR
/// Defines a dual FIFO buffer for GDB UART port
typedef UartFifo<UsartGdbSettings, 256, 64> UsartGdbBuffer;
/// The UART driver using interrupts
typedef UsartIntDriverModel<UsartGdbBuffer> UsartGdbDriver;
/// Singleton for the GDB UART
extern UsartGdbDriver gUartGdb;
#endif


#if 0
ALWAYS_INLINE void MyAbort()
{
	// Stop
	JtagOff::Enable();
	// Halt MCU
	JRST::Setup();
	JRST::SetLow();
	assert(false);
}
#endif

typedef SwoChannel<0> Trace_;
typedef SwoChannel<1> Error_;
#if DEBUG
typedef SwoChannel<2> Debug_;
#else
typedef SwoDummyChannel Debug_;
#endif
typedef SwoTraceSetup <SysClk, kAsynchronous, 720000, Trace_, Error_, Debug_> SwoTrace;
// A stream object for the trace output
typedef OutStream<Trace_> Trace;
typedef OutStream<Error_> Error;
typedef OutStream<Debug_> Debug;

#endif		// __cplusplus
