#include <msp430.h>
#include <stdint.h>
#include "../FuncletsInterface/Interface.h"

void EraseDCO(EraseCtrl *ctrl) asm("main") __attribute__((naked, noreturn, optimize("Os")));

/*
** This code is intended to run on RAM and erases the flash memory
** Parameter ctrl is passed on R12.
*/
void EraseDCO(EraseCtrl *ctrl)
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
	for (;;)
	{ }
}

#pragma pack()
