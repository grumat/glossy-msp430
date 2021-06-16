#pragma once

#include "otherlibs.h"


enum GpioPortId
{
	PA = 0,
	PB = 1,
	PC = 2,
	PD = 3,
	PE = 4,
	PF = 5,
	PG = 6,
	kUnusedPort = -1
};


class McuCore
{
public:
	ALWAYS_INLINE static void Sleep()
	{
		__WFI();
	}
	ALWAYS_INLINE static void Abort()
	{
		__BKPT(255);
	}
	ALWAYS_INLINE static void DisableInterrupts()
	{
		__disable_irq();
	}
	ALWAYS_INLINE static void EnableInterrupts()
	{
		__enable_irq();
	}
	ALWAYS_INLINE static uint32_t GetExceptionNum()
	{
		uint32_t result;
		asm volatile ("mrs %0, ipsr" : "=l"(result) : );
		return result & 0x1ff;
	}
	//! Checks if current MCU context is in a interrupt handler
	ALWAYS_INLINE static bool IsInterrupt()
	{
#if 1
		return GetExceptionNum() != 0;
#else 
		return (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0;
#endif
	}
};


/*!
Backup domain register is protected. This class disables this protection
within it's scope. The object scope controls the transaction.

\example
	// BLOCK: BkpDomainXact scope
	{
		BkpDomainXact xact;
		LseOsc<>::Init(xact);
	}
*/
class BkpDomainXact
{
public:
	BkpDomainXact()
	{
		PWR->CR |= PWR_CR_DBP;
	}
	~BkpDomainXact()
	{
		PWR->CR &= ~PWR_CR_DBP;
	}
};


#if DEBUG
ALWAYS_INLINE void ASSERT(bool expr) { if (expr == false) McuCore::Abort(); }
#else
ALWAYS_INLINE void ASSERT(bool expr) {}
#endif

