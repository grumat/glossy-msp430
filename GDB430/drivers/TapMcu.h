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
#include "TapPlayer.h"
#include "util/Breakpoints.h"


#define DEVICE_NUM_REGS		16

#define SAFE_PC_ADDRESS (0x00000004ul)

static constexpr uint16_t kTriggerBlockSize = 8;

// Common values for FCTL1 register
static constexpr uint16_t kFctl1Lock = 0xA500;
static constexpr uint16_t kFctl1Lock_X = 0xA500;
static constexpr uint16_t kFctl1Lock_Xv2 = 0xA500;

// Common values for FCTL3 register
static constexpr uint16_t kFctl3Unlock = 0xA500;
static constexpr uint16_t kFctl3Lock = 0xA510;

static constexpr uint16_t kFctl3Unlock_X = 0xA500;
static constexpr uint16_t kFctl3Lock_X = 0xA510;

static constexpr uint16_t kFctl3Unlock_Xv2 = 0xA548;


//! Flash erasing modes (dword contents: FCTL3<<16 | FCTL1)
/*!
General mask rules (*):             SLAU049 SLAU056 SLAU144 SLAU208 SLAU259 SLAU335
	Flash Key		: 0xA500A500	   X       X       X       X       X       X
	ERASE bit (1)	: 0x00000002	   X       X       X       X       X       X
	MERAS bit (1)	: 0x00000004	   X       X       X       X       X       X
	GMERAS bit (1)	: 0x00000008	           X
	WRT bit (2)		: 0x00000040       X       X       X       X       X       X
	BLKWRT bit (2)	: 0x00000040       X       X       X       X       X       X
	LOCK bit		: 0x00100000       X       X       X       X       X       X
	LOCKA bit		: 0x00400000               X      (3)      X       X      (4)

	(*) SLAU321, SLAU367, SLAU378, SLAU445 and SLAU506 families has no Embedded Flash 
		Controller.
	(1)	The meaning of ERASE+MERAS+GMERAS combinations depends on family. On SLAU208
		and SLAU259 a Mass Erase does not affect Info memory.
	(2) The meaning of WRT+BLKWRT combinations depends on family.
	(3)	SLAU144 stores TLV data into the INFOA. So this is mostly protected from
		accidental erases. Only Segment Erase is allowed for this family.
	(4) This bit is called LOCKSEG on SLAU335 as the Info Memory is just a single 
		block. As with most parts this does not store the TLV (Factory calibration 
		values) and is should be erased by an Mass Erase command.
*/
enum EraseModeFctl : uint32_t
{
	// Erase individual segment only
	kSegmentEraseGeneral = 0xA500A502	// x0x0xxxx xxxx001x
	, kSegmentEraseSlau049 = 0xA500A502	// xxx0xxxx xxxxx01x
	, kSegmentEraseSlau056 = 0xA500A502	// x0x0xxxx xxxx001x
	, kSegmentEraseSlau144 = 0xA500A502	// x0x0xxxx xxxxx01x
	, kSegmentEraseSlau208 = 0xA500A502	// x0x0xxxx xxxxx01x
	, kSegmentEraseSlau259 = 0xA500A502	// x0x0xxxx xxxxx01x
	, kSegmentEraseSlau335 = 0xA500A502	// x0x0xxxx xxxxx01x
	// Erase main memory segments of all memory arrays.
	, kMainEraseSlau049 = 0xA500A504	// xxx0xxxx xxxxx10x
	, kMainEraseSlau056 = 0xA500A50C	// x0x0xxxx xxxx110x
	, kMainEraseSlau144 = 0xA540A504	// x1x0xxxx xxxxx10x
	, kMainEraseSlau208 = 0xA500A506	// x0x0xxxx xxxxx11x
	, kMainEraseSlau259 = 0xA500A506	// x0x0xxxx xxxxx11x
	, kMainEraseSlau335 = 0xA500A504	// x0x0xxxx xxxxx10x
	// Erase all main and information memory segments
	, kMassEraseSlau049 = 0xA500A506	// xxx0xxxx xxxxx11x
	, kMassEraseSlau056 = 0xA500A50E	// x0x0xxxx xxxx111x
	, kMassEraseSlau144 = 0xA540A506	// x1x0xxxx xxxxx11x
	, kMassEraseSlau208 = 0xA500A506	// x0x0xxxx xxxxx11x
	, kMassEraseSlau259 = 0xA500A506	// x0x0xxxx xxxxx11x
	, kMassEraseSlau335 = 0xA500A506	// x0x0xxxx xxxxx11x
};


 // dedicated addresses
 //! \brief Triggers a regular reset on device release from JTAG control
