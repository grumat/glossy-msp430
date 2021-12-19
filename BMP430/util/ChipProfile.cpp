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

// This flag will define data structures to occupy here, the singletons...
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
static_assert(sizeof(Device) == 12, "Changes on ChipInfoDB::Device will impact final Flash size");
static_assert(_countof(msp430_mcus_set) < 512, "Bit-field size 'i_refd_' of ChipInfoDB::Device is too small to hold items");


namespace ChipInfoPrivate_
{

class Device_ : public Device
{
public:
	void GetID(DieInfoEx &info) const DEBUGGABLE;
	void Fill(ChipProfile &o) const DEBUGGABLE;
};

class MemoryLayoutInfo_ : public MemoryLayoutInfo
{
public:
	void Fill(ChipProfile &o) const DEBUGGABLE;
};

class MemoryClasInfo_ : public MemoryClasInfo
{
public:
	void Fill(ChipProfile &o) const DEBUGGABLE;
};

class MemoryInfo_ : public MemoryInfo
{
public:
	void Fill(MemInfo &o) const DEBUGGABLE;
};



void Device_::GetID(DieInfoEx &info) const
{
	// Resolve reference before current record
	if (i_refd_)
		((const Device_ &)(msp430_mcus_set[i_refd_ - 1])).GetID(info);
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
	info.mcu_cfg_mask = DecodeConfigMask(mcu_cfg_mask_);
}


static const MemoryLayoutInfo_ *GetMemLayout(uint8_t pos)
{
	return (const MemoryLayoutInfo_ *)GetLyt(pos);
}

void Device_::Fill(ChipProfile &o) const
{
	// Resolve reference before current record
	if (i_refd_)
		((const Device_ &)(msp430_mcus_set[i_refd_ - 1])).Fill(o);
	// Reset inherited ext-features, if required
	if (clr_ext_attr_ != kNoClrExtFeat)
	{
		/*
		** According to msp430.xsd file, these are the extended features:
		**		- Tmr
		**		- Jtag
		**		- Dtc
		**		- Sync
		**		- Instr
		**		- _1377
		**		- psach
		**		- eemInaccessibleInLPM
		**
		** Handle just those that we are tracking...
		*/
		o.issue_1377_ = false;
	}
	// Name
	if (name_)
	{
		DecompressChipName(o.name_, name_);
		o.slau_ = MapToChipToSlau(name_);
	}
	// 
	if (psa_ != kPsaNone)
		o.psa_ = psa_;
	//
	if (eem_type_ != kEmexNone)
		o.eem_type_ = eem_type_;
	//
	if (clock_ctrl_ != kGccNone)
		o.clk_ctrl_ = clock_ctrl_;
	//
	if (issue_1377_ != kNo1377)
		o.issue_1377_ = true;
	//
	if (quick_mem_read_ != kNoQuickMemRead)
		o.quick_mem_read_ = true;
	//
	if (arch_ != kNullArchitecture)
	{
		o.arch_ = arch_;
		o.bits_ = (o.arch_ == kCpu) ? k16 : k20;
	}
	//
	if (i_mem_layout_ != kLytNone)
		GetMemLayout(i_mem_layout_)->Fill(o);
}


void MemoryLayoutInfo_::Fill(ChipProfile &o) const
{
	// Resolve reference before current record
	if (i_ref_ != kLytNone)
		GetMemLayout(i_ref_)->Fill(o);
	// Override with current data
	for (int i = 0; i < entries_; ++i)
	{
		const MemoryClasInfo_ &ele = (const MemoryClasInfo_ &)array_[i];
		ele.Fill(o);
	}
}


void MemoryClasInfo_::Fill(ChipProfile &o) const
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
		else if (pEle->class_ == class_ && pTarget == NULL)
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
	pTarget->class_ = class_;
	((const MemoryInfo_ &)all_mem_infos[i_info_]).Fill(*pTarget);
	// Set FRAM flag
	if ((pTarget->access_type_ == kFramMemoryAccessBase)
		|| (pTarget->access_type_ == kFramMemoryAccessFRx9))
		o.is_fram_ = true;
	// Mark block as valid/used
	pTarget->valid_ = true;
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
		o.size_ = from_enum_to_block_size[esize_];
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
			const uint16_t mask_cfg = (mcu_cfg_mask == kCfg7F) ? 0x7F : 0xFF;
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
		assert(info.mcu_ver_ != kNoMcuId);
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
		is_fast_flash_ = true;	// grumat: control clock strobes for write/erase flash memory
		break;
	}
}


