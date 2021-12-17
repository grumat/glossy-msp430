#include "stdproj.h"

#include "TapDev430X.h"
#include "eem_defs.h"
#include "TapDev.h"



/**************************************************************************************/
/* MCU VERSION-RELATED REGISTER GET/SET METHODS                                       */
/**************************************************************************************/

//----------------------------------------------------------------------------
//! \brief Load a given address into the target CPU's program counter (PC).
//! \param[in] word address (destination address)
//! Source: slau320aj
bool TapDev430X::SetPC(address_t address)
{
	// Set CPU into instruction fetch mode, TCLK=1
	if (SetInstructionFetch() == kInvalid)
		return false;
#if 0
	// Load PC with address
	IR_Shift(IR_CNTRL_SIG_16BIT);
	DR_Shift16(0x3401);				// CPU has control of RW & BYTE.
	IR_Shift(IR_DATA_16BIT);
	// "mova #addr20,PC" instruction
	DR_Shift16((uint16_t)(0x0080 | (((address) >> 8) & 0x0F00)));

	itf_->OnPulseTclkN();			// F2xxx
	DR_Shift16(address);			// second word of "mova #addr20,PC" instruction
	itf_->OnPulseTclkN();			// F2xxx
	IR_Shift(IR_ADDR_CAPTURE);
	ClrTCLK();						// Now the PC should be on address
	IR_Shift(IR_CNTRL_SIG_16BIT);
	DR_Shift16(0x2401);				// JTAG has control of RW & BYTE.
#else
	static const TapStep steps[] =
	{
		// Load PC with address
		kIrDr16(IR_CNTRL_SIG_16BIT, 0x3401)		// CPU has control of RW & BYTE.
		, kIrDr16Argv(IR_DATA_16BIT)			// "mova #addr20,PC" instruction
		, kPulseTclkN			// F2xxx
		, kDr16Argv								// second word of "mova #addr20,PC" instruction
		, kPulseTclkN			// F2xxx
		, kIr(IR_ADDR_CAPTURE)
		, kTclk0								// Now the PC should be on address
		, kIrDr16(IR_CNTRL_SIG_16BIT, 0x2401)	// JTAG has control of RW & BYTE.
	};
	g_Player.Play(steps, _countof(steps)
		 , (uint16_t)(0x0080 | (((address) >> 8) & 0x0F00))
		 , address
	);
#endif
	return true;
}


// Source: uif
bool TapDev430X::SetReg(uint8_t reg, uint32_t value)
{
	uint16_t op = 0x0080 | (uint16_t)reg | ((value >> 8) & 0x0F00);
#if 0
	cntrl_sig_high_byte();
	SetReg_8Bits(0x34);
	data_16bit();
	IHIL_Tclk(1);
	SetReg_16Bits(op);
	IHIL_Tclk(0);
	data_capture();
	IHIL_Tclk(1);
	data_16bit();
	IHIL_Tclk(1);
	SetReg_16Bits((uint16_t)value);
	IHIL_Tclk(0);
	data_capture();
	IHIL_Tclk(1);
	data_16bit();
	IHIL_Tclk(1);
	SetReg_16Bits(0x3ffd);
	IHIL_Tclk(0);
	data_capture();
	itf_->OnPulseTclk();
	cntrl_sig_high_byte();
	SetReg_8Bits(0x24);
	IHIL_Tclk(1);
#else
	static constexpr TapStep steps[] =
	{
		kIrDr8(IR_CNTRL_SIG_HIGH_BYTE, 0x34)
		, kIr(IR_DATA_16BIT)
		, kTclk1
		, kDr16Argv				// op
		, kTclk0
		, kIr(IR_DATA_CAPTURE)
		, kTclk1
		, kIr(IR_DATA_16BIT)
		, kTclk1
		, kDr16Argv				// value
		, kTclk0
		, kIr(IR_DATA_CAPTURE)
		, kTclk1
		, kIr(IR_DATA_16BIT)
		, kTclk1
		, kDr16(0x3ffd)
		, kTclk0
		, kIr(IR_DATA_CAPTURE)
		, kPulseTclk
		, kIrDr8(IR_CNTRL_SIG_HIGH_BYTE, 0x24)
		, kTclk1
	};
	g_Player.Play(steps, _countof(steps)
		 , op
		 , value
	);
#endif
	return true;
}


