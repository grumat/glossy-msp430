#pragma once

// Bus state (typically turns hardware buffers on)
enum class BusState
{
	off = 0,	///< Bus enters Hi-Z
	sbw = 1,	///< Half bus is active
	jtag = 3,	///< Entire bus is active
};

// Sets hardware buffers in tri-state or driving
static inline void SetBusState(const BusState st);
