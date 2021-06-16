/* MSPDebug - debugging tool for MSP430 MCUs
 * Copyright (C) 2009-2013 Daniel Beer
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */


#include "stdproj.h"

#define OPT_IMPLEMENT_DB
#include "ChipProfile.h"
#include "bytes.h"


using namespace ChipInfoDB;
using namespace ChipInfoPrivate_;


namespace ChipInfoPrivate_
{

class Device_ : public Device
{
public:
	void GetID(DieInfoEx &info) const;
	void Fill(ChipProfile &o) const;
};

class MemoryLayoutInfo_ : public MemoryLayoutInfo
{
public:
	void Fill(ChipProfile &o) const;
};

class MemoryClasInfo_ : public MemoryClasInfo
{
public:
	void Fill(ChipProfile &o) const;
};

class MemoryInfo_ : public MemoryInfo
{
public:
	void Fill(MemInfo &o) const;
};


void Device_::GetID(DieInfoEx &info) const
{
	// Resolve reference before current record
	if (ref_)
		((Device_ *)ref_)->GetID(info);
	// Pointer to extract record information
	const uint8_t *misc = &mcu_misc_0;
	// Conditionally apply on this level
	if (mcu_ver_ != NO_MCU_ID0)
		info.mcu_ver_ = mcu_ver_;
	// Fuse mask
	if (mcu_fuse_mask != kFuseNoMask)
		info.mcu_fuse_mask = mcu_fuse_mask;
	// Config mask
	if (mcu_cfg_mask != kCfgNoMask)
		info.mcu_cfg_mask = mcu_cfg_mask;
	// Extract sub-version on demand
	if (mcu_sub_f != kNoSubversion)
	{
		info.mcu_sub_f = kUseSubversion;
		info.mcu_sub_ = r16le(misc);
		misc += 2;
	}
	// Extract 'self' on demand
	if (mcu_self_f != kNoSelf)
	{
		info.mcu_self_f = kUseSelf;
		info.mcu_self_ = r16le(misc);
		misc += 2;
	}
	// Extract 'revision' on demand
	if (mcu_rev_f != kNoRevision)
	{
		info.mcu_rev_f = kUseRevision;
		info.mcu_rev_ = *misc++;
	}
	// Extract 'config' on demand
	if (mcu_cfg_f != kNoConfig)
	{
		info.mcu_cfg_f = kUseConfig;
		info.mcu_cfg_ = *misc++;
	}
	// Extract 'fab' on demand
	if (mcu_fab_f != kNoFab)
	{
		info.mcu_fab_f = kUseFab;
		info.mcu_cfg_ = *misc++;
	}
	// Extract 'fuses' on demand
	if (mcu_fuse_f != kNoFuses)
	{
		info.mcu_fuse_f = kUseFuses;
		info.mcu_fuse_ = *misc++;
	}
}


void Device_::Fill(ChipProfile &o) const
{
	// Resolve reference before current record
	if (ref_)
		((Device_ *)ref_)->Fill(o);
	// Name
	if (name_)
		o.name_ = name_;
	// 
	if (psa_ != kNullPsaType)
		o.psa_ = psa_;
	//
	if (bits_ != kNullBitSize)
		o.bits_ = bits_;
	//
	if (arch_ != kNullArchitecture)
		o.arch_ = arch_;
	//
	if (mem_layout_)
		((MemoryLayoutInfo_ *)mem_layout_)->Fill(o);
}


void MemoryLayoutInfo_::Fill(ChipProfile &o) const
{
	// Resolve reference before current record
	if (ref_)
		((MemoryLayoutInfo_ *)ref_)->Fill(o);
	// Override with current data
	for (int i = 0; i < entries_; ++i)
	{
		const MemoryClasInfo_ &ele = (const MemoryClasInfo_ &)array_[i];
		ele.Fill(o);
	}
}


void MemoryClasInfo_::Fill(ChipProfile &o) const
{
	// Iterate over fixed slot count
	for (int i = 0; i < _countof(o.mem_); ++i)
	{
		MemInfo &ele = o.mem_[i];
		if (ele.valid_ == false			// empty block
			|| (ele.class_ == class_))	// match memory class
		{
			// Update memory block
			ele.class_ = class_;
			((const MemoryInfo_ *)info_)->Fill(ele);
			// Set FRAM flag
			if ((ele.access_type_ == kFramMemoryAccessBase)
				|| (ele.access_type_ == kFramMemoryAccessFRx9))
				o.is_fram_ = true;
			// mark block as valid/used
			ele.valid_ = true;
			return;
		}
	}
	// No buffer space for memory description.
	// Consider increasing array size for complex memory descriptions
	assert(false);
}


void MemoryInfo_::Fill(MemInfo &o) const
{
	if (ref_)
		((const MemoryInfo_ *)ref_)->Fill(o);
	//
	if (start_ != NO_MEM_START)
		o.start_ = start_;
	//
	if (size_ != 0)
		o.size_ = size_;
	//
	if (type_ != kNullMemType)
		o.type_ = type_;
	//
	if (bit_size_ != 0)
		o.bit_size_ = bit_size_;
	//
	if (banks_ != 0)
		o.banks_ = banks_;
	//
	if (mapped_ != 0)
		o.mapped_ = mapped_;
	//
	if (access_type_ != kNullMemAccess)
	{
		o.access_type_ = access_type_;
		o.access_mpu_ = access_mpu_;
	}
}


}	// namespace ChipInfoPrivate_


