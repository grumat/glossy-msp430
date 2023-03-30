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
	, const GpioPortId kPort					///< The GPIO port
	, const uint8_t kPin						///< The pin of the port
	, const Mode kMode = Mode::kInput			///< Mode to configure the port
	, const Speed kSpeed = Speed::kInput		///< Speed for the pin
	, const PuPd kPuPd = PuPd::kFloating		///< Additional pin configuration
	, const Level kInitialLevel = Level::kLow	///< Initial pin level
	, typename Map = AfNoRemap					///< Pin remapping feature (pinremap.h)
>
class Implementation_
{
public:
	/// Constant storing the GPIO port number
	static constexpr GpioPortId kPort_ = kPort;
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
		: uint32_t(kInitialLevel) << kPin;
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
		Set(uint32_t(kInitialLevel) != 0);
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
**	together into a GpioPortTemplate<> data type, which is able to setup the
**	entire GPIO port in a couple of CPU instructions.
**
**	Example:
**		// Sets a data-type to drive an SPI1 CLK output
**		typedef GpioTemplate<GpioPortId::PA, 5, kOutput50MHz, Mode::kPushPull, Level::kHigh> MY_SPI_CLK;
**		// Sets a data-type to inactivate the pin defined before
**		typedef InputPullUpPin<GpioPortId::PA, 5> MY_INACTIVE_SPI_CLK;
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
	const GpioPortId kPort						///< The GPIO port
	, const uint8_t kPin						///< The pin of the port
	, const Mode kMode = Mode::kInput			///< Mode to configure the port
	, const Speed kSpeed = Speed::kInput		///< Speed for the pin
	, const PuPd kPuPd = PuPd::kFloating		///< Additional pin configuration
	, const Level kInitialLevel = Level::kLow	///< Initial pin level (applies to output pin)
	, typename Map = AfNoRemap				///< Pin remapping feature (pinremap.h)
>
class AnyPin : public Private::Implementation_ <
	Private::Impl::kNormal
	, kPort
	, kPin
	, kMode
	, kSpeed
	, kPuPd
	, kInitialLevel
	, Map
>
{};


//! Template for input pins
template<
	const GpioPortId kPort						///< The GPIO port
	, const uint8_t kPin						///< The pin of the port
	, const PuPd kPuPd = PuPd::kFloating		///< Additional pin configuration
	, typename Map = AfNoRemap					///< Pin remapping feature (pinremap.h)
>
class AnyInputPin : public Private::Implementation_<
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


//! Template for input pins
template<
	const GpioPortId kPort						///< The GPIO port
	, const uint8_t kPin						///< The pin of the port
	, typename Map = AfNoRemap					///< Pin remapping feature (pinremap.h)
>
class AnyAnalogPin : public Private::Implementation_<
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


//! Template for output pins
template<
	const GpioPortId kPort						///< The GPIO port
	, const uint8_t kPin						///< The pin of the port
	, const Mode kMode = Mode::kOutput			///< Mode to configure the port
	, const Speed kSpeed = Speed::kFast			///< Speed for the pin
	, const Level kInitialLevel = Level::kLow	///< Initial pin level
	, const PuPd kPuPd = PuPd::kFloating		///< Additional pin configuration
	, typename Map = AfNoRemap					///< Pin remapping feature (pinremap.h)
>
class AnyOutputPin : public Private::Implementation_<
	Private::Impl::kNormal
	, kPort
	, kPin
	, kMode
	, kSpeed
	, kPuPd
	, kInitialLevel
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
	const GpioPortId kPort						///< The GPIO port
	, const uint8_t kPin						///< The pin of the port
	, typename Map								///< Pin remapping feature (pinremap.h)
	, const Mode kMode = Mode::kAlternate		///< Mode to configure the port
	, const PuPd kPuPd = PuPd::kFloating		///< Additional pin configuration
	, const Speed kSpeed = Speed::kFast			///< Speed for the pin
	, const Level kInitialLevel = Level::kLow	///< Initial pin level (applies to output pin)
>
class AnyAlternateOutPin : public Private::Implementation_<
	Private::Impl::kNormal
	, kPort
	, kPin
	, kMode
	, kSpeed
	, kPuPd
	, kInitialLevel
	, Map
>
{
private:
	constexpr void Validate_()
	{
		static_assert(kMode >= Mode::kAlternate, "template requires an alternate output pin configuration");
	}
};


/// A template class representing an unused pin
template<
	const uint8_t kPin						///< the pin number
	, const PuPd kPuPd = PuPd::kPullDown	///< Always configured as input; PD/PU selectable
>
class PinUnused : public Private::Implementation_
	<
	Private::Impl::kUnused
	, GpioPortId::kUnusedPort
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
class PinUnchanged : public Private::Implementation_
	<
	Private::Impl::kUnchanged
	, GpioPortId::kUnusedPort
	, kPin
	>
{
};


/// A template class to configure a port pin as a floating input pin
template <
	const GpioPortId kPort		///< The GPIO port
	, const uint8_t kPin		///< The GPIO pin number
>
class FloatingPin : public AnyInputPin<kPort, kPin>
{
};


/// A template class to configure a port pin as digital input having a pull-up
template <
	const GpioPortId kPort
	, const uint8_t kPin
>
class InputPullUpPin : public AnyInputPin<kPort, kPin, PuPd::kPullUp>
{
};


/// A template class to configure a port pin as digital input having a pull-down
template <
	const GpioPortId kPort
	, const uint8_t kPin
>
class InputPullDownPin : public AnyInputPin<kPort, kPin, PuPd::kPullDown>
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
typedef GpioTemplate<GpioPortId::PA, 0, Speed::kOutput2MHzkOutput2MHz, Mode::kPushPull, Level::kHigh> GREEN_LED;
/// Initial configuration for PORTA
typedef GpioPortTemplate <GpioPortId::PA
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
	/// The GPIO port peripheral
	static constexpr GpioPortId kPort_ = kPort;
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
			(Pin0::kPort_ == GpioPortId::kUnusedPort || Pin0::kPort_ == kPort_)
			&& (Pin1::kPort_ == GpioPortId::kUnusedPort || Pin1::kPort_ == kPort_)
			&& (Pin2::kPort_ == GpioPortId::kUnusedPort || Pin2::kPort_ == kPort_)
			&& (Pin3::kPort_ == GpioPortId::kUnusedPort || Pin3::kPort_ == kPort_)
			&& (Pin4::kPort_ == GpioPortId::kUnusedPort || Pin4::kPort_ == kPort_)
			&& (Pin5::kPort_ == GpioPortId::kUnusedPort || Pin5::kPort_ == kPort_)
			&& (Pin6::kPort_ == GpioPortId::kUnusedPort || Pin6::kPort_ == kPort_)
			&& (Pin7::kPort_ == GpioPortId::kUnusedPort || Pin7::kPort_ == kPort_)
			&& (Pin8::kPort_ == GpioPortId::kUnusedPort || Pin8::kPort_ == kPort_)
			&& (Pin9::kPort_ == GpioPortId::kUnusedPort || Pin9::kPort_ == kPort_)
			&& (Pin10::kPort_ == GpioPortId::kUnusedPort || Pin10::kPort_ == kPort_)
			&& (Pin11::kPort_ == GpioPortId::kUnusedPort || Pin11::kPort_ == kPort_)
			&& (Pin12::kPort_ == GpioPortId::kUnusedPort || Pin12::kPort_ == kPort_)
			&& (Pin13::kPort_ == GpioPortId::kUnusedPort || Pin13::kPort_ == kPort_)
			&& (Pin14::kPort_ == GpioPortId::kUnusedPort || Pin14::kPort_ == kPort_)
			&& (Pin15::kPort_ == GpioPortId::kUnusedPort || Pin15::kPort_ == kPort_)
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
		if (kPort_ = GpioPortId::PA)
			port.MODER = 0xABFFFFFF;
		else if (kPort_ = GpioPortId::PB)
			port.MODER = 0xFFFFFEBF;
		else
			port.MODER = 0xFFFFFFFF;
		port.OTYPER = 0;
		if (kPort_ = GpioPortId::PA)
			port.OSPEEDR = 0x0C000000;
		else
			port.OSPEEDR = 0;
		if (kPort_ = GpioPortId::PA)
			port.PUPDR = 0x64000000;
		else if (kPort_ = GpioPortId::PB)
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
template<const GpioPortId kPort>
class SaveGpio
{
public:
	static constexpr GpioPortId kPort_ = kPort;
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


//////////////////////////////////////////////////////////////////////
// ADC12
//////////////////////////////////////////////////////////////////////
/// A default configuration for ADC12 IN5 on PA0
typedef AnyAnalogPin<GpioPortId::PA, 0> ADC12_IN5_PA0;
/// A default configuration for ADC12 IN6 on PA1
typedef AnyAnalogPin<GpioPortId::PA, 1> ADC12_IN6_PA1;
/// A default configuration for ADC12 IN7 on PA2
typedef AnyAnalogPin<GpioPortId::PA, 2> ADC12_IN7_PA2;
/// A default configuration for ADC12 IN8 on PA3
typedef AnyAnalogPin<GpioPortId::PA, 3> ADC12_IN8_PA3;
/// A default configuration for ADC12 IN9 on PA4
typedef AnyAnalogPin<GpioPortId::PA, 4> ADC12_IN9_PA4;
/// A default configuration for ADC12 IN10 on PA5
typedef AnyAnalogPin<GpioPortId::PA, 5> ADC12_IN10_PA5;
/// A default configuration for ADC12 IN11 on PA6
typedef AnyAnalogPin<GpioPortId::PA, 6> ADC12_IN11_PA6;
/// A default configuration for ADC12 IN12 on PA7
typedef AnyAnalogPin<GpioPortId::PA, 7> ADC12_IN12_PA7;
/// A default configuration for ADC12 IN15 on PB0
typedef AnyAnalogPin<GpioPortId::PB, 0> ADC12_IN15_PB0;
/// A default configuration for ADC12 IN16 on PB1
typedef AnyAnalogPin<GpioPortId::PB, 1> ADC12_IN16_PB1;

//////////////////////////////////////////////////////////////////////
// SYS
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map MCO on PA8 pin
typedef AnyAlternateOutPin<GpioPortId::PA, 8, AfSYS_MCO_PA8>		SYS_MCO;

//////////////////////////////////////////////////////////////////////
// JTAG / SWD
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map JTMS on PA13 pin
typedef AnyAlternateOutPin<GpioPortId::PA, 13, AfSYS_JTMS_PA13>		SYS_JTMS;
/// A generic configuration to map JTCK on PA14 pin
typedef AnyAlternateOutPin<GpioPortId::PA, 14, AfSYS_JTCK_PA14>		SYS_JTCK;
/// A generic configuration to map JTDI on PA15 pin
typedef AnyAlternateOutPin<GpioPortId::PA, 15, AfSYS_JTDI_PA15>		SYS_JTDI;
/// A generic configuration to map JTDO on PB3 pin
typedef AnyAlternateOutPin<GpioPortId::PB, 3, AfSYS_JTDO_PB3>		SYS_JTDO;
/// A generic configuration to map NJTRST on PB4 pin
typedef AnyAlternateOutPin<GpioPortId::PB, 4, AfSYS_NJTRST_PB4>		SYS_NJTRST;
/// A generic configuration to map SWDIO on PA13 pin
typedef AnyAlternateOutPin<GpioPortId::PA, 13, AfSYS_SWDIO_PA13>	SYS_SWDIO;
/// A generic configuration to map SWCLK on PA14 pin
typedef AnyAlternateOutPin<GpioPortId::PA, 14, AfSYS_SWCLK_PA14>	SYS_SWCLK;
/// A generic configuration to map TRACESWO on PB3 pin
typedef AnyAlternateOutPin<GpioPortId::PB, 3, AfSYS_TRACESWO_PB3>	SYS_TRACESWO;

//////////////////////////////////////////////////////////////////////
// CAN - Configuration 1 (cannot mix pins between configuration)
//////////////////////////////////////////////////////////////////////
/// A default configuration for CAN/RX on PA11 pin
typedef AnyInputPin<GpioPortId::PA, 11, PuPd::kFloating, AfCAN1_RX_PA11>	CAN_RX_PA11;
/// A default configuration for CAN/TX on PA12 pin
typedef AnyAlternateOutPin<GpioPortId::PA, 12, AfCAN1_TX_PA12>	CAN_TX_PA12;

//////////////////////////////////////////////////////////////////////
// I2C1
//////////////////////////////////////////////////////////////////////
/// A default configuration to map I2C1 SCL on PA9 pin
typedef AnyAlternateOutPin<GpioPortId::PA, 9, AfI2C1_SCL_PA9>	I2C1_SCL_PA9;
/// A default configuration to map I2C1 SCL on PA9 pin
typedef AnyAlternateOutPin<GpioPortId::PB, 6, AfI2C1_SCL_PB6>	I2C1_SCL_PB6;
/// A default configuration to map I2C1 SDA on PA10 pin
typedef AnyAlternateOutPin<GpioPortId::PA, 10, AfI2C1_SDA_PA10>	I2C1_SDA_PA10;
/// A default configuration to map I2C1 SDA on PB7 pin
typedef AnyAlternateOutPin<GpioPortId::PB, 7, AfI2C1_SDA_PB7>	I2C1_SDA_PB7;
/// A default configuration to map I2C1 SMBA on PA1 pin
typedef AnyInputPin<GpioPortId::PA, 1, PuPd::kFloating, AfI2C1_SMBA_PA1>	I2C1_SMBA_PA1;
/// A default configuration to map I2C1 SMBA on PA14 pin
typedef AnyInputPin<GpioPortId::PA, 14, PuPd::kFloating, AfI2C1_SMBA_PA14>	I2C1_SMBA_PA14;
/// A default configuration to map I2C1 SMBA on PA14 pin
typedef AnyInputPin<GpioPortId::PB, 5, PuPd::kFloating, AfI2C1_SMBA_PB5>	I2C1_SMBA_PB5;

//////////////////////////////////////////////////////////////////////
// I2C3
//////////////////////////////////////////////////////////////////////
/// A default configuration to map I2C3 SCL on PA7 pin
typedef AnyAlternateOutPin<GpioPortId::PA, 7, AfI2C3_SCL_PA7>	I2C3_SCL_PA7;
/// A default configuration to map I2C3 SDA on PB4 pin
typedef AnyAlternateOutPin<GpioPortId::PB, 4, AfI2C3_SDA_PB4>	I2C2_SDA_PB4;

//////////////////////////////////////////////////////////////////////
// SPI1
//////////////////////////////////////////////////////////////////////
/// A default configuration to map SPI1 NSS on PA4 pin (master)
typedef AnyAlternateOutPin<GpioPortId::PA, 4, AfSPI1NSS_PA4>			SPI1_NSS_PA4;
/// A default configuration to map SPI1 NSS on PA4 pin (slave)
typedef AnyInputPin<GpioPortId::PA, 4, PuPd::kPullUp, AfSPI1NSS_PA4>	SPI1_NSS_PA4_SLAVE;
/// A default configuration to map SPI1 NSS on PA15 pin (master)
typedef AnyAlternateOutPin<GpioPortId::PA, 15, AfSPI1NSS_PA15>			SPI1_NSS_PA15;
/// A default configuration to map SPI1 NSS on PA15 pin (slave)
typedef AnyInputPin<GpioPortId::PA, 15, PuPd::kPullUp, AfSPI1NSS_PA15>	SPI1_NSS_PA15_SLAVE;
/// A default configuration to map SPI1 NSS on PB0 pin (master)
typedef AnyAlternateOutPin<GpioPortId::PB, 0, AfSPI1NSS_PB0>			SPI1_NSS_PB0;
/// A default configuration to map SPI1 NSS on PB0 pin (slave)
typedef AnyInputPin<GpioPortId::PB, 0, PuPd::kPullUp, AfSPI1NSS_PB0>	SPI1_NSS_PB0_SLAVE;
/// A default configuration to map SPI1 SCK on PA1 pin (master)
typedef AnyAlternateOutPin<GpioPortId::PA, 1, AfSPI1SCK_PA1>			SPI1_SCK_PA1;
/// A default configuration to map SPI1 SCK on PA1 pin (slave)
typedef AnyInputPin<GpioPortId::PA, 1, PuPd::kFloating, AfSPI1SCK_PA1>	SPI1_SCK_PA1_SLAVE;
/// A default configuration to map SPI1 SCK on PA5 pin (master)
typedef AnyAlternateOutPin<GpioPortId::PA, 5, AfSPI1SCK_PA5>			SPI1_SCK_PA5;
/// A default configuration to map SPI1 SCK on PA5 pin (slave)
typedef AnyInputPin<GpioPortId::PA, 5, PuPd::kFloating, AfSPI1SCK_PA5>	SPI1_SCK_PA5_SLAVE;
/// A default configuration to map SPI1 SCK on PB3 pin (master)
typedef AnyAlternateOutPin<GpioPortId::PB, 3, AfSPI1SCK_PB3>			SPI1_SCK_PB3;
/// A default configuration to map SPI1 SCK on PB3 pin (slave)
typedef AnyInputPin<GpioPortId::PB, 3, PuPd::kFloating, AfSPI1SCK_PB3>	SPI1_SCK_PB3_SLAVE;
/// A default configuration to map SPI1 MISO on PA6 pin (master)
typedef AnyInputPin<GpioPortId::PA, 6, PuPd::kPullUp, AfSPI1MISO_PA6>	SPI1_MISO_PA6;
/// A default configuration to map SPI1 MISO on PA6 pin (slave)
typedef AnyAlternateOutPin<GpioPortId::PA, 6, AfSPI1MISO_PA6>			SPI1_MISO_PA6_SLAVE;
/// A default configuration to map SPI1 MISO on PA11 pin (master)
typedef AnyInputPin<GpioPortId::PA, 11, PuPd::kPullUp, AfSPI1MISO_PA11>	SPI1_MISO_PA11;
/// A default configuration to map SPI1 MISO on PA11 pin (slave)
typedef AnyAlternateOutPin<GpioPortId::PA, 11, AfSPI1MISO_PA11>			SPI1_MISO_PA11_SLAVE;
/// A default configuration to map SPI1 MISO on PB4 pin (master)
typedef AnyInputPin<GpioPortId::PB, 4, PuPd::kPullUp, AfSPI1MISO_PB4>	SPI1_MISO_PB4;
/// A default configuration to map SPI1 MISO on PB4 pin (slave)
typedef AnyAlternateOutPin<GpioPortId::PB, 4, AfSPI1MISO_PB4>			SPI1_MISO_PB4_SLAVE;
/// A default configuration to map SPI1 MOSI on PA7 pin (master)
typedef AnyAlternateOutPin<GpioPortId::PA, 7, AfSPI1MOSI_PA7>			SPI1_MOSI_PA7;
/// A default configuration to map SPI1 MOSI on PA7 pin (slave)
typedef AnyInputPin<GpioPortId::PA, 7, PuPd::kPullUp, AfSPI1MOSI_PA7>	SPI1_MOSI_PA7_SLAVE;
/// A default configuration to map SPI1 MOSI on PA12 pin (master)
typedef AnyAlternateOutPin<GpioPortId::PA, 12, AfSPI1MOSI_PA12>			SPI1_MOSI_PA12;
/// A default configuration to map SPI1 MOSI on PA12 pin (slave)
typedef AnyInputPin<GpioPortId::PA, 12, PuPd::kPullUp, AfSPI1MOSI_PA12>	SPI1_MOSI_PA12_SLAVE;
/// A default configuration to map SPI1 MOSI on PB5 pin (master)
typedef AnyAlternateOutPin<GpioPortId::PB, 5, AfSPI1MOSI_PB5>			SPI1_MOSI_PB5;
/// A default configuration to map SPI1 MOSI on PB5 pin (slave)
typedef AnyInputPin<GpioPortId::PB, 5, PuPd::kPullUp, AfSPI1MOSI_PB5>	SPI1_MOSI_PB5_SLAVE;

//////////////////////////////////////////////////////////////////////
// SPI3
//////////////////////////////////////////////////////////////////////
/// A default configuration to map SPI3 NSS on PA4 pin (master)
typedef AnyAlternateOutPin<GpioPortId::PA, 4, AfSPI3NSS_PA4>			SPI3_NSS_PA4;
/// A default configuration to map SPI3 NSS on PA4 pin (slave)
typedef AnyInputPin<GpioPortId::PA, 4, PuPd::kPullUp, AfSPI3NSS_PA4>	SPI3_NSS_PA4_SLAVE;
/// A default configuration to map SPI3 NSS on PA15 pin (master)
typedef AnyAlternateOutPin<GpioPortId::PA, 15, AfSPI3NSS_PA15>			SPI3_NSS_PA15;
/// A default configuration to map SPI3 NSS on PA15 pin (slave)
typedef AnyInputPin<GpioPortId::PA, 15, PuPd::kPullUp, AfSPI3NSS_PA15>	SPI3_NSS_PA15_SLAVE;
/// A default configuration to map SPI3 SCK on PB3 pin (master)
typedef AnyAlternateOutPin<GpioPortId::PB, 3, AfSPI3SCK_PB3>			SPI3_SCK_PB3;
/// A default configuration to map SPI3 SCK on PB3 pin (slave)
typedef AnyInputPin<GpioPortId::PB, 3, PuPd::kFloating, AfSPI3SCK_PB3>	SPI3_SCK_PB3_SLAVE;
/// A default configuration to map SPI3 MISO on PB4 pin (master)
typedef AnyInputPin<GpioPortId::PB, 4, PuPd::kPullUp, AfSPI3MISO_PB4>	SPI3_MISO_PB4;
/// A default configuration to map SPI3 MISO on PB4 pin (slave)
typedef AnyAlternateOutPin<GpioPortId::PB, 4, AfSPI3MISO_PB4>			SPI3_MISO_PB4_SLAVE;
/// A default configuration to map SPI3 MOSI on PB5 pin (master)
typedef AnyAlternateOutPin<GpioPortId::PB, 5, AfSPI3MOSI_PB5>			SPI3_MOSI_PB5;
/// A default configuration to map SPI3 MOSI on PB5 pin (slave)
typedef AnyInputPin<GpioPortId::PB, 5, PuPd::kPullUp, AfSPI3MOSI_PB5>	SPI3_MOSI_PB5_SLAVE;


//////////////////////////////////////////////////////////////////////
// TIM1
//////////////////////////////////////////////////////////////////////
/// A default configuration to map TIM1 ETR on PA12 pin (input)
typedef AnyInputPin<GpioPortId::PA, 12, PuPd::kFloating, AfTIM1_ETR_PA12>	TIM1_ETR_PA12_IN;
/// A default configuration to map TIM1 BKIN on PA6 pin (input)
typedef AnyInputPin<GpioPortId::PA, 6, PuPd::kFloating, AfTIM1_BKIN_PA6>	TIM1_BKIN_PA6_IN;
/// A default configuration to map TIM1 BKIN2 on PA11 pin (input)
typedef AnyInputPin<GpioPortId::PA, 11, PuPd::kFloating, AfTIM1_BKIN2_PA11>	TIM1_BKIN2_PA11_IN;

/// A default configuration to map TIM1 CH1 on PA8 pin (output)
typedef AnyAlternateOutPin<GpioPortId::PA, 8, AfTIM1_CH1_PA8>				TIM1_CH1_PA8_OUT;
/// A default configuration to map TIM1 CH1 on PA8 pin (input)
typedef AnyInputPin<GpioPortId::PA, 8, PuPd::kFloating, AfTIM1_CH1_PA8>		TIM1_CH1_PA8_IN;
/// A default configuration to map TIM1 CH1N on PA7 pin (output)
typedef AnyAlternateOutPin<GpioPortId::PA, 7, AfTIM1_CH1N_PA7>				TIM1_CH1N_PA7_OUT;

/// A default configuration to map TIM1 CH2 on PA9 pin (output)
typedef AnyAlternateOutPin<GpioPortId::PA, 9, AfTIM1_CH2_PA9>				TIM1_CH2_PA9_OUT;
/// A default configuration to map TIM1 CH2 on PA9 pin (input)
typedef AnyInputPin<GpioPortId::PA, 9, PuPd::kFloating, AfTIM1_CH2_PA9>		TIM1_CH2_PA9_IN;
/// A default configuration to map TIM1 CH2N on PB0 pin (output)
typedef AnyAlternateOutPin<GpioPortId::PB, 0, AfTIM1_CH2N_PB0>				TIM1_CH2N_PB0_OUT;

/// A default configuration to map TIM1 CH3 on PA10 pin (output)
typedef AnyAlternateOutPin<GpioPortId::PA, 10, AfTIM1_CH3_PA10>				TIM1_CH3_PA10_OUT;
/// A default configuration to map TIM1 CH3 on PA10 pin (input)
typedef AnyInputPin<GpioPortId::PA, 10, PuPd::kFloating, AfTIM1_CH3_PA10>	TIM1_CH3_PA10_IN;
/// A default configuration to map TIM1 CH3N on PB1 pin (output)
typedef AnyAlternateOutPin<GpioPortId::PB, 1, AfTIM1_CH3N_PB1>				TIM1_CH3N_PB1_OUT;

/// A default configuration to map TIM1 CH4 on PA11 pin (output)
typedef AnyAlternateOutPin<GpioPortId::PA, 11, AfTIM1_CH4_PA11>				TIM1_CH4_PA11_OUT;
/// A default configuration to map TIM1 CH4 on PA11 pin (input)
typedef AnyInputPin<GpioPortId::PA, 11, PuPd::kFloating, AfTIM1_CH4_PA11>	TIM1_CH4_PA11_IN;

//////////////////////////////////////////////////////////////////////
// TIM2
//////////////////////////////////////////////////////////////////////
/// A default configuration to map TIM2 ETR on PA0 pin (input)
typedef AnyInputPin<GpioPortId::PA, 0, PuPd::kFloating, AfTIM2_ETR_PA0>		TIM2_ETR_PA0_IN;
/// A default configuration to map TIM2 ETR on PA5 pin (input)
typedef AnyInputPin<GpioPortId::PA, 5, PuPd::kFloating, AfTIM2_ETR_PA5>		TIM2_ETR_PA5_IN;
/// A default configuration to map TIM2 ETR on PA15 pin (input)
typedef AnyInputPin<GpioPortId::PA, 15, PuPd::kFloating, AfTIM2_ETR_PA15>	TIM2_ETR_PA15_IN;

/// A default configuration to map TIM2 CH1 on PA0 pin (output)
typedef AnyAlternateOutPin<GpioPortId::PA, 0, AfTIM2_CH1_PA0>				TIM2_CH1_PA0_OUT;
/// A default configuration to map TIM2 CH1 on PA0 pin (input)
typedef AnyInputPin<GpioPortId::PA, 0, PuPd::kFloating, AfTIM2_CH1_PA0>		TIM2_CH1_PA0_IN;
/// A default configuration to map TIM2 CH1 on PA5 pin (output)
typedef AnyAlternateOutPin<GpioPortId::PA, 5, AfTIM2_CH1_PA5>				TIM2_CH1_PA5_OUT;
/// A default configuration to map TIM2 CH1 on PA5 pin (input)
typedef AnyInputPin<GpioPortId::PA, 5, PuPd::kFloating, AfTIM2_CH1_PA5>		TIM2_CH1_PA5_IN;
/// A default configuration to map TIM2 CH1 on PA15 pin (output)
typedef AnyAlternateOutPin<GpioPortId::PA, 15, AfTIM2_CH1_PA15>				TIM2_CH1_PA15_OUT;
/// A default configuration to map TIM2 CH1 on PA15 pin (input)
typedef AnyInputPin<GpioPortId::PA, 15, PuPd::kFloating, AfTIM2_CH1_PA15>	TIM2_CH1_PA15_IN;

/// A default configuration to map TIM2 CH2 on PA1 pin (output)
typedef AnyAlternateOutPin<GpioPortId::PA, 1, AfTIM2_CH2_PA1>				TIM2_CH2_PA1_OUT;
/// A default configuration to map TIM2 CH2 on PA1 pin (input)
typedef AnyInputPin<GpioPortId::PA, 1, PuPd::kFloating, AfTIM2_CH2_PA1>		TIM2_CH2_PA1_IN;
/// A default configuration to map TIM2 CH2 on PB3 pin (output)
typedef AnyAlternateOutPin<GpioPortId::PB, 3, AfTIM2_CH2_PB3>				TIM2_CH2_PB3_OUT;
/// A default configuration to map TIM2 CH2 on PB3 pin (input)
typedef AnyInputPin<GpioPortId::PB, 3, PuPd::kFloating, AfTIM2_CH2_PB3>		TIM2_CH2_PB3_IN;

/// A default configuration to map TIM2 CH3 on PA2 pin (output)
typedef AnyAlternateOutPin<GpioPortId::PA, 2, AfTIM2_CH3_PA2>				TIM2_CH3_PA2_OUT;
/// A default configuration to map TIM2 CH3 on PA2 pin (input)
typedef AnyInputPin<GpioPortId::PA, 2, PuPd::kFloating, AfTIM2_CH3_PA2>		TIM2_CH3_PA2_IN;

/// A default configuration to map TIM2 CH4 on PA3 pin (output)
typedef AnyAlternateOutPin<GpioPortId::PA, 3, AfTIM2_CH4_PA3>				TIM2_CH4_PA3_OUT;
/// A default configuration to map TIM2 CH4 on PA3 pin (input)
typedef AnyInputPin<GpioPortId::PA, 3, PuPd::kFloating, AfTIM2_CH4_PA3>		TIM2_CH4_PA3_IN;

//////////////////////////////////////////////////////////////////////
// TIM15
//////////////////////////////////////////////////////////////////////
/// A default configuration to map TIM15 BKIN on PA9 pin (input)
typedef AnyInputPin<GpioPortId::PA, 9, PuPd::kFloating, AfTIM15_BKIN_PA9>	TIM15_BKIN_PA9_IN;

/// A default configuration to map TIM15 CH1 on PA2 pin (output)
typedef AnyAlternateOutPin<GpioPortId::PA, 2, AfTIM15_CH1_PA2>				TIM15_CH1_PA2_OUT;
/// A default configuration to map TIM15 CH1 on PA2 pin (input)
typedef AnyInputPin<GpioPortId::PA, 2, PuPd::kFloating, AfTIM15_CH1_PA2>	TIM15_CH1_PA2_IN;
/// A default configuration to map TIM15 CH1N on PA7 pin (output)
typedef AnyAlternateOutPin<GpioPortId::PA, 7, AfTIM15_CH1N_PA1>				TIM15_CH1N_PA7_OUT;

/// A default configuration to map TIM15 CH2 on PA3 pin (output)
typedef AnyAlternateOutPin<GpioPortId::PA, 3, AfTIM15_CH2_PA3>				TIM15_CH2_PA3_OUT;
/// A default configuration to map TIM15 CH2 on PA3 pin (input)
typedef AnyInputPin<GpioPortId::PA, 3, PuPd::kFloating, AfTIM15_CH2_PA3>	TIM15_CH2_PA3_IN;

//////////////////////////////////////////////////////////////////////
// TIM16
//////////////////////////////////////////////////////////////////////
/// A default configuration to map TIM16 BKIN on PB5 pin (input)
typedef AnyInputPin<GpioPortId::PB, 5, PuPd::kFloating, AfTIM15_BKIN_PB5>	TIM16_BKIN_PB5_IN;

/// A default configuration to map TIM16 CH1 on PA6 pin (output)
typedef AnyAlternateOutPin<GpioPortId::PA, 6, AfTIM16_CH1_PA6>				TIM16_CH1_PA6_OUT;
/// A default configuration to map TIM16 CH1 on PA6 pin (input)
typedef AnyInputPin<GpioPortId::PA, 6, PuPd::kFloating, AfTIM16_CH1_PA6>	TIM16_CH1_PA6_IN;
/// A default configuration to map TIM16 CH1N on PB6 pin (output)
typedef AnyAlternateOutPin<GpioPortId::PB, 6, AfTIM16_CH1N_PB6>				TIM16_CH1N_PB6_OUT;

//////////////////////////////////////////////////////////////////////
// LPTIM1
//////////////////////////////////////////////////////////////////////
/// A default configuration to map LPTIM1 ETR on PB6 pin (input)
typedef AnyInputPin<GpioPortId::PB, 6, PuPd::kFloating, AfLPTIM1_ETR_PB6>	LPTIM1_ETR_PB6_IN;

/// A default configuration to map LPTIM1 OUT on PA14 pin (output)
typedef AnyAlternateOutPin<GpioPortId::PA, 16, AfLPTIM1_OUT_PA14>			LPTIM1_OUT_PA14;

/// A default configuration to map LPTIM1 IN1 on PB5 pin (input)
typedef AnyInputPin<GpioPortId::PB, 5, PuPd::kFloating, AfLPTIM1_IN1_PB5>	LPTIM1_IN1_PB6;
/// A default configuration to map LPTIM1 IN2 on PB7 pin (input)
typedef AnyInputPin<GpioPortId::PB, 7, PuPd::kFloating, AfLPTIM1_IN2_PB7>	LPTIM1_IN2_PB7;

//////////////////////////////////////////////////////////////////////
// LPTIM2
//////////////////////////////////////////////////////////////////////
/// A default configuration to map LPTIM2 ETR on PA5 pin (input)
typedef AnyInputPin<GpioPortId::PA, 5, PuPd::kFloating, AfLPTIM2_ETR_PA5>	LPTIM2_ETR_PA5_IN;

/// A default configuration to map LPTIM2 OUT on PA4 pin (output)
typedef AnyAlternateOutPin<GpioPortId::PA, 4, AfLPTIM2_OUT_PA4>				LPTIM2_OUT_PA4;
/// A default configuration to map LPTIM2 OUT on PA8 pin (output)
typedef AnyAlternateOutPin<GpioPortId::PA, 8, AfLPTIM2_OUT_PA8>				LPTIM2_OUT_PA8;

/// A default configuration to map LPTIM2 IN1 on PB1 pin (input)
typedef AnyInputPin<GpioPortId::PB, 1, PuPd::kFloating, AfLPTIM2_IN1_PB1>	LPTIM2_IN1_PB1;

//////////////////////////////////////////////////////////////////////
// USART1
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map USART1 TX on PA9 pin
typedef AnyAlternateOutPin<GpioPortId::PA, 9, AfUSART1_TX_PA9>				USART1_TX_PA9;
/// A generic configuration to map USART1 TX on PB6 pin
typedef AnyAlternateOutPin<GpioPortId::PB, 6, AfUSART1_TX_PB6>				USART1_TX_PB6;

/// A generic configuration to map USART1 RX on PA10 pin
typedef AnyInputPin<GpioPortId::PA, 10, PuPd::kPullUp, AfUSART1_RX_PA10>	USART1_RX_PA10;
/// A generic configuration to map USART1 RX on PB7 pin
typedef AnyInputPin<GpioPortId::PB, 7, PuPd::kPullUp, AfUSART1_RX_PB7>		USART1_RX_PB7;

/// A generic configuration to map USART1 CK on PA8 pin
typedef AnyAlternateOutPin<GpioPortId::PA, 8, AfUSART1_CK_PA8>				USART1_CK_PA8;
/// A generic configuration to map USART1 CK on PB5 pin
typedef AnyAlternateOutPin<GpioPortId::PB, 5, AfUSART1_CK_PB5>				USART1_CK_PB5;

/// A generic configuration to map USART1 CTS on PA11 pin
typedef AnyInputPin<GpioPortId::PA, 11, PuPd::kPullUp, AfUSART1_CTS_PA11>	USART1_CTS_PA11;
/// A generic configuration to map USART1 CTS on PB4 pin
typedef AnyInputPin<GpioPortId::PB, 4, PuPd::kPullUp, AfUSART1_CTS_PB4>		USART1_CTS_PB4;

/// A generic configuration to map USART1 RTS on PA12 pin
typedef AnyAlternateOutPin<GpioPortId::PA, 12, AfUSART1_RTS_PA12>			USART1_RTS_PA12;
/// A generic configuration to map USART1 RTS on PB3 pin
typedef AnyAlternateOutPin<GpioPortId::PB, 3, AfUSART1_RTS_PB3>				USART1_RTS_PB3;

//////////////////////////////////////////////////////////////////////
// USART2
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map USART2 TX on PA2 pin
typedef AnyAlternateOutPin<GpioPortId::PA, 2, AfUSART2_TX_PA2>				USART2_TX_PA2;
/// A generic configuration to map USART2 RX on PA3 pin
typedef AnyInputPin<GpioPortId::PA, 3, PuPd::kPullUp, AfUSART2_RX_PA3>		USART2_RX_PA3;
/// A generic configuration to map USART2 RX on PA15 pin
typedef AnyInputPin<GpioPortId::PA, 15, PuPd::kPullUp, AfUSART2_RX_PA15>	USART2_RX_PA15;
/// A generic configuration to map USART2 CK on PA4 pin
typedef AnyAlternateOutPin<GpioPortId::PA, 4, AfUSART2_CK_PA4>				USART2_CK_PA4;
/// A generic configuration to map USART2 CTS on PA0 pin
typedef AnyInputPin<GpioPortId::PA, 0, PuPd::kPullUp, AfUSART2_CTS_PA0>		USART2_CTS_PA0;
/// A generic configuration to map USART2 RTS on PA1 pin
typedef AnyAlternateOutPin<GpioPortId::PA, 1, AfUSART2_RTS_PA1>				USART2_RTS_PA1;

//////////////////////////////////////////////////////////////////////
// USART3
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map USART3 CK on PB12 pin
typedef AnyAlternateOutPin<GpioPortId::PB, 0, AfUSART3_CK_PB0>				USART3_CK_PB0;
/// A generic configuration to map USART3 CTS on PA6 pin
typedef AnyInputPin<GpioPortId::PA, 6, PuPd::kPullUp, AfUSART3_CTS_PA6>		USART3_CTS_PA6;
/// A generic configuration to map USART3 RTS on PA15 pin
typedef AnyAlternateOutPin<GpioPortId::PA, 15, AfUSART3_RTS_PA15>			USART3_RTS_PA15;
/// A generic configuration to map USART3 RTS on PB1 pin
typedef AnyAlternateOutPin<GpioPortId::PB, 1, AfUSART3_RTS_PB1>				USART3_RTS_PB1;

//////////////////////////////////////////////////////////////////////
// LPUART1
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map LPUART1 TX on PA2 pin
typedef AnyAlternateOutPin<GpioPortId::PA, 2, AfLPUART1_TX_PA2>				LPUART1_TX_PA2;
/// A generic configuration to map LPUART1 RX on PA3 pin
typedef AnyInputPin<GpioPortId::PA, 3, PuPd::kPullUp, AfLPUART1_RX_PA3>		LPUART1_RX_PA3;
/// A generic configuration to map LPUART1 CTS on PA6 pin
typedef AnyInputPin<GpioPortId::PA, 6, PuPd::kPullUp, AfLPUART1_CTS_PA6>	LPUART1_CTS_PA6;
/// A generic configuration to map LPUART1 RTS on PB1 pin
typedef AnyAlternateOutPin<GpioPortId::PB, 1, AfLPUART1_RTS_PB1>			LPUART1_RTS_PB1;

//////////////////////////////////////////////////////////////////////
// USB
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map USB_CRS_SYNC on PA10 pin
typedef AnyInputPin<GpioPortId::PA, 10, PuPd::kFloating, AfUSB_CRS_SYNC_PA10>	USB_CRS_SYNC;
/// A generic configuration to map USB DM on PA11 pin
typedef AnyAnalogPin<GpioPortId::PA, 11, AfUSB_DM_PA11>							USB_DM_PA11;
/// A generic configuration to map USB DP on PA12 pin
typedef AnyAnalogPin<GpioPortId::PA, 12, AfUSB_DP_PA12>							USB_DP_PA12;
/// A generic configuration to map USB NOE on PA13 pin
typedef AnyAlternateOutPin<GpioPortId::PA, 13, AfUSB_NOE_PA13>					USB_NOE_PA13;





// IR
typedef AnyAFR<GpioPortId::PA, 13, AF::k1>	AfIR_OUT_PA13;

// TSC
typedef AnyAFR<GpioPortId::PA, 15, AF::k9>	AfTSC_G3_IO1_PA15;
typedef AnyAFR<GpioPortId::PB, 4, AF::k9>	AfTSC_G2_IO1_PB4;
typedef AnyAFR<GpioPortId::PB, 5, AF::k9>	AfTSC_G2_IO2_PB5;
typedef AnyAFR<GpioPortId::PB, 6, AF::k9>	AfTSC_G2_IO3_PB6;
typedef AnyAFR<GpioPortId::PB, 7, AF::k9>	AfTSC_G2_IO4_PB7;

// QUADSPI
typedef AnyAFR<GpioPortId::PA, 2, AF::k10>	AfQUADSPI_BK1_NCS_PA2;
typedef AnyAFR<GpioPortId::PA, 3, AF::k10>	AfQUADSPI_CLK_PA3;
typedef AnyAFR<GpioPortId::PA, 6, AF::k10>	AfQUADSPI_IO3_PA6;
typedef AnyAFR<GpioPortId::PA, 7, AF::k10>	AfQUADSPI_IO2_PA7;
typedef AnyAFR<GpioPortId::PB, 0, AF::k10>	AfQUADSPI_IO1_PB0;
typedef AnyAFR<GpioPortId::PB, 1, AF::k10>	AfQUADSPI_IO0_PB1;

// COMP1
typedef AnyAFR<GpioPortId::PA, 0, AF::k12>	AfCOMP1_OUT_PA0;
typedef AnyAFR<GpioPortId::PA, 6, AF::k6>	AfCOMP1_OUT_PA6;
typedef AnyAFR<GpioPortId::PA, 11, AF::k6>	AfCOMP1_OUT_PA11;
typedef AnyAFR<GpioPortId::PA, 11, AF::k12>	AfCOMP1_TIM1_BKIN2_PA6;
typedef AnyAFR<GpioPortId::PB, 0, AF::k12>	AfCOMP1_OUT_PB0;

// COMP2
typedef AnyAFR<GpioPortId::PA, 2, AF::k12>	AfCOMP2_OUT_PA2;
typedef AnyAFR<GpioPortId::PA, 6, AF::k12>	AfCOMP2_TIM1_BKIN_PA6;
typedef AnyAFR<GpioPortId::PA, 7, AF::k12>	AfCOMP2_OUT_PA7;
typedef AnyAFR<GpioPortId::PB, 5, AF::k12>	AfCOMP2_OUT_PB5;

// SWPMI1
typedef AnyAFR<GpioPortId::PA, 8, AF::k12>	AfSWPMI1_IO_PA8;
typedef AnyAFR<GpioPortId::PA, 13, AF::k12>	AfSWPMI1_TX_PA13;
typedef AnyAFR<GpioPortId::PA, 14, AF::k12>	AfSWPMI1_RX_PA14;
typedef AnyAFR<GpioPortId::PA, 15, AF::k12>	AfSWPMI1_SUSPEND_PA15;

// EVENTOUT
typedef AnyAFR<GpioPortId::PA, 0, AF::k15>	AfEVENTOUT_PA0;
typedef AnyAFR<GpioPortId::PA, 1, AF::k15>	AfEVENTOUT_PA1;
typedef AnyAFR<GpioPortId::PA, 2, AF::k15>	AfEVENTOUT_PA2;
typedef AnyAFR<GpioPortId::PA, 3, AF::k15>	AfEVENTOUT_PA3;
typedef AnyAFR<GpioPortId::PA, 4, AF::k15>	AfEVENTOUT_PA4;
typedef AnyAFR<GpioPortId::PA, 5, AF::k15>	AfEVENTOUT_PA5;
typedef AnyAFR<GpioPortId::PA, 6, AF::k15>	AfEVENTOUT_PA6;
typedef AnyAFR<GpioPortId::PA, 7, AF::k15>	AfEVENTOUT_PA7;
typedef AnyAFR<GpioPortId::PA, 8, AF::k15>	AfEVENTOUT_PA8;
typedef AnyAFR<GpioPortId::PA, 9, AF::k15>	AfEVENTOUT_PA9;
typedef AnyAFR<GpioPortId::PA, 10, AF::k15>	AfEVENTOUT_PA10;
typedef AnyAFR<GpioPortId::PA, 11, AF::k15>	AfEVENTOUT_PA11;
typedef AnyAFR<GpioPortId::PA, 12, AF::k15>	AfEVENTOUT_PA12;
typedef AnyAFR<GpioPortId::PA, 13, AF::k15>	AfEVENTOUT_PA13;
typedef AnyAFR<GpioPortId::PA, 14, AF::k15>	AfEVENTOUT_PA14;
typedef AnyAFR<GpioPortId::PA, 15, AF::k15>	AfEVENTOUT_PA15;
typedef AnyAFR<GpioPortId::PB, 0, AF::k15>	AfEVENTOUT_PB0;
typedef AnyAFR<GpioPortId::PB, 1, AF::k15>	AfEVENTOUT_PB1;
typedef AnyAFR<GpioPortId::PB, 2, AF::k15>	AfEVENTOUT_PB2;
typedef AnyAFR<GpioPortId::PB, 3, AF::k15>	AfEVENTOUT_PB3;
typedef AnyAFR<GpioPortId::PB, 4, AF::k15>	AfEVENTOUT_PB4;
typedef AnyAFR<GpioPortId::PB, 5, AF::k15>	AfEVENTOUT_PB5;
typedef AnyAFR<GpioPortId::PB, 6, AF::k15>	AfEVENTOUT_PB6;
typedef AnyAFR<GpioPortId::PB, 7, AF::k15>	AfEVENTOUT_PB7;
typedef AnyAFR<GpioPortId::PC, 14, AF::k15>	AfEVENTOUT_PC14;
typedef AnyAFR<GpioPortId::PC, 15, AF::k15>	AfEVENTOUT_PC15;
typedef AnyAFR<GpioPortId::PH, 3, AF::k15>	AfEVENTOUT_PH3;


}	// namespace Gpio
}	// namespace Bmt
