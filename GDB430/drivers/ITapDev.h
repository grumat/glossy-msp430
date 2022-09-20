#pragma once

#include "util/util.h"
#include "JtagId.h"

class ChipProfile;
struct DieInfo;
class Breakpoints;


// Controls erase
enum EraseMode
{
	kSimpleErase,
	kMassErase,
};


typedef uint16_t unaligned_u16 __attribute__((aligned(1)));

// Internal MCU IDs
struct CoreId
{
	JtagId jtag_id_;
	// Valid for all CPUs
	uint16_t id_data_addr_;
	// Valid for CPUXv2
	uint16_t coreip_id_;
	uint32_t ip_pointer_;
	// Legacy MSP430 only
	uint16_t device_id_;

	ALWAYS_INLINE void Init()
	{
		jtag_id_ = kInvalid;
		coreip_id_ = 0;
		id_data_addr_ = 0x0FF0;
		ip_pointer_ = 0;
		device_id_ = 0;
	}
	ALWAYS_INLINE bool IsMSP430() const
	{
		return jtag_id_ == kMspStd || jtag_id_ == kMsp_8D || jtag_id_ == kMsp_91
			|| jtag_id_ == kMsp_95 || jtag_id_ == kMsp_98 || jtag_id_ == kMsp_99;
	}

	ALWAYS_INLINE bool IsXv2() const
	{
		return jtag_id_ == kMsp_91 || jtag_id_ == kMsp_95 || jtag_id_ == kMsp_98
			|| jtag_id_ == kMsp_99;
	}
};


static constexpr uint16_t WDT_ADDR_CPU = 0x0120;
static constexpr uint16_t WDT_ADDR_XV2 = 0x015C;
static constexpr uint16_t WDT_ADDR_FR41XX = 0x01CC;

static constexpr uint16_t WDT_PASSWD = 0x5A00;
static constexpr uint16_t WDT_HOLD = 0x5A80;


// Used to save the CPU context
struct CpuContext
{
	// ID of target read during init
	JtagId jtag_id_;
	// Is target running?
	bool is_running_;
	// CPU in interrupt
	bool in_interrupt_;
	// EEM Version CpuXv2 only
	uint32_t eem_version_;
	// Clock Control (UIF: _hal_mclkCntrl0)
	uint32_t eem_clk_ctrl_;
	// Current WDT register value
	uint8_t wdt_;
	uint32_t pc_;
	uint32_t sr_;

	ALWAYS_INLINE void Init(JtagId jtag_id)
	{
		jtag_id_ = jtag_id;
		is_running_ = false;
		in_interrupt_ = false;
		eem_version_ = 0;
		eem_clk_ctrl_ = 0x0417;
		wdt_ = 0;
		pc_ = 0;
		sr_ = 0;
	}
};


/*!
General mask rules (*):             SLAU049 SLAU056 SLAU144 SLAU208 SLAU259 SLAU335
	Flash Key		: 0xA500A500	   X       X       X       X       X       X
	ERASE bit (1)	: 0x00000002	   X       X       X       X       X       X
	MERAS bit (1)	: 0x00000004	   X       X       X       X       X       X
	GMERAS bit (1)	: 0x00000008	           X
	WRT bit (2)		: 0x00000040       X       X       X       X       X       X
	BLKWRT bit (2)	: 0x00000040       X       X       X       X       X       X
	LOCK bit (3)	: 0x00100000       X       X       X       X       X       X
	LOCKA bit (3)	: 0x00400000               X      (4)      X       X      (5)

	(*) SLAU321, SLAU367, SLAU378, SLAU445 and SLAU506 families has no Embedded Flash 
		Controller.
	(1)	The meaning of ERASE+MERAS+GMERAS combinations depends on family. On SLAU208
		and SLAU259 a Mass Erase does not affect Info memory.
	(2) The meaning of WRT+BLKWRT combinations depends on family.
	(3) LOCK and LOCKA bits have negative logic.
	(4)	SLAU144 stores TLV data into the INFOA. So this is mostly protected from
		accidental erases. Only Segment Erase is allowed for this family.
	(5) This bit is called LOCKSEG on SLAU335 as the Info Memory is just a single 
		block. As with most parts this does not store the TLV (Factory calibration 
		values) and is should be erased by an Mass Erase command.
*/

namespace Fctl1Flags
{
enum
{
	// FCTL1 bits
	ERASE = 0x02,
	MERAS = 0x04,
	GMERAS = 0x08,
	WRT = 0x40,
};
}

namespace Fctl3Flags
{
enum
{
	// FCTL3 register
	LOCK = 0x10,
	LOCKA = 0x40,
};
}

// Address of Flash registers
static constexpr uint16_t kFctl1Addr = 0x0128;
static constexpr uint16_t kFctl2Addr = 0x012A;
static constexpr uint16_t kFctl3Addr = 0x012C;

