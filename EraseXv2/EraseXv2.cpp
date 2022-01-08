#include <msp430.h>
#include <stdint.h>
#include "../FuncletsInterface/Interface.h"

#ifndef __MSP430X_LARGE__
// Please make sure that compilation parameter produces 20-bit pointers
#error "Please setup Compiler with the -mlarge option"
#endif

void EraseXv2(EraseCtrlXv2 *ctrl) asm("main") __attribute__((naked, noreturn, optimize("Os"), lower));

/*
** This code is intended to run on RAM and erases the flash memory
** Parameter ctrl is passed on R12.
** Stack pointer needs to be initialized and have 4 bytes space
*/
void EraseXv2(EraseCtrlXv2 * ctrl)
{
	// Stop WDT
	WDTCTL = WDTPW | WDTHOLD;
	// Backup Flash controller settings
	uint16_t old_fctl1 = FCTL1;
	uint16_t old_fctl3 = FCTL3;	// extra measure to protect INFOA

	// Wait for Flash idle
	while (FCTL3 & BUSY)
	{ }

	FCTL3 = ctrl->fctl3_;
	// LOCKA bit is a toggle bit. Thus, values may not match and we need to provide the toggle...
	if((uint8_t)FCTL3 != (uint8_t)ctrl->fctl3_)
		FCTL3 = ctrl->fctl3_ | LOCKA;
	// Enable flash write mode
	FCTL1 = ctrl->fctl1_;
	// Dummy write to start erase
	*ctrl->addr_ = 0xFFFF;
	// Wait until Flash finishes operation
	while (FCTL3 & BUSY)
	{ }
	// Restore flash controller (with correct password)
	FCTL1 = old_fctl1 ^ (FRPW ^ FWPW);
	// The XOR will change read mode password to write mode password
	old_fctl3 ^= (FRPW ^ FWPW);
	FCTL3 = old_fctl3;
	// Again apply toggle LOCKA bit to produce the expected state
	if ((uint8_t)FCTL3 != (uint8_t)old_fctl3)
		FCTL3 = old_fctl3 | LOCKA;

	// Notifies controller that operation has completed
	SYSJMBO1 = 0xCAFE;
	SYSJMBO0 = 0xBABE;

	// Loop forever, until controller takes control
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
