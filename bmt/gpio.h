#pragma once

#include "mcu-system.h"
#include "pinremap.h"


/// Output pin speed configuration
enum GpioMode
{
	kInput = 0,				///< Pin configured as input
	kOutput10MHz = 1,		///< Intermediate speed
	kOutput2MHz = 2,		///< Lowest speed and lowest energy consumption
	kOutput50MHz = 3		///< Maximum speed and highest energy consumption
};


/// GPIO pin configuration options
enum GpioConf
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
enum Level
{
	kLow = 0,				///< Drive pin to low voltage level
	kHigh = 1				///< Drive pin to high voltage level
};


/// A template class representing an unused pin
template<
	const uint8_t kPin			///< the pin number
	>
class PinUnused
{
public:
	/// Constant storing the GPIO port number
	static constexpr GpioPortId kPort_ = kUnusedPort;
	/// The pin number constant value
	static constexpr uint8_t kPin_ = kPin;
	/// Unused pins are always configured as input
	static constexpr GpioMode kMode_ = kInput;
	/// Unused pins are always pulled with a resistor load
	static constexpr GpioConf kConf_ = kInputPushPull;
	/// Configuration combo
	static constexpr uint8_t kModeConf_ = (kInputPushPull << 2 | kInput);
	/// Constant value for CRL hardware register
	static constexpr uint32_t kModeConfLow_ = kPin < 8 ? kModeConf_ << (kPin << 2) : 0UL;
	/// Constant mask value for CRL hardware register
	static constexpr uint32_t kModeConfLowMask_ = ~(kPin < 8 ? 0x0FUL << (kPin << 2) : 0UL);
	/// Constant value for CRH hardware register
	static constexpr uint32_t kModeConfHigh_ = kPin >= 8 ? kModeConf_ << ((kPin - 8) << 2) : 0;
	/// Constant mask value for CRH hardware register
	static constexpr uint32_t kModeConfHighMask_ = ~(kPin >= 8 ? 0x0FUL << ((kPin - 8) << 2) : 0UL);
	/// Effective bit constant value
	static constexpr uint16_t kBitValue_ = 0;
	/// Constant for the initial bit level
	static constexpr uint32_t kInitialLevel_ = kLow;
	/// Alternate Function configuration constant
	static constexpr uint32_t kAfConf_ = 0x00000000U;
	/// Alternate Function configuration mask constant (inverted)
	static constexpr uint32_t kAfMask_ = 0xFFFFFFFFU;
	/// Constant Flag indicating that no Alternate Function is required
	static constexpr bool kAfDisabled_ = true;
	/// Constant flag to indicate that a bit is not used for a particular configuration
	static constexpr bool kIsUnused_ = true;

	/// An unused pin always returns false here
	ALWAYS_INLINE static constexpr bool IsHigh(void) { return false; }
	/// An unused pin always returns true here
	ALWAYS_INLINE static constexpr bool IsLow(void) { return true; }
	/// No function for an unused pin
	ALWAYS_INLINE static void Toggle(void) { }
	/// No function for an unused pin
	ALWAYS_INLINE static void SetHigh(void) { }
	/// No function for an unused pin
	ALWAYS_INLINE static void SetLow(void) { }
};


/// A template pin configuration for a pin that should not be affected
template<
	const uint8_t kPin		///< Pin number is required
>
class PinUnchanged
{
public:
	/// Constant storing the GPIO port number
	static constexpr GpioPortId kPort_ = kUnusedPort;
	/// Constant holding pin number
	static constexpr uint8_t kPin_ = kPin;
	/// Configuration constant value for hardware register
	static constexpr uint8_t kModeConf_ = 0UL;
	/// Constant value for CRL hardware register
	static constexpr uint32_t kModeConfLow_ = 0UL;
	/// Constant mask value for CRL hardware register
	static constexpr uint32_t kModeConfLowMask_ = ~(0UL);
	/// Constant value for CRH hardware register
	static constexpr uint32_t kModeConfHigh_ = 0UL;
	/// Constant mask value for CRH hardware register
	static constexpr uint32_t kModeConfHighMask_ = ~(0UL);
	/// Effective bit constant value
	static constexpr uint16_t kBitValue_ = 0;
	/// Constant for the initial bit level
	static constexpr uint32_t kInitialLevel_ = kLow;
	/// Alternate Function configuration constant
	static constexpr uint32_t kAfConf_ = 0x00000000U;
	/// Alternate Function configuration mask constant (inverted)
	static constexpr uint32_t kAfMask_ = 0xFFFFFFFFU;
	/// Constant Flag indicating that no Alternate Function is required
	static constexpr bool kAfDisabled_ = true;
	/// Constant flag to indicate that a bit is not used for a particular configuration
	static constexpr bool kIsUnused_ = true;

	/// An unchanged pin always returns false here
	ALWAYS_INLINE static constexpr bool IsHigh(void) { return false; }
	/// An unchanged pin always returns false here
	ALWAYS_INLINE static constexpr bool IsLow(void) { return true; }
	/// No function for an unchanged pin
	ALWAYS_INLINE static void Toggle(void) {}
	/// No function for an unchanged pin
	ALWAYS_INLINE static void SetHigh(void) {}
	/// No function for an unchanged pin
	ALWAYS_INLINE static void SetLow(void) {}
};


/*!
**	@brief Defines/Sets up a single GPIO pin
**
**	This template sets up a single GPIO pin. Methods allows one to bit bang pin 
**	or read its input state.
**
**	An additional powerful feature is to combine all need GPIO pin definition
**	together into a GpioPortTemplate<> data type, which is able to setup the 
**	entire GPIO port in a couple of CPU instructions.
**
**	Example:
**		// Sets a data-type to drive an SPI1 CLK output
**		typedef GpioTemplate<PA, 5, kOutput50MHz, kPushPull, kHigh> MY_SPI_CLK;
**		// Sets a data-type to inactivate the pin defined before
**		typedef InputPullUpPin<PA, 5> MY_INACTIVE_SPI_CLK;
**
**	Also see the shortcut templates that reduces the clutter to declare common
**	IO forms: FloatingPin<>, InputPullUpPin<> and InputPullDownPin<>.
**
**	Device specific peripherals are also mapped into handy data-types, like for
**	example: SPI1_SCK_PA5, ADC12_IN0 and TIM2_CH2_PA1.
**
**	@tparam kPort: the GPIO port.
**	@tparam kPin: The GPIO pin number.
**	@tparam kMode: Defines pin direction.
**	@tparam kConf: Defines how the pin is driven or pulled up/down.
**	@tparam kInitialLevel: Defines the level of the pin to be initialized.
**	@tparam Map: A data-type that allows STM32 Pin Remap. Definitions are found on remap.h.
*/
template<
	const GpioPortId kPort					///< The GPIO port
	, const uint8_t kPin					///< The pin of the port
	, const GpioMode kMode = kInput			///< Mode to configure the port
	, const GpioConf kConf = kFloating		///< Additional pin configuration
	, const Level kInitialLevel = kLow		///< Initial pin level
	, typename Map = AfNoRemap				///< Pin remapping feature (pinremap.h)
	>
class GpioTemplate
{
public:
	/// A constant to record the mapping type
	typedef Map MapType;
	/// Constant storing the GPIO port number
	static constexpr GpioPortId kPort_ = kPort;
	/// Base address of the port peripheral
	static constexpr uint32_t kPortBase_ = (GPIOA_BASE + kPort_ * 0x400);
	/// Constant storing the GPIO pin number
	static constexpr uint8_t kPin_ = kPin;
	/// Constant storing the port mode
	static constexpr GpioMode kMode_ = kMode;
	/// Constant storing the port configuration
	static constexpr GpioConf kConf_ = kConf;
	/// Combination of mode and configuration, suitable for the hardware register
	static constexpr uint8_t kModeConf_ = (kConf << 2 | kMode);
	/// Constant value for CRL hardware register
	static constexpr uint32_t kModeConfLow_ = kPin < 8 ? kModeConf_ << (kPin << 2) : 0UL;
	/// Constant mask value for CRL hardware register
	static constexpr uint32_t kModeConfLowMask_ = ~(kPin < 8 ? 0x0FUL << (kPin << 2) : 0UL);
	/// Constant value for CRH hardware register
	static constexpr uint32_t kModeConfHigh_ = kPin >= 8 ? kModeConf_ << ((kPin - 8) << 2) : 0UL;
	/// Constant mask value for CRH hardware register
	static constexpr uint32_t kModeConfHighMask_ = ~(kPin >= 8 ? 0x0FUL << ((kPin - 8) << 2) : 0UL);
	/// Effective bit constant value
	static constexpr uint16_t kBitValue_ = 1 << kPin;
	/// Constant for the initial bit level
	static constexpr uint32_t kInitialLevel_ = kInitialLevel << kPin;
	/// Alternate Function configuration constant
	static constexpr uint32_t kAfConf_ = Map::kConf;
	/// Alternate Function configuration mask constant (inverted)
	static constexpr uint32_t kAfMask_ = Map::kMask;
	/// Constant Flag indicating that no Alternate Function is required
	static constexpr bool kAfDisabled_ = Map::kNoRemap;
	/// Constant flag to indicate that a bit is not used for a particular configuration
	static constexpr bool kIsUnused_ = false;

	/// Access to the peripheral memory space
	ALWAYS_INLINE static volatile GPIO_TypeDef *GetPortBase() { return (volatile GPIO_TypeDef *)kPortBase_; }