// Source: uif
uint32_t TapDev430X::GetReg(uint8_t reg)
{
#if 0
	cntrl_sig_high_byte();
	SetReg_16Bits(0x34);
	uint16_t op = ((reg << 8) & 0x0F00) | 0x60;
	data_16bit();
	IHIL_Tclk(1);
	SetReg_16Bits(op);
	IHIL_Tclk(0);
	data_capture();
	IHIL_Tclk(1);
	data_16bit();
	IHIL_Tclk(1);
	SetReg_16Bits(0x00fc);
	IHIL_Tclk(0);
	data_capture();
	IHIL_Tclk(1);
	uint16_t rx_l = SetReg_16Bits(0);
	itf_->OnPulseTclkN();
	uint16_t rx_h = SetReg_16Bits(0);
	IHIL_Tclk(0);
	cntrl_sig_high_byte();
	SetReg_8Bits(0x24);
	IHIL_Tclk(1);
	return ((uint32_t)rx_h << 16) | rx_l;
#else
	static constexpr TapStep steps[] =
	{
		kIrDr8(IR_CNTRL_SIG_HIGH_BYTE, 0x34)
		, kIr(IR_DATA_16BIT)
		, kTclk1
		, kDr16Argv				// op = ((reg << 8) & 0x0F00) | 0x60;
		, kTclk0
		, kIr(IR_DATA_CAPTURE)
		, kTclk1
		, kIr(IR_DATA_16BIT)
		, kTclk1
		, kDr16(0x00fc)			// address part of "mova rX, &00fc"
		, kTclk0
		, kIr(IR_DATA_CAPTURE)
		, kTclk1
		, kDr16_ret(0)			// rx_l = dr16(0);
		, kPulseTclkN
		, kDr16_ret(0)			// rx_h = dr16(0);
		, kTclk0
		, kIrDr8(IR_CNTRL_SIG_HIGH_BYTE, 0x24)
		, kTclk1
	};
	uint16_t rx_l = 0xFFFF;
	uint16_t rx_h = 0xFFFF;
	g_Player.Play(steps, _countof(steps)
		 , ((reg << 8) & 0x0F00) | 0x60	// equivalent to "mova rX, &00fc"
		 , &rx_l
		 , &rx_h
	);
	return ((uint32_t)rx_h << 16) | rx_l;
#endif
}



/**************************************************************************************/
/* MCU VERSION-RELATED READ MEMORY METHODS                                            */
/**************************************************************************************/

// Source: slau320aj
uint16_t TapDev430X::ReadWord(address_t address)
{
	if (!HaltCpu())
		return 0xFFFF;
#if 1
	g_Player.itf_->OnClearTclk();
	g_Player.SetWordRead();					// Set RW to read: ir_dr16(IR_CNTRL_SIG_16BIT, 0x2409);
	// Set address
	g_Player.itf_->OnIrShift(IR_ADDR_16BIT);
	g_Player.itf_->OnDrShift20(address);
	g_Player.itf_->OnIrShift(IR_DATA_TO_ADDR);
	g_Player.itf_->OnPulseTclk();
	// Fetch 16-bit data
	uint16_t content = g_Player.itf_->OnDrShift16(0x0000);
	g_Player.ReleaseCpu();
#else
	static constexpr TapStep ReadWordX_steps[] =
	{
		kTclk0
		, kIrDr16(IR_CNTRL_SIG_16BIT, 0x2409)
		// Set address
		, kIrDr20Argv(IR_ADDR_16BIT)	// dr20(address)
		, kIr(IR_DATA_TO_ADDR)
		, kPulseTclk
		, kDr16_ret(0x0000)				// content = dr16(0x0000)
		, kReleaseCpu
	};
	uint16_t content = 0xFFFF;
	Play(
		ReadWordX_steps
		, address
		, &content
	);
#endif
	return content;
}


