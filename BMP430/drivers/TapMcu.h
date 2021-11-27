/* MSPDebug - debugging tool for MSP430 MCUs
 * Copyright (C) 2009, 2010 Daniel Beer
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

#pragma once

#include "util/util.h"
#include "TapDev.h"


#define DEVICE_NUM_REGS		16
#define DEVICE_MAX_BREAKPOINTS  32

struct chipinfo_memory;

enum device_bptype_t
{
	DEVICE_BPTYPE_BREAK,
	DEVICE_BPTYPE_WATCH,
	DEVICE_BPTYPE_READ,
	DEVICE_BPTYPE_WRITE
} ;

enum device_status_t
{
	DEVICE_STATUS_HALTED,
	DEVICE_STATUS_RUNNING,
	DEVICE_STATUS_INTR,
	DEVICE_STATUS_ERROR
};


#define DEVICE_BP_ENABLED       0x01
#define DEVICE_BP_DIRTY         0x02

struct device_breakpoint
{
	device_bptype_t type;
	address_t addr;
	int flags;
};


//! MCU being tested
class TapMcu
{
public:
	bool Open();
	void Close();

	bool IsAttached() const { return attached_; }
	//! Returns detected chip info
	inline const ChipProfile &GetChipProfile() const { return chip_info_; }

	ALWAYS_INLINE uint32_t GetReg(int reg)
	{
		if (!attached_)
			return UINT32_MAX;
		return OnGetReg(reg);
	}

	ALWAYS_INLINE bool SetReg(int reg, uint32_t val)
	{
		if (!attached_)
			return false;
		return OnSetReg(reg, val);
	}

	ALWAYS_INLINE int GetRegs(address_t *regs)
	{
		if (! attached_)
			return -1;
		return OnGetRegs(regs);
	}

	ALWAYS_INLINE int SetRegs(address_t *regs)
	{
		if (!attached_)
			return -1;
		return OnSetRegs(regs);
	}

	int ReadMem(address_t addr, void *mem, address_t len);

	int WriteMem(address_t addr, const void *mem, address_t len);

	bool EraseMain();
	bool EraseAll();
	bool EraseSegment(address_t addr);
	bool EraseRange(address_t addr, address_t size);

	ALWAYS_INLINE int SoftReset()
	{
		// Check flag before call
		assert(attached_);
		return OnSoftReset();
	}

	ALWAYS_INLINE int Run()
	{
		// Check flag before call
		assert(attached_);
		return OnRun();
	}

	ALWAYS_INLINE int SingleStep()
	{
		// Check flag before call
		assert(attached_);
		return OnSingleStep();
	}

	ALWAYS_INLINE int Halt()
	{
		// Check flag before call
		assert(attached_);
		return OnHalt();
	}

	ALWAYS_INLINE device_status_t Poll()
	{
		if (!attached_)
			return DEVICE_STATUS_ERROR;
		return OnPoll();
	}

	/*!
	Set or clear a breakpoint. The index of the modified entry is returned, or -1 if 
	no free entries were available. The modified entry is flagged so that it will be 
	reloaded on the next run.
	
	If which is specified, a particular breakpoint slot is modified. Otherwise, if 
	which < 0, breakpoint slots are selected automatically.
	*/
	int SetBrk(int which, int enabled, address_t address, device_bptype_t type);

	void ClearBrk();

public:
	/*!
	Breakpoint table. This should not be modified directly.
	Instead, you should use the device_setbrk() helper function. This
	will set the appropriate flags and ensure that the breakpoint is
	reloaded before the next run.
	*/
	int max_breakpoints;
	struct device_breakpoint breakpoints[DEVICE_MAX_BREAKPOINTS];

protected:
	address_t check_range(address_t addr, address_t size, const MemInfo **ret);
	int addbrk(address_t addr, device_bptype_t type);
	void delbrk(address_t addr, device_bptype_t type);
	int tlv_read(uint8_t *tlv_data);
	int tlv_find(const uint8_t *tlv_data, const uint8_t type, uint8_t *const size, const uint8_t **const ptr);
	void show_device_type();
	int device_is_fram();
	bool InitDevice();
	int write_flash_block(address_t addr, address_t len, const uint8_t *data);
	int refresh_bps();

	/*!
	Probe the device memory and extract ID bytes. This should be called after
	the device structure is ready.
	*/
	bool ProbeId();

