#pragma once

#include "pinremap.h"
#include "../shared/AnyHsi.h"
#include "../shared/AnyHse.h"
#include "../shared/AnyLse.h"
#include "../shared/AnyLsi.h"

namespace Bmt
{
namespace Clocks
{

/// Clock source selection
enum class Id
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
class AnyHsi : public Private::AnyHsi<Id::kHSI, 8000000UL, kTrim>
{ };


/// Template class for the HSE clock source
template<
	const uint32_t kFrequency = 8000000UL	///< Frequency of oscillator
	, bool kBypass = false					///< HSE oscillator bypassed with external clock
	, const bool kCssEnabled = false		///< Clock Security System enable
>
class AnyHse : public Private::AnyHse<Id::kHSE, kFrequency, kBypass, kCssEnabled>
{ };


/// Template class for the LSE oscillator
template<
	const uint32_t kFrequency = 32768	///< Frequency for the LSE clock
	, const bool kBypass = false		///< LSE oscillator bypassed with external clock
>
class AnyLse : public Private::AnyLse<Id::kLSE, kFrequency, kBypass>
{ };


/// Class for the LSI oscillator
class Lsi : public Private::AnyLsi<Id::kLSI, 40000UL>
{ };



//! PLL parameters according to data-sheet
template<
	typename ClockSource
>
class AnyPllVco : public Private::AnyPllVco<
	1000000UL, 25000000UL
	, 16000000UL, 72000000UL
	, 2, 16
	, 1 + (ClockSource::kClockSource_ == Id::kHSI), 2
>
{};


/// Template class for the PLL Oscillator
template<
	typename ClockSource,					///< PLL is linked to another clock source
	const uint32_t kFrequency,				///< The output frequency of the PLL
	typename PllCalculator = AnyPllVco<ClockSource>,	///< Calculator for the PLL
	const uint32_t kMaxErr = 10				///< Max approximation error
>
class AnyPll
{
public:
	/// A constant for the Clock identification
	static constexpr Id kClockSource_ = Id::kPLL;
	/// Obtains the constant with PLL configuration computed at compile time
	static constexpr PllFraction kPllFraction_ = PllCalculator::ComputePllFraction(ClockSource::kFrequency_, kFrequency);
	/// A constant for the frequency
	static constexpr uint32_t kFrequency_ = kPllFraction_.GetFrequency();
	/// Actual oscillator that generates the clock (the linked clock source)
	static constexpr Id kClockInput_ = ClockSource::kClockSource_;

	/// Starts associated oscillator and then the PLL
	constexpr static void Init(void)
	{
		// Makes sure that the source clock is supported by the PLL hardware
		static_assert(kClockInput_ == Id::kHSE || kClockInput_ == Id::kHSI, "Cannot bind clock source in chain with the PLL. Only HSI and HSE are supported");
		// Desired Frequency cannot be configured in the PLL
		static_assert(kPllFraction_.n >= PllCalculator::kN_Min_, "Underflow of PLLMUL configuration.");
		static_assert(kPllFraction_.n <= PllCalculator::kN_Max_, "Overflow of PLLMUL configuration.");
		static_assert(kPllFraction_.m >= PllCalculator::kM_Min_, "Underflow of PLLXTPRE configuration.");
		static_assert(kPllFraction_.m <= PllCalculator::kM_Max_, "Overflow of PLLXTPRE configuration.");
		// Desired Frequency cannot be configured in the PLL
		static_assert(kPllFraction_.err <= kMaxErr, "Desired frequency deviates too much from PLL capacity");
		ClockSource::Init();
		Enable();
	}

	/// Enables the PLL oscillator, assuming associated source was already started
	constexpr static void Enable(void)
	{
		// Work on register
		uint32_t tmp = 0;
		if(ClockSource::kClockSource_ == Id::kHSE)
		{
			tmp = RCC_CFGR_PLLSRC;
			if (kPllFraction_.m == 2)
				tmp |= RCC_CFGR_PLLXTPRE_HSE_DIV2;
		}
		tmp |= (kPllFraction_.n - 2) << RCC_CFGR_PLLMULL_Pos;
		// Merge bits with HW register
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
	constexpr static void Disable(void)
	{
		RCC->CR &= ~RCC_CR_PLLON;
	}
};


/// These are the possible values to source the MCO output with clock
enum class Mco : uint32_t
{
	kOff = RCC_CFGR_MCO_NOCLOCK,			///< No Clock
	kSysClk = RCC_CFGR_MCO_SYSCLK,			///< System clock (SYSCLK) selected
	kHsi = RCC_CFGR_MCO_HSI,				///< HSI clock selected
	kHse = RCC_CFGR_MCO_HSE,				///< HSE clock selected
	kPllClkDiv2 = RCC_CFGR_MCO_PLLCLK_DIV2	///< PLL clock divided by 2 selected
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
	typename ClockSource = AnyHsi<>				///< New clock source for System
	, const AhbPrscl kAhbPrs = AhbPrscl::k1		///< AHB bus prescaler
	, const ApbPrscl kApb1Prs = ApbPrscl::k2	///< APB1 bus prescaler
	, const ApbPrscl kApb2Prs = ApbPrscl::k1	///< APB2 bus prescaler
	, const AdcPrscl kAdcPrs = AdcPrscl::k8		///< ADC prescaler factor
	, const bool kHsiRcOff = true				///< Init() disables HSI, if not current clock source
	, const Mco kClockOut = Mco::kOff			///< Turn MCU clock output on (it does not enable external pin)
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
	constexpr static void Init(void)
	{
		// Initialization clocks in chain for code simplicity
		ClockSource::Init();
		// Enables itself
		Enable();
		// Disabling HSI if setting up a System clock source
		if (
			kHsiRcOff 
			&& (ClockSource::kClockSource_ == Id::kHSE
				|| (ClockSource::kClockSource_ == Id::kPLL
					&& ClockSource::kClockInput_ != Id::kHSI)
			)
		)
		{
			// Note that flash controller needs this clock for programming!!!
			AnyHsi<>::Disable();
		}
	}