	/// Sets pin up. The pin will be high as long as it is configured as GPIO output
	ALWAYS_INLINE static void SetHigh(void)
	{
		volatile GPIO_TypeDef *port = (volatile GPIO_TypeDef *)kPortBase_;
		port->BSRR = kBitValue_;
	};

	/// Sets pin down. The pin will be low as long as it is configured as GPIO output
	ALWAYS_INLINE static void SetLow(void)
	{
		volatile GPIO_TypeDef *port = (volatile GPIO_TypeDef *)kPortBase_;
		port->BRR = kBitValue_;
	}

	/// Sets the pin to the given level. Note that optimizing compiler simplifies literal constants
	ALWAYS_INLINE static void Set(bool value)
	{
		if (value)
			SetHigh();
		else
			SetLow();
	}

	/// Reads current Pin electrical state
	ALWAYS_INLINE static bool Get(void)
	{
		volatile GPIO_TypeDef *port = (volatile GPIO_TypeDef *)kPortBase_;
		return (port->IDR & kBitValue_) != 0;
	}

	/// Checks if current pin electrical state is high
	ALWAYS_INLINE static bool IsHigh(void)
	{
		volatile GPIO_TypeDef *port = (volatile GPIO_TypeDef *)kPortBase_;
		return (port->IDR & kBitValue_) != 0;
	}

	/// Checks if current pin electrical state is low
	ALWAYS_INLINE static bool IsLow(void)
	{
		return !IsHigh();
	}

	/// Toggles pin state
	ALWAYS_INLINE static void Toggle(void)
	{
		volatile GPIO_TypeDef *port = (volatile GPIO_TypeDef *)kPortBase_;
		port->ODR ^= kBitValue_;
	}
	/// Apply default configuration for the pin.
	ALWAYS_INLINE static void SetupPinMode(void)
	{
		volatile GPIO_TypeDef *port = (volatile GPIO_TypeDef *)kPortBase_;
		if (kPin < 8)
			port->CRL = (port->CRL & kModeConfLowMask_) | kModeConfLow_;
		else
			port->CRH = (port->CRH & kModeConfHighMask_) | kModeConfHigh_;
	}
	/// Apply default configuration for the pin.
	ALWAYS_INLINE static void Setup(void)
	{
		volatile GPIO_TypeDef *port = (volatile GPIO_TypeDef *)kPortBase_;
		if (kPin < 8)
			port->CRL = (port->CRL & kModeConfLowMask_) | kModeConfLow_;
		else
			port->CRH = (port->CRH & kModeConfHighMask_) | kModeConfHigh_;
		Map::Enable();
		Set(kInitialLevel_);
	}
	/// Apply a special configuration to the pin, after initialization
	ALWAYS_INLINE static void Setup(GpioMode mode, GpioConf conf)
	{
		volatile GPIO_TypeDef *port = (volatile GPIO_TypeDef *)kPortBase_;
		const uint32_t mode_conf = (conf << 2 | mode);
		/// Pin number defines if CRL or CRH is used
		if (kPin < 8)
		{
			const uint32_t kModeConfLow_ = mode_conf << (kPin << 2);
			port->CRL = (port->CRL & kModeConfLowMask_) | kModeConfLow_;
		}
		else
		{
			const uint32_t kModeConfHigh_ = mode_conf << ((kPin - 8) << 2);
			port->CRH = (port->CRH & kModeConfHighMask_) | kModeConfHigh_;
		}
	}
};


/// A template class to configure a port pin as a floating input pin
template <
	const GpioPortId kPort		///< The GPIO port
	, const uint8_t kPin		///< The GPIO pin number
>
class FloatingPin : public GpioTemplate<kPort, kPin, kInput, kFloating>
{
};


/// A template class to configure a port pin as digital input having a pull-up
template <
	const GpioPortId kPort
	, const uint8_t kPin
>
class InputPullUpPin : public GpioTemplate<kPort, kPin, kInput, kInputPushPull, kHigh>
{
};


/// A template class to configure a port pin as digital input having a pull-down
template <
	const GpioPortId kPort
	, const uint8_t kPin
>
class InputPullDownPin : public GpioTemplate<kPort, kPin, kInput, kInputPushPull, kLow>
{
};


/// A template class to configure a GPIO port at once (optimizes code footprint)
/*!
Usual port program happens bit by bit, which tends to produce too much unnecessary code.
By combining pin templates with this class it is possible to group multiple GPIO pins 
into a single operation. By grouping pins that cooperates logicall into a group it is 
possible to prepare predefined states according to a function group.

Example:

This configuration is a sample code to setup the GPIO for the USART1 through PA9/PA10
and a LED on PA0.
\code{.cpp}
/// Pin for green LED
typedef GpioTemplate<PA, 0, kOutput2MHz, kPushPull, kHigh> GREEN_LED;
/// Initial configuration for PORTA
typedef GpioPortTemplate <PA
	, GREEN_LED			///< bit bang
	, PinUnused<1>		///< not used
	, PinUnused<2>		///< not used
	, PinUnused<3>		///< not used
	, PinUnused<4>		///< not used
	, PinUnused<5>		///< not used
	, PinUnused<6>		///< not used
	, PinUnused<7>		///< not used
	, PinUnused<8>		///< not used
	, USART1_TX_PA9		///< GDB UART port
	, USART1_RX_PA10	///< GDB UART port
	, PinUnused<11>		///< USB-
	, PinUnused<12>		///< USB+
	, PinUnused<13>		///< STM32 TMS/SWDIO
	, PinUnused<14>		///< STM32 TCK/SWCLK
	, PinUnused<15>		///< STM32 TDI
> PORTA;

void MyHardwareInit()
{
	// Configure ports
	PORTA::Init();
}
\endcode
*/
template <
	const GpioPortId kPort				/// The GPIO port number
	, typename Pin0 = PinUnused<0>		/// Definition for bit 0 (defaults to unused pin, i.e an inputs)
	, typename Pin1 = PinUnused<1>		/// Definition for bit 1 (defaults to unused pin, i.e an inputs)
	, typename Pin2 = PinUnused<2>		/// Definition for bit 2 (defaults to unused pin, i.e an inputs)
	, typename Pin3 = PinUnused<3>		/// Definition for bit 3 (defaults to unused pin, i.e an inputs)
	, typename Pin4 = PinUnused<4>		/// Definition for bit 4 (defaults to unused pin, i.e an inputs)
	, typename Pin5 = PinUnused<5>		/// Definition for bit 5 (defaults to unused pin, i.e an inputs)
	, typename Pin6 = PinUnused<6>		/// Definition for bit 6 (defaults to unused pin, i.e an inputs)
	, typename Pin7 = PinUnused<7>		/// Definition for bit 7 (defaults to unused pin, i.e an inputs)
	, typename Pin8 = PinUnused<8>		/// Definition for bit 8 (defaults to unused pin, i.e an inputs)
	, typename Pin9 = PinUnused<9>		/// Definition for bit 9 (defaults to unused pin, i.e an inputs)
	, typename Pin10 = PinUnused<10>	/// Definition for bit 10 (defaults to unused pin, i.e an inputs)
	, typename Pin11 = PinUnused<11>	/// Definition for bit 11 (defaults to unused pin, i.e an inputs)
	, typename Pin12 = PinUnused<12>	/// Definition for bit 12 (defaults to unused pin, i.e an inputs)
	, typename Pin13 = PinUnused<13>	/// Definition for bit 13 (defaults to unused pin, i.e an inputs)
	, typename Pin14 = PinUnused<14>	/// Definition for bit 14 (defaults to unused pin, i.e an inputs)
	, typename Pin15 = PinUnused<15>	/// Definition for bit 15 (defaults to unused pin, i.e an inputs)
	>
