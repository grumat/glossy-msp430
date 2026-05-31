#pragma once

#include "util/ChipProfile.h"
#include "util/Properties.h"
#include "util/JtagPending.h"
#include "ITapDev.h"


// Bits of the device Status register (R2)
#define STATUS_REG_C            0x0001
#define STATUS_REG_Z            0x0002
#define STATUS_REG_N            0x0004
#define STATUS_REG_GIE          0x0008
#define STATUS_REG_CPUOFF       0x0010
#define STATUS_REG_OSCOFF       0x0020
#define STATUS_REG_SCG0         0x0040
#define STATUS_REG_SCG1         0x0080
#define STATUS_REG_V            0x0100

// JTAG instruction-register opcodes, in reverse bit order (as shifted on the
// wire). Value comments give the canonical SLAU forward code, then the
// reversed byte that is actually clocked out.
enum class Ir : uint8_t
{
	// Controlling the Memory Address Bus (MAB)
	kAddrHighByte		= 0x81,	// 0x81: 10000001 -> 10000001: 0x81
	kAddrLowByte		= 0x41,	// 0x82: 10000010 -> 01000001: 0x41
	kAddr16Bit			= 0xC1,	// 0x83: 10000011 -> 11000001: 0xC1
	kAddrCapture		= 0x21,	// 0x84: 10000100 -> 00100001: 0x21
	kCaptureCpuReg		= 0x61,	// 0x86: 10000110 -> 01100001: 0x61
	kDeviceId			= 0xE1,	// 0x87: 10000111 -> 11100001: 0xE1
	// Controlling the Memory Data Bus (MDB)
	kDataToAddr			= 0xA1,	// 0x85: 10000101 -> 10100001: 0xA1
	kData16Bit			= 0x82,	// 0x41: 01000001 -> 10000010: 0x82
	kDataCapture		= 0x42,	// 0x42: 01000010 -> 01000010: 0x42
	kDataQuick			= 0xC2,	// 0x43: 01000011 -> 11000010: 0xC2
	// Controlling the CPU
	kCntrlSig16Bit		= 0xC8,	// 0x13: 00010011 -> 11001000: 0xC8
	kCntrlSigCapture	= 0x28,	// 0x14: 00010100 -> 00101000: 0x28
	kCntrlSigRelease	= 0xA8,	// 0x15: 00010101 -> 10101000: 0xA8
	kCntrlSigHighByte	= 0x88,	// 0x11: 00010001 -> 10001000: 0x88
	kCntrlSigLowByte	= 0x48,	// 0x12: 00010010 -> 01001000: 0x48
	kCoreIpId			= 0xE8,	// 0x17: 00010111 -> 11101000: 0xE8
	kJStateId			= 0x46,	// 0x62: 01100010 -> 01000110: 0x46
	// Memory Verification by Pseudo Signature Analysis (PSA)
	kDataPsa			= 0x22,	// 0x44: 01000100 -> 00100010: 0x22
	kData16BitOpt		= 0xA2,
	kShiftOutPsa		= 0x62,	// 0x46: 01000110 -> 01100010: 0x62
	kDta				= 0xE2,
	// JTAG Access Security Fuse Programming
	kPrepareBlow		= 0x44,	// 0x22: 00100010 -> 01000100: 0x44
	kExBlow				= 0x24,	// 0x24: 00100100 -> 00100100: 0x24
	// Configuration Fuse
	kConfigFuses		= 0x94,	// 0x29: 00101001 -> 10010100: 0x94
	// Bypass instruction
	kBypass				= 0xFF,
	// EEM / EMEX
	kEmexDataExchange	= 0x90,
	kEmexReadTrigger	= 0x50,
	kEmexReadControl	= 0xD0,
	kEmexWriteControl	= 0x30,
	kEmexDataExchange32	= 0xB0,
	// FLASH register
	kFlash16BitUpdate	= 0x98,
	kFlashCapture		= 0x58,
	kFlash16BitIn		= 0xD8,
	kFlashUpdate		= 0x38,
	// Test registers
	kTestReg			= 0x54,	// 32-bit JTAG test register
	kTest3VReg			= 0xF4,	// 16-bit 3 V test register
	// JTAG mailbox
	kJmbExchange		= 0x86,
	kJmbWrite32BitMode	= 0x11,
};

