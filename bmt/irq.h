#pragma once


//! A template class for a controllable IRQ event
template <const IRQn_Type kIrq>
class IrqTemplate
{
public:
	static constexpr uint32_t mask0_ = (kIrq >= 0 && kIrq < 32) ? (1 << kIrq) : 0;
	static constexpr uint32_t mask1_ = (kIrq >= 32 && kIrq < 64) ? (1 << (kIrq - 32)) : 0;

	ALWAYS_INLINE static void Enable()
	{
		if (mask0_)
			NVIC->ISER[0U] = mask0_;
		if (mask1_)
			NVIC->ISER[1U] = mask1_;
	}
	ALWAYS_INLINE static void Disable()
	{
		if (mask0_)
			NVIC->ICER[0U] = mask0_;
		if (mask1_)
			NVIC->ICER[1U] = mask1_;
	}
	ALWAYS_INLINE static void ClearPending()
	{
		if (mask0_)
			NVIC->ICPR[0U] = mask0_;
		if (mask1_)
			NVIC->ICPR[1U] = mask1_;
	}
};


/*!
Groups a set of interrupt to enable or disable in a set.
*/
template <
	typename kIrq1
	, typename kIrq2 = IrqTemplate<SysTick_IRQn>
	, typename kIrq3 = IrqTemplate<SysTick_IRQn>
	, typename kIrq4 = IrqTemplate<SysTick_IRQn>
	, typename kIrq5 = IrqTemplate<SysTick_IRQn>
	, typename kIrq6 = IrqTemplate<SysTick_IRQn>
	, typename kIrq7 = IrqTemplate<SysTick_IRQn>
	, typename kIrq8 = IrqTemplate<SysTick_IRQn>
	, typename kIrq9 = IrqTemplate<SysTick_IRQn>
	, typename kIrq10 = IrqTemplate<SysTick_IRQn>
	, typename kIrq11 = IrqTemplate<SysTick_IRQn>
	, typename kIrq12 = IrqTemplate<SysTick_IRQn>
	, typename kIrq13 = IrqTemplate<SysTick_IRQn>
	, typename kIrq14 = IrqTemplate<SysTick_IRQn>
	, typename kIrq15 = IrqTemplate<SysTick_IRQn>
	, typename kIrq16 = IrqTemplate<SysTick_IRQn>
>
class IrqSet
{
public:
	static constexpr uint32_t mask0_ =
		kIrq1::mask0_ | kIrq2::mask0_ | kIrq3::mask0_ | kIrq4::mask0_
		| kIrq5::mask0_ | kIrq6::mask0_ | kIrq7::mask0_ | kIrq8::mask0_
		| kIrq9::mask0_ | kIrq10::mask0_ | kIrq11::mask0_ | kIrq12::mask0_
		| kIrq13::mask0_ | kIrq14::mask0_ | kIrq15::mask0_ | kIrq16::mask0_
		;
	static constexpr uint32_t mask1_ =
		kIrq1::mask1_ | kIrq2::mask1_ | kIrq3::mask1_ | kIrq4::mask1_
		| kIrq5::mask1_ | kIrq6::mask1_ | kIrq7::mask1_ | kIrq8::mask1_
		| kIrq9::mask1_ | kIrq10::mask1_ | kIrq11::mask1_ | kIrq12::mask1_
		| kIrq13::mask1_ | kIrq14::mask1_ | kIrq15::mask1_ | kIrq16::mask1_
		;

	ALWAYS_INLINE static void Enable()
	{
		if (mask0_)
			NVIC->ISER[0U] = mask0_;
		if (mask1_)
			NVIC->ISER[1U] = mask1_;
	}
	ALWAYS_INLINE static void Disable()
	{
		if (mask0_)
			NVIC->ICER[0U] = mask0_;
		if (mask1_)
			NVIC->ICER[1U] = mask1_;
	}
};
