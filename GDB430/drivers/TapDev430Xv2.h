#pragma once

#include "ITapDev.h"


class TapDev430Xv2 : public ITapDev
{
public:
	// Get device into JTAG control
	virtual bool GetDevice(CoreId &coreid) override;
	// Get device into JTAG control and resets firmware
	//virtual bool SyncJtag() override;
	// Sync JTAG, performs Power-On-Reset and saves CPU context
	virtual bool SyncJtagAssertPorSaveContext(CpuContext &ctx, const ChipProfile &prof) override;
	// Similar to SyncJtagAssertPorSaveContext, without resetting
	virtual bool SyncJtagConditionalSaveContext(CpuContext &ctx, const ChipProfile &prof) override;
	// Executes a POR (Power on reset)
	virtual bool ExecutePOR() override;
	// Releases the device
	virtual void ReleaseDevice(address_t address) override;

	// Sets the PC value
	virtual bool SetPC(address_t address) override;
	// Sets a value into a CPU register
	virtual bool SetReg(uint8_t reg, address_t address) override;

	// CPU registers have to be read in a transaction scope. Starts with this call
	virtual bool StartGetRegs() override;
	// Reads a CPU register value
	virtual uint32_t GetReg(uint8_t reg) override;
	// CPU registers have to be read in a transaction scope. Stops with this call
	virtual void StopGetRegs() override;

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

// Experimental
public:
	void ReadWordsXv2_uif(address_t address, uint16_t *buf, uint32_t len);

protected:
	uint32_t GetRegInternal(uint8_t reg);
	bool WaitForSynch();
	void DisableLpmx5(const ChipProfile &prof);
	void SyncJtagXv2();

protected:
	uint32_t back_r0_;
};

