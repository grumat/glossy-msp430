#pragma once


namespace Bmt
{
namespace Power
{

#if defined(STM32L4)

enum class Mode
{
	kRange1,	//!< Range 1 with the CPU running at up to 80 MHz.
	kRange2,	//!< Range 2 with a maximum CPU frequency of 26 MHz. 
				//!< All peripheral clocks are also limited to 26 MHz.
	kLowPower,	//!< Low-power run mode with the CPU running at up to 2 MHz. Peripherals 
				//!< with independent clock can be clocked by HSI16.
};

#elif defined(STM32F1)

enum class Mode
{
	kRange1,	//!< A single power domain is available on this family
};

#else
#	error Unsupported target MCU
#endif

}	// namespace Power
}	// namespace Bmt
