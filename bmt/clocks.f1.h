#pragma once

#include "pinremap.h"

namespace Bmt
{
namespace Clocks
{

/// Clock source selection
enum Id
{
	kHSI,	///< HSI (high speed internal) 8 MHz RC oscillator clock
	kHSE,	///< HSE oscillator clock, from 4 to 48 MHz
	kPLL,	///< PLL clock source
	kLSI,	///< 32 kHz low speed internal RC
	kLSE,	///< 32.768 kHz low speed external crystal
};


/// Default HSI trim value
static constexpr uint8_t kHsiDefaultTrim = 0b00010000;


/// Class for the HSI clock source
template<
	const uint8_t kTrim = kHsiDefaultTrim
>
class Hsi
{
public:
	/// Clock identification
	static constexpr Id kClockSource_ = kHSI;
	/// Frequency of the clock source
	static constexpr uint32_t kFrequency_ = 8000000UL;
	/// Oscillator that generates the clock (not switchable in this particular case)
	static constexpr Id kClockInput_ = kClockSource_;
	/// Starts HSI oscillator
	ALWAYS_INLINE static void Init(void)
	{
		Enable();
		Trim(kTrim);
	}
	/// Enables the HSI oscillator
	ALWAYS_INLINE static void Enable(void)
	{
		RCC->CR |= RCC_CR_HSION;
		while (!(RCC->CR & RCC_CR_HSIRDY));
	}
	/// Disables the HSI oscillator. You must ensure that associated peripherals are mapped elsewhere
	ALWAYS_INLINE static void Disable(void)
	{
		RCC->CR &= ~RCC_CR_HSION;
	}
	/// Reads factory default calibration
	ALWAYS_INLINE static uint8_t GetCal()
	{
		return uint8_t((RCC->CR & RCC_CR_HSICAL_Msk) >> RCC_CR_HSICAL_Pos);
	}
	/// Reads current trim value
	ALWAYS_INLINE static uint8_t Trim()
	{
		return (RCC->CR & ~RCC_CR_HSITRIM_Msk) >> RCC_CR_HSITRIM_Pos;
	}
	/// Sets a new trim value
	ALWAYS_INLINE static void Trim(const uint8_t v)
	{
		RCC->CR = (RCC->CR & ~RCC_CR_HSITRIM_Msk) | ((v << RCC_CR_HSITRIM_Pos) & RCC_CR_HSITRIM_Msk);
	}
};


/// Template class for the HSE clock source
template<
	const uint32_t kFrequency = 8000000UL	///< Frequency of oscillator
	, bool kBypass = false					///< HSE oscillator bypassed with external clock
	, const bool kCssEnabled = false		///< Clock Security System enable
	>
class AnyHse
{
public:
	/// A constant for the Clock identification
	static constexpr Id kClockSource_ = kHSE;
	/// A constant for the frequency
	static constexpr uint32_t kFrequency_ = kFrequency;
	/// Actual oscillator that generates the clock
	static constexpr Id kClockInput_ = kClockSource_;

