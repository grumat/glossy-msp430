#include "stdproj.h"

#include "TapDev430.h"
#include "eem_defs.h"
#include "TapMcu.h"


/**************************************************************************************/
/* MCU VERSION-RELATED REGISTER GET/SET METHODS                                       */
/**************************************************************************************/

//----------------------------------------------------------------------------
//! \brief Load a given address into the target CPU's program counter (PC).
//! \param[in] word address (destination address)
//! slau320aj
bool TapDev430::SetPC(address_t address)
{
	// Set CPU into instruction fetch mode, TCLK=1
	if (SetInstructionFetch() == kInvalid)
		return false;
#if 0
	// Load PC with address
	IR_Shift(IR_CNTRL_SIG_16BIT);
	DR_Shift16(0x3401);				// CPU has control of RW & BYTE.
	IR_Shift(IR_DATA_16BIT);
	DR_Shift16(0x4030);				// "mov #addr,PC" instruction
	itf_->OnPulseTclkN();
	DR_Shift16(address);			// "mov #addr,PC" instruction
	itf_->OnPulseTclkN();
	IR_Shift(IR_ADDR_CAPTURE);
	ClrTCLK();						// Now the PC should be on address
	IR_Shift(IR_CNTRL_SIG_16BIT);
	DR_Shift16(0x2401);				// JTAG has control of RW & BYTE.
#else
	static const TapStep steps[] =
	{
		// Load PC with address
		kIrDr16(IR_CNTRL_SIG_16BIT, 0x3401)		// CPU has control of RW & BYTE.
		, kIrDr16(IR_DATA_16BIT, 0x4030)		// "mov #addr,PC" instruction
		, kPulseTclkN		// F2xxx
		, kDr16Argv								// "mov #addr,PC" instruction
		, kPulseTclkN		// F2xxx
		, kIr(IR_ADDR_CAPTURE)
		, kTclk0								// Now the PC should be on address
		, kIrDr16(IR_CNTRL_SIG_16BIT, 0x2401)	// JTAG has control of RW & BYTE.
	};
	g_Player.Play(steps, _countof(steps)
		 , address
	);
#endif
	return true;
}


// Source: uif
bool TapDev430::SetReg(uint8_t reg, uint32_t value)
{
#if 0
	cntrl_sig_16bit();
	SetReg_16Bits(0x3401);
	data_16bit();
	uint16_t op = (0x4030 | reg);
	SetReg_16Bits(op);
	IHIL_Tclk(0);
	data_capture();
	IHIL_Tclk(1);
	data_16bit();
	SetReg_16Bits(value);
	IHIL_Tclk(0);
	data_capture();
	IHIL_Tclk(1);
	data_16bit();
	SetReg_16Bits(0x3ffd);
	IHIL_Tclk(0);
	data_capture();
	IHIL_Tclk(1);
	IHIL_Tclk(0);
	cntrl_sig_16bit();
	SetReg_16Bits(0x2401);
	IHIL_Tclk(1);
#else
	static constexpr TapStep steps[] =
	{
		kIrDr16(IR_CNTRL_SIG_16BIT, 0x3401)
		, kIrDr16Argv(IR_DATA_16BIT)
		, kTclk0
		, kIr(IR_DATA_CAPTURE)
		, kTclk1
		, kIrDr16Argv(IR_DATA_16BIT)
		, kTclk0
		, kIr(IR_DATA_CAPTURE)
		, kTclk1
		, kIrDr16(IR_DATA_16BIT, 0x3ffd)
		, kTclk0
		, kIr(IR_DATA_CAPTURE)
		, kPulseTclk
		, kIrDr16(IR_CNTRL_SIG_16BIT, 0x2401)
		, kTclk1
	};
	g_Player.Play(
		steps, _countof(steps)
		, (0x4030 | reg)
		, value
	);
#endif
	return true;
}


