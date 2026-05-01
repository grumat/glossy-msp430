#pragma once


namespace JtagFrame
{


/**
	* JTAG scan operation type
	* Determines whether to scan Data Register (DR) or Instruction Register (IR)
	*/
enum class Scan : uint8_t
{
	kDR = 0,		///< Data Register scan (JTAG state: Shift-DR)
	kIR = 1,		///< Instruction Register scan (JTAG state: Shift-IR)
	kGoIdle,		///< Forces JTAG TAP to Run-Test/Idle state (5 TMS=1 cycles)
};

/**
	* Number of bits to shift in a JTAG scan operation
	* Supports common MSP430 bit widths and special GoIdle operation
	*/
enum class NumBits : uint8_t
{
	kGoIdle = 0,	///< Special value for GoIdle operation (not a bit count)
	k8 = 8,		///< 8-bit data transfer (common for byte operations)
	k16 = 16,	///< 16-bit data transfer (MSP430 word size)
	k20 = 20,	///< 20-bit data transfer (MSP430 extended addressing)
	k32 = 32,	///< 32-bit data transfer (double word operations)
};


} // namespace JtagFrame
