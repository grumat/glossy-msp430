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

#include "util/util.h"
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
static constexpr uint16_t Fctl1Lock = 0xA500;
static constexpr uint16_t Fctl1Lock_X = 0xA500;
static constexpr uint16_t Fctl1Lock_Xv2 = 0xA500;

// Common values for FCTL3 register
static constexpr uint16_t Fctl3Unlock = 0xA500;
static constexpr uint16_t Fctl3Lock = 0xA510;

static constexpr uint16_t Fctl3Unlock_X = 0xA500;
static constexpr uint16_t Fctl3UnlockA_X = 0xA540;
static constexpr uint16_t Fctl3Lock_X = 0xA510;

static constexpr uint16_t Fctl3Unlock_Xv2 = 0xA548;
static constexpr uint16_t Fctl3UnlockA_Xv2 = 0xA508;
static constexpr uint16_t Fctl3Lock_Xv2 = 0xA558;


//! Flash erasing modes (FCTL1 and FCTL3 register values)
enum EraseModeFctl
{
	// Erase individual segment only
	kSegmentEraseSlau049 = ((uint32_t)Fctl3Unlock << 16) | 0xA502
	, kSegmentEraseSlau056 = ((uint32_t)Fctl3Unlock_X << 16) | 0xA502
	, kSegmentEraseSlau144 = ((uint32_t)Fctl3Unlock_X << 16) | 0xA502
	, kSegmentEraseSlau208 = ((uint32_t)Fctl3Unlock_Xv2 << 16) | 0xA502
	// Erase main memory segments of all memory arrays.
	, kMainEraseSlau049 = ((uint32_t)Fctl3Unlock << 16) | 0xA504
	, kMainEraseSlau056 = ((uint32_t)Fctl3Unlock_X << 16) | 0xA50C
	, kMainEraseSlau144 = ((uint32_t)Fctl3Unlock_X << 16) | 0xA504
	, kMainEraseSlau208 = ((uint32_t)Fctl3Unlock_Xv2 << 16) | 0xA506
	// Erase all main and information memory segments
	, kMassEraseSlau049 = ((uint32_t)Fctl3Unlock << 16) | 0xA506
	, kMassEraseSlau056 = ((uint32_t)Fctl3Unlock_X << 16) | 0xA50E
	, kMassEraseSlau144 = ((uint32_t)Fctl3Unlock_X << 16) | 0xA506
};


 //! Generic TAP device
class TapDev : public TapPlayer
{
public:
	enum JtagId
	{
		kInvalid = 0
		, kMspStd = 0x89
		, kMsp_8D = 0x8D
		, kMsp_91 = 0x91
		, kMsp_95 = 0x95
		, kMsp_98 = 0x98
		, kMsp_99 = 0x99
		// SLAU320AJ
		, JTAG_ID91 = kMsp_91
		, JTAG_ID98 = kMsp_98
		, JTAG_ID99 = kMsp_99
	};

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

	//! Function pointer template to set value of PC
	typedef bool (TapDev:: *FnSetPC)(address_t address);
	typedef bool (TapDev:: *FnSetReg)(uint8_t reg, uint32_t value);
	typedef uint32_t(TapDev:: *FnGetReg)(uint8_t reg);
	typedef uint16_t (TapDev:: *FnReadWord)(address_t address);
	typedef bool (TapDev:: *FnReadWords)(address_t address, uint16_t *buf, uint32_t word_count);
	typedef bool (TapDev:: *FnWriteWord)(address_t address, uint16_t data);
	typedef bool (TapDev:: *FnWriteWords)(address_t address, const uint16_t *buf, uint32_t word_count);
	typedef bool (TapDev:: *FnWriteFlash)(address_t address, const uint16_t *buf, uint32_t word_count);
	typedef bool (TapDev:: *FnEraseFlash)(address_t address, const uint16_t fctl1, const uint16_t fctl3);
	typedef bool (TapDev:: *FnExecutePOR)();
	typedef void (TapDev:: *FnReleaseDevice)(address_t address);

