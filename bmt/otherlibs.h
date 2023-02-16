#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// STM32F103xB
#if defined (STM32F100xB) || defined (STM32F100xE) || defined (STM32F101x6) || \
	defined (STM32F101xB) || defined (STM32F101xE) || defined (STM32F101xG) || defined (STM32F102x6) || defined (STM32F102xB) || defined (STM32F103x6) || \
	defined (STM32F103xB) || defined (STM32F103xE) || defined (STM32F103xG) || defined (STM32F105xC) || defined (STM32F107xC)
// %USERPROFILE%\AppData\Local\VisualGDB\EmbeddedBSPs\arm - eabi\com.sysprogs.arm.stm32\STM32F1xxxx\CMSIS_HAL\Device\ST\STM32F1xx\Include\stm32f1xx.h
#   include <stm32f1xx.h>
#elif defined(STM32L412xx) || defined(STM32L422xx) ||                                                                            \
	defined(STM32L431xx) || defined(STM32L432xx) || defined(STM32L433xx) || defined(STM32L442xx) || defined(STM32L443xx) ||    \
	defined(STM32L451xx) || defined(STM32L452xx) || defined(STM32L462xx) ||                                                    \
	defined(STM32L471xx) || defined(STM32L475xx) || defined(STM32L476xx) || defined(STM32L485xx) || defined(STM32L486xx) ||    \
	defined(STM32L496xx) || defined(STM32L4A6xx) ||                                                                            \
	defined(STM32L4P5xx) || defined(STM32L4Q5xx) ||                                                                            \
	defined(STM32L4R5xx) || defined(STM32L4R7xx) || defined(STM32L4R9xx) || defined(STM32L4S5xx) || defined(STM32L4S7xx) || defined(STM32L4S9xx)
// %USERPROFILE%\AppData\Local\VisualGDB\EmbeddedBSPs\arm-eabi\com.sysprogs.arm.stm32\STM32L4xxxx\CMSIS_HAL\Device\ST\STM32L4xx\Include\stm32l4xx.h
#   include <stm32l4xx.h>
#else
#	error Missing target specification or compatible STM32 macro is missing!
#endif

#include "compiler.h"