	/// Starts HSE oscillator
	ALWAYS_INLINE static void Init(void)
	{
		Enable();
	}
	/// Enables the HSE oscillator
	ALWAYS_INLINE static void Enable(void)
	{
		Af_PD01_OSC::Enable();
		uint32_t tmp = RCC_CR_HSEON;
		if(kBypass)
			tmp |= RCC_CR_HSEBYP;
		if(kCssEnabled)
			tmp |= RCC_CR_CSSON;
		// Apply
		RCC->CR = (RCC->CR 
			& ~(
				RCC_CR_CSSON_Msk
				| RCC_CR_HSEBYP_Msk
				| RCC_CR_HSEON_Msk
			)) | tmp;
		// Wait to settle
		while (!(RCC->CR & RCC_CR_HSERDY));
	}
	/// Disables the HSE oscillator. You must ensure that associated peripherals are mapped elsewhere
	ALWAYS_INLINE static void Disable(void)
	{
		uint32_t tmp = ~RCC_CR_HSEON;
		if(kCssEnabled)
			tmp &= ~RCC_CR_CSSON;
		RCC->CR &= tmp;
	}
};


/// Template class for the LSE oscillator
template<
	const uint32_t kFrequency = 32768	///< Frequency for the LSE clock
	, const bool kBypass = false		///< LSE oscillator bypassed with external clock
	>
class AnyLse
{
public:
	/// A constant for the Clock identification
	static constexpr Id kClockSource_ = kLSE;
	/// A constant for the frequency
	static constexpr uint32_t kFrequency_ = kFrequency;
	/// Actual oscillator that generates the clock
	static constexpr Id kClockInput_ = kClockSource_;
	/// Starts LSE oscillator within a backup domain transaction
	ALWAYS_INLINE static void Init(BkpDomainXact &xact)
	{
		Enable(xact);
	}
	/// Enables the LSE oscillator within a backup domain transaction
	ALWAYS_INLINE static void Enable(BkpDomainXact &)
	{
		uint32_t tmp = RCC_BDCR_LSEON;
		if (kBypass)
			tmp |= RCC_BDCR_LSEBYP;
		RCC->BDCR = (RCC->BDCR 
			& ~(RCC_BDCR_LSEON_Msk
				| RCC_BDCR_LSEBYP_Msk)
			) | tmp;
		while (!(RCC->BDCR & RCC_BDCR_LSERDY));
	}
	/// Disables the LSE oscillator. You must ensure that associated peripherals are mapped elsewhere
	ALWAYS_INLINE static void Disable(void)
	{
		RCC->BDCR &= ~RCC_BDCR_LSEON;
	}
};


/// Class for the LSI oscillator
class Lsi
{
public:
	/// A constant for the Clock identification
	static constexpr Id kClockSource_ = kLSI;
	/// A constant for the frequency
	static constexpr uint32_t kFrequency_ = 40000;
	/// Actual oscillator that generates the clock (40 kHz)
	static constexpr Id kClockInput_ = kClockSource_;
	/// Starts LSI oscillator
	ALWAYS_INLINE static void Init(void)
	{
		Enable();
	}
	/// Enables the LSI oscillator
	ALWAYS_INLINE static void Enable(void)
	{
		RCC->CSR |= RCC_CSR_LSION;
		while(!(RCC->CSR & RCC_CSR_LSIRDY)); 
	}
	/// Disables the LSI oscillator. You must ensure that associated peripherals are mapped elsewhere
	ALWAYS_INLINE static void Disable(void)
	{
		RCC->CSR &= ~RCC_CSR_LSION;
	}
};


/// Template class for the PLL Oscillator
template<
	typename ClockSource				///< PLL is linked to another clock source
	, const uint32_t kFrequency			///< The output frequency of the PLL
>
class AnyPll
{
public:
	/// A constant for the Clock identification
	static constexpr Id kClockSource_ = kPLL;
	/// A constant for the frequency
	static constexpr uint32_t kFrequency_ = kFrequency;
	/// Actual oscillator that generates the clock (the linked clock source)
	static constexpr Id kClockInput_ = ClockSource::kClockSource_;

	/// Computes the multiplier (a constant fixed value, according to inout constants)
	ALWAYS_INLINE static constexpr uint32_t Multiplier(const uint32_t source_frequency, const uint32_t kFrequency_)
	{
		return (kFrequency_ / source_frequency - 2) << 18;
	}

	/// Starts associated oscillator and then the PLL
	ALWAYS_INLINE static void Init(void)
	{
		ClockSource::Init();
		Enable();
	}

