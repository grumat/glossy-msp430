#include <bmt.h>

#ifdef __cplusplus

#if defined(STLINK)
#	include "stlink/platform.h"
#elif defined(HC6800EM3)
#	include "hc6800em3/platform.h"
#elif defined(BLUEPILL)
#	include "bluepill/platform.h"
#else
#	error Please define the platform for debugging
#endif

typedef DelayTimerTemplate<MicroDelayTimeBase> MicroDelay;		// single shot down-counter
typedef TimerTemplate<TickTimeBase> TickTimer;					// continuous up-counter
typedef StopWatchTemplate<TickTimer> StopWatch;

typedef UartFifo<UsartGdbSettings, 256, 64> UsartGdbBuffer;
typedef UsartIntDriverModel<UsartGdbBuffer> UsartGdbDriver;
extern UsartGdbDriver gUartGdb;


ALWAYS_INLINE void MyAbort()
{
	// Stop
	JtagOff::Enable();
	// Halt MCU
	JRST::Setup();
	JRST::SetLow();
	assert(false);
}

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