class GpioPortTemplate
{
public:
	/// The GPIO port pereipheral
	static constexpr GpioPortId kPort_ = kPort;
	/// The base address for the GPIO peripheral registers
	static constexpr uint32_t kPortBase_ = (GPIOA_BASE + kPort_ * 0x400);
	/// Combined constant value for CRL hardware register
	static constexpr uint32_t kCrl_ =
		Pin0::kModeConfLow_ | Pin1::kModeConfLow_ 
		| Pin2::kModeConfLow_ | Pin3::kModeConfLow_ 
		| Pin4::kModeConfLow_ | Pin5::kModeConfLow_ 
		| Pin6::kModeConfLow_ | Pin7::kModeConfLow_ 
		| Pin8::kModeConfLow_ | Pin9::kModeConfLow_ 
		| Pin10::kModeConfLow_ | Pin11::kModeConfLow_ 
		| Pin12::kModeConfLow_ | Pin13::kModeConfLow_ 
		| Pin14::kModeConfLow_ | Pin15::kModeConfLow_
		;
	/// Combined constant mask value for CRL hardware register
	static constexpr uint32_t kCrlMask_ =
		Pin0::kModeConfLowMask_ & Pin1::kModeConfLowMask_
		& Pin2::kModeConfLowMask_ & Pin3::kModeConfLowMask_
		& Pin4::kModeConfLowMask_ & Pin5::kModeConfLowMask_
		& Pin6::kModeConfLowMask_ & Pin7::kModeConfLowMask_
		& Pin8::kModeConfLowMask_ & Pin9::kModeConfLowMask_
		& Pin10::kModeConfLowMask_ & Pin11::kModeConfLowMask_
		& Pin12::kModeConfLowMask_ & Pin13::kModeConfLowMask_
		& Pin14::kModeConfLowMask_ & Pin15::kModeConfLowMask_
		;
	/// Combined constant value for CRH hardware register
	static constexpr uint32_t kCrh_ =
		Pin0::kModeConfHigh_ | Pin1::kModeConfHigh_ 
		| Pin2::kModeConfHigh_ | Pin3::kModeConfHigh_ 
		| Pin4::kModeConfHigh_ | Pin5::kModeConfHigh_ 
		| Pin6::kModeConfHigh_ | Pin7::kModeConfHigh_ 
		| Pin8::kModeConfHigh_ | Pin9::kModeConfHigh_ 
		| Pin10::kModeConfHigh_ | Pin11::kModeConfHigh_ 
		| Pin12::kModeConfHigh_ | Pin13::kModeConfHigh_ 
		| Pin14::kModeConfHigh_ | Pin15::kModeConfHigh_
		;
	/// Combined constant mask value for CRH hardware register
	static constexpr uint32_t kCrhMask_ =
		Pin0::kModeConfHighMask_ & Pin1::kModeConfHighMask_
		& Pin2::kModeConfHighMask_ & Pin3::kModeConfHighMask_
		& Pin4::kModeConfHighMask_ & Pin5::kModeConfHighMask_
		& Pin6::kModeConfHighMask_ & Pin7::kModeConfHighMask_
		& Pin8::kModeConfHighMask_ & Pin9::kModeConfHighMask_
		& Pin10::kModeConfHighMask_ & Pin11::kModeConfHighMask_
		& Pin12::kModeConfHighMask_ & Pin13::kModeConfHighMask_
		& Pin14::kModeConfHighMask_ & Pin15::kModeConfHighMask_
		;
	/// Constant for the initial bit level
	static constexpr uint32_t kOdr_ =
		Pin0::kInitialLevel_ | Pin1::kInitialLevel_ 
		| Pin2::kInitialLevel_ | Pin3::kInitialLevel_ 
		| Pin4::kInitialLevel_ | Pin5::kInitialLevel_ 
		| Pin6::kInitialLevel_ | Pin7::kInitialLevel_ 
		| Pin8::kInitialLevel_ | Pin9::kInitialLevel_ 
		| Pin10::kInitialLevel_ | Pin11::kInitialLevel_ 
		| Pin12::kInitialLevel_ | Pin13::kInitialLevel_ 
		| Pin14::kInitialLevel_ | Pin15::kInitialLevel_
		;
	/// Effective combined bit constant value
	static constexpr uint32_t kBitValue_ =
		Pin0::kBitValue_ | Pin1::kBitValue_
		| Pin2::kBitValue_ | Pin3::kBitValue_
		| Pin4::kBitValue_ | Pin5::kBitValue_
		| Pin6::kBitValue_ | Pin7::kBitValue_
		| Pin8::kBitValue_ | Pin9::kBitValue_
		| Pin10::kBitValue_ | Pin11::kBitValue_
		| Pin12::kBitValue_ | Pin13::kBitValue_
		| Pin14::kBitValue_ | Pin15::kBitValue_
		;
	/// Combined Alternate Function configuration constant
	static constexpr uint32_t kAfConf_ =
		Pin0::kAfConf_ | Pin1::kAfConf_
		| Pin2::kAfConf_ | Pin3::kAfConf_
		| Pin4::kAfConf_ | Pin5::kAfConf_
		| Pin6::kAfConf_ | Pin7::kAfConf_
		| Pin8::kAfConf_ | Pin9::kAfConf_
		| Pin10::kAfConf_ | Pin11::kAfConf_
		| Pin12::kAfConf_ | Pin13::kAfConf_
		| Pin14::kAfConf_ | Pin15::kAfConf_
		;
	/// Combined Alternate Function configuration mask constant (inverted)
	static constexpr uint32_t kAfMask_ =
		Pin0::kAfMask_ | Pin1::kAfMask_
		| Pin2::kAfMask_ | Pin3::kAfMask_
		| Pin4::kAfMask_ | Pin5::kAfMask_
		| Pin6::kAfMask_ | Pin7::kAfMask_
		| Pin8::kAfMask_ | Pin9::kAfMask_
		| Pin10::kAfMask_ | Pin11::kAfMask_
		| Pin12::kAfMask_ | Pin13::kAfMask_
		| Pin14::kAfMask_ | Pin15::kAfMask_
		;
	/// Combined constant Flag indicating that no Alternate Function is required
	static constexpr bool kAfDisabled_ =
		Pin0::kAfDisabled_ & Pin1::kAfDisabled_
		& Pin2::kAfDisabled_ & Pin3::kAfDisabled_
		& Pin4::kAfDisabled_ & Pin5::kAfDisabled_
		& Pin6::kAfDisabled_ & Pin7::kAfDisabled_
		& Pin8::kAfDisabled_ & Pin9::kAfDisabled_
		& Pin10::kAfDisabled_ & Pin11::kAfDisabled_
		& Pin12::kAfDisabled_ & Pin13::kAfDisabled_
		& Pin14::kAfDisabled_ & Pin15::kAfDisabled_
		;

	/// Access to the hardware IO data structure
	ALWAYS_INLINE static volatile GPIO_TypeDef& Io() { return *(volatile GPIO_TypeDef*)kPortBase_; }

	/// Initialize to Port assuming the first use of all GPIO pins
	ALWAYS_INLINE static void Init(void)
	{
		/*
		** Note all constants (i.e. constexpr) are resolved at compile time and unused code is stripped 
		** out by compiler, even for an unoptimized build.
		*/

		// Compilation will fail here if GPIO port number of pin does not match that of the peripheral!!!
		static_assert(
			(Pin0::kPort_ == kUnusedPort || Pin0::kPort_ == kPort_)
			&& (Pin1::kPort_ == kUnusedPort || Pin1::kPort_ == kPort_)
			&& (Pin2::kPort_ == kUnusedPort || Pin2::kPort_ == kPort_)
			&& (Pin3::kPort_ == kUnusedPort || Pin3::kPort_ == kPort_)
			&& (Pin4::kPort_ == kUnusedPort || Pin4::kPort_ == kPort_)
			&& (Pin5::kPort_ == kUnusedPort || Pin5::kPort_ == kPort_)
			&& (Pin6::kPort_ == kUnusedPort || Pin6::kPort_ == kPort_)
			&& (Pin7::kPort_ == kUnusedPort || Pin7::kPort_ == kPort_)
			&& (Pin8::kPort_ == kUnusedPort || Pin8::kPort_ == kPort_)
			&& (Pin9::kPort_ == kUnusedPort || Pin9::kPort_ == kPort_)
			&& (Pin10::kPort_ == kUnusedPort || Pin10::kPort_ == kPort_)
			&& (Pin11::kPort_ == kUnusedPort || Pin11::kPort_ == kPort_)
			&& (Pin12::kPort_ == kUnusedPort || Pin12::kPort_ == kPort_)
			&& (Pin13::kPort_ == kUnusedPort || Pin13::kPort_ == kPort_)
			&& (Pin14::kPort_ == kUnusedPort || Pin14::kPort_ == kPort_)
			&& (Pin15::kPort_ == kUnusedPort || Pin15::kPort_ == kPort_)
			, "Inconsistent port number"
			);

		// Compilation will fail here if one GPIO pin number does not match its **position**
		static_assert(
			Pin0::kPin_ == 0 && Pin1::kPin_ == 1 && Pin2::kPin_ == 2 && Pin3::kPin_ == 3
			&& Pin4::kPin_ == 4 && Pin5::kPin_ == 5 && Pin6::kPin_ == 6 && Pin7::kPin_ == 7
			&& Pin8::kPin_ == 8 && Pin9::kPin_ == 9 && Pin10::kPin_ == 10 && Pin11::kPin_ == 11
			&& Pin12::kPin_ == 12 && Pin13::kPin_ == 13 && Pin14::kPin_ == 14 && Pin15::kPin_ == 15
			, "Inconsistent pin position"
			);

		// Base address of the peripheral registers
		volatile GPIO_TypeDef &port = Io();
		// Don't turn alternate function clock on if not required
		if(kAfDisabled_)
			RCC->APB2ENR |= (1 << (kPort_ + RCC_APB2ENR_IOPAEN_Pos));
		else
			RCC->APB2ENR |= (1 << (kPort_ + RCC_APB2ENR_IOPAEN_Pos)) | RCC_APB2ENR_AFIOEN;
		port.CRL = kCrl_;
		port.CRH = kCrh_;
		port.ODR = kOdr_;
		// Apply Alternate Function configuration
		AfRemapTemplate<kAfConf_, kAfMask_>::Enable();
	}
	//! Apply state of pin group merging with previous GPI contents
	ALWAYS_INLINE static void Enable(void)
	{
		// Base address of the peripheral registers
		volatile GPIO_TypeDef& port = Io();
		port.CRL = (port.CRL & kCrlMask_) | kCrl_;
		port.CRH = (port.CRH & kCrhMask_) | kCrh_;
		port.ODR = (port.ODR & ~kBitValue_) | kOdr_;
		// Apply Alternate Function configuration
		AfRemapTemplate<kAfConf_, kAfMask_>::Enable();
	}
	//! Not an ideal approach, but float everything
	ALWAYS_INLINE static void Disable(void)
	{
		// Base address of the peripheral registers
		volatile GPIO_TypeDef& port = Io();
		RCC->APB2ENR |= (1 << (kPort_ + RCC_APB2ENR_IOPAEN_Pos));
		port.CRL = 0x44444444;
		port.CRH = 0x44444444;
		// Remove bits applying inverted Alternate Function configuration mask constant
		AFIO->MAPR &= kAfMask_;
		RCC->APB2ENR &= ~(1 << (kPort_ + RCC_APB2ENR_IOPAEN_Pos));
	}
};

