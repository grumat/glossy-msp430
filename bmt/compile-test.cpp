// uncomment just when compile/test an include file
#if 1
#include "gpio-utils.h"
#include "spi.h"
#include "clocks.h"

namespace Bmt
{

void Test()
{
	typedef Clocks::AnyHse<> HSE;
	typedef Clocks::PllVco<2> Calculator;
	// PLL limits for the STM32F103 hardware
	typedef Clocks::AnyPll<HSE, 50000000UL, Calculator> PLL;
	PLL::Init();
}

}	// namespace Bmt


#endif
