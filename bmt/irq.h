#pragma once


/// A template class for a controllable IRQ event
template <const IRQn_Type kIrq>
class IrqTemplate
{
public:
	/// Mask for IxER[0] register or 0 if not applicable
	static constexpr uint32_t mask0_ = (kIrq >= 0 && kIrq < 32) ? (1 << kIrq) : 0;
	/// Mask for IxER[1] register or 0 if not applicable
	static constexpr uint32_t mask1_ = (kIrq >= 32 && kIrq < 64) ? (1 << (kIrq - 32)) : 0;
	
	/// Access to the hardware register area
	ALWAYS_INLINE static volatile NVIC_Type & Io() { return *(volatile NVIC_Type*)NVIC_BASE; }

	/// Enables the interrupt on the NVIC Cortex peripheral
	ALWAYS_INLINE static void Enable()
	{
		// Compile time command exclusion
		if (mask0_)
			Io().ISER[0U] = mask0_;
		// Compile time command exclusion
		if (mask1_)
			Io().ISER[1U] = mask1_;
	}
	/// Disables the interrupt on the NVIC Cortex peripheral
	ALWAYS_INLINE static void Disable()
	{
		// Compile time command exclusion
		if (mask0_)
			Io().ICER[0U] = mask0_;
		// Compile time command exclusion
		if (mask1_)
			Io().ICER[1U] = mask1_;
	}
	/// Clears the interrupt flag on the NVIC Cortex peripheral
	ALWAYS_INLINE static void ClearPending()
	{
		// Compile time command exclusion
		if (mask0_)
			Io().ICPR[0U] = mask0_;
		// Compile time command exclusion
		if (mask1_)
			Io().ICPR[1U] = mask1_;
	}
};


/// A bogus IRQ type definition which has no effect (does not produce code)
static constexpr IRQn_Type BogusIrqType = SysTick_IRQn;


/// Groups a two up to sixteen interrupt types in a set, to enable/disable all at once
/*!
Example:
\code{.cpp}
/// A data-type representing interface to the Gpio::PC
typedef IrqTemplate<USART1_IRQn> CommPC;
/// A data-type representing interface to a Modem
typedef IrqTemplate<USART2_IRQn> CommModem;
/// Group both together
typedef IrqSet<CommPC, CommModem> AllComms;

void MyExample()
{
	// This shows how handle interrupts individually
	CommPC::Disable();
	CommModem::ClearPending();
	// BLOCK: Now a critical section that blocks CommModem ISR
	{
		CriticalSectionIrq<CommModem> lock;
		// TODO: Here your critical "modem" code
	}
	// BLOCK: This shows how to handle interrupts in a group
	{
		// Disables both interrupts to avoid race conditions
		CriticalSectionIrq<AllComms> lock;
		// TODO: write your code that is critical, without race conditions with both Comm ISR
	}
	// ENDBLOCK: IRQs are reactivated at the end of the 'lock' scope
}
\endcode
*/
template <
	typename kIrq1									///< The 1st IrqTemplate<> data-type to control IRQ
	, typename kIrq2 = IrqTemplate<BogusIrqType>	///< The 2nd IrqTemplate<> data-type to control IRQ
	, typename kIrq3 = IrqTemplate<BogusIrqType>	///< The 3rd IrqTemplate<> data-type to control IRQ
	, typename kIrq4 = IrqTemplate<BogusIrqType>	///< The 4th IrqTemplate<> data-type to control IRQ
	, typename kIrq5 = IrqTemplate<BogusIrqType>	///< The 5th IrqTemplate<> data-type to control IRQ
	, typename kIrq6 = IrqTemplate<BogusIrqType>	///< The 6th IrqTemplate<> data-type to control IRQ
	, typename kIrq7 = IrqTemplate<BogusIrqType>	///< The 7th IrqTemplate<> data-type to control IRQ
	, typename kIrq8 = IrqTemplate<BogusIrqType>	///< The 8th IrqTemplate<> data-type to control IRQ
	, typename kIrq9 = IrqTemplate<BogusIrqType>	///< The 9th IrqTemplate<> data-type to control IRQ
	, typename kIrq10 = IrqTemplate<BogusIrqType>	///< The 10th IrqTemplate<> data-type to control IRQ
	, typename kIrq11 = IrqTemplate<BogusIrqType>	///< The 11th IrqTemplate<> data-type to control IRQ
	, typename kIrq12 = IrqTemplate<BogusIrqType>	///< The 12th IrqTemplate<> data-type to control IRQ
	, typename kIrq13 = IrqTemplate<BogusIrqType>	///< The 13th IrqTemplate<> data-type to control IRQ
	, typename kIrq14 = IrqTemplate<BogusIrqType>	///< The 14th IrqTemplate<> data-type to control IRQ
	, typename kIrq15 = IrqTemplate<BogusIrqType>	///< The 15th IrqTemplate<> data-type to control IRQ
	, typename kIrq16 = IrqTemplate<BogusIrqType>	///< The 16th IrqTemplate<> data-type to control IRQ