	/// Enables the PLL oscillator, assuming associated source was already started
	ALWAYS_INLINE static void Enable(void)
	{
		// Work on register
		uint32_t tmp;
		// This complex logic is statically simplified by optimizing compiler
		// because we use constants
		if(ClockSource::kClockSource_ == kHSE)
		{
			if(kFrequency_ <= (9 * ClockSource::kFrequency_ / 2))			// PREDIV = /2
			{
				tmp = (uint32_t)(RCC_CFGR_PLLSRC 
					| RCC_CFGR_PLLXTPRE_HSE_DIV2
					| Multiplier(ClockSource::kFrequency_ /2, kFrequency_));
			}
			else if(kFrequency_ == (13 * ClockSource::kFrequency_ / 2))	// factor 6.5
			{
				tmp = (uint32_t)(RCC_CFGR_PLLSRC 
					| (0x0D << RCC_CFGR_PLLMULL_Pos));
			}
			else if(kFrequency_ == (13 * ClockSource::kFrequency_ / 4))	// factor 6.5 with PREDIV = /2
			{
				tmp = (uint32_t)(RCC_CFGR_PLLSRC 
					| RCC_CFGR_PLLXTPRE_HSE_DIV2
					| (0x0D << RCC_CFGR_PLLMULL_Pos));
			}
			else
			{
				tmp = (uint32_t)(RCC_CFGR_PLLSRC 
					| Multiplier(ClockSource::kFrequency_, kFrequency_));
			}
		}
		else
		{
			tmp = (uint32_t)(Multiplier(ClockSource::kFrequency_ /2, kFrequency_));
		}
		// Combine bits and apply
		RCC->CFGR = tmp | (RCC->CFGR 
			& ~(
				RCC_CFGR_PLLSRC_Msk 
				| RCC_CFGR_PLLXTPRE_Msk 
				| RCC_CFGR_PLLMULL_Msk
			));
		// Enable PLL 
		RCC->CR |= RCC_CR_PLLON;
		// Wait to settle
		while(!(RCC->CR & RCC_CR_PLLRDY)); 
	}
	/// Disables the PLL oscillator. You must ensure that associated peripherals are mapped elsewhere
	ALWAYS_INLINE static void Disable(void)
	{
		RCC->CR &= ~RCC_CR_PLLON;
	}
};


/// These are the possible values to source the MCO output with clock
enum class Mco : uint32_t
{
	kMcoOff = RCC_CFGR_MCO_NOCLOCK,				///< No Clock
	kMcoSysClk = RCC_CFGR_MCO_SYSCLK,			///< System clock (SYSCLK) selected
	kMcoHsi = RCC_CFGR_MCO_HSI,					///< HSI clock selected
	kMcoHse = RCC_CFGR_MCO_HSE,					///< HSE clock selected
	kMcoPllClkDiv2 = RCC_CFGR_MCO_PLLCLK_DIV2	///< PLL clock divided by 2 selected
};

/// AHB clock Prescaler
enum class AhbPrscl : uint16_t
{
	k1 = 1,			///< No clock prescaler
	k2 = 2,			///< Divide clock by 2
	k4 = 4,			///< Divide clock by 4
	k8 = 8,			///< Divide clock by 8
	k16 = 16,		///< Divide clock by 16
	k32 = 32,		///< Divide clock by 32
	k64 = 64,		///< Divide clock by 64
	k128 = 128,		///< Divide clock by 128
	k256 = 256,		///< Divide clock by 256
	k512 = 512,		///< Divide clock by 512
};

/// APB1/2 clock Prescaler
enum class ApbPrscl : uint8_t
{
	k1 = 1,			///< No clock prescaler
	k2 = 2,			///< Divide clock by 2
	k4 = 4,			///< Divide clock by 4
	k8 = 8,			///< Divide clock by 8
	k16 = 16,		///< Divide clock by 16
};

/// ADC clock Prescaler
enum class AdcPrscl : uint8_t
{
	k2 = 2,			///< Divide clock by 2
	k4 = 4,			///< Divide clock by 4
	k6 = 6,			///< Divide clock by 6
	k8 = 8,			///< Divide clock by 8
};

/*!
A class to setup System Clock. Please check the clock tree @RM0008 (r21-Fig.8).
STM32F10x allows System Clocks sourced from HSI, HSE or PLL only.
*/
template<
	typename ClockSource = Hsi<>				///< New clock source for System
	, const AhbPrscl kAhbPrs = AhbPrscl::k1		///< AHB bus prescaler
	, const ApbPrscl kApb1Prs = ApbPrscl::k2	///< APB1 bus prescaler
	, const ApbPrscl kApb2Prs = ApbPrscl::k1	///< APB2 bus prescaler
	, const AdcPrscl kAdcPrs = AdcPrscl::k8		///< ADC prescaler factor
	, const bool kHsiRcOff = true				///< Init() disables HSI, if not current clock source
	, const Mco kClockOut = Mco::kMcoOff		///< Turn MCU clock output on (it does not enable external pin)
	>
class AnySycClk
{
public:
	/// The system clock frequency (a constant)
	static constexpr uint32_t kFrequency_ = ClockSource::kFrequency_;
	/// Effective AHB clock frequency (a constant)
	static constexpr uint32_t kAhbClock_ = kFrequency_ / uint32_t(kAhbPrs);
	/// Effective APB1 clock frequency (a constant)
	static constexpr uint32_t kApb1Clock_ = kAhbClock_ / uint32_t(kApb1Prs);
	/// Effective clock for timer connected to APB1
	static constexpr uint32_t kApb1TimerClock_ = (kApb1Prs == ApbPrscl::k1) ? kApb1Clock_ : 2 * kApb1Clock_;
	/// Effective APB2 clock frequency (a constant)
	static constexpr uint32_t kApb2Clock_ = kAhbClock_ / uint32_t(kApb2Prs);
	/// Effective clock for timer connected to APB2
	static constexpr uint32_t kApb2TimerClock_ = (kApb1Prs == ApbPrscl::k1) ? kApb2Clock_ : 2 * kApb2Clock_;
	/// Effective ADC clock
	static constexpr uint32_t kAdc_ = kAhbClock_ / uint32_t(kAdcPrs);
	/// Clock output mode
	static constexpr Mco kMco_ = kClockOut;

