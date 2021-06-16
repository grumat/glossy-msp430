#pragma once

#include "mcu-system.h"


enum ExtiLine
{
	Exti0,
	Exti1,
	Exti2,
	Exti3,
	Exti4,
	Exti5,
	Exti6,
	Exti7,
	Exti8,
	Exti9,
	Exti10,
	Exti11,
	Exti12,
	Exti13,
	Exti14,
	Exti15,
	Exti16,
	Exti17,
	Exti18,

	ExtiPvd = Exti16,
	ExtiRtc = Exti17,
	ExtiUsb = Exti18,
};


enum ExtiMode
{
	kExtiSoft		//!< Software trigger
	, kExtiRising	//!< Trigger on rising edge
	, kExtiFalling	//!< Trigger on falling edge
	, kExtiBoth		//!< Trigger on both edges
};


class ExtiSourceUnused
{
public:
	static constexpr uint32_t kExtiBitValue = 0;
	static constexpr uint32_t kExtiTriggerRising = 0;
	static constexpr uint32_t kExtiTriggerFalling = 0;
	static constexpr uint32_t kExtiIntMask = 0;
	static constexpr uint32_t kExtiNvicIntMask0 = 0;
	static constexpr uint32_t kExtiNvicIntMask1 = 0;
	static constexpr uint32_t kExtiCR1 = 0;
	static constexpr uint32_t kExtiCR2 = 0;
	static constexpr uint32_t kExtiCR3 = 0;
	static constexpr uint32_t kExtiCR4 = 0;
	static constexpr uint32_t kExtiISER0 = 0;
	static constexpr uint32_t kExtiISER1 = 0;

	//! A placeholder
	ALWAYS_INLINE static void SetEvent()
	{
	}
};


template <
	const ExtiLine kExtiLine
	, const GpioPortId kPortId = kUnusedPort
	, const ExtiMode kExtiMode = kExtiSoft
	, const bool kInterrupt = false
>
class ExtiSource
{
public:
	static constexpr uint32_t kExtiSource = kExtiLine;
	static constexpr uint32_t kExtiBitValue = (1 << kExtiLine);
	static constexpr uint32_t kExtiTriggerRising = (kExtiMode == kExtiRising || kExtiMode == kExtiBoth) ? (1 << kExtiLine) : 0;
	static constexpr uint32_t kExtiTriggerFalling = (kExtiMode == kExtiFalling || kExtiMode == kExtiBoth) ? (1 << kExtiLine) : 0;
	static constexpr uint32_t kExtiIntMask = kInterrupt ? (1 << kExtiLine) : 0;
	static constexpr uint32_t kExtiNvicInt = (kExtiLine <= 4)
		? EXTI0_IRQn + kExtiLine
		: (kExtiLine <= 9)
			? EXTI9_5_IRQn
			: EXTI15_10_IRQn
		;
	static constexpr uint32_t kExtiNvicIntMask0 = (kInterrupt && kExtiLine <= 9) ? (1 << kExtiNvicInt) : 0;
	static constexpr uint32_t kExtiNvicIntMask1 = (kInterrupt && kExtiLine >= 10) ? (1 << (kExtiNvicInt & 0x1FUL)) : 0;
	static constexpr uint32_t kExtiCR1 = kExtiLine <= 3 && kPortId != kUnusedPort
		? (kPortId << kExtiLine) : 0;
	static constexpr uint32_t kExtiCR2 = kExtiLine >= 4 && kExtiLine <= 7 && kPortId != kUnusedPort
		? (kPortId << (kExtiLine - 4)) : 0;
	static constexpr uint32_t kExtiCR3 = kExtiLine >= 8 && kExtiLine <= 11 && kPortId != kUnusedPort
		? (kPortId << (kExtiLine - 8)) : 0;
	static constexpr uint32_t kExtiCR4 = kExtiLine >= 12 && kExtiLine <= 15 && kPortId != kUnusedPort
		? (kPortId << (kExtiLine - 12)) : 0;
	static constexpr uint32_t kExtiCR1_Mask = kExtiLine <= 3 && kPortId != kUnusedPort
		? (0xF << kExtiLine) : 0;
	static constexpr uint32_t kExtiCR2_Mask = kExtiLine >= 4 && kExtiLine <= 7 && kPortId != kUnusedPort
		? (0xF << (kExtiLine - 4)) : 0;
	static constexpr uint32_t kExtiCR3_Mask = kExtiLine >= 8 && kExtiLine <= 11 && kPortId != kUnusedPort
		? (0xF << (kExtiLine - 8)) : 0;
	static constexpr uint32_t kExtiCR4_Mask = kExtiLine >= 12 && kExtiLine <= 15 && kPortId != kUnusedPort
		? (0xF << (kExtiLine - 12)) : 0;

