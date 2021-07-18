#pragma once

#include "util/ChipProfile.h"

/* Instructions for the JTAG control signal register in reverse bit order
*/
/* Controlling the Memory Address Bus (MAB) */
#define IR_ADDR_HIGH_BYTE		0x81	/* 0x81: 10000001 -> 10000001: 0x81 */
#define IR_ADDR_LOW_BYTE		0x41	/* 0x82: 10000010 -> 01000001: 0x41 */
#define IR_ADDR_16BIT			0xC1	/* 0x83: 10000011 -> 11000001: 0xC1 */
#define IR_ADDR_CAPTURE			0x21	/* 0x84: 10000100 -> 00100001: 0x21 */
#define IR_CAPTURE_CPU_REG		0x61	/* 0x86: 10000110 -> 01100001: 0x61 */
#define IR_DEVICE_ID			0xE1	/* 0x87: 10000111 -> 11100001: 0xE1 */
/* Controlling the Memory Data Bus (MDB) */
#define IR_DATA_TO_ADDR			0xA1	/* 0x85: 10000101 -> 10100001: 0xA1 */
#define IR_DATA_16BIT			0x82	/* 0x41: 01000001 -> 10000010: 0x82 */
#define IR_DATA_CAPTURE			0x42	/* 0x42: 01000010 -> 01000010: 0x42 */
#define IR_DATA_QUICK			0xC2	/* 0x43: 01000011 -> 11000010: 0xC2 */
/* Controlling the CPU */
#define IR_CNTRL_SIG_16BIT		0xC8	/* 0x13: 00010011 -> 11001000: 0xC8 */
#define IR_CNTRL_SIG_CAPTURE	0x28	/* 0x14: 00010100 -> 00101000: 0x28 */
#define IR_CNTRL_SIG_RELEASE	0xA8	/* 0x15: 00010101 -> 10101000: 0xA8 */
#define IR_CNTRL_SIG_HIGH_BYTE	0x88	/* 0x11: 00010001 -> 10001000: 0x88 */
#define IR_CNTRL_SIG_LOW_BYTE	0x48	/* 0x12: 00010010 -> 01001000: 0x48 */
#define IR_COREIP_ID			0xE8	/* 0x17: 00010111 -> 11101000: 0xE8 */
#define IR_JSTATE_ID			0x46	/* 0x62: 01100010 -> 01000110: 0x46 */
/* Memory Verification by Pseudo Signature Analysis (PSA) */
#define IR_DATA_PSA				0x22	/* 0x44: 01000100 -> 00100010: 0x22 */
#define IR_DATA_16BIT_OPT		0xA2	// 45
#define IR_SHIFT_OUT_PSA		0x62	/* 0x46: 01000110 -> 01100010: 0x62 */
#define IR_DTA					0xE2	// 47
/* JTAG Access Security Fuse Programming */
#define IR_PREPARE_BLOW			0x44	/* 0x22: 00100010 -> 01000100: 0x44 */
#define IR_EX_BLOW				0x24	/* 0x24: 00100100 -> 00100100: 0x24 */
/* Instructions for the Configuration Fuse */
#define IR_CONFIG_FUSES			0x94
/* Bypass instruction */
#define IR_BYPASS				0xFF	/* 0xFF */
/* Instructions for the EEM */
#define IR_EMEX_DATA_EXCHANGE	0x90	// 09
#define IR_EMEX_READ_TRIGGER	0x50	// 0A
#define IR_EMEX_READ_CONTROL	0xD0	// 0B
#define IR_EMEX_WRITE_CONTROL	0x30	// 0C
#define IR_EMEX_DATA_EXCHANGE32	0xB0

// Instructions for the FLASH register
#define IR_FLASH_16BIT_UPDATE	0x98	// 19
#define IR_FLASH_CAPTURE		0x58	// 1A
#define IR_FLASH_16BIT_IN		0xD8	// 1B
#define IR_FLASH_UPDATE			0x38	// 1C

#define IR_TEST_REG				0x54	// 2A //Select the 32-bit JTAG test register
#define IR_TEST_3V_REG			0xF4	// 16 bit 3 volt test reg

