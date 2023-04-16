#pragma once

namespace Bmt
{
namespace Clocks
{

enum class Id;

namespace Private
{


/// Class for the LSI oscillator
template <
	const Id kClockId,
	const uint32_t kFrequency
>
class AnyLsi
{
public:
	/// A constant for the Clock identification
	static constexpr Id kClockSource_ = kClockId;
	/// A constant for the frequency
	static constexpr uint32_t kFrequency_ = 40000;
	/// Actual oscillator that generates the clock (40 kHz)
	static constexpr Id kClockInput_ = kClockSource_;

	/// Starts LSI oscillator
	constexpr static void Init(void)
	{
		Enable();
	}
	/// Enables the LSI oscillator
	constexpr static void Enable(void)
	{
		RCC->CSR |= RCC_CSR_LSION;
		while (!(RCC->CSR & RCC_CSR_LSIRDY));
	}
	/// Disables the LSI oscillator. You must ensure that associated peripherals are mapped elsewhere
	constexpr static void Disable(void)
	{
		RCC->CSR &= ~RCC_CSR_LSION;
	}
};


}	// namespace Private
}	// namespace Clocks
}	// namespace Bmt
