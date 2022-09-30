#pragma once

#include "TapDev430X.h"


class TapDev430Xv2 : public TapDev430X
{
public:
	// Load default profile according to MCU architecture
	virtual void InitDefaultChip(ChipProfile &prof) override;
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
	// Restores the CPU context and releases Jtag control
	virtual void ReleaseDevice(CpuContext &ctx, const ChipProfile &prof, bool run_to_bkpt, uint16_t mdbval = kSwBkpInstr) override;

	// Fills the device identification data
	virtual bool GetDeviceSignature(DieInfo &id, CpuContext &ctx, const CoreId &coreid) override;

	// Sets the PC value
	virtual bool SetPC(address_t address) override;
	// Sets a value into a CPU register
	virtual bool SetReg(uint8_t reg, address_t address) override;

	// CPU registers have to be read in a transaction scope. Starts with this call
	virtual bool GetRegs_Begin() override;
	// Reads a CPU register value
	virtual uint32_t GetReg(uint8_t reg) override;
	// CPU registers have to be read in a transaction scope. Stops with this call
	virtual void GetRegs_End() override;

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

	// Single step
	virtual bool SingleStep(CpuContext &ctx, const ChipProfile &prof, uint16_t mdbval = kSwBkpInstr) override;

#if 0
// Experimental
public:
	void ReadWordsXv2_uif(address_t address, uint16_t *buf, uint32_t len);
#endif

protected:
	uint32_t GetRegInternal(uint8_t reg);
	bool WaitForSynch();
	void DisableLpmx5(const ChipProfile &prof);
	void SyncJtagXv2();
	// TODO: generalize. Read
	uint32_t EemDataExchangeXv2(uint8_t xchange, const CpuContext &ctx);
	// TODO: generalize. Write
	void EemDataExchangeXv2(uint8_t xchange, uint32_t data, CpuContext &ctx);

protected:
	uint32_t back_r0_;
};