//Source: slau320aj
bool TapDev430X::ReadWords(address_t address, uint16_t *buf, uint32_t word_count)
{
	if (!HaltCpu())
		return false;
	g_Player.itf_->OnClearTclk();
	g_Player.SetWordRead();					// Set RW to read: ir_dr16(IR_CNTRL_SIG_16BIT, 0x2409);
	for (uint32_t i = 0; i < word_count; ++i)
	{
		// Set address
		g_Player.itf_->OnIrShift(IR_ADDR_16BIT);
		g_Player.itf_->OnDrShift20(address);
		g_Player.itf_->OnIrShift(IR_DATA_TO_ADDR);
		g_Player.itf_->OnPulseTclk();
		// Fetch 16-bit data
		*buf++ = g_Player.itf_->OnDrShift16(0x0000);
		address += 2;
	}
	g_Player.ReleaseCpu();
	return true;
}



/**************************************************************************************/
/* MCU VERSION-RELATED WRITE MEMORY METHODS                                           */
/**************************************************************************************/

//----------------------------------------------------------------------------
//! \brief This function writes one byte/word at a given address ( <0xA00)
//! \param[in] word address (Address of data to be written)
//! \param[in] word data (shifted data)
//! Source: slau320aj
bool TapDev430X::WriteWord(address_t address, uint16_t data)
{
	if (!HaltCpu())
		return false;

#if 0
	ClrTCLK();
	IR_Shift(IR_CNTRL_SIG_16BIT);
	DR_Shift16(0x2408);
	IR_Shift(IR_ADDR_16BIT);
	DR_Shift20(address);		// Set addr
	IR_Shift(IR_DATA_TO_ADDR);
	DR_Shift16(data);			// Shift in 16 bits
	SetTCLK();
	ReleaseCpu();
#else
	static constexpr TapStep steps[] =
	{
		kTclk0
		, kIrDr16(IR_CNTRL_SIG_16BIT, 0x2408)	// Set word write
		, kIrDr20Argv(IR_ADDR_16BIT)			// Set 'address'
		, kIrDr16Argv(IR_DATA_TO_ADDR)			// Shift in 16 'data' bits
		, kReleaseCpu
	};
	g_Player.Play(steps, _countof(steps)
		 , address
		 , data
	);
#endif

	return true;
}


//----------------------------------------------------------------------------
//! \brief This function writes an array of words into the target memory.
//! \param[in] word address (Start address of target memory)
//! \param[in] word *buf (Pointer to array with the data)
//! \param[in] word word_count (Number of words to be programmed)
//! Source: slau320aj
bool TapDev430X::WriteWords(address_t address, const uint16_t *buf, uint32_t word_count)
{
	uint32_t i;

	// Initialize writing:
	if (!TapDev430X::SetPC(address - 4)
		|| !HaltCpu())
		return false;

	g_Player.itf_->OnClearTclk();
	g_Player.SetWordWrite();			// Set RW to write: ir_dr16(IR_CNTRL_SIG_16BIT, 0x2408);
	g_Player.itf_->OnIrShift(IR_DATA_QUICK);
	for (i = 0; i < word_count; i++)
	{
		g_Player.itf_->OnDrShift16(buf[i]);	// Shift in the write data
		g_Player.itf_->OnPulseTclk();			// Increment PC by 2
	}
	g_Player.ReleaseCpu();
	return true;
}