/// Keeps a copy of the current GPIO state and restores on scope exit
/*!
This class is useful to save current state of the GPIO registers and restore them 
on exit. This is useful when one wants to perform simple changes on the GPIO 
configuration for a short period and later restore to the previous state.

Note that this affects all bits of the port.
*/
template<const GpioPortId kPort>
class SaveGpio
{
public:
	static constexpr GpioPortId kPort_ = kPort;
	static constexpr uint32_t kPortBase_ = (GPIOA_BASE + kPort_ * 0x400);

	/// Access to the hardware IO data structure
	ALWAYS_INLINE static volatile GPIO_TypeDef& Io() { return *(volatile GPIO_TypeDef*)kPortBase_; }

	/// Keeps a copy of the current GPIO state and restores on scope exit
	SaveGpio()
	{
		// Base address of the peripheral registers
		volatile GPIO_TypeDef& port = Io();
		// Make a copy of the hardware registers
		odr_ = port.ODR;
		crl_ = port.CRL;
		crh_ = port.CRH;
		mapr_ = AFIO->MAPR;
	}
	~SaveGpio()
	{
		// Base address of the peripheral registers
		volatile GPIO_TypeDef& port = Io();
		// REstores all hardware registers
		port.ODR = odr_;
		port.CRL = crl_;
		port.CRH = crh_;
		AFIO->MAPR = mapr_;
	}

protected:
	/// Copy of the ODR hardware register
	uint32_t odr_;
	/// Copy of the CRL hardware register
	uint32_t crl_;
	/// Copy of the CRH hardware register
	uint32_t crh_;
	/// Copy of the MAPR hardware register
	uint32_t mapr_;
};


// ADC12
/// A default configuration for ADC12 IN0 on PA0
typedef GpioTemplate<PA, 0, kInput, kAnalog> ADC12_IN0;
/// A default configuration for ADC12 IN1 on PA1
typedef GpioTemplate<PA, 1, kInput, kAnalog> ADC12_IN1;
/// A default configuration for ADC12 IN2 on PA2
typedef GpioTemplate<PA, 2, kInput, kAnalog> ADC12_IN2;
/// A default configuration for ADC12 IN3 on PA3
typedef GpioTemplate<PA, 3, kInput, kAnalog> ADC12_IN3;
/// A default configuration for ADC12 IN4 on PA4
typedef GpioTemplate<PA, 4, kInput, kAnalog> ADC12_IN4;
/// A default configuration for ADC12 IN5 on PA5
typedef GpioTemplate<PA, 5, kInput, kAnalog> ADC12_IN5;
/// A default configuration for ADC12 IN6 on PA6
typedef GpioTemplate<PA, 6, kInput, kAnalog> ADC12_IN6;
/// A default configuration for ADC12 IN7 on PA7
typedef GpioTemplate<PA, 7, kInput, kAnalog> ADC12_IN7;
/// A default configuration for ADC12 IN8 on PB0
typedef GpioTemplate<PB, 0, kInput, kAnalog> ADC12_IN8;
/// A default configuration for ADC12 IN9 on PB1
typedef GpioTemplate<PB, 1, kInput, kAnalog> ADC12_IN9;
/// A default configuration for ADC12 IN10 on PC0
typedef GpioTemplate<PC, 0, kInput, kAnalog> ADC12_IN10;
/// A default configuration for ADC12 IN11 on PC1
typedef GpioTemplate<PC, 1, kInput, kAnalog> ADC12_IN11;
/// A default configuration for ADC12 IN12 on PC2
typedef GpioTemplate<PC, 2, kInput, kAnalog> ADC12_IN12;
/// A default configuration for ADC12 IN13 on PC3
typedef GpioTemplate<PC, 3, kInput, kAnalog> ADC12_IN13;
/// A default configuration for ADC12 IN14 on PC4
typedef GpioTemplate<PC, 4, kInput, kAnalog> ADC12_IN14;
/// A default configuration for ADC12 IN15 on PC5
typedef GpioTemplate<PC, 5, kInput, kAnalog> ADC12_IN15;


// CAN - Configuration 1 (cannot mix pins between configuration)
/// A default configuration for CAN/RX on PA11 pin
typedef GpioTemplate<PA, 11, kInput, kInputPushPull, kHigh, AfCan_PA11_12>				CAN_RX_PA11;
/// A default configuration for CAN/TX on PA12 pin
typedef GpioTemplate<PA, 12, kOutput50MHz, kAlternateOpenDrain, kLow, AfCan_PA11_12>	CAN_TX_PA12;

// CAN - Configuration 2 (cannot mix pins between configuration)
/// A default configuration for CAN/RX on PB8 pin
typedef GpioTemplate<PB, 8, kInput, kInputPushPull, kHigh, AfCan_PB8_9>				CAN_RX_PB8;
/// A default configuration for CAN/TX on PB9 pin
typedef GpioTemplate<PB, 9, kOutput50MHz, kAlternateOpenDrain, kLow, AfCan_PB8_9>	CAN_TX_PB9;

// CAN - Configuration 3 (cannot mix pins between configuration)
/// A default configuration for CAN/RX on PD0 pin
typedef GpioTemplate<PD, 0, kInput, kInputPushPull, kHigh, AfCan_PD0_1>				CAN_RX_PD0;
/// A default configuration for CAN/TX on PD1 pin
typedef GpioTemplate<PD, 1, kOutput50MHz, kAlternateOpenDrain, kLow, AfCan_PD0_1>	CAN_TX_PD1;


// GPIO vs Oscillator
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct ALT_PD0 : GpioTemplate<PD, 0, kMode, kConf, LVL, Af_PD01_GPIO> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct ALT_PD1 : GpioTemplate<PD, 1, kMode, kConf, LVL, Af_PD01_GPIO> {};

// I2C1 - Configuration 1
typedef GpioTemplate<PB, 6, kOutput50MHz, kAlternateOpenDrain, kLow, AfI2C1_PB6_7>	I2C1_SCL_PB6;
typedef GpioTemplate<PB, 7, kOutput50MHz, kAlternateOpenDrain, kLow, AfI2C1_PB6_7>	I2C1_SDA_PB7;
// I2C1 - Configuration 2
typedef GpioTemplate<PB, 8, kOutput50MHz, kAlternateOpenDrain, kLow, AfI2C1_PB8_9>	I2C1_SCL_PB8;
typedef GpioTemplate<PB, 9, kOutput50MHz, kAlternateOpenDrain, kLow, AfI2C1_PB8_9>	I2C1_SDA_PB9;
// I2C1 - Configuration 1 & 2
typedef GpioTemplate<PB, 5, kOutput50MHz, kAlternateOpenDrain, kLow, AfNoRemap>		I2C1_SMBAI_PB5;

// I2C2
typedef GpioTemplate<PB, 10, kOutput50MHz, kAlternateOpenDrain, kLow, AfNoRemap>	I2C2_SCL_PB10;
typedef GpioTemplate<PB, 11, kOutput50MHz, kAlternateOpenDrain, kLow, AfNoRemap>	I2C2_SDA_PB11;
typedef GpioTemplate<PB, 12, kOutput50MHz, kAlternateOpenDrain, kLow, AfNoRemap>	I2C2_SMBAI_PB12;

// SPI1 - Configuration 1
typedef GpioTemplate<PA, 4, kOutput50MHz, kAlternateOpenDrain, kHigh, AfSpi1_PA4_5_6_7>	SPI1_NSS_PA4;
typedef GpioTemplate<PA, 4, kInput, kInputPushPull, kHigh, AfSpi1_PA4_5_6_7>			SPI1_NSS_PA4_SLAVE;
typedef GpioTemplate<PA, 5, kOutput50MHz, kAlternatePushPull, kLow, AfSpi1_PA4_5_6_7>	SPI1_SCK_PA5;
typedef GpioTemplate<PA, 6, kInput, kInputPushPull, kHigh, AfSpi1_PA4_5_6_7>			SPI1_MISO_PA6;
typedef GpioTemplate<PA, 7, kOutput50MHz, kAlternatePushPull, kHigh, AfSpi1_PA4_5_6_7>	SPI1_MOSI_PA7;
// SPI1 - Configuration 2
typedef GpioTemplate<PA, 15, kOutput50MHz, kAlternateOpenDrain, kHigh, AfSpi1_PA15_PB3_4_5>	SPI1_NSS_PA15;
typedef GpioTemplate<PA, 15, kInput, kInputPushPull, kHigh, AfSpi1_PA15_PB3_4_5>			SPI1_NSS_PA15_SLAVE;
typedef GpioTemplate<PB, 3, kOutput50MHz, kAlternatePushPull, kLow, AfSpi1_PA15_PB3_4_5>	SPI1_SCK_PB3;
typedef GpioTemplate<PB, 4, kInput, kInputPushPull, kHigh, AfSpi1_PA15_PB3_4_5>				SPI1_MISO_PB4;
typedef GpioTemplate<PB, 5, kOutput50MHz, kAlternatePushPull, kHigh, AfSpi1_PA15_PB3_4_5>	SPI1_MOSI_PB5;

// SPI2
typedef GpioTemplate<PB, 12, kOutput50MHz, kAlternateOpenDrain, kHigh, AfNoRemap>	SPI2_NSS_PB12;
typedef GpioTemplate<PB, 12, kInput, kInputPushPull, kHigh, AfNoRemap>				SPI2_NSS_PB12_SLAVE;
typedef GpioTemplate<PB, 13, kOutput50MHz, kAlternatePushPull, kLow, AfNoRemap>		SPI2_SCK_PB13;
typedef GpioTemplate<PB, 14, kInput, kInputPushPull, kHigh, AfNoRemap>				SPI2_MISO_PB14;
typedef GpioTemplate<PB, 15, kOutput50MHz, kAlternatePushPull, kHigh, AfNoRemap>	SPI2_MOSI_PB15;