	struct CpuTraitsFuncs
	{
		//! Sets value of PC method entry
		FnSetPC fnSetPC;
		FnSetReg fnSetReg;
		FnGetReg fnGetReg;
		//
		FnReadWord fnReadWord;
		FnReadWords fnReadWords;
		//
		FnWriteWord fnWriteWord;
		FnWriteWords fnWriteWords;
		FnWriteFlash fnWriteFlash;
		//! Flash erasing
		FnEraseFlash fnEraseFlash;
		//! Executes Power on reset
		FnExecutePOR fnExecutePOR;
		//! Release device
		FnReleaseDevice fnReleaseDevice;
	};

public:
	bool Open(ITapInterface &itf);
	void Close();
	//! Initialize TAP
	JtagId Init();
	bool StartMcu(ChipInfoDB::CpuArchitecture arch, bool fast_flash, bool issue_1377);

public:
	bool ExecutePOR() { return (this->*traits_->fnExecutePOR)(); }
	JtagId GetDevice();
	JtagId GetDeviceXv2();
	//! Reads one byte/word from a given address
	void ReadWords(address_t address, uint16_t *buf, uint32_t len);
	uint16_t ReadWord(address_t address);
	//! Reads a register from the target CPU
	address_t ReadReg(int reg) { return (this->*traits_->fnGetReg)(reg); }
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

	ALWAYS_INLINE bool IsMSP430()
	{
		return jtag_id_ == kMspStd || jtag_id_ == kMsp_8D || jtag_id_ == kMsp_91
			|| jtag_id_ == kMsp_95 || jtag_id_ == kMsp_98 || jtag_id_ == kMsp_99;
	}