// Source: slau320aj
bool TapDev430X::WriteFlash(address_t address, const uint16_t *buf, uint32_t word_count)
{
	uint32_t addr = address;				// Address counter
	if (!HaltCpu())
		return false;

#if 0
	ClrTCLK();
	IR_Shift(IR_CNTRL_SIG_16BIT);
	DR_Shift16(0x2408);			// Set RW to write
	IR_Shift(IR_ADDR_16BIT);
	DR_Shift20(0x0128);			// FCTL1 register
	IR_Shift(IR_DATA_TO_ADDR);
	DR_Shift16(0xA540);			// Enable FLASH write
	SetTCLK();

	ClrTCLK();
	IR_Shift(IR_ADDR_16BIT);
	DR_Shift20(0x012A);			// FCTL2 register
	IR_Shift(IR_DATA_TO_ADDR);
	DR_Shift16(0xA540);			// Select MCLK as source, DIV=1
	SetTCLK();

	ClrTCLK();
	IR_Shift(IR_ADDR_16BIT);
	DR_Shift20(0x012C);			// FCTL3 register
	IR_Shift(IR_DATA_TO_ADDR);
	DR_Shift16(Fctl3Unlock_X);	// Clear FCTL3; F2xxx: Unlock Info-Seg.
								// A by toggling LOCKA-Bit if required,
	SetTCLK();

	ClrTCLK();
	IR_Shift(IR_CNTRL_SIG_16BIT);
#else
	static constexpr TapStep steps_01[] =
	{
		kTclk0
		, kIrDr16(IR_CNTRL_SIG_16BIT, 0x2408)	// Set RW to write
		, kIrDr20(IR_ADDR_16BIT, 0x0128)		// FCTL1 register
		, kIrDr16(IR_DATA_TO_ADDR, 0xA540)		// Enable FLASH write
		, kTclk1

		, kTclk0
		, kIrDr20(IR_ADDR_16BIT, 0x012A)		// FCTL2 register
		, kIrDr16(IR_DATA_TO_ADDR, 0xA540)		// Select MCLK as source, DIV=1
		, kTclk1

		, kTclk0
		, kIrDr20(IR_ADDR_16BIT, 0x012C)			// FCTL3 register
		, kIrDr16(IR_DATA_TO_ADDR, kFctl3Unlock_X)	// Clear FCTL3; F2xxx: Unlock Info-Seg.
													// A by toggling LOCKA-Bit if required,
		, kTclk1

		, kTclk0
		, kIr(IR_CNTRL_SIG_16BIT)
	};
	g_Player.Play(steps_01, _countof(steps_01));
#endif
	for (uint32_t i = 0; i < word_count; i++, addr += 2)
	{
#if 0
		DR_Shift16(0x2408);				// Set RW to write
		IR_Shift(IR_ADDR_16BIT);
		DR_Shift20(addr);				// Set address
		IR_Shift(IR_DATA_TO_ADDR);
		DR_Shift16(buf[i]);				// Set data
		itf_->OnPulseTclk();
		IR_Shift(IR_CNTRL_SIG_16BIT);
		DR_Shift16(0x2409);				// Set RW to read

		TCLKstrobes(35);

#else
		static constexpr TapStep steps_02[] =
		{
			kDr16(0x2408)							// Set RW to write
			, kIrDr20Argv(IR_ADDR_16BIT)			// Set address
			, kIrDr16Argv(IR_DATA_TO_ADDR)			// Set data
			, kPulseTclk
			, kIrDr16(IR_CNTRL_SIG_16BIT, 0x2409)	// Set RW to read
			, kStrobeTclk(35)						// Provide TCLKs, min. 33 for F149 and F449
													// F2xxx: 29 are ok
		};
		g_Player.Play(steps_02, _countof(steps_02)
			 , addr
			 , buf[i]
		);
#endif
	}

#if 0
	IR_Shift(IR_CNTRL_SIG_16BIT);
	DR_Shift16(0x2408);			// Set RW to write
	IR_Shift(IR_ADDR_16BIT);
	DR_Shift20(0x0128);			// FCTL1 register
	IR_Shift(IR_DATA_TO_ADDR);
	DR_Shift16(Fctl1Lock_X);	// Disable FLASH write
	SetTCLK();

	// set LOCK-Bits again
	ClrTCLK();
	IR_Shift(IR_ADDR_16BIT);
	DR_Shift20(0x012C);			// FCTL3 address
	IR_Shift(IR_DATA_TO_ADDR);
	DR_Shift16(Fctl3Lock_X);	// Lock Inf-Seg. A by toggling LOCKA and set LOCK again
	SetTCLK();
	ReleaseCpu();
#else
	static constexpr TapStep steps_03[] =
	{
		kIrDr16(IR_CNTRL_SIG_16BIT, 0x2408)			// Set RW to write
		, kIrDr20(IR_ADDR_16BIT, 0x0128)			// FCTL1 register
		, kIrDr16(IR_DATA_TO_ADDR, kFctl3Unlock_X)	// Enable FLASH write
		, kTclk1

		, kTclk0
		, kIrDr20(IR_ADDR_16BIT, 0x012C)			// FCTL3 register
		// Lock Inf-Seg. A by toggling LOCKA and set LOCK again
		, kIrDr16(IR_DATA_TO_ADDR, kFctl3Lock_X)
		, kTclk1
		, kReleaseCpu
	};
	g_Player.Play(steps_03, _countof(steps_03));
#endif

	return true;
}



