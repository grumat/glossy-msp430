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

// This is for debug
#ifdef OPT_IMPLEMENT_TEST_DB
#	undef OPTIMIZED
#	define OPTIMIZED
#	undef ALWAYS_INLINE
#	define ALWAYS_INLINE
#endif

// This flag will define data structures to reside here (singletons)...
#define OPT_IMPLEMENT_DB
#include "ChipProfile.h"
#include "bytes.h"

using namespace ChipInfoDB;
using namespace ChipInfoPrivate_;


static_assert(sizeof(MemoryLayout) == 4, "Changes on ChipInfoDB::MemoryLayout will impact final Flash size");
static_assert(sizeof(MemWrProt) == 8, "Changes on ChipInfoDB::MemWrProt will impact final Flash size");
static_assert(sizeof(MemoryBlock) == 3, "Changes on ChipInfoDB::MemoryBlock will impact final Flash size");
static_assert(sizeof(MemConfigHdr) == 1, "Changes on ChipInfoDB::MemConfigHdr will impact final Flash size");
static_assert(sizeof(MemConfigHdrEx) == 2, "Changes on ChipInfoDB::MemConfigHdrEx will impact final Flash size");

static_assert(sizeof(EemTimer) == 2, "Changes on ChipInfoDB::EemTimer will impact final Flash size");
static_assert(sizeof(EtwCodes) == 36, "Changes on ChipInfoDB::EtwCodes will impact final Flash size");

static_assert(sizeof(PrefixResolver) == 2, "Changes on ChipInfoDB::PrefixResolver will impact final Flash size");

static_assert(sizeof(PowerSettings) == 24, "Changes on ChipInfoDB::PowerSettings will impact final Flash size");

static_assert(sizeof(Device) == 9, "Changes on ChipInfoDB::Device will impact final Flash size");


