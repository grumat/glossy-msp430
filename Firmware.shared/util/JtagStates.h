#pragma once


/*!
These is the table of the JTAG state bit durations, for a complete cycle starting
from idle, transfer and back to idle.
Please check JTAG documentation to understand each state label.
JTAG allows to burst more data into the channel without switching back to the idle
state, for improved transfer performance. This is not supported by tis implementation
and also not covered here.
*/
template <const uint8_t kPayloadSize>
struct JtagStates
{
	/*
	This is a typical state sequence of the JTAG state machine
	*/

	// Number of clocks required to enter Select-DR-Scan state
	constexpr static uint8_t kSelectDrScan_ = 1;
	// Number of clocks required to enter Capture-DR state
	constexpr static uint8_t kCaptureDr_ = 2;
	// Number of clocks required to enter Exit1-DR state
	constexpr static uint8_t kExit1Dr_ = 1;
	// Number of clocks required to enter Shift-DR state
	constexpr static uint8_t kShiftDr_ = kPayloadSize - kExit1Dr_;
	// Number of clocks required to enter Update-DR state
	constexpr static uint8_t kUpdateDr_ = 1;

	// Number of clocks required to enter Select-IR-Scan state
	constexpr static uint8_t kSelectIrScan_ = 2;
	// Number of clocks required to enter Capture-IR state
	constexpr static uint8_t kCaptureIr_ = 2;
	// Number of clocks required to enter Exit1-IR state
	constexpr static uint8_t kExit1Ir_ = 1;
	// Number of clocks required to enter Shift-IR state
	constexpr static uint8_t kShiftIr_ = kPayloadSize - kExit1Ir_;
	// Number of clocks required to enter Update-IR state
	constexpr static uint8_t kUpdateIr_ = 1;

	// Number of clocks required to enter Run-Test/Idle state
	constexpr static uint8_t kIdle_ = 1;
};