// Breakpoint block
// EMEX address = BP number * Block size + Register offset + R/W offset
// Note: In Volker's new EEM documentation, this block is called the Memory Bus Trigger
#define MX_BP					0x0000	// Breakpoint value offset
#define MX_CNTRL				0x0002	// Control offset
#define MX_MASK					0x0004	// Mask offset
#define MX_COMB					0x0006	// Combination offset
// Registers in the EMEX logic
#define MX_WRITE				0		// Write offset
#define MX_READ					1		// Read offset

// Control block
#define MX_EEMVER				0x0087
#define MX_CPUSTOP				0x0080
#define MX_GENCNTRL				0x0082
#define MX_GCLKCTRL				0x0088
#define MX_MCLKCNTL0			0x008A
#define MX_TRIGFLAG				0x008E

// Settings of the Breakpoint block Control register
#define BPCNTRL_MAB				0x0000
#define BPCNTRL_MDB				0x0001
#define BPCNTRL_RW_DISABLE		0x0000
#define BPCNTRL_RW_ENABLE		0x0020
#define BPCNTRL_EQ				0x0000
#define BPCNTRL_GE				0x0008
#define BPCNTRL_LE				0x0010
#define BPCNTRL_FREE			0x0018
#define BPCNTRL_DMA_DISABLE		0x0000
#define BPCNTRL_DMA_ENABLE		0x0040
// With BPCNTRL_DMA_DISABLE and BPCNTRL_RW_DISABLE
#define BPCNTRL_IF				0x0000
#define BPCNTRL_IFHOLD			0x0002
#define BPCNTRL_NIF				0x0004
#define BPCNTRL_BOTH			0x0006
// Settings of the Breakpoint block Mask register
#define BPMASK_WORD				0x0000
#define BPMASK_HIGHBYTE			0x00FF
#define BPMASK_LOWBYTE			0xFF00
#define BPMASK_DONTCARE			0xFFFFFFFF


// Bits of the control signal register


// JTAG Control Signal Register for 1xx, 2xx, 4xx Families
enum class CtrlSigReg : uint16_t
{
	kNone			= 0,
	kRead			= 0b0000000000000001,	// R/W: Controls the read/write (RW) signal of the CPU. 1 = Read/0 = Write
	kHalt			= 0b0000000000000010,	// HALT (Xv2):
	kIntrReq		= 0b0000000000000100,	// INTR_REQ: Interrupt request detected
	kCpuHalt		= 0b0000000000001000,	// HALT_JTAG: Sets the CPU into a controlled halt state. 1 = CPU stopped/0 = CPU operating normally
	kByte			= 0b0000000000010000,	// BYTE: Controls the BYTE signal of the CPU used for memory access data length. 1 = Byte (8-bit) access/0 = Word (16-bit) access
	kCpuOff			= 0b0000000000100000,	// CPUOFF:
	kInstrLoad		= 0b0000000010000000,	// INSTR_LOAD: Read only: Indicates the target CPU instruction state. 1 = Instruction fetch state/0 = Instruction execution state
	kCpuSusp		= 0b0000000100000000,	// CPUSUSP (Xv2): Suspend CPU
	kTce			= 0b0000001000000000,	// TCE (Test Clock Enable): Indicates CPU synchronization. 1 = Synchronized/0 = Not synchronized
	kTce1			= 0b0000010000000000,	// TCE1: Establishes JTAG control over the CPU. 1 = CPU under JTAG control/0 = CPU free running
	kPOR			= 0b0000100000000000,	// POR: Controls the power-on-reset (POR) signal. 1 = Perform POR/0 = No reset
	kCpu			= 0b0001000000000000,	// Release low byte: Selects control source of the RW and BYTE bits. 1 = CPU has control/0 = Control signal register has control
	kTagFuncSat		= 0b0010000000000000,	// TAGFUNCSAT: Sets flash module into JTAG access mode. 1 = CPU has control default)/0 = JTAG has control
	kSwitch			= 0b0100000000000000,	// SWITCH: Enables TDO output as TDI input. 1 = JTAG has control/0 = Normal operation
	kStopSel		= 0b1000000000000000,	// STOP_SEL
};
static ALWAYS_INLINE constexpr CtrlSigReg operator|(CtrlSigReg lhs, CtrlSigReg rhs)
{
	return static_cast<CtrlSigReg>(E2I(lhs) | E2I(rhs));
}
static ALWAYS_INLINE constexpr CtrlSigReg operator&(CtrlSigReg lhs, CtrlSigReg rhs)
{
	return static_cast<CtrlSigReg>(E2I(lhs) & E2I(rhs));
}


