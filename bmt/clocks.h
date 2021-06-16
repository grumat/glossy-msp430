#pragma once

#include "otherlibs.h"

#include "pinremap.h"
#include "mcu-system.h"


enum ClockSourceType
{
	kHSI_ClockSource,
	kHSE_ClockSource,
	kLSI_ClockSource,
	kLSE_ClockSource,
	kPLL_ClockSource
};


class Hsi
{
public:
	static constexpr ClockSourceType kClockSource_ = kHSI_ClockSource;
	static constexpr uint32_t kFrequency_ = 8000000UL;
	static constexpr ClockSourceType kClockInput_ = kClockSource_;	// actual oscillator that generates clock
	//! Starts HSI oscillator
	ALWAYS_INLINE static void Init(void)
	{
		Enable();
	}
	//! Enables the HSI oscillator
	ALWAYS_INLINE static void Enable(void)
	{
		RCC->CR |= RCC_CR_HSION;
		while (!(RCC->CR & RCC_CR_HSIRDY));
	}
	//! Disables the HSI oscillator. You must ensure that associated peripherals are mapped elsewhere
	ALWAYS_INLINE static void Disable(void)
	{
		RCC->CR &= ~RCC_CR_HSION;
	}
};


template<
	const uint32_t kFrequency = 8000000UL
	, bool kBypass = false
	, const bool kCssEnabled = false				//!< Clock Security System enable
	>
class HseTemplate
{
public:
	static constexpr ClockSourceType kClockSource_ = kHSE_ClockSource;
	static constexpr uint32_t kFrequency_ = kFrequency;
	static constexpr ClockSourceType kClockInput_ = kClockSource_;	// actual oscillator that generates clock

	//! Starts HSE oscillator
	ALWAYS_INLINE static void Init(void)
	{
		Enable();
	}
	//! Enables the HSE oscillator
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
	//! Disables the HSE oscillator. You must ensure that associated peripherals are mapped elsewhere
	ALWAYS_INLINE static void Disable(void)
	{
		uint32_t tmp = ~RCC_CR_HSEON;
		if(kCssEnabled)
			tmp &= ~RCC_CR_CSSON;
		RCC->CR &= tmp;
	}
};


template<
	const uint32_t kFrequency = 32768
	, const bool kBypass = false
	>
class LseOsc
{
public:
	static constexpr ClockSourceType kClockSource_ = kLSE_ClockSource;
	static constexpr uint32_t kFrequency_ = kFrequency;
	static constexpr ClockSourceType kClockInput_ = kClockSource_;	// actual oscillator that generates clock
	//! Starts LSE oscillator within a backup domain transaction
	ALWAYS_INLINE static void Init(BkpDomainXact &xact)
	{
		Enable(xact);
	}
	//! Enables the LSE oscillator within a backup domain transaction
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
	//! Disables the LSE oscillator. You must ensure that associated peripherals are mapped elsewhere
	ALWAYS_INLINE static void Disable(void)
	{
		RCC->BDCR &= ~RCC_BDCR_LSEON;
	}
};


class LsiOsc
{
public:
	static constexpr ClockSourceType kClockSource_ = kLSI_ClockSource;
	static constexpr uint32_t kFrequency_ = 40000;
	static constexpr ClockSourceType kClockInput_ = kClockSource_;	// actual oscillator that generates clock
	//! Starts LSI oscillator
	ALWAYS_INLINE static void Init(void)
	{
		Enable();
	}
	//! Enables the LSI oscillator
	ALWAYS_INLINE static void Enable(void)
	{
		RCC->CSR |= RCC_CSR_LSION;
		while(!(RCC->CSR & RCC_CSR_LSIRDY)); 
	}
	//! Disables the LSI oscillator. You must ensure that associated peripherals are mapped elsewhere
	ALWAYS_INLINE static void Disable(void)
	{
		RCC->CSR &= ~RCC_CSR_LSION;
	}
};