	/// Starts associated oscillator, initializes clock tree prescalers and use oscillator for system clock
	ALWAYS_INLINE static void Init(void)
	{
		// Initialization clocks in chain for code simplicity
		ClockSource::Init();
		// Enables itself
		Enable();
		// Disabling HSI if setting up a System clock source
		if (
			kHsiRcOff 
			&& (ClockSource::kClockSource_ == kHSE
				|| (ClockSource::kClockSource_ == kPLL
					&& ClockSource::kClockInput_ != kHSI)
			)
		)
		{
			// Note that flash controller needs this clock for programming!!!
			Hsi<>::Disable();
		}
	}

	/// Initializes clock tree prescalers, assuming associated source was already started
	ALWAYS_INLINE static void Enable(void)
	{
		// System clock restricts sources
		static_assert(
			ClockSource::kClockSource_ == kHSI
			|| ClockSource::kClockSource_ == kHSE
			|| ClockSource::kClockSource_ == kPLL
			, "Allowed System Clock source are HSI, HSE or PLL."
		);
		// Invalid AHB prescaler
		static_assert(
			kAhbPrs == AhbPrscl::k1
			|| kAhbPrs == AhbPrscl::k2
			|| kAhbPrs == AhbPrscl::k4
			|| kAhbPrs == AhbPrscl::k8
			|| kAhbPrs == AhbPrscl::k16
			|| kAhbPrs == AhbPrscl::k64
			|| kAhbPrs == AhbPrscl::k128
			|| kAhbPrs == AhbPrscl::k256
			|| kAhbPrs == AhbPrscl::k512
			, "AHB prescaler parameter is invalid."
		);
		// Invalid APB1 prescaler
		static_assert(
			kApb1Prs == ApbPrscl::k1
			|| kApb1Prs == ApbPrscl::k2
			|| kApb1Prs == ApbPrscl::k4
			|| kApb1Prs == ApbPrscl::k8
			|| kApb1Prs == ApbPrscl::k16
			, "APB1 prescaler parameter is invalid."
		);
		// Invalid APB2 prescaler
		static_assert(
			kApb2Prs == ApbPrscl::k1
			|| kApb2Prs == ApbPrscl::k2
			|| kApb2Prs == ApbPrscl::k4
			|| kApb2Prs == ApbPrscl::k8
			|| kApb2Prs == ApbPrscl::k16
			, "APB2 prescaler parameter is invalid."
		);
		// Invalid ADC prescaler
		static_assert(
			kAdcPrs == AdcPrscl::k2
			|| kAdcPrs == AdcPrscl::k4
			|| kAdcPrs == AdcPrscl::k6
			|| kAdcPrs == AdcPrscl::k8
			, "ADC prescaler parameter is invalid."
		);
		// Invalid Clock setting
		static_assert(
			kFrequency_ <= 72000000UL
			, "Clock setting is overclocking MCU"
			);
		// Invalid AHB Clock setting
		static_assert(
			kAhbClock_ <= 72000000UL
			, "AHB divisor is causing overclock"
			);
		// Invalid APB1 Clock setting
		static_assert(
			kApb1Clock_ <= 36000000UL
			, "APB1 divisor is causing overclock"
			);
		// Invalid APB2 Clock setting
		static_assert(
			kApb2Clock_ <= 72000000UL
			, "APB2 divisor is causing overclock"
			);

		/*
		** Seems unbelievable, but this complex logic handling all clock 
		** frequencies are simplified to a single assignment!
		** The power of constexpr of C++!
		*/
		uint32_t tmp;	// reset value
		// Flash memory
		if (kFrequency_ > 48000000UL)
			tmp = FLASH_ACR_LATENCY_2 | FLASH_ACR_PRFTBE;
		else if (kFrequency_ > 24000000UL)
			tmp = FLASH_ACR_LATENCY_1 | FLASH_ACR_PRFTBE;
		else
			tmp = FLASH_ACR_PRFTBE;
		// Is Flash half cycle access possible?
		if (kFrequency_ <= 8000000UL
			&& ClockSource::kClockSource_ != kPLL )
		{
			tmp |= FLASH_ACR_HLFCYA;
		}
		// Apply
		FLASH->ACR = tmp;
		// Load state to register and clear all bits handled here
		// AHB
		switch (kAhbPrs)
		{
		case AhbPrscl::k1:
			tmp = RCC_CFGR_HPRE_DIV1;
			break;
		case AhbPrscl::k2:
			tmp = RCC_CFGR_HPRE_DIV2;
			break;
		case AhbPrscl::k4:
			tmp = RCC_CFGR_HPRE_DIV4;
			break;
		case AhbPrscl::k8:
			tmp = RCC_CFGR_HPRE_DIV8;
			break;
		case AhbPrscl::k16:
			tmp = RCC_CFGR_HPRE_DIV16;
			break;
		case AhbPrscl::k64:
			tmp = RCC_CFGR_HPRE_DIV64;
			break;
		case AhbPrscl::k128:
			tmp = RCC_CFGR_HPRE_DIV128;
			break;
		case AhbPrscl::k256:
			tmp = RCC_CFGR_HPRE_DIV256;
			break;
		case AhbPrscl::k512:
			tmp = RCC_CFGR_HPRE_DIV512;
			break;
		default:
			tmp = 0;
			break;
		}
		// APB1
		switch (kApb1Prs)
		{
		case ApbPrscl::k1:
			tmp |= RCC_CFGR_PPRE1_DIV1;
			break;
		case ApbPrscl::k2:
			tmp |= RCC_CFGR_PPRE1_DIV2;
			break;
		case ApbPrscl::k4:
			tmp |= RCC_CFGR_PPRE1_DIV4;
			break;
		case ApbPrscl::k8:
			tmp |= RCC_CFGR_PPRE1_DIV8;
			break;
		case ApbPrscl::k16:
			tmp |= RCC_CFGR_PPRE1_DIV16;
			break;
		default:
			break;
		}
		// APB2
		switch (kApb2Prs)
		{
		case ApbPrscl::k1:
			tmp |= RCC_CFGR_PPRE2_DIV1;
			break;
		case ApbPrscl::k2:
			tmp |= RCC_CFGR_PPRE2_DIV2;
			break;
		case ApbPrscl::k4:
			tmp |= RCC_CFGR_PPRE2_DIV4;
			break;
		case ApbPrscl::k8:
			tmp |= RCC_CFGR_PPRE2_DIV8;
			break;
		case ApbPrscl::k16:
			tmp |= RCC_CFGR_PPRE2_DIV16;
			break;
		default:
			break;
		}
		// ADC
		switch (kAdcPrs)
		{
		case AdcPrscl::k2:
			tmp |= RCC_CFGR_ADCPRE_DIV2;
			break;
		case AdcPrscl::k4:
			tmp |= RCC_CFGR_ADCPRE_DIV4;
			break;
		case AdcPrscl::k6:
			tmp |= RCC_CFGR_ADCPRE_DIV6;
			break;
		case AdcPrscl::k8:
			tmp |= RCC_CFGR_ADCPRE_DIV8;
			break;
		default:
			break;
		}
		// Microcontroller clock output
		tmp |= uint32_t(kClockOut);
		// Set to lowest USB clock possible
		tmp |= RCC_CFGR_USBPRE;
		// Clock source
		if (ClockSource::kClockSource_ == kHSE)
			tmp |= RCC_CFGR_SW_HSE;
		else if(ClockSource::kClockSource_ == kPLL)
			tmp |= RCC_CFGR_SW_PLL;
		// Combine with current contents, preserving PLL bits and apply
		RCC->CFGR = tmp | (RCC->CFGR & (RCC_CFGR_PLLSRC_Msk | RCC_CFGR_PLLXTPRE_Msk | RCC_CFGR_PLLMULL_Msk));
		// Wait clock source settle
		if (ClockSource::kClockSource_ == kHSE)
		{
			while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSE) ;
		}
		else if(ClockSource::kClockSource_ == kPLL)
		{
			while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) ;
		}
	}
};


