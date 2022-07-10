#include "stdproj.h"
#include "ui/gdb.h"
#include "drivers/TapMcu.h"
#include "util/ChipProfile.h"
#include "util/crc32.h"


#ifdef OPT_USART_ISR
//! Instance of UART handler
UsartGdbDriver gUartGdb;
#endif


#if 0
/// The system tick handler
extern "C" void SysTick_Handler(void) __attribute__((interrupt("IRQ")));
extern "C" void SysTick_Handler(void)
{
	SystemTick::Handler();
}
#endif


#ifdef OPT_USART_ISR
/// UART Interrupt handler
extern "C" void GDB_IRQHandler() asm(OPT_USART_ISR) __attribute__((interrupt("IRQ")));
extern "C" void GDB_IRQHandler()
{
	// Let library handle communication
	gUartGdb.HandleIrq();
}
#endif


/// Initializes uC peripherals
extern "C" void SystemInit()
{
	// Configure ports
	PORTA::Init();
	PORTB::Init();
	PORTC::Init();
	// SWD pins
	AfSwj2Pin::Enable();
	// TraceSWO
	SwoTrace::Init();

	// Start clock system
	SysClk::Init();

	// Initialize delays and timers
	MicroDelay::Init();
	TickTimer::Init();
	TickTimer::CounterStart();
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
		uint32_t tim_ctick;
		double tim_tick;
		uint32_t tim2_pre;
		uint32_t tim3_pre;
	};
	Trace() << "\n\nGlossy MSP430\nStarting...\n";
	volatile MyData tmp;
	tmp.in_freq = HSE::kFrequency_;
	tmp.pll_freq = PLL::kFrequency_;
	tmp.sys_freq = SysClk::kFrequency_;
	tmp.tim_ctick = MicroDelayTimeBase::kClkTick;
	tmp.tim_tick = MicroDelayTimeBase::kTimerTick_;
	tmp.ahb_freq = SysClk::kAhbClock_;
	tmp.apb1_freq = SysClk::kApb1Clock_;
	tmp.apb2_freq = SysClk::kApb2Clock_;
	tmp.adc_freq = SysClk::kAdc_;
	tmp.tim2_pre = MicroDelayTimeBase::kPrescaler_;

#ifdef OPT_IMPLEMENT_TEST_DB
	TestDB();
#endif

	Gdb gdb;
	while (true)
	{
#if 1
		StopWatch().Delay(10);
		gdb.Serve();
#else
		static int cnt = 0;
		StopWatch().Delay(500);
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