void DieInfoEx::Clear()
{
	memset(this, 0xFF, sizeof(DieInfo));
	raw_ = 0;
}


#if OPT_DEBUG_SCORE_SYSTEM
int DieInfoEx::GetMaxLevel() const
{
	int lvl = 0;
	// Subversion
	if (mcu_sub_f)
		lvl += 2;
	// Self
	if (mcu_self_f)
		++lvl;
	// Revision
	if (mcu_rev_f)
		++lvl;
	// Config
	if (mcu_cfg_f)
		lvl += 2;
	// Fab
	if (mcu_fab_f)
		++lvl;
	// Fuses
	if (mcu_fuse_f)
		++lvl;
	return lvl;
}
#endif


DieInfoEx::MatchLevel DieInfoEx::Match(const DieInfo &qry) const
{
	constexpr static uint16_t mask_ver = 0xFFFF;
	constexpr static uint16_t mask_sub = 0xFFFF;
	constexpr static uint16_t mask_self = 0xFFFF;
	constexpr static uint16_t mask_rev = 0xFF;
	constexpr static uint16_t mask_fab = 0xFF;
	// Match by main id
	if (((qry.mcu_ver_ ^ mcu_ver_) & mask_ver) == 0)
	{
		int lvl = 0;
		int ok = 0;
		// Subversion
		if (mcu_sub_f)
		{
			ok += 2 * (((qry.mcu_sub_ ^ mcu_sub_) & mask_sub) == 0);
			lvl += 2;
		}
		// Self
		if (mcu_self_f)
		{
			ok += (((qry.mcu_self_ ^ mcu_self_) & mask_self) == 0);
			++lvl;
		}
		// Revision
		if (mcu_rev_f)
		{
			ok += (((qry.mcu_rev_ ^ mcu_rev_) & mask_rev) == 0);
			++lvl;
		}
		// Config
		if (mcu_cfg_f)
		{
			const uint16_t mask_cfg = (mcu_cfg_mask == kCfg7F) ? 0x7F : 0xFF;
			ok += 2 * (((qry.mcu_cfg_ ^ mcu_cfg_) & mask_cfg) == 0);
			lvl += 2;
		}
		// Fab
		if (mcu_fab_f)
		{
			ok += (((qry.mcu_fab_ ^ mcu_fab_) & mask_fab) == 0);
			++lvl;
		}
		// Fuses
		if (mcu_fuse_f)
		{
			static const uint8_t mask[] =
			{
				0x0f
				, 0x1f
				, 0x07
				, 0x03
				, 0x01
			};
			ok += (((qry.mcu_fuse_ ^ mcu_fuse_) & mask[mcu_fuse_mask]) == 0);
			++lvl;
		}
		return (MatchLevel)((3 * ok / lvl) + kLevel0);
	}
	return kNoMatch;
}


const Device_ *ChipProfile::Find(const DieInfo &qry, DieInfoEx &info)
{
#if OPT_DEBUG_SCORE_SYSTEM
	int max_level = 0;
#endif
	const Device_ *matchlevel[DieInfoEx::kFull] = { NULL };
	for (int i = 0; i < all_msp430_mcus.entries_; ++i)
	{
		const Device_ *dev = (Device_ *)all_msp430_mcus.array_[i];
		info.Clear();
		dev->GetID(info);
		// All devices shall have an id0 (or database corrupt?)
		assert(info.mcu_ver_ != NO_MCU_ID0);
#if OPT_DEBUG_SCORE_SYSTEM
		int l = info.GetMaxLevel();
		if(l == 1)
			Debug() << "MCU: " << dev->name_ << '\n';
		if (l > max_level)
			max_level = l;
#endif
		// Match record
		DieInfoEx::MatchLevel lvl = info.Match(qry);
		if (lvl != DieInfoEx::kNoMatch)
			matchlevel[lvl - 1] = dev;
	}
#if OPT_DEBUG_SCORE_SYSTEM
	Debug() << "Max system match level: " << max_level << '\n';
#endif
	for (int i = _countof(matchlevel); --i >= 0;)
	{
		const Device_ *dev = matchlevel[i];
		if (dev)
			return dev;
	}
	return NULL;
}


