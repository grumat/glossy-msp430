#include "stdproj.h"
#include "ui/gdb.h"
#include "drivers/TapMcu.h"
#include "util/ChipProfile.h"
#include "util/crc32.h"
#include <main.inl>


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
	struct MyData
	{
		uint32_t in_freq;
		uint32_t pll_freq;
		uint32_t sys_freq;
		uint32_t ahb_freq;
		uint32_t apb1_freq;
		uint32_t apb2_freq;
		uint32_t adc_freq;
		uint32_t any_test;
	};
	Trace() << "\n\nGlossy MSP430 " GLOSSY_FW_VERSION "\nStarting...\n";
	volatile MyData tmp;
	tmp.in_freq = HSE::kFrequency_;
	tmp.pll_freq = PLL::kFrequency_;
	tmp.sys_freq = SysClk::kFrequency_;
	tmp.ahb_freq = SysClk::kAhbClock_;
	tmp.apb1_freq = SysClk::kApb1Clock_;
	tmp.apb2_freq = SysClk::kApb2Clock_;
#if STM32F103xB
	tmp.adc_freq = SysClk::kAdc_;
#else
	tmp.adc_freq = 0;
#endif
	//tmp.any_test = DEBUG_BUS_CTRL::kIsSequential_;

#ifdef OPT_IMPLEMENT_TEST_DB
	TestDB();
#endif

	// Indicate activity after HW initialized
	SetLedState(LedState::green);
	SetLedState(LedState::on);

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

