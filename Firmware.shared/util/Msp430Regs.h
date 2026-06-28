#pragma once

//! MSP430 CPU register file: the register identifiers plus name <-> index
//! helpers used by the GDB monitor 'regs'/'set' commands.
class Msp430Regs
{
public:
	//! CPU register identifiers R0..R15. PC/SP/SR are the architectural aliases
	//! of R0/R1/R2 (R2/SR also doubles as constant generator 1).
	enum Reg : uint8_t
	{
		kPc  = 0,	//!< R0
		kSp  = 1,	//!< R1
		kSr  = 2,	//!< R2
		kR3  = 3, kR4  = 4,  kR5  = 5,  kR6  = 6,  kR7  = 7,
		kR8  = 8, kR9  = 9,  kR10 = 10, kR11 = 11,
		kR12 = 12, kR13 = 13, kR14 = 14, kR15 = 15,
	};

	//! Number of CPU registers.
	static constexpr unsigned kCount = 16;

	//! Register mnemonic ("PC".."R15") for index \p reg, or "???" if >= kCount.
	static const char *Name(unsigned reg);

	//! Parse a register name to its index: "pc"/"sp"/"sr" (case-insensitive), or
	//! an optional 'r'/'R' prefix followed by a decimal 0..15. Returns -1 on an
	//! unknown name or an out-of-range index.
	static int FromName(const char *name);
};