	//! Applies settings to an already initialized EXTI
	ALWAYS_INLINE static void Enable(void)
	{
		if(kExtiTriggerRising)
			EXTI->RTSR |= kExtiTriggerRising;
		if (kExtiTriggerFalling)
			EXTI->FTSR = kExtiTriggerFalling;
		if (kExtiIntMask)
			EXTI->IMR = kExtiIntMask;
		if (kExtiNvicIntMask0)
			NVIC->ISER[0] |= kExtiNvicIntMask0;
		if (kExtiNvicIntMask1)
			NVIC->ISER[1] |= kExtiNvicIntMask1;
		if (kExtiCR1_Mask)
			AFIO->EXTICR[0] = (AFIO->EXTICR[0] & kExtiCR1_Mask) | kExtiCR1;
		if (kExtiCR2_Mask)
			AFIO->EXTICR[1] = (AFIO->EXTICR[1] & kExtiCR2_Mask) | kExtiCR2;
		if (kExtiCR3_Mask)
			AFIO->EXTICR[2] = (AFIO->EXTICR[2] & kExtiCR3_Mask) | kExtiCR3;
		if (kExtiCR4_Mask)
			AFIO->EXTICR[3] = (AFIO->EXTICR[3] & kExtiCR4_Mask) | kExtiCR4;
	}
	//! Disables all settings for the EXTI
	ALWAYS_INLINE static void Disable()
	{
		if (kExtiTriggerRising)
			EXTI->RTSR &= ~kExtiTriggerRising;
		if (kExtiTriggerFalling)
			EXTI->FTSR &= ~kExtiTriggerFalling;
		if (kExtiIntMask)
			EXTI->IMR &= kExtiIntMask;
		if (kExtiNvicIntMask0)
			NVIC->ICER[0] |= kExtiNvicIntMask0;
		if (kExtiNvicIntMask1)
			NVIC->ICER[1] |= kExtiNvicIntMask1;
	}
	//! Clear IRQ pending flags
	ALWAYS_INLINE static void ClearIrq(void)
	{
		EXTI->PR = kExtiBitValue;
		if (kExtiNvicIntMask0)
			NVIC->ICPR[0] |= kExtiNvicIntMask0;
		if (kExtiNvicIntMask1)
			NVIC->ICPR[1] |= kExtiNvicIntMask1;
	}
	//! Suspends IRQ on the NVIC
	ALWAYS_INLINE static void SupendIrq(void)
	{
		if (kExtiNvicIntMask0)
			NVIC->ICER[0] |= kExtiNvicIntMask0;
		if (kExtiNvicIntMask1)
			NVIC->ICER[1] |= kExtiNvicIntMask1;
	}
	//! Resumes IRQ on the NVIC
	ALWAYS_INLINE static void ResumeIrq(void)
	{
		if (kExtiNvicIntMask0)
			NVIC->ISER[0] |= kExtiNvicIntMask0;
		if (kExtiNvicIntMask1)
			NVIC->ISER[1] |= kExtiNvicIntMask1;
	}
	//! Issues a software event
	ALWAYS_INLINE static void SetEvent()
	{
		EXTI->SWIER |= kExtiBitValue;
	}
	//! Clears the pending interrupt
};