// TIM1 - Configuration 1
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM1_ETR_PA12 : GpioTemplate<PA, 12, kMode, kConf, LVL, AfTim1_PA12_8_9_10_11_PB12_13_14_15> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM1_CH1_PA8 : GpioTemplate<PA, 8, kMode, kConf, LVL, AfTim1_PA12_8_9_10_11_PB12_13_14_15> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM1_CH2_PA9 : GpioTemplate<PA, 9, kMode, kConf, LVL, AfTim1_PA12_8_9_10_11_PB12_13_14_15> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM1_CH3_PA10 : GpioTemplate<PA, 10, kMode, kConf, LVL, AfTim1_PA12_8_9_10_11_PB12_13_14_15> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM1_CH4_PA11 : GpioTemplate<PA, 11, kMode, kConf, LVL, AfTim1_PA12_8_9_10_11_PB12_13_14_15> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM1_BKIN_PB12 : GpioTemplate<PB, 12, kMode, kConf, LVL, AfTim1_PA12_8_9_10_11_PB12_13_14_15> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM1_CH1N_PB13 : GpioTemplate<PB, 13, kMode, kConf, LVL, AfTim1_PA12_8_9_10_11_PB12_13_14_15> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM1_CH2N_PB14 : GpioTemplate<PB, 14, kMode, kConf, LVL, AfTim1_PA12_8_9_10_11_PB12_13_14_15> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM1_CH3N_PB15 : GpioTemplate<PB, 15, kMode, kConf, LVL, AfTim1_PA12_8_9_10_11_PB12_13_14_15> {};
typedef GpioTemplate<PA, 8, kOutput50MHz, kAlternatePushPull, kLow, AfTim1_PA12_8_9_10_11_PB12_13_14_15>	TIM1_CH1_PA8_OUT;
typedef GpioTemplate<PA, 9, kOutput50MHz, kAlternatePushPull, kLow, AfTim1_PA12_8_9_10_11_PB12_13_14_15>	TIM1_CH2_PA9_OUT;
typedef GpioTemplate<PA, 10, kOutput50MHz, kAlternatePushPull, kLow, AfTim1_PA12_8_9_10_11_PB12_13_14_15>	TIM1_CH3_PA10_OUT;
typedef GpioTemplate<PA, 11, kOutput50MHz, kAlternatePushPull, kLow, AfTim1_PA12_8_9_10_11_PB12_13_14_15>	TIM1_CH4_PA11_OUT;
typedef GpioTemplate<PB, 13, kOutput50MHz, kAlternatePushPull, kLow, AfTim1_PA12_8_9_10_11_PB12_13_14_15>	TIM1_CH1N_PB13_OUT;
typedef GpioTemplate<PB, 14, kOutput50MHz, kAlternatePushPull, kLow, AfTim1_PA12_8_9_10_11_PB12_13_14_15>	TIM1_CH2N_PB14_OUT;
typedef GpioTemplate<PB, 15, kOutput50MHz, kAlternatePushPull, kLow, AfTim1_PA12_8_9_10_11_PB12_13_14_15>	TIM1_CH3N_PB15_OUT;
typedef GpioTemplate<PA, 12, kInput, kFloating, kLow, AfTim1_PA12_8_9_10_11_PB12_13_14_15>					TIM1_ETR_PA12_IN;
typedef GpioTemplate<PA, 8, kInput, kFloating, kLow, AfTim1_PA12_8_9_10_11_PB12_13_14_15>					TIM1_CH1_PA8_IN;
typedef GpioTemplate<PA, 9, kInput, kFloating, kLow, AfTim1_PA12_8_9_10_11_PB12_13_14_15>					TIM1_CH2_PA9_IN;
typedef GpioTemplate<PA, 10, kInput, kFloating, kLow, AfTim1_PA12_8_9_10_11_PB12_13_14_15>					TIM1_CH3_PA10_IN;
typedef GpioTemplate<PA, 11, kInput, kFloating, kLow, AfTim1_PA12_8_9_10_11_PB12_13_14_15>					TIM1_CH4_PA11_IN;
typedef GpioTemplate<PB, 12, kInput, kFloating, kLow, AfTim1_PA12_8_9_10_11_PB12_13_14_15>					TIM1_BKIN_PB12_IN;
// TIM1 - Configuration 2 (partial remap)
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM1_ETR_PA12_P : GpioTemplate<PA, 12, kMode, kConf, LVL, AfTim1_PA12_8_9_10_11_6_7_PB0_1> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM1_CH1_PA8_P : GpioTemplate<PA, 8, kMode, kConf, LVL, AfTim1_PA12_8_9_10_11_6_7_PB0_1> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM1_CH2_PA9_P : GpioTemplate<PA, 9, kMode, kConf, LVL, AfTim1_PA12_8_9_10_11_6_7_PB0_1> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM1_CH3_PA10_P : GpioTemplate<PA, 10, kMode, kConf, LVL, AfTim1_PA12_8_9_10_11_6_7_PB0_1> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM1_CH4_PA11_P : GpioTemplate<PA, 11, kMode, kConf, LVL, AfTim1_PA12_8_9_10_11_6_7_PB0_1> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM1_BKIN_PA6_P : GpioTemplate<PA, 6, kMode, kConf, LVL, AfTim1_PA12_8_9_10_11_6_7_PB0_1> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM1_CH1N_PA7_P : GpioTemplate<PA, 7, kMode, kConf, LVL, AfTim1_PA12_8_9_10_11_6_7_PB0_1> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM1_CH2N_PB0_P : GpioTemplate<PB, 0, kMode, kConf, LVL, AfTim1_PA12_8_9_10_11_6_7_PB0_1> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM1_CH3N_PB1_P : GpioTemplate<PB, 1, kMode, kConf, LVL, AfTim1_PA12_8_9_10_11_6_7_PB0_1> {};
typedef GpioTemplate<PA, 8, kOutput50MHz, kAlternatePushPull, kLow, AfTim1_PA12_8_9_10_11_6_7_PB0_1>		TIM1_CH1_PA8_OUT_CFG2;
typedef GpioTemplate<PA, 9, kOutput50MHz, kAlternatePushPull, kLow, AfTim1_PA12_8_9_10_11_6_7_PB0_1>		TIM1_CH2_PA9_OUT_CFG2;
typedef GpioTemplate<PA, 10, kOutput50MHz, kAlternatePushPull, kLow, AfTim1_PA12_8_9_10_11_6_7_PB0_1>		TIM1_CH3_PA10_OUT_CFG2;
typedef GpioTemplate<PA, 11, kOutput50MHz, kAlternatePushPull, kLow, AfTim1_PA12_8_9_10_11_6_7_PB0_1>		TIM1_CH4_PA11_OUT_CFG2;
typedef GpioTemplate<PA, 7, kOutput50MHz, kAlternatePushPull, kLow, AfTim1_PA12_8_9_10_11_6_7_PB0_1>		TIM1_CH1N_PA7_OUT_CFG2;
typedef GpioTemplate<PB, 0, kOutput50MHz, kAlternatePushPull, kLow, AfTim1_PA12_8_9_10_11_6_7_PB0_1>		TIM1_CH2N_PB0_OUT_CFG2;
typedef GpioTemplate<PB, 1, kOutput50MHz, kAlternatePushPull, kLow, AfTim1_PA12_8_9_10_11_6_7_PB0_1>		TIM1_CH3N_PB1_OUT_CFG2;
typedef GpioTemplate<PA, 12, kInput, kFloating, kLow, AfTim1_PA12_8_9_10_11_6_7_PB0_1>						TIM1_ETR_PA12_IN_CFG2;
typedef GpioTemplate<PA, 8, kInput, kFloating, kLow, AfTim1_PA12_8_9_10_11_6_7_PB0_1>						TIM1_CH1_PA8_IN_CFG2;
typedef GpioTemplate<PA, 9, kInput, kFloating, kLow, AfTim1_PA12_8_9_10_11_6_7_PB0_1>						TIM1_CH2_PA9_IN_CFG2;
typedef GpioTemplate<PA, 10, kInput, kFloating, kLow, AfTim1_PA12_8_9_10_11_6_7_PB0_1>						TIM1_CH3_PA10_IN_CFG2;
typedef GpioTemplate<PA, 11, kInput, kFloating, kLow, AfTim1_PA12_8_9_10_11_6_7_PB0_1>						TIM1_CH4_PA11_IN_CFG2;
typedef GpioTemplate<PA, 6, kInput, kFloating, kLow, AfTim1_PA12_8_9_10_11_6_7_PB0_1>						TIM1_BKIN_PA6_IN_CFG2;
// TIM1 - Configuration 3
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM1_ETR_PE7 : GpioTemplate<PE, 7, kMode, kConf, LVL, AfTim1_PE7_9_11_13_14_15_8_10_12> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM1_CH1_PE9 : GpioTemplate<PE, 9, kMode, kConf, LVL, AfTim1_PE7_9_11_13_14_15_8_10_12> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM1_CH2_PE11 : GpioTemplate<PE, 11, kMode, kConf, LVL, AfTim1_PE7_9_11_13_14_15_8_10_12> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM1_CH3_PE13 : GpioTemplate<PE, 13, kMode, kConf, LVL, AfTim1_PE7_9_11_13_14_15_8_10_12> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM1_CH4_PE14 : GpioTemplate<PE, 14, kMode, kConf, LVL, AfTim1_PE7_9_11_13_14_15_8_10_12> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM1_BKIN_PE15 : GpioTemplate<PE, 15, kMode, kConf, LVL, AfTim1_PE7_9_11_13_14_15_8_10_12> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM1_CH1N_PE8 : GpioTemplate<PE, 8, kMode, kConf, LVL, AfTim1_PE7_9_11_13_14_15_8_10_12> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM1_CH2N_PE10 : GpioTemplate<PE, 10, kMode, kConf, LVL, AfTim1_PE7_9_11_13_14_15_8_10_12> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM1_CH3N_PE12 : GpioTemplate<PE, 12, kMode, kConf, LVL, AfTim1_PE7_9_11_13_14_15_8_10_12> {};
typedef GpioTemplate<PE, 8, kOutput50MHz, kAlternatePushPull, kLow, AfTim1_PE7_9_11_13_14_15_8_10_12>		TIM1_CH1N_PE7_OUT_CFG3;
typedef GpioTemplate<PE, 9, kOutput50MHz, kAlternatePushPull, kLow, AfTim1_PE7_9_11_13_14_15_8_10_12>		TIM1_CH1_PE9_OUT_CFG3;
typedef GpioTemplate<PE, 10, kOutput50MHz, kAlternatePushPull, kLow, AfTim1_PE7_9_11_13_14_15_8_10_12>		TIM1_CH2N_PE10_OUT_CFG3;
typedef GpioTemplate<PE, 11, kOutput50MHz, kAlternatePushPull, kLow, AfTim1_PE7_9_11_13_14_15_8_10_12>		TIM1_CH2_PE11_OUT_CFG3;
typedef GpioTemplate<PE, 12, kOutput50MHz, kAlternatePushPull, kLow, AfTim1_PE7_9_11_13_14_15_8_10_12>		TIM1_CH3N_PE12_OUT_CFG3;
typedef GpioTemplate<PE, 13, kOutput50MHz, kAlternatePushPull, kLow, AfTim1_PE7_9_11_13_14_15_8_10_12>		TIM1_CH3_PE13_OUT_CFG3;
typedef GpioTemplate<PE, 14, kOutput50MHz, kAlternatePushPull, kLow, AfTim1_PE7_9_11_13_14_15_8_10_12>		TIM1_CH4_PE14_OUT_CFG3;
typedef GpioTemplate<PE, 7, kInput, kFloating, kLow, AfTim1_PE7_9_11_13_14_15_8_10_12>						TIM1_ETR_PE7_IN_CFG3;
typedef GpioTemplate<PE, 9, kInput, kFloating, kLow, AfTim1_PE7_9_11_13_14_15_8_10_12>						TIM1_CH1_PE9_IN_CFG3;
typedef GpioTemplate<PE, 11, kInput, kFloating, kLow, AfTim1_PE7_9_11_13_14_15_8_10_12>						TIM1_CH2_PE11_IN_CFG3;
typedef GpioTemplate<PE, 13, kInput, kFloating, kLow, AfTim1_PE7_9_11_13_14_15_8_10_12>						TIM1_CH3_PE13_IN_CFG3;
typedef GpioTemplate<PE, 14, kInput, kFloating, kLow, AfTim1_PE7_9_11_13_14_15_8_10_12>						TIM1_CH4_PE14_IN_CFG3;
typedef GpioTemplate<PE, 15, kInput, kFloating, kLow, AfTim1_PE7_9_11_13_14_15_8_10_12>						TIM1_BKIN_PE15_IN_CFG3;

