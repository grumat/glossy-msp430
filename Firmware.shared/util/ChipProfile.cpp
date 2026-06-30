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
	info.mcuVer = mcuVer;
	// Extract 'sub-version'
	info.mcuSub = DecodeSubversion(mcuSubv);
	// Extract 'revision'
	info.mcuRev = DecodeRevision(mcuRev);
	// Extract 'fab'
	info.mcuFab = DecodeFab(mcuFab);
	// Extract 'self'
	info.mcuSelf = DecodeSelf(mcuSelf);
	// Extract 'config'
	info.mcuCfg = DecodeConfig(mcuCfg);
	// Extract 'fuses'
	info.mcuFuse = DecodeFuse(mcuFuses);

	// Fuse mask
	info.mcuFuseMask = DecodeFuseMask(mcuFuseMask);
	// Config mask
	info.mcuCfgMask = (info.mcuCfg != kNoConfig) ? kConfigMask : 0xFF;
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
	if (hdr->fHasBase)
	{
		// Cast to record with inheritance
		const MemConfigHdrEx *hdrx = (const MemConfigHdrEx *)hdr;
		// Data size are appended after header
		cnt = hdrx->count - sizeof(MemConfigHdrEx);
		// Start of data for this header type
		pBlk = &hdrx->memBlocks[0];
		// First, resolve base attributes
		FillMemory(o, hdrx->baseCfg);
	}
	else
	{
		// Data size are appended after header
		cnt = hdr->count - sizeof(MemConfigHdr);
		// Start of data for this header type
		pBlk = &hdr->memBlocks[0];
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
	const char *name = symtab_part_names + nameSym;
	DecompressChipName(o.szName, name);
	o.slau = MapToChipToSlau(name);
	//
	o.eemType = eemType;
	//
	o.clkCtrl = clockCtrl;
	//
	o.fIssue1377 = fIssue1377;
	//
	o.fQuickMemRead = ChipInfoDB::NoQuickMemRead(*this);
	// These devices require Read-Modify-Write on Info A segment
	o.fTlvClash = fTlvClash;
	// It's rare, but some chips have a GMERAS bit
	if (o.fHasGmeras == 0)
		o.fHasGmeras = ChipInfoDB::HasGmeras(*this);
	//
	o.arch = (idx < kMcuX_) 
		? kCpu : (idx < kMcuXv2_) 
		? kCpuX 
		: kCpuXv2;
	o.bits = ChipInfoDB::EnumBitSize(uint8_t((o.arch == kCpu) ? Exti::k16 : Exti::k20));
	// Standard for all MSP430 parts
	const MemoryBlock_ &cpu = (const MemoryBlock_ &)GetMemoryBlock(kBlkCpu_0);
	cpu.Fill(o);
	const MemoryBlock_ &eem = (const MemoryBlock_ &)GetMemoryBlock(kBlkEem_0);
	eem.Fill(o);
	// 
	FillMemory(o, memConfig);
	//
	DecodeEemTimer(o.eemTimers, eemTimers);
	//
	if (stopFll != kNoStopFllDbg)
		o.stopFll = stopFll;
}


void MemoryBlock_::Fill(ChipProfile &o) const
{
	MemInfo *pTarget = NULL;
	// Iterate over fixed slot count
	for (int i = 0; i < _countof(o.mem_); ++i)
	{
		MemInfo *pEle = &o.mem_[i];
		// Selects first empty block
		if (pEle->valid_ == false)
		{
			if (pTarget == NULL)
				pTarget = pEle;
		}
		// Selects a mem class match
		else if (pEle->memClass == memoryKey)
		{
			pTarget = pEle;
			break;
		}
	}
	// No match found, so use a new empty block
	if (pTarget == NULL)
	{
		// No buffer space for memory description.
		// Consider increasing array size for complex memory descriptions
		assert(false);
	}

	// Update memory block
	pTarget->memClass = memoryKey;
	pTarget->accessType = accessType;
	// Set FRAM flag
	switch (pTarget->accessType)
	{
	case kAccFramMemoryAccessBase:
	case kAccFramMemoryAccessFRx9:
		o.fIsFram = true;
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
	pTarget->type_ = memoryType;
	// Mark block as valid/used
	pTarget->valid_ = true;
	DecodeMemBlock(memLayout
				   , pTarget->start_
				   , pTarget->size_
				   , pTarget->segsize_
				   , pTarget->banks_
				   );
	pTarget->bit_size_ = pTarget->memClass == kMkeyPeripheral8bit ? 8 : 16;
	pTarget->mapped_ = pTarget->memClass != kMkeyCpu && pTarget->memClass != kMkeyEem;
	// This flag only affected by SLAU272 & SLAU367 families
	pTarget->access_mpu_
		= (o.slau == kSLAU272
		   || o.slau == kSLAU367)
		&& (pTarget->memClass == kMkeyInfo
			|| pTarget->memClass == kMkeyMain);
	// FRAM write protection table
	if (wrProt)
		pTarget->mem_wr_prot_ = &ChipInfoDB::mem_wr_prot[wrProt];
}


}	// namespace ChipInfoPrivate_