// Methods here could be potentially promoted to overrides (kept normal calls for performance)
protected:
	ALWAYS_INLINE void OnClearState() { g_JtagDev.ClearError(); }
	int OnGetRegs(address_t *regs);
	int OnSetRegs(address_t *regs);
	uint32_t OnGetReg(int reg);
	bool OnSetReg(int reg, uint32_t val);
	address_t OnReadWords(address_t addr, void *data, address_t len);
	int OnWriteWords(const MemInfo *m, address_t addr, const void *data, address_t len);
	int OnSoftReset();
	int OnRun();
	int OnSingleStep();
	int OnHalt();
	device_status_t OnPoll();
	void OnReadChipId(void *buf, uint32_t size);
	int OnGetConfigFuses();

protected:
	bool attached_;
	// Device information loaded from device database
	ChipProfile chip_info_;
};



extern TapMcu g_tap_mcu;


#if 0
#include <stdint.h>
#include "util/util.h"
//#include "util/powerbuf.h"
#include "util/chipinfo.h"
#include "util/bytes.h"

struct device;
typedef struct device *device_t;

#define DEVICE_FLAG_JTAG	0x01 /* default is SBW */
#define DEVICE_FLAG_LONG_PW	0x02
#define DEVICE_FLAG_TTY		0x04 /* default is USB */
#define DEVICE_FLAG_FORCE_RESET	0x08
#define DEVICE_FLAG_DO_FWUPDATE 0x10
#define DEVICE_FLAG_SKIP_CLOSE	0x20
#define DEVICE_FLAG_BSL_NME     0x40 /* BSL no-mass-erase */

struct device_args {
	int			flags;
	int			vcc_mv;
	//const char		*path;
	const char		*forced_chip_id;
	const char		*requested_serial;
	const char		*require_fwupdate;
	const char		*bsl_entry_seq;
	int			bsl_gpio_used;
	int			bsl_gpio_rts;
	int			bsl_gpio_dtr;
	uint8_t                 bsl_entry_password[32];
};

struct device_class {
	const char		*name;
	const char		*help;

	/* Create a new device */
	device_t (*open)(const struct device_args *args);

	/* Close the connection to the device and destroy the driver object */
	void (*destroy)(device_t dev);

	/* Read/write memory */
	int (*readmem)(device_t dev, address_t addr,
		       uint8_t *mem, address_t len);
	int (*writemem)(device_t dev, address_t addr,
			const uint8_t *mem, address_t len);

	/* Erase memory */
	int (*erase)(device_t dev, device_erase_type_t type,
		     address_t address);

	/* Read/write registers */
	int (*getregs)(device_t dev, address_t *regs);
	int (*setregs)(device_t dev, const address_t *regs);

	/* CPU control */
	int (*ctl)(device_t dev, device_ctl_t op);

	/* Wait a little while for the CPU to change state */
	device_status_t (*poll)(device_t dev);

	/* Get the configuration fuse values */
	int (*getconfigfuses)(device_t dev);
};

struct device {
	const struct device_class	*type;

	int max_breakpoints;
	struct device_breakpoint breakpoints[DEVICE_MAX_BREAKPOINTS];

#if 0
	/* Power sample buffer, if power profiling is supported by this
	 * device.
	 */
	powerbuf_t power_buf;
#endif

	/* Chip information data.
	 */
	const struct chipinfo *chip;
	int need_probe;
};


/* Determine, from the device ID bytes, whether this chip is an FRAM or
 * flash-based device.
 */
int device_is_fram(device_t dev);

extern device_t device_default;

ALWAYS_INLINE static void device_destroy()
{
	if (device_default != NULL)
		device_default->type->destroy(device_default);
}

ALWAYS_INLINE static int device_getregs(address_t* regs)
{
	if (device_default == NULL)
		return -1;
	return device_default->type->getregs(device_default, regs);
}


address_t check_range(const struct chipinfo *chip,
			     address_t addr, address_t size,
			     const struct chipinfo_memory **m_ret);

int readmem(device_t dev, address_t addr,
		uint8_t *mem, address_t len,
		int (*read_words)(device_t dev,
			const struct chipinfo_memory *m,
			address_t addr, address_t len,
			uint8_t *data)
		);

int writemem(device_t dev, address_t addr,
		const uint8_t *mem, address_t len,
		int (*write_words)(device_t dev,
			const struct chipinfo_memory *m,
			address_t addr, address_t len,
			const uint8_t *data),
		int (*read_words)(device_t dev,
			const struct chipinfo_memory *m,
			address_t addr, address_t len,
			uint8_t *data)
		);
#endif