template<
	typename Source0
	, typename Source1 = ExtiSourceUnused
	, typename Source2 = ExtiSourceUnused
	, typename Source3 = ExtiSourceUnused
	, typename Source4 = ExtiSourceUnused
	, typename Source5 = ExtiSourceUnused
	, typename Source6 = ExtiSourceUnused
	, typename Source7 = ExtiSourceUnused
	, typename Source8 = ExtiSourceUnused
	, typename Source9 = ExtiSourceUnused
	, typename Source10 = ExtiSourceUnused
	, typename Source11 = ExtiSourceUnused
	, typename Source12 = ExtiSourceUnused
	, typename Source13 = ExtiSourceUnused
	, typename Source14 = ExtiSourceUnused
	, typename Source15 = ExtiSourceUnused
	, typename Source16 = ExtiSourceUnused
	, typename Source17 = ExtiSourceUnused
	, typename Source18 = ExtiSourceUnused
>
class ExtiTemplate
{
public:
	static constexpr uint32_t kExtiBitValue =
		Source0::kExtiBitValue | Source1::kExtiBitValue | Source2::kExtiBitValue | Source3::kExtiBitValue
		| Source4::kExtiBitValue | Source5::kExtiBitValue | Source6::kExtiBitValue | Source7::kExtiBitValue
		| Source8::kExtiBitValue | Source9::kExtiBitValue | Source10::kExtiBitValue | Source11::kExtiBitValue
		| Source12::kExtiBitValue | Source13::kExtiBitValue | Source14::kExtiBitValue | Source15::kExtiBitValue
		| Source16::kExtiBitValue | Source17::kExtiBitValue | Source18::kExtiBitValue
		;
	static constexpr uint32_t kExtiTriggerRising =
		Source0::kExtiTriggerRising | Source1::kExtiTriggerRising | Source2::kExtiTriggerRising | Source3::kExtiTriggerRising
		| Source4::kExtiTriggerRising | Source5::kExtiTriggerRising | Source6::kExtiTriggerRising | Source7::kExtiTriggerRising
		| Source8::kExtiTriggerRising | Source9::kExtiTriggerRising | Source10::kExtiTriggerRising | Source11::kExtiTriggerRising
		| Source12::kExtiTriggerRising | Source13::kExtiTriggerRising | Source14::kExtiTriggerRising | Source15::kExtiTriggerRising
		| Source16::kExtiTriggerRising | Source17::kExtiTriggerRising | Source18::kExtiTriggerRising
		;
	static constexpr uint32_t kExtiTriggerFalling =
		Source0::kExtiTriggerFalling | Source1::kExtiTriggerFalling | Source2::kExtiTriggerFalling | Source3::kExtiTriggerFalling
		| Source4::kExtiTriggerFalling | Source5::kExtiTriggerFalling | Source6::kExtiTriggerFalling | Source7::kExtiTriggerFalling
		| Source8::kExtiTriggerFalling | Source9::kExtiTriggerFalling | Source10::kExtiTriggerFalling | Source11::kExtiTriggerFalling
		| Source12::kExtiTriggerFalling | Source13::kExtiTriggerFalling | Source14::kExtiTriggerFalling | Source15::kExtiTriggerFalling
		| Source16::kExtiTriggerFalling | Source17::kExtiTriggerFalling | Source18::kExtiTriggerFalling
		;
	static constexpr uint32_t kExtiIntMask =
		Source0::kExtiIntMask | Source1::kExtiIntMask | Source2::kExtiIntMask | Source3::kExtiIntMask
		| Source4::kExtiIntMask | Source5::kExtiIntMask | Source6::kExtiIntMask | Source7::kExtiIntMask
		| Source8::kExtiIntMask | Source9::kExtiIntMask | Source10::kExtiIntMask | Source11::kExtiIntMask
		| Source12::kExtiIntMask | Source13::kExtiIntMask | Source14::kExtiIntMask | Source15::kExtiIntMask
		| Source16::kExtiIntMask | Source17::kExtiIntMask | Source18::kExtiIntMask
		;
	static constexpr uint32_t kExtiNvicIntMask0 =
		Source0::kExtiNvicIntMask0 | Source1::kExtiNvicIntMask0 | Source2::kExtiNvicIntMask0 | Source3::kExtiNvicIntMask0
		| Source4::kExtiNvicIntMask0 | Source5::kExtiNvicIntMask0 | Source6::kExtiNvicIntMask0 | Source7::kExtiNvicIntMask0
		| Source8::kExtiNvicIntMask0 | Source9::kExtiNvicIntMask0 | Source10::kExtiNvicIntMask0 | Source11::kExtiNvicIntMask0
		| Source12::kExtiNvicIntMask0 | Source13::kExtiNvicIntMask0 | Source14::kExtiNvicIntMask0 | Source15::kExtiNvicIntMask0
		| Source16::kExtiNvicIntMask0 | Source17::kExtiNvicIntMask0 | Source18::kExtiNvicIntMask0
		;
	static constexpr uint32_t kExtiNvicIntMask1 =
		Source0::kExtiNvicIntMask1 | Source1::kExtiNvicIntMask1 | Source2::kExtiNvicIntMask1 | Source3::kExtiNvicIntMask1
		| Source4::kExtiNvicIntMask1 | Source5::kExtiNvicIntMask1 | Source6::kExtiNvicIntMask1 | Source7::kExtiNvicIntMask1
		| Source8::kExtiNvicIntMask1 | Source9::kExtiNvicIntMask1 | Source10::kExtiNvicIntMask1 | Source11::kExtiNvicIntMask1
		| Source12::kExtiNvicIntMask1 | Source13::kExtiNvicIntMask1 | Source14::kExtiNvicIntMask1 | Source15::kExtiNvicIntMask1
		| Source16::kExtiNvicIntMask1 | Source17::kExtiNvicIntMask1 | Source18::kExtiNvicIntMask1
		;
	static constexpr uint32_t kExtiCR1 =
		Source0::kExtiCR1 | Source1::kExtiCR1 | Source2::kExtiCR1 | Source3::kExtiCR1
		| Source4::kExtiCR1 | Source5::kExtiCR1 | Source6::kExtiCR1 | Source7::kExtiCR1
		| Source8::kExtiCR1 | Source9::kExtiCR1 | Source10::kExtiCR1 | Source11::kExtiCR1
		| Source12::kExtiCR1 | Source13::kExtiCR1 | Source14::kExtiCR1 | Source15::kExtiCR1
		| Source16::kExtiCR1 | Source17::kExtiCR1 | Source18::kExtiCR1
		;
	static constexpr uint32_t kExtiCR2 =
		Source0::kExtiCR2 | Source1::kExtiCR2 | Source2::kExtiCR2 | Source3::kExtiCR2
		| Source4::kExtiCR2 | Source5::kExtiCR2 | Source6::kExtiCR2 | Source7::kExtiCR2
		| Source8::kExtiCR2 | Source9::kExtiCR2 | Source10::kExtiCR2 | Source11::kExtiCR2
		| Source12::kExtiCR2 | Source13::kExtiCR2 | Source14::kExtiCR2 | Source15::kExtiCR2
		| Source16::kExtiCR2 | Source17::kExtiCR2 | Source18::kExtiCR2
		;
	static constexpr uint32_t kExtiCR3 =
		Source0::kExtiCR3 | Source1::kExtiCR3 | Source2::kExtiCR3 | Source3::kExtiCR3
		| Source4::kExtiCR3 | Source5::kExtiCR3 | Source6::kExtiCR3 | Source7::kExtiCR3
		| Source8::kExtiCR3 | Source9::kExtiCR3 | Source10::kExtiCR3 | Source11::kExtiCR3
		| Source12::kExtiCR3 | Source13::kExtiCR3 | Source14::kExtiCR3 | Source15::kExtiCR3
		| Source16::kExtiCR3 | Source17::kExtiCR3 | Source18::kExtiCR3
		;
	static constexpr uint32_t kExtiCR4 =
		Source0::kExtiCR4 | Source1::kExtiCR4 | Source2::kExtiCR4 | Source3::kExtiCR4
		| Source4::kExtiCR4 | Source5::kExtiCR4 | Source6::kExtiCR4 | Source7::kExtiCR4
		| Source8::kExtiCR4 | Source9::kExtiCR4 | Source10::kExtiCR4 | Source11::kExtiCR4
		| Source12::kExtiCR4 | Source13::kExtiCR4 | Source14::kExtiCR4 | Source15::kExtiCR4
		| Source16::kExtiCR4 | Source17::kExtiCR4 | Source18::kExtiCR4
		;

