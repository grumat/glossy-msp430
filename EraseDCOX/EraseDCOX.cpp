#include <msp430.h>
#include <stdint.h>
#include "../FuncletsInterface/Interface.h"

#ifndef __MSP430X_LARGE__
// Please make sure that compilation parameter produces 20-bit pointers
#error "Please setup Compiler with the -mlarge option"
#endif

void EraseDCOX(EraseCtrlX *ctrl) asm("main") __attribute__((naked, noreturn, optimize("Os"), lower));

/*
** This code is intended to run on RAM and erases the flash memory
** Parameter ctrl is passed on R12.
*/
void EraseDCOX(EraseCtrlX * ctrl)
{
	// Stop WDT
	WDTCTL = WDTPW | WDTHOLD;
	// Set DCO clock speed
	DCOCTL = ctrl->dcoctl_;
	BCSCTL1 = ctrl->bcsctl1_;

	// Enable flash write mode
	FCTL1 = ctrl->fctl1_;
	// Dummy write to start erase
	*ctrl->addr_ = 0xFFFF;
	// Wait until Flash finishes operation
	while (FCTL3 & BUSY)
	{ }
	// clear ERASE+MASS_ERASE bits
	FCTL1 = FWKEY;

	// Lock forever, until controller takes control
#if 0
	// Removed, since compiler ignores the -fPIC/-fPIE compiler flag and generates absolute jump, 
	// instead of the relative jump
	for (;;)
	{ }
#else
	__asm volatile(
		".FOREVER:\r\n"
		"	jmp	.FOREVER\r\n"
		);
#endif
}

#pragma pack()
