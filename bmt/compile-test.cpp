// uncomment just when compile/test an include file
#if 1
#include "gpio-utils.h"
#include "spi.h"
#include "clocks.h"

namespace Bmt
{

void Test()
{
	Clocks::Hsi::Enable();
}

}	// namespace Bmt


#endif
