using System.IO;
using System.Text;

namespace MakeChipInfoDB
{
	class FileRenderer
	{
		void DoHfileStart(TextWriter stream)
		{
			stream.Write(@"/////////////////////////////////////////////////////////////////////////////////////
// THIS FILE WAS AUTOMATICALLY GENERATED BY **ExtraChipInfo.py** SCRIPT!
// DO NOT EDIT!
/////////////////////////////////////////////////////////////////////////////////////
// Information extracted from:
// MSPDebugStack_OS_Package_3_15_1_1.zip\DLL430_v3\src\TI\DLL430\DeviceDb\devicedb.h
/////////////////////////////////////////////////////////////////////////////////////

/*
 * C:\MSP430\mspdebugstack\DLL430_v3\src\TI\DLL430\DeviceDb\devicedb.h
 *
 * Copyright (C) 2020 Texas Instruments Incorporated - http://www.ti.com/
 *
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  ""AS IS"" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

namespace ChipInfoDB {

//! Signals that start address field of the record is undefined
static constexpr uint32_t kNoMemStart = 0x000FFFFF;
//! Used in DecodeSubversion to indicate unpopulated field
static constexpr uint16_t kNoSubver = 0xFFFF;
//! Used in DecodeSelf to indicate unpopulated field
static constexpr uint16_t kNoSelf = 0xFFFF;
//! Used in DecodeRevision to indicate unpopulated field
static constexpr uint8_t kNoRev = 0xFF;
//! Used in DecodeConfig to indicate unpopulated field
static constexpr uint8_t kNoConfig = 0xFF;
//! Used in DecodeConfigMask to indicate unpopulated field
static constexpr uint8_t kNoConfigMask = 0xFF;
//! Used in DecodeFab to indicate unpopulated field
static constexpr uint8_t kNoFab = 0xFF;
//! Used in DecodeFuse to indicate unpopulated field
static constexpr uint8_t kNoFuse = 0xFF;
//! Signals the the main ID field of the record is undefined
static constexpr uint16_t kNoMcuId = 0xFFFF;

#pragma pack(1)

// Memory class
enum MemoryClass : uint8_t
{
	kClasMain
	, kClasRam
	, kClasRam2
	, kClasTinyRam
	, kClasLeaRam
	, kClasInfo
	, kClasCpu
	, kClasIrVec
	, kClasLcd
	, kClasEem
	, kClasBootCode
	, kClasBootCode2
	, kClasBsl
	, kClasBsl2
	, kClasMidRom
	, kClasUsbRam
	, kClasPeripheral8bit
	, kClasPeripheral16bit
	, kClasPeripheral16bit1
	, kClasPeripheral16bit2
	, kClasPeripheral16bit3
	, kClasUssPeripheral
	, kClasLeaPeripheral
	, kClasLib
	, kClasMax_
};

// Type of memory
enum MemoryType : uint8_t
{
	kNullMemType
	, kRegister
	, kFlash
	, kRam
	, kRom
	, kMemTypeMax
};

// Type of memory access
enum MemAccessType : uint8_t
{
	kNullMemAccess
	, kBootcodeRomAccess
	, kBslRomAccess
	, kBslRomAccessGR
	, kBslFlashAccess
	, kFlashMemoryAccess2ByteAligned
	, kInformationFlashAccess
	, kFramMemoryAccessBase
	, kFramMemoryAccessFRx9
	, kTinyRandomMemoryAccess
	, kLockableRamMemoryAccess
	, kUsbRamAccess
	, kRegisterAccess5xx
	, kMemAccessMax
};

// PSA Support
enum PsaType : uint16_t
{
	kPsaNone
	, kPsaRegular
	, kPsaEnhanced
	, kPsaUpper_ = kPsaEnhanced
};

// Architecture of the CPU
enum CpuArchitecture : uint16_t
{
	kNullArchitecture
	, kCpu
	, kCpuX
	, kCpuXv2
};

// Embedded Emulation Module (EEM) type
enum EemType : uint16_t
{
	kEmexNone
	, kEmexLow
	, kEmexMedium
	, kEmexHigh
	, kEmexExtraSmall5XX
	, kEmexSmall5XX
	, kEmexMedium5XX
	, kEmexLarge5XX
	, kEemUpper_ = kEmexLarge5XX
};

// Bit size of the CPU or Peripheral
enum BitSize : uint8_t
{
	kNullBitSize
	, k8
	, k16
	, k20
	, BitSizeUpper_ = k20
};

// Enumeration that specifies the address start of a memory block
enum AddressStart : uint16_t
{
	kStart_None
	, kStart_0x0
	, kStart_0x6
	, kStart_0x20
	, kStart_0x90
	, kStart_0x100
	, kStart_0x200
	, kStart_0xa80
	, kStart_0xb00
	, kStart_0xc00
	, kStart_0xe00
	, kStart_0xf00
	, kStart_0x1000
	, kStart_0x1100
	, kStart_0x1800
	, kStart_0x1900
	, kStart_0x1a00
	, kStart_0x1c00
	, kStart_0x1c80
	, kStart_0x1e00
	, kStart_0x2000
	, kStart_0x2100
	, kStart_0x2380
	, kStart_0x2400
	, kStart_0x2500
	, kStart_0x2c00
	, kStart_0x3100
	, kStart_0x4000
	, kStart_0x4400
	, kStart_0x5c00
	, kStart_0x6000
	, kStart_0x6c00
	, kStart_0x8000
	, kStart_0xa000
	, kStart_0xa400
	, kStart_0xc000
	, kStart_0xc200
	, kStart_0xc400
	, kStart_0xe000
	, kStart_0xe300
	, kStart_0xf000
	, kStart_0xf100
	, kStart_0xf800
	, kStart_0xf840
	, kStart_0xf880
	, kStart_0xfc00
	, kStart_0xfe00
	, kStart_0xffe0
	, kStart_0xc0000
	, kStart_0xf0000
	, kStart_0xf8000
	, kStart_0xfac00
	, kStart_0xffc00
	, kStart_Max_
};

// Enumeration that specifies the size of a memory block
enum BlockSize : uint16_t
{
	kSize_None
	, kSize_0x6
	, kSize_0xd
	, kSize_0x10
	, kSize_0x15
	, kSize_0x1a
	, kSize_0x20
	, kSize_0x40
	, kSize_0x60
	, kSize_0x80
	, kSize_0x100
	, kSize_0x200
	, kSize_0x300
	, kSize_0x400
	, kSize_0x500
	, kSize_0x600
	, kSize_0x760
	, kSize_0x780
	, kSize_0x7c0
	, kSize_0x7e0
	, kSize_0x800
	, kSize_0xa00
	, kSize_0xa60
	, kSize_0xe00
	, kSize_0xf00
	, kSize_0xf80
	, kSize_0xfe0
	, kSize_0x1000
	, kSize_0x1400
	, kSize_0x1c00
	, kSize_0x1d00
	, kSize_0x1800
	, kSize_0x2000
	, kSize_0x2800
	, kSize_0x3000
	, kSize_0x3c00
	, kSize_0x3e00
	, kSize_0x4000
	, kSize_0x5000
	, kSize_0x6000
	, kSize_0x8000
	, kSize_0xc000
	, kSize_0xbc00
	, kSize_0xdb00
	, kSize_0xdf00
	, kSize_0xe000
	, kSize_0xef00
	, kSize_0xfc00
	, kSize_0x10000
	, kSize_0x16f00
	, kSize_0x18000
	, kSize_0x1cf00
	, kSize_0x1df00
	, kSize_0x20000
	, kSize_0x30000
	, kSize_0x40000
	, kSize_0x1fc00
	, kSize_0x17c00
	, kSize_0x60000
	, kSize_0x80000
	, kSize_Max_
};


// Types of Config masks
enum ConfigMask : uint8_t
{
	kCfgNoMask		// 0xFF
	, kCfg7F		// 0x7F
};

// Types of fuse masks
enum FusesMask : uint16_t
{
	kFuse1F			// 0x1F
	, kFuse0F		// 0xF
	, kFuse07		// 0x7
	, kFuse03		// 0x3
	, kFuse01		// 0x1
	, kFuseUpper_ = kFuse01
};

// Enumeration for the Sub-version field on device identification
enum SubversionEnum : uint8_t
{
	kSubver_None = 3		// 0xffff
	, kSubver_0000 = 0		// 0x0000
	, kSubver_0001 = 1		// 0x0001
	, kSubver_Upper_ = kSubver_0001
};

// Self field possible values
enum SelfEnum : uint8_t
{
	kSelf_None		// 0xffff
	, kSelf_0000	// 0x0000
};

enum RevisionEnum : uint16_t
{
	kRev_None		// 0xff
	, kRev_00		// 0x00
	, kRev_02		// 0x02
	, kRev_10		// 0x10
	, kRev_13		// 0x13
	, kRev_20		// 0x20
	, kRev_21		// 0x21
	, kRev_Upper_ = kRev_21
};

// Allowed values for Fab field
enum FabEnum : uint8_t
{
	kFab_None	// 0xff
	, kFab_40	// 0x40
};

// Allowed values for Config field
enum ConfigEnum : uint8_t
{
	kCfg_None	// 0xff
	, kCfg_00	// 0x00
	, kCfg_01	// 0x01
	, kCfg_02	// 0x02
	, kCfg_03	// 0x03
	, kCfg_45	// 0x45
	, kCfg_47	// 0x47
	, kCfg_57	// 0x57
	, kCfg_Upper_ = kCfg_57
};

// Allowed fuse values
enum FusesEnum : uint16_t
{
	kFuse_None = 0x1f	// 0x1f
	, kFuse_00 = 0		// 0x00
	, kFuse_01			// 0x01
	, kFuse_02			// 0x02
	, kFuse_03			// 0x03
	, kFuse_04			// 0x04
	, kFuse_05			// 0x05
	, kFuse_06			// 0x06
	, kFuse_07			// 0x07
	, kFuse_08			// 0x08
	, kFuse_09			// 0x09
	, kFuse_0a			// 0x0a
	, kFuse_0b			// 0x0b
	, kFuse_0c			// 0x0c
	, kFuse_0d			// 0x0d
	, kFuse_0e			// 0x0e
	, kFuse_0f			// 0x0f
	, kFuse_10			// 0x10
	, kFuse_11			// 0x11
	, kFuse_12			// 0x12
	, kFuse_Upper = kFuse_12
};

// Device has an issue with the JTAG MailBox peripheral
/*!
grumat: Was unable to locate the issue documentation. Checked Errata datasheets
and candidates could be: EEM6, EEM13, JTAG17
Note that XML logic does not matches these errata sheets. For example: MSP430F5438
is the single variant in the family that is not tagged with 1377 issue, but its
errata-sheet is just identical to MSP430F5418. XML may also be the issue.
*/
enum Issue1377 : uint16_t
{
	kNo1377
	, k1377
};

// Device supports quick memory read
enum QuickMemRead : uint16_t
{
	kNoQuickMemRead
	, kQuickMemRead
};

// Device supports quick memory read
enum StopFllDbg : uint8_t
{
	kNoStopFllDbg
	, kStopFllDbg
};

// Fixes a weird XML schema that resets all inherited <extFeatures> values
enum ClrExtFeat : uint16_t
{
	kNoClrExtFeat
	, kClrExtFeat
};

// Clock type supported by device
enum ClockControl : uint16_t
{
	kGccNone
	, kGccStandard
	, kGccStandardI
	, kGccExtended
};

// Maps device to TI User's Guide
enum FamilySLAU : uint8_t
{
	kSLAU012			// SLAU012	(don't care, old parts have no JTAG)
	, kSLAU049			// SLAU049
	, kSLAU056			// SLAU056
	, kSLAU144			// SLAU144
	, kSLAU208			// SLAU208
	, kSLAU259			// SLAU259
	, kSLAU321			// SLAU321	(no Flash memory)
	, kSLAU335			// SLAU335
	, kSLAU367			// SLAU367
	, kSLAU378			// SLAU378
	, kSLAU445			// SLAU445
	, kSLAU506			// SLAU506
	, kSlauMax_
};

// Used for CpuXv2 init
struct EemTimer
{
	uint8_t idx_;
	uint8_t value_;

	ALWAYS_INLINE bool IsEofMark() const { return value_ == 0; }
};

// Enumeration with valid indexes for EemTimers
enum EemTimerEnum : uint8_t
{
		kEmmTimer0,
		kEmmTimer1,
		kEmmTimer2,
		kEmmTimer3,
		kEmmTimer4,
		kEmmTimer5,
		kEmmTimer6,
		kEmmTimer7,
		kEmmTimer8,
		kEmmTimer9,
		kEmmTimer10,
		kEmmTimer11,
		kEmmTimer12,
		kEmmTimer13,
		kEmmTimer14,
		kEmmTimer15,
		kEmmTimer16,
		kEmmTimer17,
		kEmmTimer18,
		kEmmTimer19,
		kEmmTimer20,
		kEmmTimer21,
		kEmmTimer22,
		kEmmTimer23,
		kEmmTimer24,
		kEmmTimer25,
		kEmmTimer26,
		kEmmTimer27,
		kEmmTimer28,
		kEmmTimer29,
		kEmmTimer30,
		kEmmTimer31,
		kEmmTimer32,
		kEmmTimer33,
		kEmmTimer34,
		kEmmTimer35,
		kEmmTimer_Upper_ = kEmmTimer35,
		kEmmTimer_None = 0x3f,
};


// Describes a memory block
struct MemoryInfo
{
	// A chained memory info to use as basis (or NULL)
	uint8_t i_refm_ : 8;				// 0

	// Type of memory
	MemoryType type_ : 3;				// 1
	// Memory bit alignment
	BitSize bit_size_ : 3;
	// Mapped flag
	uint8_t mapped_ : 1;
	// Accessible by MPU
	uint8_t access_mpu_ : 1;

	// Total memory banks
	uint16_t banks_ : 4;				// 2
	// Start address or kNoMemStart
	AddressStart estart_ : 6;
	// Size of block (ignored for kNoMemStart)
	BlockSize esize_ : 6;

	// Type of access
	MemAccessType access_type_ : 4;		// 4
};										// Structure size = 5 bytes

// Describes a memory block and it's class
struct MemoryClasInfo
{
	// Memory class
	MemoryClass class_;					// 0

	// Completes the memory information
	uint8_t i_info_ : 8;				// 1
};										// Structure size = 2 bytes


enum LytIndexes : uint8_t;

// A complete memory layout
struct MemoryLayoutInfo
{
	// Size of the memory descriptors
	uint8_t entries_;					// 0

	// Chained memory info to walk before merging with (or 255)
	LytIndexes i_ref_;					// 1

	// Memory descriptors
	const MemoryClasInfo array_[];		// 2...
};										// Structure size is variable (min 2 bytes)


// Compresses MemoryLayoutInfo
struct MemoryLayoutBlob
{
	uint8_t low_;
	uint8_t hi_;
};										// Structure size = 2 bytes


// Extra PowerSettings records (loose records, shall be extra coded because of space constraints)
struct PowerSettings
{
	uint32_t test_reg_mask_;			// 0
	uint32_t test_reg_default;			// 4
	uint32_t test_reg_enable_lpm5_;		// 8
	uint32_t test_reg_disable_lpm5_;	// 12
	uint16_t test_reg3v_mask_;			// 16
	uint16_t test_reg3v_default;		// 18
	uint16_t test_reg3v_enable_lpm5_;	// 20
	uint16_t test_reg3v_disable_lpm5_;	// 22
};										// Structure size = 24 bytes


// Part name prefix resolver (First byte of name_) and TI SLAU number
struct PrefixResolver
{
	// Chip part number prefix
	const char *prefix;					// 0

	// TI User's guide
	FamilySLAU family;					// 4
};										// Structure size = 5 bytes


// Describes the device or common attributes of a device group
struct Device
{
	// A compressed part number/name (use DecompressChipName())
	const char *name_;					// 0
	// Main ID of the device

	uint16_t mcu_ver_;					// 4
	// A base device to copy similarities of (or NULL)

	uint16_t i_refd_ : 9;				// 6
	// Clears inherited ""ext attributes""
	ClrExtFeat clr_ext_attr_ : 1;
	// MCU architecture
	CpuArchitecture arch_ : 2;
	// Type of PSA
	PsaType psa_ : 2;
	// Type of clock required by device
	ClockControl clock_ctrl_: 2;

	// Embedded Emulation Module type
	EemType eem_type_ : 3;				// 8
	// Issue 1377 with the JTAG MailBox
	Issue1377 issue_1377_ : 1;
	// Supports Quick Memory Read
	QuickMemRead quick_mem_read_ : 1;
	// Revision device identification
	RevisionEnum mcu_rev_ : 3;

	// The fuse value
	FusesEnum mcu_fuses_ : 5;			// 9
	// The fuses mask
	FusesMask mcu_fuse_mask_ : 3;	

	// A recursive chain that forms the Memory layout (or NULL)
	LytIndexes i_mem_layout_;			// 10

	// Sub-version device identification
	SubversionEnum mcu_subv_ : 2;		// 11
	// Config device identification
	ConfigEnum mcu_cfg_ : 3;
	// Mask to apply to Config
	ConfigMask mcu_cfg_mask_ : 1;
	// Fab device identification
	FabEnum mcu_fab_ : 1;
	// Self device identification
	SelfEnum mcu_self_ : 1;

	// EemTimers
	EemTimerEnum eem_timers_ : 6;		// 12
	// Stop FLL clock
	StopFllDbg stop_fll_ : 1;
};										// Total of 13 bytes

enum McuIndexes : uint16_t;

// Complete list of devices
struct DeviceList
{
	// Total MCU parts
	uint16_t entries_;
	// The list of MCU's
	const McuIndexes array_[];
};

#pragma pack()

// A single file should enable this macro to implement the database
#ifdef OPT_IMPLEMENT_DB


// Decompress titles using a char to string map (see NAME_PREFIX_TAB in ExtractChipInfo.py)
static constexpr const PrefixResolver msp430_part_name_prefix[] =
{
	// Ordered from larger string to shorter
	{ ""MSP430SL5438A""	, kSLAU208 },
	{ ""MSP430FE42""		, kSLAU056 },
	{ ""MSP430AFE2""		, kSLAU144 },
	{ ""RF430FRL15""		, kSLAU506 },
	{ ""MSP430F67""		, kSLAU208 },
	{ ""MSP430FG4""		, kSLAU056 },
	{ ""MSP430FG6""		, kSLAU208 },
	{ ""MSP430FR2""		, kSLAU445 },
	{ ""MSP430FR4""		, kSLAU445 },
	{ ""MSP430FR5""		, kSLAU367 },
	{ ""MSP430FR6""		, kSLAU367 },
	{ ""MSP430F1""		, kSLAU049 },
	{ ""MSP430F2""		, kSLAU144 },
	{ ""MSP430F4""		, kSLAU056 },
	{ ""MSP430F5""		, kSLAU208 },
	{ ""MSP430F6""		, kSLAU208 },
	{ ""MSP430FW""		, kSLAU056 },
	{ ""MSP430G2""		, kSLAU144 },
	{ ""MSP430C""			, kSLAU321 },
	{ ""MSP430I""			, kSLAU335 },
	{ ""MSP430L""			, kSLAU321 },
	{ ""RF430F5""			, kSLAU378 },
	{ ""CC430F""			, kSLAU259 },
	{ ""MSP430""			, kSLAU144 },	// the ""default"" fits SLAU144
	// All items that the first char of name_ does not fits the range 
	// of 'a' to 'a' + _countof(msp430_part_name_prefix) have integral part names
	// and are of kSLAU144 User's guide family.
};


// Utility to decompress the chip Part number
static void DecompressChipName(char *t, const char *s)
{
	// Check prefix range
	if ( (*s >= 'a') && (*s < ('a' + _countof(msp430_part_name_prefix))) )
	{
		// Locate prefix
		const char *f = msp430_part_name_prefix[*s - 'a'].prefix;
		// Copy prefix...
		while (*f)
			*t++ = *f++;
		// ...and suffix
		++s;
	}
	// Copy string or append suffix
	while (*s)
		*t++ = *s++;
	*t = 0;
}


// Utility to retrieve TI's User's Guide SLAU number
static FamilySLAU MapToChipToSlau(const char *s)
{
	if (*s < 'a' || *s >('a' + _countof(msp430_part_name_prefix)))
		return kSLAU144;
	return msp430_part_name_prefix[*s - 'a'].family;
}

static constexpr uint32_t from_enum_to_bit_size[] =
{
	0
	, 8
	, 16
	, 32
};

static constexpr uint32_t from_enum_to_address[] =
{
	kNoMemStart
	, 0x0
	, 0x6
	, 0x20
	, 0x90
	, 0x100
	, 0x200
	, 0xa80
	, 0xb00
	, 0xc00
	, 0xe00
	, 0xf00
	, 0x1000
	, 0x1100
	, 0x1800
	, 0x1900
	, 0x1a00
	, 0x1c00
	, 0x1c80
	, 0x1e00
	, 0x2000
	, 0x2100
	, 0x2380
	, 0x2400
	, 0x2500
	, 0x2c00
	, 0x3100
	, 0x4000
	, 0x4400
	, 0x5c00
	, 0x6000
	, 0x6c00
	, 0x8000
	, 0xa000
	, 0xa400
	, 0xc000
	, 0xc200
	, 0xc400
	, 0xe000
	, 0xe300
	, 0xf000
	, 0xf100
	, 0xf800
	, 0xf840
	, 0xf880
	, 0xfc00
	, 0xfe00
	, 0xffe0
	, 0xc0000
	, 0xf0000
	, 0xf8000
	, 0xfac00
	, 0xffc00
};

static constexpr uint32_t from_enum_to_block_size[] =
{
	0
	, 0x6
	, 0xd
	, 0x10
	, 0x15
	, 0x1a
	, 0x20
	, 0x40
	, 0x60
	, 0x80
	, 0x100
	, 0x200
	, 0x300
	, 0x400
	, 0x500
	, 0x600
	, 0x760
	, 0x780
	, 0x7c0
	, 0x7e0
	, 0x800
	, 0xa00
	, 0xa60
	, 0xe00
	, 0xf00
	, 0xf80
	, 0xfe0
	, 0x1000
	, 0x1400
	, 0x1c00
	, 0x1d00
	, 0x1800
	, 0x2000
	, 0x2800
	, 0x3000
	, 0x3c00
	, 0x3e00
	, 0x4000
	, 0x5000
	, 0x6000
	, 0x8000
	, 0xc000
	, 0xbc00
	, 0xdb00
	, 0xdf00
	, 0xe000
	, 0xef00
	, 0xfc00
	, 0x10000
	, 0x16f00
	, 0x18000
	, 0x1cf00
	, 0x1df00
	, 0x20000
	, 0x30000
	, 0x40000
	, 0x1fc00
	, 0x17c00
	, 0x60000
	, 0x80000
};

//! Decodes the 'sub-version' field
ALWAYS_INLINE static uint16_t DecodeSubversion(SubversionEnum v)
{
	return v == kSubver_None ? kNoSubver : (uint16_t)v;
}

//! Decodes the 'self' field
ALWAYS_INLINE static uint16_t DecodeSelf(SelfEnum v)
{
	return v == kSelf_None ? kNoSelf : 0x0000;
}

//! Decodes the 'revision' field
ALWAYS_INLINE static uint8_t DecodeRevision(RevisionEnum v)
{
	// Table map to solve the 'revision' field
	static constexpr uint8_t from_enum_to_revision_val[] =
	{
		kNoRev
		, 0x00
		, 0x02
		, 0x10
		, 0x13
		, 0x20
		, 0x21
	};
	// For further refactoring, ensures that lookup table is in sync with the enum
	static_assert(_countof(from_enum_to_revision_val) == RevisionEnum::kRev_Upper_ + 1, ""enum range does not match table"");

	// Resolves using the 
	return from_enum_to_revision_val[v];
}

//! Decodes the 'config' field
ALWAYS_INLINE static uint8_t DecodeConfig(ConfigEnum v)
{
	// Table map to solve the 'config' field
	static constexpr uint8_t from_enum_to_config_val[] =
	{
		kNoConfig
		, 0x00
		, 0x01
		, 0x02
		, 0x03
		, 0x45
		, 0x47
		, 0x57
	};
	// For further refactoring, ensures that lookup table is in sync with the enum
	static_assert(_countof(from_enum_to_config_val) == ConfigEnum::kCfg_Upper_ + 1, ""enum range does not match table"");

	return from_enum_to_config_val[v];
}

//! Decodes the 'config mask' field
ALWAYS_INLINE static uint8_t DecodeConfigMask(ConfigMask v)
{
	return v == kCfgNoMask ? kNoConfigMask : 0x7F;
}

//! Decodes the 'Fab' field
ALWAYS_INLINE static uint8_t DecodeFab(FabEnum v)
{
	return v == kFab_None ? kNoFab : 0x40;
}

//! Decodes the 'fuse mask' field
ALWAYS_INLINE static uint8_t DecodeFuseMask(FusesMask v)
{
	// Table map to solve the 'fuse mask' field
	static constexpr uint8_t from_enum_to_fuse_mask_val[] =
	{
		0x1f
		, 0x0f
		, 0x07
		, 0x03
		, 0x01
	};
	// For further refactoring, ensures that lookup table is in sync with the enum
	static_assert(_countof(from_enum_to_fuse_mask_val) == FusesMask::kFuseUpper_ + 1, ""enum range does not match table"");

	return from_enum_to_fuse_mask_val[v];
}

//! Decodes the 'fuse' field
ALWAYS_INLINE static uint8_t DecodeFuse(FusesEnum v)
{
	return v == kFuse_None ? kNoFuse : (uint8_t)v;
}


");
		}

		void DoHfileStop(TextWriter fh)
		{
			fh.Write(@"
#else	// OPT_IMPLEMENT_DB

extern const DeviceList all_msp430_mcus;

#endif	// OPT_IMPLEMENT_DB

}	// namespace ChipInfoDB

");
		}

		public void WriteFile(string fname, XmlManager mng)
		{
			using (TextWriter stream = new StreamWriter(fname, false, Encoding.Latin1))
			{
				DoHfileStart(stream);
				mng.EemTimerDB_.DoHFile(stream);
				mng.Pwr_.DoHFile(stream);
				mng.Mems_.DoHFile(stream);
				mng.Lyts_.DoHFile(stream, mng.Mems_);
				mng.Devs_.DoHFile(stream, mng.Devs_);
				DoHfileStop(stream);
			}
		}
	}
}