	ALWAYS_INLINE bool IsXv2()
	{
		return jtag_id_ == kMsp_91 || jtag_id_ == kMsp_95 || jtag_id_ == kMsp_98
			|| jtag_id_ == kMsp_99;
	}
	bool ReadChipId(void *buf, uint32_t size);

protected:
	TapDev::JtagId SetInstructionFetch();
	bool HaltCpu();
#if 0
	bool VerifyPsa(uint32_t start_address, uint32_t length, const uint16_t *data);
#endif
	bool IsFuseBlown();

private:
	ALWAYS_INLINE void ReadWordsXv2_uif(address_t address, uint16_t *buf, uint32_t len);

private:
	bool SetPc_slau320aj(address_t address);
	bool SetPcX_slau320aj(address_t address);
	bool SetPcXv2_slau320aj(address_t address);
	bool SetReg_uif(uint8_t reg, uint32_t value);
	bool SetRegX_uif(uint8_t reg, uint32_t value);
	bool SetRegXv2_uif(uint8_t reg, uint32_t value);
	uint32_t GetReg_uif(uint8_t reg);
	uint32_t GetRegX_uif(uint8_t reg);
	uint32_t GetRegXv2_uif(uint8_t reg);

private:
	uint16_t ReadWord_slau320aj(address_t address);
	uint16_t ReadWordX_slau320aj(address_t address);
	uint16_t ReadWordXv2_slau320aj(address_t address);
	bool ReadWords_slau320aj(address_t address, uint16_t *buf, uint32_t word_count);
	bool ReadWordsX_slau320aj(address_t address, uint16_t *buf, uint32_t word_count);
	bool ReadWordsXv2_slau320aj(address_t address, uint16_t *buf, uint32_t word_count);

private:
	bool WriteWord_slau320aj(address_t address, uint16_t data);
	bool WriteWordX_slau320aj(address_t address, uint16_t data);
	bool WriteWordXv2_slau320aj(address_t address, uint16_t data);
	bool WriteWords_slau320aj(address_t address, const uint16_t *buf, uint32_t word_count);
	bool WriteWordsX_slau320aj(address_t address, const uint16_t *buf, uint32_t word_count);
	bool WriteWordsXv2_slau320aj(address_t address, const uint16_t *buf, uint32_t word_count);
	bool WriteFlash_slau320aj(address_t address, const uint16_t *buf, uint32_t word_count);
	bool WriteFlashX_slau320aj(address_t address, const uint16_t *buf, uint32_t word_count);
	bool WriteFlashXv2_slau320aj(address_t address, const uint16_t *buf, uint32_t word_count);

private:
	bool EraseFlash_slau320aj(address_t address, const uint16_t fctl1, const uint16_t fctl3);
	bool EraseFlashX_slau320aj(address_t address, const uint16_t fctl1, const uint16_t fctl3);
	bool EraseFlashXv2_slau320aj(address_t address, const uint16_t fctl1, const uint16_t fctl3);

private:
	bool ExecutePor_slau320aj();
	bool ExecutePorX_slau320aj();
	bool ExecutePorXv2_slau320aj();

private:
	void ReleaseDevice_slau320aj(address_t address);
	void ReleaseDeviceXv2_slau320aj(address_t address);
	bool SyncJtag_AssertPor();


// MSP-FET-UIF code portability
private:
	ALWAYS_INLINE void addr_capture() { itf_->OnIrShift(IR_ADDR_CAPTURE); }
	ALWAYS_INLINE void addr_16bit() { itf_->OnIrShift(IR_ADDR_16BIT); }
	ALWAYS_INLINE void cntrl_sig_16bit() { itf_->OnIrShift(IR_CNTRL_SIG_16BIT); }
	ALWAYS_INLINE JtagId cntrl_sig_capture() { return (JtagId)(itf_->OnIrShift(IR_CNTRL_SIG_CAPTURE)); }
	ALWAYS_INLINE void cntrl_sig_high_byte() { itf_->OnIrShift(IR_CNTRL_SIG_HIGH_BYTE); }
	ALWAYS_INLINE uint8_t core_ip_pointer() { return itf_->OnIrShift(IR_COREIP_ID); }
	ALWAYS_INLINE uint8_t data_16bit() { return itf_->OnIrShift(IR_DATA_16BIT); }
	ALWAYS_INLINE uint8_t data_capture() { return itf_->OnIrShift(IR_DATA_CAPTURE); }
	ALWAYS_INLINE uint8_t data_quick() { return itf_->OnIrShift(IR_DATA_QUICK); }
	ALWAYS_INLINE uint8_t data_to_addr() { return itf_->OnIrShift(IR_DATA_TO_ADDR); }
	ALWAYS_INLINE uint8_t device_ip_pointer() { return itf_->OnIrShift(IR_DEVICE_ID); }
	ALWAYS_INLINE void halt_cpu() { HaltCpu(); }
	ALWAYS_INLINE void instrLoad() { itf_->OnInstrLoad(); }
	ALWAYS_INLINE void release_cpu() { ReleaseCpu(); }
	ALWAYS_INLINE uint8_t SetReg_8Bits(uint8_t n) { return itf_->OnDrShift8(n); }
	ALWAYS_INLINE uint16_t SetReg_16Bits(uint16_t n) { return itf_->OnDrShift16(n); }
	ALWAYS_INLINE uint32_t SetReg_20Bits(uint32_t n) { return itf_->OnDrShift20(n); }
	ALWAYS_INLINE void IHIL_Tclk(const bool b) { b ? itf_->OnSetTclk() : itf_->OnClearTclk(); }
	ALWAYS_INLINE void IHIL_TCLK() { itf_->OnPulseTclkN(); }

// SLAU320AJ compatibility
private:
	ALWAYS_INLINE uint8_t IR_Shift(uint8_t ir) { return itf_->OnIrShift(ir); }
	ALWAYS_INLINE void ClrTCLK() { itf_->OnClearTclk(); }
	ALWAYS_INLINE void SetTCLK() { itf_->OnSetTclk(); }
	ALWAYS_INLINE uint8_t DR_Shift8(uint8_t n) { return itf_->OnDrShift8(n); }
	ALWAYS_INLINE uint16_t DR_Shift16(uint16_t n) { return itf_->OnDrShift16(n); }
	ALWAYS_INLINE uint32_t DR_Shift20(uint32_t n) { return itf_->OnDrShift20(n); }
	ALWAYS_INLINE void TCLKstrobes(uint32_t n) { itf_->OnFlashTclk(n); }
	ALWAYS_INLINE uint32_t i_ReadJmbOut() { return itf_->OnReadJmbOut(); }
	ALWAYS_INLINE bool i_WriteJmbIn16(uint16_t data) { return itf_->OnWriteJmbIn16(data); }

protected:
	const CpuTraitsFuncs *traits_;
	bool failed_;
	bool issue_1377_;
	bool fast_flash_;
	JtagId jtag_id_;
	uint16_t coreip_id_;
	uint16_t id_data_addr_;
	uint32_t ip_pointer_;

protected:
	static const CpuTraitsFuncs msp430legacy_;
	static const CpuTraitsFuncs msp430X_;
	static const CpuTraitsFuncs msp430Xv2_;
};

