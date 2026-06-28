#pragma once

//! This type fits an MSP430X register value.
using address_t = uint32_t;

//! True if \p c is a hexadecimal digit (0-9, A-F, a-f).
constexpr bool ishex(int c)
{
	return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

//! Numeric value of a hex digit \p c (assumes ishex(c)); 0 for non-hex input.
constexpr int hexval(int c)
{
	if (c >= 'a')
		return c - 'a' + 10;
	if (c >= 'A')
		return c - 'A' + 10;
	if (c >= '0')
		return c - '0';
	return 0;
}
