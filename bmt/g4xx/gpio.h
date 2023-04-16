#pragma once

#include "mcu-system.h"
#include "pinremap.h"

namespace Bmt
{
namespace Gpio
{

namespace Private
{


/// Used for specific implementation behavior
enum class Impl
{
	kNormal,	///< Normal Pin functionality
	kUnused,	///< Pin that can be initialized to a passive state, only.
	kUnchanged,	///< No change allowed (use it to setup a partial group of pins)
};


template<
	const Impl kImpl							///< Behavior of this implementation
	, const Gpio::Port kPort					///< The GPIO port
	, const uint8_t kPin						///< The pin of the port
	, const Mode kMode = Mode::kInput			///< Mode to configure the port
	, const Speed kSpeed = Speed::kInput		///< Speed for the pin
	, const PuPd kPuPd = PuPd::kFloating		///< Additional pin configuration
	, const Level kLevel = Level::kLow			///< Initial pin level
	, typename Map = AfNoRemap					///< Pin remapping feature (pinremap.h)
>
class Implementation_
{
public:
	/// Constant storing the GPIO port number
	static constexpr Gpio::Port kPort_ = kPort;
	/// Base address of the port peripheral
	static constexpr uint32_t kPortBase_ = (GPIOA_BASE + uint32_t(kPort_) * 0x400);
	/// Constant storing the GPIO pin number
	static constexpr uint8_t kPin_ = kPin;
	/// Constant storing desired port speed
	static constexpr Speed kSpeed_ = kSpeed;
	/// Constant storing the port configuration
	static constexpr Mode kMode_ = kMode;
	/// Constant storing desired Pull-Up/Pull-Down configuration
	static constexpr PuPd kPuPd_ = kPuPd;
	/// Input function flag
	static constexpr bool kIsInput_ = kMode_ == Mode::kInput || kMode_ == Mode::kAnalog;
	/// Value for MODER without offset
	static constexpr uint8_t kMODER_Bits_ =
		kMode_ == Mode::kInput
			? 0b00
		: kMode_ == Mode::kOutput || kMode_ == Mode::kOpenDrain
			? 0b01
		: kMode_ == Mode::kAlternate || kMode_ == Mode::kOpenDrainAlt
			? 0b10
		// Mode::kAnalog
			: 0b11
		;
	/// Constant value for MODER hardware register
	static constexpr uint32_t kMODER_ = (kImpl == Impl::kUnchanged) ? 0UL
		: (uint32_t)kMODER_Bits_ << (kPin << 1);
	/// Constant mask value for MODER hardware register
	static constexpr uint32_t kMODER_Mask_ = (kImpl == Impl::kUnchanged) ? 0UL
		: ~(0b11UL << (kPin << 1));
	/// Constant value for OTYPER hardware register
	static constexpr uint32_t kOTYPER_ =
		(kMode_ != Mode::kOpenDrain && kMode_ != Mode::kOpenDrainAlt) || (kImpl == Impl::kUnchanged)
		? 0UL
		: (1UL << kPin)
		;
	/// Constant mask for OTYPER hardware register
	static constexpr uint32_t kOTYPER_Mask_ =
		kIsInput_ || (kImpl == Impl::kUnchanged)
		? 0UL
		: ~(1UL << kPin)
		;
	/// Constant value for OSPEEDR hardware register (no offset)
	static constexpr uint32_t kOSPEEDR_Bits_ =
		kSpeed_ == Speed::kFastest ? 0b11UL
		: kSpeed_ == Speed::kFast ? 0b10UL
		: kSpeed_ == Speed::kMedium ? 0b01UL
		: 0b00UL
		;
	/// Constant value for OSPEEDR hardware register
	static constexpr uint32_t kOSPEEDR_ = 
		kIsInput_ || (kImpl == Impl::kUnchanged) 
		? 0UL
		: kOSPEEDR_Bits_ << (kPin << 1);
	/// Constant mask value for OSPEEDR hardware register
	static constexpr uint32_t kOSPEEDR_Mask_ =
		kIsInput_ || (kImpl == Impl::kUnchanged)
		? 0UL
		: ~(0b11UL << (kPin << 1));
	/// Constant value for PUPD hardware register (no offset)
	static constexpr uint32_t kPUPDR_Bits_ =
		kMode_ == Mode::kAnalog ? 0UL
		: kPuPd_ == PuPd::kPullDown ? 0b10UL
		: kPuPd_ == PuPd::kPullUp ? 0b01UL
		: 0UL
		;
	/// Constant value for PUPD hardware register
	static constexpr uint32_t kPUPDR_ = 
		(kImpl != Impl::kUnchanged)
		? kPUPDR_Bits_ << (kPin << 1)
		: 0UL;
	/// Constant mask for PUPD hardware register
	static constexpr uint32_t kPUPDR_Mask_ = 
		(kImpl != Impl::kUnchanged)
		? ~(0b11UL << (kPin << 1))
		: 0UL;
	/// Effective bit constant value
	static constexpr uint16_t kBitValue_ = (kImpl != Impl::kNormal) ? 0
		: 1 << kPin;
	/// Value that clears the bit on the GPIOx_BSRR register
	static constexpr uint32_t kBsrrValue_ = (kImpl != Impl::kNormal) ? 0
		: 1 << (kPin + 16);
	/// Constant for the initial bit level
	static constexpr uint32_t kODR_ = (kImpl != Impl::kUnchanged) ? 0
		: uint32_t(kLevel) << kPin;
	/// Alternate Function configuration constant
	static constexpr uint32_t kAFRL_ = Map::kAFRL_;
	/// Alternate Function configuration mask constant (inverted)
	static constexpr uint32_t kAFRL_Mask_ = Map::kAFRL_Mask;
	/// Alternate Function configuration constant
	static constexpr uint32_t kAFRH_ = Map::kAFRH_;
	/// Alternate Function configuration mask constant (inverted)
	static constexpr uint32_t kAFRH_Mask_ = Map::kAFRH_Mask;
	/// Constant Flag indicating that no Alternate Function is required
	static constexpr bool kAfDisabled_ = Map::kNoRemap;
	/// Constant flag to indicate that a bit is not used for a particular configuration
	static constexpr bool kIsUnused_ = (kImpl != Impl::kNormal);

