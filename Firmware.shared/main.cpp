#include "stdproj.h"
#include "ui/gdb.h"
#include "drivers/TapMcu.h"
#include "drivers/TargetPower.h"
#include "util/ChipProfile.h"
#include "util/crc32.h"
#include <main.inl>

#if OPT_TEST_TIM_DMA_TIMING
#include "util/TimDmaTiming.h"
/// Bench-only timer→DMA latency probe (driver-decoupled). The enabling target's
/// platform.h supplies the resource bundle (timer, SBWCLK PWM channel, two
/// frozen-compare DMA-trigger channels, the SBWCLK AF pin and the pulsed data pin)
/// under #if OPT_TEST_TIM_DMA_TIMING. 1 MHz wire clock — a round number near the SBW
/// ceiling. See util/TimDmaTiming.h.
using TimDmaTimingProbe = TimDmaTiming_ns::TimDmaTiming<
	SysClk, kTimDmaTimer, kTimDmaClkCh, kTimDmaTrigACh, kTimDmaTrigBCh,
	TimDmaTimingClkPin, TimDmaTimingDio,
	OPT_TEST_TIM_DMA_TIMING, OPT_TEST_TIM_DMA_MULT, 1000000UL, kTimDmaClkCmpComplementary>;
#endif


#if OPT_GDB_IMPLEMENTATION != OPT_GDB_IMPL_VCP
//! Instance of UART handler
UsartGdbDriver gUartGdb;
#endif


/// Initializes uC peripherals
extern "C" void SystemInit()
{
	/*
	** STM32F103 clones have problems with that initialization order:
	** - First initialize SWD before anything
	** - Then SWO tracing or GPIO in any desired order
	** By moving SWD initialization after GPIO, SWO will simply not work
	*/

	// One-shot peripheral clock + reset for everything this firmware uses.
	// Listed in target.*/platform.h. Replaces per-class Init()::RCC writes,
	// which are now deprecated and emit warnings if anyone still calls them.
	PeripheralEnabler::Init();

	// SWD pins
#if STM32F103xB
	AfSwd3::Enable();
#endif
	// BMT initialization
	System::Init();
	// Enable peripheral clocks
	PeripheralEnabler::Reset();
	PeripheralEnabler::Enable();
	// Initializes all GPIO pins to the initial setup
	AllGpioStartup::Setup();
	// TraceSWO
	SwoTrace::Init();

	// Start clock system
	SysClk::Init();

	// Initialize delays and timers
	TickTimer::Init();
	// Serial port
	gUartGdb.Init();
	// Enable interrupts
	__enable_irq();
}


/// Main program entry
extern "C" int main()
{
	Trace() << "\n\nGlossy MSP430 " GLOSSY_FW_VERSION "\nStarting...\n";

#if OPT_TEST_TIM_DMA_TIMING
	// Bench mode: emit the timer→DMA latency burst for the logic analyzer and halt.
	// Never returns (single-shot, like DoLogicAnalyzerTest()).
	TimDmaTimingProbe::Run();
#endif

#ifdef OPT_IMPLEMENT_TEST_DB
	TestDB();
#endif

	// Indicate activity after HW initialized
	SetLedState(LedState::green);
	SetLedState(LedState::on);

	// Bring up the target-voltage sense ADC (no-op on probes without sense).
	TargetPower::Init();

#if OPT_BARE_RUN != OPT_BARE_RUN_GDB
	// ── Bench detect-only mode (no GDB) ──────────────────────────────────────
	// Autonomously acquire the target on the configured transport and report it
	// over SWO trace, looping so MCUs can be hot-swapped on the bench. Open()
	// already emits the identify trace + "Device:" line (+ the full memory map
	// in DEBUG builds), so no GDB host is needed to see the result.
#if OPT_BARE_RUN == OPT_BARE_RUN_SBW
	g_TapMcu.SetTransport(TapMcu::Transport::kSbw);
	Trace() << "Bare detect mode (SBW) - no GDB\n";
#else
	g_TapMcu.SetTransport(TapMcu::Transport::kJtag);
	Trace() << "Bare detect mode (JTAG) - no GDB\n";
#endif
	while (true)
	{
		Trace() << "\n--- scanning ---\n";
		if (g_TapMcu.Open())
		{
			SetLedState(LedState::red);		// target found / attached
			g_TapMcu.Close();				// release so the part can be swapped
		}
		else
		{
			SetLedState(LedState::green);	// nothing detected
			Trace() << "No target detected\n";
		}
		StopWatch().Delay<Timer::Msec(1000)>();
	}
#else
	Gdb gdb;
	while (true)
	{
		StopWatch().Delay<Timer::Msec(10)>();
		gdb.Serve();
	}
#endif
	return 0;
}

