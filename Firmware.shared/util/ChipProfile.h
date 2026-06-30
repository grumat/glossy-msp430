
#pragma once

/*
Uncomment this to test the database search function. It uses an additional Table, 
provided by the Python Script with the fields used in a query.
All device records are loaded based on the ID's of the database, so the search 
routine correctness can be evaluated.
Note that a clash happens for MSP430C092 and MSP430L092, as they have exactly the 
same ID. So we are unable to distinguish them.
*/
//#define OPT_IMPLEMENT_TEST_DB

#include "../ChipInfoDB.h"
#include "../drivers/JtagId.h"
#include "util.h"

// Forward declaration of private classes
namespace ChipInfoPrivate_
{
class Device_;
class MemoryLayoutInfo_;
class MemoryClasInfo_;
class MemoryInfo_;
};


#pragma pack(1)

//! MCU die info record
struct DieInfo
{
	uint16_t	mcuVer;
	uint16_t	mcuSub;
	uint8_t		mcuRev;
	uint8_t		mcuFab;
	uint16_t	mcuSelf;
	uint8_t		mcuCfg;
	uint8_t		mcuFuse;
};

//! Complements MCU die info record with validity flags taken from the database
struct DieInfoEx : public DieInfo
{
	union
	{
		uint16_t raw;
		struct
		{
			uint8_t mcuFuseMask;
			uint8_t mcuCfgMask;
		};
	};

	void Clear();
	enum MatchLevel
	{
		kNoMatch
		, kLevel0
		, kLevel1
		, kLevel2
		, kFull
	};
	MatchLevel Match(const DieInfo &qry) const;
#if OPT_DEBUG_SCORE_SYSTEM || defined(OPT_IMPLEMENT_TEST_DB)
	int GetMaxLevel() const;
#endif
};


//! Describes an MCU memory block
struct MemInfo 
{
	uint8_t fValid; // record validation flag
	ChipInfoDB::EnumMemoryKey memClass;
	ChipInfoDB::EnumMemoryType type;
	ChipInfoDB::EnumMemAccessType accessType;
	uint32_t start;								// start address of block
	uint32_t size;									// total size of memory block
	const ChipInfoDB::MemWrProt *pMemWrProt;		// for FRAM parts
	uint16_t segsize;								// size of flash segment (for erase operation)
	uint8_t bitSize;								// bit-size organization
	uint8_t banks;									// total bank count
	uint8_t fMapped;								// mapped to MCU bus
	uint8_t accessMpu;
};


struct FlashRegs
{
	// Low byte of the FCTL1 register
	uint8_t flctl1;
	// Low byte of the FCTL3 register
	uint8_t flctl3;
};

/// Device specific flash memory unlock codes
struct FlashUnlockCodes
{
	// Segment erase register values
	FlashRegs segmentErase;
	// Info A segment erase register values
	FlashRegs infoAErase;
	// Main erase register values
	FlashRegs mainErase;
	// Mass erase register values
	FlashRegs massErase;
};

#pragma pack()


//! Collects all information from the MCU (die + database)
class ChipProfile
{
public:
	enum
	{
		kNameBufCount = 26,
	};
public:
	ALWAYS_INLINE void Init() { memset(this, 0, sizeof(*this)); }
	bool IsClear() const { return szName[0] == 0; }
	bool Load(const DieInfo &qry) OPTIMIZED;
	void DefaultMcu() OPTIMIZED;
	void DefaultMcuX() OPTIMIZED;
	void DefaultMcuXv2(JtagId jtag_id) OPTIMIZED;
	const MemInfo *FindMemByAddress(address_t addr) const OPTIMIZED;
	const MemInfo &GetInfoMem() const OPTIMIZED;
	const MemInfo &GetMainMem() const OPTIMIZED;
	const MemInfo &GetRamMem() const OPTIMIZED;
	// Check is a DeviceID is for a CPUX (assuming JTAGID == 0x89)
	static bool IsCpuX_ID(uint16_t id) OPTIMIZED;

public:
	char szName[kNameBufCount];
	DieInfoEx mcuInfo;
	ChipInfoDB::EnumBitSize bits;

	ChipInfoDB::EnumCpuType arch;
	ChipInfoDB::EnumEemType eemType;
	ChipInfoDB::EnumClockControl clkCtrl;

	ChipInfoDB::EnumSlau slau;		// stores TI's SLAU reference users guide

	ChipInfoDB::EnumStopFllDbg stopFll;
	
	uint8_t fIsFram : 1;
	// Device has a issue 1377 with the JTAG Mailbox
	uint8_t fIssue1377 : 1;
	// Device supports quick memory read routine
	uint8_t fQuickMemRead : 1;
	// Number of breakpoints
	uint8_t numBreakpoints;
	// Memory layout information
	MemInfo mem_[ChipInfoDB::kMaxMemConfigs];
	
	// Device has LOCKA bit
	uint8_t fHasLocka;
	// TLV record clashes with InfoA segment which requires Read-Modify-Write logic
	uint8_t fTlvClash;
	// Device has GMERAS bit
	uint8_t fHasGmeras;
	// Mass erase in a single pass (MAIN + INFO)
	uint8_t fHas1pMassErase;

	// PowerSettings (for devices having an LDO) or NULL
	const ChipInfoDB::PowerSettings *pwr_settings_;
	// For Flash operations on legacy parts
	const ChipInfoDB::FlashTimings *flash_timings_;
	// Pointer to EemTimer control structure (initialization)
	ChipInfoDB::EtwCodes eemTimers;

private:
	friend class ChipInfoPrivate_::Device_;
	friend class ChipInfoPrivate_::MemoryLayoutInfo_;
	friend class ChipInfoPrivate_::MemoryClasInfo_;
	friend class ChipInfoPrivate_::MemoryInfo_;
	ChipInfoDB::EnumMcu Find(const DieInfo &qry, DieInfoEx &info) OPTIMIZED;
	void CompleteLoad() OPTIMIZED;
};


#ifdef OPT_IMPLEMENT_TEST_DB
void TestDB();
#endif