void DieInfoEx::Clear()
{
	memset(this, 0xFF, sizeof(DieInfo));
	raw = 0;
}


#ifdef OPT_IMPLEMENT_TEST_DB
static int qry_level_;
#endif

#if OPT_DEBUG_SCORE_SYSTEM || defined(OPT_IMPLEMENT_TEST_DB)
int DieInfoEx::GetMaxLevel() const
{
	int lvl = 0;
	// Subversion
	if (mcuSub != kNoSubver)
		lvl += 2;
	// Self
	if (mcuSelf != kNoSelf)
		++lvl;
	// Revision
	if (mcuRev != kNoRev)
		++lvl;
	// Config
	if (mcuCfg != kNoConfig)
		lvl += 2;
	// Fab
	if (mcuFab != kNoFab)
		++lvl;
	// Fuses
	if (mcuFuse != kNoFuse)
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
	if (((qry.mcuVer ^ mcuVer) & mask_ver) == 0)
	{
		int lvl = 0;
		int ok = 0;
		// Subversion
		if (mcuSub != DecodeSubversion(kSubver_None))
		{
			ok += 2 * (((qry.mcuSub ^ mcuSub) & mask_sub) == 0);
			lvl += 2;
		}
		// Self
		if (mcuSelf != DecodeSelf(kSelf_None))
		{
			ok += (((qry.mcuSelf ^ mcuSelf) & mask_self) == 0);
			++lvl;
		}
		// Revision
		if (mcuRev != DecodeRevision(kRev_None))
		{
			ok += (((qry.mcuRev ^ mcuRev) & mask_rev) == 0);
			++lvl;
		}
		// Config
		if (mcuCfg != DecodeConfig(kCfg_None))
		{
			const uint16_t mask_cfg = mcuCfgMask;
			ok += 2 * (((qry.mcuCfg ^ mcuCfg) & mask_cfg) == 0);
			lvl += 2;
		}
		// Fab
		if (mcuFab != DecodeFab(kFab_None))
		{
			ok += (((qry.mcuFab ^ mcuFab) & mask_fab) == 0);
			++lvl;
		}
		// Fuses
		if (mcuFuse != DecodeFuse(kFuse_None))
		{
			ok += (((qry.mcuFuse ^ mcuFuse) & mcuFuseMask) == 0);
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
			Debug() << "MCU: " << dev->szName << '\n';
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
		return pr->memClass - pl->memClass;
}


void ChipProfile::CompleteLoad()
{
	// Chips having TLV clash issue and all families above SLAU144 have LOCKA bit
	fHasLocka = fTlvClash
		|| (slau >= kSLAU144)
		;
	// Old chips can clear info with main. SLAU144 depends on LOCKA bit and all others cannot
	fHas1pMassErase = (slau <= kSLAU056);
	// Sort memory by address and size
	qsort(&mem_, _countof(mem_), sizeof(mem_[0]), cmp);
	pwr_settings_ = DecodePowerSettings(slau);
	switch (eemType)
	{
	case kEmexLow:
	case kEmexExtraSmall5xx:
		numBreakpoints = 2;
		break;
	case kEmexMedium:
	case kEmexSmall5xx:
		numBreakpoints = 3;
		break;
	case kEmexMedium5xx:
		numBreakpoints = 5;
		break;
	default:
		numBreakpoints = 8;
		break;
	}
}

bool ChipProfile::Load(const DieInfo &qry)
{
	EnumMcu mcu = Find(qry, mcuInfo);
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
	strcpy(szName, "DefaultChip");
	CompleteLoad();
}


void ChipProfile::DefaultMcuX()
{
	Init();
	((const Device_ &)msp430_mcus_set[kMcu_MSP430F2416]).Fill(*this, kMcu_MSP430F2416);
	strcpy(szName, "DefaultChip");
	CompleteLoad();
}


void ChipProfile::DefaultMcuXv2(JtagId jtag_id)
{
	// Pick a representative of the matching Xv2 family by jtag_id. The descriptor
	// lookup failed, so this is a best-effort "big brother" with the right memory
	// technology / EEM / 1377 traits — see TapDev430Xv2::InitDefaultChip (issue #19).
	ChipInfoDB::EnumMcu rep;
	switch (jtag_id)
	{
	case kMsp_99:	rep = kMcu_MSP430FR5994;	break;	// large FRAM (FR5xx/FR6xx)
	case kMsp_98:	rep = kMcu_MSP430FR4133;	break;	// low-density FRAM (FR2xx/FR4xx)
	case kMsp_91:	// Flash Xv2 (F5xx/F6xx)
	case kMsp_95:	// Flash Xv2 variant
	default:		rep = kMcu_MSP430F5418A;	break;
	}
	Init();
	((const Device_ &)msp430_mcus_set[rep]).Fill(*this, rep);
	strcpy(szName, "DefaultChip");
	CompleteLoad();
}


const MemInfo *ChipProfile::FindMemByAddress(address_t addr) const
{
	for (int i = 0; i < _countof(mem_); ++i)
	{
		const MemInfo &m = mem_[i];
		if (m.valid_ == false)
			break;
		if (m.mapped_		// Bus mapped  blocks
			&& m.start_ <= addr 
			&& addr < (m.start_ + m.size_))
		{
			return &m;
		}
	}
	return NULL;
}


const MemInfo &ChipProfile::GetInfoMem() const
{
	// Copy values from kMem_Info_Default as default values
	static constexpr const MemInfo def =
	{
		.valid_ = true,
		.memClass = kMkeyInfo,
		.type_ = kMtypFlash,
		.accessType = kAccInformationFlashAccess,
		.start_ = 0x1000,
		.size_ = 0x100,
		.segsize_ = 0x40,
		.bit_size_ = 16,
		.banks_ = 4,
		.mapped_ = true,
		.access_mpu_ = false,
	};

	for (int i = 0; i < _countof(mem_); ++i)
	{
		const MemInfo &m = mem_[i];
		if (m.valid_ == false)
			break;
		if (m.memClass == kMkeyInfo)
			return m;
	}
	return def;
}


const MemInfo &ChipProfile::GetMainMem() const
{
	// Copy values from kMem_Main_Flash as default values
	static constexpr const MemInfo def =
	{
		.valid_ = true,
		.memClass = kMkeyMain,
		.type_ = kMtypFlash,
		.accessType = kAccNone,
		.start_ = 0xc000,
		.size_ = 0x4000,
		.segsize_ = 512,
		.bit_size_ = 16,
		.banks_ = 1,
		.mapped_ = true,
		.access_mpu_ = 0,
	};

	for (int i = 0; i < _countof(mem_); ++i)
	{
		const MemInfo &m = mem_[i];
		if (m.valid_ == false)
			break;
		if (m.memClass == kMkeyMain)
			return m;
	}
	return def;
}


const MemInfo &ChipProfile::GetRamMem() const
{
	// Copy values from kMem_Main_Flash as default values
	static constexpr const MemInfo def =
	{
		.valid_ = true,
		.memClass = kMkeyRam,
		.type_ = kMtypRam,
		.accessType = kAccNone,
		.start_ = 0x1c00,
		.size_ = 0x0100,
		.segsize_ = 1,
		.bit_size_ = 16,
		.banks_ = 1,
		.mapped_ = true,
		.access_mpu_ = 0,
	};

	for (int i = 0; i < _countof(mem_); ++i)
	{
		const MemInfo &m = mem_[i];
		if (m.valid_ == false)
			break;
		if (m.memClass == kMkeyRam)
			return m;
	}
	for (int i = 0; i < _countof(mem_); ++i)
	{
		const MemInfo &m = mem_[i];
		if (m.valid_ == false)
			break;
		if (m.memClass == kMkeyRam2)
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
		qry.mcuVer		= pi.mcuVer;
		qry.mcuSub		= pi.mcuSub;
		qry.mcuRev		= pi.mcuRev;
		qry.mcuFab		= pi.mcuFab;
		qry.mcuSelf		= pi.mcuSelf;
		qry.mcuCfg		= pi.mcuCfg;
		qry.mcuFuse		= pi.mcuFuse;
		char buf[ChipProfile::kNameBufCount];
		const char *name = symtab_part_names + msp430_mcus_set[pi.i_refd_].nameSym;
		DecompressChipName(buf, name);
		Debug() << "Testing: " << f::S<24>(buf) << "\t";
		ChipProfile chip;
		chip.Init();
		if (!chip.Load(qry))
		{
			Debug() << "Load Failure!\n";
			continue;
		}
		if (strcmp(buf, chip.szName) == 0)
			Debug() << "OK (" << qry_level_ << ") [" << chip.fIssue1377 << "]\n";
		else
			Debug() << "Located " << chip.szName << " instead! (" << qry_level_ << ")\n";
	}
}
#endif
