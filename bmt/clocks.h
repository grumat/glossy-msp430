#pragma once

#include "otherlibs.h"

#include "mcu-system.h"

#if defined(STM32L4)
#	include "clocks.l4.h"
#elif defined(STM32F1)
#	include "clocks.f1.h"
#else
#	error Unsupported target MCU
#endif

