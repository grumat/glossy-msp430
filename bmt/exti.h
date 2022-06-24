#pragma once

#include "mcu-system.h"


/// Enumeration for all EXTI input signals
enum ExtiLine : uint32_t
{
	Exti0,				///< EXTI line 0
	Exti1,				///< EXTI line 1
	Exti2,				///< EXTI line 2
	Exti3,				///< EXTI line 3
	Exti4,				///< EXTI line 4
	Exti5,				///< EXTI line 5
	Exti6,				///< EXTI line 6
	Exti7,				///< EXTI line 7
	Exti8,				///< EXTI line 8
	Exti9,				///< EXTI line 9
	Exti10,				///< EXTI line 10
	Exti11,				///< EXTI line 11
	Exti12,				///< EXTI line 12
	Exti13,				///< EXTI line 13
	Exti14,				///< EXTI line 14
	Exti15,				///< EXTI line 15
	Exti16,				///< EXTI line 16
	Exti17,				///< EXTI line 17
	Exti18,				///< EXTI line 18

	ExtiPvd = Exti16,	///< EXTI PVD line, shared with line 16
	ExtiRtc = Exti17,	///< EXTI RTC line, shared with line 17
	ExtiUsb = Exti18,	///< EXTI USB line, shared with line 18
};


/// Enumeration for EXTI trigger mode
enum ExtiMode
{
	kExtiSoft		///< Software trigger
	, kExtiRising	///< Trigger on rising edge
	, kExtiFalling	///< Trigger on falling edge
	, kExtiBoth		///< Trigger on both edges
};


/// A bogus interrupt line with no configuration effect, just used as default template parameter filler
class ExtiSourceUnused
{
public:
	/// Affected bit mask constant (no effect)
	static constexpr uint32_t kExtiBitValue = 0;
	/// Constant for RTSR (Rising trigger selection register) (no effect)
	static constexpr uint32_t kExtiTriggerRising = 0;
	/// Constant for FTSR (Falling trigger selection register) (no effect)
	static constexpr uint32_t kExtiTriggerFalling = 0;
	/// Constant for Interrupt mask register (no effect)
	static constexpr uint32_t kExtiIntMask = 0;
	/// Constant mask value for Interrupt set-enable registers 0 on NVIC
	static constexpr uint32_t kExtiNvicIntMask0 = 0;
	/// Constant mask value for Interrupt set-enable registers 1 on NVIC
	static constexpr uint32_t kExtiNvicIntMask1 = 0;
	/// Constant bit value to External interrupt configuration register 1
	static constexpr uint32_t kExtiCR1 = 0;
	/// Constant bit value to External interrupt configuration register 2
	static constexpr uint32_t kExtiCR2 = 0;
	/// Constant bit value to External interrupt configuration register 3
	static constexpr uint32_t kExtiCR3 = 0;
	/// Constant bit value to External interrupt configuration register 4
	static constexpr uint32_t kExtiCR4 = 0;
	/// Constant mask value (4 bit-group) to External interrupt configuration register 1
	static constexpr uint32_t kExtiCR1_Mask = 0;
	/// Constant mask value (4 bit-group) to External interrupt configuration register 2
	static constexpr uint32_t kExtiCR2_Mask = 0;
	/// Constant mask value (4 bit-group) to External interrupt configuration register 3
	static constexpr uint32_t kExtiCR3_Mask = 0;
	/// Constant mask value (4 bit-group) to External interrupt configuration register 4
	static constexpr uint32_t kExtiCR4_Mask = 0;

	/// A placeholder (stripped out by compiler)
	ALWAYS_INLINE static void Enable()
	{}
	/// A placeholder (stripped out by compiler)
	ALWAYS_INLINE static void Disable()
	{}
	/// A placeholder (stripped out by compiler)
	ALWAYS_INLINE static void ClearIrq()
	{}
	/// A placeholder (stripped out by compiler)
	ALWAYS_INLINE static void SupendIrq(void)
	{}
	/// A placeholder (stripped out by compiler)
	ALWAYS_INLINE static void ResumeIrq(void)
	{}
	/// A placeholder (stripped out by compiler)
	ALWAYS_INLINE static void SetEvent()
	{}
};