// Bits of the FLASH register
#define FLASH_SESEL1			0x0080
#define FLASH_TMR				0x0800

// ROM address (for use in work-around to RAM-corrupted-during-JTAG-access bug).
#define ROM_ADDR				0x0c04

#define ETKEY					0x9600
#define ETKEYSEL				0x0110
#define ETCLKSEL				0x011E

#define	EEM_STOPPED				0x0080


enum TapCmd : uint32_t
{
	cmdIrShift				// OnIrShift
	, cmdIrShift8			// OnIrShift / OnDrShift8 (arg)
	, cmdIrShift16			// OnIrShift / OnDrShift16 (arg)
	, cmdIrShift20			// OnIrShift / OnDrShift20 (arg) [for arg <= 0xFFFF]
	, cmdIrShift32			// OnIrShift / OnDrShift32 (arg) [for arg <= 0xFFFF]
	, cmdIrData16			// kIr(Ir::kData16Bit) + clk0 + OnDrShift16(arg) + clk1
	, cmdDrShift8			// OnDrShift8 (arg)
	, cmdDrShift16			// OnDrShift16 (arg)
	, cmdDrShift20			// OnDrShift20 (arg)
	, cmdDrShift32			// OnDrShift32 (arg)
	, cmdClrTclk			// OnClearTclk
	, cmdSetTclk			// OnSetTclk
	, cmdPulseTclk			// OnPulseTclk
	, cmdPulseTclkN			// OnPulseTclkN
	, cmdStrobeN			// OnFlashTclk (arg)
	, cmdReleaseCpu			// ReleaseCpu()
	, cmdDelay1ms			// Delay arg ms
	//////// argv commands /////////
	, cmdIrShift_argv_p		// OnIrShift (return to argv ptr)
	, cmdIrShift16_argv		// OnIrShift / OnDrShift16 (argv)
	, cmdDrShift16_argv		// OnDrShift16 (argv)
	, cmdDrShift16_argv_i	// OnDrShift16 (argv, indirect set)
	, cmdDrShift16_argv_p	// OnDrShift16 (return to argv ptr)
	, cmdIrShift20_argv		// OnIrShift / OnDrShift20 (argv)
	, cmdDrShift20_argv		// OnDrShift20 (argv)
	, cmdDrShift20_argv_p	// OnDrShift20 (return to argv ptr)
	, cmdIrShift32_argv		// OnIrShift / OnDrShift20 (argv)
	, cmdDrShift32_argv		// OnDrShift32 (argv)
	, cmdDrShift32_argv_p	// OnDrShift32 (return to argv ptr)
	, cmdIrData16_argv		// kIr(Ir::kData16Bit) + clk0 + OnDrShift16(argv) + clk1
	, cmdStrobe_argv		// OnFlashTclk (argv)
};


