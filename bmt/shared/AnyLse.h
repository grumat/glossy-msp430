#pragma once

namespace Bmt
{
namespace Clocks
{

enum class Id;

namespace Private
{


/// Template class for the LSE oscillator
template<
	const Id kClockId,
	const uint32_t kFrequency = 32768,	///< Frequency for the LSE clock
	const bool kBypass = false			///< LSE oscillator bypassed with external clock
>
class AnyLse
{
public:
	/// A constant for the Clock identification
	static constexpr Id kClockSource_ = kClockId;
	/// A constant for the frequency
	static constexpr uint32_t kFrequency_ = kFrequency;
	/// Actual oscillator that generates the clock
	static constexpr Id kClockInput_ = kClockSource_;

	/// Starts LSE oscillator within a backup domain transaction
	constexpr static void Init(BkpDomainXact &xact)
	{
		Enable(xact);
	}
	/// Enables the LSE oscillator within a backup domain transaction
	constexpr static void Enable(BkpDomainXact &)
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
	constexpr static void Disable(void)
	{
		RCC->BDCR &= ~RCC_BDCR_LSEON;
	}
};


}	// namespace Private
}	// namespace Clocks
}	// namespace Bmt
