#pragma once

#include "mcu-system.h"
#include "pinremap.h"


/// Output pin speed configuration
enum class GpioSpeed
{
	kInput = 0,				///< Pin configured as input
	kOutput10MHz = 1,		///< Intermediate speed
	kOutput2MHz = 2,		///< Lowest speed and lowest energy consumption
	kOutput50MHz = 3		///< Maximum speed and highest energy consumption
};


/// GPIO pin configuration options
enum class GpioMode
{
	kAnalog = 0,			///< Analog pin
	kFloating = 1,			///< Floating input pin
	kInputPushPull = 2,		///< Input with push or pull load resistor
	kPushPull = 0,			///< Push/Pull output driver
	kOpenDrain = 1,			///< Open Drain output driver
	kAlternatePushPull = 2,	///< Alternate Function using Push/Pull output driver
	kAlternateOpenDrain = 3	///< Alternate function using open drain output driver
};


/// Pin voltage/logical level
enum class Level
{
	kLow = 0,				///< Drive pin to low voltage level
	kHigh = 1				///< Drive pin to high voltage level
};


#if defined(STM32L4)
#	include "gpio.l4.h"
#elif defined(STM32F1)
#	include "gpio.f1.h"
#else
#	error Unsupported target MCU
#endif