// Source: uif
uint32_t TapDev430::GetReg(uint8_t reg)
{
#if 0
	cntrl_sig_16bit();
	SetReg_16Bits(0x3401);
	data_16bit();
	uint16_t op = ((reg << 8) & 0x0F00) | 0x4082;
	SetReg_16Bits(op);
	IHIL_Tclk(0);
	data_capture();
	IHIL_Tclk(1);
	data_16bit();
	SetReg_16Bits(0x00fe);
	IHIL_Tclk(0);
	data_capture();
	itf_->OnPulseTclk();
	IHIL_Tclk(1);
	uint16_t data = SetReg_16Bits(0);
	IHIL_Tclk(0);
	cntrl_sig_16bit();
	SetReg_16Bits(0x2401);
	IHIL_Tclk(1);
	return data;
#else
	static constexpr TapStep steps[] =
	{
		kIrDr16(IR_CNTRL_SIG_16BIT, 0x3401)
		, kIrDr16Argv(IR_DATA_CAPTURE)
		, kTclk0
		, kIr(IR_DATA_CAPTURE)
		, kTclk1
		, kIrDr16(IR_DATA_16BIT, 0x00fe)	// address part of "mov rX, &00fe"
		, kTclk0
		, kIr(IR_DATA_CAPTURE)
		, kPulseTclk
		, kTclk1
		, kDr16_ret(0)			// *data = dr16(0)
		, kTclk0
		, kIrDr16(IR_CNTRL_SIG_16BIT, 0x2401)
		, kTclk1
	};

	uint16_t data = 0xFFFF;
	g_Player.Play(steps, _countof(steps)
		 , ((reg << 8) & 0x0F00) | 0x4082	// equivalent to "mov rX, &00fe"
		 , &data
	);
	return data;
#endif
}



/**************************************************************************************/
/* MCU VERSION-RELATED READ MEMORY METHODS                                            */
/**************************************************************************************/

// Source: slau320aj
uint16_t TapDev430::ReadWord(address_t address)
{
	if (!HaltCpu())
		return 0xFFFF;
#if 0
	itf_->OnClearTclk();
	itf_->OnIrShift(IR_CNTRL_SIG_16BIT);
	itf_->OnDrShift16(0x2409);
	// Set address
	itf_->OnIrShift(IR_ADDR_16BIT);
	itf_->OnDrShift16(address);
	itf_->OnIrShift(IR_DATA_TO_ADDR);
	itf_->OnPulseTclk();
	// Fetch 16-bit data
	uint16_t content = itf_->OnDrShift16(0x0000);
	//itf_->OnSetTclk(); /* is also the first instruction in ReleaseCpu() */
	ReleaseCpu();
#else
	static constexpr TapStep steps[] =
	{
		kTclk0
		, kIrDr16(IR_CNTRL_SIG_16BIT, 0x2409)
		// Set address
		, kIrDr16Argv(IR_ADDR_16BIT)	// dr16(address)
		, kIr(IR_DATA_TO_ADDR)
		, kPulseTclk
		, kDr16_ret(0x0000)				// content = dr16(0x0000)
		, kReleaseCpu
	};
	uint16_t content = 0xFFFF;
	g_Player.Play(steps, _countof(steps)
		 , address
		 , &content
	);
#endif
	return content;
}


// Source: slau320aj
bool TapDev430::ReadWords(address_t address, uint16_t *buf, uint32_t word_count)
{
	if (!HaltCpu())
		return false;
	g_Player.itf_->OnClearTclk();
	g_Player.SetWordRead();				// Set RW to read: ir_dr16(IR_CNTRL_SIG_16BIT, 0x2409);
	for (uint32_t i = 0; i < word_count; ++i)
	{
		// Set address
		g_Player.itf_->OnIrShift(IR_ADDR_16BIT);
		g_Player.itf_->OnDrShift16(address);
		g_Player.itf_->OnIrShift(IR_DATA_TO_ADDR);
		g_Player.itf_->OnPulseTclk();
		// Fetch 16-bit data
		*buf++ = g_Player.itf_->OnDrShift16(0x0000);
		address += 2;
	}
	//g_Player.itf_->OnSetTclk(); // is also the first instruction in ReleaseCpu()
	g_Player.ReleaseCpu();
	return true;
}


