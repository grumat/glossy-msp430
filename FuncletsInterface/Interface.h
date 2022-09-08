#pragma once

#ifdef __MSP430__
// Declare as pointer if possible for funclet compilation
#	ifdef __MSP430X_LARGE__
		typedef uint16_t addr_t;
		typedef uint16_t * addr_large_t;
#	else
		typedef uint16_t *addr_t;
		typedef uint32_t addr_large_t;
#	endif
#else
// For host address pointers aren't pointers anymore
typedef uint16_t addr_t;
typedef uint32_t addr_large_t;
#endif

#define MSP430_ALIGN	__attribute__((aligned(2)))


#pragma pack(1)

struct MSP430_ALIGN EraseCtrlXv2
{
	// Address to trigger flash erase operation
	addr_large_t addr_;
	// FCTL1 unlock word with desired option
	uint16_t fctl1_;
	// FCTL3 unlock word with desired option
	uint16_t fctl3_;
};

struct MSP430_ALIGN WriteCtrlXv2
{
	// Start address for writing words
	uint16_t *addr_;
	// Counter
	addr_large_t cnt_;
	// 
	uint16_t unlock_;
	//
	uint16_t reg_bak1_;
	//
	uint16_t reg_bak3_;
};

#pragma pack()

