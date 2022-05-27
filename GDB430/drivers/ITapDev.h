#pragma once

#include "util/util.h"
#include "JtagId.h"

class ChipProfile;
struct DieInfo;
class Breakpoints;

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
	virtual bool ReadWords(address_t address, uint16_t *buf, uint32_t word_count) = 0;
	// Write a word into a word aligned memory position
	virtual bool WriteWord(address_t address, uint16_t data) = 0;
	// Writes a set of words
	virtual bool WriteWords(address_t address, const uint16_t *buf, uint32_t word_count) = 0;
	// Writes to flash memory
	virtual bool WriteFlash(address_t address, const uint16_t *buf, uint32_t word_count) = 0;
	// Erases flash memory
	virtual bool EraseFlash(address_t address, const uint16_t fctl1, const uint16_t fctl3, bool mass_erase) = 0;
	
	// Set breakpoints
	virtual void UpdateEemBreakpoints(Breakpoints &bkpts, const ChipProfile &prof) = 0;
	
	// Single step
	virtual bool SingleStep() = 0;
};

