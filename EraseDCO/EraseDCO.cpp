#include <msp430.h>
#include <stdint.h>

static constexpr uint16_t ChildInit = 0x1111;
static constexpr uint16_t MasterAck = 0x2222;
static constexpr uint16_t ChildEnd = 0xDEAD;

#pragma pack(1)

struct __attribute__((aligned(2))) EraseCtrl
{
	// Semaphore used to interact with JTAG controller (initially 0)
	volatile uint16_t sema_;
	// Value to program DCO
	uint8_t dcoctl_;
	// Value to program DCO
	uint8_t bcsctl1_;
	// Flash unlock word with desired option
	uint16_t fctl1_;
	// Address to trigger flash erase operation
	uint16_t *addr_;
};

void EraseDCO(EraseCtrl *ctrl) asm("main") __attribute__((noreturn));

/*
** This code is intended to run on RAM and erases the flash memory
** Parameter ctrl is passed on R12.
*/
void EraseDCO(EraseCtrl *ctrl)
{
	// Statup
	WDTCTL = WDTPW | WDTHOLD;
	// Set clock
	DCOCTL = ctrl->dcoctl_;
	BCSCTL1 = ctrl->bcsctl1_;

	// Signal JTAG controller
	ctrl->sema_ = ChildInit;
	// Wait for JTAG ack
	while(ctrl->sema_ != MasterAck)
	{ }

	FCTL1 = ctrl->fctl1_;
	// Dummy write to start erase
	*ctrl->addr_ = 0xFFFF;
	// Wait until
	while (FCTL3 & BUSY)
	{ }
	// clear ERASE+MASS_ERASE bits
	FCTL1 = FWKEY;

	// Signal JTAG controller that operation has ended
	ctrl->sema_ = ChildEnd;
	// Lock forever, until controller takes control
	for (;;)
	{ }
}

#pragma pack()
