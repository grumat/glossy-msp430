#pragma once

#include "util/util.h"
#include "JtagId.h"


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
};


class ITapDev
{
public:
	// Get device into JTAG control
	virtual bool GetDevice(CoreId &coreid) = 0;
	// Get MCU into JTAG control and resets firmware
	virtual bool SyncJtag() = 0;
	// Executes a POR (Power on reset)
	virtual bool ExecutePOR() = 0;
	// Releases the device
	virtual void ReleaseDevice(address_t address) = 0;

	// Sets the PC value
	virtual bool SetPC(address_t address) = 0;
	// Sets a value into a CPU register
	virtual bool SetReg(uint8_t reg, address_t address) = 0;
	// CPU registers have to be read in a transaction scope. Starts with this call
	virtual bool StartGetRegs() = 0;
	// Reads a CPU register value
	virtual uint32_t GetReg(uint8_t reg) = 0;
	// CPU registers have to be read in a transaction scope. Stops with this call
	virtual void StopGetRegs() = 0;

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
	virtual bool EraseFlash(address_t address, const uint16_t fctl1, const uint16_t fctl3) = 0;
};