/// A template class for single EXTI line configuration
template <
	const ExtiLine kExtiLine					///< The EXTI line
	, const GpioPortId kPortId = kUnusedPort	///< The port that triggers the EXTI interrupt
	, const ExtiMode kExtiMode = kExtiSoft		///< 
	, const bool kInterrupt = false
>
class ExtiSource
{
public:
	/// Enumeration constant for this particular EXTI line
	static constexpr ExtiLine kExtiSource = kExtiLine;
	/// Affected bit mask constant
	static constexpr uint32_t kExtiBitValue = (1 << kExtiLine);
	/// Constant for RTSR (Rising trigger selection register)
	static constexpr uint32_t kExtiTriggerRising = (kExtiMode == kExtiRising || kExtiMode == kExtiBoth) ? (1 << kExtiLine) : 0;
	/// Constant for FTSR (Falling trigger selection register)
	static constexpr uint32_t kExtiTriggerFalling = (kExtiMode == kExtiFalling || kExtiMode == kExtiBoth) ? (1 << kExtiLine) : 0;
	/// Constant for Interrupt mask register
	static constexpr uint32_t kExtiIntMask = kInterrupt ? (1 << kExtiLine) : 0;
	/// Constant for interrupt line in NVIC controller
	static constexpr uint32_t kExtiNvicInt = (kExtiLine <= 4)
		? EXTI0_IRQn + kExtiLine
		: (kExtiLine <= 9)
			? EXTI9_5_IRQn
			: EXTI15_10_IRQn
		;
	/// Constant mask value for Interrupt set-enable registers 0 on NVIC
	static constexpr uint32_t kExtiNvicIntMask0 = (kInterrupt && kExtiLine <= 9) ? (1 << kExtiNvicInt) : 0;
	/// Constant mask value for Interrupt set-enable registers 1 on NVIC
	static constexpr uint32_t kExtiNvicIntMask1 = (kInterrupt && kExtiLine >= 10) ? (1 << (kExtiNvicInt & 0x1FUL)) : 0;
	/// Constant bit value to External interrupt configuration register 1
	static constexpr uint32_t kExtiCR1 = kExtiLine <= 3 && kPortId != kUnusedPort
		? (kPortId << (4*kExtiLine)) : 0;
	/// Constant bit value to External interrupt configuration register 2
	static constexpr uint32_t kExtiCR2 = kExtiLine >= 4 && kExtiLine <= 7 && kPortId != kUnusedPort
		? (kPortId << (4*(kExtiLine - 4))) : 0;
	/// Constant bit value to External interrupt configuration register 3
	static constexpr uint32_t kExtiCR3 = kExtiLine >= 8 && kExtiLine <= 11 && kPortId != kUnusedPort
		? (kPortId << (4*(kExtiLine - 8))) : 0;
	/// Constant bit value to External interrupt configuration register 4
	static constexpr uint32_t kExtiCR4 = kExtiLine >= 12 && kExtiLine <= 15 && kPortId != kUnusedPort
		? (kPortId << (4*(kExtiLine - 12))) : 0;
	/// Combined constant mask value (4 bit-group) to External interrupt configuration register 1
	static constexpr uint32_t kExtiCR1_Mask = kExtiLine <= 3 && kPortId != kUnusedPort
		? (0xF << (4*kExtiLine)) : 0;
	/// Combined constant mask value (4 bit-group) to External interrupt configuration register 2
	static constexpr uint32_t kExtiCR2_Mask = kExtiLine >= 4 && kExtiLine <= 7 && kPortId != kUnusedPort
		? (0xF << (4*(kExtiLine - 4))) : 0;
	/// Combined constant mask value (4 bit-group) to External interrupt configuration register 3
	static constexpr uint32_t kExtiCR3_Mask = kExtiLine >= 8 && kExtiLine <= 11 && kPortId != kUnusedPort
		? (0xF << (4*(kExtiLine - 8))) : 0;
	/// Combined constant mask value (4 bit-group) to External interrupt configuration register 4
	static constexpr uint32_t kExtiCR4_Mask = kExtiLine >= 12 && kExtiLine <= 15 && kPortId != kUnusedPort
		? (0xF << (4*(kExtiLine - 12))) : 0;

