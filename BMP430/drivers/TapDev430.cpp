#include "stdproj.h"

#include "TapDev430.h"
#include "eem_defs.h"
#include "TapDev.h"


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
		if (g_JtagDev.IsFlastFlash())
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
		/* Set device into JTAG mode + read */
#if 0
		itf_->OnIrShift(IR_CNTRL_SIG_16BIT);
		itf_->OnDrShift16(0x2401);
#else
		g_Player.SetJtagRunRead();		// JTAG mode + CPU run + read
#endif

		JtagId jtag_id = (JtagId)g_Player.itf_->OnIrShift(IR_CNTRL_SIG_CAPTURE);
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
	g_JtagDev.failed_ = true;

	return kInvalid;
}


/*!
Set the CPU into a controlled stop state
*/
bool TapDev430::HaltCpu()
{
	g_JtagDev.failed_ = false;
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
	/* Set device into JTAG mode + read */
	g_Player.SetJtagRunRead();		// JTAG mode + CPU run + read

	if (g_Player.IR_Shift(IR_CNTRL_SIG_CAPTURE) != kMspStd)
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
		g_JtagDev.failed_ = true;
		/* timeout reached */
		return kInvalid;
	}
	coreid.device_id_ = TapDev430::ReadWord(0x0FF0);
	return true;
}

