#pragma once

#include "eem_defs.h"
#include "util/util.h"
#include "TapPlayer.h"
#include "util/Breakpoints.h"


#define DEVICE_NUM_REGS		16

#define SAFE_PC_ADDRESS (0x00000004ul)

static constexpr uint16_t kTriggerBlockSize = (kTb1 - kTb0);


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
	//! Physical transport to the target TAP. Chosen at runtime (replaces the
	//! old compile-time OPT_HARD_SELECT_SBW_TMP mux); the GDB monitor
	//! jtag_scan / sbw_scan commands set this before Open().
	enum class Transport : uint8_t
	{
		kJtag,	//!< 4-wire JTAG (JtagDev)
		kSbw,	//!< 2-wire Spy-Bi-Wire (SbwDev) — only if OPT_INCLUDE_SBW_TIM_
	};

	bool Open();
	void Close();

	//! Select the transport used by the next Open(). Returns false if the
	//! requested transport is not compiled into this build (e.g. SBW when
	//! OPT_INCLUDE_SBW_TIM_ is off). Does not itself (re)open the target.
	bool SetTransport(Transport t);
	Transport GetTransport() const { return transport_; }

	//! Bus-speed grade applied at the next OnConnectJtag (i.e. the next Open() /
	//! scan). The interface has no live re-speed, so a change takes effect on the
	//! next connect. Set by the GDB monitor "speed" command.
	void SetBusSpeed(BusSpeed s) { speed_ = s; }
	BusSpeed GetBusSpeed() const { return speed_; }

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
	// Legacy name: JTAG ID 0x98 uses the FR2xx/FR4xx low-density Xv2 system register map
	ALWAYS_INLINE bool IsFr41xx() const { return (core_id_.jtag_id_ == kMsp_98); }

	ALWAYS_INLINE bool HasIssue1377() const { return chip_info_.issue_1377_; }

	ALWAYS_INLINE bool ExecutePOR() { return traits_->ExecutePOR(); }

	bool ReadMem(address_t addr, void *mem, address_t len);

	bool WriteMem(address_t addr, const void *mem, address_t len);

	bool EraseMain();
	bool EraseAll();
	bool EraseInfoA();
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

	/*!
	Format the resolved chip profile to an output stream.

	Templated on the stream type so the same formatter serves both the SWO
	\c Trace channel (ShowDeviceType) and the GDB \c MonitorStream (qRcmd
	\c chipinfo / \c jtag_scan / \c sbw_scan replies) — both are \c OutStream<>
	instantiations. Call only after a successful identification (chip_info_ loaded).

	\param os    destination stream
	\param full  false: one-line "Device:" summary + breakpoint count.
	             true:  also dump the memory map (the GDB memory-map substitute,
	                    since GDB/MSP430 carries no memory-map XML).
	*/
	template <typename S>
	void PrintChipInfo(S &os, bool full) const
	{
		os << "Device: " << chip_info_.name_;
		if (chip_info_.arch_ == ChipInfoDB::kCpuXv2)
			os << " [CPUXv2]";
		else if (chip_info_.arch_ == ChipInfoDB::kCpuX)
			os << " [CPUX]";
		if (chip_info_.is_fram_)
			os << " [FRAM]";
		os << " [EMEX_" << EemName(chip_info_.eem_type_)
			<< "] [" << SlauName(chip_info_.slau_) << "]";
		if (chip_info_.issue_1377_)
			os << " [1377]";
		if (chip_info_.quick_mem_read_)
			os << " [QUICK]";
		os << '\n';
		if (full)
		{
			for (int i = 0; i < ChipInfoDB::kMaxMemConfigs; ++i)
			{
				const MemInfo &mem = chip_info_.mem_[i];
				if (mem.valid_ == false)
					break;
				// Left-align the human-readable size
				StringBuf<18> tmp;
				tmp << f::K(mem.size_);
				os << '\t' << f::S<12>(MemClassName(mem.class_)) << " 0x" << f::X<4>(mem.start_)
					<< "-0x" << f::X<4>(mem.start_ + mem.size_ - 1)
					<< '\t' << f::S<-8>(tmp)
					<< " [" << MemTypeName(mem.type_);
				if (mem.banks_ > 1)
					os << " - " << mem.banks_ << " banks";
				os << "]\n";
			}
		}
		os << "Hardware breakpoints: " << chip_info_.num_breakpoints_ << '\n';
	}

protected:
	// Enum-to-name lookups for PrintChipInfo. Defined in TapMcu.cpp where the
	// tables live as the single source of truth (with matching static_asserts).
	static const char *MemClassName(ChipInfoDB::EnumMemoryKey v);
	static const char *MemTypeName(ChipInfoDB::EnumMemoryType v);
	static const char *SlauName(ChipInfoDB::EnumSlau v);
	static const char *EemName(ChipInfoDB::EnumEemType v);

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
	void OnReadBytes(address_t addr, void *data, address_t byte_count);
	void OnReadWords(address_t addr, void *data, address_t word_count);
	void OnWriteWords(const MemInfo *m, address_t addr, const void *data, int wordcount);
	int OnSoftReset();
	int OnRun();
	int OnSingleStep();
	bool OnHalt();
	device_status_t OnPoll();
	bool StartMcu();
	//!
	void ClearError() { failed_ = false; }
	ALWAYS_INLINE bool EraseFlash(address_t erase_address, const FlashEraseFlags erase_mode, EraseMode mass_erase = kSimpleErase)
	{
		return traits_->EraseFlash(erase_address, erase_mode, mass_erase);
	}
	//!
	bool GetCpuState();

	//! Point g_Player.itf_ at the driver for the current transport_. Returns
	//! false if that transport is not compiled in. Called by Open().
	bool SelectActiveDriver();

protected:
	bool attached_;
	// Device information loaded from device database
	ChipProfile chip_info_;

	ITapDev *traits_;
	CoreId core_id_;
	CpuContext cpu_ctx_;
	// Active transport for the next Open(). Default seeded from the legacy
	// compile-time lever so existing per-target behavior is preserved until a
	// monitor command picks otherwise.
#if OPT_HARD_SELECT_SBW_TMP
	Transport transport_ = Transport::kSbw;
#else
	Transport transport_ = Transport::kJtag;
#endif
	// Desired bus-speed grade for the next connect (see SetBusSpeed). Defaults to
	// medium: a sane starting point that the operator can raise or lower by
	// re-running jtag_scan/sbw_scan with a speed argument to probe link stability.
	BusSpeed speed_ = BusSpeed::kMedium;
};



extern TapMcu g_TapMcu;
