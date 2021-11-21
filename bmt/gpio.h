#pragma once

#include "mcu-system.h"
#include "pinremap.h"


enum GpioMode
{
	kInput = 0,
	kOutput10MHz = 1,
	kOutput2MHz = 2,
	kOutput50MHz = 3
};


enum GpioConf
{
	kAnalog = 0,
	kFloating = 1,
	kInputPushPull = 2,
	kPushPull = 0,
	kOpenDrain = 1,
	kAlternatePushPull = 2,
	kAlternateOpenDrain = 3
};


enum Level
{
	kLow = 0,
	kHigh = 1
};


template<
	const uint8_t kPin
	>
class PinUnused
{
public:
	static constexpr uint8_t kPin_ = kPin;
	static constexpr GpioMode kMode_ = kInput;
	static constexpr GpioConf kConf_ = kInputPushPull;
	static constexpr uint8_t kModeConf_ = (kInputPushPull << 2 | kInput);
	static constexpr uint32_t kModeConfLow_ = kPin < 8 ? kModeConf_ << (kPin << 2) : 0UL;
	static constexpr uint32_t kModeConfLowMask_ = ~(kPin < 8 ? 0x0FUL << (kPin << 2) : 0UL);
	static constexpr uint32_t kModeConfHigh_ = kPin >= 8 ? kModeConf_ << ((kPin - 8) << 2) : 0;
	static constexpr uint32_t kModeConfHighMask_ = ~(kPin >= 8 ? 0x0FUL << ((kPin - 8) << 2) : 0UL);
	static constexpr uint16_t kBitValue_ = 0;
	static constexpr uint32_t kInitialLevel_ = kLow;
	static constexpr uint32_t kAfConf_ = 0x00000000U;
	static constexpr uint32_t kAfMask_ = 0xFFFFFFFFU;
	static constexpr bool kAfDisabled_ = true;
	static constexpr bool kIsUnused_ = true;

	ALWAYS_INLINE static constexpr bool IsHigh(void) { return false; }
	ALWAYS_INLINE static constexpr bool IsLow(void) { return true; }
	ALWAYS_INLINE static void Toggle(void) { }
	ALWAYS_INLINE static void SetHigh(void) { }
	ALWAYS_INLINE static void SetLow(void) { }
};


template<
	const uint8_t kPin
>
class PinUnchanged
{
public:
	static constexpr uint8_t kPin_ = kPin;
	static constexpr uint8_t kModeConf_ = 0UL;
	static constexpr uint32_t kModeConfLow_ = 0UL;
	static constexpr uint32_t kModeConfLowMask_ = ~(0UL);
	static constexpr uint32_t kModeConfHigh_ = 0UL;
	static constexpr uint32_t kModeConfHighMask_ = ~(0UL);
	static constexpr uint16_t kBitValue_ = 0;
	static constexpr uint32_t kInitialLevel_ = kLow;
	static constexpr uint32_t kAfConf_ = 0x00000000U;
	static constexpr uint32_t kAfMask_ = 0xFFFFFFFFU;
	static constexpr bool kAfDisabled_ = true;
	static constexpr bool kIsUnused_ = true;

