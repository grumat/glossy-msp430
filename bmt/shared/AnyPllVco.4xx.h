#pragma once

namespace Bmt
{
namespace Clocks
{

enum class Id;


//! Calculates VCO output frequency by specifying a fixed '/R' divider
template<
	typename PllVcoAttr,
	const uint32_t k_R = 2
>
class AnyPllVco : public Private::PllVcoAttr_<PllVcoAttr>
{
	typedef Private::PllVcoAttr_<PllVcoAttr> super;
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
template<typename PllVcoAttr>
class AnyPllVcoAuto : public Private::PllVcoAttr_<PllVcoAttr>
{
	typedef Private::PllVcoAttr_<PllVcoAttr> super;
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
	typename ClockSource,			///< PLL is linked to another clock source
	const uint32_t kFrequency,		///< The output frequency of the PLL (using PllCalculator)
	typename PllCalculator,			///< How to compute the VCO
	const bool kRDiv = true,		///< Sets and enables PLLCLK (if false PLLR is not set even if used in PllCalculator)
	const uint32_t kPDiv = 0,		///< PDIV value (0 for disable, 2...31)
	const uint32_t kQDiv = 0,		///< QDIV value (0 for disable, 2, 4, 6, 8)
	const uint32_t kMaxErr = 5		///< Max approximation error
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
#if defined(RCC_PLLCFGR_PLLSRC_MSI)
		static_assert(kClockInput_ == Id::kMSI || kClockInput_ == Id::kHSI16 || kClockInput_ == Id::kHSE
			, "The source clock circuit cannot be chained to the PLL");
#else
		static_assert(kClockInput_ == Id::kHSI16 || kClockInput_ == Id::kHSE
			, "The source clock circuit cannot be chained to the PLL");
#endif
		ClockSource::Init();
		Enable();
	}

	/// Enables the PLL oscillator, assuming associated source was already started
	constexpr static void Enable(void)
	{
		uint32_t tmp = 0;
#if defined(RCC_PLLCFGR_PLLSRC_MSI)
		if (kClockInput_ == Id::kMSI)
			tmp |= RCC_PLLCFGR_PLLSRC_MSI;
#endif
		if (kClockInput_ == Id::kHSI16)
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

}	// namespace Clocks
}	// namespace Bmt