//----------------------------------------------------------------------------
//! \brief This function writes one byte/word at a given address ( <0xA00)
//! \param[in] word address (Address of data to be written)
//! \param[in] word data (shifted data)
//! Source: slau320aj
bool TapDev430::WriteWord(address_t address, uint16_t data)
{
	if (!HaltCpu())
		return false;

#if 0
	ClrTCLK();
	IR_Shift(IR_CNTRL_SIG_16BIT);
	DR_Shift16(0x2408);			// Set word write
	IR_Shift(IR_ADDR_16BIT);
	DR_Shift16(address);		// Set address
	IR_Shift(IR_DATA_TO_ADDR);
	DR_Shift16(data);			// Shift in 16 bits
	SetTCLK();

	ReleaseCpu();
#else
	static constexpr TapStep steps[] =
	{
		kTclk0
		, kIrDr16(IR_CNTRL_SIG_16BIT, 0x2408)	// Set word write
		, kIrDr16Argv(IR_ADDR_16BIT)			// Set address
		, kIrDr16Argv(IR_DATA_TO_ADDR)			// Shift in 16 bits
		, kTclk1
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
//! \param[in] word word_count (Number of words to be programmed)
//! \param[in] word *buf (Pointer to array with the data)
//! Source: slau320aj
bool TapDev430::WriteWords(address_t address, const uint16_t *buf, uint32_t word_count)
{
	// Initialize writing:
	if (!TapDev430::SetPC(address - 4)
		|| !HaltCpu())
		return false;

	g_Player.itf_->OnClearTclk();
	g_Player.SetWordWrite();			// Set RW to write: ir_dr16(IR_CNTRL_SIG_16BIT, 0x2408);
	g_Player.itf_->OnIrShift(IR_DATA_QUICK);
	for (uint32_t i = 0; i < word_count; i++)
	{
		g_Player.itf_->OnDrShift16(buf[i]);				// Shift in the write data
		g_Player.itf_->OnPulseTclk();	// Increment PC by 2
	}
	g_Player.ReleaseCpu();
	return true;
}


// Source: slau320aj
bool TapDev430::WriteFlash(address_t address, const uint16_t *buf, uint32_t word_count)
{
	address_t addr = address;				// Address counter
	if (!HaltCpu())
		return false;

#if 0
	ClrTCLK();
	IR_Shift(IR_CNTRL_SIG_16BIT);
	DR_Shift16(0x2408);			// Set RW to write
	IR_Shift(IR_ADDR_16BIT);
	DR_Shift16(0x0128);			// FCTL1 register
	IR_Shift(IR_DATA_TO_ADDR);
	DR_Shift16(0xA540);			// Enable FLASH write
	SetTCLK();

	ClrTCLK();
	IR_Shift(IR_ADDR_16BIT);
	DR_Shift16(0x012A);			// FCTL2 register
	IR_Shift(IR_DATA_TO_ADDR);
	DR_Shift16(0xA540);			// Select MCLK as source, DIV=1
	SetTCLK();

	ClrTCLK();
	IR_Shift(IR_ADDR_16BIT);
	DR_Shift16(0x012C);			// FCTL3 register
	IR_Shift(IR_DATA_TO_ADDR);
	DR_Shift16(Fctl3Unlock);	// Clear FCTL3; F2xxx: Unlock Info-Seg.
								// A by toggling LOCKA-Bit if required,
	SetTCLK();

	ClrTCLK();
	IR_Shift(IR_CNTRL_SIG_16BIT);
#else
	static constexpr TapStep steps_01[] =
	{
		kTclk0
		, kIrDr16(IR_CNTRL_SIG_16BIT, 0x2408)		// Set RW to write
		, kIrDr16(IR_ADDR_16BIT, 0x0128)			// FCTL1 register
		, kIrDr16(IR_DATA_TO_ADDR, 0xA540)			// Enable FLASH write
		, kTclk1

		, kTclk0
		, kIrDr16(IR_ADDR_16BIT, 0x012A)			// FCTL2 register
		, kIrDr16(IR_DATA_TO_ADDR, 0xA540)			// Select MCLK as source, DIV=1
		, kTclk1

		, kTclk0
		, kIrDr16(IR_ADDR_16BIT, 0x012C)			// FCTL3 register
		, kIrDr16(IR_DATA_TO_ADDR, kFctl3Unlock)	// Clear FCTL3; F2xxx: Unlock Info-Seg.
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
		DR_Shift16(addr);				// Set address
		IR_Shift(IR_DATA_TO_ADDR);
		DR_Shift16(buf[i]);				// Set data
		itf_->OnPulseTclk();
		IR_Shift(IR_CNTRL_SIG_16BIT);
		DR_Shift16(0x2409);				// Set RW to read

		TCLKstrobes(35);		// Provide TCLKs, min. 33 for F149 and F449
								// F2xxx: 29 are ok
#else
		static constexpr TapStep steps_02[] =
		{
			kDr16(0x2408)							// Set RW to write
			, kIrDr16Argv(IR_ADDR_16BIT)			// Set address
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
	DR_Shift16(0x0128);			// FCTL1 register
	IR_Shift(IR_DATA_TO_ADDR);
	DR_Shift16(Fctl1Lock);		// Disable FLASH write
	SetTCLK();

	// set LOCK-Bits again
	ClrTCLK();
	IR_Shift(IR_ADDR_16BIT);
	DR_Shift16(0x012C);			// FCTL3 address
	IR_Shift(IR_DATA_TO_ADDR);
	DR_Shift16(Fctl3Lock);		// Lock Inf-Seg. A by toggling LOCKA and set LOCK again
	SetTCLK();
	ReleaseCpu();
#else
	static constexpr TapStep steps_03[] =
	{
		kIrDr16(IR_CNTRL_SIG_16BIT, 0x2408)		// Set RW to write
		, kIrDr16(IR_ADDR_16BIT, 0x0128)		// FCTL1 register
		, kIrDr16(IR_DATA_TO_ADDR, kFctl1Lock)	// Disable FLASH write
		, kTclk1

		, kTclk0
		, kIrDr16(IR_ADDR_16BIT, 0x012C)		// FCTL3 register
		// Lock Inf-Seg. A by toggling LOCKA and set LOCK again
		, kIrDr16(IR_DATA_TO_ADDR, kFctl3Lock)
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
bool TapDev430::EraseFlash(address_t address, const uint16_t fctl1, const uint16_t fctl3)
{
	uint32_t strobe_amount = 4820;			// default for Segment Erase
	uint32_t loop_cnt = 1;					// erase cycle repeating for Mass Erase

	if ((fctl1 == kMassEraseSlau049) || (fctl1 == kMainEraseSlau049))
	{
		if (g_TapMcu.IsFastFlash())
		{
			strobe_amount = 10600;		// Larger Flash memories require
		}
		else
		{
			strobe_amount = 5300;		// Larger Flash memories require
			loop_cnt = 19;				// additional cycles for erase.
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
		DR_Shift16(0x0128);			// FCTL1 address
		IR_Shift(IR_DATA_TO_ADDR);
		DR_Shift16(fctl1);		// Enable erase mode
		SetTCLK();

		ClrTCLK();
		IR_Shift(IR_ADDR_16BIT);
		DR_Shift16(0x012A);			// FCTL2 address
		IR_Shift(IR_DATA_TO_ADDR);
		DR_Shift16(0xA540);			// MCLK is source, DIV=1
		SetTCLK();

		ClrTCLK();
		IR_Shift(IR_ADDR_16BIT);
		DR_Shift16(0x012C);			// FCTL3 address
		IR_Shift(IR_DATA_TO_ADDR);
		DR_Shift16(fctl3);			// Clear FCTL3; F2xxx: Unlock Info-Seg. A by toggling LOCKA-Bit if required,
		SetTCLK();

		ClrTCLK();
		IR_Shift(IR_ADDR_16BIT);
		DR_Shift16(address);		// Set erase address
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
		DR_Shift16(0x0128);			// FCTL1 address
		IR_Shift(IR_DATA_TO_ADDR);
		DR_Shift16(Fctl1Lock);		// Disable erase
		SetTCLK();
#else
		static constexpr TapStep steps_01[] =
		{
			kTclk0
			, kIrDr16(IR_CNTRL_SIG_16BIT, 0x2408)
			, kIrDr16(IR_ADDR_16BIT, 0x0128)		// FCTL1 address
			, kIrDr16Argv(IR_DATA_TO_ADDR)			// Enable erase "fctl1"
			, kTclk1

			, kTclk0
			, kIrDr16(IR_ADDR_16BIT, 0x012A)		// FCTL2 address
			, kIrDr16(IR_DATA_TO_ADDR, 0xA540)		// MCLK is source, DIV=1
			, kTclk1

			, kTclk0
			, kIrDr16(IR_ADDR_16BIT, 0x012C)		// FCTL3 address
			, kIrDr16Argv(IR_DATA_TO_ADDR)			// Clear FCTL3; F2xxx: Unlock Info-Seg. A 
			, kTclk1								// by toggling LOCKA-Bit if required,

			, kTclk0
			, kIrDr16Argv(IR_ADDR_16BIT)			// Set erase "address"
			, kIrDr16(IR_DATA_TO_ADDR, 0x55AA)		// Dummy write to start erase
			, kTclk1								// by toggling LOCKA-Bit if required,

			, kTclk0
			, kIrDr16(IR_CNTRL_SIG_16BIT, 0x2409)	// Set RW to read
			, kStrobeTclkArgv						// Provide 'strobe_amount' TCLKs
			, kIrDr16(IR_CNTRL_SIG_16BIT, 0x2408)	// Set RW to write
			, kIrDr16(IR_ADDR_16BIT, 0x0128)		// FCTL1 address
			, kIrDr16(IR_DATA_TO_ADDR, kFctl1Lock)	// Disable erase
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
	DR_Shift16(0x012C);				// FCTL3 address
	IR_Shift(IR_DATA_TO_ADDR);
	DR_Shift16(Fctl3Lock);			// Lock Inf-Seg. A by toggling LOCKA (F2xxx) and set LOCK again
	SetTCLK();

	ReleaseCpu();
#else
	static constexpr TapStep steps_02[] =
	{
		kTclk0
		, kIrDr16(IR_ADDR_16BIT, 0x012C)		// FCTL3 address
		, kIrDr16(IR_DATA_TO_ADDR, kFctl3Lock)	// Lock Inf-Seg. A by toggling LOCKA (F2xxx) and set LOCK again
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
bool TapDev430::IsInstrLoad()
{
	if ((g_Player.GetCtrlSigReg()
		 & (CNTRL_SIG_INSTRLOAD | CNTRL_SIG_READ)) != (CNTRL_SIG_INSTRLOAD | CNTRL_SIG_READ))
	{
		return false;
	}
	return true;
}

// Source UIF
bool TapDev430::InstrLoad()
{
	unsigned short i = 0;

	g_Player.Play(kIrDr8(IR_CNTRL_SIG_LOW_BYTE, CNTRL_SIG_READ));
	g_Player.SetTCLK();

	for (i = 0; i < 10; i++)
	{
		if (IsInstrLoad())
			return true;
		g_Player.PulseTCLKN();
	}
	return false;
}

// Source UIF
uint16_t TapDev430::SyncJtag()
{
	unsigned short lOut = 0, i = 50;
	g_Player.SetJtagRunReadLegacy();
	do
	{
		lOut = g_Player.DR_Shift16(0x0000);
		if (!--i)
			return 0;
	}
	while (((lOut == 0xFFFF) || !(lOut & 0x0200)));
	return lOut;
}


// Source UIF
bool TapDev430::SyncJtagAssertPorSaveContext(CpuContext &ctx, const ChipProfile &prof)
{
	const uint16_t address = WDT_ADDR_CPU;
	uint16_t wdtval = WDT_HOLD;
	uint16_t ctl_sync = 0;

	ctx.is_running_ = false;

	// Sync the JTAG
	if (g_Player.SetJtagRunReadLegacy() != kMspStd)
		return false;

	uint16_t lOut = g_Player.GetCtrlSigReg();    // read control register once

	g_Player.SetTCLK();

	if (!(lOut & CNTRL_SIG_TCE))
	{
		// If the JTAG and CPU are not already synchronized ...
		// Initiate Jtag and CPU synchronization. Read/Write is under CPU control. Source TCLK via TDI.
		// Do not effect bits used by DTC (CPU_HALT, MCLKON).
		static constexpr TapStep steps_01[] =
		{
			// initiate CPU synchronization but release low byte of CNTRL sig register to CPU control
			kIrDr8(IR_CNTRL_SIG_HIGH_BYTE, (CNTRL_SIG_TAGFUNCSAT | CNTRL_SIG_TCE1 | CNTRL_SIG_CPU) >> 8)
			// Address Force Sync special handling
			// read access to EEM General Clock Control Register (GCLKCTRL)
			, kIrDr16(IR_EMEX_DATA_EXCHANGE, MX_GCLKCTRL + MX_READ)
			// read the content of GCLKCNTRL into lOut
			, kDr16_ret(0)
		};

		g_Player.Play(steps_01, _countof(steps_01), &lOut);
		// Set Force Jtag Synchronization bit in Emex General Clock Control register.
		lOut |= 0x0040;							// 0x0040 = FORCE_SYN

		// Stability improvement: should be possible to remove this, required only once at the beginning
		// write access to EEM General Clock Control Register (GCLKCTRL)
		g_Player.Play(kIrDr16(IR_EMEX_DATA_EXCHANGE, MX_GCLKCTRL + MX_WRITE));
		lOut = g_Player.DR_Shift16(lOut);		// write into GCLKCNTRL

		// Reset Force Jtag Synchronization bit in Emex General Clock Control register.
		lOut &= ~0x0040;
		// Stability improvement: should be possible to remove this, required only once at the beginning
		// write access to EEM General Clock Control Register (GCLKCTRL)
		g_Player.Play(kIrDr16(IR_EMEX_DATA_EXCHANGE, MX_GCLKCTRL + MX_WRITE));
		lOut = g_Player.DR_Shift16(lOut);		// write into GCLKCNTRL

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
	}// end of if(!(lOut & CNTRL_SIG_TCE))

	// TODO: This has something to do with (activationKey == 0x20404020)
	if (prof.slau_ == ChipInfoDB::kSLAU335)
	{
		// here we add bit de assert bit 7 in JTAG test reg to enable clocks again
		lOut = g_Player.Play(kIrDr8(IR_TEST_REG, 0));
		lOut |= 0x80; //DE_ASSERT_BSL_VALID;
		g_Player.Play(kIrDr8(IR_TEST_REG, lOut));	// Bit 7 is de asserted now
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
			kIrDr16(IR_EMEX_DATA_EXCHANGE, MX_GENCNTRL + MX_WRITE)		// write access to EEM General Control Register (MX_GENCNTRL)
			, kDr16(EMU_FEAT_EN | EMU_CLK_EN | CLEAR_STOP | EEM_EN)		// write into MX_GENCNTRL
			// Stability improvement: should be possible to remove this, required only once at the beginning
			, kIrDr16(IR_EMEX_DATA_EXCHANGE, MX_GENCNTRL + MX_WRITE)	// write access to EEM General Control Register (MX_GENCNTRL)
			, kDr16(EMU_FEAT_EN | EMU_CLK_EN)							// write into MX_GENCNTRL
		};
		g_Player.Play(steps, _countof(steps));
	}
	else if (prof.clk_ctrl_ == ChipInfoDB::kGccStandardI)
	{
		static constexpr TapStep steps[] =
		{
			kIrDr16(IR_EMEX_DATA_EXCHANGE, MX_GENCNTRL + MX_WRITE)		// write access to EEM General Control Register (MX_GENCNTRL)
			, kDr16(EMU_FEAT_EN)										// write into MX_GENCNTRL

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
		lOut = g_Player.Play(kIrDr16(IR_ADDR_CAPTURE, 0x0000));
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
	ctx.pc_ = g_Player.Play(kIrDr16(IR_ADDR_CAPTURE, 0));

	// set PC to a save address pointing to ROM to avoid RAM corruption on certain devices
	SetPC(ROM_ADDR);

	// read status register
	ctx.sr_ = GetReg(2);

	return true;
}

// Source: slau320aj
bool TapDev430::ExecutePOR()
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

	TapDev430::WriteWord(0x0120, 0x5A80);	// Disable Watchdog on target device

	if (jtag_ver != kMspStd)
		return false;
	return true;
}



/**************************************************************************************/
/* MCU VERSION-RELATED DEVICE RELEASE                                                 */
/**************************************************************************************/

// Source: slau320aj
void TapDev430::ReleaseDevice(address_t address)
{
	switch (address)
	{
	case V_RUNNING: /* Nothing to do */
		break;
	case V_BOR:
	case V_RESET: /* Perform reset */
		/* issue reset */
		g_Player.Play(kIrDr16(IR_CNTRL_SIG_16BIT, 0x2C01));
		g_Player.itf_->OnDrShift16(0x2401);
		break;
	default: /* Set target CPU's PC */
		SetPC(address);
		break;
	}

	SetInstructionFetch();

#if 0
	itf_->OnIrShift(IR_EMEX_DATA_EXCHANGE);
	itf_->OnDrShift16(BREAKREACT + READ);
	itf_->OnDrShift16(0x0000);

	itf_->OnIrShift(IR_EMEX_WRITE_CONTROL);
	itf_->OnDrShift16(0x000f);

	itf_->OnIrShift(IR_CNTRL_SIG_RELEASE);
#else
	static constexpr TapStep steps[] =
	{
		kIrDr16(IR_EMEX_DATA_EXCHANGE, BREAKREACT + READ)
		, kDr16(0x0000)
		, kIrDr16(IR_EMEX_WRITE_CONTROL, 0x000f)
		, kIr(IR_CNTRL_SIG_RELEASE)
	};
	g_Player.Play(steps, _countof(steps));
#endif
}




/**************************************************************************************/
/* SUPPORT METHODS                                                                    */
/**************************************************************************************/

/*!
Set target CPU JTAG state machine into the instruction fetch state

\return: 1 - instruction fetch was set; 0 - otherwise
*/
JtagId TapDev430::SetInstructionFetch()
{
	for (int retries = 5; retries > 0; --retries)
	{
		// JTAG mode + CPU run + read
		JtagId jtag_id = (JtagId)g_Player.SetJtagRunReadLegacy();
		/* Wait until CPU is in instruction fetch state
		 * timeout after limited attempts
		 */
		for (int loop_counter = 30; loop_counter > 0; loop_counter--)
		{
			if ((g_Player.itf_->OnDrShift16(0x0000) & 0x0080) == 0x0080)
				return jtag_id;
			/*
			The TCLK pulse before OnDrShift16 leads to problems at MEM_QUICK_READ,
			it's from SLAU265
			*/
			g_Player.itf_->OnPulseTclkN();
		}
	}

	Error() << "SetInstructionFetch: failed\n";
	g_TapMcu.failed_ = true;

	return kInvalid;
}


/*!
Set the CPU into a controlled stop state
*/
bool TapDev430::HaltCpu()
{
	g_TapMcu.failed_ = false;
	/* Set CPU into instruction fetch mode */
	if (SetInstructionFetch() == kInvalid)
		return false;
#if 0
	/* Set device into JTAG mode + read */
	itf_->OnIrShift(IR_CNTRL_SIG_16BIT);
	itf_->OnDrShift16(0x2401);

	/* Send JMP $ instruction to keep CPU from changing the state */
	itf_->OnIrShift(IR_DATA_16BIT);
	itf_->OnDrShift16(0x3FFF);
	itf_->OnClearTclk();

	/* Set JTAG_HALT bit */
	itf_->OnIrShift(IR_CNTRL_SIG_16BIT);
	itf_->OnDrShift16(0x2409);
	itf_->OnSetTclk();
#else
	static const TapStep steps[] =
	{
		/* Set device into JTAG mode + read */
		kIrDr16(IR_CNTRL_SIG_16BIT, 0x2401)
		/* Send JMP $ instruction to keep CPU from changing the state */
		, kIrDr16(IR_DATA_16BIT, 0x3FFF)
		, kTclk0
		/* Set JTAG_HALT bit */
		, kIrDr16(IR_CNTRL_SIG_16BIT, 0x2409)
		, kTclk1
	};
	g_Player.Play(steps, _countof(steps));
#endif
	return true;
}


bool TapDev430::GetDevice(CoreId &coreid)
{
	coreid.id_data_addr_ = 0x0FF0;
	coreid.coreip_id_ = 0;
	coreid.ip_pointer_ = 0;
	// JTAG mode + CPU run + read
	if (g_Player.SetJtagRunReadLegacy() != kMspStd)
		goto error_exit;

	unsigned int loop_counter;
	for (loop_counter = 50; loop_counter > 0; loop_counter--)
	{
		if ((g_Player.itf_->OnDrShift16(0x0000) & 0x0200) == 0x0200)
			break;
	}

	if (loop_counter == 0)
	{
		Error() << "TapDev430::GetDevice: timed out\n";
error_exit:
		g_TapMcu.failed_ = true;
		/* timeout reached */
		return kInvalid;
	}
	coreid.device_id_ = TapDev430::ReadWord(0x0FF0);
	return true;
}