	//! Starts module clock and enables configuration
	ALWAYS_INLINE static void Init(void)
	{
		RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
		Enable();
	}
	//! Applies settings to an already initialized EXTI
	ALWAYS_INLINE static void Enable(void)
	{
		EXTI->RTSR = kExtiTriggerRising;
		EXTI->FTSR = kExtiTriggerFalling;
		EXTI->IMR = kExtiIntMask;
		if (kExtiNvicIntMask0)
			NVIC->ISER[0] |= kExtiNvicIntMask0;
		if (kExtiNvicIntMask1)
			NVIC->ISER[1] |= kExtiNvicIntMask1;
		AFIO->EXTICR[0] = kExtiCR1;
		AFIO->EXTICR[1] = kExtiCR2;
		AFIO->EXTICR[2] = kExtiCR3;
		AFIO->EXTICR[3] = kExtiCR4;
	}
	//! Disables all settings for the EXTI
	ALWAYS_INLINE static void Disable(const bool kDeInit = false)
	{
		if (kDeInit)
			RCC->APB2ENR &= ~RCC_APB2ENR_AFIOEN;
		if (kExtiTriggerRising)
			EXTI->RTSR &= ~kExtiTriggerRising;
		if (kExtiTriggerFalling)
			EXTI->FTSR &= ~kExtiTriggerFalling;
		if(kExtiIntMask)
			EXTI->IMR &= kExtiIntMask;
		if (kExtiNvicIntMask0)
			NVIC->ICER[0] |= kExtiNvicIntMask0;
		if (kExtiNvicIntMask1)
			NVIC->ICER[1] |= kExtiNvicIntMask1;
	}
	//! Clear IRQ pending flags
	ALWAYS_INLINE static void ClearAllIrqs(void)
	{
		EXTI->PR = kExtiBitValue;
		if (kExtiNvicIntMask0)
			NVIC->ICPR[0] |= kExtiNvicIntMask0;
		if (kExtiNvicIntMask1)
			NVIC->ICPR[1] |= kExtiNvicIntMask1;
	}
	//! Suspends IRQ on the NVIC
	ALWAYS_INLINE static void SupendIrqs(void)
	{
		if (kExtiNvicIntMask0)
			NVIC->ICER[0] |= kExtiNvicIntMask0;
		if (kExtiNvicIntMask1)
			NVIC->ICER[1] |= kExtiNvicIntMask1;
	}
	//! Resumes IRQ on the NVIC
	ALWAYS_INLINE static void ResumeIrqs(void)
	{
		if (kExtiNvicIntMask0)
			NVIC->ISER[0] |= kExtiNvicIntMask0;
		if (kExtiNvicIntMask1)
			NVIC->ISER[1] |= kExtiNvicIntMask1;
	}
};

