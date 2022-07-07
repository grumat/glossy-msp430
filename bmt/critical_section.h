#pragma once

#include "mcu-system.h"
#include "irq.h"
#include "exti.h"


/// Disables interrupts in a code section controlled by object scope
class CriticalSection
{
public:
	/// Disables all interrupts
	ALWAYS_INLINE CriticalSection()
	{
		McuCore::DisableInterrupts();
	}
	/// Reenable interrupts
	ALWAYS_INLINE ~CriticalSection()
	{
		McuCore::EnableInterrupts();
	}
};


/// Disables interrupts in a code section and restores interrupt flag state on the same condition when entered
/*!
This class is intended for code sections shared by interrupts and the main program.
*/
class CriticalSectionEx
{
public:
	/// Records interrupt flag and disables interrupts
	ALWAYS_INLINE CriticalSectionEx()
	{
		state_ = McuCore::IsInterrupt();
		McuCore::DisableInterrupts();
	}
	/// Restores interrupt flag
	ALWAYS_INLINE ~CriticalSectionEx()
	{
		if(state_ == 0)
			McuCore::EnableInterrupts();
	}
protected:
	/// Stores interrupt state
	bool state_;
};


/// Suspend resume a single interrupt type, allowing to fine-tune critical sections
/*!
Differently than \ref CriticalSection class this class works on selected interrupt, while 
all other interrupts are left working. This is optimal to access a shared resource that 
is critical only for a very specific interrupt type. This improves interrupt latencies 
for unrelated events.
This is a more fine grained option than the use of a giant lock.

\par Example

A \b MyClass class implements a FIFO that is used by an interrupt routine to transfer 
data during a dedicated interrupt handler:

\code{.cpp}
// Interrupt request routine to send next FIFO byte
void MyClass::MyXmitInt()
{
	if(fifo_.HasData())
		Send(fifo_.GetByte())	// remove byte from FIFO and send
	else
		SuspendInterrupt();		// stop from interrupting while no more data
}
\endcode

On the main program FIFO cannot be accessed during interrupt, because it is a shared 
resource. A critical section for that specific interrupt ensures FIFO integrity:

\code{.cpp}
// The interrupt that is critical (see IRQn_Type type for real-world cases)
typedef IrqSet<IrqTemplate<JustAnExample_IRQn>> MyInt;

// Puts a char to the xmit FIFO from the main program
bool MyClass::MyXMit(uint8_t data)
{
	CriticalSectionIrq<MyInt> lock;	// locks interrupt until the end of function
	bool sent = fifo_.Put(data);	// append to FIFO
	EnableInterrupt();				// reenable interrupt as we have at least on byte to xmit
	// Returns false if FIFO is full
	return sent;
}
\endcode
*/
template <typename irq_set>
class CriticalSectionIrq
{
public:
	/// Datatype for the hardware instance
	typedef irq_set IrqHandler;
	/// Disables specific interrupt during object scope
	CriticalSectionIrq() { IrqHandler::Disable(); }
	/// Reenables specific interrupt
	~CriticalSectionIrq() { IrqHandler::Enable(); }
};