// TIM2 - Configuration 1 (no remap)
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM2_CH1_PA0 : GpioTemplate<PA, 0, kMode, kConf, LVL, AfTim2_PA0_1_2_3> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM2_CH2_PA1 : GpioTemplate<PA, 1, kMode, kConf, LVL, AfTim2_PA0_1_2_3> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM2_CH3_PA2 : GpioTemplate<PA, 2, kMode, kConf, LVL, AfTim2_PA0_1_2_3> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM2_CH4_PA3 : GpioTemplate<PA, 3, kMode, kConf, LVL, AfTim2_PA0_1_2_3> {};
typedef GpioTemplate<PA, 0, kOutput50MHz, kAlternatePushPull, kLow, AfTim2_PA0_1_2_3>	TIM2_CH1_PA0_OUT;
typedef GpioTemplate<PA, 1, kOutput50MHz, kAlternatePushPull, kLow, AfTim2_PA0_1_2_3>	TIM2_CH2_PA1_OUT;
typedef GpioTemplate<PA, 2, kOutput50MHz, kAlternatePushPull, kLow, AfTim2_PA0_1_2_3>	TIM2_CH3_PA2_OUT;
typedef GpioTemplate<PA, 3, kOutput50MHz, kAlternatePushPull, kLow, AfTim2_PA0_1_2_3>	TIM2_CH4_PA3_OUT;
typedef GpioTemplate<PA, 0, kInput, kFloating, kLow, AfTim2_PA0_1_2_3>					TIM2_ETR_PA0_IN;
typedef GpioTemplate<PA, 0, kInput, kFloating, kLow, AfTim2_PA0_1_2_3>					TIM2_CH1_PA0_IN;
typedef GpioTemplate<PA, 1, kInput, kFloating, kLow, AfTim2_PA0_1_2_3>					TIM2_CH2_PA1_IN;
typedef GpioTemplate<PA, 2, kInput, kFloating, kLow, AfTim2_PA0_1_2_3>					TIM2_CH3_PA2_IN;
typedef GpioTemplate<PA, 3, kInput, kFloating, kLow, AfTim2_PA0_1_2_3>					TIM2_CH4_PA3_IN;
// TIM2 - Configuration 2 (partial remap)
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM2_CH1_PA15_P : GpioTemplate<PA, 15, kMode, kConf, LVL, AfTim2_PA15_PB3_PA2_3> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM2_CH2_PB3_P : GpioTemplate<PB, 3, kMode, kConf, LVL, AfTim2_PA15_PB3_PA2_3> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM2_CH3_PA2_P : GpioTemplate<PA, 2, kMode, kConf, LVL, AfTim2_PA15_PB3_PA2_3> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM2_CH4_PA3_P : GpioTemplate<PA, 3, kMode, kConf, LVL, AfTim2_PA15_PB3_PA2_3> {};
typedef GpioTemplate<PA, 15, kOutput50MHz, kAlternatePushPull, kLow, AfTim2_PA15_PB3_PA2_3>	TIM2_CH1_PA15_OUT_CFG2;
typedef GpioTemplate<PB, 3, kOutput50MHz, kAlternatePushPull, kLow, AfTim2_PA15_PB3_PA2_3>	TIM2_CH2_PB3_OUT_CFG2;
typedef GpioTemplate<PA, 2, kOutput50MHz, kAlternatePushPull, kLow, AfTim2_PA15_PB3_PA2_3>	TIM2_CH3_PA2_OUT_CFG2;
typedef GpioTemplate<PA, 3, kOutput50MHz, kAlternatePushPull, kLow, AfTim2_PA15_PB3_PA2_3>	TIM2_CH4_PA3_OUT_CFG2;
typedef GpioTemplate<PA, 15, kInput, kFloating, kLow, AfTim2_PA15_PB3_PA2_3>				TIM2_ETR_PA15_IN_CFG2;
typedef GpioTemplate<PA, 15, kInput, kFloating, kLow, AfTim2_PA15_PB3_PA2_3>				TIM2_CH1_PA15_IN_CFG2;
typedef GpioTemplate<PB, 3, kInput, kFloating, kLow, AfTim2_PA15_PB3_PA2_3>					TIM2_CH2_PB3_IN_CFG2;
typedef GpioTemplate<PA, 2, kInput, kFloating, kLow, AfTim2_PA15_PB3_PA2_3>					TIM2_CH3_PA2_IN_CFG2;
typedef GpioTemplate<PA, 3, kInput, kFloating, kLow, AfTim2_PA15_PB3_PA2_3>					TIM2_CH4_PA3_IN_CFG2;
// TIM2 - Configuration 3 (partial remap)
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM2_CH1_PA0_P : GpioTemplate<PA, 0, kMode, kConf, LVL, AfTim2_PA0_1_PB10_11> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM2_CH2_PA1_P : GpioTemplate<PA, 1, kMode, kConf, LVL, AfTim2_PA0_1_PB10_11> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM2_CH3_PB10_P : GpioTemplate<PB, 10, kMode, kConf, LVL, AfTim2_PA0_1_PB10_11> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM2_CH4_PB11_P : GpioTemplate<PB, 11, kMode, kConf, LVL, AfTim2_PA0_1_PB10_11> {};
typedef GpioTemplate<PA, 0, kOutput50MHz, kAlternatePushPull, kLow, AfTim2_PA0_1_PB10_11>	TIM2_CH1_PA0_OUT_CFG3;
typedef GpioTemplate<PA, 1, kOutput50MHz, kAlternatePushPull, kLow, AfTim2_PA0_1_PB10_11>	TIM2_CH2_PA1_OUT_CFG3;
typedef GpioTemplate<PB, 10, kOutput50MHz, kAlternatePushPull, kLow, AfTim2_PA0_1_PB10_11>	TIM2_CH3_PB10_OUT_CFG3;
typedef GpioTemplate<PB, 11, kOutput50MHz, kAlternatePushPull, kLow, AfTim2_PA0_1_PB10_11>	TIM2_CH4_PB11_OUT_CFG3;
typedef GpioTemplate<PA, 0, kInput, kFloating, kLow, AfTim2_PA0_1_PB10_11>					TIM2_ETR_PA0_IN_CFG3;
typedef GpioTemplate<PA, 0, kInput, kFloating, kLow, AfTim2_PA0_1_PB10_11>					TIM2_CH1_PA0_IN_CFG3;
typedef GpioTemplate<PA, 1, kInput, kFloating, kLow, AfTim2_PA0_1_PB10_11>					TIM2_CH2_PA1_IN_CFG3;
typedef GpioTemplate<PB, 10, kInput, kFloating, kLow, AfTim2_PA0_1_PB10_11>					TIM2_CH3_PB10_IN_CFG3;
typedef GpioTemplate<PB, 11, kInput, kFloating, kLow, AfTim2_PA0_1_PB10_11>					TIM2_CH4_PB11_IN_CFG3;
// TIM2 - Configuration 4 (full remap)
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM2_CH1_PA15 : GpioTemplate<PA, 15, kMode, kConf, LVL, AfTim2_PA15_PB3_10_11> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM2_CH2_PB3 : GpioTemplate<PB, 3, kMode, kConf, LVL, AfTim2_PA15_PB3_10_11> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM2_CH3_PB10 : GpioTemplate<PB, 10, kMode, kConf, LVL, AfTim2_PA15_PB3_10_11> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM2_CH4_PB11 : GpioTemplate<PB, 11, kMode, kConf, LVL, AfTim2_PA15_PB3_10_11> {};
typedef GpioTemplate<PA, 15, kOutput50MHz, kAlternatePushPull, kLow, AfTim2_PA15_PB3_10_11>	TIM2_CH1_PA15_OUT_CFG4;
typedef GpioTemplate<PB, 3, kOutput50MHz, kAlternatePushPull, kLow, AfTim2_PA15_PB3_10_11>	TIM2_CH2_PB3_OUT_CFG4;
typedef GpioTemplate<PB, 10, kOutput50MHz, kAlternatePushPull, kLow, AfTim2_PA15_PB3_10_11>	TIM2_CH3_PB10_OUT_CFG4;
typedef GpioTemplate<PB, 11, kOutput50MHz, kAlternatePushPull, kLow, AfTim2_PA15_PB3_10_11>	TIM2_CH4_PB11_OUT_CFG4;
typedef GpioTemplate<PA, 15, kInput, kFloating, kLow, AfTim2_PA15_PB3_10_11>				TIM2_ETR_PA15_IN_CFG4;
typedef GpioTemplate<PA, 15, kInput, kFloating, kLow, AfTim2_PA15_PB3_10_11>				TIM2_CH1_PA15_IN_CFG4;
typedef GpioTemplate<PB, 3, kInput, kFloating, kLow, AfTim2_PA15_PB3_10_11>					TIM2_CH2_PB3_IN_CFG4;
typedef GpioTemplate<PB, 10, kInput, kFloating, kLow, AfTim2_PA15_PB3_10_11>				TIM2_CH3_PB10_IN_CFG4;
typedef GpioTemplate<PB, 11, kInput, kFloating, kLow, AfTim2_PA15_PB3_10_11>				TIM2_CH4_PB11_IN_CFG4;

