#pragma once

#include "mcu-system.h"
#include "irq.h"


class CriticalSection
{
public:
	ALWAYS_INLINE CriticalSection()
	{
		McuCore::DisableInterrupts();
	}
	ALWAYS_INLINE ~CriticalSection()
	{
		McuCore::EnableInterrupts();
	}
};


class CriticalSectionEx
{
public:
	ALWAYS_INLINE CriticalSectionEx()
	{
		state_ = McuCore::IsInterrupt();
		McuCore::DisableInterrupts();
	}
	ALWAYS_INLINE ~CriticalSectionEx()
	{
		if(state_ == 0)
			McuCore::EnableInterrupts();
	}
protected:
	bool state_;
};


//! Groups a set of interrupts for suspend/resume
template <typename irq_set>
class CriticalSectionIrq
{
public:
	CriticalSectionIrq() { irq_set::Disable(); }
	~CriticalSectionIrq() { irq_set::Enable(); }
};