enum DataClk : uint32_t
{
	kdTclk0			// set TCLK to 0
	, kdTclk1		// set TCLK to 1
	, kdTclkP		// TCLK pulse
	, kdTclkN		// negative TCLK pulse
	, kdTclk2P		// double TCLK pulse
	, kdTclk2N		// double negative TCLK pulse
	, kdNone = 0x7
};

struct TapStep2
{
	TapCmd cmd : 8;
	uint32_t arg : 24;
};
struct TapStep3
{
	TapCmd cmd : 8;
	uint32_t arg8 : 8;
	uint32_t arg16 : 16;
};
struct TapStep4
{
	TapCmd cmd : 8;
	uint32_t arg4a : 4;
	uint32_t arg4b : 4;
	uint32_t arg16 : 16;
};
union TapStep
{
	TapStep2 s2;
	TapStep3 s3;
	TapStep4 s4;
};


/// Bus speed grade selected by the host. The actual bit-rate depends on the
/// hardware backend; the comments below reflect the typical 72 MHz STM32F1
/// SPI-based JTAG implementation.
enum class BusSpeed : uint8_t
{
	kSlowest,	///< ~562.5 kbps — safe default, longest cables / slowest targets
	kSlow,		///< ~1.125 Mbps
	kMedium,	///< ~2.25 Mbps
	kFast,		///< ~4.5 Mbps
	kFastest,	///< ~9 Mbps — backends may need clock-anticipation at this rate
};


/// Hardware-agnostic JTAG/SBW transport interface used by the TAP layer.
///
/// All MSP430-protocol logic (TapDev430 / TapDev430X / TapDev430Xv2) runs on
/// top of this interface; concrete drivers (JtagDev variants) provide the
/// actual bit-bang or DMA implementation. Implementations are singletons and
/// own the JTAG pins for the lifetime of the firmware.
class ITapInterface
{
	// VIRTUAL DESTRUCTOR IS NOT NECESSARY:
	// Instance of this objetc is **static** and will never be destroyed
	// since there is no "exit program" operation in a firmware.
	// This spares 2K of Flash + some more RAM

public:
	/// One-shot bring-up of the underlying peripherals (timers, DMA, SPI, GPIO).
	/// Called once at firmware start before any JTAG traffic. Returns false on
	/// hardware-init failure.
	virtual bool OnOpen() = 0;
	/// Shut the transport down: release DMA, tri-state pins, cut bus drivers.
	/// After this call the JTAG bus is electrically inert.
	virtual void OnClose() = 0;
	/// Acquire the JTAG bus and configure the selected bus speed. Sets up pin
	/// modes / SPI clock / TMS shaper for the given grade. Must be called
	/// before OnEnterTap().
	virtual void OnConnectJtag(BusSpeed speed) = 0;
	/// Release the JTAG bus (TEST low, RST low, pins tri-stated) so the target
	/// can run free. Mirror of OnConnectJtag.
	virtual void OnReleaseJtag() = 0;

public:
	/// Drive the slau320aj fuse-check / TAP entry sequence on RST and TEST.
	/// Leaves the target in the reset state with the TAP controller energized.
	virtual void OnEnterTap() = 0;
	/// Pulse TMS high long enough to force the TAP state machine to
	/// Test-Logic-Reset, then return to Run-Test/Idle. Required after
	/// OnEnterTap() and after any protocol error.
	virtual void OnResetTap() = 0;

public:
	/// Shift one IR opcode (`Ir`); kicks off the DMA and returns a `JtagPending`
	/// handle. The previous frame's DMA completes before this one starts;
	/// the returned handle resolves to the captured IR value (TDO out) on
	/// implicit conversion or `.Get()`. Leaves the TAP in Run-Test/Idle.
	virtual JtagPending<uint8_t>  OnIrShift(Ir instr) = 0;
	/// Shift 8 data bits through DR; returns a Pending for the previous
	/// DR contents. See `OnIrShift` for the async lifecycle.
	virtual JtagPending<uint8_t>  OnDrShift8(uint8_t) = 0;
	/// Shift 16 data bits through DR; returns a Pending for the previous
	/// DR contents. See `OnIrShift` for the async lifecycle.
	virtual JtagPending<uint16_t> OnDrShift16(uint16_t) = 0;
	/// Shift 20 data bits through DR (CPUX address bus); returns a Pending
	/// for the previous DR contents in MSP430 word/byte-swapped layout.
	virtual JtagPending<uint32_t> OnDrShift20(uint32_t) = 0;
	/// Shift 32 data bits through DR; returns a Pending for the previous
	/// DR contents.
	virtual JtagPending<uint32_t> OnDrShift32(uint32_t) = 0;
	/// Wait until the CPU reports "instruction-load" via the control-signal
	/// register. Returns false on timeout (target not responding).
	virtual bool OnInstrLoad() = 0;

