#include "stdproj.h"
#include "ui/gdb.h"
#include "drivers/TapMcu.h"
#include "util/ChipProfile.h"
#include "util/crc32.h"


UsartGdbDriver gUartGdb;


#if 0
extern "C" void SysTick_Handler(void)
{
	SystemTick::Handler();
}
#endif

extern "C" void USART1_IRQHandler()
{
	//__NOP();
	gUartGdb.HandleIrq();
}

extern "C" void SystemInit()
{
	PORTA::Init();
	PORTB::Init();
	PORTC::Init();
	AfSwj2Pin::Enable();
	SwoTrace::Init();

	SysClk::Init();

	//RedLedOff();
	//GreenLedOn();

	//	SystemTick::Init();
	MicroDelay::Init();
	TickTimer::Init();
	TickTimer::CounterStart();
	gUartGdb.Init();
	__enable_irq();
}


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
	Trace() << "\n\nBlack Magic Probe - MSP430 Edition\nStarting...\n";
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

