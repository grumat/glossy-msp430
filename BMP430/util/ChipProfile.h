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

#pragma once

#include "../ChipInfoDB.h"
#include "util.h"

#define OPT_DEBUG_SCORE_SYSTEM	0

// private classes
namespace ChipInfoPrivate_
{
class Device_;
class MemoryLayoutInfo_;
class MemoryClasInfo_;
class MemoryInfo_;
};


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
	//uint32_t	activation_key;
};

//! Complements MCU die info record with validity flags taken from the database
struct DieInfoEx : public DieInfo
{
	// Flags
	union
	{
		uint32_t raw_;
		struct
		{
			ChipInfoDB::FusesMask mcu_fuse_mask : 3;
			ChipInfoDB::ConfigMask mcu_cfg_mask : 1;
			ChipInfoDB::SubversionPresence mcu_sub_f : 1;
			ChipInfoDB::SelfPresence mcu_self_f : 1;
			ChipInfoDB::RevisionPresence mcu_rev_f : 1;
			ChipInfoDB::ConfigPresence mcu_cfg_f : 1;
			ChipInfoDB::FabPresence mcu_fab_f : 1;
			ChipInfoDB::FusesPresence mcu_fuse_f : 1;
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
#if OPT_DEBUG_SCORE_SYSTEM
	int GetMaxLevel() const;
#endif
};


//! Describes an MCU memory block
struct MemInfo 
{
	ChipInfoDB::MemoryClass class_;
	uint32_t start_;
	uint32_t size_;
	ChipInfoDB::MemoryType type_;
	uint8_t bit_size_;
	uint8_t banks_;
	uint8_t mapped_;
	ChipInfoDB::MemAccessType access_type_;
	uint8_t access_mpu_;
	uint8_t valid_;
};


//! Collects all information from the MCU (die + database)
class ChipProfile
{
public:
	void Init() { memset(this, 0, sizeof(*this)); }
	bool Load(const DieInfo &qry);
	void DefaultMcu();
	void DefaultMcuXv2();
	const MemInfo *FindMemByAddress(address_t addr);

public:
	const char *name_;
	DieInfoEx mcu_info_;
	ChipInfoDB::PsaType psa_;
	ChipInfoDB::BitSize bits_;
	ChipInfoDB::CpuArchitecture arch_;
	uint8_t is_fram_;
	//! Valid for Standard architecture only; indicates Flash with faster timing
	uint8_t is_fast_flash_;
	MemInfo mem_[16];

private:
	friend class ChipInfoPrivate_::Device_;
	friend class ChipInfoPrivate_::MemoryLayoutInfo_;
	friend class ChipInfoPrivate_::MemoryClasInfo_;
	friend class ChipInfoPrivate_::MemoryInfo_;
	const ChipInfoPrivate_::Device_ *Find(const DieInfo &qry, DieInfoEx &info);
	void UpdateFastFlash();
};