int ChipProfile::FixSegSize()
{
	int cnt = 0;
	for (; cnt < _countof(mem_); ++cnt)
	{
		MemInfo &info = mem_[cnt];
		if (info.valid_ == false)
			break;
		info.segsize_ = 1;
		if (info.type_ == kFlash)
		{
			if (info.class_ == kClasInfo)
				info.segsize_ = info.size_ / info.banks_;
			else if (slau_ == kSLAU335)
				info.segsize_ = 1024;	// exception to standard flash size
			else
				info.segsize_ = 512;	// no known Flash device with page size other than 512 bytes
		}
	}
	return cnt;
}


bool ChipProfile::Load(const DieInfo &qry)
{
	const Device_ *dev = Find(qry, mcu_info_);
	if (dev == NULL)
		return false;
	Init();
	dev->Fill(*this);
	// Size of array
	int cnt = FixSegSize();
	qsort(&mem_, cnt, sizeof(mem_[0]), cmp);
	UpdateFastFlash();
	return true;
}


void ChipProfile::DefaultMcu()
{
	Init();
	((const Device_ &)msp430_mcus_set[kMcu_MSP430F149]).Fill(*this);
	strcpy(name_, "DefaultChip");
}


void ChipProfile::DefaultMcuXv2()
{
	Init();
	((const Device_ &)msp430_mcus_set[kMcu_MSP430F5438A]).Fill(*this);
	strcpy(name_, "DefaultChip");
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
		.class_ = kClasInfo,
		.type_ = all_mem_infos[kMem_Info_Default].type_,
		.access_type_ = all_mem_infos[kMem_Info_Default].access_type_,
		.start_ = from_enum_to_address[all_mem_infos[kMem_Info_Default].estart_],
		.size_ = from_enum_to_block_size[all_mem_infos[kMem_Info_Default].esize_],
		.segsize_ = from_enum_to_block_size[all_mem_infos[kMem_Info_Default].esize_]
					/ all_mem_infos[kMem_Info_Default].banks_,
		.bit_size_ = from_enum_to_bit_size[all_mem_infos[kMem_Info_Default].bit_size_],
		.banks_ = all_mem_infos[kMem_Info_Default].banks_,
		.mapped_ = all_mem_infos[kMem_Info_Default].mapped_,
		.access_mpu_ = all_mem_infos[kMem_Info_Default].access_mpu_,
		.valid_ = true
	};

	for (int i = 0; i < _countof(mem_); ++i)
	{
		const MemInfo &m = mem_[i];
		if (m.valid_ == false)
			break;
		if (m.class_ == kClasInfo)
			return m;
	}
	return def;
}


const MemInfo &ChipProfile::GetMainMem() const
{
	// Copy values from kMem_Main_Flash as default values
	static constexpr const MemInfo def =
	{
		.class_ = kClasMain,
		.type_ = all_mem_infos[kMem_Main_Flash].type_,
		.access_type_ = all_mem_infos[kMem_Main_Flash].access_type_,
		.start_ = from_enum_to_address[all_mem_infos[kMem_Main_Flash].estart_],
		.size_ = from_enum_to_block_size[all_mem_infos[kMem_Main_Flash].esize_],
		.segsize_ = 512,
		.bit_size_ = from_enum_to_bit_size[all_mem_infos[kMem_Main_Flash].bit_size_],
		.banks_ = all_mem_infos[kMem_Main_Flash].banks_,
		.mapped_ = all_mem_infos[kMem_Main_Flash].mapped_,
		.access_mpu_ = all_mem_infos[kMem_Main_Flash].access_mpu_,
		.valid_ = true
	};

	for (int i = 0; i < _countof(mem_); ++i)
	{
		const MemInfo &m = mem_[i];
		if (m.valid_ == false)
			break;
		if (m.class_ == kClasMain)
			return m;
	}
	return def;
}


bool ChipProfile::IsCpuX_ID(uint16_t id)
{
	// there are not many as TI moved to Xv2 and deprecated these cores
	return (id == 0x6ff2)
		|| (id == 0x6ff4)
		|| (id == 0x7ff4)
		;
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
			Debug() << "OK (" << qry_level_ << ") [" << chip.issue_1377_ << "]\n";
		else
			Debug() << "Located " << chip.name_ << " instead! (" << qry_level_ << ")\n";
	}
}
#endif
