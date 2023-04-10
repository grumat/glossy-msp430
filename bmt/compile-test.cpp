// uncomment just when compile/test an include file
#if 1
#include "gpio-utils.h"
#include "spi.h"
#include "clocks.h"

namespace Bmt
{

extern "C" const Clocks::PllFraction *g_Test;

void Test()
{
	typedef Clocks::AnyHse<> HSE;
	typedef Clocks::PllVcoAuto<> Calculator;
	typedef Clocks::AnyPll<HSE, 11500000UL, Calculator> PLL;
	PLL::Init();
	g_Test = &PLL::kPllFraction_;
}

}	// namespace Bmt


#endif