// TIM3 - Configuration 1
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM3_CH1_PA6 : GpioTemplate<PA, 6, kMode, kConf, LVL, AfTim3_PA6_7_PB0_1> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM3_CH2_PA7 : GpioTemplate<PA, 7, kMode, kConf, LVL, AfTim3_PA6_7_PB0_1> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM3_CH3_PB0 : GpioTemplate<PB, 0, kMode, kConf, LVL, AfTim3_PA6_7_PB0_1> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM3_CH4_PB1 : GpioTemplate<PB, 1, kMode, kConf, LVL, AfTim3_PA6_7_PB0_1> {};
typedef GpioTemplate<PA, 6, kOutput50MHz, kAlternatePushPull, kLow, AfTim3_PA6_7_PB0_1>		TIM3_CH1_PA6_OUT;
typedef GpioTemplate<PA, 7, kOutput50MHz, kAlternatePushPull, kLow, AfTim3_PA6_7_PB0_1>		TIM3_CH2_PA7_OUT;
typedef GpioTemplate<PB, 0, kOutput50MHz, kAlternatePushPull, kLow, AfTim3_PA6_7_PB0_1>		TIM3_CH3_PB0_OUT;
typedef GpioTemplate<PB, 1, kOutput50MHz, kAlternatePushPull, kLow, AfTim3_PA6_7_PB0_1>		TIM3_CH4_PB1_OUT;
typedef GpioTemplate<PA, 6, kInput, kFloating, kLow, AfTim3_PA6_7_PB0_1>					TIM3_CH1_PA6_IN;
typedef GpioTemplate<PA, 7, kInput, kFloating, kLow, AfTim3_PA6_7_PB0_1>					TIM3_CH2_PA7_IN;
typedef GpioTemplate<PB, 0, kInput, kFloating, kLow, AfTim3_PA6_7_PB0_1>					TIM3_CH3_PB0_IN;
typedef GpioTemplate<PB, 1, kInput, kFloating, kLow, AfTim3_PA6_7_PB0_1>					TIM3_CH4_PB1_IN;
// TIM3 - Configuration 2 (partial remap)
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM3_CH1_PB4_P : GpioTemplate<PB, 4, kMode, kConf, LVL, AfTim3_PB4_5_0_1> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM3_CH2_PB5_P : GpioTemplate<PB, 5, kMode, kConf, LVL, AfTim3_PB4_5_0_1> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM3_CH3_PB0_P : GpioTemplate<PB, 0, kMode, kConf, LVL, AfTim3_PB4_5_0_1> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM3_CH4_PB1_P : GpioTemplate<PB, 1, kMode, kConf, LVL, AfTim3_PB4_5_0_1> {};
typedef GpioTemplate<PB, 4, kOutput50MHz, kAlternatePushPull, kLow, AfTim3_PB4_5_0_1>		TIM3_CH1_PB4_OUT_CFG2;
typedef GpioTemplate<PB, 5, kOutput50MHz, kAlternatePushPull, kLow, AfTim3_PB4_5_0_1>		TIM3_CH2_PB5_OUT_CFG2;
typedef GpioTemplate<PB, 0, kOutput50MHz, kAlternatePushPull, kLow, AfTim3_PB4_5_0_1>		TIM3_CH3_PB0_OUT_CFG2;
typedef GpioTemplate<PB, 1, kOutput50MHz, kAlternatePushPull, kLow, AfTim3_PB4_5_0_1>		TIM3_CH4_PB1_OUT_CFG2;
typedef GpioTemplate<PB, 4, kInput, kFloating, kLow, AfTim3_PB4_5_0_1>						TIM3_CH1_PB4_IN_CFG2;
typedef GpioTemplate<PB, 5, kInput, kFloating, kLow, AfTim3_PB4_5_0_1>						TIM3_CH2_PB5_IN_CFG2;
typedef GpioTemplate<PB, 0, kInput, kFloating, kLow, AfTim3_PB4_5_0_1>						TIM3_CH3_PB0_IN_CFG2;
typedef GpioTemplate<PB, 1, kInput, kFloating, kLow, AfTim3_PB4_5_0_1>						TIM3_CH4_PB1_IN_CFG2;
// TIM3 - Configuration 3 (full remap)
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM3_CH1_PC6 : GpioTemplate<PC, 6, kMode, kConf, LVL, AfTim3_PC6_7_8_9> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM3_CH2_PC7 : GpioTemplate<PC, 7, kMode, kConf, LVL, AfTim3_PC6_7_8_9> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM3_CH3_PC8 : GpioTemplate<PC, 8, kMode, kConf, LVL, AfTim3_PC6_7_8_9> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM3_CH4_PC9 : GpioTemplate<PC, 9, kMode, kConf, LVL, AfTim3_PC6_7_8_9> {};
typedef GpioTemplate<PC, 6, kOutput50MHz, kAlternatePushPull, kLow, AfTim3_PC6_7_8_9>		TIM3_CH1_PC6_OUT_CFG3;
typedef GpioTemplate<PC, 7, kOutput50MHz, kAlternatePushPull, kLow, AfTim3_PC6_7_8_9>		TIM3_CH2_PC7_OUT_CFG3;
typedef GpioTemplate<PC, 8, kOutput50MHz, kAlternatePushPull, kLow, AfTim3_PC6_7_8_9>		TIM3_CH3_PC8_OUT_CFG3;
typedef GpioTemplate<PC, 9, kOutput50MHz, kAlternatePushPull, kLow, AfTim3_PC6_7_8_9>		TIM3_CH4_PC9_OUT_CFG3;
typedef GpioTemplate<PC, 6, kInput, kFloating, kLow, AfTim3_PC6_7_8_9>						TIM3_CH1_PC6_IN_CFG3;
typedef GpioTemplate<PC, 7, kInput, kFloating, kLow, AfTim3_PC6_7_8_9>						TIM3_CH2_PC7_IN_CFG3;
typedef GpioTemplate<PC, 8, kInput, kFloating, kLow, AfTim3_PC6_7_8_9>						TIM3_CH3_PC8_IN_CFG3;
typedef GpioTemplate<PC, 9, kInput, kFloating, kLow, AfTim3_PC6_7_8_9>						TIM3_CH4_PC9_IN_CFG3;

