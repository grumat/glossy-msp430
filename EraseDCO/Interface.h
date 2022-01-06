#pragma once

#ifdef __MSP430__
typedef uint16_t *wraddr_t;
#else
typedef uint16_t wraddr_t;
#endif


#pragma pack(1)

struct __attribute__((aligned(2))) EraseCtrl
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