template<
	typename ClockSource = Hsi<>				//!< New clock source for System
	, const AhbPrscl kAhbPrs = AhbPrscl::k1		//!< AHB bus prescaler
	, const ApbPrscl kApb1Prs = ApbPrscl::k2	//!< APB1 bus prescaler
	, const ApbPrscl kApb2Prs = ApbPrscl::k1	//!< APB2 bus prescaler
	, const AdcPrscl kAdcPrs = AdcPrscl::k8		//!< ADC prescaler factor
	, const bool kHsiRcOff = true				//!< Init() disables HSI, if not current clock source
	, const Mco kClockOut = Mco::kMcoOff		//!< Turn MCU clock output on (it does not enable external pin)
	>
class AnyUsbSycClk : public AnySycClk<ClockSource, kAhbPrs, kApb1Prs, kApb2Prs, kAdcPrs, kHsiRcOff, kClockOut>
{
public:
	/// Alias for the Base class
	typedef AnySycClk<ClockSource, kAhbPrs, kApb1Prs, kApb2Prs, kAdcPrs, kHsiRcOff, kClockOut> super;
	/// Clock required by the USB peripheral
	static constexpr uint32_t kUsbClock = 48000000UL;
	/// Starts associated oscillator, initializes system clock prescalers and use oscillator for system clock
	ALWAYS_INLINE static void Init(void)
	{
		super::Init();
		SetUsbClock();
	}
	/// Initializes clock tree prescalers, assuming associated source was already started
	ALWAYS_INLINE static void Enable(void)
	{
		super::Enable();
		SetUsbClock();
	}
protected:
	/// Computes the USB clock
	ALWAYS_INLINE static void SetUsbClock(void)
	{
		static_assert
		(
			super::kFrequency_ == 3*kUsbClock/2
			|| super::kFrequency_ == kUsbClock
			, "USB clock imposes PLL clock of 48 or 72 MHz."
		);
		// Select prescaler to obtain 48 Mhz for USB device
		if (super::kFrequency_ == 3*kUsbClock/2)
			RCC->CFGR &= ~RCC_CFGR_USBPRE;
		else if (super::kFrequency_ == kUsbClock)
			RCC->CFGR |= RCC_CFGR_USBPRE;
	}
};