// TIM4 - Configuration 1
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM4_CH1_PB6 : GpioTemplate<PB, 6, kMode, kConf, LVL, AfTim4_PB6_7_8_9> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM4_CH2_PB7 : GpioTemplate<PB, 7, kMode, kConf, LVL, AfTim4_PB6_7_8_9> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM4_CH3_PB8 : GpioTemplate<PB, 8, kMode, kConf, LVL, AfTim4_PB6_7_8_9> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM4_CH4_PB9 : GpioTemplate<PB, 9, kMode, kConf, LVL, AfTim4_PB6_7_8_9> {};
typedef GpioTemplate<PB, 6, kOutput50MHz, kAlternatePushPull, kLow, AfTim4_PB6_7_8_9>		TIM4_CH1_PB6_OUT;
typedef GpioTemplate<PB, 7, kOutput50MHz, kAlternatePushPull, kLow, AfTim4_PB6_7_8_9>		TIM4_CH2_PB7_OUT;
typedef GpioTemplate<PB, 8, kOutput50MHz, kAlternatePushPull, kLow, AfTim4_PB6_7_8_9>		TIM4_CH3_PB8_OUT;
typedef GpioTemplate<PB, 9, kOutput50MHz, kAlternatePushPull, kLow, AfTim4_PB6_7_8_9>		TIM4_CH4_PB9_OUT;
typedef GpioTemplate<PB, 6, kInput, kFloating, kLow, AfTim4_PB6_7_8_9>						TIM4_CH1_PB6_IN;
typedef GpioTemplate<PB, 7, kInput, kFloating, kLow, AfTim4_PB6_7_8_9>						TIM4_CH2_PB7_IN;
typedef GpioTemplate<PB, 8, kInput, kFloating, kLow, AfTim4_PB6_7_8_9>						TIM4_CH3_PB8_IN;
typedef GpioTemplate<PB, 9, kInput, kFloating, kLow, AfTim4_PB6_7_8_9>						TIM4_CH4_PB9_IN;
// TIM4 - Configuration 2
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM4_CH1_PD12 : GpioTemplate<PD, 12, kMode, kConf, LVL, AfTim4_PD12_13_14_15> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM4_CH2_PD13 : GpioTemplate<PD, 13, kMode, kConf, LVL, AfTim4_PD12_13_14_15> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM4_CH3_PD14 : GpioTemplate<PD, 14, kMode, kConf, LVL, AfTim4_PD12_13_14_15> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM4_CH4_PD15 : GpioTemplate<PD, 15, kMode, kConf, LVL, AfTim4_PD12_13_14_15> {};
typedef GpioTemplate<PD, 12, kOutput50MHz, kAlternatePushPull, kLow, AfTim4_PD12_13_14_15>		TIM4_CH1_PD12_OUT;
typedef GpioTemplate<PD, 13, kOutput50MHz, kAlternatePushPull, kLow, AfTim4_PD12_13_14_15>		TIM4_CH2_PD13_OUT;
typedef GpioTemplate<PD, 14, kOutput50MHz, kAlternatePushPull, kLow, AfTim4_PD12_13_14_15>		TIM4_CH3_PD14_OUT;
typedef GpioTemplate<PD, 15, kOutput50MHz, kAlternatePushPull, kLow, AfTim4_PD12_13_14_15>		TIM4_CH4_PD15_OUT;
typedef GpioTemplate<PD, 12, kInput, kFloating, kLow, AfTim4_PD12_13_14_15>						TIM4_CH1_PD12_IN;
typedef GpioTemplate<PD, 13, kInput, kFloating, kLow, AfTim4_PD12_13_14_15>						TIM4_CH2_PD13_IN;
typedef GpioTemplate<PD, 14, kInput, kFloating, kLow, AfTim4_PD12_13_14_15>						TIM4_CH3_PD14_IN;
typedef GpioTemplate<PD, 15, kInput, kFloating, kLow, AfTim4_PD12_13_14_15>						TIM4_CH4_PD15_IN;

// USART1 - Configuration 1
typedef GpioTemplate<PA, 9, kOutput50MHz, kAlternatePushPull, kLow, AfUsart1_PA9_10>	USART1_TX_PA9;
typedef GpioTemplate<PA, 10, kInput, kInputPushPull, kHigh, AfUsart1_PA9_10>			USART1_RX_PA10;
// USART1 - Configuration 2
typedef GpioTemplate<PB, 6, kOutput50MHz, kAlternatePushPull, kLow, AfUsart1_PB6_7>		USART1_TX_PB6;
typedef GpioTemplate<PB, 7, kInput, kInputPushPull, kHigh, AfUsart1_PB6_7>				USART1_RX_PB7;
// USART1 - Configuration 1 & 2
typedef GpioTemplate<PA, 8, kOutput50MHz, kAlternatePushPull, kLow, AfNoRemap>			USART1_CK_PA8;
typedef GpioTemplate<PA, 11, kInput, kInputPushPull, kHigh, AfNoRemap>					USART1_CT2_PA11;
typedef GpioTemplate<PA, 12, kOutput50MHz, kAlternatePushPull, kLow, AfNoRemap>			USART1_RTS_PA12;

// USART2 - Configuration 1
typedef GpioTemplate<PA, 0, kInput, kInputPushPull, kHigh, AfUsart2_PA0_1_2_3_4>			USART2_CT2_PA0;
typedef GpioTemplate<PA, 1, kOutput50MHz, kAlternatePushPull, kLow, AfUsart2_PA0_1_2_3_4>	USART2_RTS_PA1;
typedef GpioTemplate<PA, 2, kOutput50MHz, kAlternatePushPull, kLow, AfUsart2_PA0_1_2_3_4>	USART2_TX_PA2;
typedef GpioTemplate<PA, 3, kInput, kInputPushPull, kHigh, AfUsart2_PA0_1_2_3_4>			USART2_RX_PA3;
typedef GpioTemplate<PA, 4, kOutput50MHz, kAlternatePushPull, kLow, AfUsart2_PA0_1_2_3_4>	USART2_CK_PA4;
// USART2 - Configuration 2
typedef GpioTemplate<PD, 3, kInput, kInputPushPull, kHigh, AfUsart2_PD3_4_5_6_7>			USART2_CT2_PD3;
typedef GpioTemplate<PD, 4, kOutput50MHz, kAlternatePushPull, kLow, AfUsart2_PD3_4_5_6_7>	USART2_RTS_PD4;
typedef GpioTemplate<PD, 5, kOutput50MHz, kAlternatePushPull, kLow, AfUsart2_PD3_4_5_6_7>	USART2_TX_PD5;
typedef GpioTemplate<PD, 6, kInput, kInputPushPull, kHigh, AfUsart2_PD3_4_5_6_7>			USART2_RX_PD6;
typedef GpioTemplate<PD, 7, kOutput50MHz, kAlternatePushPull, kLow, AfUsart2_PD3_4_5_6_7>	USART2_CK_PD7;

// USART3 - Configuration 1
typedef GpioTemplate<PB, 10, kOutput50MHz, kAlternatePushPull, kLow, AfUsart3_PB10_11_12_13_14> 	USART3_TX_PB10;
typedef GpioTemplate<PB, 11, kInput, kInputPushPull, kHigh, AfUsart3_PB10_11_12_13_14> 				USART3_RX_PB11;
typedef GpioTemplate<PB, 12, kOutput50MHz, kAlternatePushPull, kLow, AfUsart3_PB10_11_12_13_14> 	USART3_CK_PB12;
// USART3 - Configuration 1 & 2
typedef GpioTemplate<PB, 13, kInput, kInputPushPull, kHigh, AfUsart3_PB10_11_12_13_14> 				USART3_CTS_PB13;
typedef GpioTemplate<PB, 14, kOutput50MHz, kAlternatePushPull, kLow, AfUsart3_PB10_11_12_13_14> 	USART3_RTS_PB14;
// USART3 - Configuration 2
typedef GpioTemplate<PC, 10, kOutput50MHz, kAlternatePushPull, kLow, AfUsart3_PC10_11_12_PB13_14> 	USART3_TX_PC10;
typedef GpioTemplate<PC, 11, kInput, kInputPushPull, kHigh, AfUsart3_PC10_11_12_PB13_14> 			USART3_RX_PC11;
typedef GpioTemplate<PC, 12, kOutput50MHz, kAlternatePushPull, kLow, AfUsart3_PC10_11_12_PB13_14> 	USART3_CK_PC12;
// USART3 - Configuration 3
typedef GpioTemplate<PD, 8, kOutput50MHz, kAlternatePushPull, kLow, AfUsart3_PD8_9_10_11_12> 	USART3_TX_PD9;
typedef GpioTemplate<PD, 9, kInput, kInputPushPull, kHigh, AfUsart3_PD8_9_10_11_12> 			USART3_RX_PD9;
typedef GpioTemplate<PD, 10, kOutput50MHz, kAlternatePushPull, kLow, AfUsart3_PD8_9_10_11_12> 	USART3_CK_PD10;
typedef GpioTemplate<PD, 11, kInput, kInputPushPull, kHigh, AfUsart3_PD8_9_10_11_12> 			USART3_CTS_PD11;
typedef GpioTemplate<PD, 12, kOutput50MHz, kAlternatePushPull, kLow, AfUsart3_PD8_9_10_11_12> 	USART3_RTS_PD12;

// USB
typedef GpioTemplate<PA, 11, kOutput50MHz, kAlternatePushPull, kLow, AfNoRemap>		USBDM;
typedef GpioTemplate<PA, 12, kOutput50MHz, kAlternatePushPull, kLow, AfNoRemap>		USBDP;

// SWO
typedef GpioTemplate<PB, 3, kOutput50MHz, kAlternatePushPull, kHigh, AfNoRemap>		TRACESWO;