	ALWAYS_INLINE static constexpr bool IsHigh(void) { return false; }
	ALWAYS_INLINE static constexpr bool IsLow(void) { return true; }
	ALWAYS_INLINE static void Toggle(void) {}
	ALWAYS_INLINE static void SetHigh(void) {}
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
	const GpioPortId kPort
	, const uint8_t kPin
	, const GpioMode kMode = kInput
	, const GpioConf kConf = kFloating
	, const Level kInitialLevel = kLow
	, typename Map = AfNoRemap
	>
class GpioTemplate
{
public:
	typedef Map MapType;
	static constexpr GpioPortId kPort_ = kPort;
	static constexpr uint32_t kPortBase_ = (GPIOA_BASE + kPort_ * 0x400);
	static constexpr uint8_t kPin_ = kPin;
	static constexpr GpioMode kMode_ = kMode;
	static constexpr GpioConf kConf_ = kConf;
	static constexpr uint8_t kModeConf_ = (kConf << 2 | kMode);
	static constexpr uint32_t kModeConfLow_ = kPin < 8 ? kModeConf_ << (kPin << 2) : 0UL;
	static constexpr uint32_t kModeConfLowMask_ = ~(kPin < 8 ? 0x0FUL << (kPin << 2) : 0UL);
	static constexpr uint32_t kModeConfHigh_ = kPin >= 8 ? kModeConf_ << ((kPin - 8) << 2) : 0UL;
	static constexpr uint32_t kModeConfHighMask_ = ~(kPin >= 8 ? 0x0FUL << ((kPin - 8) << 2) : 0UL);
	static constexpr uint16_t kBitValue_ = 1 << kPin;
	static constexpr uint32_t kInitialLevel_ = kInitialLevel << kPin;
	static constexpr uint32_t kAfConf_ = Map::kConf;
	static constexpr uint32_t kAfMask_ = Map::kMask;
	static constexpr bool kAfDisabled_ = Map::kNoRemap;
	static constexpr bool kIsUnused_ = false;

	//! Access to the peripheral memory space
	ALWAYS_INLINE static volatile GPIO_TypeDef *GetPortBase() { return (volatile GPIO_TypeDef *)kPortBase_; }

	//! Sets pin up. The pin will be high as long as it is configured as GPIO output
	ALWAYS_INLINE static void SetHigh(void)
	{
		volatile GPIO_TypeDef *port = (volatile GPIO_TypeDef *)kPortBase_;
		port->BSRR = kBitValue_;
	};

	//! Sets pin down. The pin will be low as long as it is configured as GPIO output
	ALWAYS_INLINE static void SetLow(void)
	{
		volatile GPIO_TypeDef *port = (volatile GPIO_TypeDef *)kPortBase_;
		port->BRR = kBitValue_;
	}

	//! Sets the pin to the given level. Note that optimizing compiler simplifies literal constants
	ALWAYS_INLINE static void Set(bool value)
	{
		if (value)
			SetHigh();
		else
			SetLow();
	}

	//! Reads current Pin electrical state
	ALWAYS_INLINE static bool Get(void)
	{
		volatile GPIO_TypeDef *port = (volatile GPIO_TypeDef *)kPortBase_;
		return (port->IDR & kBitValue_) != 0;
	}

	//! Checks if current pin electrical state is high
	ALWAYS_INLINE static bool IsHigh(void)
	{
		volatile GPIO_TypeDef *port = (volatile GPIO_TypeDef *)kPortBase_;
		return (port->IDR & kBitValue_) != 0;
	}

	//! Checks if current pin electrical state is low
	ALWAYS_INLINE static bool IsLow(void)
	{
		return !IsHigh();
	}