/// A template class for the RTC peripheral
template<
	typename ClockSource = Lsi					//!< New clock source for System
	>
class AnyRtcClk
{
public:
	/// Frequency of the clock
	static constexpr uint32_t kFrequency_ = ClockSource::kClockSource_ == kHSE
			? ClockSource::kFrequency_ / 128 : ClockSource::kFrequency_;
	/// Enable RTC clock within a backup domain transaction, assuming associated clock is already active and running
	ALWAYS_INLINE static void Enable(BkpDomainXact &)
	{
		// Makes sure that the right clock is passed
		static_assert
		(
			ClockSource::kClockSource_ == kLSE
			|| ClockSource::kClockSource_ == kLSI
			|| ClockSource::kClockSource_ == kHSE
			, "Allowed RTC clock source are LSE, LSI or HSE."
		);

		uint32_t tmp = RCC_BDCR_RTCEN;
		if(ClockSource::kClockSource_ == kLSE)
			tmp |= RCC_BDCR_RTCSEL_LSE;
		else if(ClockSource::kClockSource_ == kLSI)
			tmp |= RCC_BDCR_RTCSEL_LSI;
		else if(ClockSource::kClockSource_ == kHSE)
			tmp |= RCC_BDCR_RTCSEL_HSE;
		// Apply
		RCC->BDCR =
		(
			RCC->BDCR &
			(
				RCC_BDCR_RTCSEL_Msk
				| RCC_BDCR_RTCEN_Msk
			)
		) | tmp;
	}

	//! Disables the RTC oscillator. You must ensure that associated peripherals are mapped elsewhere
	ALWAYS_INLINE static void Disable(BkpDomainXact &)
	{
		RCC->BDCR = RCC->BDCR &
		~(
			RCC_BDCR_RTCSEL_Msk
			| RCC_BDCR_RTCEN_Msk
		);
	}
	
	//! Reset backup domain system
	ALWAYS_INLINE static void ResetAll()
	{
		RCC->BDCR != RCC_BDCR_BDRST;
	}
};

}	// namespace Clocks
}	// namespace Bmt