//! \brief Request a JTAG mailbox exchange
#define IR_JMB_EXCHANGE			0x86	// 61
#define IR_JMB_WRITE_32BIT_MODE	0x11


// Bits of the control signal register
#define CNTRL_SIG_READ			0x0001
#define CNTRL_SIG_CPU_HALT		0x0002
#define CNTRL_SIG_INTR_REQ		0x0004
#define CNTRL_SIG_HALT_JTAG		0x0008
#define CNTRL_SIG_BYTE			0x0010
#define CNTRL_SIG_CPU_OFF		0x0020
#define CNTRL_SIG_MCLKON		0x0040
#define CNTRL_SIG_INSTRLOAD		0x0080
#define CNTRL_SIG_TMODE			0x0100
#define CNTRL_SIG_TCE			0x0200
#define CNTRL_SIG_TCE1			0x0400
#define CNTRL_SIG_PUC			0x0800
#define CNTRL_SIG_CPU			0x1000
#define CNTRL_SIG_TAGFUNCSAT	0x2000
#define CNTRL_SIG_SWITCH		0x4000
#define CNTRL_SIG_STOP_SEL		0x8000
#define CNTRL_SIG_CPUSUSP		(0x0001<<8)
#define CNTRL_SIG_CPUOFF		(0x0001<<5)
#define CNTRL_SIG_INTREQ		(0x0001<<2)
#define CNTRL_SIG_HALT			(0x0001<<1)


enum TapCmd : uint32_t
{
	cmdIrShift				// OnIrShift
	, cmdIrShift8			// OnIrShift / OnDrShift8 (arg)
	, cmdIrShift16			// OnIrShift / OnDrShift16 (arg)
	, cmdIrShift20			// OnIrShift / OnDrShift20 (arg) [for arg <= 0xFFFF]
	, cmdDrShift8			// OnDrShift8 (arg)
	, cmdDrShift16			// OnDrShift16 (arg)
	, cmdDrShift20			// OnDrShift20 (arg)
	, cmdClrTclk			// OnClearTclk
	, cmdSetTclk			// OnSetTclk
	, cmdPulseTclk			// OnPulseTclk
	, cmdPulseTclkN			// OnPulseTclkN
	, cmdStrobeN			// OnFlashTclk (arg)
	, cmdReleaseCpu			// ReleaseCpu()
	//////// argv commands /////////
	, cmdIrShift_argv_p		// OnIrShift (return to argv ptr)
	, cmdIrShift16_argv		// OnIrShift / OnDrShift16 (argv)
	, cmdIrShift20_argv		// OnIrShift / OnDrShift20 (argv)
	, cmdDrShift16_argv		// OnDrShift16 (argv)
	, cmdDrShift16_argv_p	// OnDrShift16 (return to argv ptr)
	, cmdDrShift20_argv		// OnDrShift20 (argv)
	, cmdStrobe_argv		// OnFlashTclk (argv)
};


struct TapStep
{
	TapCmd cmd : 8;
	uint32_t arg : 24;
};


class ITapInterface
{
public:
	virtual bool OnOpen() = 0;
	virtual void OnClose() = 0;
	virtual void OnConnectJtag() = 0;
	virtual void OnReleaseJtag() = 0;

public:
	virtual void OnEnterTap() = 0;
	virtual void OnResetTap() = 0;

public:
	virtual uint8_t OnIrShift(uint8_t byte) = 0;
	virtual uint8_t OnDrShift8(uint8_t) = 0;
	virtual uint16_t OnDrShift16(uint16_t) = 0;
	virtual uint32_t OnDrShift20(uint32_t) = 0;
	virtual bool OnInstrLoad() = 0;

	//virtual void OnClockThroughPsa() = 0;

	virtual void OnSetTclk() = 0;
	virtual void OnClearTclk() = 0;
	virtual void OnPulseTclk() = 0;
	virtual void OnPulseTclk(int count) = 0;
	virtual void OnPulseTclkN() = 0;
	virtual void OnFlashTclk(uint32_t min_pulses) = 0;

	virtual uint32_t OnReadJmbOut() = 0;
	virtual bool OnWriteJmbIn16(uint16_t data) = 0;

