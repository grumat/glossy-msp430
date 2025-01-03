#include "stdproj.h"
#include "ui/gdb.h"
#include "drivers/TapMcu.h"
#include "util/ChipProfile.h"
#include "util/crc32.h"


#ifdef OPT_USART_ISR
//! Instance of UART handler
UsartGdbDriver gUartGdb;
#endif


#ifdef OPT_USART_ISR
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"
/// UART Interrupt handler
extern "C" void GDB_IRQHandler() asm(OPT_USART_ISR) __attribute__((interrupt("IRQ")));
extern "C" void GDB_IRQHandler()
{
	// Let library handle communication
	gUartGdb.HandleIrq();
}
#pragma GCC diagnostic pop
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
	
	// SWD pins
#if STM32F103xB
	AfSwd3::Enable();
#endif
	System::Init();
	// Configure ports
	PORTD::Init();
	PORTA::Init();
	PORTB::Init();
	PORTC::Init();
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
	Trace() << "\n\nGlossy MSP430\nStarting...\n";
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
	tmp.any_test = DEBUG_BUS_CTRL::kIsSequential_;

#ifdef OPT_IMPLEMENT_TEST_DB
	TestDB();
#endif

	// Indicate activity after HW initialized
	SetLedState(LedState::green);
	SetLedState(LedState::on);
	
	Gdb gdb;
	while (true)
	{
#if 1
		StopWatch().Delay<Timer::Msec(10)>();
		gdb.Serve();
#else
		static int cnt = 0;
		StopWatch().Delay<500>();
		switch (cnt)
		{
		case 1:
			RedLedOff();
			GreenLedOn();
			break;
		case 3:
			RedLedOn();
			GreenLedOff();
			break;
		case 5:
			RedLedOn();
			GreenLedOn();
			cnt = -1;
			break;
		default:
			RedLedOff();
			GreenLedOff();
			break;
		}
		++cnt;
#endif
	}
	return 0;
}

