#pragma once

#include "util.h"
#include "ChipProfile.h"

// Total breakpoints (HW+SW)
static constexpr uint32_t kMaxBreakpoints = 20;
// Instruction used as software breakpoint
static constexpr uint16_t kSwBkpInstr = 0x4343;


enum class DeviceBpType : uint8_t
{
	// A CPU breakpoint
	kBpTypeBreak,
	kBpTypeWatch,
	kBpTypeRead,
	kBpTypeWrite
};

#if 0
#define DEVICE_BP_ENABLED       0x01
#define DEVICE_BP_DIRTY         0x02
#endif

struct DeviceBreakpointAttr
{
	// Type of breakpoint
	DeviceBpType type_;
	// Breakpoint is enabled flag
	uint8_t enabled_ : 1;
	// Breakpoint fired when data is fetched (address otherwise)
	uint8_t datafetch_ : 1;
	// If a kBpTypeBreak type is implemented via software
	uint8_t is_sw_ : 1;
};

// A single breakpoint entry
class DeviceBreakpoint : public DeviceBreakpointAttr
{
public:
	// Address for the breakpoint (or Data for datafetch_ == true)
	address_t addr_;
};


// Breakpoint identifier
enum class BkptId : int
{
	kInvalidBkpt = -1,
};

// Collection of breakpoints
class Breakpoints
{
public:
	// Constructor
	void ctor();
	void Clear() { ctor(); }
	int GetCount(const ChipProfile &prof)
	{
		return sw_bkp_ ? _countof(breakpoints_) : prof.num_breakpoints_;
	}
	// Array access
	const DeviceBreakpoint &operator[](BkptId id) const
	{
		return breakpoints_[int(id)];
	}
	// Writable Array access
	DeviceBreakpoint &operator[](BkptId id)
	{
		return breakpoints_[int(id)];
	}
	// Add a breakpoint
	BkptId Add(const ChipProfile &prof, address_t addr, DeviceBpType type);
	// GDB-like breakpoint management
	BkptId Set(const ChipProfile &prof, address_t addr, DeviceBpType type, bool enabled, BkptId which = BkptId::kInvalidBkpt);
	// Disable Breakpoint
	bool Remove(const ChipProfile &prof, BkptId id)
	{
		const int last = sw_bkp_ ? _countof(breakpoints_) : prof.num_breakpoints_;
		if (id == BkptId::kInvalidBkpt
			|| int(id) >= last)
			return false;
		breakpoints_[int(id)].enabled_ = false;
		return true;
	}
	// Remove brteakpoint by address
	BkptId Remove(const ChipProfile &prof, address_t addr, DeviceBpType type);
	// Prepare breakpoint hardware setup
	uint16_t PrepareEemSetup(const ChipProfile &prof);
	// Locate first match
	BkptId FindAddress(address_t addr, BkptId after_id = BkptId::kInvalidBkpt)
	{
		// start after input argument
		for (int i = int(after_id) + 1; i < _countof(breakpoints_); ++i)
		{
			const DeviceBreakpoint &bp = breakpoints_[i];
			// enabled breakpoint with address?
			if (bp.enabled_
				&& bp.addr_ == addr)
				return BkptId(i);
		}
		return BkptId::kInvalidBkpt;
	}

protected:
	// Breakpoint table
	DeviceBreakpoint breakpoints_[kMaxBreakpoints];
	// Use software breakpoints
	bool sw_bkp_;
};