namespace ChipInfoPrivate_
{

class Device_ : public Device
{
public:
	void GetID(DieInfoEx &info) const OPTIMIZED;
	void Fill(ChipProfile &o, EnumMcu idx) const OPTIMIZED;

protected:
	void FillMemory(ChipProfile &o, EnumMemoryConfigs idx) const OPTIMIZED;
};

class MemoryBlock_ : public MemoryBlock
{
public:
	void Fill(ChipProfile &o) const OPTIMIZED;
};

#if 0
class MemoryClasInfo_ : public MemoryClasInfo
{
public:
	void Fill(ChipProfile &o) const OPTIMIZED;
};

class MemoryInfo_ : public MemoryInfo
{
public:
	void Fill(MemInfo &o) const OPTIMIZED;
};
#endif



void Device_::GetID(DieInfoEx &info) const
{
	// Conditionally apply on this level
	info.mcu_ver_ = mcu_ver_;
	// Extract 'sub-version'
	info.mcu_sub_ = DecodeSubversion(mcu_subv_);
	// Extract 'revision'
	info.mcu_rev_ = DecodeRevision(mcu_rev_);
	// Extract 'fab'
	info.mcu_fab_ = DecodeFab(mcu_fab_);
	// Extract 'self'
	info.mcu_self_ = DecodeSelf(mcu_self_);
	// Extract 'config'
	info.mcu_cfg_ = DecodeConfig(mcu_cfg_);
	// Extract 'fuses'
	info.mcu_fuse_ = DecodeFuse(mcu_fuses_);

	// Fuse mask
	info.mcu_fuse_mask = DecodeFuseMask(mcu_fuse_mask_);
	// Config mask
	info.mcu_cfg_mask = (info.mcu_cfg_ != kNoConfig) ? kConfigMask : 0xFF;
}


void Device_::FillMemory(ChipProfile &o, EnumMemoryConfigs idx) const
{
	// Count of blocks in record
	uint8_t cnt;
	// Where list of indexes starts
	const EnumMemoryBlock *pBlk;

	// Locate config by index
	const MemConfigHdr *hdr = GetMemConfig(idx);
	// A inheritance is marked with this flag
	if (hdr->has_base_)
	{
		// Cast to record with inheritance
		const MemConfigHdrEx *hdrx = (const MemConfigHdrEx *)hdr;
		// Data size are appended after header
		cnt = hdrx->count_ - sizeof(MemConfigHdrEx);
		// Start of data for this header type
		pBlk = &hdrx->mem_blocks[0];
		// First, resolve base attributes
		FillMemory(o, hdrx->base_cfg_);
	}
	else
	{
		// Data size are appended after header
		cnt = hdr->count_ - sizeof(MemConfigHdr);
		// Start of data for this header type
		pBlk = &hdr->mem_blocks[0];
	}
	// Fill blocks according to provided data
	for (; cnt; --cnt, ++pBlk)
	{
		// Locate block by index
		const MemoryBlock_ &blk = (const MemoryBlock_ &)GetMemoryBlock(*pBlk);
		blk.Fill(o);
	}
}


void Device_::Fill(ChipProfile &o, EnumMcu idx) const
{
	// Name
	const char *name = symtab_part_names + name_sym_;
	DecompressChipName(o.name_, name);
	o.slau_ = MapToChipToSlau(name);
	//
	o.eem_type_ = eem_type_;
	//
	o.clk_ctrl_ = clock_ctrl_;
	//
	o.issue_1377_ = issue_1377_;
	//
	o.quick_mem_read_ = quick_mem_read_;
	//
	o.arch_ = (idx < kMcuX_) 
		? kCpu : (idx < kMcuXv2_) 
		? kCpuX 
		: kCpuXv2;
	o.bits_ = (o.arch_ == kCpu) ? k16 : k20;
	// Standard for all MSP430 parts
	const MemoryBlock_ &cpu = (const MemoryBlock_ &)GetMemoryBlock(kBlkCpu_0);
	cpu.Fill(o);
	const MemoryBlock_ &eem = (const MemoryBlock_ &)GetMemoryBlock(kBlkEem_0);
	eem.Fill(o);
	// 
	FillMemory(o, mem_config_);
	//
	DecodeEemTimer(o.eem_timers_, eem_timers_);
	//
	if (stop_fll_ != kNoStopFllDbg)
		o.stop_fll_ = stop_fll_;
}


void MemoryBlock_::Fill(ChipProfile &o) const
{
	MemInfo *pTarget = NULL;
	MemInfo *pClear = NULL;
	// Iterate over fixed slot count
	for (int i = 0; i < _countof(o.mem_); ++i)
	{
		MemInfo *pEle = &o.mem_[i];
		// Selects first empty block
		if (pEle->valid_ == false)
		{
			if (pClear == NULL)
				pClear = pEle;
		}
		// Selects a mem class match
		else if (pEle->class_ == memory_key_ && pTarget == NULL)
		{
			pTarget = pEle;
			break;
		}
	}
	// No match found, so use a new empty block
	if (pTarget == NULL)
		pTarget = pClear;

	// No buffer space for memory description.
	// Consider increasing array size for complex memory descriptions
	assert(pTarget != NULL);

	// Update memory block
	pTarget->class_ = memory_key_;
	pTarget->access_type_ = access_type_;
	// Set FRAM flag
	switch (pTarget->access_type_)
	{
	case kAccFramMemoryAccessBase:
	case kAccFramMemoryAccessFRx9:
		o.is_fram_ = true;
		break;
	case kAccFlashTimingGen1:
		o.flash_timings_ = &flash_timing_gen1;
		break;
	case kAccFlashTimingGen2a:
		o.flash_timings_ = &flash_timing_gen2a;
		break;
	case kAccFlashTimingGen2b:
		o.flash_timings_ = &flash_timing_gen2b;
		break;
	}
	pTarget->type_ = memory_type_;
	// Mark block as valid/used
	pTarget->valid_ = true;
	DecodeMemBlock(mem_layout_
				   , pTarget->start_
				   , pTarget->size_
				   , pTarget->segsize_
				   , pTarget->banks_
				   );
	pTarget->bit_size_ = pTarget->class_ == kMkeyPeripheral8bit ? 8 : 16;
	pTarget->mapped_ = pTarget->class_ != kMkeyCpu && pTarget->class_ != kMkeyEem;
	// This flag only affected by SLAU272 & SLAU367 families
	pTarget->access_mpu_
		= (o.slau_ == kSLAU272
		   || o.slau_ == kSLAU367)
		&& (pTarget->class_ == kMkeyInfo
			|| pTarget->class_ == kMkeyMain);
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
	if (mcu_sub_ != kNoSubver)
		lvl += 2;
	// Self
	if (mcu_self_ != kNoSelf)
		++lvl;
	// Revision
	if (mcu_rev_ != kNoRev)
		++lvl;
	// Config
	if (mcu_cfg_ != kNoConfig)
		lvl += 2;
	// Fab
	if (mcu_fab_ != kNoFab)
		++lvl;
	// Fuses
	if (mcu_fuse_ != kNoFuse)
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
		if (mcu_sub_ != DecodeSubversion(kSubver_None))
		{
			ok += 2 * (((qry.mcu_sub_ ^ mcu_sub_) & mask_sub) == 0);
			lvl += 2;
		}
		// Self
		if (mcu_self_ != DecodeSelf(kSelf_None))
		{
			ok += (((qry.mcu_self_ ^ mcu_self_) & mask_self) == 0);
			++lvl;
		}
		// Revision
		if (mcu_rev_ != DecodeRevision(kRev_None))
		{
			ok += (((qry.mcu_rev_ ^ mcu_rev_) & mask_rev) == 0);
			++lvl;
		}
		// Config
		if (mcu_cfg_ != DecodeConfig(kCfg_None))
		{
			const uint16_t mask_cfg = mcu_cfg_mask;
			ok += 2 * (((qry.mcu_cfg_ ^ mcu_cfg_) & mask_cfg) == 0);
			lvl += 2;
		}
		// Fab
		if (mcu_fab_ != DecodeFab(kFab_None))
		{
			ok += (((qry.mcu_fab_ ^ mcu_fab_) & mask_fab) == 0);
			++lvl;
		}
		// Fuses
		if (mcu_fuse_ != DecodeFuse(kFuse_None))
		{
			ok += (((qry.mcu_fuse_ ^ mcu_fuse_) & mcu_fuse_mask) == 0);
			++lvl;
		}
		return (MatchLevel)((3 * ok / lvl) + kLevel0);
	}
	return kNoMatch;
}


