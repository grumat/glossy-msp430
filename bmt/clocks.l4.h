#pragma once

namespace Bmt
{

/// Clock source selection
enum ClockSourceType
{
	kHSI16_ClockSource,	///< HSI16 (high speed internal)16 MHz RC oscillator clock
	kMSI_ClockSource,	///< MSI (multispeed internal) RC oscillator clock
	kHSE_ClockSource,	///< HSE oscillator clock, from 4 to 48 MHz
	kPLL_ClockSource,	///< PLL clock source
	kLSI_ClockSource,	///< 32 kHz low speed internal RC
	kLSE_ClockSource,	///< 32.768 kHz low speed external crystal
	kHSI48_ClockSource,	///< RC 48 MHz internal clock sources
};


/// Class for the HSI16 clock source
class Hsi16
{
public:
	/// Clock identification
	static constexpr ClockSourceType kClockSource_ = kHSI16_ClockSource;
	/// Frequency of the clock source
	static constexpr uint32_t kFrequency_ = 16000000UL;
	/// Oscillator that generates the clock (not switchable in this particular case)
	static constexpr ClockSourceType kClockInput_ = kClockSource_;
	/// Starts HSI16 oscillator
	ALWAYS_INLINE static void Init(void)
	{
		Enable();
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
		return uint8_t((RCC->ICSCR & RCC_ICSCR_HSICAL_Msk) >> RCC_ICSCR_HSICAL_Pos);
	}
	/// Reads current trim value
	ALWAYS_INLINE static uint8_t Trim()
	{
		return (RCC->ICSCR & ~RCC_ICSCR_HSITRIM_Msk) >> RCC_ICSCR_HSITRIM_Pos;
	}
	/// Sets a new trim value
	ALWAYS_INLINE static void Trim(const uint8_t v)
	{
		RCC->ICSCR = (RCC->ICSCR & ~RCC_ICSCR_HSITRIM_Msk) | ((v << RCC_ICSCR_HSITRIM_Pos) & RCC_ICSCR_HSITRIM_Msk);
	}
};


/// Possible frequencies for MSI clock
enum class MsiFreq : uint8_t
{
	k100_kHz,
	k200_kHz,
	k400_kHz,
	k800_kHz,
	k1_MHz,
	k2_MHz,
	k4_MHz,
	k8_MHz,
	k16_MHz,
	k24_MHz,
	k32_MHz,
	k48_MHz,
	kMax,
};


/// Class for the Msi clock source
template<
	const MsiFreq kFreqCR = MsiFreq::k4_MHz	///< Frequency of oscillator
	, const bool kPllMode = false	///< Don't touch; use MsiPll template (see below)
>
class Msi
{
public:
	/// Clock identification
	static constexpr ClockSourceType kClockSource_ = kHSI16_ClockSource;
	/// Frequency of the clock source
	static constexpr uint32_t kFrequency_ =
		kFreqCR == MsiFreq::k100_kHz ? 100000UL
		: kFreqCR == MsiFreq::k200_kHz ? 200000UL
		: kFreqCR == MsiFreq::k400_kHz ? 400000UL
		: kFreqCR == MsiFreq::k800_kHz ? 800000UL
		: kFreqCR == MsiFreq::k1_MHz ? 1000000UL
		: kFreqCR == MsiFreq::k2_MHz ? 2000000UL
		: kFreqCR == MsiFreq::k4_MHz ? 4000000UL
		: kFreqCR == MsiFreq::k8_MHz ? 8000000UL
		: kFreqCR == MsiFreq::k16_MHz ? 16000000UL
		: kFreqCR == MsiFreq::k24_MHz ? 24000000UL
		: kFreqCR == MsiFreq::k32_MHz ? 32000000UL
		: kFreqCR == MsiFreq::k48_MHz ? 48000000UL
		: 1;
	/// Oscillator that generates the clock (not switchable in this particular case)
	static constexpr ClockSourceType kClockInput_ = kClockSource_;
	/// Configuration register setup
	static constexpr MsiFreq kCrEnum_ = kFreqCR;
	/// Special Configuration for MsiPll class
	static constexpr bool kUsePllMode_ = kPllMode;

