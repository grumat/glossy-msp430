#pragma once

namespace Bmt
{
namespace Clocks
{

enum class Id;

namespace Private
{


/// Template class for the HSE clock source
template<
	const Id kClockId,
	const uint32_t kFrequency,	///< Frequency of oscillator
	const bool kBypass,			///< Low pin count devices have only CK input
	const bool kCssEnabled		///< Clock Security System enable
>
class AnyHse
{
public:
	/// A constant for the Clock identification
	static constexpr Id kClockSource_ = kClockId;
	/// A constant for the frequency
	static constexpr uint32_t kFrequency_ = kFrequency;
	/// Actual oscillator that generates the clock
	static constexpr Id kClockInput_ = kClockSource_;
	/// In low pin count devices only external clock is available
	static constexpr bool kBypass_ = kBypass;

	/// Starts HSE oscillator
	constexpr static void Init()
	{
		Enable();
	}
	/// Enables the HSE oscillator
	constexpr static void Enable()
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
	constexpr static void Disable()
	{
		uint32_t tmp = ~RCC_CR_HSEON;
		if (kCssEnabled)
			tmp &= ~RCC_CR_CSSON;
		RCC->CR &= tmp;
	}
};


}	// namespace Private
}	// namespace Clocks
}	// namespace Bmt
