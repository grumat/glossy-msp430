#pragma once

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
	kHSI16,		///< HSI16 (high speed internal)16 MHz RC oscillator clock
	kHSE,		///< HSE oscillator clock, from 4 to 48 MHz
	kPLL,		///< PLL clock source
	kLSI,		///< 32 kHz low speed internal RC
	kLSE,		///< 32.768 kHz low speed external crystal
	kHSI48,		///< RC 48 MHz internal clock sources
};

}	// namespace Clocks
}	// namespace Bmt

#include "../shared/AnyPllVco.4xx.h"

namespace Bmt
{
namespace Clocks
{

/// Default HSI trim value
#if defined(RCC_ICSCR_HSITRIM_6)
static constexpr uint8_t kHsiDefaultTrim = 0b01000000;
#else
static constexpr uint8_t kHsiDefaultTrim = 0b00010000;
#endif

/// Class for the HSI16 clock source
template<
	const uint8_t kTrim = kHsiDefaultTrim
>
class AnyHsi16 : public Private::AnyHsi<Id::kHSI16, 16000000UL, kHsiDefaultTrim>
{ };


/// Template class for the HSE clock source
template<
	const uint32_t kFrequency = 8000000UL	///< Frequency of oscillator
	, const bool kBypass = true				///< Low pin count devices have only CK input
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


//! Use this as base for the PllVco<>/PllVcoAuto<> calculator
using PllRange1 = Private::AnyPllVco<
	4000000UL,						//!< VCO input is after '/M' divisor
	16000000UL,						//!< Data-sheet limits PLL to 16 MHz
	96000000UL,						//!< 96 Mhz regardless of power mode
	344000000UL,					//!< Power mode determines the PLL top range
	8UL, 86UL,						//!< PLL 'xN' adjustment range
	1UL, 8UL						//!< PLL '/M' adjustment range
>;

//! Use this as base for the PllVco<>/PllVcoAuto<> calculator
using PllRange2 = Private::AnyPllVco<
	4000000UL,						//!< VCO input is after '/M' divisor
	16000000UL,						//!< Data-sheet limits PLL to 16 MHz
	96000000UL,						//!< 96 Mhz regardless of power mode
	128000000UL,					//!< Power mode determines the PLL top range
	8UL, 86UL,						//!< PLL 'xN' adjustment range
	1UL, 8UL						//!< PLL '/M' adjustment range
>;


// General PLL calculator for Range 1 core voltage
typedef AnyPllVco<PllRange1> Range1;
// General PLL calculator for Range 2 core voltage
typedef AnyPllVco<PllRange2> Range2;
// Most flexible PLL calculator ('/R' auto-selected)
typedef AnyPllVcoAuto<PllRange1> AutoRange1;
// Most flexible PLL calculator ('/R' auto-selected)
typedef AnyPllVcoAuto<PllRange2> AutoRange2;


/// These are the possible values to source the MCO output with clock
enum class Mco : uint32_t
{
	kOff	= 0b0000,			///< No Clock
	kSysClk = 0b0001,			///< System clock (SYSCLK) selected
	kMsi	= 0b0010,			///< HSI clock selected
	kHsi16	= 0b0011,			///< HSI16 clock selected
	kHse	= 0b0100,			///< HSE clock selected
	kPllClk = 0b0101,			///< Main PLL clock selected
	kLsi	= 0b0110,			///< LSI clock selected
	kLse	= 0b0111,			///< LSE clock selected
	kHsi48	= 0b1000,			///< HSI48 clock selected
};


//! The MCO output can be divided by a factor
enum  class McoPrscl : uint32_t
{
	k1,							//!< MCO is divided by 1
	k2,							//!< MCO is divided by 2
	k4,							//!< MCO is divided by 4
	k8,							//!< MCO is divided by 8
	k16,						//!< MCO is divided by 16
};


/*!
A class to setup System Clock. Please check the clock tree @RM0008 (r21-Fig.8).
STM32F10x allows System Clocks sourced from HSI, HSE or PLL only.
*/
template<
	typename ClockSource = AnyHsi16<>				///< New clock source for System
	, const Power::Mode	kMode = Power::Mode::kRange1	///< Power mode to use the configuration
	, const AhbPrscl kAhbPrs = AhbPrscl::k1		///< AHB bus prescaler
	, const ApbPrscl kApb1Prs = ApbPrscl::k2	///< APB1 bus prescaler
	, const ApbPrscl kApb2Prs = ApbPrscl::k1	///< APB2 bus prescaler
	, const bool kMsiRcOff = true				///< Init() disables MSI, if not current clock source
	, const Mco kClockOut = Mco::kOff			///< Turn MCU clock output on (it does not enable external pin)
	, const McoPrscl kMcoPrscl = McoPrscl::k1	///< If MCO is active, the factor to divide output frequency
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
	/// Clock output mode
	static constexpr Mco kMco_ = kClockOut;
	/// Clock output mode
	static constexpr McoPrscl kMcoPrscl_ = kMcoPrscl;