	/// Applies settings to an already initialized EXTI
	ALWAYS_INLINE static void Enable()
	{
		// Constant for Rising trigger selection register
		if(kExtiTriggerRising)
			EXTI->RTSR |= kExtiTriggerRising;
		// Constant for Falling trigger selection register
		if (kExtiTriggerFalling)
			EXTI->FTSR |= kExtiTriggerFalling;
		// Constant for Interrupt mask register
		if (kExtiIntMask)
			EXTI->IMR = kExtiIntMask;
		// Apply constant mask value for Interrupt set-enable registers 0 on NVIC
		if (kExtiNvicIntMask0)
			NVIC->ISER[0] |= kExtiNvicIntMask0;
		// Apply constant mask value for Interrupt set-enable registers 1 on NVIC
		if (kExtiNvicIntMask1)
			NVIC->ISER[1] |= kExtiNvicIntMask1;
		// Apply constant mask value to External interrupt configuration register 1
		if (kExtiCR1_Mask)
			AFIO->EXTICR[0] = (AFIO->EXTICR[0] & ~kExtiCR1_Mask) | kExtiCR1;
		// Apply constant mask value to External interrupt configuration register 2
		if (kExtiCR2_Mask)
			AFIO->EXTICR[1] = (AFIO->EXTICR[1] & ~kExtiCR2_Mask) | kExtiCR2;
		// Apply constant mask value to External interrupt configuration register 3
		if (kExtiCR3_Mask)
			AFIO->EXTICR[2] = (AFIO->EXTICR[2] & ~kExtiCR3_Mask) | kExtiCR3;
		// Apply constant mask value to External interrupt configuration register 4
		if (kExtiCR4_Mask)
			AFIO->EXTICR[3] = (AFIO->EXTICR[3] & ~kExtiCR4_Mask) | kExtiCR4;
	}
	/// Disables all settings for the EXTI
	ALWAYS_INLINE static void Disable()
	{
		// Constant for Rising trigger selection register
		if (kExtiTriggerRising)
			EXTI->RTSR &= ~kExtiTriggerRising;
		// Constant for Falling trigger selection register
		if (kExtiTriggerFalling)
			EXTI->FTSR &= ~kExtiTriggerFalling;
		// Constant for Interrupt mask register
		if (kExtiIntMask)
			EXTI->IMR &= kExtiIntMask;
		// Apply constant mask value for Interrupt set-enable registers 0 on NVIC
		if (kExtiNvicIntMask0)
			NVIC->ICER[0] |= kExtiNvicIntMask0;
		// Apply constant mask value for Interrupt set-enable registers 1 on NVIC
		if (kExtiNvicIntMask1)
			NVIC->ICER[1] |= kExtiNvicIntMask1;
	}
	/// Clear IRQ pending flags
	ALWAYS_INLINE static void ClearIrq()
	{
		// Apply constant on pending register
		EXTI->PR = kExtiBitValue;
		// Apply constant mask value for Interrupt set-enable registers 0 on NVIC
		if (kExtiNvicIntMask0)
			NVIC->ICPR[0] |= kExtiNvicIntMask0;
		// Apply constant mask value for Interrupt set-enable registers 1 on NVIC
		if (kExtiNvicIntMask1)
			NVIC->ICPR[1] |= kExtiNvicIntMask1;
	}
	/// Suspends IRQ on the NVIC
	ALWAYS_INLINE static void SupendIrq(void)
	{
		// Apply constant mask value for Interrupt set-enable registers 0 on NVIC
		if (kExtiNvicIntMask0)
			NVIC->ICER[0] |= kExtiNvicIntMask0;
		// Apply constant mask value for Interrupt set-enable registers 1 on NVIC
		if (kExtiNvicIntMask1)
			NVIC->ICER[1] |= kExtiNvicIntMask1;
	}
	/// Resumes IRQ on the NVIC
	ALWAYS_INLINE static void ResumeIrq(void)
	{
		// Apply constant mask value for Interrupt set-enable registers 0 on NVIC
		if (kExtiNvicIntMask0)
			NVIC->ISER[0] |= kExtiNvicIntMask0;
		// Apply constant mask value for Interrupt set-enable registers 1 on NVIC
		if (kExtiNvicIntMask1)
			NVIC->ISER[1] |= kExtiNvicIntMask1;
	}
	/// Issues a software event
	ALWAYS_INLINE static void SetEvent()
	{
		EXTI->SWIER |= kExtiBitValue;
	}
};


