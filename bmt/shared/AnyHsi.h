#pragma once

namespace Bmt
{
namespace Clocks
{

enum class Id;

namespace Private
{


template<
	const Id kClockId,
	const uint32_t kFrequency,
	const uint8_t kTrim
>
class AnyHsi
{
public:
	/// Clock identification
	static constexpr Id kClockSource_ = kClockId;
	/// Frequency of the clock source
	static constexpr uint32_t kFrequency_ = 16000000UL;
	/// Oscillator that generates the clock (not switchable in this particular case)
	static constexpr Id kClockInput_ = kClockSource_;

	/// Starts HSI16 oscillator
	constexpr static void Init(void)
	{
		Enable();
		// Apply calibration
		Trim(kTrim);
	}
	/// Enables the HSI oscillator
	constexpr static void Enable(void)
	{
		RCC->CR |= RCC_CR_HSION;
		while (!(RCC->CR & RCC_CR_HSIRDY));
	}
	/// Disables the HSI oscillator. You must ensure that associated peripherals are mapped elsewhere
	constexpr static void Disable(void)
	{
		RCC->CR &= ~RCC_CR_HSION;
	}
	/// Reads factory default calibration
	constexpr static uint8_t GetCal()
	{
#ifdef RCC_ICSCR_HSICAL_Pos
		return uint8_t((RCC->ICSCR & RCC_ICSCR_HSICAL_Msk) >> RCC_ICSCR_HSICAL_Pos);
#else
		return uint8_t((RCC->CR & RCC_CR_HSICAL_Msk) >> RCC_CR_HSICAL_Pos);
#endif
	}
	/// Reads current trim value
	constexpr static uint8_t Trim()
	{
#ifdef RCC_ICSCR_HSITRIM_Pos
		return (RCC->ICSCR & ~RCC_ICSCR_HSITRIM_Msk) >> RCC_ICSCR_HSITRIM_Pos;
#else
		return (RCC->CR & ~RCC_CR_HSITRIM_Msk) >> RCC_CR_HSITRIM_Pos;
#endif
	}
	/// Sets a new trim value
	constexpr static void Trim(const uint8_t v)
	{
#ifdef RCC_ICSCR_HSITRIM_Pos
		RCC->ICSCR = (RCC->ICSCR & ~RCC_ICSCR_HSITRIM_Msk) | (((uint32_t)v << RCC_ICSCR_HSITRIM_Pos) & RCC_ICSCR_HSITRIM_Msk);
#else
		RCC->CR = (RCC->CR & ~RCC_CR_HSITRIM_Msk) | ((v << RCC_CR_HSITRIM_Pos) & RCC_CR_HSITRIM_Msk);
#endif
	}
};


}	// namespace Private
}	// namespace Clocks
}	// namespace Bmt