/**************************************************************************************/
/* MCU VERSION-RELATED FLASH ERASE                                                    */
/**************************************************************************************/

// Source: slau320aj
bool TapDev430X::EraseFlash(address_t address, const uint16_t fctl1, const uint16_t fctl3)
{
	uint32_t strobe_amount = 4820;			// default for Segment Erase
	uint32_t loop_cnt = 1;					// erase cycle repeating for Mass Erase

	if ((fctl1 == kMassEraseSlau056)
		|| (fctl1 == kMainEraseSlau056)
		)
	{
		if (g_JtagDev.IsFlastFlash())
		{
			strobe_amount = 10600;	// Larger Flash memories require
		}
		else
		{
			strobe_amount = 5300;	// Larger Flash memories require
			loop_cnt = 19;			// additional cycles for erase.
		}
	}
	if (!HaltCpu())
		return false;

	for (uint32_t i = loop_cnt; i > 0; i--)
	{
#if 0
		ClrTCLK();
		IR_Shift(IR_CNTRL_SIG_16BIT);
		DR_Shift16(0x2408);			// set RW to write
		IR_Shift(IR_ADDR_16BIT);
		DR_Shift20(0x0128);			// FCTL1 address
		IR_Shift(IR_DATA_TO_ADDR);
		DR_Shift16(fctl1);			// Enable erase mode
		SetTCLK();

		ClrTCLK();
		IR_Shift(IR_ADDR_16BIT);
		DR_Shift20(0x012A);			// FCTL2 address
		IR_Shift(IR_DATA_TO_ADDR);
		DR_Shift16(0xA540);			// MCLK is source, DIV=1
		SetTCLK();

		ClrTCLK();
		IR_Shift(IR_ADDR_16BIT);
		DR_Shift20(0x012C);			// FCTL3 address
		IR_Shift(IR_DATA_TO_ADDR);
		DR_Shift16(fctl3);			// Clear FCTL3; F2xxx: Unlock Info-Seg. A by toggling LOCKA-Bit if required,
		SetTCLK();

		ClrTCLK();
		IR_Shift(IR_ADDR_16BIT);
		DR_Shift20(address);		// Set erase address
		IR_Shift(IR_DATA_TO_ADDR);
		DR_Shift16(0x55AA);			// Dummy write to start erase
		SetTCLK();

		ClrTCLK();
		IR_Shift(IR_CNTRL_SIG_16BIT);
		DR_Shift16(0x2409);			// Set RW to read
		TCLKstrobes(strobe_amount);	// Provide TCLKs
		IR_Shift(IR_CNTRL_SIG_16BIT);
		DR_Shift16(0x2408);			// Set RW to write
		IR_Shift(IR_ADDR_16BIT);
		DR_Shift20(0x0128);			// FCTL1 address
		IR_Shift(IR_DATA_TO_ADDR);
		DR_Shift16(Fctl1Lock_X);	// Disable erase
		SetTCLK();
#else
		static constexpr TapStep steps_01[] =
		{
			kTclk0
			, kIrDr16(IR_CNTRL_SIG_16BIT, 0x2408)
			, kIrDr20(IR_ADDR_16BIT, 0x0128)		// FCTL1 address
			, kIrDr16Argv(IR_DATA_TO_ADDR)			// Enable erase "fctl1"
			, kTclk1

			, kTclk0
			, kIrDr20(IR_ADDR_16BIT, 0x012A)		// FCTL2 address
			, kIrDr16(IR_DATA_TO_ADDR, 0xA540)		// MCLK is source, DIV=1
			, kTclk1

			, kTclk0
			, kIrDr20(IR_ADDR_16BIT, 0x012C)		// FCTL3 address
			, kIrDr16Argv(IR_DATA_TO_ADDR)			// Clear FCTL3; F2xxx: Unlock Info-Seg. A 
			, kTclk1								// by toggling LOCKA-Bit if required,

			, kTclk0
			, kIrDr20Argv(IR_ADDR_16BIT)			// Set erase "address"
			, kIrDr16(IR_DATA_TO_ADDR, 0x55AA)		// Dummy write to start erase
			, kTclk1								// by toggling LOCKA-Bit if required,

			, kTclk0
			, kIrDr16(IR_CNTRL_SIG_16BIT, 0x2409)	// Set RW to read
			, kStrobeTclkArgv						// Provide 'strobe_amount' TCLKs
			, kIrDr16(IR_CNTRL_SIG_16BIT, 0x2408)	// Set RW to write
			, kIrDr20(IR_ADDR_16BIT, 0x0128)		// FCTL1 address
			, kIrDr16(IR_DATA_TO_ADDR, kFctl1Lock_X)	// Disable erase
			, kTclk1
		};
		g_Player.Play(steps_01, _countof(steps_01)
			 , fctl1
			 , fctl3
			 , address
			 , strobe_amount
		);
#endif
	}
	// set LOCK-Bits again
#if 0
	ClrTCLK();
	IR_Shift(IR_ADDR_16BIT);
	DR_Shift20(0x012C);				// FCTL3 address
	IR_Shift(IR_DATA_TO_ADDR);
	DR_Shift16(Fctl3Lock_X);		// Lock Inf-Seg. A by toggling LOCKA (F2xxx) and set LOCK again
	SetTCLK();

	ReleaseCpu();
#else
	static constexpr TapStep steps_02[] =
	{
		kTclk0
		, kIrDr20(IR_ADDR_16BIT, 0x012C)				// FCTL3 address
		, kIrDr16(IR_DATA_TO_ADDR, kFctl3Lock_X)	// Lock Inf-Seg. A by toggling LOCKA (F2xxx) and set LOCK again
		, kTclk1
		, kReleaseCpu
	};
	g_Player.Play(steps_02, _countof(steps_02));
#endif
	return true;
}