	/// Access to the peripheral memory space
	constexpr static volatile GPIO_TypeDef *Io() { return (volatile GPIO_TypeDef *)kPortBase_; }

	/// Apply default configuration for the pin.
	constexpr static void SetupPinMode(void)
	{
		volatile GPIO_TypeDef *port = Io();
		if (kMODER_Mask_ != 0UL)
			port->MODER = (port->MODER & kMODER_Mask_) | kMODER_;
		if (kOTYPER_Mask_ != 0UL)
			port->OTYPER = (port->OTYPER & kOTYPER_Mask_) | kOTYPER_;
		if (kOSPEEDR_Mask_ != 0UL)
			port->OSPEEDR = (port->OSPEEDR & kOSPEEDR_Mask_) | kOSPEEDR_;
		if (kPUPDR_Mask_ != 0UL)
			port->PUPDR = (port->PUPDR & kPUPDR_Mask_) | kPUPDR_;
	}
	/// Apply default configuration for the pin.
	constexpr static void Setup(void)
	{
		// Selected alternate function does not match pin. Functionality will break!
		static_assert(Map::kNoRemap || (Map::kPort_ == kPort_ && Map::kBit_ == kPin_));

		SetupPinMode();
		//Map::Enable();
		Set(uint32_t(kLevel) != 0);
	}
	/// Apply a custom configuration to the pin
	constexpr static void Setup(Mode mode, Speed speed, PuPd pupd)
	{
		volatile GPIO_TypeDef *port = Io();
		const bool is_input = mode == Mode::kInput || mode == Mode::kAnalog;
		/// Value for MODER without offset
		const uint8_t moder_bits =
			mode == Mode::kInput
			? 0b00
			: mode == Mode::kOutput || mode == Mode::kOpenDrain
			? 0b01
			: mode == Mode::kAlternate || mode == Mode::kOpenDrainAlt
			? 0b10
			// Mode::kAnalog
			: 0b11
			;
		/// Value for MODER hardware register
		const uint32_t moder = (uint32_t)moder_bits << (kPin << 1);
		/// Mask value for MODER hardware register
		const uint32_t moder_mask = ~(0b11UL << (kPin << 1));
		port->MODER = (port->MODER & moder_mask) | moder;
		//
		if (!is_input)
		{
			/// Value for OTYPER hardware register
			const uint32_t otyper =
				(mode != Mode::kOpenDrain && mode != Mode::kOpenDrainAlt)
				? 0UL
				: (1UL << kPin)
				;
			/// Mask for OTYPER hardware register
			const uint32_t otyper_mask = ~(1UL << kPin);
			port->OTYPER = (port->OTYPER & otyper_mask) | otyper;
			//
			/// Value for OSPEEDR hardware register (no offset)
			const uint32_t ospeedr_bits =
				speed == Speed::kFastest ? 0b11UL
				: speed == Speed::kFast ? 0b10UL
				: speed == Speed::kMedium ? 0b01UL
				: 0b00UL
				;
			/// Value for OSPEEDR hardware register
			const uint32_t ospeedr = ospeedr_bits << (kPin << 1);
			/// Mask value for OSPEEDR hardware register
			const uint32_t ospeedr_mask = ~(0b11UL << (kPin << 1));
			port->OSPEEDR = (port->OSPEEDR & ospeedr_mask) | ospeedr;
		}
		/// Constant value for PUPD hardware register (no offset)
		const uint32_t pupdr_bits =
			mode == Mode::kAnalog ? 0UL
			: pupd == PuPd::kPullDown ? 0b10UL
			: pupd == PuPd::kPullUp ? 0b01UL
			: 0UL
			;
		/// Constant value for MODER hardware register
		const uint32_t pupdr = pupdr_bits << (kPin << 1);
		/// Constant mask for MODER hardware register
		const uint32_t pupdr_mask = 0b11UL << (kPin << 1);
		port->PUPDR = (port->PUPDR & pupdr_mask) | pupdr;
	}

