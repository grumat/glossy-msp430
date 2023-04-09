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
		double err_o = INFINITY;
		// Target PLL ratio
		const double x = double(fq) / double(clk);
		// Scan all multipliers first (favors power consumption)
		for (uint32_t n = kN_Min_; (err_o != 0.0) && (n <= kN_Max_); ++n)
		{
			// Scan all dividers
			for (uint32_t m = kM_Min_; (err_o != 0.0) && (m <= kM_Max_); ++m)
			{
				// Effective VCO input frequency
				const double fin = double(clk) / m;
				// Validate
				if (fin >= kVcoInMin_ && fin <= kVcoInMax_)
				{
					// Effective VCO output frequency
					const double fout = fin * n;
					if (fout >= kVcoOutMin_ && fout <= kVcoOutMax_)
					{
						const double v = double(n) / double(m);
						const double err = x >= v ? (x - v) : (v - x);
						if (err < err_o)
						{
							err_o = err;
							res.fin = uint32_t(fin);
							res.fout = uint32_t(fout);
							res.n = n;
							res.m = m;
						}
					}
				}
			}
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
#	include "clocks.f1.h"
#else
#	error Unsupported target MCU
#endif