/**************************************************************************************/
/* MCU VERSION-RELATED POWER ON RESET                                                 */
/**************************************************************************************/

// Source UIF
bool TapDev430X::SyncJtagAssertPorSaveContext(CpuContext &ctx, const ChipProfile &prof)
{
	uint16_t address = WDT_ADDR_CPU;
	uint16_t wdtval = WDT_HOLD;
	uint16_t ctl_sync = 0;

	ctx.is_running_ = false;

	// Sync the JTAG
	if (g_Player.SetJtagRunReadLegacy() != kMspStd)
		return false;

	uint32_t lOut = g_Player.GetCtrlSigReg();    // read control register once

	g_Player.SetTCLK();

	if (!(lOut & CNTRL_SIG_TCE))
	{
		// If the JTAG and CPU are not already synchronized ...
		// Initiate Jtag and CPU synchronization. Read/Write is under CPU control. Source TCLK via TDI.
		static constexpr TapStep steps_01[] =
		{
			// Do not effect bits used by DTC (CPU_HALT, MCLKON).
			kIrDr8(IR_CNTRL_SIG_HIGH_BYTE, (CNTRL_SIG_TAGFUNCSAT | CNTRL_SIG_TCE1 | CNTRL_SIG_CPU) >> 8),
			// Address Force Sync special handling
			// read access to EEM General Clock Control Register (GCLKCTRL)
			kIrDr32(IR_EMEX_DATA_EXCHANGE32, MX_GCLKCTRL + MX_READ),
			// read the content of GCLKCNTRL into lOut
			kDr32_ret(0),
		};

		g_Player.Play(steps_01, _countof(steps_01), &lOut);
		// Set Force Jtag Synchronization bit in Emex General Clock Control register.
		lOut |= 0x0040;							// 0x0040 = FORCE_SYN

		// Stability improvement: should be possible to remove this, required only once at the beginning
		// write access to EEM General Clock Control Register (GCLKCTRL)
		g_Player.Play(kIrDr32(IR_EMEX_DATA_EXCHANGE32, MX_GCLKCTRL + MX_WRITE));
		lOut = g_Player.DR_Shift32(lOut);		// write into GCLKCNTRL

		// Reset Force Jtag Synchronization bit in Emex General Clock Control register.
		lOut &= ~0x0040;
		// Stability improvement: should be possible to remove this, required only once at the beginning
		// write access to EEM General Clock Control Register (GCLKCTRL)
		g_Player.Play(kIrDr32(IR_EMEX_DATA_EXCHANGE32, MX_GCLKCTRL + MX_WRITE));
		lOut = g_Player.DR_Shift32(lOut);		// write into GCLKCNTRL

		ctl_sync = SyncJtag();

		if (!(ctl_sync & CNTRL_SIG_CPU_HALT))
		{ // Synchronization failed!
			return false;
		}
		g_Player.ClrTCLK();
		g_Player.Play(TapPlayer::kSetJtagRunRead_);
		g_Player.SetTCLK();
	}
	else
	{
		g_Player.Play(TapPlayer::kSetJtagRunRead_);
	}

	// execute a dummy instruction here
	static constexpr TapStep steps_02[] =
	{
		kIr(IR_DATA_16BIT)
		, kTclk1			// Stability improvement: should be possible to remove this TCLK is already 1
		, kDr16(0x4303)		// 0x4303 = NOP
		, kTclk0
		, kIr(IR_DATA_CAPTURE)
		, kTclk1
	};
	g_Player.Play(steps_02, _countof(steps_02));

	// step until next instruction load boundary if not being already there
	if (!IsInstrLoad())
		return false;

	if (prof.clk_ctrl_ == ChipInfoDB::kGccExtended)
	{
		static constexpr TapStep steps[] =
		{
			// Perform the POR
			kIrDr32(IR_EMEX_DATA_EXCHANGE32, MX_GENCNTRL + MX_WRITE)	// write access to EEM General Control Register (MX_GENCNTRL)
			, kDr32(EMU_FEAT_EN | EMU_CLK_EN | CLEAR_STOP | EEM_EN)		// write into MX_GENCNTRL
			// Stability improvement: should be possible to remove this, required only once at the beginning
			, kIrDr32(IR_EMEX_DATA_EXCHANGE32, MX_GENCNTRL + MX_WRITE)	// write access to EEM General Control Register (MX_GENCNTRL)
			, kDr32(EMU_FEAT_EN | EMU_CLK_EN)							// write into MX_GENCNTRL
		};
		g_Player.Play(steps, _countof(steps));
	}

	static constexpr TapStep steps_03[] =
	{
		kTclk0
		// Assert PUC
		, kIrDr16(IR_CNTRL_SIG_16BIT, CNTRL_SIG_READ | CNTRL_SIG_TCE1 | CNTRL_SIG_PUC | CNTRL_SIG_TAGFUNCSAT)
		, kTclk1
		// Negate PUC
		, kIrDr16(IR_CNTRL_SIG_16BIT, CNTRL_SIG_READ | CNTRL_SIG_TCE1 | CNTRL_SIG_TAGFUNCSAT)

		, kTclk0
		// Assert PUC
		, kIrDr16(IR_CNTRL_SIG_16BIT, CNTRL_SIG_READ | CNTRL_SIG_TCE1 | CNTRL_SIG_PUC | CNTRL_SIG_TAGFUNCSAT)
		, kTclk1
		// Negate PUC
		, kIrDr16(IR_CNTRL_SIG_16BIT, CNTRL_SIG_READ | CNTRL_SIG_TCE1 | CNTRL_SIG_TAGFUNCSAT)

		// Explicitly set TMR
		, kDr16(CNTRL_SIG_READ | CNTRL_SIG_TCE1)		// Enable access to Flash registers

		, kIrDr16(IR_FLASH_16BIT_UPDATE, FLASH_SESEL1)	// Disable flash test mode
		, kDr16(FLASH_SESEL1 | FLASH_TMR)				// Pulse TMR
		, kDr16(FLASH_SESEL1)
		, kDr16(FLASH_SESEL1 | FLASH_TMR)				// Set TMR to user mode

		// Disable access to Flash register
		, kIrDr8(IR_CNTRL_SIG_HIGH_BYTE, (CNTRL_SIG_TAGFUNCSAT | CNTRL_SIG_TCE1) >> 8)
	};
	g_Player.Play(steps_03, _countof(steps_03));

	// step until an appropriate instruction load boundary
	uint32_t i = 10;
	while (true)
	{
		lOut = g_Player.Play(kIrDr20(IR_ADDR_CAPTURE, 0x0000)) & 0xffff;
		if (lOut == 0xFFFE || lOut == 0x0F00)
			break;
		if (i == 0)
			return false;
		--i;
		g_Player.PulseTCLKN();
	}

	static constexpr TapStep steps_04[] =
	{
		kPulseTclkN
		, kPulseTclkN

		, kTclk0
		, kIrDr16(IR_ADDR_CAPTURE, 0x0000)
		, kTclk1
	};
	g_Player.Play(steps_04, _countof(steps_04));

	// step until next instruction load boundary if not being already there
	if (!IsInstrLoad())
		return false;

	// Hold Watchdog
	ctx.wdt_ = ReadWord(address);	// safe WDT value
	wdtval |= ctx.wdt_;				// set original bits in addition to stop bit
	WriteWord(address, wdtval);

	// read MAB = PC here
	ctx.pc_ = g_Player.Play(kIrDr20(IR_ADDR_CAPTURE, 0));

	// set PC to a save address pointing to ROM to avoid RAM corruption on certain devices
	SetPC(ROM_ADDR);

	// read status register
	ctx.sr_ = GetReg(2);

	return true;
}


