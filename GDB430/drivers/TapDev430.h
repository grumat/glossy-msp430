#pragma once

#include "TapPlayer.h"


class TapDev430 : public ITapDev
{
public:
	static constexpr uint16_t MAX_TCE1 = 10;
public:
	// Load default profile according to MCU architecture
	virtual void InitDefaultChip(ChipProfile &prof) override;
	// Get device into JTAG control
	virtual bool GetDevice(CoreId &coreid) override;
	// Get device into JTAG control and resets firmware
	//virtual bool SyncJtag() override { return ExecutePOR(); }
	// Sync JTAG, performs Power-On-Reset and saves CPU context
	virtual bool SyncJtagAssertPorSaveContext(CpuContext &ctx, const ChipProfile &prof) override;
	// Similar to SyncJtagAssertPorSaveContext, without resetting
	virtual bool SyncJtagConditionalSaveContext(CpuContext &ctx, const ChipProfile &prof) override;
	// Executes a POR (Power on reset)
	virtual bool ExecutePOR() override;
	// Releases the device
	virtual void ReleaseDevice(address_t address) override;

	// Fills the device identification data
	virtual bool GetDeviceSignature(DieInfo &id, CpuContext &ctx, const CoreId &coreid) override;

	// Sets the PC value
	virtual bool SetPC(address_t address) override;
	// Sets a value into a CPU register
	virtual bool SetReg(uint8_t reg, address_t address) override;

	// CPU registers have to be read in a transaction scope. Starts with this call
	virtual bool GetRegs_Begin() override { return true; }
	// Reads a CPU register value
	virtual uint32_t GetReg(uint8_t reg) override;
	// CPU registers have to be read in a transaction scope. Stops with this call
	virtual void GetRegs_End() override {}

	// Reads a word from a word aligned address
	virtual uint16_t ReadWord(address_t address) override;
	// Reads a set of words
	virtual bool ReadWords(address_t address, uint16_t *buf, uint32_t word_count) override;
	// Write a word into a word aligned memory position
	virtual bool WriteWord(address_t address, uint16_t data) override;
	// Writes a set of words
	virtual bool WriteWords(address_t address, const uint16_t *buf, uint32_t word_count) override;
	
	// Writes to flash memory
	virtual bool WriteFlash(address_t address, const uint16_t *buf, uint32_t word_count) override;
	// Erases flash memory
	virtual bool EraseFlash(address_t address, const uint16_t fctl1, const uint16_t fctl3, bool mass_erase) override;

	// Set breakpoints
	virtual void UpdateEemBreakpoints(Breakpoints &bkpts, const ChipProfile &prof) override;

	// Single step
	virtual bool SingleStep() override;

public:
	JtagId SetInstructionFetch();
	bool HaltCpu();
	uint16_t SyncJtag();
	bool IsInstrLoad();
	bool InstrLoad();
	bool ClkTclkAndCheckDTC();
};

