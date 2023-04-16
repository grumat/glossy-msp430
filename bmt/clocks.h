#pragma once

#include "mcu-system.h"

#include "gpio.h"

#include "shared/AnyPllVco.h"

#if defined(STM32L4)
#	include "l4xx/clocks.h"
#elif defined(STM32G4)
#	include "g4xx/clocks.h"
#elif defined(STM32F1)
#	include "f1xx/clocks.h"
#else
#	error Unsupported target MCU
#endif

