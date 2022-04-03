#include "stdproj.h"

#include "Breakpoints.h"



void Breakpoints::ctor()
{
	// Same effect as looping `(*this)[i].ctor()`
	memset(this, 0, sizeof(*this));
	sw_bkp_ = true;
}


BkptId Breakpoints::Add(const ChipProfile &prof, address_t addr, DeviceBpType type)
{
	BkptId sel = BkptId::kInvalidBkpt;

	const int last = GetCount(prof);

	// First entry is used to implement software breakpoints
	for(int i = sw_bkp_ ; i < last ; ++i)
	{
		DeviceBreakpoint &bp = breakpoints_[i];
		// In use?
		if (bp.enabled_)
		{
			// Match breakpoint?
			if (bp.addr_ == addr && bp.type_ == type)
				return BkptId(i);
		}
		else if (sel == BkptId::kInvalidBkpt)
			sel = BkptId(i);	// get first free entry
	}

	// Entry found?
	if (sel != BkptId::kInvalidBkpt)
	{
		// Up to 20 breakpoints; other types can only be implemented using HW
		if (type == DeviceBpType::kBpTypeBreak || int(sel) < prof.num_breakpoints_)
		{
			// Populate entry
			breakpoints_[int(sel)] =
			{ 
				{
					.type_ = type,
					.enabled_ = true,
					.dirty_ = true,
					.datafetch_ = false,
					.is_sw_ = int(sel) < prof.num_breakpoints_
				},
				addr
			};
		}
		else
		{
			// Watch, Read and Write breaks can only be implemented using HW
			sel = BkptId::kInvalidBkpt;	
		}
	}
	return sel;
}

/*!
Set or clear a breakpoint. The index of the modified entry is returned, or -1 if 
no free entries were available. The modified entry is flagged so that it will be 
reloaded on the next run.
	
If which is specified, a particular breakpoint slot is modified. Otherwise, if 
which < 0, breakpoint slots are selected automatically.
*/
BkptId Breakpoints::Set(const ChipProfile &prof, address_t addr, DeviceBpType type, bool enabled, BkptId which)
{
	if (which == BkptId::kInvalidBkpt)
	{
		if (enabled)
			return Add(prof, addr, type);
		which = Remove(prof, addr, type);
	}
	else
	{
		breakpoints_[int(which)] =
		{ 
			{
				.type_ = type,
				.enabled_ = enabled,
				.dirty_ = true,
				.datafetch_ = false,
				.is_sw_ = int(which) < prof.num_breakpoints_
			},
			addr
		};
	}
	return which;
}


BkptId Breakpoints::Remove(const ChipProfile &prof, address_t addr, DeviceBpType type)
{
	const int last = GetCount(prof);
	for (int i = sw_bkp_; i < last; ++i)
	{
		DeviceBreakpoint &bp = breakpoints_[i];
		// In use?
		if (bp.enabled_
			&& bp.addr_ == addr
			&& bp.type_ == type)
		{
			bp.enabled_ = false;
			return BkptId(i);
		}		
	}
	return BkptId::kInvalidBkpt;
}


uint16_t Breakpoints::PrepareEemSetup(const ChipProfile &prof)
{
	bool uses_sw = false;
	uint16_t mask = 0;
	// last breakpoint
	const int last = GetCount(prof);
	// Builds a mask used to program EEM and check for SW breakpoint occurrence
	for (int i = sw_bkp_; i < last; ++i)
	{
		const DeviceBreakpoint &bp = breakpoints_[i];
		if(bp.enabled_)
		{
			/*
			** Two array ranges:
			**		0 --> prof.num_breakpoints_							: Hardware Breakpoints
			**		prof.num_breakpoints_ --> _countof(breakpoints_)	: Software Breakpoints
			*/
			if (i < prof.num_breakpoints_)
				mask |= (1 << i);
			else
			{
				// Found a least one SW BKPT
				uses_sw = true;
				// Just the first SW BKPT is enough
				break;
			}
		}
	}
	// Software BKPT are fired when instruction 0x4343 (NOP variant) is fetched
	if (sw_bkp_)
	{
		if (uses_sw)
		{
			// Initializes a SW breakpoint ctrl
			static constexpr DeviceBreakpoint swbp_dirty = 
			{ 
				{
					.type_ = DeviceBpType::kBpTypeBreak,
					.enabled_ = true,
					.dirty_ = true,
					.datafetch_ = true,
					.is_sw_ = false
				},
				kSwBkpInstr // instr to be fetched
			};
			// Already initilized sw ctrl
			static constexpr DeviceBreakpoint swbp_set = 
			{ 
				{
					.type_ = DeviceBpType::kBpTypeBreak,
					.enabled_ = true,
					.datafetch_ = true,
					.is_sw_ = false
				},
				kSwBkpInstr // instr to be fetched
			};
			// Update software breakpoint control entry?
			if (breakpoints_[0] != swbp_set)
				breakpoints_[0] = swbp_dirty;
			mask |= 1; // updates HW register
		}
		else
			breakpoints_[0].enabled_ = false;
	}
	// Mask to be programmed into BREAKREACT
	return mask;
}