	virtual ITapInterface& OnStetupArchitecture(ChipInfoDB::CpuArchitecture arch) = 0;
};



//static constexpr TapStep cntrl_sig_16bit = { irShiftCmd, IR_CNTRL_SIG_16BIT };
ALWAYS_INLINE static constexpr TapStep kIr(const uint8_t ir)
{
	return { .cmd = cmdIrShift, .arg = ir };
}
ALWAYS_INLINE static constexpr TapStep kIrRet(const uint8_t ir)
{
	return { .cmd = cmdIrShift_argv_p, .arg = ir };
}
ALWAYS_INLINE static constexpr TapStep kIrDr8(const uint8_t ir, const uint8_t d)
{
	return { .cmd = cmdIrShift8, .arg = (uint32_t)(d << 8) | ir };
}
ALWAYS_INLINE static constexpr TapStep kIrDr16(const uint8_t ir, const uint16_t d)
{
	return { .cmd = cmdIrShift16, .arg = (uint32_t)(d << 8) | ir };
}
ALWAYS_INLINE static constexpr TapStep kIrDr20(const uint8_t ir, const uint16_t d)
{
	return { .cmd = cmdIrShift20, .arg = (uint32_t)(d << 8) | ir };
}
ALWAYS_INLINE static constexpr TapStep kIrDr16Argv(const uint8_t ir)
{
	return { .cmd = cmdIrShift16_argv, .arg = ir };
}
ALWAYS_INLINE static constexpr TapStep kIrDr20Argv(const uint8_t ir)
{
	return { .cmd = cmdIrShift20_argv, .arg = ir };
}
ALWAYS_INLINE static constexpr TapStep kDr16(const uint16_t d)
{
	return { .cmd = cmdDrShift16, .arg = d };
}
ALWAYS_INLINE static constexpr TapStep kDr16_ret(const uint16_t d)
{
	return { .cmd = cmdDrShift16_argv_p, .arg = d };
}
static constexpr TapStep kDr16Argv = { cmdDrShift16_argv };
ALWAYS_INLINE static constexpr TapStep kDr20(const uint32_t d)
{
	return { .cmd = cmdDrShift20, .arg = d };
}
static constexpr TapStep kDr20Argv = { cmdDrShift20_argv };
static constexpr TapStep kTclk0 = { cmdClrTclk };
static constexpr TapStep kTclk1 = { cmdSetTclk };
static constexpr TapStep kPulseTclk = { cmdPulseTclk };
static constexpr TapStep kPulseTclkN = { cmdPulseTclkN };
ALWAYS_INLINE static constexpr TapStep kStrobeTclk(const uint32_t n)
{
	return { .cmd = cmdStrobeN, .arg = n };
}
static constexpr TapStep kStrobeTclkArgv = { cmdStrobe_argv };
static constexpr TapStep kReleaseCpu = { cmdReleaseCpu };



//! Plays sequences of Jtag primitives
class TapPlayer
{
public:
	void ReleaseCpu();
	//! Plays a Jtag primitive
	uint32_t Play(const TapStep cmd);
	//! Plays sequences of Jtag primitives
	void Play(const TapStep cmds[], const uint32_t count, ...) NO_INLINE;
	void SetJtagRunRead() { Play(kSetJtagRunRead_); }
	void SetWordRead() { Play(kSetWordRead_); }
	void SetWordWrite() { Play(kSetWordWrite_); }
	void SetWordReadXv2() { Play(kSetWordReadXv2_); }

protected:
	ITapInterface* itf_;
	static constexpr TapStep kSetJtagRunRead_ = kIrDr16(IR_CNTRL_SIG_16BIT, 0x2401);
	static constexpr TapStep kSetWordRead_ = kIrDr16(IR_CNTRL_SIG_16BIT, 0x2409);
	static constexpr TapStep kSetWordWrite_ = kIrDr16(IR_CNTRL_SIG_16BIT, 0x2408);
	static constexpr TapStep kSetWordReadXv2_ = kIrDr16(IR_CNTRL_SIG_16BIT, 0x0501);
};

