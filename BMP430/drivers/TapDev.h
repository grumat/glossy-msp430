/* MSPDebug - debugging tool for MSP430 MCUs
 * Copyright (C) 2009-2012 Daniel Beer
 * Copyright (C) 2012-2015 Peter Bägel
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

/* jtag functions are taken from TIs SLAA149–September 2002
 *
 * breakpoint implementation influenced by a posting of Ruisheng Lin
 * to Travis Goodspeed at 2012-09-20 found at:
 * http://sourceforge.net/p/goodfet/mailman/message/29860790/
 *
 *
 * 2012-10-03 Peter Bägel (DF5EQ)
 * 2012-10-03   initial release          Peter Bägel (DF5EQ)
 * 2014-12-26   jtag_single_step added   Peter Bägel (DF5EQ)
 * 2015-02-21   jtag_set_breakpoint added   Peter Bägel (DF5EQ)
 *              jtag_cpu_state      added
 */

#pragma once

#include "TapPlayer.h"



#define SAFE_PC_ADDRESS (0x00000004ul)


// dedicated addresses
//! \brief Triggers a regular reset on device release from JTAG control
#define V_RESET					0xFFFE
//! \brief Triggers a "brown-out" reset on device release from JTAG control
#define V_BOR					0x1B08
//! \brief Triggers a regular reset on device release from JTAG control
#define V_RUNNING				0xFFFF


class TapDev;


// Common values for FCTL1 register
static constexpr uint16_t kFctl1Lock = 0xA500;
static constexpr uint16_t kFctl1Lock_X = 0xA500;
static constexpr uint16_t kFctl1Lock_Xv2 = 0xA500;

// Common values for FCTL3 register
static constexpr uint16_t kFctl3Unlock = 0xA500;
static constexpr uint16_t kFctl3Lock = 0xA510;

static constexpr uint16_t kFctl3Unlock_X = 0xA500;
static constexpr uint16_t kFctl3Lock_X = 0xA510;

static constexpr uint16_t kFctl3Unlock_Xv2 = 0xA548;


//! Flash erasing modes (dword contents: FCTL3<<16 | FCTL1)
/*!
General mask rules (*):             SLAU049 SLAU056 SLAU144 SLAU208 SLAU259 SLAU335
	Flash Key		: 0xA500A500	   X       X       X       X       X       X
	ERASE bit (1)	: 0x00000002	   X       X       X       X       X       X
	MERAS bit (1)	: 0x00000004	   X       X       X       X       X       X
	GMERAS bit (1)	: 0x00000008	           X
	WRT bit (2)		: 0x00000040       X       X       X       X       X       X
	BLKWRT bit (2)	: 0x00000040       X       X       X       X       X       X
	LOCK bit		: 0x00100000       X       X       X       X       X       X
	LOCKA bit		: 0x00400000               X      (3)      X       X      (4)

	(*) SLAU321, SLAU367, SLAU378, SLAU445 and SLAU506 families has no Embedded Flash 
		Controller.
	(1)	The meaning of ERASE+MERAS+GMERAS combinations depends on family. On SLAU208
		and SLAU259 a Mass Erase does not affect Info memory.
	(2) The meaning of WRT+BLKWRT combinations depends on family.
	(3)	SLAU144 stores TLV data into the INFOA. So this is mostly protected from
		accidental erases. Only Segment Erase is allowed for this family.
	(4) This bit is called LOCKSEG on SLAU335 as the Info Memory is just a single 
		block. As with most parts this does not store the TLV (Factory calibration 
		values) and is should be erased by an Mass Erase command.
*/
enum EraseModeFctl : uint32_t
{
	// Erase individual segment only
	kSegmentEraseGeneral = 0xA500A502	// x0x0xxxx xxxx001x
	, kSegmentEraseSlau049 = 0xA500A502	// xxx0xxxx xxxxx01x
	, kSegmentEraseSlau056 = 0xA500A502	// x0x0xxxx xxxx001x
	, kSegmentEraseSlau144 = 0xA500A502	// x0x0xxxx xxxxx01x
	, kSegmentEraseSlau208 = 0xA500A502	// x0x0xxxx xxxxx01x
	, kSegmentEraseSlau259 = 0xA500A502	// x0x0xxxx xxxxx01x
	, kSegmentEraseSlau335 = 0xA500A502	// x0x0xxxx xxxxx01x
	// Erase main memory segments of all memory arrays.
	, kMainEraseSlau049 = 0xA500A504	// xxx0xxxx xxxxx10x
	, kMainEraseSlau056 = 0xA500A50C	// x0x0xxxx xxxx110x
	, kMainEraseSlau144 = 0xA540A504	// x1x0xxxx xxxxx10x
	, kMainEraseSlau208 = 0xA500A506	// x0x0xxxx xxxxx11x
	, kMainEraseSlau259 = 0xA500A506	// x0x0xxxx xxxxx11x
	, kMainEraseSlau335 = 0xA500A504	// x0x0xxxx xxxxx10x
	// Erase all main and information memory segments
	, kMassEraseSlau049 = 0xA500A506	// xxx0xxxx xxxxx11x
	, kMassEraseSlau056 = 0xA500A50E	// x0x0xxxx xxxx111x
	, kMassEraseSlau144 = 0xA540A506	// x1x0xxxx xxxxx11x
	, kMassEraseSlau208 = 0xA500A506	// x0x0xxxx xxxxx11x
	, kMassEraseSlau259 = 0xA500A506	// x0x0xxxx xxxxx11x
	, kMassEraseSlau335 = 0xA500A506	// x0x0xxxx xxxxx11x
};


 //! Generic TAP device
