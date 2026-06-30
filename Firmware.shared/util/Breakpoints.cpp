#include "stdproj.h"

#include "Breakpoints.h"
#include "drivers/ITapDev.h"



void Breakpoints::Clear()
{
	for (DeviceBreakpoint &bp : breakpoints_)
		bp = DeviceBreakpoint();
	fSwBkp_ = true;
}


BkptId Breakpoints::Add(const ChipProfile &prof, address_t addr, DeviceBpType type)
{
	BkptId sel = BkptId::kInvalidBkpt;

	const int last = GetCount(prof);

	// First entry is used to implement software breakpoints
	for(int i = fSwBkp_ ; i < last ; ++i)
	{
		DeviceBreakpoint &bp = breakpoints_[i];
		// In use?
		if (bp.fEnabled)
		{
			// Match breakpoint?
			if (bp.addr == addr && bp.type == type)
				return BkptId(i);
		}
		else if (sel == BkptId::kInvalidBkpt)
			sel = BkptId(i);	// get first free entry
	}

	// Entry found?
	if (sel != BkptId::kInvalidBkpt)
	{
		// Up to 20 breakpoints; other types can only be implemented using HW
		if (type == DeviceBpType::kBpTypeBreak || int(sel) < prof.numBreakpoints)
		{
			// Populate entry
			breakpoints_[int(sel)] =
			{ 
				{
					.type = type,
					.fEnabled = true,
					.fDirty = true,
					.fDataFetch = false,
					.fIsSw = int(sel) < prof.numBreakpoints
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
				.type = type,
				.fEnabled = enabled,
				.fDirty = true,
				.fDataFetch = false,
				.fIsSw = int(which) < prof.numBreakpoints
			},
			addr
		};
	}
	return which;
}


BkptId Breakpoints::Remove(const ChipProfile &prof, address_t addr, DeviceBpType type)
{
	const int last = GetCount(prof);
	for (int i = fSwBkp_; i < last; ++i)
	{
		DeviceBreakpoint &bp = breakpoints_[i];
		// In use?
		if (bp.fEnabled
			&& bp.addr == addr
			&& bp.type == type)
		{
			bp.fEnabled = false;
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
	for (int i = fSwBkp_; i < last; ++i)
	{
		const DeviceBreakpoint &bp = breakpoints_[i];
		if(bp.fEnabled)
		{
			/*
			** Two array ranges:
			**		0 --> prof.numBreakpoints						: Hardware Breakpoints
			**		prof.numBreakpoints --> _countof(breakpoints_)	: Software Breakpoints
			*/
			if (i < prof.numBreakpoints)
				mask |= (uint16_t)(1U << i);
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
	if (fSwBkp_)
	{
		if (uses_sw)
		{
			// Initializes a SW breakpoint ctrl
			static constexpr DeviceBreakpoint swbp_dirty = 
			{ 
				{
					.type = DeviceBpType::kBpTypeBreak,
					.fEnabled = true,
					.fDirty = true,
					.fDataFetch = true,
					.fIsSw = false
				},
				kSwBkpInstr // instr to be fetched
			};
			// Already initialized sw ctrl
			static constexpr DeviceBreakpoint swbp_set = 
			{ 
				{
					.type = DeviceBpType::kBpTypeBreak,
					.fEnabled = true,
					.fDataFetch = true,
					.fIsSw = false
				},
				kSwBkpInstr // instr to be fetched
			};
			// Update software breakpoint control entry?
			if (breakpoints_[0] != swbp_set)
				breakpoints_[0] = swbp_dirty;
			mask |= 1; // updates HW register
		}
		else
			breakpoints_[0].fEnabled = false;
	}
	// Mask to be programmed into BREAKREACT
	return mask;
}