// Source: slau320aj
bool TapDev430X::ExecutePOR()
{
	// Perform Reset
#if 0
	IR_Shift(IR_CNTRL_SIG_16BIT);
	DR_Shift16(0x2C01);						// Apply Reset
	DR_Shift16(0x2401);						// Remove Reset
	itf_->OnPulseTclkN();					// F2xxx
	itf_->OnPulseTclkN();
	ClrTCLK();
	// read JTAG ID, checked at function end
	uint8_t jtag_ver = IR_Shift(IR_ADDR_CAPTURE);
	SetTCLK();
#else
	uint8_t jtag_ver;
	static constexpr TapStep steps[] =
	{
		kIrDr16(IR_CNTRL_SIG_16BIT, 0x2C01)	// Apply Reset
		, kDr16(0x2401)						// Remove Reset
		, kPulseTclkN						// F2xxx
		, kPulseTclkN
		, kTclk0
		, kIrRet(IR_ADDR_CAPTURE)			// returns the jtag ID
		, kTclk1
	};
	g_Player.Play(steps, _countof(steps)
		 , &jtag_ver
	);
#endif

	TapDev430X::WriteWord(0x0120, 0x5A80);	// Disable Watchdog on target device

	if (jtag_ver != kMspStd)
		return false;
	return true;
}



/**************************************************************************************/
/* MCU VERSION-RELATED DEVICE RELEASE                                                 */
/**************************************************************************************/

