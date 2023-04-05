#pragma once

#include "mcu-system.h"


#if defined(STM32L4)
#	define OPT_EXTI2	1
#endif

namespace Bmt
{

/// Enumeration for all EXTI input signals
enum class Exti : uint32_t
{
	k0,						///< EXTI line 0
	k1,						///< EXTI line 1
	k2,						///< EXTI line 2
	k3,						///< EXTI line 3
	k4,						///< EXTI line 4
	k5,						///< EXTI line 5
	k6,						///< EXTI line 6
	k7,						///< EXTI line 7
	k8,						///< EXTI line 8
	k9,						///< EXTI line 9
	k10,					///< EXTI line 10
	k11,					///< EXTI line 11
	k12,					///< EXTI line 12
	k13,					///< EXTI line 13
	k14,					///< EXTI line 14
	k15,					///< EXTI line 15
	k16,					///< EXTI line 16
	k17,					///< EXTI line 17
	k18,					///< EXTI line 18
	k19,					///< EXTI line 19
	k20,					///< EXTI line 20
	k21,					///< EXTI line 21
	k22,					///< EXTI line 22
	k23,					///< EXTI line 23
	k24,					///< EXTI line 24
	k25,					///< EXTI line 25
	k26,					///< EXTI line 26
	k27,					///< EXTI line 27
	k28,					///< EXTI line 28
	k29,					///< EXTI line 29
	k30,					///< EXTI line 30
	k31,					///< EXTI line 31
	k32,					///< EXTI line 32
	k33,					///< EXTI line 33
	k34,					///< EXTI line 34
	k35,					///< EXTI line 35
	k36,					///< EXTI line 36
	k37,					///< EXTI line 37
	k38,					///< EXTI line 38
	k39,					///< EXTI line 39
	k40,					///< EXTI line 40

#if defined(STM32F1)
	kPvd = k16,				///< EXTI PVD line, shared with line 16
	kRtc = k17,				///< EXTI RTC line, shared with line 17
	kUsb = k18,				///< EXTI USB line, shared with line 18
#elif defined(STM32L4)
	kPvd = k16,				///< EXTI PVD line, shared with line 16
	kUsb = k17,				///< EXTI USB line, shared with line 17
	kRtcAlarm = k18,		///< EXTI RTC Alarm line, shared with line 18
	kRtcTamper = k19,		///< EXTI RTC Tamper line, shared with line 19
	kRtcWakeup = k20,		///< EXTI RTC Wakeup line, shared with line 20
	kComp1 = k21,			///< EXTI COMP1 output line, shared with line 21
	kComp2 = k22,			///< EXTI COMP2 output line, shared with line 22
	kI2C1 = k23,			///< EXTI I2C1 Wakeup line, shared with line 22
	kI2C2 = k24,			///< EXTI I2C2 Wakeup line, shared with line 24
	kI2C3 = k25,			///< EXTI I2C3 Wakeup line, shared with line 25
	kUSART1 = k26,			///< EXTI USART1 Wakeup line, shared with line 26
	kUSART2 = k27,			///< EXTI USART2 Wakeup line, shared with line 27
	kUSART3 = k28,			///< EXTI USART3 Wakeup line, shared with line 28
	kUART4 = k29,			///< EXTI UART4 Wakeup line, shared with line 29
							// Exti30 unused
	kLPUART1 = k31,			///< EXTI LPUART1 Wakeup line, shared with line 31
	kLPTIM1 = k32,			///< EXTI LPTIM1 line, shared with line 32
	kLPTIM2 = k33,			///< EXTI LPTIM2 line, shared with line 33
	kSWPMI1 = k34,			///< EXTI SWPMI1 Wakeup line, shared with line 34
	kPVM1 = k35,			///< EXTI PVM1 Wakeup line, shared with line 35
							// Exti36 unused
	kPVM3 = k37,			///< EXTI PVM3 Wakeup line, shared with line 37
	kPVM4 = k38,			///< EXTI PVM4 Wakeup line, shared with line 38
	kLCD = k39,				///< EXTI LCD Wakeup line, shared with line 39
	//kI2C4 = k40,			///< EXTI I2C4 Wakeup line, shared with line 40
#endif
};


/// Enumeration for EXTI trigger mode
enum ExtiTrigger
{
	kSoft		///< Software trigger
	, kRising	///< Trigger on rising edge
	, kFalling	///< Trigger on falling edge
	, kBoth		///< Trigger on both edges
};


/// A bogus interrupt line with no configuration effect, just used as default template parameter filler
class ExtiSourceUnused
{
public:
	/// Affected bit mask constant (no effect)
	static constexpr uint32_t kExtiBitValue = 0;
#if OPT_EXTI2
	static constexpr uint32_t kExtiBitValue2 = 0;
#endif
	/// Constant for RTSR (Rising trigger selection register) (no effect)
	static constexpr uint32_t kExtiTriggerRising = 0;
#if OPT_EXTI2
	static constexpr uint32_t kExtiTriggerRising2 = 0;
#endif
	/// Constant for FTSR (Falling trigger selection register) (no effect)
	static constexpr uint32_t kExtiTriggerFalling = 0;
#if OPT_EXTI2
	static constexpr uint32_t kExtiTriggerFalling2 = 0;
#endif
	/// Constant for Interrupt mask register (no effect)
	static constexpr uint32_t kExtiIntMask = 0;
#if OPT_EXTI2
	static constexpr uint32_t kExtiIntMask2 = 0;
#endif
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
	const Exti kExtiLine									///< The EXTI line
	, const Gpio::Port kPortId = Gpio::Port::kUnusedPort	///< The port that triggers the EXTI interrupt
	, const ExtiTrigger kExtiMode = ExtiTrigger::kSoft		///< Event mode
	, const bool kInterrupt = false							///< Implements code for interruption (NVIC and EXTI_IMR)
>
class ExtiSource
{
public:
	/// Enumeration constant for this particular EXTI line
	static constexpr Exti kExtiSource = kExtiLine;
	/// Affected bit mask constant
	static constexpr uint32_t kExtiBitValue = (uint32_t(kExtiLine) < 32) ? (1 << uint32_t(kExtiLine)) : 0;
#if OPT_EXTI2
	static constexpr uint32_t kExtiBitValue2 = (uint32_t(kExtiLine) >= 32) ? (1 << (uint32_t(kExtiLine) - 32)) : 0;
#endif
	/// Constant for RTSR (Rising trigger selection register)
	static constexpr uint32_t kExtiTriggerRising = (kExtiMode == ExtiTrigger::kRising || kExtiMode == ExtiTrigger::kBoth) && (uint32_t(kExtiLine) < 32) ? (1 << uint32_t(kExtiLine)) : 0;
#if OPT_EXTI2
	static constexpr uint32_t kExtiTriggerRising2 = (kExtiMode == ExtiTrigger::kRising || kExtiMode == ExtiTrigger::kBoth) && (uint32_t(kExtiLine) >= 32) ? (1 << (uint32_t(kExtiLine) - 32)) : 0;
#endif
	/// Constant for FTSR (Falling trigger selection register)
	static constexpr uint32_t kExtiTriggerFalling = (kExtiMode == ExtiTrigger::kFalling || kExtiMode == ExtiTrigger::kBoth) && (uint32_t(kExtiLine) < 32) ? (1 << uint32_t(kExtiLine)) : 0;
#if OPT_EXTI2
	static constexpr uint32_t kExtiTriggerFalling2 = (kExtiMode == ExtiTrigger::kFalling || kExtiMode == ExtiTrigger::kBoth) && (uint32_t(kExtiLine) >= 32) ? (1 << (uint32_t(kExtiLine) - 32)) : 0;
#endif
	/// Constant for Interrupt mask register
	static constexpr uint32_t kExtiIntMask = kInterrupt && (uint32_t(kExtiLine) < 32) ? (1 << uint32_t(kExtiLine)) : 0;
#if OPT_EXTI2
	static constexpr uint32_t kExtiIntMask2 = kInterrupt && (uint32_t(kExtiLine) >= 32) ? (1 << (uint32_t(kExtiLine) - 32)) : 0;
#endif
	/// Constant for interrupt line in NVIC controller
	static constexpr uint32_t kExtiNvicInt = 
		(uint32_t(kExtiLine) <= 4) ? EXTI0_IRQn + uint32_t(kExtiLine)
		: (uint32_t(kExtiLine) <= 9) ? EXTI9_5_IRQn
#if OPT_EXTI2
		: (kExtiLine == Exti::kPvd) ? PVD_PVM_IRQn
		: (kExtiLine == Exti::kPVM1) ? PVD_PVM_IRQn
		: (kExtiLine == Exti::kPVM3) ? PVD_PVM_IRQn
		: (kExtiLine == Exti::kPVM4) ? PVD_PVM_IRQn
		: (kExtiLine == Exti::kUsb) ? USB_IRQn
		: (kExtiLine == Exti::kRtcAlarm) ? RTC_Alarm_IRQn
		: (kExtiLine == Exti::kRtcTamper) ? TAMP_STAMP_IRQn
		: (kExtiLine == Exti::kRtcWakeup) ? RTC_WKUP_IRQn
		: (kExtiLine == Exti::kComp1) ? COMP_IRQn
		: (kExtiLine == Exti::kComp2) ? COMP_IRQn
#else
		: (kExtiLine == Exti::kPvd) ? PVD_IRQn
		: (kExtiLine == Exti::kUsb) ? USBWakeUp_IRQn
		: (kExtiLine == Exti::kRtc) ? RTC_Alarm_IRQn
#endif
		: EXTI15_10_IRQn
		;
	/// Constant mask value for Interrupt set-enable registers 0 on NVIC
	static constexpr uint32_t kExtiNvicIntMask0 = (kInterrupt && uint32_t(kExtiLine) <= 9) ? (1 << kExtiNvicInt) : 0;
	/// Constant mask value for Interrupt set-enable registers 1 on NVIC
	static constexpr uint32_t kExtiNvicIntMask1 = (kInterrupt && uint32_t(kExtiLine) >= 10) ? (1 << (kExtiNvicInt & 0x1FUL)) : 0;
	/// Constant bit value to External interrupt configuration register 1
	static constexpr uint32_t kExtiCR1 = uint32_t(kExtiLine) <= 3 && kPortId != Gpio::Port::kUnusedPort
		? (uint32_t(kPortId) << (4 * uint32_t(kExtiLine))) : 0;
	/// Constant bit value to External interrupt configuration register 2
	static constexpr uint32_t kExtiCR2 = uint32_t(kExtiLine) >= 4 && uint32_t(kExtiLine) <= 7 && kPortId != Gpio::Port::kUnusedPort
		? (uint32_t(kPortId) << (4 * (uint32_t(kExtiLine) - 4))) : 0;
	/// Constant bit value to External interrupt configuration register 3
	static constexpr uint32_t kExtiCR3 = uint32_t(kExtiLine) >= 8 && uint32_t(kExtiLine) <= 11 && kPortId != Gpio::Port::kUnusedPort
		? (uint32_t(kPortId) << (4 * (uint32_t(kExtiLine) - 8))) : 0;
	/// Constant bit value to External interrupt configuration register 4
	static constexpr uint32_t kExtiCR4 = uint32_t(kExtiLine) >= 12 && uint32_t(kExtiLine) <= 15 && kPortId != Gpio::Port::kUnusedPort
		? (uint32_t(kPortId) << (4 * (uint32_t(kExtiLine) - 12))) : 0;
	/// Combined constant mask value (4 bit-group) to External interrupt configuration register 1
	static constexpr uint32_t kExtiCR1_Mask = uint32_t(kExtiLine) <= 3 && kPortId != Gpio::Port::kUnusedPort
		? (0xF << (4 * uint32_t(kExtiLine))) : 0;
	/// Combined constant mask value (4 bit-group) to External interrupt configuration register 2
	static constexpr uint32_t kExtiCR2_Mask = uint32_t(kExtiLine) >= 4 && uint32_t(kExtiLine) <= 7 && kPortId != Gpio::Port::kUnusedPort
		? (0xF << (4 * (uint32_t(kExtiLine) - 4))) : 0;
	/// Combined constant mask value (4 bit-group) to External interrupt configuration register 3
	static constexpr uint32_t kExtiCR3_Mask = uint32_t(kExtiLine) >= 8 && uint32_t(kExtiLine) <= 11 && kPortId != Gpio::Port::kUnusedPort
		? (0xF << (4 * (uint32_t(kExtiLine) - 8))) : 0;
	/// Combined constant mask value (4 bit-group) to External interrupt configuration register 4
	static constexpr uint32_t kExtiCR4_Mask = uint32_t(kExtiLine) >= 12 && uint32_t(kExtiLine) <= 15 && kPortId != Gpio::Port::kUnusedPort
		? (0xF << (4 * (uint32_t(kExtiLine) - 12))) : 0;