	/// Sets pin up. The pin will be high as long as it is configured as GPIO output
	constexpr static void SetHigh(void)
	{
		if (kBitValue_ != 0)
		{
			volatile GPIO_TypeDef *port = Io();
			port->BSRR = kBitValue_;
		}
	};

	/// Sets pin down. The pin will be low as long as it is configured as GPIO output
	constexpr static void SetLow(void)
	{
		if (kBitValue_ != 0)
		{
			volatile GPIO_TypeDef *port = Io();
			port->BRR = kBitValue_;
		}
	}

	/// Sets the pin to the given level. Note that optimizing compiler simplifies literal constants
	constexpr static void Set(const bool value)
	{
		if (value)
			SetHigh();
		else
			SetLow();
	}

	/// Reads current Pin electrical state
	constexpr static bool Get(void)
	{
		if (kBitValue_ != 0)
		{
			volatile GPIO_TypeDef *port = Io();
			return (port->IDR & kBitValue_) != 0;
		}
		else
			return false;
	}

	/// Checks if current pin electrical state is high
	constexpr static bool IsHigh(void)
	{
		if (kBitValue_ != 0)
		{
			volatile GPIO_TypeDef *port = Io();
			return (port->IDR & kBitValue_) != 0;
		}
		else
			return false;
	}

	/// Checks if current pin electrical state is low
	constexpr static bool IsLow(void)
	{
		if (kBitValue_ != 0)
		{
			volatile GPIO_TypeDef *port = Io();
			return (port->IDR & kBitValue_) == 0;
		}
		else
			return false;
	}