	/// Starts associated oscillator, initializes clock tree prescalers and use oscillator for system clock
	ALWAYS_INLINE static void Init(void)
	{
		// Initialization clocks in chain for code simplicity
		ClockSource::Init();
		// Enables itself
		Enable();
		// Disabling HSI if setting up a System clock source
		if (kMsiRcOff
			&& ClockSource::kClockSource_ != Id::kHSI16 )
		{
			// Note that flash controller needs this clock for programming!!!
			AnyHsi16<>::Disable();
		}
	}

	/// Initializes clock tree prescalers, assuming associated source was already started
	ALWAYS_INLINE static void Enable(void)
	{
		// System clock restricts sources
		static_assert(
			ClockSource::kClockSource_ == Id::kHSI16
			|| ClockSource::kClockSource_ == Id::kHSE
			|| ClockSource::kClockSource_ == Id::kPLL
			, "Allowed System Clock source are MSI, HSI16, HSE or PLL."
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
		// Invalid Clock setting
		static_assert(
			kFrequency_ <= 80000000UL
			, "Clock setting is overclocking MCU"
			);
		// Invalid AHB Clock setting
		static_assert(
			kAhbClock_ <= 80000000UL
			, "AHB divisor is overclocked"
			);
		// Invalid APB1 Clock setting
		static_assert(
			kApb1Clock_ <= 80000000UL
			, "APB1 divisor is overclocked"
			);
		// Invalid APB2 Clock setting
		static_assert(
			kApb2Clock_ <= 80000000UL
			, "APB2 divisor is overclocked"
			);

		uint32_t tmp;	// reset value
		// Flash memory
		if (kMode == Power::Mode::kRange1)
		{
			if (kFrequency_ > 64000000UL)
				tmp = FLASH_ACR_LATENCY_4WS;
			else if (kFrequency_ > 48000000UL)
				tmp = FLASH_ACR_LATENCY_3WS;
			else if (kFrequency_ > 32000000UL)
				tmp = FLASH_ACR_LATENCY_2WS;
			else if (kFrequency_ > 16000000UL)
				tmp = FLASH_ACR_LATENCY_1WS;
			else
				tmp = FLASH_ACR_LATENCY_0WS;
		}
		else
		{
			if (kFrequency_ > 16000000UL)
				tmp = FLASH_ACR_LATENCY_3WS;
			else if (kFrequency_ > 12000000UL)
				tmp = FLASH_ACR_LATENCY_2WS;
			else if (kFrequency_ > 6000000UL)
				tmp = FLASH_ACR_LATENCY_1WS;
			else
				tmp = FLASH_ACR_LATENCY_0WS;
		}
		tmp |= FLASH_ACR_DCEN;		// data cache enable
		tmp |= FLASH_ACR_ICEN;		// instruction cache enable
		tmp |= FLASH_ACR_PRFTEN;	// prefetch enable
		// Apply
		FLASH->ACR = tmp;
		// Load state to register and clear all bits handled here
		// AHB
		switch (kAhbPrs)
		{
		case 2:
			tmp = RCC_CFGR_HPRE_DIV2;
			break;
		case 4:
			tmp = RCC_CFGR_HPRE_DIV4;
			break;
		case 8:
			tmp = RCC_CFGR_HPRE_DIV8;
			break;
		case 16:
			tmp = RCC_CFGR_HPRE_DIV16;
			break;
		case 64:
			tmp = RCC_CFGR_HPRE_DIV64;
			break;
		case 128:
			tmp = RCC_CFGR_HPRE_DIV128;
			break;
		case 256:
			tmp = RCC_CFGR_HPRE_DIV256;
			break;
		case 512:
			tmp = RCC_CFGR_HPRE_DIV512;
			break;
		case 1:
		default:
			tmp = RCC_CFGR_HPRE_DIV1;
			break;
		}
		// APB1
		switch (kApb1Prs)
		{
		case 1:
			tmp |= RCC_CFGR_PPRE1_DIV1;
			break;
		case 2:
			tmp |= RCC_CFGR_PPRE1_DIV2;
			break;
		case 4:
			tmp |= RCC_CFGR_PPRE1_DIV4;
			break;
		case 8:
			tmp |= RCC_CFGR_PPRE1_DIV8;
			break;
		case 16:
			tmp |= RCC_CFGR_PPRE1_DIV16;
			break;
		default:
			break;
		}
		// APB2
		switch (kApb2Prs)
		{
		case 1:
			tmp |= RCC_CFGR_PPRE2_DIV1;
			break;
		case 2:
			tmp |= RCC_CFGR_PPRE2_DIV2;
			break;
		case 4:
			tmp |= RCC_CFGR_PPRE2_DIV4;
			break;
		case 8:
			tmp |= RCC_CFGR_PPRE2_DIV8;
			break;
		case 16:
			tmp |= RCC_CFGR_PPRE2_DIV16;
			break;
		default:
			break;
		}
		// Microcontroller clock output
		tmp |= uint32_t(kClockOut) << RCC_CFGR_MCOSEL_Pos;
		tmp |= uint32_t(kMcoPrscl) << RCC_CFGR_MCOPRE_Pos;
		// Clock source
		if (ClockSource::kClockSource_ == Id::kHSE)
			tmp |= RCC_CFGR_SW_HSE;
		else if (ClockSource::kClockSource_ == Id::kPLL)
			tmp |= RCC_CFGR_SW_PLL;
		else
			tmp |= RCC_CFGR_SW_HSI;
		// Combine with current contents, preserving PLL bits and apply
		RCC->CFGR = tmp | (RCC->CFGR & ~(
			RCC_CFGR_SW_Msk 
			| RCC_CFGR_HPRE_Msk 
			| RCC_CFGR_PPRE1_Msk
			| RCC_CFGR_PPRE2_Msk
			| RCC_CFGR_MCOSEL_Msk
			| RCC_CFGR_MCOPRE_Msk
			));
		// Wait clock source settle
		if (ClockSource::kClockSource_ == Id::kHSE)
		{
			while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_HSE);
		}
		else if (ClockSource::kClockSource_ == Id::kPLL)
		{
			while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_PLL);
		}
		else
		{
			while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_HSI);
		}
	}
};