#define V_RESET					0xFFFE
//! \brief Triggers a "brown-out" reset on device release from JTAG control
#define V_BOR					0x1B08
//! \brief Triggers a regular reset on device release from JTAG control
#define V_RUNNING				0xFFFF

struct chipinfo_memory;

enum device_status_t
{
	DEVICE_STATUS_HALTED,
	DEVICE_STATUS_RUNNING,
	DEVICE_STATUS_INTR,
	DEVICE_STATUS_ERROR
};


//! MCU being tested
class TapMcu
{
public:
	enum
	{
		kMinFlashPeriod = 2
		, kMaxEntryTry = 4
	};

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

	ALWAYS_INLINE bool GetRegs(address_t *regs)
	{
		if (! attached_)
			return false;
		return OnGetRegs(regs);
	}

	ALWAYS_INLINE int SetRegs(address_t *regs)
	{
		if (!attached_)
			return -1;
		return OnSetRegs(regs);
	}

	ALWAYS_INLINE bool IsMSP430() const { return core_id_.IsMSP430(); }
	ALWAYS_INLINE bool IsXv2() const { return core_id_.IsXv2(); }
	// Checks i device is MSP430FR2xxx/MSP430FR41xx
	ALWAYS_INLINE bool IsFr41xx() const { return (core_id_.jtag_id_ == kMsp_98); }

	ALWAYS_INLINE bool HasIssue1377() const { return chip_info_.issue_1377_; }

	ALWAYS_INLINE bool ExecutePOR() { return traits_->ExecutePOR(); }

	bool ReadMem(address_t addr, void *mem, address_t len);

	bool WriteMem(address_t addr, const void *mem, address_t len);

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

public:
	bool failed_;

	Breakpoints breakpoints_;
	
	// GDB-like breakpoint management
	BkptId Set(address_t addr, DeviceBpType type, bool enabled, BkptId which = BkptId::kInvalidBkpt)
	{
		return breakpoints_.Set(chip_info_, addr, type, enabled, which);
	}
	// Clear breakpoint array
	void ClearBrk()
	{
		breakpoints_.Clear();
	}
	int GetMaxBreakpoints()
	{
		return breakpoints_.GetCount(chip_info_);
	}
	void UpdateEemBreakpoints()
	{
		traits_->UpdateEemBreakpoints(breakpoints_, chip_info_);
	}


protected:
	address_t CheckRange(address_t addr, address_t size, const MemInfo **ret);
	void ShowDeviceType();
	int device_is_fram();
	bool InitDevice();
	int refresh_bps();

	/*!
	Probe the device memory and extract ID bytes. This should be called after
	the device structure is ready.
	*/
	bool ProbeId();

// Methods here could be potentially promoted to overrides (kept normal calls for performance)
protected:
	bool IsFuseBlown();
	void OnClearState();
	bool OnGetRegs(address_t *regs);
	int OnSetRegs(address_t *regs);
	uint32_t OnGetReg(int reg);
	bool OnSetReg(int reg, uint32_t val);
	void OnReadWords(address_t addr, void *data, address_t len);
	void OnWriteWords(const MemInfo *m, address_t addr, const void *data, int wordcount);
	int OnSoftReset();
	int OnRun();
	int OnSingleStep();
	bool OnHalt();
	device_status_t OnPoll();
	//! Release the target device from JTAG control
	void ReleaseDevice(address_t address);
	bool StartMcu();
	//!
	void ClearError() { failed_ = false; }
	ALWAYS_INLINE bool EraseFlash(address_t erase_address, EraseModeFctl erase_mode, bool mass_erase)
	{
		return traits_->EraseFlash(erase_address, (uint16_t)erase_mode, (uint16_t)(erase_mode >> 16), mass_erase);
	}
	//!
	bool GetCpuState();

protected:
	bool attached_;
	// Device information loaded from device database
	ChipProfile chip_info_;

	ITapDev *traits_;
	CoreId core_id_;
	CpuContext cpu_ctx_;
};



extern TapMcu g_TapMcu;


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
	device_breakpoint breakpoints[DEVICE_MAX_BREAKPOINTS];

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