	/// Initializes clock tree prescalers, assuming associated source was already started
	constexpr static void Enable(void)
	{
		// System clock restricts sources
		static_assert(
			ClockSource::kClockSource_ == Id::kHSI
			|| ClockSource::kClockSource_ == Id::kHSE
			|| ClockSource::kClockSource_ == Id::kPLL
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
			, "AHB divisor is overclocked"
			);
		// Invalid APB1 Clock setting
		static_assert(
			kApb1Clock_ <= 36000000UL
			, "APB1 divisor is overclocked"
			);
		// Invalid APB2 Clock setting
		static_assert(
			kApb2Clock_ <= 72000000UL
			, "APB2 divisor is overclocked"
			);

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
			&& ClockSource::kClockSource_ != Id::kPLL )
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
		if (ClockSource::kClockSource_ == Id::kHSE)
			tmp |= RCC_CFGR_SW_HSE;
		else if(ClockSource::kClockSource_ == Id::kPLL)
			tmp |= RCC_CFGR_SW_PLL;
		// Combine with current contents, preserving PLL bits and apply
		RCC->CFGR = tmp | (RCC->CFGR & ~(RCC_CFGR_PLLSRC_Msk | RCC_CFGR_PLLXTPRE_Msk | RCC_CFGR_PLLMULL_Msk));
		// Wait clock source settle
		if (ClockSource::kClockSource_ == Id::kHSE)
		{
			while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSE) ;
		}
		else if(ClockSource::kClockSource_ == Id::kPLL)
		{
			while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) ;
		}
	}
};


template<
	typename ClockSource = AnyHsi<>				//!< New clock source for System
	, const AhbPrscl kAhbPrs = AhbPrscl::k1		//!< AHB bus prescaler
	, const ApbPrscl kApb1Prs = ApbPrscl::k2	//!< APB1 bus prescaler
	, const ApbPrscl kApb2Prs = ApbPrscl::k1	//!< APB2 bus prescaler
	, const AdcPrscl kAdcPrs = AdcPrscl::k8		//!< ADC prescaler factor
	, const bool kHsiRcOff = true				//!< Init() disables HSI, if not current clock source
	, const Mco kClockOut = Mco::kOff			//!< Turn MCU clock output on (it does not enable external pin)
	>
class AnyUsbSycClk : public AnySycClk<ClockSource, kAhbPrs, kApb1Prs, kApb2Prs, kAdcPrs, kHsiRcOff, kClockOut>
{
public:
	/// Alias for the Base class
	typedef AnySycClk<ClockSource, kAhbPrs, kApb1Prs, kApb2Prs, kAdcPrs, kHsiRcOff, kClockOut> super;
	/// Clock required by the USB peripheral
	static constexpr uint32_t kUsbClock = 48000000UL;
	/// Starts associated oscillator, initializes system clock prescalers and use oscillator for system clock
	constexpr static void Init(void)
	{
		super::Init();
		SetUsbClock();
	}
	/// Initializes clock tree prescalers, assuming associated source was already started
	constexpr static void Enable(void)
	{
		super::Enable();
		SetUsbClock();
	}
protected:
	/// Computes the USB clock
	constexpr static void SetUsbClock(void)
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
	static constexpr uint32_t kFrequency_ = ClockSource::kClockSource_ == Id::kHSE
			? ClockSource::kFrequency_ / 128 : ClockSource::kFrequency_;
	/// Enable RTC clock within a backup domain transaction, assuming associated clock is already active and running
	constexpr static void Enable(BkpDomainXact &)
	{
		// Makes sure that the right clock is passed
		static_assert
		(
			ClockSource::kClockSource_ == Id::kLSE
			|| ClockSource::kClockSource_ == Id::kLSI
			|| ClockSource::kClockSource_ == Id::kHSE
			, "Allowed RTC clock source are LSE, LSI or HSE."
		);

		uint32_t tmp = RCC_BDCR_RTCEN;
		if(ClockSource::kClockSource_ == Id::kLSE)
			tmp |= RCC_BDCR_RTCSEL_LSE;
		else if(ClockSource::kClockSource_ == Id::kLSI)
			tmp |= RCC_BDCR_RTCSEL_LSI;
		else if(ClockSource::kClockSource_ == Id::kHSE)
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
	constexpr static void Disable(BkpDomainXact &)
	{
		RCC->BDCR = RCC->BDCR &
		~(
			RCC_BDCR_RTCSEL_Msk
			| RCC_BDCR_RTCEN_Msk
		);
	}
	
	//! Reset backup domain system
	constexpr static void ResetAll()
	{
		RCC->BDCR != RCC_BDCR_BDRST;
	}
};

}	// namespace Clocks
}	// namespace Bmt