class TapDev
{
public:
	enum Format
	{
		F_BYTE = 8,
		F_WORD = 16,
	};

	enum
	{
		kMinFlashPeriod = 2
		, kMaxEntryTry = 4
	};

public:
	bool Open(ITapInterface &itf);
	void Close();
	//! Initialize TAP
	JtagId Init();
	bool StartMcu(ChipInfoDB::CpuArchitecture arch, bool fast_flash, bool issue_1377);

public:
	bool GetDevice() { return traits_->GetDevice(core_id_); }
	bool ExecutePOR() { return traits_->ExecutePOR(); }
	//! Reads one byte/word from a given address
	void ReadWords(address_t address, uint16_t *buf, uint32_t len);
	uint16_t ReadWord(address_t address);

	//!
	bool StartReadReg() { return traits_->StartGetRegs(); }
	//! Reads a register from the target CPU
	address_t ReadReg(int reg) { return traits_->GetReg(reg); }
	//!
	void StopReadReg() { traits_->StopGetRegs(); }

	//! Writes a value into PC of the target CPU
	bool SetPC(address_t address);
	//! Writes a value into a register of the target CPU
	bool WriteReg(int reg, address_t value);
	//! Writes an array of words into target memory
	bool WriteWord(address_t address, uint16_t data);
	//! Writes an array of words into target memory
	bool WriteMem(address_t address, const uint16_t *data, uint32_t length);
	//! Release the target device from JTAG control
	void ReleaseDevice(address_t address);
	//! 
	bool SetBreakpoint(int bp_num, address_t bp_addr);
#if 0
	//! Performs a verification over the given memory range
	ALWAYS_INLINE bool VerifyMem(address_t start_address, uint32_t length, const uint16_t *data)
	{
		return VerifyPsa(start_address, length, data);
	}
	ALWAYS_INLINE bool EraseCheck(address_t start_address, uint32_t length)
	{
		return VerifyPsa(start_address, length, NULL);
	}
#endif
	//! Programs/verifies an array of words into a FLASH by using the FLASH controller.
	bool WriteFlash(address_t address, const uint16_t *data, uint32_t word_count);
	//! Performs a mass erase (with and w/o info memory) or a segment erase
	void EraseFlash(address_t erase_address, EraseModeFctl erase_mode);
	//!
	void SingleStep();
	//!
	bool GetCpuState();
	//!
	int GetConfigFuses();
	//!
	bool HasFailed() const { return failed_; }
	//!
	void ClearError() { failed_ = false; }

	ALWAYS_INLINE bool IsMSP430() const
	{
		return core_id_.IsMSP430();
	}

	ALWAYS_INLINE bool IsXv2() const
	{
		return core_id_.IsXv2();
	}
	// Checks i device is MSP430FR2xxx/MSP430FR41xx
	ALWAYS_INLINE bool IsFr41xx() const
	{
		return (core_id_.jtag_id_ == kMsp_98);
	}
	bool ReadChipId(void *buf, uint32_t size);

protected:
#if 0
	bool VerifyPsa(uint32_t start_address, uint32_t length, const uint16_t *data);
#endif
	bool IsFuseBlown();

public:
	ALWAYS_INLINE bool IsFlastFlash() const { return fast_flash_; }

public:
	bool failed_;

protected:
	ITapDev *traits_;
	CoreId core_id_;
	bool issue_1377_;
	bool fast_flash_;
};

// Singleton for the JTAG device and helper functions
extern TapDev g_JtagDev;