>
class IrqSet
{
public:
	/// A constant used as bit mask for the IxER[0] registers of the NVIC
	static constexpr uint32_t mask0_ =
		kIrq1::mask0_ | kIrq2::mask0_ | kIrq3::mask0_ | kIrq4::mask0_
		| kIrq5::mask0_ | kIrq6::mask0_ | kIrq7::mask0_ | kIrq8::mask0_
		| kIrq9::mask0_ | kIrq10::mask0_ | kIrq11::mask0_ | kIrq12::mask0_
		| kIrq13::mask0_ | kIrq14::mask0_ | kIrq15::mask0_ | kIrq16::mask0_
		;
	/// A constant used as bit mask for the IxER[1] registers of the NVIC
	static constexpr uint32_t mask1_ =
		kIrq1::mask1_ | kIrq2::mask1_ | kIrq3::mask1_ | kIrq4::mask1_
		| kIrq5::mask1_ | kIrq6::mask1_ | kIrq7::mask1_ | kIrq8::mask1_
		| kIrq9::mask1_ | kIrq10::mask1_ | kIrq11::mask1_ | kIrq12::mask1_
		| kIrq13::mask1_ | kIrq14::mask1_ | kIrq15::mask1_ | kIrq16::mask1_
		;

	/// Access to the NVIC register map
	ALWAYS_INLINE static volatile NVIC_Type & Io() { return *(volatile NVIC_Type*)NVIC_BASE; }

	/// Enables interrupts at once
	ALWAYS_INLINE static void Enable()
	{
		// Ensures that code is generated only if at least one bit is defined in mask0_
		if (mask0_)
			Io().ISER[0U] = mask0_;
		// Ensures that code is generated only if at least one bit is defined in mask1_
		if (mask1_)
			Io().ISER[1U] = mask1_;
	}
	/// Disables interrupts at once
	ALWAYS_INLINE static void Disable()
	{
		// Ensures that code is generated only if at least one bit is defined in mask0_
		if (mask0_)
			Io().ICER[0U] = mask0_;
		// Ensures that code is generated only if at least one bit is defined in mask1_
		if (mask1_)
			Io().ICER[1U] = mask1_;
	}
	/// Disables interrupts at once
	ALWAYS_INLINE static void ClearPending()
	{
		// Ensures that code is generated only if at least one bit is defined in mask0_
		if (mask0_)
			Io().ICPR[0U] = mask0_;
		// Ensures that code is generated only if at least one bit is defined in mask1_
		if (mask1_)
			Io().ICPR[1U] = mask1_;
	}
};

/// Similar to IrqSet<> but handles IRQ enumeration directly
/*!
This template class is similar to IrqSet<> but reduces typing when IRQ type enumeration 
is used.
Example:
\code{.cpp}
void CriticalFunction()
{
	// BLOCK: Disables both interrupts to avoid race conditions
	{
		CriticalSectionIrq<USART1_IRQn, USART2_IRQn> lock;
		// TODO: write your code that is critical, without race conditions with both Comm ISR
	}
	// ENDBLOCK: IRQs are reactivated at the end of the 'lock' scope
}
\endcode
*/
template <
	const IRQn_Type kIrq1						///< The 1st IRQ type enumeration
	, const IRQn_Type kIrq2 = BogusIrqType		///< The 2nd IRQ type enumeration
	, const IRQn_Type kIrq3 = BogusIrqType		///< The 3rd IRQ type enumeration
	, const IRQn_Type kIrq4 = BogusIrqType		///< The 4th IRQ type enumeration
	, const IRQn_Type kIrq5 = BogusIrqType		///< The 5th IRQ type enumeration
	, const IRQn_Type kIrq6 = BogusIrqType		///< The 6th IRQ type enumeration
	, const IRQn_Type kIrq7 = BogusIrqType		///< The 7th IRQ type enumeration
	, const IRQn_Type kIrq8 = BogusIrqType		///< The 8th IRQ type enumeration
	, const IRQn_Type kIrq9 = BogusIrqType		///< The 9th IRQ type enumeration
	, const IRQn_Type kIrq10 = BogusIrqType		///< The 10th IRQ type enumeration
	, const IRQn_Type kIrq11 = BogusIrqType		///< The 11th IRQ type enumeration
	, const IRQn_Type kIrq12 = BogusIrqType		///< The 12th IRQ type enumeration
	, const IRQn_Type kIrq13 = BogusIrqType		///< The 13th IRQ type enumeration
	, const IRQn_Type kIrq14 = BogusIrqType		///< The 14th IRQ type enumeration
	, const IRQn_Type kIrq15 = BogusIrqType		///< The 15th IRQ type enumeration
	, const IRQn_Type kIrq16 = BogusIrqType		///< The 16th IRQ type enumeration
>
class IrqNSet : public IrqSet<
	IrqTemplate<kIrq1>							///< Remaps every template argument to a IrqTemplate template type
	, IrqTemplate<kIrq2>
	, IrqTemplate<kIrq3>
	, IrqTemplate<kIrq4>
	, IrqTemplate<kIrq5>
	, IrqTemplate<kIrq6>
	, IrqTemplate<kIrq7>
	, IrqTemplate<kIrq8>
	, IrqTemplate<kIrq9>
	, IrqTemplate<kIrq10>
	, IrqTemplate<kIrq11>
	, IrqTemplate<kIrq12>
	, IrqTemplate<kIrq13>
	, IrqTemplate<kIrq14>
	, IrqTemplate<kIrq15>
	, IrqTemplate<kIrq16>
> { };