// Common values for FCTL1 register
static constexpr uint16_t kFctl1Lock = 0xA500;
// Write flag
static constexpr uint16_t kFctl1Wrt = 0xA540;

// Common values for FCTL2 register
static constexpr uint16_t kFctl2MclkDiv0 = 0xA540;

// Common values for FCTL3 register
static constexpr uint16_t kFctl3Unlock = 0xA500;
static constexpr uint16_t kFctl3UnlockA = 0xA540;

//static constexpr uint16_t kFctl3Unlock_Xv2 = 0xA548;

#pragma pack(1)
// Combo of FCTL1 and FCTL3 registers
union FlashEraseFlags
{
	uint32_t raw_;
	struct 
	{
		// FCTL1 register
		uint8_t fctl1_;
		// The 0xA5 register key
		uint8_t key1_;
		// FCTL3 register
		uint8_t fctl3_;
		// The 0xA5 register key
		uint8_t key2_;
	} b;
	struct
	{
		// FCTL1 register
		uint16_t fctl1_;
		// FCTL3 register
		uint16_t fctl3_;
	} w;
	
// Constructor
	ALWAYS_INLINE FlashEraseFlags(const bool has_locka, const bool unlock)
	{
		raw_ = 0xA500A500;
		if (has_locka
			&& !unlock)
		{
			b.fctl3_ |= 0x40;
		}
	}
	// Erase segment mode
	ALWAYS_INLINE void EraseSegment() { b.fctl1_ |= Fctl1Flags::ERASE; }
	// Main erase mode
	ALWAYS_INLINE void MainErase(const bool gmeras)
	{
		uint8_t bits = gmeras 
			? Fctl1Flags::GMERAS | Fctl1Flags::MERAS 
			: Fctl1Flags::MERAS;
		b.fctl1_ |= bits;
	}
	// Mass erase mode
	ALWAYS_INLINE void MassErase(const bool gmeras)
	{
		uint8_t bits = gmeras 
			? Fctl1Flags::GMERAS | Fctl1Flags::MERAS | Fctl1Flags::ERASE 
			: Fctl1Flags::MERAS | Fctl1Flags::ERASE;
		b.fctl1_ |= bits;
	}
};
#pragma pack()


class ITapDev
{
public:
	// Load default profile according to MCU architecture
	virtual void InitDefaultChip(ChipProfile &prof) = 0;
	// Get device into JTAG control
	virtual bool GetDevice(CoreId &coreid) = 0;
	// Get MCU into JTAG control and resets firmware
	//virtual bool SyncJtag() = 0;
	// Sync JTAG, performs Power-On-Reset and saves CPU context
	virtual bool SyncJtagAssertPorSaveContext(CpuContext &ctx, const ChipProfile &prof) = 0;
	// Similar to SyncJtagAssertPorSaveContext, without resetting
	virtual bool SyncJtagConditionalSaveContext(CpuContext &ctx, const ChipProfile &prof) = 0;
	// Executes a POR (Power on reset)
	virtual bool ExecutePOR() = 0;
	// Releases the device
	virtual void ReleaseDevice(address_t address) = 0;

	// Fills the device identification data
	virtual bool GetDeviceSignature(DieInfo &id, CpuContext &ctx, const CoreId &coreid) = 0;

	// Sets the PC value
	virtual bool SetPC(address_t address) = 0;
	// Sets a value into a CPU register
	virtual bool SetReg(uint8_t reg, address_t address) = 0;
	// CPU registers have to be read in a transaction scope. Starts with this call
	virtual bool GetRegs_Begin() = 0;
	// Reads a CPU register value
	virtual uint32_t GetReg(uint8_t reg) = 0;
	// CPU registers have to be read in a transaction scope. Stops with this call
	virtual void GetRegs_End() = 0;

	// Reads a word from a word aligned address
	virtual uint16_t ReadWord(address_t address) = 0;
	// Reads a set of words
	virtual void ReadWords(address_t address, unaligned_u16 *buf, uint32_t word_count) = 0;
	// Write a word into a word aligned memory position
	virtual void WriteWord(address_t address, uint16_t data) = 0;
	// Writes a set of words
	virtual void WriteWords(address_t address, const unaligned_u16 *buf, uint32_t word_count) = 0;
	// Writes to flash memory
	virtual void WriteFlash(address_t address, const unaligned_u16 *buf, uint32_t word_count) = 0;
	// Erases flash memory
	virtual bool EraseFlash(address_t address, const FlashEraseFlags flags, EraseMode mass_erase) = 0;
	
	// Set breakpoints
	virtual void UpdateEemBreakpoints(Breakpoints &bkpts, const ChipProfile &prof) = 0;
	
	// Single step
	virtual bool SingleStep() = 0;
};