	//virtual void OnClockThroughPsa() = 0;

	/// Drive TCLK high (for protocol sequences that hold TCLK at a known level).
	virtual void OnSetTclk() = 0;
	/// Drive TCLK low.
	virtual void OnClearTclk() = 0;
	/// Emit one TCLK pulse (rising-then-falling). The hot-path helper for
	/// shift loops; backends try to make this as cheap as possible.
	virtual void OnPulseTclk() = 0;
	/// Emit one TCLK pulse with the inverted polarity (falling-then-rising),
	/// used by a handful of slau320aj sequences that begin on a low edge.
	virtual void OnPulseTclkN() = 0;
	/// Generate at least `min_pulses` TCLK pulses at the ~470 kHz "flash
	/// strobe" rate required during flash erase/write on Gen1/Gen2 MSP430s.
	virtual void OnFlashTclk(uint32_t min_pulses) = 0;
	/// Apply one of the encoded TCLK shapes (kdTclk0 / kdTclk1 / kdTclkP /
	/// kdTclkN / kdTclk2P / kdTclk2N) used by the TapStep stream.
	virtual void OnTclk(DataClk tclk) = 0;
	/// Composite helper: TCLK shape `clk0`, then a 16-bit DR shift, then TCLK
	/// shape `clk1`. Returns the previous DR contents. Used by the data-bus
	/// access primitives in TapDev430*.
	virtual uint16_t OnData16(DataClk clk0, uint16_t data, DataClk clk1) = 0;

	/// Poll the JTAG mailbox out-register; returns 0 if no data is pending.
	/// Used for target-to-host messaging.
	virtual uint32_t OnReadJmbOut() = 0;
	/// Push a 16-bit word into the JTAG mailbox in-register. Returns false on
	/// timeout (target did not drain the previous word).
	virtual bool OnWriteJmbIn16(uint16_t data) = 0;
};