	//! Toggles pin state
	ALWAYS_INLINE static void Toggle(void)
	{
		volatile GPIO_TypeDef *port = (volatile GPIO_TypeDef *)kPortBase_;
		port->ODR ^= kBitValue_;
	}
	//! Apply default configuration for the pin.
	ALWAYS_INLINE static void SetupPinMode(void)
	{
		volatile GPIO_TypeDef *port = (volatile GPIO_TypeDef *)kPortBase_;
		if (kPin < 8)
			port->CRL = (port->CRL & kModeConfLowMask_) | kModeConfLow_;
		else
			port->CRH = (port->CRH & kModeConfHighMask_) | kModeConfHigh_;
	}
	//! Apply default configuration for the pin.
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
	//! Apply a special configuration to the pin, after initialization
	ALWAYS_INLINE static void Setup(GpioMode mode, GpioConf conf)
	{
		volatile GPIO_TypeDef *port = (volatile GPIO_TypeDef *)kPortBase_;
		const uint32_t mode_conf = (conf << 2 | mode);
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


template <
	const GpioPortId kPort
	, const uint8_t kPin
>
class FloatingPin : public GpioTemplate<kPort, kPin, kInput, kFloating>
{
};


template <
	const GpioPortId kPort
	, const uint8_t kPin
>
class InputPullUpPin : public GpioTemplate<kPort, kPin, kInput, kInputPushPull, kHigh>
{
};


template <
	const GpioPortId kPort
	, const uint8_t kPin
>
class InputPullDownPin : public GpioTemplate<kPort, kPin, kInput, kInputPushPull, kLow>
{
};


template <
	const GpioPortId kPort
	, typename Pin0 = PinUnused<0>
	, typename Pin1 = PinUnused<1>
	, typename Pin2 = PinUnused<2>
	, typename Pin3 = PinUnused<3>
	, typename Pin4 = PinUnused<4>
	, typename Pin5 = PinUnused<5>
	, typename Pin6 = PinUnused<6>
	, typename Pin7 = PinUnused<7>
	, typename Pin8 = PinUnused<8>
	, typename Pin9 = PinUnused<9>
	, typename Pin10 = PinUnused<10>
	, typename Pin11 = PinUnused<11>
	, typename Pin12 = PinUnused<12>
	, typename Pin13 = PinUnused<13>
	, typename Pin14 = PinUnused<14>
	, typename Pin15 = PinUnused<15>
	>
class GpioPortTemplate
{
public:
	static constexpr GpioPortId kPort_ = kPort;
	static constexpr uint32_t kPortBase_ = (GPIOA_BASE + kPort_ * 0x400);
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

	//! Initialize to Port assuming the first use of all GPIO pins
	ALWAYS_INLINE static void Init(void)
	{
		// Pin numbering does not match template parameter position
		static_assert(
			Pin0::kPin_ == 0 && Pin1::kPin_ == 1 && Pin2::kPin_ == 2 && Pin3::kPin_ == 3
			&& Pin4::kPin_ == 4 && Pin5::kPin_ == 5 && Pin6::kPin_ == 6 && Pin7::kPin_ == 7
			&& Pin8::kPin_ == 8 && Pin9::kPin_ == 9 && Pin10::kPin_ == 10 && Pin11::kPin_ == 11
			&& Pin12::kPin_ == 12 && Pin13::kPin_ == 13 && Pin14::kPin_ == 14 && Pin15::kPin_ == 15
			, "Inconsistent pin position"
			);

		volatile GPIO_TypeDef *port = (volatile GPIO_TypeDef *)kPortBase_;
		if(kAfDisabled_)
			RCC->APB2ENR |= (1 << (kPort_ + RCC_APB2ENR_IOPAEN_Pos));
		else
			RCC->APB2ENR |= (1 << (kPort_ + RCC_APB2ENR_IOPAEN_Pos)) | RCC_APB2ENR_AFIOEN;
		port->CRL = kCrl_;
		port->CRH = kCrh_;
		port->ODR = kOdr_;
		AfRemapTemplate<kAfConf_, kAfMask_>::Enable();
	}
	//! Apply state of pin group merging with previous GPI contents
	ALWAYS_INLINE static void Enable(void)
	{
		volatile GPIO_TypeDef* port = (GPIO_TypeDef*)kPortBase_;
		port->CRL = (port->CRL & kCrlMask_) | kCrl_;
		port->CRH = (port->CRH & kCrhMask_) | kCrh_;
		port->ODR = (port->ODR & ~kBitValue_) | kOdr_;
		AfRemapTemplate<kAfConf_, kAfMask_>::Enable();
	}
	//! Not an ideal approach, but float everything
	ALWAYS_INLINE static void Disable(void)
	{
		volatile GPIO_TypeDef *port = (volatile GPIO_TypeDef *)kPortBase_;
		RCC->APB2ENR |= (1 << (kPort_ + RCC_APB2ENR_IOPAEN_Pos));
		port->CRL = 0x44444444;
		port->CRH = 0x44444444;
		AFIO->MAPR &= kAfMask_;
		RCC->APB2ENR &= ~(1 << (kPort_ + RCC_APB2ENR_IOPAEN_Pos));
	}
};

template<const GpioPortId kPort>
class SaveGpio
{
public:
	static constexpr GpioPortId kPort_ = kPort;
	static constexpr uint32_t kPortBase_ = (GPIOA_BASE + kPort_ * 0x400);
	SaveGpio()
	{
		volatile GPIO_TypeDef *port = (volatile GPIO_TypeDef *)kPortBase_;
		odr_ = port->ODR;
		crl_ = port->CRL;
		crh_ = port->CRH;
		mapr_ = AFIO->MAPR;
	}
	~SaveGpio()
	{
		volatile GPIO_TypeDef *port = (volatile GPIO_TypeDef *)kPortBase_;
		port->ODR = odr_;
		port->CRL = crl_;
		port->CRH = crh_;
		AFIO->MAPR = mapr_;
	}

protected:
	uint32_t odr_;
	uint32_t crl_;
	uint32_t crh_;
	uint32_t mapr_;
};


// ADC12
typedef GpioTemplate<PA, 0, kInput, kAnalog> ADC12_IN0;
typedef GpioTemplate<PA, 1, kInput, kAnalog> ADC12_IN1;
typedef GpioTemplate<PA, 2, kInput, kAnalog> ADC12_IN2;
typedef GpioTemplate<PA, 3, kInput, kAnalog> ADC12_IN3;
typedef GpioTemplate<PA, 4, kInput, kAnalog> ADC12_IN4;
typedef GpioTemplate<PA, 5, kInput, kAnalog> ADC12_IN5;
typedef GpioTemplate<PA, 6, kInput, kAnalog> ADC12_IN6;
typedef GpioTemplate<PA, 7, kInput, kAnalog> ADC12_IN7;
typedef GpioTemplate<PB, 0, kInput, kAnalog> ADC12_IN8;
typedef GpioTemplate<PB, 1, kInput, kAnalog> ADC12_IN9;
typedef GpioTemplate<PC, 0, kInput, kAnalog> ADC12_IN10;
typedef GpioTemplate<PC, 1, kInput, kAnalog> ADC12_IN11;
typedef GpioTemplate<PC, 2, kInput, kAnalog> ADC12_IN12;
typedef GpioTemplate<PC, 3, kInput, kAnalog> ADC12_IN13;
typedef GpioTemplate<PC, 4, kInput, kAnalog> ADC12_IN14;
typedef GpioTemplate<PC, 5, kInput, kAnalog> ADC12_IN15;

// CAN - Configuration 1
typedef GpioTemplate<PA, 11, kInput, kInputPushPull, kHigh, AfCan_PA11_12>				CAN_RX_PA11;
typedef GpioTemplate<PA, 12, kOutput50MHz, kAlternateOpenDrain, kLow, AfCan_PA11_12>	CAN_TX_PA12;
// CAN - Configuration 2
typedef GpioTemplate<PB, 8, kInput, kInputPushPull, kHigh, AfCan_PB8_9>				CAN_RX_PB8;
typedef GpioTemplate<PB, 9, kOutput50MHz, kAlternateOpenDrain, kLow, AfCan_PB8_9>	CAN_TX_PB9;
// CAN - Configuration 3
typedef GpioTemplate<PD, 0, kInput, kInputPushPull, kHigh, AfCan_PD0_1>				CAN_RX_PD0;
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
	struct TIM2_CH3_PA2_P : GpioTemplate<PA, 2, kMode, kConf, LVL, AfTim2_PA0_1_2_3> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM2_CH4_PA3_P : GpioTemplate<PA, 3, kMode, kConf, LVL, AfTim2_PA0_1_2_3> {};
// TIM2 - Configuration 3 (partial remap)
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM2_CH1_PA0_P : GpioTemplate<PA, 0, kMode, kConf, LVL, AfTim2_PA0_1_PB10_11> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM2_CH2_PA1_P : GpioTemplate<PA, 1, kMode, kConf, LVL, AfTim2_PA0_1_PB10_11> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM2_CH3_PB10_P : GpioTemplate<PB, 10, kMode, kConf, LVL, AfTim2_PA0_1_PB10_11> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM2_CH4_PB11_P : GpioTemplate<PB, 11, kMode, kConf, LVL, AfTim2_PA0_1_PB10_11> {};
// TIM2 - Configuration 4 (full remap)
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM2_CH1_PA15 : GpioTemplate<PA, 15, kMode, kConf, LVL, AfTim2_PA15_PB3_10_11> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM2_CH2_PB3 : GpioTemplate<PB, 3, kMode, kConf, LVL, AfTim2_PA15_PB3_10_11> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM2_CH3_PB10 : GpioTemplate<PB, 10, kMode, kConf, LVL, AfTim2_PA15_PB3_10_11> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM2_CH4_PB11 : GpioTemplate<PB, 11, kMode, kConf, LVL, AfTim2_PA15_PB3_10_11> {};

// TIM3 - Configuration 1
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM3_CH1_PA6 : GpioTemplate<PA, 6, kMode, kConf, LVL, AfTim3_PA6_7_PB0_1> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM3_CH2_PA7 : GpioTemplate<PA, 7, kMode, kConf, LVL, AfTim3_PA6_7_PB0_1> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM3_CH3_PB0 : GpioTemplate<PB, 0, kMode, kConf, LVL, AfTim3_PA6_7_PB0_1> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM3_CH4_PB1 : GpioTemplate<PB, 1, kMode, kConf, LVL, AfTim3_PA6_7_PB0_1> {};
// TIM3 - Configuration 2 (partial remap)
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM3_CH1_PB4_P : GpioTemplate<PB, 4, kMode, kConf, LVL, AfTim3_PB4_PB5_0_1> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM3_CH2_PB5_P : GpioTemplate<PB, 5, kMode, kConf, LVL, AfTim3_PB4_PB5_0_1> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM3_CH3_PB0_P : GpioTemplate<PB, 0, kMode, kConf, LVL, AfTim3_PB4_PB5_0_1> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM3_CH4_PB1_P : GpioTemplate<PB, 1, kMode, kConf, LVL, AfTim3_PB4_PB5_0_1> {};
// TIM3 - Configuration 3 (full remap)
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM3_CH1_PC6 : GpioTemplate<PC, 6, kMode, kConf, LVL, AfTim3_PC6_7_8_9> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM3_CH2_PC7 : GpioTemplate<PC, 7, kMode, kConf, LVL, AfTim3_PC6_7_8_9> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM3_CH3_PC8 : GpioTemplate<PC, 8, kMode, kConf, LVL, AfTim3_PC6_7_8_9> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM3_CH4_PC9 : GpioTemplate<PC, 9, kMode, kConf, LVL, AfTim3_PC6_7_8_9> {};

// TIM4 - Configuration 1
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM4_CH1_PB6 : GpioTemplate<PB, 6, kMode, kConf, LVL, AfTim4_PB6_7_8_9> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM4_CH2_PB7 : GpioTemplate<PB, 7, kMode, kConf, LVL, AfTim4_PB6_7_8_9> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM4_CH3_PB8 : GpioTemplate<PB, 8, kMode, kConf, LVL, AfTim4_PB6_7_8_9> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM4_CH4_PB9 : GpioTemplate<PB, 9, kMode, kConf, LVL, AfTim4_PB6_7_8_9> {};
// TIM4 - Configuration 2
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM4_CH1_PD12 : GpioTemplate<PD, 12, kMode, kConf, LVL, AfTim4_PD12_13_14_15> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM4_CH2_PD13 : GpioTemplate<PD, 13, kMode, kConf, LVL, AfTim4_PD12_13_14_15> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM4_CH3_PD14 : GpioTemplate<PD, 14, kMode, kConf, LVL, AfTim4_PD12_13_14_15> {};
template<const GpioMode kMode, const GpioConf kConf, const Level LVL = kLow>
	struct TIM4_CH4_PD15 : GpioTemplate<PD, 15, kMode, kConf, LVL, AfTim4_PD12_13_14_15> {};

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

