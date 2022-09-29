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
	// Restores the CPU context and releases Jtag control
	virtual void ReleaseDevice(CpuContext &ctx, const ChipProfile &prof, bool run_to_bkpt, uint16_t mdbval = kSwBkpInstr) override;

	// Sets the PC value
	virtual bool SetPC(address_t address) override;
	// Sets a value into a CPU register
	virtual bool SetReg(uint8_t reg, address_t address) override;
	// Reads a CPU register value
	virtual uint32_t GetReg(uint8_t reg) override;
	// Reads a word from a word aligned address
	virtual uint16_t ReadWord(address_t address) override;
	// Reads a set of words
	virtual void ReadWords(address_t address, unaligned_u16 *buf, uint32_t word_count) override;
	// Write a word into a word aligned memory position
	virtual void WriteWord(address_t address, uint16_t data) override;
	// Writes a set of words
	virtual void WriteWords(address_t address, const unaligned_u16 *buf, uint32_t word_count) override;
	
	// Writes to flash memory
	virtual void WriteFlash(address_t address, const unaligned_u16 *buf, uint32_t word_count) override;
	// Erases flash memory
	virtual bool EraseFlash(address_t address, const FlashEraseFlags flags, EraseMode mass_erase) override;
	// Executes a POR (Power on reset)
	virtual bool ExecutePOR() override;

	// Single step
	virtual bool SingleStep(CpuContext &ctx, const ChipProfile &prof,
							uint16_t mdbval = kSwBkpInstr) override;

	// Set breakpoints
	virtual void UpdateEemBreakpoints(Breakpoints &bkpts, const ChipProfile &prof) override;
};