	/// Toggles pin state
	constexpr static void Toggle(void)
	{
		if (kBitValue_ != 0)
		{
			volatile GPIO_TypeDef *port = Io();
			port->ODR ^= kBitValue_;
		}
	}
};

}	// namespace Private


/*!
**	@brief Defines/Sets up a single GPIO pin
**
**	This template sets up a single GPIO pin. Methods allows one to bit bang pin
**	or read its input state.
**
**	An additional powerful feature is to combine all need GPIO pin definition
**	together into a AnyPortSetup<> data type, which is able to setup the
**	entire GPIO port in a couple of CPU instructions.
**
**	Example:
**		// Sets a data-type to drive an SPI1 CLK output
**		typedef GpioTemplate<Port::PA, 5, kOutput50MHz, Mode::kPushPull, Level::kHigh> MY_SPI_CLK;
**		// Sets a data-type to inactivate the pin defined before
**		typedef AnyInPu<Port::PA, 5> MY_INACTIVE_SPI_CLK;
**
**	Also see the shortcut templates that reduces the clutter to declare common
**	IO forms: Floating<>, AnyInPu<> and AnyInPd<>.
**
**	Device specific peripherals are also mapped into handy data-types, like for
**	example: SPI1_SCK_PA5, ADC12_IN0 and TIM2_CH2_PA1.
**
**	@tparam kPort: the GPIO port.
**	@tparam kPin: The GPIO pin number.
**	@tparam kMode: Defines pin direction.
**	@tparam kConf: Defines how the pin is driven or pulled up/down.
**	@tparam kLevel: Defines the level of the pin to be initialized.
**	@tparam Map: A data-type that allows STM32 Pin Remap. Definitions are found on remap.h.
*/
template<
	const Gpio::Port kPort						///< The GPIO port
	, const uint8_t kPin						///< The pin of the port
	, const Mode kMode = Mode::kInput			///< Mode to configure the port
	, const Speed kSpeed = Speed::kInput		///< Speed for the pin
	, const PuPd kPuPd = PuPd::kFloating		///< Additional pin configuration
	, const Level kLevel = Level::kLow			///< Initial pin level (applies to output pin)
	, typename Map = AfNoRemap					///< Pin remapping feature (pinremap.h)
>
class AnyPin : public Private::Implementation_ <
	Private::Impl::kNormal
	, kPort
	, kPin
	, kMode
	, kSpeed
	, kPuPd
	, kLevel
	, Map
>
{};


/// A template class representing an unused pin
template<
	const uint8_t kPin						///< the pin number
	, const PuPd kPuPd = PuPd::kPullDown	///< Always configured as input; PD/PU selectable
>
class Unused : public Private::Implementation_
	<
	Private::Impl::kUnused
	, Port::kUnusedPort
	, kPin
	, Mode::kInput
	, Speed::kInput
	, kPuPd
	>
{
};


/// A template pin configuration for a pin that should not be affected
template<
	const uint8_t kPin		///< Pin number is required
>
class Unchanged : public Private::Implementation_
	<
	Private::Impl::kUnchanged
	, Port::kUnusedPort
	, kPin
	>
{
};


//! Template for input pins
template<
	const Gpio::Port kPort						///< The GPIO port
	, const uint8_t kPin						///< The pin of the port
	, typename Map = AfNoRemap					///< Pin remapping feature (pinremap.h)
>
class AnyAnalog : public Private::Implementation_<
	Private::Impl::kNormal
	, kPort
	, kPin
	, Mode::kAnalog
	, Speed::kInput
	, PuPd::kFloating
	, Level::kLow
	, Map
>
{};


//! Template for input pins
template<
	const Gpio::Port kPort						///< The GPIO port
	, const uint8_t kPin						///< The pin of the port
	, const PuPd kPuPd = PuPd::kFloating		///< Additional pin configuration
	, typename Map = AfNoRemap					///< Pin remapping feature (pinremap.h)
>
class AnyIn : public Private::Implementation_<
	Private::Impl::kNormal
	, kPort
	, kPin
	, Mode::kInput
	, Speed::kInput
	, kPuPd
	, Level::kLow
	, Map
>
{};


/// A template class to configure a port pin as a floating input pin
template <
	const Gpio::Port kPort		///< The GPIO port
	, const uint8_t kPin		///< The GPIO pin number
	, typename Map = AfNoRemap	///< Pin remapping feature (pinremap.h)
>
class Floating : public AnyIn<kPort, kPin, PuPd::kFloating, Map>
{
};


/// A template class to configure a port pin as digital input having a pull-up
template <
	const Gpio::Port kPort
	, const uint8_t kPin
	, typename Map = AfNoRemap	///< Pin remapping feature (pinremap.h)
>
class AnyInPu : public AnyIn<kPort, kPin, PuPd::kPullUp, Map>
{
};


/// A template class to configure a port pin as digital input having a pull-down
template <
	const Gpio::Port kPort
	, const uint8_t kPin
	, typename Map = AfNoRemap	///< Pin remapping feature (pinremap.h)
>
class AnyInPd : public AnyIn<kPort, kPin, PuPd::kPullDown, Map>
{
};


//! Template for output pins
template<
	const Gpio::Port kPort						///< The GPIO port
	, const uint8_t kPin						///< The pin of the port
	, const Speed kSpeed = Speed::kFast			///< Speed for the pin
	, const Level kLevel = Level::kLow			///< Initial pin level
	, const PuPd kPuPd = PuPd::kFloating		///< Additional pin configuration
	, const Mode kMode = Mode::kOutput			///< Mode to configure the port
	, typename Map = AfNoRemap					///< Pin remapping feature (pinremap.h)
>
class AnyOut : public Private::Implementation_<
	Private::Impl::kNormal
	, kPort
	, kPin
	, kMode
	, kSpeed
	, kPuPd
	, kLevel
	, Map
>
{
private:
	constexpr void Validate_()
	{
		static_assert(kMode >= Mode::kOutput, "template requires an output pin configuration");
	}
};


//! Template for output pins
template<
	const Gpio::Port kPort						///< The GPIO port
	, const uint8_t kPin						///< The pin of the port
	, const Speed kSpeed = Speed::kFast			///< Speed for the pin
	, const Level kLevel = Level::kLow			///< Initial pin level (applies to output pin)
	, typename Map = AfNoRemap					///< Pin remapping feature (pinremap.h)
>
class AnyOutOD : public AnyOut<
	kPort
	, kPin
	, kSpeed
	, kLevel
	, PuPd::kPullUp
	, Mode::kOpenDrain
	, Map
>
{
};


//! Template for output pins
template<
	const Gpio::Port kPort						///< The GPIO port
	, const uint8_t kPin						///< The pin of the port
	, typename Map								///< Pin remapping feature (pinremap.h)
	, const Speed kSpeed = Speed::kFast			///< Speed for the pin
	, const Level kLevel = Level::kLow			///< Initial pin level (applies to output pin)
	, const PuPd kPuPd = PuPd::kFloating		///< Additional pin configuration
	, const Mode kMode = Mode::kAlternate		///< Mode to configure the port
>
class AnyAltOut : public Private::Implementation_<
	Private::Impl::kNormal
	, kPort
	, kPin
	, kMode
	, kSpeed
	, kPuPd
	, kLevel
	, Map
>
{
private:
	constexpr void Validate_()
	{
		static_assert(kMode >= Mode::kAlternate, "template requires an alternate output pin configuration");
	}
};


//! Template for Push-Pull alternate output pins
template<
	const Gpio::Port kPort						///< The GPIO port
	, const uint8_t kPin						///< The pin of the port
	, typename Map								///< Pin remapping feature (pinremap.h)
	, const Speed kSpeed = Speed::kFast			///< Speed for the pin
	, const Level kLevel = Level::kLow			///< Initial pin level (applies to output pin)
>
class AnyAltOutPP : public AnyAltOut <
	kPort
	, kPin
	, Map
	, kSpeed
	, kLevel
	, PuPd::kFloating
	, Mode::kAlternate
>
{
};


//! Template for Open-Drain alternate output pins
template<
	const Gpio::Port kPort						///< The GPIO port
	, const uint8_t kPin						///< The pin of the port
	, typename Map								///< Pin remapping feature (pinremap.h)
	, const Speed kSpeed = Speed::kFast			///< Speed for the pin
	, const Level kLevel = Level::kLow			///< Initial pin level (applies to output pin)
>
class AnyAltOutOD : public AnyAltOut <
	kPort
	, kPin
	, Map
	, kSpeed
	, kLevel
	, PuPd::kPullUp
	, Mode::kOpenDrainAlt
>
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
typedef GpioTemplate<Port::PA, 0, Speed::kOutput2MHzkOutput2MHz, Mode::kPushPull, Level::kHigh> GREEN_LED;
/// Initial configuration for PORTA
typedef AnyPortSetup <Port::PA
	, GREEN_LED			///< bit bang
	, Unused<1>		///< not used
	, Unused<2>		///< not used
	, Unused<3>		///< not used
	, Unused<4>		///< not used
	, Unused<5>		///< not used
	, Unused<6>		///< not used
	, Unused<7>		///< not used
	, Unused<8>		///< not used
	, USART1_TX_PA9		///< GDB UART port
	, USART1_RX_PA10	///< GDB UART port
	, Unused<11>		///< USB-
	, Unused<12>		///< USB+
	, Unused<13>		///< STM32 TMS/SWDIO
	, Unused<14>		///< STM32 TCK/SWCLK
	, Unused<15>		///< STM32 TDI
> PORTA;

void MyHardwareInit()
{
	// Configure ports
	PORTA::Init();
}
\endcode
*/
template <
	const Gpio::Port kPort				/// The GPIO port number
	, typename Pin0 = Unused<0>		/// Definition for bit 0 (defaults to unused pin, i.e an inputs)
	, typename Pin1 = Unused<1>		/// Definition for bit 1 (defaults to unused pin, i.e an inputs)
	, typename Pin2 = Unused<2>		/// Definition for bit 2 (defaults to unused pin, i.e an inputs)
	, typename Pin3 = Unused<3>		/// Definition for bit 3 (defaults to unused pin, i.e an inputs)
	, typename Pin4 = Unused<4>		/// Definition for bit 4 (defaults to unused pin, i.e an inputs)
	, typename Pin5 = Unused<5>		/// Definition for bit 5 (defaults to unused pin, i.e an inputs)
	, typename Pin6 = Unused<6>		/// Definition for bit 6 (defaults to unused pin, i.e an inputs)
	, typename Pin7 = Unused<7>		/// Definition for bit 7 (defaults to unused pin, i.e an inputs)
	, typename Pin8 = Unused<8>		/// Definition for bit 8 (defaults to unused pin, i.e an inputs)
	, typename Pin9 = Unused<9>		/// Definition for bit 9 (defaults to unused pin, i.e an inputs)
	, typename Pin10 = Unused<10>	/// Definition for bit 10 (defaults to unused pin, i.e an inputs)
	, typename Pin11 = Unused<11>	/// Definition for bit 11 (defaults to unused pin, i.e an inputs)
	, typename Pin12 = Unused<12>	/// Definition for bit 12 (defaults to unused pin, i.e an inputs)
	, typename Pin13 = Unused<13>	/// Definition for bit 13 (defaults to unused pin, i.e an inputs)
	, typename Pin14 = Unused<14>	/// Definition for bit 14 (defaults to unused pin, i.e an inputs)
	, typename Pin15 = Unused<15>	/// Definition for bit 15 (defaults to unused pin, i.e an inputs)
	>
class AnyPortSetup
{
public:
	/// The GPIO port peripheral
	static constexpr Gpio::Port kPort_ = kPort;
	/// The base address for the GPIO peripheral registers
	static constexpr uint32_t kPortBase_ = (GPIOA_BASE + uint32_t(kPort_) * 0x400);
	/// Combined constant value for MODER hardware register
	static constexpr uint32_t kMODER_ =
		Pin0::kMODER_ | Pin1::kMODER_
		| Pin2::kMODER_ | Pin3::kMODER_
		| Pin4::kMODER_ | Pin5::kMODER_
		| Pin6::kMODER_ | Pin7::kMODER_
		| Pin8::kMODER_ | Pin9::kMODER_
		| Pin10::kMODER_ | Pin11::kMODER_
		| Pin12::kMODER_ | Pin13::kMODER_
		| Pin14::kMODER_ | Pin15::kMODER_
		;
	/// Combined constant mask value for MODER hardware register
	static constexpr uint32_t kMODER_Mask_ =
		Pin0::kMODER_Mask_ & Pin1::kMODER_Mask_
		& Pin2::kMODER_Mask_ & Pin3::kMODER_Mask_
		& Pin4::kMODER_Mask_ & Pin5::kMODER_Mask_
		& Pin6::kMODER_Mask_ & Pin7::kMODER_Mask_
		& Pin8::kMODER_Mask_ & Pin9::kMODER_Mask_
		& Pin10::kMODER_Mask_ & Pin11::kMODER_Mask_
		& Pin12::kMODER_Mask_ & Pin13::kMODER_Mask_
		& Pin14::kMODER_Mask_ & Pin15::kMODER_Mask_
		;
	/// Combined constant value for OTYPER hardware register
	static constexpr uint32_t kOTYPER_ =
		Pin0::kOTYPER_ | Pin1::kOTYPER_
		| Pin2::kOTYPER_ | Pin3::kOTYPER_
		| Pin4::kOTYPER_ | Pin5::kOTYPER_
		| Pin6::kOTYPER_ | Pin7::kOTYPER_
		| Pin8::kOTYPER_ | Pin9::kOTYPER_
		| Pin10::kOTYPER_ | Pin11::kOTYPER_
		| Pin12::kOTYPER_ | Pin13::kOTYPER_
		| Pin14::kOTYPER_ | Pin15::kOTYPER_
		;
	/// Combined constant mask value for OTYPER hardware register
	static constexpr uint32_t kOTYPER_Mask_ =
		Pin0::kOTYPER_Mask_ & Pin1::kOTYPER_Mask_
		& Pin2::kOTYPER_Mask_ & Pin3::kOTYPER_Mask_
		& Pin4::kOTYPER_Mask_ & Pin5::kOTYPER_Mask_
		& Pin6::kOTYPER_Mask_ & Pin7::kOTYPER_Mask_
		& Pin8::kOTYPER_Mask_ & Pin9::kOTYPER_Mask_
		& Pin10::kOTYPER_Mask_ & Pin11::kOTYPER_Mask_
		& Pin12::kOTYPER_Mask_ & Pin13::kOTYPER_Mask_
		& Pin14::kOTYPER_Mask_ & Pin15::kOTYPER_Mask_
		;
	/// Combined constant value for OSPEEDR hardware register
	static constexpr uint32_t kOSPEEDR_ =
		Pin0::kOSPEEDR_ | Pin1::kOSPEEDR_
		| Pin2::kOSPEEDR_ | Pin3::kOSPEEDR_
		| Pin4::kOSPEEDR_ | Pin5::kOSPEEDR_
		| Pin6::kOSPEEDR_ | Pin7::kOSPEEDR_
		| Pin8::kOSPEEDR_ | Pin9::kOSPEEDR_
		| Pin10::kOSPEEDR_ | Pin11::kOSPEEDR_
		| Pin12::kOSPEEDR_ | Pin13::kOSPEEDR_
		| Pin14::kOSPEEDR_ | Pin15::kOSPEEDR_
		;
	/// Combined constant mask value for OSPEEDR hardware register
	static constexpr uint32_t kOSPEEDR_Mask_ =
		Pin0::kOSPEEDR_Mask_ & Pin1::kOSPEEDR_Mask_
		& Pin2::kOSPEEDR_Mask_ & Pin3::kOSPEEDR_Mask_
		& Pin4::kOSPEEDR_Mask_ & Pin5::kOSPEEDR_Mask_
		& Pin6::kOSPEEDR_Mask_ & Pin7::kOSPEEDR_Mask_
		& Pin8::kOSPEEDR_Mask_ & Pin9::kOSPEEDR_Mask_
		& Pin10::kOSPEEDR_Mask_ & Pin11::kOSPEEDR_Mask_
		& Pin12::kOSPEEDR_Mask_ & Pin13::kOSPEEDR_Mask_
		& Pin14::kOSPEEDR_Mask_ & Pin15::kOSPEEDR_Mask_
		;
	/// Combined constant value for PUPD hardware register
	static constexpr uint32_t kPUPDR_ =
		Pin0::kPUPDR_ | Pin1::kPUPDR_
		| Pin2::kPUPDR_ | Pin3::kPUPDR_
		| Pin4::kPUPDR_ | Pin5::kPUPDR_
		| Pin6::kPUPDR_ | Pin7::kPUPDR_
		| Pin8::kPUPDR_ | Pin9::kPUPDR_
		| Pin10::kPUPDR_ | Pin11::kPUPDR_
		| Pin12::kPUPDR_ | Pin13::kPUPDR_
		| Pin14::kPUPDR_ | Pin15::kPUPDR_
		;
	/// Combined constant mask value for PUPD hardware register
	static constexpr uint32_t kPUPDR_Mask_ =
		Pin0::kPUPDR_Mask_ & Pin1::kPUPDR_Mask_
		& Pin2::kPUPDR_Mask_ & Pin3::kPUPDR_Mask_
		& Pin4::kPUPDR_Mask_ & Pin5::kPUPDR_Mask_
		& Pin6::kPUPDR_Mask_ & Pin7::kPUPDR_Mask_
		& Pin8::kPUPDR_Mask_ & Pin9::kPUPDR_Mask_
		& Pin10::kPUPDR_Mask_ & Pin11::kPUPDR_Mask_
		& Pin12::kPUPDR_Mask_ & Pin13::kPUPDR_Mask_
		& Pin14::kPUPDR_Mask_ & Pin15::kPUPDR_Mask_
		;
	/// Constant for the initial bit level
	static constexpr uint32_t kODR_ =
		Pin0::kODR_ | Pin1::kODR_
		| Pin2::kODR_ | Pin3::kODR_
		| Pin4::kODR_ | Pin5::kODR_
		| Pin6::kODR_ | Pin7::kODR_
		| Pin8::kODR_ | Pin9::kODR_
		| Pin10::kODR_ | Pin11::kODR_
		| Pin12::kODR_ | Pin13::kODR_
		| Pin14::kODR_ | Pin15::kODR_
		;
	/// Constant for the AFRL register
	static constexpr uint32_t kAFRL_ =
		Pin0::kAFRL_ | Pin1::kAFRL_
		| Pin2::kAFRL_ | Pin3::kAFRL_
		| Pin4::kAFRL_ | Pin5::kAFRL_
		| Pin6::kAFRL_ | Pin7::kAFRL_
		| Pin8::kAFRL_ | Pin9::kAFRL_
		| Pin10::kAFRL_ | Pin11::kAFRL_
		| Pin12::kAFRL_ | Pin13::kAFRL_
		| Pin14::kAFRL_ | Pin15::kAFRL_
		;
	/// Constant mask for the AFRL register
	static constexpr uint32_t kAFRL_Mask_ =
		Pin0::kAFRL_Mask_ & Pin1::kAFRL_Mask_
		& Pin2::kAFRL_Mask_ & Pin3::kAFRL_Mask_
		& Pin4::kAFRL_Mask_ & Pin5::kAFRL_Mask_
		& Pin6::kAFRL_Mask_ & Pin7::kAFRL_Mask_
		& Pin8::kAFRL_Mask_ & Pin9::kAFRL_Mask_
		& Pin10::kAFRL_Mask_ & Pin11::kAFRL_Mask_
		& Pin12::kAFRL_Mask_ & Pin13::kAFRL_Mask_
		& Pin14::kAFRL_Mask_ & Pin15::kAFRL_Mask_
		;
	/// Constant for the AFRH register
	static constexpr uint32_t kAFRH_ =
		Pin0::kAFRH_ | Pin1::kAFRH_
		| Pin2::kAFRH_ | Pin3::kAFRH_
		| Pin4::kAFRH_ | Pin5::kAFRH_
		| Pin6::kAFRH_ | Pin7::kAFRH_
		| Pin8::kAFRH_ | Pin9::kAFRH_
		| Pin10::kAFRH_ | Pin11::kAFRH_
		| Pin12::kAFRH_ | Pin13::kAFRH_
		| Pin14::kAFRH_ | Pin15::kAFRH_
		;
	/// Constant mask for the AFRH register
	static constexpr uint32_t kAFRH_Mask_ =
		Pin0::kAFRH_Mask_ & Pin1::kAFRH_Mask_
		& Pin2::kAFRH_Mask_ & Pin3::kAFRH_Mask_
		& Pin4::kAFRH_Mask_ & Pin5::kAFRH_Mask_
		& Pin6::kAFRH_Mask_ & Pin7::kAFRH_Mask_
		& Pin8::kAFRH_Mask_ & Pin9::kAFRH_Mask_
		& Pin10::kAFRH_Mask_ & Pin11::kAFRH_Mask_
		& Pin12::kAFRH_Mask_ & Pin13::kAFRH_Mask_
		& Pin14::kAFRH_Mask_ & Pin15::kAFRH_Mask_
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