EnumMcu ChipProfile::Find(const DieInfo &qry, DieInfoEx &info)
{
#if OPT_DEBUG_SCORE_SYSTEM || defined(OPT_IMPLEMENT_TEST_DB)
	int max_level = 0;
#endif
	const Device *matchlevel[DieInfoEx::kFull] = { NULL };
	for (int i = 0; i < _countof(msp430_mcus_set); ++i)
	{
		const Device_ *dev = (Device_ *)&msp430_mcus_set[i];
		info.Clear();
		dev->GetID(info);
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
		const Device *dev = matchlevel[i];
		if (dev)
			return (EnumMcu)(dev - msp430_mcus_set);
	}
	return kMcu_None;
}


static int cmp(const void *l, const void *r)
{
	const MemInfo *pl = (const MemInfo *)l;
	const MemInfo *pr = (const MemInfo *)r;
	int d = pr->valid_ - pl->valid_;
	if (d != 0)
		return d;
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


void ChipProfile::CompleteLoad()
{
	// since SLAU144 is the default for not specific parts, just need to check for McuXv2, which
	// was not compatible at time.
	if (slau_ == kSLAU144 && arch_ == kCpuXv2)
		slau_ = kSLAU208;	// probably unnecessary, but reasonable.
	qsort(&mem_, _countof(mem_), sizeof(mem_[0]), cmp);
	pwr_settings_ = DecodePowerSettings(slau_);
	switch (eem_type_)
	{
	case kEmexLow:
	case kEmexExtraSmall5xx:
		num_breakpoints = 2;
		break;
	case kEmexMedium:
	case kEmexSmall5xx:
		num_breakpoints = 3;
		break;
	case kEmexMedium5xx:
		num_breakpoints = 5;
		break;
	default:
		num_breakpoints = 8;
		break;
	}
}

bool ChipProfile::Load(const DieInfo &qry)
{
	EnumMcu mcu = Find(qry, mcu_info_);
	if (mcu > kMcu_Last_)
		return false;
	const Device_ &dev = (const Device_ &)msp430_mcus_set[mcu];
	Init();
	dev.Fill(*this, mcu);
	CompleteLoad();
	return true;
}


void ChipProfile::DefaultMcu()
{
	Init();
	((const Device_ &)msp430_mcus_set[kMcu_MSP430F133]).Fill(*this, kMcu_MSP430F133);
	strcpy(name_, "DefaultChip");
	CompleteLoad();
}


void ChipProfile::DefaultMcuX()
{
	Init();
	((const Device_ &)msp430_mcus_set[kMcu_MSP430F2416]).Fill(*this, kMcu_MSP430F2416);
	strcpy(name_, "DefaultChip");
	CompleteLoad();
}


void ChipProfile::DefaultMcuXv2()
{
	Init();
	((const Device_ &)msp430_mcus_set[kMcu_MSP430F5418A]).Fill(*this, kMcu_MSP430F5418A);
	strcpy(name_, "DefaultChip");
	CompleteLoad();
}


const MemInfo *ChipProfile::FindMemByAddress(address_t addr) const
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


const MemInfo &ChipProfile::GetInfoMem() const
{
	// Copy values from kMem_Info_Default as default values
	static constexpr const MemInfo def =
	{
		.class_ = kMkeyInfo,
		.type_ = kMtypFlash,
		.access_type_ = kAccInformationFlashAccess,
		.start_ = 0x1000,
		.size_ = 0x100,
		.segsize_ = 0x40,
		.bit_size_ = 16,
		.banks_ = 4,
		.mapped_ = true,
		.access_mpu_ = false,
		.valid_ = true
	};

	for (int i = 0; i < _countof(mem_); ++i)
	{
		const MemInfo &m = mem_[i];
		if (m.valid_ == false)
			break;
		if (m.class_ == kMkeyInfo)
			return m;
	}
	return def;
}


const MemInfo &ChipProfile::GetMainMem() const
{
	// Copy values from kMem_Main_Flash as default values
	static constexpr const MemInfo def =
	{
		.class_ = kMkeyMain,
		.type_ = kMtypFlash,
		.access_type_ = kAccNone,
		.start_ = 0xc000,
		.size_ = 0x4000,
		.segsize_ = 512,
		.bit_size_ = 16,
		.banks_ = 1,
		.mapped_ = 1,
		.access_mpu_ = 0,
		.valid_ = true
	};

	for (int i = 0; i < _countof(mem_); ++i)
	{
		const MemInfo &m = mem_[i];
		if (m.valid_ == false)
			break;
		if (m.class_ == kMkeyMain)
			return m;
	}
	return def;
}


bool ChipProfile::IsCpuX_ID(uint16_t id)
{
	// there are not many as TI moved to Xv2 and deprecated these cores
	for (uint32_t i = 0; i < _countof(ChipInfoDB::McuXs); ++i)
	{
		if (ChipInfoDB::McuXs[i] == id)
			return true;
	}
	return false;
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
		const char *name = symtab_part_names + msp430_mcus_set[pi.i_refd_].name_sym_;
		DecompressChipName(buf, name);
		Debug() << "Testing: " << f::S<24>(buf) << "\t";
		ChipProfile chip;
		chip.Init();
		if (!chip.Load(qry))
		{
			Debug() << "Load Failure!\n";
			continue;
		}
		if (strcmp(buf, chip.name_) == 0)
			Debug() << "OK (" << qry_level_ << ") [" << chip.issue_1377_ << "]\n";
		else
			Debug() << "Located " << chip.name_ << " instead! (" << qry_level_ << ")\n";
	}
}
#endif