	/// Starts HSI16 oscillator
	ALWAYS_INLINE static void Init(void)
	{
		static_assert(kFrequency_ > 1, "Hardware does not supports specified frequency");
#if 0
		// Initialized on system reset!
		Init();
#endif
	}
	/// Enables the HSI oscillator
	ALWAYS_INLINE static void Enable(void)
	{
		if (kUsePllMode_)
		{
			RCC->CR = (RCC->CR & ~RCC_CR_MSIRANGE_Msk)
				| RCC_CR_MSION
				| RCC_CR_MSIPLLEN
				| (uint32_t(kCrEnum_) << RCC_CR_MSIRANGE_Pos);
		}
		else
		{
			RCC->CR = (RCC->CR & ~(RCC_CR_MSIRANGE_Msk | RCC_CR_MSIPLLEN_Msk))
				| RCC_CR_MSION
				| (uint32_t(kCrEnum_) << RCC_CR_MSIRANGE_Pos);
		}
		while (!(RCC->CR & RCC_CR_MSIRDY));
	}
	/// Disables the HSI oscillator. You must ensure that associated peripherals are mapped elsewhere
	ALWAYS_INLINE static void Disable(void)
	{
		RCC->CR &= ~(RCC_CR_MSION_Msk | RCC_CR_MSIPLLEN_Msk);
	}
	/// Reads factory default calibration
	ALWAYS_INLINE static uint8_t GetCal()
	{
		return uint8_t((RCC->ICSCR & RCC_ICSCR_MSICAL_Msk) >> RCC_ICSCR_MSICAL_Pos);
	}
	/// Reads current trim value
	ALWAYS_INLINE static uint8_t Trim()
	{
		return (RCC->ICSCR & ~RCC_ICSCR_MSITRIM_Msk) >> RCC_ICSCR_MSITRIM_Pos;
	}
	/// Sets a new trim value
	ALWAYS_INLINE static void Trim(const uint8_t v)
	{
		RCC->ICSCR = (RCC->ICSCR & ~RCC_ICSCR_MSITRIM_Msk) | ((v << RCC_ICSCR_MSITRIM_Pos) & RCC_ICSCR_MSITRIM_Msk);
	}
};


/// Template class for the HSE clock source
template<
	const uint32_t kFrequency = 8000000UL	///< Frequency of oscillator
	, const bool kCssEnabled = false		///< Clock Security System enable
	, const bool kBypass = true				///< Low pin count devices have only CK input
>
class HseTemplate
{
public:
	/// A constant for the Clock identification
	static constexpr ClockSourceType kClockSource_ = kHSE_ClockSource;
	/// A constant for the frequency
	static constexpr uint32_t kFrequency_ = kFrequency;
	/// Actual oscillator that generates the clock
	static constexpr ClockSourceType kClockInput_ = kClockSource_;
	/// In low pin count devices only external clock is available
	static constexpr bool kBypass_ = kBypass;

