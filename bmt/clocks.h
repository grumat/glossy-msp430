#pragma once

#include "mcu-system.h"

#include "gpio.h"

namespace Bmt
{
namespace Clocks
{

//! This is a configuration result for the PLL
struct PllFraction
{
	uint32_t clk;	//!< The clock input frequency
	uint32_t fq;	//!< The desired frequency for approximation
	uint32_t fin;	//!< The VCO input frequency
	uint32_t fout;	//!< The VCO output frequency
	uint32_t n;		//!< The 'xN' multiplier
	uint32_t m;		//!< The '/M 'divisor
	uint32_t r;		//!< The '/R' divisor (valid for hardware that supports it)
	uint32_t err;	//!< The computed error between desired and output frequency (in percent)

	//! Used to validate results from ComputePllFraction() method
	constexpr bool IsValid() const { return n != 0; }
	//! Returns the frequency by dividing VCO(out) by the '/R' factor
	constexpr uint32_t GetFrequency() const { return fout / r; }
};


// Represents a basic PLL VCO circuit containing data-sheet specs
/*!
This model fits a PLL that follows the formula:
	F_out = (F_in / M) * N
In words, the input frequency is first divided by M and then multiplied by N to
produce the output frequency. All parameters have to meet data-sheet specs, 
given as template parameters.
*/
template<
	const uint32_t kVcoInMin,	//!< The minimum allowed input frequency
	const uint32_t kVcoInMax,	//!< The maximum allowed input frequency
	const uint32_t kVcoOutMin,	//!< The minimum allowed output frequency
	const uint32_t kVcoOutMax,	//!< The maximum allowed output frequency
	const uint32_t kN_Min,		//!< The minimum allowed multiplier
	const uint32_t kN_Max,		//!< The maximum allowed multiplier
	const uint32_t kM_Min,		//!< The minimum allowed divisor
	const uint32_t kM_Max		//!< The maximum allowed divisor
>
class AnyPllVco
{
public:
	//! The minimum allowed input frequency
	static constexpr uint32_t kVcoInMin_ = kVcoInMin;
	//! The maximum allowed input frequency
	static constexpr uint32_t kVcoInMax_ = kVcoInMax;
	//! The minimum allowed output frequency
	static constexpr uint32_t kVcoOutMin_ = kVcoOutMin;
	//! The maximum allowed output frequency
	static constexpr uint32_t kVcoOutMax_ = kVcoOutMax;
	//! The minimum allowed multiplier
	static constexpr uint32_t kN_Min_ = kN_Min;
	//! The maximum allowed multiplier
	static constexpr uint32_t kN_Max_ = kN_Max;
	//! The minimum allowed divisor
	static constexpr uint32_t kM_Min_ = kM_Min;
	//! The maximum allowed divisor
	static constexpr uint32_t kM_Max_ = kM_Max;

	//! Computes the result for a given input and desired output frequencies
	/*!
	Formula for the PLL:
		VCO(out) = ( (CLKIN / M) * N )
	Where:
		VCO(out)	: Typically the PLLCLK that drives CPU. It also drives
						PLL48M1CLK and PLLSAI2CLK.
		CLKIN		: This is selectable between MSI, HSI16 or HSE
		M			: Input divisor for the PLL. It is important the clock runs
						in the range on 4 to 16 MHz for correct operation.
		N			: VCO multiplier that produces a high frequency. For the
						correct operation an operating range between 
						'kVcoOutMin_' and 'kVcoOutMax_' are verified. This range 
						is device specific and should be specified according to 
						data-sheet.
	This routine will try to compute '/M' and 'xN' terms to achieve the best
	approximation to a given target frequency.
	This routine uses a classic brute force technique were all values are 
	checked until a match is found. An alternative were studied which is a 
	smarter algorithm, which is the Farey sequence. In fact it worked like a 
	charm on the newer STM32L432, but failed badly on the STM32F103, since PLL
	there is not flexible enough.
	Since this is a constexpr only compile time would benefit of an optimized 
	approach.
	*/
	static constexpr PllFraction ComputePllFraction(const uint32_t clk, const uint32_t fq)
	{
		PllFraction res = {
			.clk = clk,
			.fq = fq,
			.fin = 0,
			.fout = 0,
			.n = 0,
			.m = 1,
			.r = 1,			// unused member in this method
			.err = 100
		};
		PllFraction alt = res;
		double err_o = INFINITY;
		double err_a = INFINITY;
		uint32_t fout_a = 0;
		// Target PLL ratio
		const double x = double(fq) / double(clk);
		// Scan all multipliers first (favors power consumption)
		for (uint32_t n = kN_Min_; (err_o != 0.0) && (n <= kN_Max_); ++n)
		{
			// Scan all dividers
			for (uint32_t m = kM_Min_; (err_o != 0.0) && (m <= kM_Max_); ++m)
			{
				// VCO ratio
				const double v = double(n) / double(m);
				const double err = x >= v ? (x - v) : (v - x);
				// Effective VCO input frequency
				double fin = double(clk) / m;
				// Effective VCO output frequency
				double fout = fin * n;
				// Validate
				if (fin >= kVcoInMin_ && fin <= kVcoInMax_
					&& fout >= kVcoOutMin_ && fout <= kVcoOutMax_)
				{
					// Minimize errors
					if (err < err_o)
					{
						err_o = err;
						res.fin = uint32_t(fin + 0.5);
						res.fout = uint32_t(fout + 0.5);
						res.n = n;
						res.m = m;
					}
				}
				else
				{
					/*
					** This branch handles situation of over/under-clock
					** Just limit the frequency as per specs and consider it
					** as deviation error. at the end, record will store the
					** option with lowest chance to saturate PLL.
					*/
					// Force limit on frequencies
					if (fin > kVcoInMax_)
						fin = kVcoInMax_;
					else if (fin < kVcoInMin_)
						fin = kVcoInMin_;
					// The 'good' value
					double fgood = fin * n;
					if (fgood > kVcoOutMax_)
						fgood = kVcoOutMax_;
					else if (fgood < kVcoOutMin_)
						fgood = kVcoOutMin_;
					// Error is computed as the deviation from specs
					const double err =
						(fout >= fgood ? (fout - fgood) : (fgood - fout))
							/ double(clk)
						;
					if (err < err_a)
					{
						err_a = err;
						alt.fin = uint32_t(fin + 0.5);
						fout_a = alt.fout = uint32_t(fout + 0.5);
						alt.n = n;
						alt.m = m;
					}
				}
			}
		}
		// Special case when Over/under-clocking
		if (err_o == INFINITY)
		{
			if (fout_a > fq)
				alt.err = (100.0 * (fout_a - fq) / fq + 0.5) / x;
			else
				alt.err = (100.0 * (fq - fout_a) / fq + 0.5) / x;
			return alt;
		}
		res.err = (100.0 * err_o + 0.5) / x;	// relative error
		return res;
	}
};


}	// namespace Clocks
}	// namespace Bmt

#if defined(STM32L4)
#	include "clocks.l4.h"
#elif defined(STM32F1)
#	include "f1xx/clocks.h"
#else
#	error Unsupported target MCU
#endif

