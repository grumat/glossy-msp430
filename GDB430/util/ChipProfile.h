
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
#include "util.h"

//! Produces debug output for internal part number matching system
#define OPT_DEBUG_SCORE_SYSTEM	0

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
	uint16_t	mcu_ver_;
	uint16_t	mcu_sub_;
	uint8_t		mcu_rev_;
	uint8_t		mcu_fab_;
	uint16_t	mcu_self_;
	uint8_t		mcu_cfg_;
	uint8_t		mcu_fuse_;
};

//! Complements MCU die info record with validity flags taken from the database
struct DieInfoEx : public DieInfo
{
	union
	{
		uint16_t raw_;
		struct
		{
			uint8_t mcu_fuse_mask;
			uint8_t mcu_cfg_mask;
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
	ChipInfoDB::EnumMemoryKey class_;
	ChipInfoDB::EnumMemoryType type_;
	ChipInfoDB::EnumMemAccessType access_type_;
	uint32_t start_;								// start address of block
	uint32_t size_;									// total size of memory block
	const ChipInfoDB::MemWrProt *mem_wr_prot_;		// for FRAM parts
	uint16_t segsize_;								// size of flash segment (for erase operation)
	uint8_t bit_size_;								// bit-size organization
	uint8_t banks_;									// total bank count
	uint8_t mapped_;								// mapped to MCU bus
	uint8_t access_mpu_;
	uint8_t valid_;									// record validation flag
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
	bool IsClear() const { return name_[0] == 0; }
	bool Load(const DieInfo &qry) OPTIMIZED;
	void DefaultMcu() OPTIMIZED;
	void DefaultMcuX() OPTIMIZED;
	void DefaultMcuXv2() OPTIMIZED;
	const MemInfo *FindMemByAddress(address_t addr) const OPTIMIZED;
	const MemInfo &GetInfoMem() const OPTIMIZED;
	const MemInfo &GetMainMem() const OPTIMIZED;
	// Check is a DeviceID is for a CPUX (assuming JTAGID == 0x89)
	static bool IsCpuX_ID(uint16_t id) OPTIMIZED;

public:
	char name_[kNameBufCount];
	DieInfoEx mcu_info_;
	ChipInfoDB::EnumBitSize bits_;

	ChipInfoDB::EnumCpuType arch_;
	ChipInfoDB::EnumEemType eem_type_;
	ChipInfoDB::EnumClockControl clk_ctrl_;

	ChipInfoDB::EnumSlau slau_;		// stores TI's SLAU reference users guide

	ChipInfoDB::EnumStopFllDbg stop_fll_ : 1;
	uint8_t is_fram_ : 1;
	// Device has a issue 1377 with the JTAG Mailbox
	uint8_t issue_1377_ : 1;
	// Device supports quick memory read routine
	uint8_t quick_mem_read_ : 1;
	// Number of breakpoints
	uint8_t num_breakpoints_;
	// Memory layout information
	MemInfo mem_[ChipInfoDB::kMaxMemConfigs];

	// PowerSettings (for devices having an LDO) or NULL
	const ChipInfoDB::PowerSettings *pwr_settings_;
	// For Flash operations on legacy parts
	const ChipInfoDB::FlashTimings *flash_timings_;
	// Pointer to EemTimer control structure (initialization)
	ChipInfoDB::EtwCodes eem_timers_;

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