	/// Access to the hardware IO data structure
	constexpr static volatile GPIO_TypeDef& Io() { return *(volatile GPIO_TypeDef*)kPortBase_; }

	/// Initialize to Port assuming the first use of all GPIO pins
	constexpr static void Init(void)
	{
		/*
		** Note all constants (i.e. constexpr) are resolved at compile time and unused code is stripped 
		** out by compiler, even for an unoptimized build.
		*/

		// Compilation will fail here if GPIO port number of pin does not match that of the peripheral!!!
		static_assert(
			(Pin0::kPort_ == Port::kUnusedPort || Pin0::kPort_ == kPort_)
			&& (Pin1::kPort_ == Port::kUnusedPort || Pin1::kPort_ == kPort_)
			&& (Pin2::kPort_ == Port::kUnusedPort || Pin2::kPort_ == kPort_)
			&& (Pin3::kPort_ == Port::kUnusedPort || Pin3::kPort_ == kPort_)
			&& (Pin4::kPort_ == Port::kUnusedPort || Pin4::kPort_ == kPort_)
			&& (Pin5::kPort_ == Port::kUnusedPort || Pin5::kPort_ == kPort_)
			&& (Pin6::kPort_ == Port::kUnusedPort || Pin6::kPort_ == kPort_)
			&& (Pin7::kPort_ == Port::kUnusedPort || Pin7::kPort_ == kPort_)
			&& (Pin8::kPort_ == Port::kUnusedPort || Pin8::kPort_ == kPort_)
			&& (Pin9::kPort_ == Port::kUnusedPort || Pin9::kPort_ == kPort_)
			&& (Pin10::kPort_ == Port::kUnusedPort || Pin10::kPort_ == kPort_)
			&& (Pin11::kPort_ == Port::kUnusedPort || Pin11::kPort_ == kPort_)
			&& (Pin12::kPort_ == Port::kUnusedPort || Pin12::kPort_ == kPort_)
			&& (Pin13::kPort_ == Port::kUnusedPort || Pin13::kPort_ == kPort_)
			&& (Pin14::kPort_ == Port::kUnusedPort || Pin14::kPort_ == kPort_)
			&& (Pin15::kPort_ == Port::kUnusedPort || Pin15::kPort_ == kPort_)
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

		RCC->AHB2ENR |= (1 << (uint32_t(kPort_) + RCC_AHB2ENR_GPIOAEN_Pos));
		// Base address of the peripheral registers
		volatile GPIO_TypeDef &port = Io();
		port.MODER = kMODER_;
		port.OTYPER = kOTYPER_;
		port.OSPEEDR = kOSPEEDR_;
		port.PUPDR = kPUPDR_;
		port.AFR[0] = kAFRL_;
		port.AFR[1] = kAFRH_;
	}
	//! Apply state of pin group merging with previous GPI contents
	constexpr static void Enable(void)
	{
		// Base address of the peripheral registers
		volatile GPIO_TypeDef& port = Io();
		port.MODER = (port.MODER & kMODER_Mask_) | kMODER_;
		port.OTYPER = (port.OTYPER & kOTYPER_Mask_) | kOTYPER_;
		port.OSPEEDR = (port.OSPEEDR & kOSPEEDR_Mask_) | kOSPEEDR_;
		port.PUPDR = (port.PUPDR & kPUPDR_Mask_) | kPUPDR_;
		port.AFR[0] = (port.AFR[0] & kAFRL_Mask_) | kAFRL_;
		port.AFR[1] = (port.AFR[1] & kAFRH_Mask_) | kAFRH_;
	}
	//! Sets the reset values (according to data-sheet)
	constexpr static void Disable(void)
	{
		// Base address of the peripheral registers
		volatile GPIO_TypeDef& port = Io();
		// Reset values according to data-sheet
		if (kPort_ = Port::PA)
			port.MODER = 0xABFFFFFF;
		else if (kPort_ = Port::PB)
			port.MODER = 0xFFFFFEBF;
		else
			port.MODER = 0xFFFFFFFF;
		port.OTYPER = 0;
		if (kPort_ = Port::PA)
			port.OSPEEDR = 0x0C000000;
		else
			port.OSPEEDR = 0;
		if (kPort_ = Port::PA)
			port.PUPDR = 0x64000000;
		else if (kPort_ = Port::PB)
			port.PUPDR = 0x00000100;
		else
			port.PUPDR = 0;
		port.AFR[0] = 0;
		port.AFR[1] = 0;
		RCC->AHB2ENR &= ~(1 << (uint32_t(kPort_) + RCC_AHB2ENR_GPIOAEN_Pos));
	}
};

/// Keeps a copy of the current GPIO state and restores on scope exit
/*!
This class is useful to save current state of the GPIO registers and restore them 
on exit. This is useful when one wants to perform simple changes on the GPIO 
configuration for a short period and later restore to the previous state.

Note that this affects all bits of the port.
*/
template<const Gpio::Port kPort>
class SaveGpio
{
public:
	static constexpr Gpio::Port kPort_ = kPort;
	static constexpr uint32_t kPortBase_ = (GPIOA_BASE + uint32_t(kPort_) * 0x400);

