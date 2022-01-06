#pragma once

#ifdef __MSP430__
#ifndef __MSP430X_LARGE__
// Please make sure that compilation parameter produces 20-bit pointers
#error "Please setup Compiler with the -mlarge option"
#endif
typedef uint16_t * wraddr_t;
#else
typedef uint32_t wraddr_t;
#endif

#define ERASECTLX	__attribute__((aligned(2)))


#pragma pack(1)

struct ERASECTLX EraseCtrlX
{
	// Address to trigger flash erase operation (ARM compatible data-type)
	wraddr_t addr_;
	// Flash unlock word with desired option
	uint16_t fctl1_;
	// Value to program DCO
	uint8_t dcoctl_;
	// Value to program DCO
	uint8_t bcsctl1_;
};

#pragma pack()