/// A template class for bulk configuration
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
	/// Combined constant for affected bit mask
	static constexpr uint32_t kExtiBitValue =
		Source0::kExtiBitValue | Source1::kExtiBitValue | Source2::kExtiBitValue | Source3::kExtiBitValue
		| Source4::kExtiBitValue | Source5::kExtiBitValue | Source6::kExtiBitValue | Source7::kExtiBitValue
		| Source8::kExtiBitValue | Source9::kExtiBitValue | Source10::kExtiBitValue | Source11::kExtiBitValue
		| Source12::kExtiBitValue | Source13::kExtiBitValue | Source14::kExtiBitValue | Source15::kExtiBitValue
		| Source16::kExtiBitValue | Source17::kExtiBitValue | Source18::kExtiBitValue
		;
	/// Combined constant for RTSR (Rising trigger selection register)
	static constexpr uint32_t kExtiTriggerRising =
		Source0::kExtiTriggerRising | Source1::kExtiTriggerRising | Source2::kExtiTriggerRising | Source3::kExtiTriggerRising
		| Source4::kExtiTriggerRising | Source5::kExtiTriggerRising | Source6::kExtiTriggerRising | Source7::kExtiTriggerRising
		| Source8::kExtiTriggerRising | Source9::kExtiTriggerRising | Source10::kExtiTriggerRising | Source11::kExtiTriggerRising
		| Source12::kExtiTriggerRising | Source13::kExtiTriggerRising | Source14::kExtiTriggerRising | Source15::kExtiTriggerRising
		| Source16::kExtiTriggerRising | Source17::kExtiTriggerRising | Source18::kExtiTriggerRising
		;
	/// Combined constant for FTSR (Falling trigger selection register)
	static constexpr uint32_t kExtiTriggerFalling =
		Source0::kExtiTriggerFalling | Source1::kExtiTriggerFalling | Source2::kExtiTriggerFalling | Source3::kExtiTriggerFalling
		| Source4::kExtiTriggerFalling | Source5::kExtiTriggerFalling | Source6::kExtiTriggerFalling | Source7::kExtiTriggerFalling
		| Source8::kExtiTriggerFalling | Source9::kExtiTriggerFalling | Source10::kExtiTriggerFalling | Source11::kExtiTriggerFalling
		| Source12::kExtiTriggerFalling | Source13::kExtiTriggerFalling | Source14::kExtiTriggerFalling | Source15::kExtiTriggerFalling
		| Source16::kExtiTriggerFalling | Source17::kExtiTriggerFalling | Source18::kExtiTriggerFalling
		;
	/// Combined constant for Interrupt mask register
	static constexpr uint32_t kExtiIntMask =
		Source0::kExtiIntMask | Source1::kExtiIntMask | Source2::kExtiIntMask | Source3::kExtiIntMask
		| Source4::kExtiIntMask | Source5::kExtiIntMask | Source6::kExtiIntMask | Source7::kExtiIntMask
		| Source8::kExtiIntMask | Source9::kExtiIntMask | Source10::kExtiIntMask | Source11::kExtiIntMask
		| Source12::kExtiIntMask | Source13::kExtiIntMask | Source14::kExtiIntMask | Source15::kExtiIntMask
		| Source16::kExtiIntMask | Source17::kExtiIntMask | Source18::kExtiIntMask
		;
	/// Combined constant mask value for Interrupt set-enable registers 0 on NVIC
	static constexpr uint32_t kExtiNvicIntMask0 =
		Source0::kExtiNvicIntMask0 | Source1::kExtiNvicIntMask0 | Source2::kExtiNvicIntMask0 | Source3::kExtiNvicIntMask0
		| Source4::kExtiNvicIntMask0 | Source5::kExtiNvicIntMask0 | Source6::kExtiNvicIntMask0 | Source7::kExtiNvicIntMask0
		| Source8::kExtiNvicIntMask0 | Source9::kExtiNvicIntMask0 | Source10::kExtiNvicIntMask0 | Source11::kExtiNvicIntMask0
		| Source12::kExtiNvicIntMask0 | Source13::kExtiNvicIntMask0 | Source14::kExtiNvicIntMask0 | Source15::kExtiNvicIntMask0
		| Source16::kExtiNvicIntMask0 | Source17::kExtiNvicIntMask0 | Source18::kExtiNvicIntMask0
		;
	/// Combined constant mask value for Interrupt set-enable registers 1 on NVIC
	static constexpr uint32_t kExtiNvicIntMask1 =
		Source0::kExtiNvicIntMask1 | Source1::kExtiNvicIntMask1 | Source2::kExtiNvicIntMask1 | Source3::kExtiNvicIntMask1
		| Source4::kExtiNvicIntMask1 | Source5::kExtiNvicIntMask1 | Source6::kExtiNvicIntMask1 | Source7::kExtiNvicIntMask1
		| Source8::kExtiNvicIntMask1 | Source9::kExtiNvicIntMask1 | Source10::kExtiNvicIntMask1 | Source11::kExtiNvicIntMask1
		| Source12::kExtiNvicIntMask1 | Source13::kExtiNvicIntMask1 | Source14::kExtiNvicIntMask1 | Source15::kExtiNvicIntMask1
		| Source16::kExtiNvicIntMask1 | Source17::kExtiNvicIntMask1 | Source18::kExtiNvicIntMask1
		;
	/// Combined constant bit value to External interrupt configuration register 1
	static constexpr uint32_t kExtiCR1 =
		Source0::kExtiCR1 | Source1::kExtiCR1 | Source2::kExtiCR1 | Source3::kExtiCR1
		| Source4::kExtiCR1 | Source5::kExtiCR1 | Source6::kExtiCR1 | Source7::kExtiCR1
		| Source8::kExtiCR1 | Source9::kExtiCR1 | Source10::kExtiCR1 | Source11::kExtiCR1
		| Source12::kExtiCR1 | Source13::kExtiCR1 | Source14::kExtiCR1 | Source15::kExtiCR1
		| Source16::kExtiCR1 | Source17::kExtiCR1 | Source18::kExtiCR1
		;
	/// Combined constant bit value to External interrupt configuration register 2
	static constexpr uint32_t kExtiCR2 =
		Source0::kExtiCR2 | Source1::kExtiCR2 | Source2::kExtiCR2 | Source3::kExtiCR2
		| Source4::kExtiCR2 | Source5::kExtiCR2 | Source6::kExtiCR2 | Source7::kExtiCR2
		| Source8::kExtiCR2 | Source9::kExtiCR2 | Source10::kExtiCR2 | Source11::kExtiCR2
		| Source12::kExtiCR2 | Source13::kExtiCR2 | Source14::kExtiCR2 | Source15::kExtiCR2
		| Source16::kExtiCR2 | Source17::kExtiCR2 | Source18::kExtiCR2
		;
	/// Combined constant bit value to External interrupt configuration register 3
	static constexpr uint32_t kExtiCR3 =
		Source0::kExtiCR3 | Source1::kExtiCR3 | Source2::kExtiCR3 | Source3::kExtiCR3
		| Source4::kExtiCR3 | Source5::kExtiCR3 | Source6::kExtiCR3 | Source7::kExtiCR3
		| Source8::kExtiCR3 | Source9::kExtiCR3 | Source10::kExtiCR3 | Source11::kExtiCR3
		| Source12::kExtiCR3 | Source13::kExtiCR3 | Source14::kExtiCR3 | Source15::kExtiCR3
		| Source16::kExtiCR3 | Source17::kExtiCR3 | Source18::kExtiCR3
		;
	/// Combined constant bit value to External interrupt configuration register 4
	static constexpr uint32_t kExtiCR4 =
		Source0::kExtiCR4 | Source1::kExtiCR4 | Source2::kExtiCR4 | Source3::kExtiCR4
		| Source4::kExtiCR4 | Source5::kExtiCR4 | Source6::kExtiCR4 | Source7::kExtiCR4
		| Source8::kExtiCR4 | Source9::kExtiCR4 | Source10::kExtiCR4 | Source11::kExtiCR4
		| Source12::kExtiCR4 | Source13::kExtiCR4 | Source14::kExtiCR4 | Source15::kExtiCR4
		| Source16::kExtiCR4 | Source17::kExtiCR4 | Source18::kExtiCR4
		;
	static constexpr uint32_t kExtiCR1_Mask =
		Source0::kExtiCR1_Mask | Source1::kExtiCR1_Mask | Source2::kExtiCR1_Mask | Source3::kExtiCR1_Mask
		| Source4::kExtiCR1_Mask | Source5::kExtiCR1_Mask | Source6::kExtiCR1_Mask | Source7::kExtiCR1_Mask
		| Source8::kExtiCR1_Mask | Source9::kExtiCR1_Mask | Source10::kExtiCR1_Mask | Source11::kExtiCR1_Mask
		| Source12::kExtiCR1_Mask | Source13::kExtiCR1_Mask | Source14::kExtiCR1_Mask | Source15::kExtiCR1_Mask
		| Source16::kExtiCR1_Mask | Source17::kExtiCR1_Mask | Source18::kExtiCR1_Mask
		;
	static constexpr uint32_t kExtiCR2_Mask =
		Source0::kExtiCR2_Mask | Source1::kExtiCR2_Mask | Source2::kExtiCR2_Mask | Source3::kExtiCR2_Mask
		| Source4::kExtiCR2_Mask | Source5::kExtiCR2_Mask | Source6::kExtiCR2_Mask | Source7::kExtiCR2_Mask
		| Source8::kExtiCR2_Mask | Source9::kExtiCR2_Mask | Source10::kExtiCR2_Mask | Source11::kExtiCR2_Mask
		| Source12::kExtiCR2_Mask | Source13::kExtiCR2_Mask | Source14::kExtiCR2_Mask | Source15::kExtiCR2_Mask
		| Source16::kExtiCR2_Mask | Source17::kExtiCR2_Mask | Source18::kExtiCR2_Mask
		;
	static constexpr uint32_t kExtiCR3_Mask =
		Source0::kExtiCR3_Mask | Source1::kExtiCR3_Mask | Source2::kExtiCR3_Mask | Source3::kExtiCR3_Mask
		| Source4::kExtiCR3_Mask | Source5::kExtiCR3_Mask | Source6::kExtiCR3_Mask | Source7::kExtiCR3_Mask
		| Source8::kExtiCR3_Mask | Source9::kExtiCR3_Mask | Source10::kExtiCR3_Mask | Source11::kExtiCR3_Mask
		| Source12::kExtiCR3_Mask | Source13::kExtiCR3_Mask | Source14::kExtiCR3_Mask | Source15::kExtiCR3_Mask
		| Source16::kExtiCR3_Mask | Source17::kExtiCR3_Mask | Source18::kExtiCR3_Mask
		;
	static constexpr uint32_t kExtiCR4_Mask =
		Source0::kExtiCR4_Mask | Source1::kExtiCR4_Mask | Source2::kExtiCR4_Mask | Source3::kExtiCR4_Mask
		| Source4::kExtiCR4_Mask | Source5::kExtiCR4_Mask | Source6::kExtiCR4_Mask | Source7::kExtiCR4_Mask
		| Source8::kExtiCR4_Mask | Source9::kExtiCR4_Mask | Source10::kExtiCR4_Mask | Source11::kExtiCR4_Mask
		| Source12::kExtiCR4_Mask | Source13::kExtiCR4_Mask | Source14::kExtiCR4_Mask | Source15::kExtiCR3_Mask
		| Source16::kExtiCR4_Mask | Source17::kExtiCR4_Mask | Source18::kExtiCR4_Mask
		;

	/// Starts module clock and enables configuration
	ALWAYS_INLINE static void Init(void)
	{
		RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
		Enable();
	}
	/// Applies settings to an already initialized EXTI
	ALWAYS_INLINE static void Enable(void)
	{
		// Apply constant on Rising trigger selection register
		EXTI->RTSR = kExtiTriggerRising;
		// Apply constant on Falling trigger selection register
		EXTI->FTSR = kExtiTriggerFalling;
		// Apply constant for Interrupt mask register
		EXTI->IMR = kExtiIntMask;
		// Apply constant mask value for Interrupt set-enable registers 0 on NVIC
		if (kExtiNvicIntMask0)
			NVIC->ISER[0] |= kExtiNvicIntMask0;
		// Apply constant mask value for Interrupt set-enable registers 1 on NVIC
		if (kExtiNvicIntMask1)
			NVIC->ISER[1] |= kExtiNvicIntMask1;
		// Apply constant mask value to External interrupt configuration register 1
		if(kExtiCR1_Mask != 0)
			AFIO->EXTICR[0] = (AFIO->EXTICR[0] & ~kExtiCR1_Mask) | kExtiCR1;
		// Apply constant mask value to External interrupt configuration register 2
		if(kExtiCR2_Mask != 0)
			AFIO->EXTICR[1] = (AFIO->EXTICR[1] & ~kExtiCR2_Mask) | kExtiCR2;
		// Apply constant mask value to External interrupt configuration register 3
		if(kExtiCR3_Mask != 0)
			AFIO->EXTICR[2] = (AFIO->EXTICR[2] & ~kExtiCR3_Mask) | kExtiCR3;
		// Apply constant mask value to External interrupt configuration register 4
		if(kExtiCR4_Mask != 0)
			AFIO->EXTICR[3] = (AFIO->EXTICR[3] & ~kExtiCR4_Mask) | kExtiCR4;
	}
	/// Disables all settings for the EXTI
	ALWAYS_INLINE static void Disable(const bool kDeInit = false)
	{
		if (kDeInit)
			RCC->APB2ENR &= ~RCC_APB2ENR_AFIOEN;
		// Apply constant on Rising trigger selection register
		if (kExtiTriggerRising)
			EXTI->RTSR &= ~kExtiTriggerRising;
		// Apply constant on Falling trigger selection register
		if (kExtiTriggerFalling)
			EXTI->FTSR &= ~kExtiTriggerFalling;
		// Apply constant for Interrupt mask register
		if(kExtiIntMask)
			EXTI->IMR &= kExtiIntMask;
		// Apply constant mask value for Interrupt set-enable registers 0 on NVIC
		if (kExtiNvicIntMask0)
			NVIC->ICER[0] |= kExtiNvicIntMask0;
		// Apply constant mask value for Interrupt set-enable registers 1 on NVIC
		if (kExtiNvicIntMask1)
			NVIC->ICER[1] |= kExtiNvicIntMask1;
	}
	/// Clear IRQ pending flags
	ALWAYS_INLINE static void ClearAllIrqs(void)
	{
		EXTI->PR = kExtiBitValue;
		// Apply constant mask value for Interrupt set-enable registers 0 on NVIC
		if (kExtiNvicIntMask0)
			NVIC->ICPR[0] |= kExtiNvicIntMask0;
		// Apply constant mask value for Interrupt set-enable registers 1 on NVIC
		if (kExtiNvicIntMask1)
			NVIC->ICPR[1] |= kExtiNvicIntMask1;
	}
	/// Suspends IRQ on the NVIC
	ALWAYS_INLINE static void SupendIrqs(void)
	{
		// Apply constant mask value for Interrupt set-enable registers 0 on NVIC
		if (kExtiNvicIntMask0)
			NVIC->ICER[0] |= kExtiNvicIntMask0;
		// Apply constant mask value for Interrupt set-enable registers 1 on NVIC
		if (kExtiNvicIntMask1)
			NVIC->ICER[1] |= kExtiNvicIntMask1;
	}
	/// Resumes IRQ on the NVIC
	ALWAYS_INLINE static void ResumeIrqs(void)
	{
		// Apply constant mask value for Interrupt set-enable registers 0 on NVIC
		if (kExtiNvicIntMask0)
			NVIC->ISER[0] |= kExtiNvicIntMask0;
		// Apply constant mask value for Interrupt set-enable registers 1 on NVIC
		if (kExtiNvicIntMask1)
			NVIC->ISER[1] |= kExtiNvicIntMask1;
	}
};