	/// Starts HSE oscillator
	ALWAYS_INLINE static void Init(void)
	{
		Enable();
	}
	/// Enables the HSE oscillator
	ALWAYS_INLINE static void Enable(void)
	{
		uint32_t tmp = RCC_CR_HSEON;
		if (kBypass_)
			tmp |= RCC_CR_HSEBYP;
		if (kCssEnabled)
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
		if (kCssEnabled)
			tmp &= ~RCC_CR_CSSON;
		RCC->CR &= tmp;
	}
};


/// Template class for the LSE oscillator
template<
	const uint32_t kFrequency = 32768	///< Frequency for the LSE clock
	, const bool kBypass = false		///< LSE oscillator bypassed with external clock
>
class LseOsc
{
public:
	/// A constant for the Clock identification
	static constexpr ClockSourceType kClockSource_ = kLSE_ClockSource;
	/// A constant for the frequency
	static constexpr uint32_t kFrequency_ = kFrequency;
	/// Actual oscillator that generates the clock
	static constexpr ClockSourceType kClockInput_ = kClockSource_;
	/// Starts LSE oscillator within a backup domain transaction
	ALWAYS_INLINE static void Init(BkpDomainXact& xact)
	{
		Enable(xact);
	}
	/// Enables the LSE oscillator within a backup domain transaction
	ALWAYS_INLINE static void Enable(BkpDomainXact&)
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
class LsiOsc
{
public:
	/// A constant for the Clock identification
	static constexpr ClockSourceType kClockSource_ = kLSI_ClockSource;
	/// A constant for the frequency
	static constexpr uint32_t kFrequency_ = 40000;
	/// Actual oscillator that generates the clock (40 kHz)
	static constexpr ClockSourceType kClockInput_ = kClockSource_;
	/// Starts LSI oscillator
	ALWAYS_INLINE static void Init(void)
	{
		Enable();
	}
	/// Enables the LSI oscillator
	ALWAYS_INLINE static void Enable(void)
	{
		RCC->CSR |= RCC_CSR_LSION;
		while (!(RCC->CSR & RCC_CSR_LSIRDY));
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
class PllTemplate
{
public:
	/// A constant for the Clock identification
	static constexpr ClockSourceType kClockSource_ = kPLL_ClockSource;
	/// A constant for the frequency
	static constexpr uint32_t kFrequency_ = kFrequency;
	/// Actual oscillator that generates the clock (the linked clock source)
	static constexpr ClockSourceType kClockInput_ = ClockSource::kClockSource_;

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
		if (ClockSource::kClockSource_ == kHSE_ClockSource)
		{
			if (kFrequency_ <= (9 * ClockSource::kFrequency_ / 2))			// PREDIV = /2
			{
				tmp = (uint32_t)(RCC_CFGR_PLLSRC
					| RCC_CFGR_PLLXTPRE_HSE_DIV2
					| Multiplier(ClockSource::kFrequency_ / 2, kFrequency_));
			}
			else if (kFrequency_ == (13 * ClockSource::kFrequency_ / 2))	// factor 6.5
			{
				tmp = (uint32_t)(RCC_CFGR_PLLSRC
					| (0x0D << RCC_CFGR_PLLMULL_Pos));
			}
			else if (kFrequency_ == (13 * ClockSource::kFrequency_ / 4))	// factor 6.5 with PREDIV = /2
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
			tmp = (uint32_t)(Multiplier(ClockSource::kFrequency_ / 2, kFrequency_));
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
		while (!(RCC->CR & RCC_CR_PLLRDY));
	}
	/// Disables the PLL oscillator. You must ensure that associated peripherals are mapped elsewhere
	ALWAYS_INLINE static void Disable(void)
	{
		RCC->CR &= ~RCC_CR_PLLON;
	}
};


/// These are the possible values to source the MCO output with clock
enum ClockOutputType : uint32_t
{
	kMcoOff = RCC_CFGR_MCO_NOCLOCK,				///< No Clock
	kMcoSysClk = RCC_CFGR_MCO_SYSCLK,			///< System clock (SYSCLK) selected
	kMcoHsi = RCC_CFGR_MCO_HSI,					///< HSI clock selected
	kMcoHse = RCC_CFGR_MCO_HSE,					///< HSE clock selected
	kMcoPllClkDiv2 = RCC_CFGR_MCO_PLLCLK_DIV2	///< PLL clock divided by 2 selected
};

/// AHB clock Prescaler
enum AHBPrescaler : uint16_t
{
	kAhbPres_1 = 1,			///< No clock prescaler
	kAhbPres_2 = 2,			///< Divide clock by 2
	kAhbPres_4 = 4,			///< Divide clock by 4
	kAhbPres_8 = 8,			///< Divide clock by 8
	kAhbPres_16 = 16,		///< Divide clock by 16
	kAhbPres_32 = 32,		///< Divide clock by 32
	kAhbPres_64 = 64,		///< Divide clock by 64
	kAhbPres_128 = 128,		///< Divide clock by 128
	kAhbPres_256 = 256,		///< Divide clock by 256
	kAhbPres_512 = 512,		///< Divide clock by 512
};

/// APB1/2 clock Prescaler
enum APBPrescaler : uint8_t
{
	kApbPres_1 = 1,			///< No clock prescaler
	kApbPres_2 = 2,			///< Divide clock by 2
	kApbPres_4 = 4,			///< Divide clock by 4
	kApbPres_8 = 8,			///< Divide clock by 8
	kApbPres_16 = 16,		///< Divide clock by 16
};

/// ADC clock Prescaler
enum ADCPrescaler : uint8_t
{
	kAdcPres_2 = 2,			///< Divide clock by 2
	kAdcPres_4 = 4,			///< Divide clock by 4
	kAdcPres_8 = 8,			///< Divide clock by 8
	kAdcPres_16 = 16,		///< Divide clock by 16
};

/*!
A class to setup System Clock. Please check the clock tree @RM0008 (r21-Fig.8).
STM32F10x allows System Clocks sourced from HSI, HSE or PLL only.
*/
template<
	typename ClockSource = Hsi							///< New clock source for System
	, const AHBPrescaler kAhbPrescaler = kAhbPres_1		///< AHB bus prescaler
	, const APBPrescaler kApb1Prescaler = kApbPres_2	///< APB1 bus prescaler
	, const APBPrescaler kApb2Prescaler = kApbPres_1	///< APB2 bus prescaler
	, const ADCPrescaler kAdcPrescaler = kAdcPres_8		///< ADC prescaler factor
	, const bool kHsiRcOff = true						///< Init() disables HSI, if not current clock source
	, const ClockOutputType kClockOut = kMcoOff			///< Turn MCU clock output on (it does not enable external pin)
>
class SysClkTemplate
{
public:
	/// The system clock frequency (a constant)
	static constexpr uint32_t kFrequency_ = ClockSource::kFrequency_;
	/// Effective AHB clock frequency (a constant)
	static constexpr uint32_t kAhbClock_ = kFrequency_ / kAhbPrescaler;
	/// Effective APB1 clock frequency (a constant)
	static constexpr uint32_t kApb1Clock_ = kAhbClock_ / kApb1Prescaler;
	/// Effective clock for timer connected to APB1
	static constexpr uint32_t kApb1TimerClock_ = (kApb1Prescaler == 1) ? kApb1Clock_ : 2 * kApb1Clock_;
	/// Effective APB2 clock frequency (a constant)
	static constexpr uint32_t kApb2Clock_ = kAhbClock_ / kApb2Prescaler;
	/// Effective clock for timer connected to APB2
	static constexpr uint32_t kApb2TimerClock_ = (kApb1Prescaler == 1) ? kApb2Clock_ : 2 * kApb2Clock_;
	/// Effective ADC clock
	static constexpr uint32_t kAdc_ = kAhbClock_ / kAdcPrescaler;
	/// Clock output mode
	static constexpr ClockOutputType kMco_ = kClockOut;

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

	/// Initializes clock tree prescalers, assuming associated source was already started
	ALWAYS_INLINE static void Enable(void)
	{
		// System clock restricts sources
		static_assert(
			ClockSource::kClockSource_ == kHSI_ClockSource
			|| ClockSource::kClockSource_ == kHSE_ClockSource
			|| ClockSource::kClockSource_ == kPLL_ClockSource
			, "Allowed System Clock source are HSI, HSE or PLL."
			);
		// Invalid AHB prescaler
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
		// Invalid APB1 prescaler
		static_assert(
			kApb1Prescaler == 1
			|| kApb1Prescaler == 2
			|| kApb1Prescaler == 4
			|| kApb1Prescaler == 8
			|| kApb1Prescaler == 16
			, "APB1 prescaler parameter is invalid."
			);
		// Invalid APB2 prescaler
		static_assert(
			kApb2Prescaler == 1
			|| kApb2Prescaler == 2
			|| kApb2Prescaler == 4
			|| kApb2Prescaler == 8
			|| kApb2Prescaler == 16
			, "APB2 prescaler parameter is invalid."
			);
		// Invalid ADC prescaler
		static_assert(
			kAdcPrescaler == 2
			|| kAdcPrescaler == 4
			|| kAdcPrescaler == 6
			|| kAdcPrescaler == 8
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
			&& ClockSource::kClockSource_ != kPLL_ClockSource)
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
		else if (ClockSource::kClockSource_ == kPLL_ClockSource)
			tmp |= RCC_CFGR_SW_PLL;
		// Combine with current contents, preserving PLL bits and apply
		RCC->CFGR = tmp | (RCC->CFGR & (RCC_CFGR_PLLSRC_Msk | RCC_CFGR_PLLXTPRE_Msk | RCC_CFGR_PLLMULL_Msk));
		// Wait clock source settle
		if (ClockSource::kClockSource_ == kHSE_ClockSource)
		{
			while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSE);
		}
		else if (ClockSource::kClockSource_ == kPLL_ClockSource)
		{
			while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
		}
	}
};


template<
	typename ClockSource = Hsi							//!< New clock source for System
	, const AHBPrescaler kAhbPrescaler = kAhbPres_1		//!< AHB bus prescaler
	, const APBPrescaler kApb1Prescaler = kApbPres_2	//!< APB1 bus prescaler
	, const APBPrescaler kApb2Prescaler = kApbPres_1	//!< APB2 bus prescaler
	, const ADCPrescaler kAdcPrescaler = kAdcPres_8		//!< ADC prescaler factor
	, const bool kHsiRcOff = true						//!< Init() disables HSI, if not current clock source
	, const ClockOutputType kClockOut = kMcoOff			//!< Turn MCU clock output on (it does not enable external pin)
>
class SysClkUsbTemplate : public SysClkTemplate<ClockSource, kAhbPrescaler, kApb1Prescaler, kApb2Prescaler, kAdcPrescaler, kHsiRcOff, kClockOut>
{
public:
	/// Alias for the Base class
	typedef SysClkTemplate<ClockSource, kAhbPrescaler, kApb1Prescaler, kApb2Prescaler, kAdcPrescaler, kHsiRcOff, kClockOut> super;
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


/// A template class for the RTC peripheral
template<
	typename ClockSource = LsiOsc					//!< New clock source for System
>
class RtcClkTemplate
{
public:
	/// Frequency of the clock
	static constexpr uint32_t kFrequency_ = ClockSource::kClockSource_ == kHSE_ClockSource
		? ClockSource::kFrequency_ / 128 : ClockSource::kFrequency_;
	/// Enable RTC clock within a backup domain transaction, assuming associated clock is already active and running
	ALWAYS_INLINE static void Enable(BkpDomainXact&)
	{
		// Makes sure that the right clock is passed
		static_assert
			(
				ClockSource::kClockSource_ == kLSE_ClockSource
				|| ClockSource::kClockSource_ == kLSI_ClockSource
				|| ClockSource::kClockSource_ == kHSE_ClockSource
				, "Allowed RTC clock source are LSE, LSI or HSE."
				);

		uint32_t tmp = RCC_BDCR_RTCEN;
		if (ClockSource::kClockSource_ == kLSE_ClockSource)
			tmp |= RCC_BDCR_RTCSEL_LSE;
		else if (ClockSource::kClockSource_ == kLSI_ClockSource)
			tmp |= RCC_BDCR_RTCSEL_LSI;
		else if (ClockSource::kClockSource_ == kHSE_ClockSource)
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

}	// namespace Bmt
