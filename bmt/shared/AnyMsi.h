#pragma once

#ifdef RCC_CR_MSION

namespace Bmt
{
namespace Clocks
{

enum class Id;


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


namespace Private
{


/// Class for the AnyMsi clock source
template<
	const Id kClockId,
	const MsiFreq kFreqCR = MsiFreq::k4_MHz,	///< Frequency of oscillator
	const bool kPllMode = false	///< Don't touch; use MsiPll template (see below)
>
class AnyMsi
{
public:
	/// Clock identification
	static constexpr Id kClockSource_ = kClockId;
	/// Configuration register setup
	static constexpr MsiFreq kCrEnum_ = (kFreqCR < MsiFreq::kMax) ? kFreqCR : MsiFreq::kMax;
	/// Frequency of the clock source
	static constexpr uint32_t kFrequency_ =
		kCrEnum_ == MsiFreq::k100_kHz ? 100000UL
		: kCrEnum_ == MsiFreq::k200_kHz ? 200000UL
		: kCrEnum_ == MsiFreq::k400_kHz ? 400000UL
		: kCrEnum_ == MsiFreq::k800_kHz ? 800000UL
		: kCrEnum_ == MsiFreq::k1_MHz ? 1000000UL
		: kCrEnum_ == MsiFreq::k2_MHz ? 2000000UL
		: kCrEnum_ == MsiFreq::k4_MHz ? 4000000UL
		: kCrEnum_ == MsiFreq::k8_MHz ? 8000000UL
		: kCrEnum_ == MsiFreq::k16_MHz ? 16000000UL
		: kCrEnum_ == MsiFreq::k24_MHz ? 24000000UL
		: kCrEnum_ == MsiFreq::k32_MHz ? 32000000UL
		: kCrEnum_ == MsiFreq::k48_MHz ? 48000000UL
		: 1;
	/// Oscillator that generates the clock (not switchable in this particular case)
	static constexpr Id kClockInput_ = kClockSource_;
	/// Special Configuration for MsiPll class
	static constexpr bool kUsePllMode_ = kPllMode;

	/// Starts MSI oscillator
	constexpr static void Init(void)
	{
		static_assert(kFrequency_ > 1, "Hardware does not supports specified frequency");
		Init();
	}
	/// Enables the MSI oscillator
	constexpr static void Enable(void)
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
	constexpr static void Disable(void)
	{
		RCC->CR &= ~(RCC_CR_MSION_Msk | RCC_CR_MSIPLLEN_Msk);
	}
	/// Reads factory default calibration
	constexpr static uint8_t GetCal()
	{
		return uint8_t((RCC->ICSCR & RCC_ICSCR_MSICAL_Msk) >> RCC_ICSCR_MSICAL_Pos);
	}
	/// Reads current trim value
	constexpr static uint8_t Trim()
	{
		return (RCC->ICSCR & ~RCC_ICSCR_MSITRIM_Msk) >> RCC_ICSCR_MSITRIM_Pos;
	}
	/// Sets a new trim value
	constexpr static void Trim(const uint8_t v)
	{
		RCC->ICSCR = (RCC->ICSCR & ~RCC_ICSCR_MSITRIM_Msk) | ((v << RCC_ICSCR_MSITRIM_Pos) & RCC_ICSCR_MSITRIM_Msk);
	}
};


}	// namespace Private
}	// namespace Clocks
}	// namespace Bmt

#endif // RCC_CR_MSION
