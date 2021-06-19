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


static_assert(_countof(all_mem_infos) < 256, "Bit-field size 'i_refm_' of ChipInfoDB::MemoryInfo is too small to hold items");
static_assert(sizeof(MemoryInfo) == 5, "Changes on ChipInfoDB::MemoryInfo will impact final Flash size");
static_assert(kStart_Max_ < 64, "Bit-field size 'estart_' of ChipInfoDB::MemoryInfo is too small to hold items");
static_assert(kSize_Max_ < 64, "Bit-field size 'esize_' of ChipInfoDB::MemoryInfo is too small to hold items");
static_assert(kClasMax_ < 64, "Bit-field size 'class_' of ChipInfoDB::MemoryClasInfo is too small to hold items");
static_assert(sizeof(MemoryClasInfo) == 2, "Changes on ChipInfoDB::MemoryClasInfo will impact final Flash size");
static_assert(sizeof(Device) == 17, "Changes on ChipInfoDB::Device will impact final Flash size");
static_assert(_countof(msp430_mcus_set) < 1024, "Bit-field size 'i_refd_' of ChipInfoDB::Device is too small to hold items");


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
	if (i_refd_)
		((const Device_ &)(msp430_mcus_set[i_refd_ - 1])).GetID(info);
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
	if (i_refd_)
		((const Device_ &)(msp430_mcus_set[i_refd_ - 1])).Fill(o);
	// Name
	if (name_)
	{
		DecompressChipName(o.name_, name_);
		o.slau_ = MapToChipToSlau(name_);
	}
	// 
	if (psa_ != kNullPsaType)
		o.psa_ = psa_;
	//
	if (arch_ != kNullArchitecture)
	{
		o.arch_ = arch_;
		o.bits_ = (o.arch_ == kCpu) ? k16 : k20;
	}
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
			((const MemoryInfo_ &)all_mem_infos[i_info_]).Fill(ele);
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
	if (i_refm_)
		((const MemoryInfo_ &)all_mem_infos[i_refm_ - 1]).Fill(o);
	//
	if (estart_ != kStart_None)
		o.start_ = from_enum_to_address[estart_];
	//
	if (esize_ != 0)
		o.size_ = from_block_to_size[esize_];
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


#ifdef OPT_IMPLEMENT_TEST_DB
static int qry_level_;
#endif

#if OPT_DEBUG_SCORE_SYSTEM || defined(OPT_IMPLEMENT_TEST_DB)
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
#if OPT_DEBUG_SCORE_SYSTEM || defined(OPT_IMPLEMENT_TEST_DB)
	int max_level = 0;
#endif
	const Device_ *matchlevel[DieInfoEx::kFull] = { NULL };
	for (int i = 0; i < all_msp430_mcus.entries_; ++i)
	{
		const Device_ *dev = (Device_ *)&msp430_mcus_set[all_msp430_mcus.array_[i]];
		info.Clear();
		dev->GetID(info);
		// All devices shall have an id0 (or database corrupt?)
		assert(info.mcu_ver_ != NO_MCU_ID0);
#if OPT_DEBUG_SCORE_SYSTEM || defined(OPT_IMPLEMENT_TEST_DB)
		int l = info.GetMaxLevel();
#if !defined(OPT_IMPLEMENT_TEST_DB)
		if(l == 1)
			Debug() << "MCU: " << dev->name_ << '\n';
#endif
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
#ifdef  OPT_IMPLEMENT_TEST_DB
	qry_level_ = max_level;
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


void ChipProfile::FixDeviceQuirks(const ChipInfoDB::Device *dev)
{
	if (arch_ == kCpuXv2
		&& &msp430_mcus_set[mcu_MSP430F5438] != dev	// The MSP430F5438 is OK, but not it's variants
		&& slau_ != kSLAU259)		// CC430F does not suffer from the 1377 issue
		issue_1377_ = true;
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
	FixDeviceQuirks(dev);
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

	strcpy(name_, "DefaultChip");
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

	strcpy(name_, "DefaultChip");
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

#ifdef OPT_IMPLEMENT_TEST_DB
void TestDB()
{
	for (uint32_t i = 0; i < _countof(all_part_codes); ++i)
	{
		const PartInfo &pi = all_part_codes[i];
		DieInfo qry;
		qry.mcu_ver_	= pi.mcu_ver_;
		qry.mcu_sub_	= pi.mcu_sub_;
		qry.mcu_rev_	= pi.mcu_rev_;
		qry.mcu_fab_	= pi.mcu_fab_;
		qry.mcu_self_	= pi.mcu_self_;
		qry.mcu_cfg_	= pi.mcu_cfg_;
		qry.mcu_fuse_	= pi.mcu_fuse_;
		char buf[ChipProfile::kNameBufCount];
		DecompressChipName(buf, msp430_mcus_set[pi.i_refd_].name_);
		Debug() << "Testing: " << f::S<24>(buf) << "\t";
		ChipProfile chip;
		chip.Init();
		if (!chip.Load(qry))
		{
			Debug() << "Load Failure!\n";
			continue;
		}
		if (strcmp(buf, chip.name_) == 0)
			Debug() << "OK (" << qry_level_ << ")\n";
		else
			Debug() << "Located " << chip.name_ << " instead! (" << qry_level_ << ")\n";
	}
}
#endif
