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

typedef TimeBase_us<kTim3, SysClk, 1U> MicroDelayTimeBase;
typedef DelayTimerTemplate<MicroDelayTimeBase> MicroDelay;		// single shot down-counter
typedef TimeBase_us<kTim2, SysClk> TickTimeBase;
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
typedef SwoChannel<0> Error_;
#if DEBUG
typedef SwoChannel<0> Debug_;
#else
typedef SwoDummyChannel Debug_;
#endif
typedef SwoTraceSetup <SysClk, kAsynchronous, 921600, Trace_, Error_, Debug_> SwoTrace;
// A stream object for the trace output
typedef OutStream<Trace_> Trace;
typedef OutStream<Error_> Error;
typedef OutStream<Debug_> Debug;

// A DMA channel for JTAG wave generation
template<
	const DmaDirection DIRECTION
	, const DmaPointerCtrl SRC_PTR
	, const DmaPointerCtrl DST_PTR
	, const DmaPriority PRIO = kDmaMediumPrio
>
class DmaForJtagWave : public DmaChannel<kDmaForJtag, kDmaChForJtag, DIRECTION, SRC_PTR, DST_PTR, PRIO>
{
public:
};

#endif		// __cplusplus