template<typename ClockSource, const uint32_t kFrequency>
class PllTemplate
{
public:
	static constexpr ClockSourceType kClockSource_ = kPLL_ClockSource;
	static constexpr uint32_t kFrequency_ = kFrequency;
	static constexpr ClockSourceType kClockInput_ = ClockSource::kClockSource_;	// actual oscillator that generates clock
	ALWAYS_INLINE static constexpr uint32_t Multiplier(const uint32_t source_frequency, const uint32_t kFrequency_)
	{
		return (kFrequency_ / source_frequency - 2) << 18;
	}
	//! Starts associated oscillator and then the PLL
	ALWAYS_INLINE static void Init(void)
	{
		ClockSource::Init();
		Enable();
	}
	//! Enables the PLL oscillator, assuming associated source was already started
	ALWAYS_INLINE static void Enable(void)
	{
		// Work on register
		uint32_t tmp;
		// This complex logic is statically simplified by optimizing compiler
		// when one uses constants
		if(ClockSource::kClockSource_ == kHSE_ClockSource)
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
	//! Disables the PLL oscillator. You must ensure that associated peripherals are mapped elsewhere
	ALWAYS_INLINE static void Disable(void)
	{
		RCC->CR &= ~RCC_CR_PLLON;
	}
};


//! These are the possible values to source a GPIO pin with clock
enum ClockOutputType : uint32_t
{
	kMcoOff = RCC_CFGR_MCO_NOCLOCK,
	kMcoSysClk = RCC_CFGR_MCO_SYSCLK,
	kMcoHsi = RCC_CFGR_MCO_HSI,
	kMcoHse = RCC_CFGR_MCO_HSE,
	kMcoPllClkDiv2 = RCC_CFGR_MCO_PLLCLK_DIV2
};


/*!
A class to setup System Clock. Please check the clock tree @RM0008 (r21-Fig.8).
STM32F10x allows System Clocks sourced from HSI, HSE or PLL only.
*/
template<
	typename ClockSource = Hsi					//!< New clock source for System
	, const uint16_t kAhbPrescaler = 1				//!< AHB bus prescaler
	, const uint8_t kApb1Prescaler = 2				//!< APB1 bus prescaler
	, const uint8_t kApb2Prescaler = 1				//!< APB2 bus prescaler
	, const uint8_t kAdcPrescaler = 8				//!< ADC prescaler factor
	, const bool kHsiRcOff = true					//!< Init() disables HSI, if not current clock source
	, const ClockOutputType kClockOut = kMcoOff	//!< Turn MCU clock output on (it does not enable external pin)
	>
class SysClkTemplate
{
public:
	static constexpr uint32_t kFrequency_ = ClockSource::kFrequency_;
	static constexpr uint32_t kAhbClock_ = kFrequency_ / kAhbPrescaler;
	static constexpr uint32_t kApb1Clock_ = kAhbClock_ / kApb1Prescaler;
	static constexpr uint32_t kApb1TimerClock_ = (kApb1Prescaler == 1) ? kApb1Clock_ : 2 * kApb1Clock_;
	static constexpr uint32_t kApb2Clock_ = kAhbClock_ / kApb2Prescaler;
	static constexpr uint32_t kApb2TimerClock_ = (kApb1Prescaler == 1) ? kApb2Clock_ : 2 * kApb2Clock_;
	static constexpr uint32_t kAdc_ = kAhbClock_ / kAdcPrescaler;
	static constexpr ClockOutputType kMco_ = kClockOut;
	//! Starts associated oscillator, initializes clock tree prescalers and use oscillator for system clock
	ALWAYS_INLINE static void Init(void)
	{
		// Initialization clocks in chain for code simplicity
		ClockSource::Init();
		// Enables itself
		Enable();
		// Disabling HSI if setting up a System clock source
		if (
			kHsiRcOff 
			&& (ClockSource::kClockSource_ == kHSE_ClockSource
				|| (ClockSource::kClockSource_ == kPLL_ClockSource
					&& ClockSource::kClockInput_ != kHSI_ClockSource)
			)
		)
		{
			// Note that flash controller needs this clock for programming!!!
			Hsi::Disable();
		}
	}
	//! Initializes clock tree prescalers, assuming associated source was already started
	ALWAYS_INLINE static void Enable(void)
	{
		// System clock restricts sources
		static_assert(
			ClockSource::kClockSource_ == kHSI_ClockSource
			|| ClockSource::kClockSource_ == kHSE_ClockSource
			|| ClockSource::kClockSource_ == kPLL_ClockSource
			, "Allowed System Clock source are HSI, HSE or PLL."
		);
		static_assert(
			kAhbPrescaler == 1
			|| kAhbPrescaler == 2
			|| kAhbPrescaler == 4
			|| kAhbPrescaler == 8
			|| kAhbPrescaler == 16
			|| kAhbPrescaler == 64
			|| kAhbPrescaler == 128
			|| kAhbPrescaler == 256
			|| kAhbPrescaler == 512
			, "AHB prescaler parameter is invalid."
		);
		static_assert(
			kApb1Prescaler == 1
			|| kApb1Prescaler == 2
			|| kApb1Prescaler == 4
			|| kApb1Prescaler == 8
			|| kApb1Prescaler == 16
			, "APB1 prescaler parameter is invalid."
		);
		static_assert(
			kApb2Prescaler == 1
			|| kApb2Prescaler == 2
			|| kApb2Prescaler == 4
			|| kApb2Prescaler == 8
			|| kApb2Prescaler == 16
			, "APB2 prescaler parameter is invalid."
		);
		static_assert(
			kAdcPrescaler == 2
			|| kAdcPrescaler == 4
			|| kAdcPrescaler == 6
			|| kAdcPrescaler == 8
			, "ADC prescaler parameter is invalid."
		);
		static_assert(
			kFrequency_ <= 72000000UL
			, "Clock setting is overclocking MCU"
			);
		static_assert(
			kAhbClock_ <= 72000000UL
			, "AHB divisor is causing overclock"
			);
		static_assert(
			kApb1Clock_ <= 36000000UL
			, "APB1 divisor is causing overclock"
			);
		static_assert(
			kApb2Clock_ <= 72000000UL
			, "APB2 divisor is causing overclock"
			);

		/*
		** Seems unbelivable, but this complex logic handling all clock 
		** frequencies are simplified to a single assignment!
		** The power of C++!
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
			&& ClockSource::kClockSource_ != kPLL_ClockSource )
		{
			tmp |= FLASH_ACR_HLFCYA;
		}
		// Apply
		FLASH->ACR = tmp;
		// Load state to register and clear all bits handled here
		// AHB
		switch (kAhbPrescaler)
		{
		case 1:
			tmp = RCC_CFGR_HPRE_DIV1;
			break;
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
		default:
			tmp = 0;
			break;
		}
		// APB1
		switch (kApb1Prescaler)
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
		switch (kApb2Prescaler)
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
		// ADC
		switch (kAdcPrescaler)
		{
		case 2:
			tmp |= RCC_CFGR_ADCPRE_DIV2;
			break;
		case 4:
			tmp |= RCC_CFGR_ADCPRE_DIV4;
			break;
		case 6:
			tmp |= RCC_CFGR_ADCPRE_DIV6;
			break;
		case 8:
			tmp |= RCC_CFGR_ADCPRE_DIV8;
			break;
		default:
			break;
		}
		// Microcontroller clock output
		tmp |= kClockOut;
		// Set to lowest USB clock possible
		tmp |= RCC_CFGR_USBPRE;
		// Clock source
		if (ClockSource::kClockSource_ == kHSE_ClockSource)
			tmp |= RCC_CFGR_SW_HSE;
		else if(ClockSource::kClockSource_ == kPLL_ClockSource)
			tmp |= RCC_CFGR_SW_PLL;
		// Combine with current contents, preserving PLL bits and apply
		RCC->CFGR = tmp | (RCC->CFGR & (RCC_CFGR_PLLSRC_Msk | RCC_CFGR_PLLXTPRE_Msk | RCC_CFGR_PLLMULL_Msk));
		// Wait clock source settle
		if (ClockSource::kClockSource_ == kHSE_ClockSource)
		{
			while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSE) ;
		}
		else if(ClockSource::kClockSource_ == kPLL_ClockSource)
		{
			while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) ;
		}
	}
};


template<
	typename ClockSource = Hsi					//!< New clock source for System
	, const uint16_t kAhbPrescaler = 1				//!< AHB bus prescaler
	, const uint8_t kApb1Prescaler = 2				//!< APB1 bus prescaler
	, const uint8_t kApb2Prescaler = 2				//!< APB2 bus prescaler
	, const uint8_t kAdcPrescaler = 8				//!< ADC prescaler factor
	, const bool kHsiRcOff = true					//!< Init() disables HSI, if not current clock source
	, const ClockOutputType kClockOut = kMcoOff	//!< Turn MCU clock output on (it does not enable external pin)
	>
class SysClkUsbTemplate : public SysClkTemplate<ClockSource, kAhbPrescaler, kApb1Prescaler, kApb2Prescaler, kAdcPrescaler, kHsiRcOff, kClockOut>
{
public:
	typedef SysClkTemplate<ClockSource, kAhbPrescaler, kApb1Prescaler, kApb2Prescaler, kAdcPrescaler, kHsiRcOff, kClockOut> super;
	static constexpr uint32_t kUsbClock = 48000000UL;
	//! Starts associated oscillator, initializes system clock prescalers and use oscillator for system clock
	ALWAYS_INLINE static void Init(void)
	{
		super::Init();
		SetUsbClock();
	}
	//! Initializes clock tree prescalers, assuming associated source was already started
	ALWAYS_INLINE static void Enable(void)
	{
		super::Enable();
		SetUsbClock();
	}
protected:
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


template<
	typename ClockSource = LsiOsc					//!< New clock source for System
	>
class RtcClkTemplate
{
public:
	static constexpr uint32_t kFrequency_ = ClockSource::kClockSource_ == kHSE_ClockSource
			? ClockSource::kFrequency_ / 128 : ClockSource::kFrequency_;
	//! Enable RTC clock within a backup domain transaction, assuming associated clock is already active and running
	ALWAYS_INLINE static void Enable(BkpDomainXact &)
	{
		static_assert
		(
			ClockSource::kClockSource_ == kLSE_ClockSource
			|| ClockSource::kClockSource_ == kLSI_ClockSource
			|| ClockSource::kClockSource_ == kHSE_ClockSource
			, "Allowed RTC clock source are LSE, LSI or HSE."
		);

		uint32_t tmp = RCC_BDCR_RTCEN;
		if(ClockSource::kClockSource_ == kLSE_ClockSource)
			tmp |= RCC_BDCR_RTCSEL_LSE;
		else if(ClockSource::kClockSource_ == kLSI_ClockSource)
			tmp |= RCC_BDCR_RTCSEL_LSI;
		else if(ClockSource::kClockSource_ == kHSE_ClockSource)
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