#if 0
template<
	typename ClockSource = Hsi							//!< New clock source for System
	, const AhbPrscl kAhbPrs = k1		//!< AHB bus prescaler
	, const ApbPrscl kApb1Prs = k2	//!< APB1 bus prescaler
	, const ApbPrscl kApb2Prs = k1	//!< APB2 bus prescaler
	, const AdcPrscl kAdcPrs = k8		//!< ADC prescaler factor
	, const bool kHsiRcOff = true						//!< Init() disables HSI, if not current clock source
	, const Mco kClockOut = kMcoOff			//!< Turn MCU clock output on (it does not enable external pin)
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
				super::kFrequency_ == 3 * kUsbClock / 2
				|| super::kFrequency_ == kUsbClock
				, "USB clock imposes PLL clock of 48 or 72 MHz."
				);
		// Select prescaler to obtain 48 Mhz for USB device
		if (super::kFrequency_ == 3 * kUsbClock / 2)
			RCC->CFGR &= ~RCC_CFGR_USBPRE;
		else if (super::kFrequency_ == kUsbClock)
			RCC->CFGR |= RCC_CFGR_USBPRE;
	}
};
#endif


/// A template class for the RTC peripheral
template<
	typename ClockSource = Lsi					//!< New clock source for System
>
class AnyRtcClk
{
public:
	/// Frequency of the clock
	static constexpr uint32_t kFrequency_ = ClockSource::kClockSource_ == Id::kHSE
		? ClockSource::kFrequency_ / 32 : ClockSource::kFrequency_;
	/// Enable RTC clock within a backup domain transaction, assuming associated clock is already active and running
	ALWAYS_INLINE static void Enable(BkpDomainXact&)
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
		if (ClockSource::kClockSource_ == Id::kLSE)
			tmp |= 0b0001UL << RCC_BDCR_RTCSEL_Pos;
		else if (ClockSource::kClockSource_ == Id::kLSI)
			tmp |= 0b0010UL << RCC_BDCR_RTCSEL_Pos;
		else if (ClockSource::kClockSource_ == Id::kHSE)
			tmp |= 0b0011UL << RCC_BDCR_RTCSEL_Pos;
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
	ALWAYS_INLINE static void Disable(BkpDomainXact&)
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
