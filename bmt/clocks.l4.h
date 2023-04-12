#pragma once

namespace Bmt
{
namespace Clocks
{

/// Clock source selection
enum class Id
{
	kHSI16,		///< HSI16 (high speed internal)16 MHz RC oscillator clock
	kMSI,		///< MSI (multispeed internal) RC oscillator clock
	kHSE,		///< HSE oscillator clock, from 4 to 48 MHz
	kPLL,		///< PLL clock source
	kLSI,		///< 32 kHz low speed internal RC
	kLSE,		///< 32.768 kHz low speed external crystal
	kHSI48,		///< RC 48 MHz internal clock sources
};

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
class AnyHsi16
{
public:
	/// Clock identification
	static constexpr Id kClockSource_ = Id::kHSI16;
	/// Frequency of the clock source
	static constexpr uint32_t kFrequency_ = 16000000UL;
	/// Oscillator that generates the clock (not switchable in this particular case)
	static constexpr Id kClockInput_ = kClockSource_;
	/// Starts HSI16 oscillator
	ALWAYS_INLINE static void Init(void)
	{
		Enable();
		// Apply calibration
		Trim(kHsiDefaultTrim);
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
		RCC->ICSCR = (RCC->ICSCR & ~RCC_ICSCR_HSITRIM_Msk) | (((uint32_t)v << RCC_ICSCR_HSITRIM_Pos) & RCC_ICSCR_HSITRIM_Msk);
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
	static constexpr Id kClockSource_ = Id::kHSI16;
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
	static constexpr Id kClockInput_ = kClockSource_;
	/// Configuration register setup
	static constexpr MsiFreq kCrEnum_ = kFreqCR;
	/// Special Configuration for MsiPll class
	static constexpr bool kUsePllMode_ = kPllMode;

	/// Starts MSI oscillator
	ALWAYS_INLINE static void Init(void)
	{
		static_assert(kFrequency_ > 1, "Hardware does not supports specified frequency");
		Init();
	}
	/// Enables the MSI oscillator
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
	/// Disables the MSI oscillator. You must ensure that associated peripherals are mapped elsewhere
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
	, const bool kBypass = true				///< Low pin count devices have only CK input
	, const bool kCssEnabled = false		///< Clock Security System enable
>
class AnyHse
{
public:
	/// A constant for the Clock identification
	static constexpr Id kClockSource_ = Id::kHSE;
	/// A constant for the frequency
	static constexpr uint32_t kFrequency_ = kFrequency;
	/// Actual oscillator that generates the clock
	static constexpr Id kClockInput_ = kClockSource_;
	/// In low pin count devices only external clock is available
	static constexpr bool kBypass_ = kBypass;

	/// Starts HSE oscillator
	constexpr static void Init(void)
	{
		Enable();
	}
	/// Enables the HSE oscillator
	constexpr static void Enable(void)
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
	constexpr static void Disable(void)
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
class AnyLse
{
public:
	/// A constant for the Clock identification
	static constexpr Id kClockSource_ = Id::kLSE;
	/// A constant for the frequency
	static constexpr uint32_t kFrequency_ = kFrequency;
	/// Actual oscillator that generates the clock
	static constexpr Id kClockInput_ = kClockSource_;
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
class Lsi
{
public:
	/// A constant for the Clock identification
	static constexpr Id kClockSource_ = Id::kLSI;
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
		while (!(RCC->CSR & RCC_CSR_LSIRDY));
	}
	/// Disables the LSI oscillator. You must ensure that associated peripherals are mapped elsewhere
	ALWAYS_INLINE static void Disable(void)
	{
		RCC->CSR &= ~RCC_CSR_LSION;
	}
};


namespace Private
{

using PllRange1 = AnyPllVco<
	4000000UL,						//!< VCO input is after '/M' divisor
	16000000UL,						//!< Data-sheet limits PLL to 16 MHz
	96000000UL,						//!< 96 Mhz regardless of power mode
	344000000UL,					//!< Power mode determines the PLL top range
	8UL, 86UL,						//!< PLL 'xN' adjustment range
	1UL, 8UL						//!< PLL '/M' adjustment range
>;
using PllRange2 = AnyPllVco<
	4000000UL,						//!< VCO input is after '/M' divisor
	16000000UL,						//!< Data-sheet limits PLL to 16 MHz
	96000000UL,						//!< 96 Mhz regardless of power mode
	128000000UL,					//!< Power mode determines the PLL top range
	8UL, 86UL,						//!< PLL 'xN' adjustment range
	1UL, 8UL						//!< PLL '/M' adjustment range
>;


//! Establishes parameters for the STM32L432 PLL (do not use directly)
/*!
Formula for the PLL:
	VCO(out) = ( (CLKIN / M) * N ) / R
Where:
	VCO(out)	: Typically the PLLCLK that drives CPU. It also drives
					PLL48M1CLK and PLLSAI2CLK.
	CLKIN		: This is selectable between MSI, HSI16 or HSE
	M			: Input divisor for the PLL. It is important the clock runs
					in the range on 4 to 16 MHz for correct operation.
					In the data-sheet this is labeled as '/M'.
	N			: VCO multiplier that produces a high frequency. For the
					correct operation, data-sheet states an operating range
					between 96 and 344 Mhz. In the data-sheet this is labeled
					'xN'.
	R			: Output divisor for the PLLCLK. Hardware also offers the
					/P and /Q divisors with the same functionality. These
					have to be set as not to exceed 80 MHz. In the data-sheet
					this is labeled '/R'.

This routine will try to compute '/M' and 'xN' terms to achieve the best
approximation to a given target frequency and a given '/R' factor.
*/
template<
	const Power::Mode kMode = Power::Mode::kRange1
>
class PllVcoAttr_
{
public:
	//! The minimum allowed input frequency
	static constexpr uint32_t kVcoInMin_ = kMode == Power::Mode::kRange1 ? PllRange1::kVcoInMin_ : PllRange2::kVcoInMin_;
	//! The maximum allowed input frequency
	static constexpr uint32_t kVcoInMax_ = kMode == Power::Mode::kRange1 ? PllRange1::kVcoInMax_ : PllRange2::kVcoInMax_;
	//! The minimum allowed output frequency
	static constexpr uint32_t kVcoOutMin_ = kMode == Power::Mode::kRange1 ? PllRange1::kVcoOutMin_ : PllRange2::kVcoOutMin_;
	//! The maximum allowed output frequency
	static constexpr uint32_t kVcoOutMax_ = kMode == Power::Mode::kRange1 ? PllRange1::kVcoOutMax_ : PllRange2::kVcoOutMax_;
	//! The minimum allowed multiplier
	static constexpr uint32_t kN_Min_ = kMode == Power::Mode::kRange1 ? PllRange1::kN_Min_ : PllRange2::kN_Min_;
	//! The maximum allowed multiplier
	static constexpr uint32_t kN_Max_ = kMode == Power::Mode::kRange1 ? PllRange1::kN_Max_ : PllRange2::kN_Max_;
	//! The minimum allowed divisor
	static constexpr uint32_t kM_Min_ = kMode == Power::Mode::kRange1 ? PllRange1::kM_Min_ : PllRange2::kM_Min_;
	//! The maximum allowed divisor
	static constexpr uint32_t kM_Max_ = kMode == Power::Mode::kRange1 ? PllRange1::kM_Max_ : PllRange2::kM_Max_;

	static constexpr PllFraction FindBestMatch(const uint32_t clk, const uint32_t fq, const uint32_t div)
	{
		// PLL cannot be used in low power mode
		static_assert(kMode != Power::Mode::kLowPower, "PLL not available in LPM");
		PllFraction res = kMode == Power::Mode::kRange1
			? PllRange1::ComputePllFraction(clk, fq * div)
			: PllRange2::ComputePllFraction(clk, fq * div)
			;
		res.r = div;
		return res;
	}
};
}	// namespace Private


//! Calculates VCO output frequency by specifying a fixed '/R' divider
template<
	const uint32_t k_R = 2,
	const Power::Mode kMode = Power::Mode::kRange1
>
class PllVco : public Private::PllVcoAttr_<kMode>
{
	typedef Private::PllVcoAttr_<kMode> super;
public:
	//! Divisor '/R' for the PLLCLK output frequency
	static constexpr uint32_t k_R_ = k_R;
	//! Algorithm to compute PLL fraction for a specified '/R' divisor
	static constexpr PllFraction ComputePllFraction(const uint32_t clk, const uint32_t fq)
	{
		static_assert(k_R == 2 || k_R == 4 || k_R == 6 || k_R == 8, "Divisor value for '/R' is not not supported by hardware");
		return super::FindBestMatch(clk, fq, k_R);
	}
};


//! Calculates VCO output frequency by selecting a proper '/R' divider
template<
	const Power::Mode kMode = Power::Mode::kRange1
>
class PllVcoAuto : public Private::PllVcoAttr_<kMode>
{
	typedef Private::PllVcoAttr_<kMode> super;
public:
	//! Tries the best approach
	static constexpr PllFraction ComputePllFraction(const uint32_t clk, const uint32_t fq)
	{
		// Use '/R=2'
		PllFraction res = super::FindBestMatch(clk, fq, 2);
		// Check for error
		if (res.err != 0)
		{
			// Use '/R=4'
			PllFraction r2 = super::FindBestMatch(clk, fq, 4);
			// Copy if deviation improves
			if (res.err > r2.err)
				res = r2;
			// Check for error
			if (res.err != 0)
			{
				// Use '/R=6'
				r2 = super::FindBestMatch(clk, fq, 6);
				// Copy if deviation improves
				if (res.err > r2.err)
					res = r2;
				// Check for error
				if (res.err != 0)
				{
					// Use '/R=8'
					r2 = super::FindBestMatch(clk, fq, 8);
					// Copy if deviation improves
					if (res.err > r2.err)
						res = r2;
				}
			}
		}
		// Return selection
		return res;
	}
};


/// Template class for the PLL Oscillator
template<
	typename ClockSource				///< PLL is linked to another clock source
	, const uint32_t kFrequency			///< The output frequency of the PLL (using PllCalculator)
	, typename PllCalculator = PllVco<>	///< How to compute the VCO
	, const bool kRDiv = true			///< Sets and enables PLLCLK (if false PLLR is not set even if used in PllCalculator)
	, const uint32_t kPDiv = 0			///< PDIV value (0 for disable, 2...31)
	, const uint32_t kQDiv = 0			///< QDIV value (0 for disable, 2, 4, 6, 8)
	, const uint32_t kMaxErr = 5		///< Max approximation error
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
		// The '/R' divisor of the PLL (aka RCC_PLLCFGR.PLLR) has fixed options
		static_assert(kPllFraction_.r == 2
			|| kPllFraction_.r == 4
			|| kPllFraction_.r == 6
			|| kPllFraction_.r == 8
			, "'/R' divisor value not supported by HW");
		// Check for device limits (Note: If one wants to over-clock device, do it by itself)
		static_assert(kFrequency_ <= 84000000UL
			, "System Over-clock detected (>5%!!!).");
		// Desired Frequency cannot be configured in the PLL
		static_assert(kPllFraction_.n >= PllCalculator::kN_Min_, "Underflow of PLLN configuration.");
		static_assert(kPllFraction_.n <= PllCalculator::kN_Max_, "Overflow of PLLN configuration.");
		static_assert(kPllFraction_.m >= PllCalculator::kM_Min_, "Underflow of PLLM configuration.");
		static_assert(kPllFraction_.m <= PllCalculator::kM_Max_, "Overflow of PLLM configuration.");
		// Desired Frequency cannot be configured in the PLL
		static_assert(kPllFraction_.err <= kMaxErr, "Desired frequency deviates too much from PLL capacity");
		// See RCC_PLLCFGR.PLLPDIV
		static_assert(kPDiv != 1 && kPDiv <= 31, "Value is not supported for PLLPDIV HW register");
		// See RCC_PLLCFGR.PLLQ
		static_assert((kQDiv <= 8) && (kQDiv % 2 == 0), "Invalid value for PLLQ; 2, 4, 6, 8 and 0 for disable");
		// Clock chaining is only possible with specific sources
		static_assert(kClockInput_ == Id::kMSI || kClockInput_ == Id::kHSI16 || kClockInput_ == Id::kHSE
			, "The source clock circuit cannot be chained to the PLL");
		ClockSource::Init();
		Enable();
	}

	/// Enables the PLL oscillator, assuming associated source was already started
	constexpr static void Enable(void)
	{
		uint32_t tmp = 0;
		if (kClockInput_ == Id::kMSI)
			tmp |= RCC_PLLCFGR_PLLSRC_MSI;
		else if (kClockInput_ == Id::kHSI16)
			tmp |= RCC_PLLCFGR_PLLSRC_HSI;
		else
			tmp |= RCC_PLLCFGR_PLLSRC_HSE;
		// Computes the register value
		// PLLM
		tmp |= ((kPllFraction_.m - 1) << RCC_PLLCFGR_PLLM_Pos);
		// PLLN
		tmp |= (kPllFraction_.n << RCC_PLLCFGR_PLLN_Pos);
		// PLLQ + PLLQEN
		if (kQDiv != 0)
		{
			const uint32_t div_2_pllq = ((kQDiv >> 1) - 1);
			tmp |= (div_2_pllq << RCC_PLLCFGR_PLLQ_Pos) | RCC_PLLCFGR_PLLQEN;
		}
		else
			tmp |= RCC_PLLCFGR_PLLQ_Msk;	// slowest freq if disabled
		// PLLREN + PLLR
		if (kRDiv)
		{
			// Converts division factor to PLLR enumeration
			const uint32_t div_2_pllr = ((kPllFraction_.r >> 1) - 1);
			tmp |= (div_2_pllr << RCC_PLLCFGR_PLLR_Pos) | RCC_PLLCFGR_PLLREN;
		}
		else
			tmp |= RCC_PLLCFGR_PLLR_Msk;	// slowest freq if disabled
		// PLLPDIV + PLLPEN
		if (kPDiv != 0)
			tmp |= (kPDiv << RCC_PLLCFGR_PLLPDIV_Pos) | RCC_PLLCFGR_PLLPEN;
		else
			tmp |= RCC_PLLCFGR_PLLPDIV_Msk;	// slowest freq if disabled

		// Merge into HW register
		RCC->PLLCFGR = RCC->PLLCFGR & 
			~(
				RCC_PLLCFGR_PLLSRC_Msk
				| RCC_PLLCFGR_PLLM_Msk
				| RCC_PLLCFGR_PLLN_Msk
				| RCC_PLLCFGR_PLLPEN_Msk
				| RCC_PLLCFGR_PLLP_Msk
				| RCC_PLLCFGR_PLLQEN_Msk
				| RCC_PLLCFGR_PLLQ_Msk
				| RCC_PLLCFGR_PLLREN_Msk
				| RCC_PLLCFGR_PLLR_Msk
				| RCC_PLLCFGR_PLLPDIV_Msk
			)
			| tmp
			;
		// Enable PLL 
		RCC->CR |= RCC_CR_PLLON;
		// Wait to settle
		while (!(RCC->CR & RCC_CR_PLLRDY));
	}
	/// Disables the PLL oscillator. You must ensure that associated peripherals are mapped elsewhere
	ALWAYS_INLINE static void Disable(void)
	{
		RCC->CR &= ~(RCC_CR_PLLON);
		RCC->PLLCFGR & ~(RCC_PLLCFGR_PLLSRC_Msk);	// Disables clock chaining
	}
	// 
};


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
			&& ClockSource::kClockSource_ != Id::kMSI )
		{
			// Note that flash controller needs this clock for programming!!!
			Msi<>::Disable();
		}
	}

	/// Initializes clock tree prescalers, assuming associated source was already started
	ALWAYS_INLINE static void Enable(void)
	{
		// System clock restricts sources
		static_assert(
			ClockSource::kClockSource_ == Id::kMSI
			|| ClockSource::kClockSource_ == Id::kHSI16
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
		if (ClockSource::kClockSource_ == Id::kHSI16)
			tmp |= RCC_CFGR_SW_HSI;
		else if (ClockSource::kClockSource_ == Id::kHSE)
			tmp |= RCC_CFGR_SW_HSE;
		else if (ClockSource::kClockSource_ == Id::kPLL)
			tmp |= RCC_CFGR_SW_PLL;
		else
			tmp |= RCC_CFGR_SW_MSI;
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
		if (ClockSource::kClockSource_ == Id::kHSI16)
		{
			while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_HSI);
		}
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
			while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_MSI);
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
