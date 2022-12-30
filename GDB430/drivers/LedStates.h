#pragma once

/// States for the LED
enum class LedState
{
	off,	///< Turn LED drivers off
	on,		///< Turn LED drivers on (ensures pins are not Hi-Z; color is undefined)
	red,	///< Switch to red color
	green,	///< Switch to green color
};

static inline void SetLedState(const LedState st);

