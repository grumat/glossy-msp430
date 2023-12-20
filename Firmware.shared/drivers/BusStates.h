#pragma once

enum class BusState
{
	off = 0,	///< Bus enters Hi-Z
	sbw = 1,	///< Half bus is active
	jtag = 3,	///< Entire bus is active
};

static inline void SetBusState(const BusState st);