	/// Applies settings to an already initialized EXTI
	ALWAYS_INLINE static void Enable()
	{
#if OPT_EXTI2
		// Constant for Rising trigger selection register
		if (kExtiTriggerRising)
			EXTI->RTSR1 |= kExtiTriggerRising;
		if (kExtiTriggerRising2)
			EXTI->RTSR2 |= kExtiTriggerRising2;
		// Constant for Falling trigger selection register
		if (kExtiTriggerFalling)
			EXTI->FTSR1 |= kExtiTriggerFalling;
		if (kExtiTriggerFalling2)
			EXTI->FTSR2 |= kExtiTriggerFalling2;
		// Constant for Interrupt mask register
		if (kExtiIntMask)
			EXTI->IMR1 = kExtiIntMask;
		if (kExtiIntMask2)
			EXTI->IMR2 = kExtiIntMask2;
#else
		// Constant for Rising trigger selection register
		if (kExtiTriggerRising)
			EXTI->RTSR |= kExtiTriggerRising;
		// Constant for Falling trigger selection register
		if (kExtiTriggerFalling)
			EXTI->FTSR |= kExtiTriggerFalling;
		// Constant for Interrupt mask register
		if (kExtiIntMask)
			EXTI->IMR = kExtiIntMask;
#endif
		// Apply constant mask value for Interrupt set-enable registers 0 on NVIC
		if (kExtiNvicIntMask0)
			NVIC->ISER[0] |= kExtiNvicIntMask0;
		// Apply constant mask value for Interrupt set-enable registers 1 on NVIC
		if (kExtiNvicIntMask1)
			NVIC->ISER[1] |= kExtiNvicIntMask1;
#if OPT_EXTI2
		// Apply constant mask value to External interrupt configuration register 1
		if (kExtiCR1_Mask)
			SYSCFG->EXTICR[0] = (SYSCFG->EXTICR[0] & ~kExtiCR1_Mask) | kExtiCR1;
		// Apply constant mask value to External interrupt configuration register 2
		if (kExtiCR2_Mask)
			SYSCFG->EXTICR[1] = (SYSCFG->EXTICR[1] & ~kExtiCR2_Mask) | kExtiCR2;
		// Apply constant mask value to External interrupt configuration register 3
		if (kExtiCR3_Mask)
			SYSCFG->EXTICR[2] = (SYSCFG->EXTICR[2] & ~kExtiCR3_Mask) | kExtiCR3;
		// Apply constant mask value to External interrupt configuration register 4
		if (kExtiCR4_Mask)
			SYSCFG->EXTICR[3] = (SYSCFG->EXTICR[3] & ~kExtiCR4_Mask) | kExtiCR4;
#else
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
#endif
	}
	/// Disables all settings for the EXTI
	ALWAYS_INLINE static void Disable()
	{
#if OPT_EXTI2
		// Constant for Rising trigger selection register
		if (kExtiTriggerRising)
			EXTI->RTSR1 &= ~kExtiTriggerRising;
		if (kExtiTriggerRising2)
			EXTI->RTSR2 &= ~kExtiTriggerRising2;
		// Constant for Falling trigger selection register
		if (kExtiTriggerFalling)
			EXTI->FTSR1 &= ~kExtiTriggerFalling;
		if (kExtiTriggerFalling2)
			EXTI->FTSR2 &= ~kExtiTriggerFalling2;
		// Constant for Interrupt mask register
		if (kExtiIntMask)
			EXTI->IMR1 &= kExtiIntMask;
		if (kExtiIntMask2)
			EXTI->IMR2 &= kExtiIntMask2;
		// Apply constant mask value for Interrupt set-enable registers 0 on NVIC
		if (kExtiNvicIntMask0)
			NVIC->ICER[0] |= kExtiNvicIntMask0;
		// Apply constant mask value for Interrupt set-enable registers 1 on NVIC
		if (kExtiNvicIntMask1)
			NVIC->ICER[1] |= kExtiNvicIntMask1;
#else
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
#endif
	}
	/// Checks if event is pending
	ALWAYS_INLINE static bool IsPending()
	{
#if OPT_EXTI2
		if (kExtiBitValue)
			return (EXTI->PR1 & kExtiBitValue) != 0;
		else if (kExtiBitValue2)
			return (EXTI->PR2 & kExtiBitValue2) != 0;
		else
			return false;
#else
		if (kExtiBitValue)
			return (EXTI->PR & kExtiBitValue) != 0;
		else
			return false;
#endif
	}
	/// Clear IRQ pending flags
	ALWAYS_INLINE static void ClearPending()
	{
		// Writing a 1 clears the event
#if OPT_EXTI2
		if (kExtiBitValue)
			EXTI->PR1 = kExtiBitValue;
		if (kExtiBitValue2)
			EXTI->PR2 = kExtiBitValue2;
#else
		if (kExtiBitValue)
			EXTI->PR = kExtiBitValue;
#endif
#if 0
		if (kExtiIntMask)
		{
			// Apply constant mask value for Interrupt set-enable registers 0 on NVIC
			if (kExtiNvicIntMask0)
				NVIC->ICPR[0] |= kExtiNvicIntMask0;
			// Apply constant mask value for Interrupt set-enable registers 1 on NVIC
			if (kExtiNvicIntMask1)
				NVIC->ICPR[1] |= kExtiNvicIntMask1;
		}
#endif
	}
	/// Disables IRQ on the EXTI peripheral
	ALWAYS_INLINE static void EnableIrq(void)
	{
#if OPT_EXTI2
		if (kExtiIntMask)
			EXTI->IMR1 |= kExtiIntMask;
		if (kExtiIntMask2)
			EXTI->IMR2 |= kExtiIntMask2;
#else
		if (kExtiIntMask)
			EXTI->IMR |= kExtiIntMask;
#endif
	}
	/// Disables IRQ on the EXTI peripheral
	ALWAYS_INLINE static void DisableIrq(void)
	{
#if OPT_EXTI2
		if (kExtiIntMask)
			EXTI->IMR1 &= ~kExtiIntMask;
		if (kExtiIntMask2)
			EXTI->IMR2 &= ~kExtiIntMask2;
#else
		if (kExtiIntMask)
			EXTI->IMR &= ~kExtiIntMask;
#endif
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
#if OPT_EXTI2
		if (kExtiBitValue)
			EXTI->SWIER1 |= kExtiBitValue;
		if (kExtiBitValue2)
			EXTI->SWIER2 |= kExtiBitValue2;
#else
		if (kExtiBitValue)
			EXTI->SWIER |= kExtiBitValue;
#endif
	}
};


/// A template class for bulk EXTI configuration
/*!
A combined set of EXTI sources bound together. Optimizes code footprint and performance as
operations are performed in bulk.

\par Example

An hypothetical device with 4 button connected on PA5, PB6, PA7 and PB8:
\code{.cpp}
typedef ExtiSource<Exti5, Gpio::Port::PA, ExtiTrigger::kFalling, true> ButtonLeft;
typedef ExtiSource<Exti6, Gpio::Port::PB, ExtiTrigger::kFalling, true> ButtonRight;
typedef ExtiSource<Exti7, Gpio::Port::PA, ExtiTrigger::kFalling, true> ButtonUp;
typedef ExtiSource<Exti8, Gpio::Port::PB, ExtiTrigger::kFalling, true> ButtonDown;
typedef ExtiTemplate<ButtonLeft, ButtonRight, ButtonUp, ButtonDown> Joystick;

void Initialize()
{
	...
	// Setup EXTI interrupts
	Joystick::Init();
	...
}
\endcode

To suspend all interrupts of the joystick at once:
\code{.cpp}
	// Disable all Joystick interrupts
	Joystick::DisableIrq();
\endcode

To reenable all interrupts of the joystick:
\code{.cpp}
	// Disable all Joystick interrupts
	Joystick::EnableIrq();
\endcode

A minimal interrupt handler routine is shown next:
\code{.cpp}
void EXTI9_5_IRQHandler(void)
{
	if (ButtonLeft::IsPending())
	{
		// Handle left movement
	}
	if (ButtonRight::IsPending())
	{
		// Handle right movement
	}
	if (ButtonUp::IsPending())
	{
		// Handle up movement
	}
	if (ButtonDown::IsPending())
	{
		// Handle down movement
	}
	// Clear all pending flags
	Joystick::ClearAllPendingFlags();
}
\endcode
*/
template<
	typename Source0						///< The First EXTI source
	, typename Source1						///< A second EXTI source
	, typename Source2 = ExtiSourceUnused	///< Third EXTI source
	, typename Source3 = ExtiSourceUnused	///< 4th EXTI source
	, typename Source4 = ExtiSourceUnused	///< 5th EXTI source
	, typename Source5 = ExtiSourceUnused	///< 6th EXTI source
	, typename Source6 = ExtiSourceUnused	///< 7th EXTI source
	, typename Source7 = ExtiSourceUnused	///< 8th EXTI source
	, typename Source8 = ExtiSourceUnused	///< 9th EXTI source
	, typename Source9 = ExtiSourceUnused	///< 10th EXTI source
	, typename Source10 = ExtiSourceUnused	///< 11th EXTI source
	, typename Source11 = ExtiSourceUnused	///< 12th EXTI source
	, typename Source12 = ExtiSourceUnused	///< 13th EXTI source
	, typename Source13 = ExtiSourceUnused	///< 14th EXTI source
	, typename Source14 = ExtiSourceUnused	///< 15th EXTI source
	, typename Source15 = ExtiSourceUnused	///< 16th EXTI source
	, typename Source16 = ExtiSourceUnused	///< 17th EXTI source
	, typename Source17 = ExtiSourceUnused	///< 18th EXTI source
	, typename Source18 = ExtiSourceUnused	///< 19th EXTI source
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
#if OPT_EXTI2
	static constexpr uint32_t kExtiBitValue2 =
		Source0::kExtiBitValue2 | Source1::kExtiBitValue2 | Source2::kExtiBitValue2 | Source3::kExtiBitValue2
		| Source4::kExtiBitValue2 | Source5::kExtiBitValue2 | Source6::kExtiBitValue2 | Source7::kExtiBitValue2
		| Source8::kExtiBitValue2 | Source9::kExtiBitValue2 | Source10::kExtiBitValue2 | Source11::kExtiBitValue2
		| Source12::kExtiBitValue2 | Source13::kExtiBitValue2 | Source14::kExtiBitValue2 | Source15::kExtiBitValue2
		| Source16::kExtiBitValue2 | Source17::kExtiBitValue2 | Source18::kExtiBitValue2
		;
#endif
	/// Combined constant for RTSR (Rising trigger selection register)
	static constexpr uint32_t kExtiTriggerRising =
		Source0::kExtiTriggerRising | Source1::kExtiTriggerRising | Source2::kExtiTriggerRising | Source3::kExtiTriggerRising
		| Source4::kExtiTriggerRising | Source5::kExtiTriggerRising | Source6::kExtiTriggerRising | Source7::kExtiTriggerRising
		| Source8::kExtiTriggerRising | Source9::kExtiTriggerRising | Source10::kExtiTriggerRising | Source11::kExtiTriggerRising
		| Source12::kExtiTriggerRising | Source13::kExtiTriggerRising | Source14::kExtiTriggerRising | Source15::kExtiTriggerRising
		| Source16::kExtiTriggerRising | Source17::kExtiTriggerRising | Source18::kExtiTriggerRising
		;
#if OPT_EXTI2
	static constexpr uint32_t kExtiTriggerRising2 =
		Source0::kExtiTriggerRising2 | Source1::kExtiTriggerRising2 | Source2::kExtiTriggerRising2 | Source3::kExtiTriggerRising2
		| Source4::kExtiTriggerRising2 | Source5::kExtiTriggerRising2 | Source6::kExtiTriggerRising2 | Source7::kExtiTriggerRising2
		| Source8::kExtiTriggerRising2 | Source9::kExtiTriggerRising2 | Source10::kExtiTriggerRising2 | Source11::kExtiTriggerRising2
		| Source12::kExtiTriggerRising2 | Source13::kExtiTriggerRising2 | Source14::kExtiTriggerRising2 | Source15::kExtiTriggerRising2
		| Source16::kExtiTriggerRising2 | Source17::kExtiTriggerRising2 | Source18::kExtiTriggerRising2
		;
#endif
	/// Combined constant for FTSR (Falling trigger selection register)
	static constexpr uint32_t kExtiTriggerFalling =
		Source0::kExtiTriggerFalling | Source1::kExtiTriggerFalling | Source2::kExtiTriggerFalling | Source3::kExtiTriggerFalling
		| Source4::kExtiTriggerFalling | Source5::kExtiTriggerFalling | Source6::kExtiTriggerFalling | Source7::kExtiTriggerFalling
		| Source8::kExtiTriggerFalling | Source9::kExtiTriggerFalling | Source10::kExtiTriggerFalling | Source11::kExtiTriggerFalling
		| Source12::kExtiTriggerFalling | Source13::kExtiTriggerFalling | Source14::kExtiTriggerFalling | Source15::kExtiTriggerFalling
		| Source16::kExtiTriggerFalling | Source17::kExtiTriggerFalling | Source18::kExtiTriggerFalling
		;
#if OPT_EXTI2
	static constexpr uint32_t kExtiTriggerFalling2 =
		Source0::kExtiTriggerFalling2 | Source1::kExtiTriggerFalling2 | Source2::kExtiTriggerFalling2 | Source3::kExtiTriggerFalling2
		| Source4::kExtiTriggerFalling2 | Source5::kExtiTriggerFalling2 | Source6::kExtiTriggerFalling2 | Source7::kExtiTriggerFalling2
		| Source8::kExtiTriggerFalling2 | Source9::kExtiTriggerFalling2 | Source10::kExtiTriggerFalling2 | Source11::kExtiTriggerFalling2
		| Source12::kExtiTriggerFalling2 | Source13::kExtiTriggerFalling2 | Source14::kExtiTriggerFalling2 | Source15::kExtiTriggerFalling2
		| Source16::kExtiTriggerFalling2 | Source17::kExtiTriggerFalling2 | Source18::kExtiTriggerFalling2
		;
#endif
	/// Combined constant for Interrupt mask register
	static constexpr uint32_t kExtiIntMask =
		Source0::kExtiIntMask | Source1::kExtiIntMask | Source2::kExtiIntMask | Source3::kExtiIntMask
		| Source4::kExtiIntMask | Source5::kExtiIntMask | Source6::kExtiIntMask | Source7::kExtiIntMask
		| Source8::kExtiIntMask | Source9::kExtiIntMask | Source10::kExtiIntMask | Source11::kExtiIntMask
		| Source12::kExtiIntMask | Source13::kExtiIntMask | Source14::kExtiIntMask | Source15::kExtiIntMask
		| Source16::kExtiIntMask | Source17::kExtiIntMask | Source18::kExtiIntMask
		;
#if OPT_EXTI2
	static constexpr uint32_t kExtiIntMask2 =
		Source0::kExtiIntMask2 | Source1::kExtiIntMask2 | Source2::kExtiIntMask2 | Source3::kExtiIntMask2
		| Source4::kExtiIntMask2 | Source5::kExtiIntMask2 | Source6::kExtiIntMask2 | Source7::kExtiIntMask2
		| Source8::kExtiIntMask2 | Source9::kExtiIntMask2 | Source10::kExtiIntMask2 | Source11::kExtiIntMask2
		| Source12::kExtiIntMask2 | Source13::kExtiIntMask2 | Source14::kExtiIntMask2 | Source15::kExtiIntMask2
		| Source16::kExtiIntMask2 | Source17::kExtiIntMask2 | Source18::kExtiIntMask2
		;
#endif
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
	/// Combined constant mask value (4 bit-group) to External interrupt configuration register 1
	static constexpr uint32_t kExtiCR1_Mask =
		Source0::kExtiCR1_Mask | Source1::kExtiCR1_Mask | Source2::kExtiCR1_Mask | Source3::kExtiCR1_Mask
		| Source4::kExtiCR1_Mask | Source5::kExtiCR1_Mask | Source6::kExtiCR1_Mask | Source7::kExtiCR1_Mask
		| Source8::kExtiCR1_Mask | Source9::kExtiCR1_Mask | Source10::kExtiCR1_Mask | Source11::kExtiCR1_Mask
		| Source12::kExtiCR1_Mask | Source13::kExtiCR1_Mask | Source14::kExtiCR1_Mask | Source15::kExtiCR1_Mask
		| Source16::kExtiCR1_Mask | Source17::kExtiCR1_Mask | Source18::kExtiCR1_Mask
		;
	/// Combined constant mask value (4 bit-group) to External interrupt configuration register 2
	static constexpr uint32_t kExtiCR2_Mask =
		Source0::kExtiCR2_Mask | Source1::kExtiCR2_Mask | Source2::kExtiCR2_Mask | Source3::kExtiCR2_Mask
		| Source4::kExtiCR2_Mask | Source5::kExtiCR2_Mask | Source6::kExtiCR2_Mask | Source7::kExtiCR2_Mask
		| Source8::kExtiCR2_Mask | Source9::kExtiCR2_Mask | Source10::kExtiCR2_Mask | Source11::kExtiCR2_Mask
		| Source12::kExtiCR2_Mask | Source13::kExtiCR2_Mask | Source14::kExtiCR2_Mask | Source15::kExtiCR2_Mask
		| Source16::kExtiCR2_Mask | Source17::kExtiCR2_Mask | Source18::kExtiCR2_Mask
		;
	/// Combined constant mask value (4 bit-group) to External interrupt configuration register 3
	static constexpr uint32_t kExtiCR3_Mask =
		Source0::kExtiCR3_Mask | Source1::kExtiCR3_Mask | Source2::kExtiCR3_Mask | Source3::kExtiCR3_Mask
		| Source4::kExtiCR3_Mask | Source5::kExtiCR3_Mask | Source6::kExtiCR3_Mask | Source7::kExtiCR3_Mask
		| Source8::kExtiCR3_Mask | Source9::kExtiCR3_Mask | Source10::kExtiCR3_Mask | Source11::kExtiCR3_Mask
		| Source12::kExtiCR3_Mask | Source13::kExtiCR3_Mask | Source14::kExtiCR3_Mask | Source15::kExtiCR3_Mask
		| Source16::kExtiCR3_Mask | Source17::kExtiCR3_Mask | Source18::kExtiCR3_Mask
		;
	/// Combined constant mask value (4 bit-group) to External interrupt configuration register 4
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
#ifdef RCC_APB2ENR_AFIOEN
		RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
		volatile uint32_t delay = RCC->APB2ENR & RCC_APB2ENR_AFIOEN;
#endif
		Enable();
	}
	/// Applies settings to an already initialized EXTI
	ALWAYS_INLINE static void Enable(void)
	{
#if OPT_EXTI2
		// Apply constant on Rising trigger selection register
		EXTI->RTSR1 = kExtiTriggerRising;
		EXTI->RTSR2 = kExtiTriggerRising2;
		// Apply constant on Falling trigger selection register
		EXTI->FTSR1 = kExtiTriggerFalling;
		EXTI->FTSR2 = kExtiTriggerFalling2;
		// Apply constant for Interrupt mask register
		EXTI->IMR1 = kExtiIntMask;
		EXTI->IMR2 = kExtiIntMask2;
#else
		// Apply constant on Rising trigger selection register
		EXTI->RTSR = kExtiTriggerRising;
		// Apply constant on Falling trigger selection register
		EXTI->FTSR = kExtiTriggerFalling;
		// Apply constant for Interrupt mask register
		EXTI->IMR = kExtiIntMask;
#endif
		// Apply constant mask value for Interrupt set-enable registers 0 on NVIC
		if (kExtiNvicIntMask0)
			NVIC->ISER[0] |= kExtiNvicIntMask0;
		// Apply constant mask value for Interrupt set-enable registers 1 on NVIC
		if (kExtiNvicIntMask1)
			NVIC->ISER[1] |= kExtiNvicIntMask1;
#if OPT_EXTI2
		// Apply constant mask value to External interrupt configuration register 1
		if (kExtiCR1_Mask != 0)
			SYSCFG->EXTICR[0] = (SYSCFG->EXTICR[0] & ~kExtiCR1_Mask) | kExtiCR1;
		// Apply constant mask value to External interrupt configuration register 2
		if (kExtiCR2_Mask != 0)
			SYSCFG->EXTICR[1] = (SYSCFG->EXTICR[1] & ~kExtiCR2_Mask) | kExtiCR2;
		// Apply constant mask value to External interrupt configuration register 3
		if (kExtiCR3_Mask != 0)
			SYSCFG->EXTICR[2] = (SYSCFG->EXTICR[2] & ~kExtiCR3_Mask) | kExtiCR3;
		// Apply constant mask value to External interrupt configuration register 4
		if (kExtiCR4_Mask != 0)
			SYSCFG->EXTICR[3] = (SYSCFG->EXTICR[3] & ~kExtiCR4_Mask) | kExtiCR4;
#else
		// Apply constant mask value to External interrupt configuration register 1
		if (kExtiCR1_Mask != 0)
			AFIO->EXTICR[0] = (AFIO->EXTICR[0] & ~kExtiCR1_Mask) | kExtiCR1;
		// Apply constant mask value to External interrupt configuration register 2
		if (kExtiCR2_Mask != 0)
			AFIO->EXTICR[1] = (AFIO->EXTICR[1] & ~kExtiCR2_Mask) | kExtiCR2;
		// Apply constant mask value to External interrupt configuration register 3
		if (kExtiCR3_Mask != 0)
			AFIO->EXTICR[2] = (AFIO->EXTICR[2] & ~kExtiCR3_Mask) | kExtiCR3;
		// Apply constant mask value to External interrupt configuration register 4
		if (kExtiCR4_Mask != 0)
			AFIO->EXTICR[3] = (AFIO->EXTICR[3] & ~kExtiCR4_Mask) | kExtiCR4;
#endif
	}
	/// Disables all settings for the EXTI
	ALWAYS_INLINE static void Disable(const bool kDeInit = false)
	{
#if OPT_EXTI2
		// Apply constant on Rising trigger selection register
		if (kExtiTriggerRising)
			EXTI->RTSR1 &= ~kExtiTriggerRising;
		if (kExtiTriggerRising2)
			EXTI->RTSR2 &= ~kExtiTriggerRising2;
		// Apply constant on Falling trigger selection register
		if (kExtiTriggerFalling)
			EXTI->FTSR1 &= ~kExtiTriggerFalling;
		if (kExtiTriggerFalling2)
			EXTI->FTSR2 &= ~kExtiTriggerFalling2;
		// Apply constant for Interrupt mask register
		if (kExtiIntMask)
			EXTI->IMR1 &= kExtiIntMask;
		if (kExtiIntMask2)
			EXTI->IMR2 &= kExtiIntMask2;
#else
		if (kDeInit)
			RCC->APB2ENR &= ~RCC_APB2ENR_AFIOEN;
		// Apply constant on Rising trigger selection register
		if (kExtiTriggerRising)
			EXTI->RTSR &= ~kExtiTriggerRising;
		// Apply constant on Falling trigger selection register
		if (kExtiTriggerFalling)
			EXTI->FTSR &= ~kExtiTriggerFalling;
		// Apply constant for Interrupt mask register
		if (kExtiIntMask)
			EXTI->IMR &= kExtiIntMask;
#endif
		// Apply constant mask value for Interrupt set-enable registers 0 on NVIC
		if (kExtiNvicIntMask0)
			NVIC->ICER[0] |= kExtiNvicIntMask0;
		// Apply constant mask value for Interrupt set-enable registers 1 on NVIC
		if (kExtiNvicIntMask1)
			NVIC->ICER[1] |= kExtiNvicIntMask1;
	}
	/// Disables IRQ on the EXTI peripheral
	ALWAYS_INLINE static void EnableIrq(void)
	{
#if OPT_EXTI2
		if (kExtiIntMask)
			EXTI->IMR1 |= kExtiIntMask;
		if (kExtiIntMask2)
			EXTI->IMR2 |= kExtiIntMask2;
#else
		if (kExtiIntMask)
			EXTI->IMR |= kExtiIntMask;
#endif
	}
	/// Disables IRQ on the EXTI peripheral
	ALWAYS_INLINE static void DisableIrq(void)
	{
#if OPT_EXTI2
		if (kExtiIntMask)
			EXTI->IMR1 &= ~kExtiIntMask;
		if (kExtiIntMask2)
			EXTI->IMR2 &= ~kExtiIntMask2;
#else
		if (kExtiIntMask)
			EXTI->IMR &= ~kExtiIntMask;
#endif
	}
	/// Clear IRQ pending flags
	ALWAYS_INLINE static void ClearAllPendingFlags(void)
	{
#if OPT_EXTI2
		if (kExtiBitValue)
			EXTI->PR1 = kExtiBitValue;
		if (kExtiBitValue2)
			EXTI->PR2 = kExtiBitValue2;
#else
		EXTI->PR = kExtiBitValue;
#endif
#if 0
		// Apply constant mask value for Interrupt set-enable registers 0 on NVIC
		if (kExtiNvicIntMask0)
			NVIC->ICPR[0] |= kExtiNvicIntMask0;
		// Apply constant mask value for Interrupt set-enable registers 1 on NVIC
		if (kExtiNvicIntMask1)
			NVIC->ICPR[1] |= kExtiNvicIntMask1;
#endif
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


//! A wrapper for a EXTI-interrupt compatibility with CriticalSectionIrq<>
/*!
\note Works with ExtiTemplate<> and ExtiSource<>

\par Example
\code{.cpp}
// A button on PA0 using a falling edge interrupt
typedef ExtiSource<Exti::k0, Gpio::Port::PA, ExtiTrigger::kFalling, true> MyButtonOnPA0;
void MyCriticalCode()
{
	CriticalSectionIrq<ExtiSet<MyButtonOnPA0>> crit_sec;
	// Now do critical code while interruption on MyButtonOnPA0 is suspended
	...
}
\endcode
*/
template <typename exti_set>
class ExtiSet
{
public:
	/// Enable EXTI interrupt(s)
	ALWAYS_INLINE static void Enable()
	{
		exti_set::EnableIrq();
	}
	/// Disable EXTI interrupt(s)
	ALWAYS_INLINE static void Disable()
	{
		exti_set::DisableIrq();
	}
};


}	// namespace Bmt
