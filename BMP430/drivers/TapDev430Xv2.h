#pragma once

#include "ITapDev.h"

class TapDev430Xv2 : public ITapDev
{
public:
	// Sets the PC value
	virtual bool SetPC(address_t address) override;
	// Sets a value into a CPU register
	virtual bool SetReg(uint8_t reg, address_t address) override;
	// Reads a CPU register value
	virtual uint32_t GetReg(uint8_t reg) override;
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
	virtual bool EraseFlash(address_t address, const uint16_t fctl1, const uint16_t fctl3) override;
	// Get device into JTAG control
	virtual bool GetDevice(CoreId &coreid) override;
	// Get device into JTAG control and resets firmware
	virtual bool SyncJtag() override;
	// Executes a POR (Power on reset)
	virtual bool ExecutePOR() override;
	// Releases the device
	virtual void ReleaseDevice(address_t address) override;

// Experimental
public:
	void ReadWordsXv2_uif(address_t address, uint16_t *buf, uint32_t len);
};

