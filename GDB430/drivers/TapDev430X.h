#pragma once

#include "TapDev430.h"

class TapDev430X : public TapDev430
{
public:
	// Load default profile according to MCU architecture
	virtual void InitDefaultChip(ChipProfile &prof) override;
	// Sync JTAG, performs Power-On-Reset and saves CPU context
	virtual bool SyncJtagAssertPorSaveContext(CpuContext &ctx, const ChipProfile &prof) override;
	// Similar to SyncJtagAssertPorSaveContext, without resetting
	virtual bool SyncJtagConditionalSaveContext(CpuContext &ctx, const ChipProfile &prof) override;
	// Sets the PC value
	virtual bool SetPC(address_t address) override;
	// Sets a value into a CPU register
	virtual bool SetReg(uint8_t reg, address_t address) override;
	// Reads a CPU register value
	virtual uint32_t GetReg(uint8_t reg) override;
	// Reads a word from a word aligned address
	virtual uint16_t ReadWord(address_t address) override;
	// Reads a set of words
	virtual bool ReadWords(address_t address, unaligned_u16 *buf, uint32_t word_count) override;
	// Write a word into a word aligned memory position
	virtual bool WriteWord(address_t address, uint16_t data) override;
	// Writes a set of words
	virtual bool WriteWords(address_t address, const unaligned_u16 *buf, uint32_t word_count) override;
	
	// Writes to flash memory
	virtual bool WriteFlash(address_t address, const unaligned_u16 *buf, uint32_t word_count) override;
	// Erases flash memory
	virtual bool EraseFlash(address_t address, const uint16_t fctl1, const uint16_t fctl3, bool mass_erase) override;
	// Executes a POR (Power on reset)
	virtual bool ExecutePOR() override;

	// Set breakpoints
	virtual void UpdateEemBreakpoints(Breakpoints &bkpts, const ChipProfile &prof) override;
};

