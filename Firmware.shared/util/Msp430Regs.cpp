#include "stdproj.h"

#include "Msp430Regs.h"


const char *Msp430Regs::Name(unsigned reg)
{
	static const char *const kNames[kCount] =
	{
		"PC", "SP", "SR", "R3", "R4", "R5", "R6", "R7",
		"R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15",
	};
	return reg < kCount ? kNames[reg] : "???";
}


int Msp430Regs::FromName(const char *name)
{
	// These mnemonics are short and constant — a direct char compare is both
	// faster and clearer than strcasecmp.
	auto lower = [](char c) -> char { return (c >= 'A' && c <= 'Z') ? char(c + ('a' - 'A')) : c; };

	if (name[0] == 0)
		return -1;

	// Two-letter architectural aliases: pc / sp / sr.
	const char c0 = lower(name[0]);
	const char c1 = lower(name[1]);
	if (c1 != 0 && name[2] == 0)
	{
		if (c0 == 'p' && c1 == 'c')
			return kPc;
		if (c0 == 's' && c1 == 'p')
			return kSp;
		if (c0 == 's' && c1 == 'r')
			return kSr;
		// not an alias — fall through (e.g. "r5")
	}

	// Optional 'r' prefix, then a decimal index 0..15.
	const char *p = (c0 == 'r') ? name + 1 : name;
	if (*p == 0)
		return -1;
	int idx = 0;
	for (; *p != 0; ++p)
	{
		if (*p < '0' || *p > '9')
			return -1;
		idx = idx * 10 + (*p - '0');
		if (idx >= static_cast<int>(kCount))
			return -1;
	}
	return idx;
}