	/// Access to the hardware IO data structure
	constexpr static volatile GPIO_TypeDef& Io() { return *(volatile GPIO_TypeDef*)kPortBase_; }

	/// Keeps a copy of the current GPIO state and restores on scope exit
	SaveGpio()
	{
		// Base address of the peripheral registers
		volatile GPIO_TypeDef& port = Io();
		// Make a copy of the hardware registers
		moder_ = port.MODER;
		otyper_ = port.OTYPER;
		ospeedr_ = port.OSPEEDR;
		pupdr_ = port.PUPDR;
		afrl_ = port.AFR[0];
		afrh_ = port.AFR[1];
	}
	~SaveGpio()
	{
		// Base address of the peripheral registers
		volatile GPIO_TypeDef& port = Io();
		// Restores all hardware registers
		port.MODER = moder_;
		port.OTYPER = otyper_;
		port.OSPEEDR = ospeedr_;
		port.PUPDR = pupdr_;
		port.AFR[0] = afrl_;
		port.AFR[1] = afrh_;
	}

protected:
	/// Copy of the ODR hardware register
	uint32_t moder_;
	uint32_t otyper_;
	uint32_t ospeedr_;
	uint32_t pupdr_;
	uint32_t afrl_;
	uint32_t afrh_;;
};


}	// namespace Gpio
}	// namespace Bmt