// A sequence of kIr(ir) + clk2
ALWAYS_INLINE static constexpr TapStep kIr(const Ir ir, const DataClk clk2 = kdNone)
{
	return { .s2 = { .cmd = cmdIrShift, .arg = (uint32_t)(E2I(ir) << 8) | (clk2 << 4) | kdNone } };
}
// A sequence of clk1 + kIr(ir) + clk2
ALWAYS_INLINE static constexpr TapStep kIr(const DataClk clk1, const Ir ir, const DataClk clk2 = kdNone)
{
	return { .s2 = { .cmd = cmdIrShift, .arg = (uint32_t)(E2I(ir) << 8) | (clk2 << 4) | clk1 } };
}
ALWAYS_INLINE static constexpr TapStep kIrRet(const Ir ir)
{
	return { .s2 = { .cmd = cmdIrShift_argv_p, .arg = E2I(ir) } };
}
ALWAYS_INLINE static constexpr TapStep kIrDr8(const Ir ir, const uint8_t d)
{
	return { .s2 = { .cmd = cmdIrShift8, .arg = (uint32_t)(d << 8) | E2I(ir) } };
}
ALWAYS_INLINE static constexpr TapStep kIrDr16(const Ir ir, const uint16_t d)
{
	return { .s2 = { .cmd = cmdIrShift16, .arg = (uint32_t)(d << 8) | E2I(ir) } };
}
ALWAYS_INLINE static constexpr TapStep kIrDr20(const Ir ir, const uint16_t d)
{
	return { .s2 = { .cmd = cmdIrShift20, .arg = (uint32_t)(d << 8) | E2I(ir) } };
}
ALWAYS_INLINE static constexpr TapStep kIrDr32(const Ir ir, const uint16_t d)
{
	return { .s2 = { .cmd = cmdIrShift32, .arg = (uint32_t)(d << 8) | E2I(ir) } };
}
// A sequence of kIr(Ir::kData16Bit) + clk1 + kDr16(d) + clk2
ALWAYS_INLINE static constexpr TapStep kIrData16(const DataClk clk1, const uint16_t d, const DataClk clk2 = kdNone)
{
	return { .s2 = { .cmd = cmdIrData16, .arg = (uint32_t)(d << 8) | clk1 | (clk2 << 4) } };
}
// A sequence of kIr(ir) + kDr16(argv) + clk2
ALWAYS_INLINE static constexpr TapStep kIrDr16Argv(const Ir ir, const DataClk clk2 = kdNone)
{
	return { .s2 = { .cmd = cmdIrShift16_argv, .arg = ((uint32_t)E2I(ir) << 8) | (clk2 << 4) | kdNone } };
}
// A sequence of clk1 + kIr(ir) + kDr16(argv) + clk2
ALWAYS_INLINE static constexpr TapStep kIrDr16Argv(const DataClk clk1, const Ir ir, const DataClk clk2 = kdNone)
{
	return { .s2 = { .cmd = cmdIrShift16_argv, .arg = ((uint32_t)E2I(ir) << 8) | (clk2 << 4) | clk1 } };
}
ALWAYS_INLINE static constexpr TapStep kIrDr20Argv(const Ir ir)
{
	return { .s2 = { .cmd = cmdIrShift20_argv, .arg = E2I(ir) } };
}
ALWAYS_INLINE static constexpr TapStep kIrDr32Argv(const Ir ir)
{
	return { .s2 = { .cmd = cmdIrShift32_argv, .arg = E2I(ir) } };
}
// Same as DR_Shift16(d)
ALWAYS_INLINE static constexpr TapStep kDr16(const uint16_t d)
{
	return { .s2 = { .cmd = cmdDrShift16, .arg = d } };
}
// Same as *(uint16_t*)argv = DR_Shift16(d)
ALWAYS_INLINE static constexpr TapStep kDr16_ret(const uint16_t d)
{
	return { .s2 = { .cmd = cmdDrShift16_argv_p, .arg = d } };
}
// Same as DR_Shift16(argv)
static constexpr TapStep kDr16Argv = { .s2 = { cmdDrShift16_argv } };
// Same as DR_Shift16(*(uint16_t *)argv)
static constexpr TapStep kDr16ArgvI = { .s2 = { cmdDrShift16_argv_i } };
ALWAYS_INLINE static constexpr TapStep kDr20(const uint32_t d)
{
	return { .s2 = { .cmd = cmdDrShift20, .arg = d } };
}
static constexpr TapStep kDr20Argv = { .s2 = { cmdDrShift20_argv } };
ALWAYS_INLINE static constexpr TapStep kDr20_ret(const uint32_t d)
{
	return { .s2 = { .cmd = cmdDrShift20_argv_p, .arg = d } };
}
ALWAYS_INLINE static constexpr TapStep kDr32(const uint32_t d)
{
	return { .s2 = { .cmd = cmdDrShift32, .arg = d } };
}
static constexpr TapStep kDr32Argv = { .s2 = { cmdDrShift32_argv } };
ALWAYS_INLINE static constexpr TapStep kDr32_ret(const uint32_t d)
{
	return { .s2 = { .cmd = cmdDrShift32_argv_p, .arg = d } };
}
ALWAYS_INLINE static constexpr TapStep kIrData16Argv(const DataClk clk1, const DataClk clk2 = kdNone)
{
	return { .s2 = { .cmd = cmdIrData16_argv, .arg = (uint32_t)(clk1) | ((uint32_t)clk2 << 4) } };
}
static constexpr TapStep kTclk0 = { .s2 = { cmdClrTclk } };
static constexpr TapStep kTclk1 = { .s2 = { cmdSetTclk } };
static constexpr TapStep kPulseTclk = { .s2 = { cmdPulseTclk } };
static constexpr TapStep kPulseTclkN = { .s2 = { cmdPulseTclkN } };
ALWAYS_INLINE static constexpr TapStep kStrobeTclk(const uint32_t n)
{
	return { .s2 = { .cmd = cmdStrobeN, .arg = n } };
}
static constexpr TapStep kStrobeTclkArgv = { .s2 = { cmdStrobe_argv } };
static constexpr TapStep kReleaseCpu = { .s2 = { cmdReleaseCpu } };
ALWAYS_INLINE static constexpr TapStep kDelay1ms(const uint32_t d)
{
	return { .s2 = { .cmd = cmdDelay1ms, .arg = (uint32_t)d } };
}



