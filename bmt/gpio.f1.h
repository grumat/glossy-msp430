#pragma once

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


/// Private template definition, for internal use only
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
	/// A constant to record the mapping type
	typedef Map MapType;
	/// Constant storing the GPIO port number
	static constexpr GpioPortId kPort_ = kPort;
	/// Base address of the port peripheral
	static constexpr uint32_t kPortBase_ = (GPIOA_BASE + uint32_t(kPort_) * 0x400);
	/// The pin number constant value
	static constexpr uint8_t kPin_ = kPin;
	/// Unused pins are always configured as input
	static constexpr Speed kSpeed_ = kSpeed;
	/// Unused pins are always configured as input
	static constexpr Mode kMode_ = Mode::kInput;
	/// Unused pins are always pulled with a resistor load
	static constexpr PuPd kPuPd_ = kPuPd;
	/// Input function flag
	static constexpr bool kIsInput_ = kMode_ == Mode::kInput || kMode_ == Mode::kAnalog;
	/// Configuration register combo
	static constexpr uint8_t kCrBits_ = 
		kIsInput_					? 0b0000 
		: kSpeed_ == Speed::kFast	? 0b0011 
		: kSpeed_ == Speed::kMedium	? 0x0001 
									: 0b0010
		|
		kIsInput_ && kPuPd_ == PuPd::kFloating			? 0b0100
		: kIsInput_ && kPuPd_ != PuPd::kFloating		? 0b1000
		: !kIsInput_ && kMode_ == Mode::kOpenDrain		? 0b0100
		: !kIsInput_ && kMode_ == Mode::kAlternate		? 0b1000
		: !kIsInput_ && kMode_ == Mode::kOpenDrainAlt	? 0b1100
														: 0b0000
		;
	/// Constant value for CRL hardware register
	static constexpr uint32_t kCRL_ = kPin >= 8 ? 0UL 
		: kCrBits_ << (kPin_ << 2);
	/// Constant mask value for CRL hardware register
	static constexpr uint32_t kCRL_Mask_ = (kImpl == Impl::kUnchanged) ? 0UL
		: ~(kPin < 8 ? 0x0FUL << (kPin << 2) : 0UL);
	/// Constant value for CRH hardware register
	static constexpr uint32_t kCRH_ = kPin < 8 ? 0UL
		: kCrBits_ << ((kPin - 8) << 2);
	/// Constant mask value for CRH hardware register
	static constexpr uint32_t kCRH_Mask_ = (kImpl == Impl::kUnchanged) ? 0UL
		: ~(kPin >= 8 ? 0x0FUL << ((kPin - 8) << 2) : 0UL);
	/// Effective bit constant value
	static constexpr uint16_t kBitValue_ = (kImpl != Impl::kNormal) ? 0
		: 1 << kPin;
	/// Value that clears the bit on the GPIOx_BSRR register
	static constexpr uint32_t kBsrrValue_ = (kImpl != Impl::kNormal) ? 0
		: 1 << (kPin + 16);
	/// Constant for the initial bit level
	static constexpr uint32_t kODR_ = uint32_t(kInitialLevel) << kPin;
	/// Alternate Function configuration constant
	static constexpr uint32_t kAfConf_ = (kImpl == Impl::kNormal)
		? Map::kConf : 0x00000000U;
	/// Alternate Function configuration mask constant (inverted)
	static constexpr uint32_t kAfMask_ = (kImpl == Impl::kNormal)
		? Map::kMask : 0xFFFFFFFFU;
	/// Constant Flag indicating that no Alternate Function is required
	static constexpr bool kAfDisabled_ = (kImpl == Impl::kNormal) 
		? Map::kNoRemap : true;
	/// Constant flag to indicate that a bit is not used for a particular configuration
	static constexpr bool kIsUnused_ = (kImpl != Impl::kNormal);


	/// Access to the peripheral memory space
	constexpr static volatile GPIO_TypeDef* Io() { return (volatile GPIO_TypeDef*)kPortBase_; }

	/// Apply default configuration for the pin.
	constexpr static void SetupPinMode()
	{
		if (kImpl != Impl::kUnchanged)
		{
			volatile GPIO_TypeDef* port = Io();
			if (kPin < 8)
				port->CRL = (port->CRL & kCRL_Mask_) | kCRL_;
			else
				port->CRH = (port->CRH & kCRH_Mask_) | kCRH_;
		}
	}
	/// Apply default configuration for the pin.
	constexpr static void Setup()
	{
		if (kImpl != Impl::kUnchanged)
		{
			SetupPinMode();
			Map::Enable();
			if (kIsInput_ && kPuPd_ == PuPd::kPullUp)
				Set(Level::kHigh);
			else if (kIsInput_ && kPuPd_ == PuPd::kPullDown)
				Set(Level::kLow);
			else if(!kIsInput_)
				Set(kInitialLevel);
		}
	}
	/// Apply a custom configuration to the pin, after initialization
	constexpr static void Setup(Mode conf, Speed mode)
	{
		if (kImpl == Impl::kNormal)
		{
			/// Input function flag
			const bool is_input = kMode_ == Mode::kInput || kMode_ == Mode::kAnalog;
			/// Configuration register combo
			const uint32_t cr_bits =
				is_input ? 0b0000
				: kSpeed_ == Speed::kFast ? 0b0011
				: kSpeed_ == Speed::kMedium ? 0x0001
				: 0b0010
				|
				is_input && kPuPd_ == PuPd::kFloating ? 0b0100
				: is_input && kPuPd_ != PuPd::kFloating ? 0b1000
				: !is_input && kMode_ == Mode::kOpenDrain ? 0b0100
				: !is_input && kMode_ == Mode::kAlternate ? 0b1000
				: !is_input && kMode_ == Mode::kOpenDrainAlt ? 0b1100
				: 0b0000
				;
			volatile GPIO_TypeDef* port = Io();
			/// Pin number defines if CRL or CRH is used
			if (kPin < 8)
			{
				const uint32_t crl = cr_bits << (kPin << 2);
				port->CRL = (port->CRL & kCRL_Mask_) | crl;
			}
			else
			{
				const uint32_t crh = cr_bits << ((kPin - 8) << 2);
				port->CRH = (port->CRH & kCRH_Mask_) | crh;
			}
		}
	}

	/// Sets pin up. The pin will be high as long as it is configured as GPIO output
	constexpr static void SetHigh(void)
	{
		if (kImpl == Impl::kNormal)
		{
			volatile GPIO_TypeDef* port = Io();
			port->BSRR = kBitValue_;
		}
	};

	/// Sets pin down. The pin will be low as long as it is configured as GPIO output
	constexpr static void SetLow(void)
	{
		if (kImpl == Impl::kNormal)
		{
			volatile GPIO_TypeDef* port = Io();
			port->BRR = kBitValue_;
		}
	}

	/// Sets the pin to the given level. Note that optimizing compiler simplifies literal constants
	constexpr static void Set(const Level value)
	{
		if (value == Level::kHigh)
			SetHigh();
		else
			SetLow();
	}

	/// Reads current Pin electrical state
	constexpr static bool Get(void)
	{
		if (kImpl == Impl::kNormal)
		{
			volatile GPIO_TypeDef* port = Io();
			return (port->IDR & kBitValue_) != 0;
		}
		else
			return false;
	}

	/// Checks if current pin electrical state is high
	constexpr static bool IsHigh(void)
	{
		if (kImpl == Impl::kNormal)
		{
			volatile GPIO_TypeDef* port = (volatile GPIO_TypeDef*)kPortBase_;
			return (port->IDR & kBitValue_) != 0;
		}
		else
			return false;	// an unused pin always returns false here
	}
	
	/// Checks if current pin electrical state is low
	constexpr static bool IsLow(void)
	{
		return !IsHigh();
	}

	/// Toggles pin state
	constexpr static void Toggle(void)
	{
		if (kImpl == Impl::kNormal)
		{
			volatile GPIO_TypeDef* port = (volatile GPIO_TypeDef*)kPortBase_;
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
**		typedef AnyPin<GpioPortId::PA, 5, Mode::kOutput, Speed::kFast, Level::kHigh> MY_SPI_CLK;
**
**	Also see the shortcut templates that reduces the clutter to declare common
**	IO forms: AnyInputPin<>, FloatingPin<>, InputPullUpPin<> and InputPullDownPin<>.
**
**	Device specific peripherals are also mapped into handy data-types, like for
**	example: SPI1_SCK_PA5, ADC12_IN0 and TIM2_CH2_PA1.
**
**	@tparam kPort: the GPIO port.
**	@tparam kPin: The GPIO pin number.
**	@tparam kMode: Defines pin direction or output configuration.
**	@tparam kPuPd: Defines pull up/Pull down activity.
**	@tparam kInitialLevel: Defines the level of the pin to be initialized.
**	@tparam Map: A data-type that allows STM32 Pin Remap. Definitions are found on remap.h.
*/
template<
	const GpioPortId kPort						///< The GPIO port
	, const uint8_t kPin						///< The pin of the port
	, const Mode kMode = Mode::kInput			///< Mode to configure the port
	, const Speed kSpeed = Speed::kInput		///< Speed for the pin
	, const PuPd kPuPd = PuPd::kFloating		///< Additional pin configuration (applies to input pin)
	, const Level kInitialLevel = Level::kLow	///< Initial pin level (applies to output pin)
	, typename Map = AfNoRemap					///< Pin remapping feature (pinremap.h)
>
class AnyPin : public Private::Implementation_<
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
	, const PuPd kPuPd = PuPd::kFloating		///< Additional pin configuration (applies to input pin)
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
	, const Level kInitialLevel = Level::kLow	///< Initial pin level (applies to output pin)
	, typename Map = AfNoRemap					///< Pin remapping feature (pinremap.h)
>
class AnyOutputPin : public Private::Implementation_<
	Private::Impl::kNormal
	, kPort
	, kPin
	, kMode
	, kSpeed
	, PuPd::kFloating
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
	, const Mode kMode = Mode::kOutput			///< Mode to configure the port
	, const Speed kSpeed = Speed::kFast			///< Speed for the pin
	, const Level kInitialLevel = Level::kLow	///< Initial pin level (applies to output pin)
	, typename Map = AfNoRemap					///< Pin remapping feature (pinremap.h)
>
class AnyAlternateOutPin : public Private::Implementation_<
	Private::Impl::kNormal
	, kPort
	, kPin
	, kMode
	, kSpeed
	, PuPd::kFloating
	, kInitialLevel
	, Map
>
{
private:
	constexpr void Validate_()
	{
		static_assert(kMode >= Mode::kAlternate, "template requires an output pin configuration");
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
typedef AnyOutputPin<GpioPortId::PA, 0, Speed::kOutput2MHz, Mode::kPushPull, Level::kHigh> GREEN_LED;
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
	/// Combined constant value for CRL hardware register
	static constexpr uint32_t kCrl_ =
		Pin0::kCRL_ | Pin1::kCRL_
		| Pin2::kCRL_ | Pin3::kCRL_
		| Pin4::kCRL_ | Pin5::kCRL_
		| Pin6::kCRL_ | Pin7::kCRL_
		| Pin8::kCRL_ | Pin9::kCRL_
		| Pin10::kCRL_ | Pin11::kCRL_
		| Pin12::kCRL_ | Pin13::kCRL_
		| Pin14::kCRL_ | Pin15::kCRL_
		;
	/// Combined constant mask value for CRL hardware register
	static constexpr uint32_t kCrlMask_ =
		Pin0::kCRL_Mask_ & Pin1::kCRL_Mask_
		& Pin2::kCRL_Mask_ & Pin3::kCRL_Mask_
		& Pin4::kCRL_Mask_ & Pin5::kCRL_Mask_
		& Pin6::kCRL_Mask_ & Pin7::kCRL_Mask_
		& Pin8::kCRL_Mask_ & Pin9::kCRL_Mask_
		& Pin10::kCRL_Mask_ & Pin11::kCRL_Mask_
		& Pin12::kCRL_Mask_ & Pin13::kCRL_Mask_
		& Pin14::kCRL_Mask_ & Pin15::kCRL_Mask_
		;
	/// Combined constant value for CRH hardware register (negative logic)
	static constexpr uint32_t kCrh_ =
		Pin0::kCRH_ | Pin1::kCRH_
		| Pin2::kCRH_ | Pin3::kCRH_
		| Pin4::kCRH_ | Pin5::kCRH_
		| Pin6::kCRH_ | Pin7::kCRH_
		| Pin8::kCRH_ | Pin9::kCRH_
		| Pin10::kCRH_ | Pin11::kCRH_
		| Pin12::kCRH_ | Pin13::kCRH_
		| Pin14::kCRH_ | Pin15::kCRH_
		;
	/// Combined constant mask value for CRH hardware register
	static constexpr uint32_t kCrhMask_ =
		Pin0::kCRH_Mask_ & Pin1::kCRH_Mask_
		& Pin2::kCRH_Mask_ & Pin3::kCRH_Mask_
		& Pin4::kCRH_Mask_ & Pin5::kCRH_Mask_
		& Pin6::kCRH_Mask_ & Pin7::kCRH_Mask_
		& Pin8::kCRH_Mask_ & Pin9::kCRH_Mask_
		& Pin10::kCRH_Mask_ & Pin11::kCRH_Mask_
		& Pin12::kCRH_Mask_ & Pin13::kCRH_Mask_
		& Pin14::kCRH_Mask_ & Pin15::kCRH_Mask_
		;
	/// Constant for the initial bit level
	static constexpr uint32_t kOdr_ =
		Pin0::kODR_ | Pin1::kODR_
		| Pin2::kODR_ | Pin3::kODR_
		| Pin4::kODR_ | Pin5::kODR_
		| Pin6::kODR_ | Pin7::kODR_
		| Pin8::kODR_ | Pin9::kODR_
		| Pin10::kODR_ | Pin11::kODR_
		| Pin12::kODR_ | Pin13::kODR_
		| Pin14::kODR_ | Pin15::kODR_
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
	constexpr static volatile GPIO_TypeDef& Io() { return *(volatile GPIO_TypeDef*)kPortBase_; }

	/// Initialize to GPIO overwriting all previous configuration of the port
	/// This means that PinUnchanged<> pins have the same behavior as PinUnused<>.
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

		// Apply Alternate Function configuration
		AfRemapTemplate<kAfConf_, kAfMask_>::Enable();
		// Base address of the peripheral registers
		volatile GPIO_TypeDef &port = Io();
		// Don't turn alternate function clock on if not required
		if(kAfDisabled_)
			RCC->APB2ENR |= (1 << (uint32_t(kPort_) + RCC_APB2ENR_IOPAEN_Pos));
		else
			RCC->APB2ENR |= (1 << (uint32_t(kPort_) + RCC_APB2ENR_IOPAEN_Pos)) | RCC_APB2ENR_AFIOEN;
		port.CRL = kCrl_;
		port.CRH = kCrh_;
		port.ODR = kOdr_;
	}
	//! Apply state of pin group merging with previous GPI contents
	constexpr static void Enable(void)
	{
		// Apply Alternate Function configuration
		AfRemapTemplate<kAfConf_, kAfMask_>::Enable();
		// Base address of the peripheral registers
		volatile GPIO_TypeDef& port = Io();
		port.CRL = (port.CRL & kCrlMask_) | kCrl_;
		port.CRH = (port.CRH & kCrhMask_) | kCrh_;
		port.ODR = (port.ODR & ~kBitValue_) | kOdr_;
	}
	//! Not an ideal approach, but float everything
	constexpr static void Disable(void)
	{
		// Base address of the peripheral registers
		volatile GPIO_TypeDef& port = Io();
		RCC->APB2ENR |= (1 << (uint32_t(kPort_) + RCC_APB2ENR_IOPAEN_Pos));
		volatile uint32_t delay = RCC->APB2ENR & (1 << (uint32_t(kPort_) + RCC_APB2ENR_IOPAEN_Pos));
		port.CRL = 0x44444444;
		port.CRH = 0x44444444;
		// Remove bits applying inverted Alternate Function configuration mask constant
		AFIO->MAPR &= kAfMask_;
		RCC->APB2ENR &= ~(1 << (uint32_t(kPort_) + RCC_APB2ENR_IOPAEN_Pos));
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


//////////////////////////////////////////////////////////////////////
// ADC12
//////////////////////////////////////////////////////////////////////
/// A default configuration for ADC12 IN0 on PA0
typedef AnyAnalogPin<GpioPortId::PA, 0> ADC12_IN0;
/// A default configuration for ADC12 IN1 on PA1
typedef AnyAnalogPin<GpioPortId::PA, 1> ADC12_IN1;
/// A default configuration for ADC12 IN2 on PA2
typedef AnyAnalogPin<GpioPortId::PA, 2> ADC12_IN2;
/// A default configuration for ADC12 IN3 on PA3
typedef AnyAnalogPin<GpioPortId::PA, 3> ADC12_IN3;
/// A default configuration for ADC12 IN4 on PA4
typedef AnyAnalogPin<GpioPortId::PA, 4> ADC12_IN4;
/// A default configuration for ADC12 IN5 on PA5
typedef AnyAnalogPin<GpioPortId::PA, 5> ADC12_IN5;
/// A default configuration for ADC12 IN6 on PA6
typedef AnyAnalogPin<GpioPortId::PA, 6> ADC12_IN6;
/// A default configuration for ADC12 IN7 on PA7
typedef AnyAnalogPin<GpioPortId::PA, 7> ADC12_IN7;
/// A default configuration for ADC12 IN8 on PB0
typedef AnyAnalogPin<GpioPortId::PB, 0> ADC12_IN8;
/// A default configuration for ADC12 IN9 on PB1
typedef AnyAnalogPin<GpioPortId::PB, 1> ADC12_IN9;
/// A default configuration for ADC12 IN10 on PC0
typedef AnyAnalogPin<GpioPortId::PC, 0> ADC12_IN10;
/// A default configuration for ADC12 IN11 on PC1
typedef AnyAnalogPin<GpioPortId::PC, 1> ADC12_IN11;
/// A default configuration for ADC12 IN12 on PC2
typedef AnyAnalogPin<GpioPortId::PC, 2> ADC12_IN12;
/// A default configuration for ADC12 IN13 on PC3
typedef AnyAnalogPin<GpioPortId::PC, 3> ADC12_IN13;
/// A default configuration for ADC12 IN14 on PC4
typedef AnyAnalogPin<GpioPortId::PC, 4> ADC12_IN14;
/// A default configuration for ADC12 IN15 on PC5
typedef AnyAnalogPin<GpioPortId::PC, 5> ADC12_IN15;


//////////////////////////////////////////////////////////////////////
// CAN - Configuration 1 (cannot mix pins between configuration)
//////////////////////////////////////////////////////////////////////
/// A default configuration for CAN/RX on PA11 pin
typedef AnyInputPin<GpioPortId::PA, 11, PuPd::kPullUp, AfCan_PA11_12>			CAN_RX_PA11;
/// A default configuration for CAN/TX on PA12 pin
typedef AnyAlternateOutPin<GpioPortId::PA, 12, Mode::kOpenDrainAlt, Speed::kFast, Level::kLow, AfCan_PA11_12>	CAN_TX_PA12;

//////////////////////////////////////////////////////////////////////
// CAN - Configuration 2 (cannot mix pins between configuration)
//////////////////////////////////////////////////////////////////////
/// A default configuration for CAN/RX on PB8 pin
typedef AnyInputPin<GpioPortId::PB, 8, PuPd::kPullUp, AfCan_PB8_9>				CAN_RX_PB8;
/// A default configuration for CAN/TX on PB9 pin
typedef AnyAlternateOutPin<GpioPortId::PB, 9, Mode::kOpenDrainAlt, Speed::kFast, Level::kLow, AfCan_PB8_9>	CAN_TX_PB9;

//////////////////////////////////////////////////////////////////////
// CAN - Configuration 3 (cannot mix pins between configuration)
//////////////////////////////////////////////////////////////////////
/// A default configuration for CAN/RX on PD0 pin
typedef AnyInputPin<GpioPortId::PD, 0, PuPd::kPullUp, AfCan_PD0_1>				CAN_RX_PD0;
/// A default configuration for CAN/TX on PD1 pin
typedef AnyAlternateOutPin<GpioPortId::PD, 1, Mode::kOpenDrainAlt, Speed::kFast, Level::kLow, AfCan_PD0_1>	CAN_TX_PD1;

//////////////////////////////////////////////////////////////////////
// GPIO vs Oscillator
//////////////////////////////////////////////////////////////////////
/// A default configuration to map OSC_IN to PD0
template<const Mode kMode, const Speed kSpeed, const Level kLevel = Level::kLow>
struct ALT_PD0 : AnyAlternateOutPin<GpioPortId::PD, 0, kMode, kSpeed, kLevel, Af_PD01_GPIO> {};
/// A default configuration to map OSC_OUT to PD1
template<const Mode kMode, const Speed kSpeed, const Level kLevel = Level::kLow>
struct ALT_PD1 : AnyAlternateOutPin<GpioPortId::PD, 1, kMode, kSpeed, kLevel, Af_PD01_GPIO> {};

//////////////////////////////////////////////////////////////////////
// I2C1 - Configuration 1
//////////////////////////////////////////////////////////////////////
/// A default configuration to map I2C1 SCL on PB6 pin
typedef AnyAlternateOutPin<GpioPortId::PB, 6, Mode::kOpenDrainAlt, Speed::kFast, Level::kLow, AfI2C1_PB6_7>	I2C1_SCL_PB6;
/// A default configuration to map I2C1 SDA on PB7 pin
typedef AnyAlternateOutPin<GpioPortId::PB, 7, Mode::kOpenDrainAlt, Speed::kFast, Level::kLow, AfI2C1_PB6_7>	I2C1_SDA_PB7;
// I2C1 - Configuration 2
/// A default configuration to map I2C1 SCL on PB8 pin
typedef AnyAlternateOutPin<GpioPortId::PB, 8, Mode::kOpenDrainAlt, Speed::kFast, Level::kLow, AfI2C1_PB8_9>	I2C1_SCL_PB8;
/// A default configuration to map I2C1 SDA on PB9 pin
typedef AnyAlternateOutPin<GpioPortId::PB, 9, Mode::kOpenDrainAlt, Speed::kFast, Level::kLow, AfI2C1_PB8_9>	I2C1_SDA_PB9;
// I2C1 - Configuration 1 & 2
/// A default configuration to map I2C1 SMBAI on PB5 pin
typedef AnyAlternateOutPin<GpioPortId::PB, 5, Mode::kOpenDrainAlt, Speed::kFast, Level::kLow, AfNoRemap>	I2C1_SMBAI_PB5;

//////////////////////////////////////////////////////////////////////
// I2C2
//////////////////////////////////////////////////////////////////////
/// A default configuration to map I2C2 SCL on PB10 pin
typedef AnyAlternateOutPin<GpioPortId::PB, 10, Mode::kOpenDrainAlt, Speed::kFast, Level::kLow, AfNoRemap>	I2C2_SCL_PB10;
/// A default configuration to map I2C2 SDA on PB11 pin
typedef AnyAlternateOutPin<GpioPortId::PB, 11, Mode::kOpenDrainAlt, Speed::kFast, Level::kLow, AfNoRemap>	I2C2_SDA_PB11;
/// A default configuration to map I2C2 SMBAI on PB12 pin
typedef AnyAlternateOutPin<GpioPortId::PB, 12, Mode::kOpenDrainAlt, Speed::kFast, Level::kLow, AfNoRemap>	I2C2_SMBAI_PB12;

//////////////////////////////////////////////////////////////////////
// SPI1 - Configuration 1
//////////////////////////////////////////////////////////////////////
/// A default configuration to map SPI1 NSS on PA4 pin (master)
typedef AnyAlternateOutPin<GpioPortId::PA, 4, Mode::kOpenDrainAlt, Speed::kFast, Level::kHigh, AfSpi1_PA4_5_6_7>	SPI1_NSS_PA4;
/// A default configuration to map SPI1 NSS on PA4 pin (slave)
typedef AnyInputPin<GpioPortId::PA, 4, PuPd::kPullUp, AfSpi1_PA4_5_6_7>												SPI1_NSS_PA4_SLAVE;
/// A default configuration to map SPI1 SCK on PA5 pin
typedef AnyAlternateOutPin<GpioPortId::PA, 5, Mode::kAlternate, Speed::kFast, Level::kLow, AfSpi1_PA4_5_6_7>		SPI1_SCK_PA5;
/// A default configuration to map SPI1 MISO on PA6 pin
typedef AnyInputPin<GpioPortId::PA, 6, PuPd::kPullUp, AfSpi1_PA4_5_6_7>												SPI1_MISO_PA6;
/// A default configuration to map SPI1 MOSI on PA7 pin
typedef AnyAlternateOutPin<GpioPortId::PA, 7, Mode::kAlternate, Speed::kFast, Level::kHigh, AfSpi1_PA4_5_6_7>		SPI1_MOSI_PA7;
// SPI1 - Configuration 2
/// A default configuration to map SPI1 NSS on PA15 pin (master)
typedef AnyAlternateOutPin<GpioPortId::PA, 15, Mode::kOpenDrainAlt, Speed::kFast, Level::kHigh, AfSpi1_PA15_PB3_4_5>	SPI1_NSS_PA15;
/// A default configuration to map SPI1 NSS on PA15 pin (slave)
typedef AnyInputPin<GpioPortId::PA, 15, PuPd::kPullUp, AfSpi1_PA15_PB3_4_5>											SPI1_NSS_PA15_SLAVE;
/// A default configuration to map SPI1 SCK on PB3 pin
typedef AnyAlternateOutPin<GpioPortId::PB, 3, Mode::kAlternate, Speed::kFast, Level::kLow, AfSpi1_PA15_PB3_4_5>		SPI1_SCK_PB3;
/// A default configuration to map SPI1 MISO on PB4 pin
typedef AnyInputPin<GpioPortId::PB, 4, PuPd::kPullUp, AfSpi1_PA15_PB3_4_5>											SPI1_MISO_PB4;
/// A default configuration to map SPI1 MOSI on PB5 pin
typedef AnyAlternateOutPin<GpioPortId::PB, 5, Mode::kAlternate, Speed::kFast, Level::kHigh, AfSpi1_PA15_PB3_4_5>	SPI1_MOSI_PB5;

//////////////////////////////////////////////////////////////////////
// SPI2
//////////////////////////////////////////////////////////////////////
/// A default configuration to map SPI2 NSS on PB12 pin (master)
typedef AnyAlternateOutPin<GpioPortId::PB, 12, Mode::kOpenDrainAlt, Speed::kFast, Level::kHigh, AfNoRemap>	SPI2_NSS_PB12;
/// A default configuration to map SPI2 NSS on PB12 pin (slave)
typedef AnyInputPin<GpioPortId::PB, 12, PuPd::kPullUp, AfNoRemap>											SPI2_NSS_PB12_SLAVE;
/// A default configuration to map SPI2 SCK on PB13 pin
typedef AnyAlternateOutPin<GpioPortId::PB, 13, Mode::kAlternate, Speed::kFast, Level::kLow, AfNoRemap>		SPI2_SCK_PB13;
/// A default configuration to map SPI2 MISO on PB14 pin
typedef AnyInputPin<GpioPortId::PB, 14, PuPd::kPullUp, AfNoRemap>											SPI2_MISO_PB14;
/// A default configuration to map SPI2 MOSI on PB15 pin
typedef AnyAlternateOutPin<GpioPortId::PB, 15, Mode::kAlternate, Speed::kFast, Level::kHigh, AfNoRemap>		SPI2_MOSI_PB15;


//////////////////////////////////////////////////////////////////////
// TIM1 - Configuration 1
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map TIM1 ETR on PA12 pin
template<const PuPd kPuPd = PuPd::kFloating>
struct TIM1_ETR_PA12 : AnyInputPin<GpioPortId::PA, 12, kPuPd, AfTim1_PA12_8_9_10_11_PB12_13_14_15> {};
/// A default configuration to map TIM1 ETR on PA12 pin (input)
typedef TIM1_ETR_PA12<>												TIM1_ETR_PA12_IN;

/// A generic configuration to map TIM1 CH1 on PA8 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM1_CH1_PA8 : AnyPin<GpioPortId::PA, 8, kMode, kSpeed, kPuPd, kLevel, AfTim1_PA12_8_9_10_11_PB12_13_14_15> {};
/// A default configuration to map TIM1 CH1 on PA8 pin (output)
typedef TIM1_CH1_PA8<Mode::kAlternate, Speed::kFast>	TIM1_CH1_PA8_OUT;
/// A default configuration to map TIM1 CH1 on PA8 pin (input)
typedef TIM1_CH1_PA8<Mode::kInput, Speed::kInput>					TIM1_CH1_PA8_IN;

/// A generic configuration to map TIM1 CH2 on PA9 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM1_CH2_PA9 : AnyPin<GpioPortId::PA, 9, kMode, kSpeed, kPuPd, kLevel, AfTim1_PA12_8_9_10_11_PB12_13_14_15> {};
/// A default configuration to map TIM1 CH2 on PA9 pin (output)
typedef TIM1_CH2_PA9<Mode::kAlternate, Speed::kFast>	TIM1_CH2_PA9_OUT;
/// A default configuration to map TIM1 CH2 on PA9 pin (input)
typedef TIM1_CH2_PA9<Mode::kInput, Speed::kInput>					TIM1_CH2_PA9_IN;

/// A generic configuration to map TIM1 CH3 on PA10 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM1_CH3_PA10 : AnyPin<GpioPortId::PA, 10, kMode, kSpeed, kPuPd, kLevel, AfTim1_PA12_8_9_10_11_PB12_13_14_15> {};
/// A default configuration to map TIM1 CH3 on PA10 pin (output)
typedef TIM1_CH3_PA10<Mode::kAlternate, Speed::kFast>	TIM1_CH3_PA10_OUT;
/// A default configuration to map TIM1 CH3 on PA10 pin (input)
typedef TIM1_CH3_PA10<Mode::kInput, Speed::kInput>					TIM1_CH3_PA10_IN;

/// A generic configuration to map TIM1 CH4 on PA11 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM1_CH4_PA11 : AnyPin<GpioPortId::PA, 11, kMode, kSpeed, kPuPd, kLevel, AfTim1_PA12_8_9_10_11_PB12_13_14_15> {};
/// A default configuration to map TIM1 CH4 on PA11 pin (output)
typedef TIM1_CH4_PA11<Mode::kAlternate, Speed::kFast>	TIM1_CH4_PA11_OUT;
/// A default configuration to map TIM1 CH4 on PA11 pin (input)
typedef TIM1_CH4_PA11<Mode::kInput, Speed::kInput>					TIM1_CH4_PA11_IN;

/// A generic configuration to map TIM1 BKIN on PB12 pin
template<const PuPd kPuPd>
struct TIM1_BKIN_PB12 : AnyInputPin<GpioPortId::PB, 12, kPuPd, AfTim1_PA12_8_9_10_11_PB12_13_14_15> {};
/// A default configuration to map TIM1 BKIN on PB12 pin (input)
typedef TIM1_BKIN_PB12<PuPd::kFloating>								TIM1_BKIN_PB12_IN;

/// A generic configuration to map TIM1 CH1N on PB13 pin
template<const Mode kMode, const Speed kSpeed, const Level kLevel = Level::kLow>
struct TIM1_CH1N_PB13 : AnyAlternateOutPin<GpioPortId::PB, 13, kMode, kSpeed, kLevel, AfTim1_PA12_8_9_10_11_PB12_13_14_15> {};
/// A default configuration to map TIM1 CH1N on PB13 pin (output)
typedef TIM1_CH1N_PB13<Mode::kAlternate, Speed::kFast>				TIM1_CH1N_PB13_OUT;

/// A generic configuration to map TIM1 CH2N on PB14 pin
template<const Mode kMode, const Speed kSpeed, const Level kLevel = Level::kLow>
struct TIM1_CH2N_PB14 : AnyAlternateOutPin<GpioPortId::PB, 14, kMode, kSpeed, kLevel, AfTim1_PA12_8_9_10_11_PB12_13_14_15> {};
/// A default configuration to map TIM1 CH2N on PB14 pin (output)
typedef TIM1_CH2N_PB14<Mode::kAlternate, Speed::kFast>				TIM1_CH2N_PB14_OUT;

/// A generic configuration to map TIM1 CH3N on PB15 pin
template<const Mode kMode, const Speed kSpeed, const Level kLevel = Level::kLow>
struct TIM1_CH3N_PB15 : AnyAlternateOutPin<GpioPortId::PB, 15, kMode, kSpeed, kLevel, AfTim1_PA12_8_9_10_11_PB12_13_14_15> {};
/// A default configuration to map TIM1 CH3N on PB15 pin (output)
typedef TIM1_CH3N_PB15<Mode::kAlternate, Speed::kFast>				TIM1_CH3N_PB15_OUT;

//////////////////////////////////////////////////////////////////////
// TIM1 - Configuration 2 (partial remap)
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map TIM1 ETR on PA12 pin
template<const PuPd kPuPd = PuPd::kFloating>
struct TIM1_ETR_PA12_P : AnyInputPin<GpioPortId::PA, 12, kPuPd, AfTim1_PA12_8_9_10_11_6_7_PB0_1> {};
/// A default configuration to map TIM1 ETR on PA12 pin (input)
typedef TIM1_ETR_PA12_P<PuPd::kFloating>							TIM1_ETR_PA12_IN_CFG2;

/// A generic configuration to map TIM1 CH1 on PA8 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM1_CH1_PA8_P : AnyPin<GpioPortId::PA, 8, kMode, kSpeed, kPuPd, kLevel, AfTim1_PA12_8_9_10_11_6_7_PB0_1> {};
/// A default configuration to map TIM1 CH1 on PA8 pin (output)
typedef TIM1_CH1_PA8_P<Mode::kAlternate, Speed::kFast>	TIM1_CH1_PA8_OUT_CFG2;
/// A default configuration to map TIM1 CH1 on PA8 pin (input)
typedef TIM1_CH1_PA8_P<Mode::kInput, Speed::kInput>					TIM1_CH1_PA8_IN_CFG2;

/// A generic configuration to map TIM1 CH2 on PA9 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM1_CH2_PA9_P : AnyPin<GpioPortId::PA, 9, kMode, kSpeed, kPuPd, kLevel, AfTim1_PA12_8_9_10_11_6_7_PB0_1> {};
/// A default configuration to map TIM1 CH2 on PA9 pin (output)
typedef TIM1_CH2_PA9_P<Mode::kAlternate, Speed::kFast>	TIM1_CH2_PA9_OUT_CFG2;
/// A default configuration to map TIM1 CH2 on PA9 pin (input)
typedef TIM1_CH2_PA9_P<Mode::kInput, Speed::kInput>					TIM1_CH2_PA9_IN_CFG2;

/// A generic configuration to map TIM1 CH3 on PA10 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM1_CH3_PA10_P : AnyPin<GpioPortId::PA, 10, kMode, kSpeed, kPuPd, kLevel, AfTim1_PA12_8_9_10_11_6_7_PB0_1> {};
/// A default configuration to map TIM1 CH3 on PA10 pin (output)
typedef TIM1_CH3_PA10_P<Mode::kAlternate, Speed::kFast>	TIM1_CH3_PA10_OUT_CFG2;
/// A default configuration to map TIM1 CH3 on PA10 pin (input)
typedef TIM1_CH3_PA10_P<Mode::kInput, Speed::kInput>				TIM1_CH3_PA10_IN_CFG2;

/// A generic configuration to map TIM1 CH4 on PA11 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM1_CH4_PA11_P : AnyPin<GpioPortId::PA, 11, kMode, kSpeed, kPuPd, kLevel, AfTim1_PA12_8_9_10_11_6_7_PB0_1> {};
/// A default configuration to map TIM1 CH4 on PA11 pin (output)
typedef TIM1_CH4_PA11_P<Mode::kAlternate, Speed::kFast>	TIM1_CH4_PA11_OUT_CFG2;
/// A default configuration to map TIM1 CH4 on PA11 pin (input)
typedef TIM1_CH4_PA11_P<Mode::kInput, Speed::kInput>				TIM1_CH4_PA11_IN_CFG2;

/// A generic configuration to map TIM1 BKIN on PA6 pin
template<const PuPd kPuPd = PuPd::kFloating>
struct TIM1_BKIN_PA6_P : AnyInputPin<GpioPortId::PA, 6, kPuPd, AfTim1_PA12_8_9_10_11_6_7_PB0_1> {};
/// A default configuration to map TIM1 BKIN on PA6 pin (input)
typedef TIM1_BKIN_PA6_P<PuPd::kFloating>							TIM1_BKIN_PA6_IN_CFG2;

/// A generic configuration to map TIM1 CH1N on PA7 pin
template<const Mode kMode, const Speed kSpeed, const Level kLevel = Level::kLow>
struct TIM1_CH1N_PA7_P : AnyAlternateOutPin<GpioPortId::PA, 7, kMode, kSpeed, kLevel, AfTim1_PA12_8_9_10_11_6_7_PB0_1> {};
/// A default configuration to map TIM1 CH1N on PA7 pin (output)
typedef TIM1_CH1N_PA7_P<Mode::kAlternate, Speed::kFast>				TIM1_CH1N_PA7_OUT_CFG2;

/// A generic configuration to map TIM1 CH2N on PB0 pin
template<const Mode kMode, const Speed kSpeed, const Level kLevel = Level::kLow>
struct TIM1_CH2N_PB0_P : AnyAlternateOutPin<GpioPortId::PB, 0, kMode, kSpeed, kLevel, AfTim1_PA12_8_9_10_11_6_7_PB0_1> {};
/// A default configuration to map TIM1 CH2N on PB0 pin (output)
typedef TIM1_CH2N_PB0_P<Mode::kAlternate, Speed::kFast>				TIM1_CH2N_PB0_OUT_CFG2;

/// A generic configuration to map TIM1 CH3N on PB1 pin
template<const Mode kMode, const Speed kSpeed, const Level kLevel = Level::kLow>
struct TIM1_CH3N_PB1_P : AnyAlternateOutPin<GpioPortId::PB, 1, kMode, kSpeed, kLevel, AfTim1_PA12_8_9_10_11_6_7_PB0_1> {};
/// A default configuration to map TIM1 CH3N on PB1 pin (output)
typedef TIM1_CH3N_PB1_P<Mode::kAlternate, Speed::kFast>				TIM1_CH3N_PB1_OUT_CFG2;

//////////////////////////////////////////////////////////////////////
// TIM1 - Configuration 3
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map TIM1 ETR on PE7 pin
template<const PuPd kPuPd = PuPd::kFloating>
struct TIM1_ETR_PE7 : AnyInputPin<GpioPortId::PE, 7, kPuPd, AfTim1_PE7_9_11_13_14_15_8_10_12> {};
/// A default configuration to map TIM1 ETR on PE7 pin (input)
typedef TIM1_ETR_PE7<PuPd::kFloating>								TIM1_ETR_PE7_IN_CFG3;

/// A generic configuration to map TIM1 CH1 on PE9 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM1_CH1_PE9 : AnyPin<GpioPortId::PE, 9, kMode, kSpeed, kPuPd, kLevel, AfTim1_PE7_9_11_13_14_15_8_10_12> {};
/// A default configuration to map TIM1 CH1 on PE9 pin (output)
typedef TIM1_CH1_PE9<Mode::kAlternate, Speed::kFast>				TIM1_CH1_PE9_OUT_CFG3;
/// A default configuration to map TIM1 CH1 on PE9 pin (input)
typedef TIM1_CH1_PE9<Mode::kInput, Speed::kInput>					TIM1_CH1_PE9_IN_CFG3;

/// A generic configuration to map TIM1 CH2 on PE11 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM1_CH2_PE11 : AnyPin<GpioPortId::PE, 11, kMode, kSpeed, kPuPd, kLevel, AfTim1_PE7_9_11_13_14_15_8_10_12> {};
/// A default configuration to map TIM1 CH2 on PE11 pin (output)
typedef TIM1_CH2_PE11<Mode::kAlternate, Speed::kFast>				TIM1_CH2_PE11_OUT_CFG3;
/// A default configuration to map TIM1 CH2 on PE11 pin (input)
typedef TIM1_CH2_PE11<Mode::kInput, Speed::kInput>					TIM1_CH2_PE11_IN_CFG3;

/// A generic configuration to map TIM1 CH3 on PE13 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM1_CH3_PE13 : AnyPin<GpioPortId::PE, 13, kMode, kSpeed, kPuPd, kLevel, AfTim1_PE7_9_11_13_14_15_8_10_12> {};
/// A default configuration to map TIM1 CH3 on PE13 pin (output)
typedef TIM1_CH3_PE13<Mode::kAlternate, Speed::kFast>				TIM1_CH3_PE13_OUT_CFG3;
/// A default configuration to map TIM1 CH3 on PE13 pin (input)
typedef TIM1_CH3_PE13<Mode::kInput, Speed::kInput>					TIM1_CH3_PE13_IN_CFG3;

/// A generic configuration to map TIM1 CH4 on PE14 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM1_CH4_PE14 : AnyPin<GpioPortId::PE, 14, kMode, kSpeed, kPuPd, kLevel, AfTim1_PE7_9_11_13_14_15_8_10_12> {};
/// A default configuration to map TIM1 CH4 on PE14 pin (output)
typedef TIM1_CH4_PE14<Mode::kAlternate, Speed::kFast>				TIM1_CH4_PE14_OUT_CFG3;
/// A default configuration to map TIM1 CH4 on PE14 pin (input)
typedef TIM1_CH4_PE14<Mode::kInput, Speed::kInput>					TIM1_CH4_PE14_IN_CFG3;

/// A generic configuration to map TIM1 BKIN on PE15 pin
template<const PuPd kPuPd = PuPd::kFloating>
struct TIM1_BKIN_PE15 : AnyInputPin<GpioPortId::PE, 15, kPuPd, AfTim1_PE7_9_11_13_14_15_8_10_12> {};
/// A default configuration to map TIM1 BKIN on PE15 pin (input)
typedef TIM1_BKIN_PE15<PuPd::kFloating>								TIM1_BKIN_PE15_IN_CFG3;

/// A generic configuration to map TIM1 CH1N on PE8 pin
template<const Mode kMode, const Speed kSpeed, const Level kLevel = Level::kLow>
struct TIM1_CH1N_PE8 : AnyAlternateOutPin<GpioPortId::PE, 8, kMode, kSpeed, kLevel, AfTim1_PE7_9_11_13_14_15_8_10_12> {};
/// A default configuration to map TIM1 CH1N on PE8 pin (output)
typedef TIM1_CH1N_PE8<Mode::kAlternate, Speed::kFast>				TIM1_CH1N_PE8_OUT_CFG3;

/// A generic configuration to map TIM1 CH2N on PE10 pin
template<const Mode kMode, const Speed kSpeed, const Level kLevel = Level::kLow>
struct TIM1_CH2N_PE10 : AnyAlternateOutPin<GpioPortId::PE, 10, kMode, kSpeed, kLevel, AfTim1_PE7_9_11_13_14_15_8_10_12> {};
/// A default configuration to map TIM1 CH2N on PE10 pin (output)
typedef TIM1_CH2N_PE10<Mode::kAlternate, Speed::kFast>				TIM1_CH2N_PE10_OUT_CFG3;

/// A generic configuration to map TIM1 CH3N on PE12 pin
template<const Mode kMode, const Speed kSpeed, const Level kLevel = Level::kLow>
struct TIM1_CH3N_PE12 : AnyAlternateOutPin<GpioPortId::PE, 12, kMode, kSpeed, kLevel, AfTim1_PE7_9_11_13_14_15_8_10_12> {};
/// A default configuration to map TIM1 CH3N on PE12 pin (output)
typedef TIM1_CH3N_PE12<Mode::kAlternate, Speed::kFast>				TIM1_CH3N_PE12_OUT_CFG3;

//////////////////////////////////////////////////////////////////////
// TIM2 - Configuration 1 (no remap)
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map TIM1 CH1 on PA0 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM2_CH1_PA0 : AnyPin<GpioPortId::PA, 0, kMode, kSpeed, kPuPd, kLevel, AfTim2_PA0_1_2_3> {};
/// A default configuration to map TIM1 CH1 on PA0 pin (output)
typedef TIM2_CH1_PA0<Mode::kAlternate, Speed::kFast>				TIM2_CH1_PA0_OUT;
/// A default configuration to map TIM1 CH1 on PA0 pin (input)
typedef TIM2_CH1_PA0<Mode::kInput, Speed::kInput>					TIM2_CH1_PA0_IN;
/// A default configuration to map TIM1 CH1 on PA0 pin (input)
typedef TIM2_CH1_PA0<Mode::kInput, Speed::kInput>					TIM2_ETR_PA0_IN;

/// A generic configuration to map TIM1 CH2 on PA1 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM2_CH2_PA1 : AnyPin<GpioPortId::PA, 1, kMode, kSpeed, kPuPd, kLevel, AfTim2_PA0_1_2_3> {};
/// A default configuration to map TIM1 CH2 on PA1 pin (output)
typedef TIM2_CH2_PA1<Mode::kAlternate, Speed::kFast>				TIM2_CH2_PA1_OUT;
/// A default configuration to map TIM1 CH2 on PA1 pin (input)
typedef TIM2_CH2_PA1<Mode::kInput, Speed::kInput>					TIM2_CH2_PA1_IN;

/// A generic configuration to map TIM1 CH3 on PA2 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM2_CH3_PA2 : AnyPin<GpioPortId::PA, 2, kMode, kSpeed, kPuPd, kLevel, AfTim2_PA0_1_2_3> {};
/// A default configuration to map TIM1 CH3 on PA2 pin (output)
typedef TIM2_CH3_PA2<Mode::kAlternate, Speed::kFast>				TIM2_CH3_PA2_OUT;
/// A default configuration to map TIM1 CH3 on PA2 pin (input)
typedef TIM2_CH3_PA2<Mode::kInput, Speed::kInput>					TIM2_CH3_PA2_IN;

/// A generic configuration to map TIM1 CH4 on PA3 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM2_CH4_PA3 : AnyPin<GpioPortId::PA, 3, kMode, kSpeed, kPuPd, kLevel, AfTim2_PA0_1_2_3> {};
/// A default configuration to map TIM1 CH4 on PA3 pin (output)
typedef TIM2_CH4_PA3<Mode::kAlternate, Speed::kFast>				TIM2_CH4_PA3_OUT;
/// A default configuration to map TIM1 CH4 on PA3 pin (input)
typedef TIM2_CH4_PA3<Mode::kInput, Speed::kInput>					TIM2_CH4_PA3_IN;

//////////////////////////////////////////////////////////////////////
// TIM2 - Configuration 2 (partial remap)
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map TIM2 CH1 on PA15 pin (partial remap/config 2)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM2_CH1_PA15_P : AnyPin<GpioPortId::PA, 15, kMode, kSpeed, kPuPd, kLevel, AfTim2_PA15_PB3_PA2_3> {};
/// A default configuration to map TIM2 CH1 on PA15 pin (partial remap/config 2 - output)
typedef TIM2_CH1_PA15_P<Mode::kAlternate, Speed::kFast>				TIM2_CH1_PA15_OUT_CFG2;
/// A default configuration to map TIM2 CH1 on PA15 pin (partial remap/config 2 - input)
typedef TIM2_CH1_PA15_P<Mode::kInput, Speed::kInput>				TIM2_CH1_PA15_IN_CFG2;
/// A default configuration to map TIM2 ETR on PA15 pin (partial remap/config 2 - input)
typedef TIM2_CH1_PA15_P<Mode::kInput, Speed::kInput>				TIM2_ETR_PA15_IN_CFG2;

/// A generic configuration to map TIM2 CH2 on PB3 pin (partial remap/config 2)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM2_CH2_PB3_P : AnyPin<GpioPortId::PB, 3, kMode, kSpeed, kPuPd, kLevel, AfTim2_PA15_PB3_PA2_3> {};
/// A default configuration to map TIM2 CH2 on PB3 pin (partial remap/config 2 - output)
typedef TIM2_CH2_PB3_P<Mode::kAlternate, Speed::kFast>				TIM2_CH2_PB3_OUT_CFG2;
/// A default configuration to map TIM2 CH2 on PB3 pin (partial remap/config 2 - input)
typedef TIM2_CH2_PB3_P<Mode::kInput, Speed::kInput>					TIM2_CH2_PB3_IN_CFG2;

/// A generic configuration to map TIM2 CH3 on PA2 pin (partial remap/config 2)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM2_CH3_PA2_P : AnyPin<GpioPortId::PA, 2, kMode, kSpeed, kPuPd, kLevel, AfTim2_PA15_PB3_PA2_3> {};
/// A default configuration to map TIM2 CH3 on PA2 pin (partial remap/config 2 - output)
typedef TIM2_CH3_PA2_P<Mode::kAlternate, Speed::kFast>				TIM2_CH3_PA2_OUT_CFG2;
/// A default configuration to map TIM2 CH3 on PA2 pin (partial remap/config 2 - input)
typedef TIM2_CH3_PA2_P<Mode::kInput, Speed::kInput>					TIM2_CH3_PA2_IN_CFG2;

/// A generic configuration to map TIM2 CH4 on PA3 pin (partial remap/config 2)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM2_CH4_PA3_P : AnyPin<GpioPortId::PA, 3, kMode, kSpeed, kPuPd, kLevel, AfTim2_PA15_PB3_PA2_3> {};
/// A default configuration to map TIM2 CH4 on PA3 pin (partial remap/config 2 - output)
typedef TIM2_CH4_PA3_P<Mode::kAlternate, Speed::kFast>				TIM2_CH4_PA3_OUT_CFG2;
/// A default configuration to map TIM2 CH4 on PA3 pin (partial remap/config 2 - input)
typedef TIM2_CH4_PA3_P<Mode::kInput, Speed::kInput>					TIM2_CH4_PA3_IN_CFG2;

//////////////////////////////////////////////////////////////////////
// TIM2 - Configuration 3 (partial remap)
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map TIM2 CH1 on PA0 pin (partial remap/config 3)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM2_CH1_PA0_P : AnyPin<GpioPortId::PA, 0, kMode, kSpeed, kPuPd, kLevel, AfTim2_PA0_1_PB10_11> {};
/// A default configuration to map TIM2 CH1 on PA0 pin (partial remap/config 3 - output)
typedef TIM2_CH1_PA0_P<Mode::kAlternate, Speed::kFast>				TIM2_CH1_PA0_OUT_CFG3;
/// A default configuration to map TIM2 CH1 on PA0 pin (partial remap/config 3 - input)
typedef TIM2_CH1_PA0_P<Mode::kInput, Speed::kInput>					TIM2_CH1_PA0_IN_CFG3;
/// A default configuration to map TIM2 ETR on PA0 pin (partial remap/config 3 - input)
typedef TIM2_CH1_PA0_P<Mode::kInput, Speed::kInput>					TIM2_ETR_PA0_IN_CFG3;

/// A generic configuration to map TIM2 CH2 on PA1 pin (partial remap/config 3)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM2_CH2_PA1_P : AnyPin<GpioPortId::PA, 1, kMode, kSpeed, kPuPd, kLevel, AfTim2_PA0_1_PB10_11> {};
/// A default configuration to map TIM2 CH2 on PA1 pin (partial remap/config 3 - output)
typedef TIM2_CH2_PA1_P<Mode::kAlternate, Speed::kFast>				TIM2_CH2_PA1_OUT_CFG3;
/// A default configuration to map TIM2 CH2 on PA1 pin (partial remap/config 3 - input)
typedef TIM2_CH2_PA1_P<Mode::kInput, Speed::kInput>					TIM2_CH2_PA1_IN_CFG3;

/// A generic configuration to map TIM2 CH3 on PB10 pin (partial remap/config 3)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM2_CH3_PB10_P : AnyPin<GpioPortId::PB, 10, kMode, kSpeed, kPuPd, kLevel, AfTim2_PA0_1_PB10_11> {};
/// A default configuration to map TIM2 CH3 on PA10 pin (partial remap/config 3 - output)
typedef TIM2_CH3_PB10_P<Mode::kAlternate, Speed::kFast>				TIM2_CH3_PB10_OUT_CFG3;
/// A default configuration to map TIM2 CH3 on PA10 pin (partial remap/config 3 - input)
typedef TIM2_CH3_PB10_P<Mode::kInput, Speed::kInput>				TIM2_CH3_PB10_IN_CFG3;

/// A generic configuration to map TIM2 CH3 on PB11 pin (partial remap/config 3)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM2_CH4_PB11_P : AnyPin<GpioPortId::PB, 11, kMode, kSpeed, kPuPd, kLevel, AfTim2_PA0_1_PB10_11> {};
/// A default configuration to map TIM2 CH3 on PB11 pin (partial remap/config 3 - output)
typedef TIM2_CH4_PB11_P<Mode::kAlternate, Speed::kFast>				TIM2_CH4_PB11_OUT_CFG3;
/// A default configuration to map TIM2 CH3 on PB11 pin (partial remap/config 3 - input)
typedef TIM2_CH4_PB11_P<Mode::kInput, Speed::kInput>				TIM2_CH4_PB11_IN_CFG3;

//////////////////////////////////////////////////////////////////////
// TIM2 - Configuration 4 (full remap)
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map TIM2 CH1 on PA15 pin (full remap/config 4)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM2_CH1_PA15 : AnyPin<GpioPortId::PA, 15, kMode, kSpeed, kPuPd, kLevel, AfTim2_PA15_PB3_10_11> {};
/// A default configuration to map TIM2 CH1 on PA15 pin (full remap/config 4 - output)
typedef TIM2_CH1_PA15<Mode::kAlternate, Speed::kFast>	TIM2_CH1_PA15_OUT_CFG4;
/// A default configuration to map TIM2 CH1 on PA15 pin (full remap/config 4 - input)
typedef TIM2_CH1_PA15<Mode::kInput, Speed::kInput>				TIM2_CH1_PA15_IN_CFG4;
/// A default configuration to map TIM2 ETR on PA15 pin (full remap/config 4 - input)
typedef TIM2_CH1_PA15<Mode::kInput, Speed::kInput>				TIM2_ETR_PA15_IN_CFG4;

/// A generic configuration to map TIM2 CH2 on PB3 pin (full remap/config 4)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM2_CH2_PB3 : AnyPin<GpioPortId::PB, 3, kMode, kSpeed, kPuPd, kLevel, AfTim2_PA15_PB3_10_11> {};
/// A default configuration to map TIM2 CH2 on PB3 pin (full remap/config 4 - output)
typedef TIM2_CH2_PB3<Mode::kAlternate, Speed::kFast>			TIM2_CH2_PB3_OUT_CFG4;
/// A default configuration to map TIM2 CH2 on PB3 pin (full remap/config 4 - input)
typedef TIM2_CH2_PB3<Mode::kInput, Speed::kInput>				TIM2_CH2_PB3_IN_CFG4;

/// A generic configuration to map TIM2 CH3 on PB10 pin (full remap/config 4)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM2_CH3_PB10 : AnyPin<GpioPortId::PB, 10, kMode, kSpeed, kPuPd, kLevel, AfTim2_PA15_PB3_10_11> {};
/// A default configuration to map TIM2 CH3 on PB10 pin (full remap/config 4 - output)
typedef TIM2_CH3_PB10<Mode::kAlternate, Speed::kFast>			TIM2_CH3_PB10_OUT_CFG4;
/// A default configuration to map TIM2 CH3 on PB10 pin (full remap/config 4 - input)
typedef TIM2_CH3_PB10<Mode::kInput, Speed::kInput>				TIM2_CH3_PB10_IN_CFG4;

/// A generic configuration to map TIM2 CH4 on PB11 pin (full remap/config 4)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM2_CH4_PB11 : AnyPin<GpioPortId::PB, 11, kMode, kSpeed, kPuPd, kLevel, AfTim2_PA15_PB3_10_11> {};
/// A default configuration to map TIM2 CH4 on PB11 pin (full remap/config 4 - output)
typedef TIM2_CH4_PB11<Mode::kAlternate, Speed::kFast>			TIM2_CH4_PB11_OUT_CFG4;
/// A default configuration to map TIM2 CH4 on PB11 pin (full remap/config 4 - input)
typedef TIM2_CH4_PB11<Mode::kInput, Speed::kInput>				TIM2_CH4_PB11_IN_CFG4;

//////////////////////////////////////////////////////////////////////
// TIM3 - Configuration 1
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map TIM3 CH1 on PA6 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM3_CH1_PA6 : AnyPin<GpioPortId::PA, 6, kMode, kSpeed, kPuPd, kLevel, AfTim3_PA6_7_PB0_1> {};
/// A default configuration to map TIM3 CH1 on PA6 pin (output)
typedef TIM3_CH1_PA6<Mode::kAlternate, Speed::kFast>			TIM3_CH1_PA6_OUT;
/// A default configuration to map TIM3 CH1 on PA6 pin (input)
typedef TIM3_CH1_PA6<Mode::kInput, Speed::kInput>				TIM3_CH1_PA6_IN;

/// A generic configuration to map TIM3 CH1 on PA7 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM3_CH2_PA7 : AnyPin<GpioPortId::PA, 7, kMode, kSpeed, kPuPd, kLevel, AfTim3_PA6_7_PB0_1> {};
/// A default configuration to map TIM3 CH2 on PA7 pin (output)
typedef TIM3_CH2_PA7<Mode::kAlternate, Speed::kFast>			TIM3_CH2_PA7_OUT;
/// A default configuration to map TIM3 CH2 on PA7 pin (input)
typedef TIM3_CH2_PA7<Mode::kInput, Speed::kInput>				TIM3_CH2_PA7_IN;

/// A generic configuration to map TIM3 CH3 on PB0 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM3_CH3_PB0 : AnyPin<GpioPortId::PB, 0, kMode, kSpeed, kPuPd, kLevel, AfTim3_PA6_7_PB0_1> {};
/// A default configuration to map TIM3 CH3 on PB0 pin (input)
typedef TIM3_CH3_PB0<Mode::kAlternate, Speed::kFast>			TIM3_CH3_PB0_OUT;
/// A default configuration to map TIM3 CH3 on PB0 pin (input)
typedef TIM3_CH3_PB0<Mode::kInput, Speed::kInput>				TIM3_CH3_PB0_IN;

/// A generic configuration to map TIM3 CH4 on PB1 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM3_CH4_PB1 : AnyPin<GpioPortId::PB, 1, kMode, kSpeed, kPuPd, kLevel, AfTim3_PA6_7_PB0_1> {};
/// A default configuration to map TIM3 CH4 on PB1 pin (input)
typedef TIM3_CH4_PB1<Mode::kAlternate, Speed::kFast>			TIM3_CH4_PB1_OUT;
/// A default configuration to map TIM3 CH4 on PB1 pin (input)
typedef TIM3_CH4_PB1<Mode::kInput, Speed::kInput>				TIM3_CH4_PB1_IN;

// TIM3 - Configuration 2 (partial remap)
/// A generic configuration to map TIM3 CH1 on PB4 pin (partial remap/config 2)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM3_CH1_PB4_P : AnyPin<GpioPortId::PB, 4, kMode, kSpeed, kPuPd, kLevel, AfTim3_PB4_5_0_1> {};
/// A default configuration to map TIM3 CH1 on PB4 pin (partial remap/config 2 - output)
typedef TIM3_CH1_PB4_P<Mode::kAlternate, Speed::kFast>			TIM3_CH1_PB4_OUT_CFG2;
/// A default configuration to map TIM3 CH1 on PB4 pin (partial remap/config 2 - input)
typedef TIM3_CH1_PB4_P<Mode::kInput, Speed::kInput>				TIM3_CH1_PB4_IN_CFG2;

/// A generic configuration to map TIM3 CH2 on PB5 pin (partial remap/config 2)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM3_CH2_PB5_P : AnyPin<GpioPortId::PB, 5, kMode, kSpeed, kPuPd, kLevel, AfTim3_PB4_5_0_1> {};
/// A default configuration to map TIM3 CH2 on PB5 pin (partial remap/config 2 - output)
typedef TIM3_CH2_PB5_P<Mode::kAlternate, Speed::kFast>			TIM3_CH2_PB5_OUT_CFG2;
/// A default configuration to map TIM3 CH2 on PB5 pin (partial remap/config 2 - input)
typedef TIM3_CH2_PB5_P<Mode::kInput, Speed::kInput>				TIM3_CH2_PB5_IN_CFG2;

/// A generic configuration to map TIM3 CH3 on PB0 pin (partial remap/config 2)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM3_CH3_PB0_P : AnyPin<GpioPortId::PB, 0, kMode, kSpeed, kPuPd, kLevel, AfTim3_PB4_5_0_1> {};
/// A default configuration to map TIM3 CH3 on PB0 pin (partial remap/config 2 - output)
typedef TIM3_CH3_PB0_P<Mode::kAlternate, Speed::kFast>			TIM3_CH3_PB0_OUT_CFG2;
/// A default configuration to map TIM3 CH3 on PB0 pin (partial remap/config 2 - input)
typedef TIM3_CH3_PB0_P<Mode::kInput, Speed::kInput>				TIM3_CH3_PB0_IN_CFG2;

/// A generic configuration to map TIM3 CH4 on PB1 pin (partial remap/config 2)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM3_CH4_PB1_P : AnyPin<GpioPortId::PB, 1, kMode, kSpeed, kPuPd, kLevel, AfTim3_PB4_5_0_1> {};
/// A default configuration to map TIM3 CH4 on PB1 pin (partial remap/config 2 - output)
typedef TIM3_CH4_PB1_P<Mode::kAlternate, Speed::kFast>			TIM3_CH4_PB1_OUT_CFG2;
/// A default configuration to map TIM3 CH4 on PB1 pin (partial remap/config 2 - input)
typedef TIM3_CH4_PB1_P<Mode::kInput, Speed::kInput>				TIM3_CH4_PB1_IN_CFG2;

//////////////////////////////////////////////////////////////////////
// TIM3 - Configuration 3 (full remap)
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map TIM3 CH1 on PC6 pin (full remap/config 3)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM3_CH1_PC6 : AnyPin<GpioPortId::PC, 6, kMode, kSpeed, kPuPd, kLevel, AfTim3_PC6_7_8_9> {};
/// A default configuration to map TIM3 CH1 on PC6 pin (full remap/config 3 - output)
typedef TIM3_CH1_PC6<Mode::kAlternate, Speed::kFast>			TIM3_CH1_PC6_OUT_CFG3;
/// A default configuration to map TIM3 CH1 on PC6 pin (full remap/config 3 - input)
typedef TIM3_CH1_PC6<Mode::kInput, Speed::kInput>				TIM3_CH1_PC6_IN_CFG3;

/// A generic configuration to map TIM3 CH2 on PC7 pin (full remap/config 3)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM3_CH2_PC7 : AnyPin<GpioPortId::PC, 7, kMode, kSpeed, kPuPd, kLevel, AfTim3_PC6_7_8_9> {};
/// A default configuration to map TIM3 CH2 on PC7 pin (full remap/config 3 - output)
typedef TIM3_CH2_PC7<Mode::kAlternate, Speed::kFast>			TIM3_CH2_PC7_OUT_CFG3;
/// A default configuration to map TIM3 CH2 on PC7 pin (full remap/config 3 - input)
typedef TIM3_CH2_PC7<Mode::kInput, Speed::kInput>				TIM3_CH2_PC7_IN_CFG3;

/// A generic configuration to map TIM3 CH3 on PC8 pin (full remap/config 3)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM3_CH3_PC8 : AnyPin<GpioPortId::PC, 8, kMode, kSpeed, kPuPd, kLevel, AfTim3_PC6_7_8_9> {};
/// A default configuration to map TIM3 CH3 on PC8 pin (full remap/config 3 - output)
typedef TIM3_CH3_PC8<Mode::kAlternate, Speed::kFast>			TIM3_CH3_PC8_OUT_CFG3;
/// A default configuration to map TIM3 CH3 on PC8 pin (full remap/config 3 - input)
typedef TIM3_CH3_PC8<Mode::kInput, Speed::kInput>				TIM3_CH3_PC8_IN_CFG3;

/// A generic configuration to map TIM3 CH4 on PC9 pin (full remap/config 3)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM3_CH4_PC9 : AnyPin<GpioPortId::PC, 9, kMode, kSpeed, kPuPd, kLevel, AfTim3_PC6_7_8_9> {};
/// A default configuration to map TIM3 CH4 on PC9 pin (full remap/config 3 - output)
typedef TIM3_CH4_PC9<Mode::kAlternate, Speed::kFast>			TIM3_CH4_PC9_OUT_CFG3;
/// A default configuration to map TIM3 CH4 on PC9 pin (full remap/config 3 - input)
typedef TIM3_CH4_PC9<Mode::kInput, Speed::kInput>				TIM3_CH4_PC9_IN_CFG3;

//////////////////////////////////////////////////////////////////////
// TIM4 - Configuration 1
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map TIM4 CH1 on PB6 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM4_CH1_PB6 : AnyPin<GpioPortId::PB, 6, kMode, kSpeed, kPuPd, kLevel, AfTim4_PB6_7_8_9> {};
/// A default configuration to map TIM4 CH1 on PB6 pin (output)
typedef TIM4_CH1_PB6<Mode::kAlternate, Speed::kFast>			TIM4_CH1_PB6_OUT;
/// A default configuration to map TIM4 CH1 on PB6 pin (input)
typedef TIM4_CH1_PB6<Mode::kInput, Speed::kInput>				TIM4_CH1_PB6_IN;

/// A generic configuration to map TIM4 CH2 on PB7 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM4_CH2_PB7 : AnyPin<GpioPortId::PB, 7, kMode, kSpeed, kPuPd, kLevel, AfTim4_PB6_7_8_9> {};
/// A default configuration to map TIM4 CH2 on PB7 pin (output)
typedef TIM4_CH2_PB7<Mode::kAlternate, Speed::kFast>			TIM4_CH2_PB7_OUT;
/// A default configuration to map TIM4 CH2 on PB7 pin (input)
typedef TIM4_CH2_PB7<Mode::kInput, Speed::kInput>				TIM4_CH2_PB7_IN;

/// A generic configuration to map TIM4 CH3 on PB8 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM4_CH3_PB8 : AnyPin<GpioPortId::PB, 8, kMode, kSpeed, kPuPd, kLevel, AfTim4_PB6_7_8_9> {};
/// A default configuration to map TIM4 CH3 on PB8 pin (output)
typedef TIM4_CH3_PB8<Mode::kAlternate, Speed::kFast>			TIM4_CH3_PB8_OUT;
/// A default configuration to map TIM4 CH3 on PB8 pin (input)
typedef TIM4_CH3_PB8<Mode::kInput, Speed::kInput>				TIM4_CH3_PB8_IN;

/// A generic configuration to map TIM4 CH4 on PB9 pin
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM4_CH4_PB9 : AnyPin<GpioPortId::PB, 9, kMode, kSpeed, kPuPd, kLevel, AfTim4_PB6_7_8_9> {};
/// A default configuration to map TIM4 CH4 on PB9 pin (output)
typedef TIM4_CH4_PB9<Mode::kAlternate, Speed::kFast>			TIM4_CH4_PB9_OUT;
/// A default configuration to map TIM4 CH4 on PB9 pin (input)
typedef TIM4_CH4_PB9<Mode::kInput, Speed::kInput>				TIM4_CH4_PB9_IN;

//////////////////////////////////////////////////////////////////////
// TIM4 - Configuration 2
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map TIM4 CH1 on PD12 pin (config 2)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM4_CH1_PD12 : AnyPin<GpioPortId::PD, 12, kMode, kSpeed, kPuPd, kLevel, AfTim4_PD12_13_14_15> {};
/// A default configuration to map TIM4 CH1 on PD12 pin (output)
typedef TIM4_CH1_PD12<Mode::kAlternate, Speed::kFast>			TIM4_CH1_PD12_OUT;
/// A default configuration to map TIM4 CH1 on PD12 pin (input)
typedef TIM4_CH1_PD12<Mode::kInput, Speed::kInput>				TIM4_CH1_PD12_IN;

/// A generic configuration to map TIM4 CH2 on PD13 pin (config 2)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM4_CH2_PD13 : AnyPin<GpioPortId::PD, 13, kMode, kSpeed, kPuPd, kLevel, AfTim4_PD12_13_14_15> {};
/// A default configuration to map TIM4 CH2 on PD13 pin (output)
typedef TIM4_CH2_PD13<Mode::kAlternate, Speed::kFast>			TIM4_CH2_PD13_OUT;
/// A default configuration to map TIM4 CH2 on PD13 pin (input)
typedef TIM4_CH2_PD13<Mode::kInput, Speed::kInput>				TIM4_CH2_PD13_IN;

/// A generic configuration to map TIM4 CH3 on PD14 pin (config 2)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM4_CH3_PD14 : AnyPin<GpioPortId::PD, 14, kMode, kSpeed, kPuPd, kLevel, AfTim4_PD12_13_14_15> {};
/// A default configuration to map TIM4 CH3 on PD14 pin (output)
typedef TIM4_CH3_PD14<Mode::kAlternate, Speed::kFast>			TIM4_CH3_PD14_OUT;
/// A default configuration to map TIM4 CH3 on PD14 pin (input)
typedef TIM4_CH3_PD14<Mode::kInput, Speed::kInput>				TIM4_CH3_PD14_IN;

/// A generic configuration to map TIM4 CH4 on PD15 pin (config 2)
template<const Mode kMode, const Speed kSpeed, const PuPd kPuPd = PuPd::kFloating, const Level kLevel = Level::kLow>
struct TIM4_CH4_PD15 : AnyPin<GpioPortId::PD, 15, kMode, kSpeed, kPuPd, kLevel, AfTim4_PD12_13_14_15> {};
/// A default configuration to map TIM4 CH4 on PD15 pin (output)
typedef TIM4_CH4_PD15<Mode::kAlternate, Speed::kFast>			TIM4_CH4_PD15_OUT;
/// A default configuration to map TIM4 CH4 on PD15 pin (input)
typedef TIM4_CH4_PD15<Mode::kInput, Speed::kInput>				TIM4_CH4_PD15_IN;


//////////////////////////////////////////////////////////////////////
// USART1 - Configuration 1
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map USART1 TX on PA9 pin
typedef AnyAlternateOutPin<GpioPortId::PA, 9, Mode::kAlternate, Speed::kFast, Level::kLow, AfUsart1_PA9_10>	USART1_TX_PA9;
/// A generic configuration to map USART1 RX on PA10 pin
typedef AnyInputPin<GpioPortId::PA, 10, PuPd::kPullUp, AfUsart1_PA9_10>									USART1_RX_PA10;

//////////////////////////////////////////////////////////////////////
// USART1 - Configuration 2
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map USART1 TX on PB6 pin (config 2)
typedef AnyAlternateOutPin<GpioPortId::PB, 6, Mode::kAlternate, Speed::kFast, Level::kLow, AfUsart1_PB6_7> USART1_TX_PB6;
/// A generic configuration to map USART1 RX on PB7 pin (config 2)
typedef AnyInputPin<GpioPortId::PB, 7, PuPd::kPullUp, AfUsart1_PB6_7>									USART1_RX_PB7;

//////////////////////////////////////////////////////////////////////
// USART1 - Configuration 1 & 2
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map USART1 CK on PA8 pin (config 1 & 2)
typedef AnyAlternateOutPin<GpioPortId::PA, 8, Mode::kAlternate, Speed::kFast, Level::kLow, AfNoRemap>	USART1_CK_PA8;
/// A generic configuration to map USART1 CTS on PA8 pin (config 1 & 2)
typedef AnyInputPin<GpioPortId::PA, 11, PuPd::kPullUp, AfNoRemap>										USART1_CTS_PA11;
/// A generic configuration to map USART1 RTS on PA8 pin (config 1 & 2)
typedef AnyAlternateOutPin<GpioPortId::PA, 12, Mode::kAlternate, Speed::kFast, Level::kLow, AfNoRemap>	USART1_RTS_PA12;

//////////////////////////////////////////////////////////////////////
// USART2 - Configuration 1
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map USART2 CTS on PA0 pin (config 1)
typedef AnyInputPin<GpioPortId::PA, 0, PuPd::kPullUp, AfUsart2_PA0_1_2_3_4>								USART2_CTS_PA0;
/// A generic configuration to map USART2 RTS on PA1 pin (config 1)
typedef AnyAlternateOutPin<GpioPortId::PA, 1, Mode::kAlternate, Speed::kFast, Level::kLow, AfUsart2_PA0_1_2_3_4> USART2_RTS_PA1;
/// A generic configuration to map USART2 TX on PA2 pin (config 1)
typedef AnyAlternateOutPin<GpioPortId::PA, 2, Mode::kAlternate, Speed::kFast, Level::kLow, AfUsart2_PA0_1_2_3_4> USART2_TX_PA2;
/// A generic configuration to map USART2 RX on PA3 pin (config 1)
typedef AnyInputPin<GpioPortId::PA, 3, PuPd::kPullUp, AfUsart2_PA0_1_2_3_4>								USART2_RX_PA3;
/// A generic configuration to map USART2 CK on PA4 pin (config 1)
typedef AnyAlternateOutPin<GpioPortId::PA, 4, Mode::kAlternate, Speed::kFast, Level::kLow, AfUsart2_PA0_1_2_3_4> USART2_CK_PA4;

//////////////////////////////////////////////////////////////////////
// USART2 - Configuration 2
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map USART2 CTS on PD3 pin (config 2)
typedef AnyInputPin<GpioPortId::PD, 3, PuPd::kPullUp, AfUsart2_PD3_4_5_6_7>								USART2_CTS_PD3;
/// A generic configuration to map USART2 RTS on PD4 pin (config 2)
typedef AnyAlternateOutPin<GpioPortId::PD, 4, Mode::kAlternate, Speed::kFast, Level::kLow, AfUsart2_PD3_4_5_6_7> USART2_RTS_PD4;
/// A generic configuration to map USART2 TX on PD5 pin (config 2)
typedef AnyAlternateOutPin<GpioPortId::PD, 5, Mode::kAlternate, Speed::kFast, Level::kLow, AfUsart2_PD3_4_5_6_7> USART2_TX_PD5;
/// A generic configuration to map USART2 RX on PD6 pin (config 2)
typedef AnyInputPin<GpioPortId::PD, 6, PuPd::kPullUp, AfUsart2_PD3_4_5_6_7>								USART2_RX_PD6;
/// A generic configuration to map USART2 CK on PD7 pin (config 2)
typedef AnyAlternateOutPin<GpioPortId::PD, 7, Mode::kAlternate, Speed::kFast, Level::kLow, AfUsart2_PD3_4_5_6_7> USART2_CK_PD7;

//////////////////////////////////////////////////////////////////////
// USART3 - Configuration 1
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map USART3 TX on PB10 pin (config 2)
typedef AnyAlternateOutPin<GpioPortId::PB, 10, Mode::kAlternate, Speed::kFast, Level::kLow, AfUsart3_PB10_11_12_13_14> USART3_TX_PB10;
/// A generic configuration to map USART3 RX on PB11 pin (config 2)
typedef AnyInputPin<GpioPortId::PB, 11, PuPd::kPullUp, AfUsart3_PB10_11_12_13_14> 						USART3_RX_PB11;
/// A generic configuration to map USART3 CK on PB12 pin (config 2)
typedef AnyAlternateOutPin<GpioPortId::PB, 12, Mode::kAlternate, Speed::kFast, Level::kLow, AfUsart3_PB10_11_12_13_14> 	USART3_CK_PB12;

//////////////////////////////////////////////////////////////////////
// USART3 - Configuration 1 & 2
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map USART3 CTS on PB13 pin (config 1 & 2)
typedef AnyInputPin<GpioPortId::PB, 13, PuPd::kPullUp, AfUsart3_PB10_11_12_13_14> 						USART3_CTS_PB13;
/// A generic configuration to map USART3 RTS on PB14 pin (config 1 & 2)
typedef AnyAlternateOutPin<GpioPortId::PB, 14, Mode::kAlternate, Speed::kFast, Level::kLow, AfUsart3_PB10_11_12_13_14> USART3_RTS_PB14;

//////////////////////////////////////////////////////////////////////
// USART3 - Configuration 2
//////////////////////////////////////////////////////////////////////
typedef AnyAlternateOutPin<GpioPortId::PC, 10, Mode::kAlternate, Speed::kFast, Level::kLow, AfUsart3_PC10_11_12_PB13_14> USART3_TX_PC10;
typedef AnyInputPin<GpioPortId::PC, 11, PuPd::kPullUp, AfUsart3_PC10_11_12_PB13_14> 					USART3_RX_PC11;
typedef AnyAlternateOutPin<GpioPortId::PC, 12, Mode::kAlternate, Speed::kFast, Level::kLow, AfUsart3_PC10_11_12_PB13_14> USART3_CK_PC12;

//////////////////////////////////////////////////////////////////////
// USART3 - Configuration 3
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map USART3 TX on PD8 pin (config 3)
typedef AnyAlternateOutPin<GpioPortId::PD, 8, Mode::kAlternate, Speed::kFast, Level::kLow, AfUsart3_PD8_9_10_11_12> USART3_TX_PD8;
/// A generic configuration to map USART3 RX on PD9 pin (config 3)
typedef AnyInputPin<GpioPortId::PD, 9, PuPd::kPullUp, AfUsart3_PD8_9_10_11_12> 							USART3_RX_PD9;
/// A generic configuration to map USART3 CK on PD10 pin (config 3)
typedef AnyAlternateOutPin<GpioPortId::PD, 10, Mode::kAlternate, Speed::kFast, Level::kLow, AfUsart3_PD8_9_10_11_12> USART3_CK_PD10;
/// A generic configuration to map USART3 CTS on PD11 pin (config 3)
typedef AnyInputPin<GpioPortId::PD, 11, PuPd::kPullUp, AfUsart3_PD8_9_10_11_12> 						USART3_CTS_PD11;
/// A generic configuration to map USART3 RTS on PD12 pin (config 3)
typedef AnyAlternateOutPin<GpioPortId::PD, 12, Mode::kAlternate, Speed::kFast, Level::kLow, AfUsart3_PD8_9_10_11_12> USART3_RTS_PD12;

//////////////////////////////////////////////////////////////////////
// USB
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map USB DM on PA11 pin
typedef AnyAlternateOutPin<GpioPortId::PA, 11, Mode::kAlternate, Speed::kFast, Level::kLow, AfNoRemap>	USBDM;
/// A generic configuration to map USB DP on PA12 pin
typedef AnyAlternateOutPin<GpioPortId::PA, 12, Mode::kAlternate, Speed::kFast, Level::kLow, AfNoRemap>	USBDP;

//////////////////////////////////////////////////////////////////////
// SWO
//////////////////////////////////////////////////////////////////////
/// A generic configuration to map TRACESWO on PB3 pin
typedef AnyAlternateOutPin<GpioPortId::PB, 3, Mode::kAlternate, Speed::kFast, Level::kLow, AfNoRemap>	TRACESWO;


}	// namespace Gpio
}	// namespace Bmt