static int cmp(const void *l, const void *r)
{
	const MemInfo *pl = (const MemInfo *)l;
	const MemInfo *pr = (const MemInfo *)r;
	if (pl->start_ < pr->start_)
		return -1;
	else if (pl->start_ > pr->start_)
		return 1;
	else if (pl->size_ < pr->size_)
		return -1;
	else if (pl->size_ > pr->size_)
		return 1;
	else
		return pr->class_ - pl->class_;
}


void ChipProfile::UpdateFastFlash()
{
	is_fast_flash_ = false;
	// Does not affect MSP430Xv2
	if (arch_ == kCpuXv2)
		return;
	// Applies to some specific MSP430 & MSP430X parts
	switch (mcu_info_.mcu_ver_)
	{
	case 0x01f2:	// MSP430F2013
	case 0x13f2:	// MSP430F2131 family
	case 0x27f2:	// MSP430F2274 family
	case 0x37f2:	// MSP430F2370 family
	case 0x49f2:	// MSP430F249 family
	case 0x6FF2:	// MSP430F2619 family
	case 0x6FF4:	// MSP430FG4619 family
		is_fast_flash_ = true;
		break;
	}
}


bool ChipProfile::Load(const DieInfo &qry)
{
	const Device_ *dev = Find(qry, mcu_info_);
	if (dev == NULL)
		return false;
	Init();
	dev->Fill(*this);
	// Size of array
	int cnt = 0;
	for (; cnt < _countof(mem_); ++cnt)
		if (mem_[cnt].valid_ == false)
			break;
	qsort(&mem_, cnt, sizeof(mem_[0]), cmp);
	UpdateFastFlash();
	return true;
}


void ChipProfile::DefaultMcu()
{
	static const MemInfo def_mem[] =
	{
		{
			.class_ = kClasMain,
			.start_ = 0x01000,
			.size_ = 0xF000,
			.type_ = kFlash,
			.bit_size_ = 16,
			.banks_ = 1,
			.mapped_ = 1,
			.access_type_ = kFlashMemoryAccess2ByteAligned,
			.access_mpu_ = 0,
			.valid_ = 1,
		},
		{
			.class_ = kClasRam,
			.start_ = 0x00000,
			.size_ = 0x01000,
			.type_ = kRam,
			.bit_size_ = 16,
			.banks_ = 1,
			.mapped_ = 1,
			.access_type_ = kNullMemAccess,
			.access_mpu_ = 0,
			.valid_ = 1,
		},
	};

	name_ = "DefaultChip";
	mcu_info_.Clear();
	psa_ = kRegular;
	bits_ = k16;
	arch_ = kCpu;
	is_fram_ = false;
	is_fast_flash_ = false;
	memcpy(&mem_, &def_mem, sizeof(def_mem));
}


void ChipProfile::DefaultMcuXv2()
{
	// TODO: apply realistic values
	static const MemInfo def_mem[] =
	{
		{
			.class_ = kClasMain,
			.start_ = 0x01000,
			.size_ = 0xF000,
			.type_ = kFlash,
			.bit_size_ = 20,
			.banks_ = 1,
			.mapped_ = 1,
			.access_type_ = kFlashMemoryAccess2ByteAligned,
			.access_mpu_ = 0,
			.valid_ = 1,
		},
		{
			.class_ = kClasRam,
			.start_ = 0x00000,
			.size_ = 0x01000,
			.type_ = kRam,
			.bit_size_ = 20,
			.banks_ = 1,
			.mapped_ = 1,
			.access_type_ = kNullMemAccess,
			.access_mpu_ = 0,
			.valid_ = 1,
		},
	};

	name_ = "DefaultChip";
	mcu_info_.Clear();
	psa_ = kRegular;
	bits_ = k20;
	arch_ = kCpu;
	is_fram_ = false;
	is_fast_flash_ = false;
	memcpy(&mem_, &def_mem, sizeof(def_mem));
}


const MemInfo *ChipProfile::FindMemByAddress(address_t addr)
{
	for (int i = 0; i < _countof(mem_); ++i)
	{
		const MemInfo &m = mem_[i];
		if (m.valid_ == false)
			break;
		if (m.start_ <= addr && addr <= (m.start_ + m.size_))
			return &m;
	}
	return NULL;
}