//! Plays sequences of Jtag primitives
class TapPlayer
{
public:
	void ReleaseCpu();
	//! Plays a Jtag primitive
	uint32_t Play(const TapStep cmd) OPTIMIZED;
	//! Plays a single write-only Jtag primitive WITHOUT waiting for the result.
	//! Fire-and-forget: the shift's DMA is left in flight (the returned previous
	//! DR contents are discarded), so the caller's next shift overlaps its render
	//! with this frame's DMA — the next OnXxxShift() drains it. Use for control /
	//! EMEX / data writes whose result is don't-care, instead of `Play(cmd)` which
	//! blocks on the JtagPending->uint32_t conversion even when nobody reads it.
	//! Shift steps only; non-shift and value-returning (cmdIrData16) steps fall
	//! back to the blocking Play(cmd).
	void PlayAsync(const TapStep cmd) OPTIMIZED;
	//! Plays sequences of Jtag primitives
	void Play(const TapStep cmds[], const uint32_t count, ...) NO_INLINE OPTIMIZED;
	JtagId SetJtagRunReadLegacy();
	// Returns JTAG control signal register
	ALWAYS_INLINE uint16_t GetCtrlSigReg() { return Play(kIrDr16(Ir::kCntrlSigCapture, 0)); }

public:
	ITapInterface *itf_;

// SLAU320AJ compatibility
public:
	ALWAYS_INLINE JtagPending<uint8_t> IR_Shift(Ir ir) { return itf_->OnIrShift(ir); }
	ALWAYS_INLINE void ClrTCLK() { itf_->OnClearTclk(); }
	ALWAYS_INLINE void SetTCLK() { itf_->OnSetTclk(); }
	ALWAYS_INLINE void PulseTCLK() { itf_->OnPulseTclk(); }
	ALWAYS_INLINE void PulseTCLKN() { itf_->OnPulseTclkN(); }
	ALWAYS_INLINE JtagPending<uint8_t> DR_Shift8(uint8_t n) { return itf_->OnDrShift8(n); }
	ALWAYS_INLINE JtagPending<uint16_t> DR_Shift16(uint16_t n) { return itf_->OnDrShift16(n); }
	ALWAYS_INLINE JtagPending<uint32_t> DR_Shift20(uint32_t n) { return itf_->OnDrShift20(n); }
	ALWAYS_INLINE JtagPending<uint32_t> DR_Shift32(uint32_t n) { return itf_->OnDrShift32(n); }
	ALWAYS_INLINE void TCLKstrobes(uint32_t n) { itf_->OnFlashTclk(n); }
	ALWAYS_INLINE uint32_t i_ReadJmbOut() { return itf_->OnReadJmbOut(); }
	ALWAYS_INLINE bool i_WriteJmbIn16(uint16_t data) { return itf_->OnWriteJmbIn16(data); }

// MSP-FET-UIF code portability
public:
	ALWAYS_INLINE void addr_capture() { itf_->OnIrShift(Ir::kAddrCapture); }
	ALWAYS_INLINE void addr_16bit() { itf_->OnIrShift(Ir::kAddr16Bit); }
	ALWAYS_INLINE void cntrl_sig_16bit() { itf_->OnIrShift(Ir::kCntrlSig16Bit); }
	ALWAYS_INLINE JtagId cntrl_sig_capture() { return (JtagId)(uint8_t)(itf_->OnIrShift(Ir::kCntrlSigCapture)); }
	ALWAYS_INLINE void cntrl_sig_low_byte() { itf_->OnIrShift(Ir::kCntrlSigLowByte); }
	ALWAYS_INLINE void cntrl_sig_high_byte() { itf_->OnIrShift(Ir::kCntrlSigHighByte); }
	ALWAYS_INLINE JtagPending<uint8_t> core_ip_pointer() { return itf_->OnIrShift(Ir::kCoreIpId); }
	ALWAYS_INLINE JtagPending<uint8_t> data_16bit() { return itf_->OnIrShift(Ir::kData16Bit); }
	ALWAYS_INLINE JtagPending<uint8_t> data_capture() { return itf_->OnIrShift(Ir::kDataCapture); }
	ALWAYS_INLINE JtagPending<uint8_t> data_quick() { return itf_->OnIrShift(Ir::kDataQuick); }
	ALWAYS_INLINE JtagPending<uint8_t> data_to_addr() { return itf_->OnIrShift(Ir::kDataToAddr); }
	ALWAYS_INLINE JtagPending<uint8_t> device_ip_pointer() { return itf_->OnIrShift(Ir::kDeviceId); }
	ALWAYS_INLINE void eem_read_control() { itf_->OnIrShift(Ir::kEmexReadControl); }
	ALWAYS_INLINE void eem_write_control() { itf_->OnIrShift(Ir::kEmexWriteControl); }
	ALWAYS_INLINE void eem_data_exchange() { itf_->OnIrShift(Ir::kEmexDataExchange); }
	ALWAYS_INLINE void eem_data_exchange32() { itf_->OnIrShift(Ir::kEmexDataExchange32); }
	ALWAYS_INLINE void test_reg_3V() { itf_->OnIrShift(Ir::kTest3VReg); }
	ALWAYS_INLINE void test_reg() { itf_->OnIrShift(Ir::kTestReg); }
	ALWAYS_INLINE void instrLoad() { itf_->OnInstrLoad(); }
	ALWAYS_INLINE void release_cpu() { ReleaseCpu(); }
	ALWAYS_INLINE uint8_t SetReg_8Bits(uint8_t n) { return itf_->OnDrShift8(n); }
	ALWAYS_INLINE uint16_t SetReg_16Bits(uint16_t n) { return itf_->OnDrShift16(n); }
	ALWAYS_INLINE uint32_t SetReg_20Bits(uint32_t n) { return itf_->OnDrShift20(n); }
	ALWAYS_INLINE uint32_t SetReg_32Bits(uint32_t n) { return itf_->OnDrShift32(n); }
	ALWAYS_INLINE void IHIL_Tclk(const bool b) { b ? itf_->OnSetTclk() : itf_->OnClearTclk(); }
	ALWAYS_INLINE void IHIL_TCLK() { itf_->OnPulseTclkN(); }

public:
	// static constexpr TapStep kSetWordRead_ = kIrDr16(Ir::kCntrlSig16Bit, 0x2409);
	// static constexpr TapStep kSetWordWrite_ = kIrDr16(Ir::kCntrlSig16Bit, 0x2408);
	//static constexpr TapStep kSetWordReadXv2_ = kIrDr16(Ir::kCntrlSig16Bit, 0x0501);
};


// The singleton object that plays JTAG sequences to the configured device generation
extern TapPlayer g_Player;
