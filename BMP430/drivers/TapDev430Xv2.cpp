#include "stdproj.h"

#include "TapDev430Xv2.h"
#include "TapMcu.h"
#include "eem_defs.h"



/**************************************************************************************/
/* MCU VERSION-RELATED REGISTER GET/SET METHODS                                       */
/**************************************************************************************/

//----------------------------------------------------------------------------
//! \brief Load a given address into the target CPU's program counter (PC).
//! \param[in] uint32_t address (destination address)
//! 
//!  Source: slau320aj
bool TapDev430Xv2::SetPC(address_t address)
{
	const uint16_t Mova = 0x0080
		| (uint16_t)((address >> 8) & 0x00000F00);
	const uint16_t Pc_l = (uint16_t)((address & 0xFFFF));

	// Check Full-Emulation-State at the beginning
	if (g_Player.GetCtrlSigReg() & 0x0301)
	{
#if 0
		// MOVA #imm20, PC
		ClrTCLK();
		// take over bus control during clock LOW phase
		IR_Shift(IR_DATA_16BIT);
		SetTCLK();
		DR_Shift16(Mova);
		ClrTCLK();
		// above is just for delay
		IR_Shift(IR_CNTRL_SIG_16BIT);
		DR_Shift16(0x1400);
		IR_Shift(IR_DATA_16BIT);
		itf_->OnPulseTclkN();			// F2xxx
		DR_Shift16(Pc_l);
		itf_->OnPulseTclkN();			// F2xxx
		DR_Shift16(0x4303);				// NOP
		ClrTCLK();
		IR_Shift(IR_ADDR_CAPTURE);
		DR_Shift20(0x00000);
#else
		static const TapStep steps[] =
		{
			// MOVA #imm20, PC
			kTclk0
			// take over bus control during clock LOW phase
			, kIr(IR_DATA_16BIT)
			, kTclk1
			, kDr16Argv				// Mova
			, kTclk0
			// above is just for delay
			, kIrDr16(IR_CNTRL_SIG_16BIT, 0x1400)
			, kIr(IR_DATA_16BIT)
			, kPulseTclkN			// F2xxx
			, kDr16Argv				// Pc_l
			, kPulseTclkN			// F2xxx
			, kDr16(0x4303)			// NOP
			, kTclk0
			, kIrDr20(IR_ADDR_CAPTURE, 0)
		};
		g_Player.Play(steps, _countof(steps)
			 , Mova
			 , Pc_l
		);
#endif
	}
	return true;
}


// Source:  uif
bool TapDev430Xv2::SetReg(uint8_t reg, uint32_t value)
{
	uint16_t mova = 0x0080;
	mova += (uint16_t)((value >> 8) & 0x00000F00);
	mova += (reg & 0x000F);
	uint16_t rx_l = (uint16_t)value;

#if 0
	IHIL_Tclk(0);
	data_16bit();
	IHIL_Tclk(1);
	SetReg_16Bits(mova);
	cntrl_sig_16bit();
	SetReg_16Bits(0x1401);
	data_16bit();
	itf_->OnPulseTclkN();
	SetReg_16Bits(rx_l);
	itf_->OnPulseTclkN();
	SetReg_16Bits(0x3ffd);
	itf_->OnPulseTclkN();
	IHIL_Tclk(0);
	addr_capture();
	SetReg_20Bits(0x00000);
	IHIL_Tclk(1);
	cntrl_sig_16bit();
	SetReg_16Bits(0x0501);
	itf_->OnPulseTclkN();
	IHIL_Tclk(0);
	data_capture();
	IHIL_Tclk(1);
#else
	static constexpr TapStep steps[] =
	{
		kTclk0
		, kIr(IR_DATA_16BIT)
		, kTclk1
		, kDr16Argv				// mova
		, kIrDr16(IR_CNTRL_SIG_16BIT, 0x1401)
		, kIr(IR_DATA_16BIT)
		, kPulseTclkN
		, kDr16Argv				// rx_l
		, kPulseTclkN
		, kDr16(0x3ffd)
		, kPulseTclkN
		, kTclk0
		, kIr(IR_DATA_CAPTURE)
		, kDr20(0x00000)
		, kTclk1
		, kIrDr16(IR_CNTRL_SIG_16BIT, 0x0501)
		, kPulseTclkN
		, kTclk0
		, kIr(IR_DATA_CAPTURE)
		, kTclk1
	};
	g_Player.Play(steps, _countof(steps)
		 , mova
		 , rx_l
	);
#endif
	return true;
}


bool TapDev430Xv2::StartGetRegs()
{
	back_r0_ = GetRegInternal(0);
	return true;
}


// Source: uif
uint32_t TapDev430Xv2::GetRegInternal(uint8_t reg)
{
	const uint16_t Mova = 0x0060
		| ((uint16_t)reg << 8) & 0x0F00;

	JtagId jtagId = (JtagId)(g_Player.itf_->OnIrShift(IR_CNTRL_SIG_CAPTURE));
	const uint16_t jmbAddr = (jtagId == kMsp_98)
		? 0x14c							// SYSJMBO0 on low density MSP430FR2xxx
		: 0x18c;						// SYSJMBO0 on most high density parts

	uint16_t Rx_l = 0xFFFF;
	uint16_t Rx_h = 0xFFFF;
	static constexpr TapStep steps[] =
	{
		kTclk0
		, kIr(IR_DATA_16BIT)
		, kTclk1
		, kDr16Argv								// dr16(Mova)
		, kIrDr16(IR_CNTRL_SIG_16BIT, 0x1401)	// RD + JTAGCTRL + RELEASE_LBYTE:01
		, kIr(IR_DATA_16BIT)
		, kPulseTclkN
		, kDr16Argv								// dr16(jmbAddr)
		, kPulseTclkN
		, kDr16(0x3ffd)
		, kTclk0
		, kIr(IR_DATA_CAPTURE)
		, kTclk1
		, kDr16_ret(0)							// Rx_l = dr16(0)
		, kPulseTclkN
		, kDr16_ret(0)							// Rx_h = dr16(0)
		, kPulseTclkN
		, kPulseTclkN
		, kPulseTclkN
		, TapPlayer::kSetWordReadXv2_			// Set Word read CpuXv2
		, kTclk0
		, kIr(IR_DATA_CAPTURE)
		, kTclk1
	};
	g_Player.Play(steps, _countof(steps)
		 , Mova
		 , jmbAddr
		 , &Rx_l
		 , &Rx_h
	);
	g_Player.itf_->OnReadJmbOut();

	return (((uint32_t)Rx_h << 16) | Rx_l) & 0xfffff;
}


uint32_t TapDev430Xv2::GetReg(uint8_t reg)
{
	if (reg == 0)
		return back_r0_;
	return GetRegInternal(reg);
}


void TapDev430Xv2::StopGetRegs()
{
	SetPC(back_r0_ - 4);
}



/**************************************************************************************/
/* MCU VERSION-RELATED READ MEMORY METHODS                                            */
/**************************************************************************************/

// Source: slau320aj
uint16_t TapDev430Xv2::ReadWord(address_t address)
{
#if 0
	// Reference: Slau320aj
	itf_->OnClearTclk();
	itf_->OnIrShift(IR_CNTRL_SIG_16BIT);
	/* set word read */
	itf_->OnDrShift16(0x0501);
	/* set address */
	itf_->OnIrShift(IR_ADDR_16BIT);
	itf_->OnDrShift20(address);
	itf_->OnIrShift(IR_DATA_TO_ADDR);
	// Delay cause a previous memory access
	while (!MicroDelay::HasEllapsed())
	{
	}
	// ARM processor is too fast. avoid bus saturation by forcing a delay
	MicroDelay::StartShot(kMinFlashPeriod);
	itf_->OnPulseTclk();
	/* shift out 16 bits */
	//itf_->OnDrShift16(0x0000);
	uint16_t content = itf_->OnDrShift16(0x0000);
	itf_->OnPulseTclk();
	itf_->OnSetTclk(); /* is also the first instruction in ReleaseCpu() */
#else
	static constexpr TapStep steps[] =
	{
		kTclk0
		// set word read
		, kIrDr16(IR_CNTRL_SIG_16BIT, 0x0501)
		// Set address
		, kIrDr20Argv(IR_ADDR_16BIT)	// dr16(address)
		, kIr(IR_DATA_TO_ADDR)
		, kPulseTclk
		// shift out 16 bits
		, kDr16_ret(0x0000)				// content = dr16(0x0000)
		, kPulseTclk
		, kTclk1						// is also the first instruction in ReleaseCpu()
	};
	uint16_t content = 0xFFFF;
	g_Player.Play(steps, _countof(steps)
		 , address
		 , &content
	);
#endif
	return content;
}


//----------------------------------------------------------------------------
//! \brief This function reads an array of words from the memory.
//! \param[in] word address (Start address of memory to be read)
//! \param[in] word word_count (Number of words to be read)
//! \param[out] word *buf (Pointer to array for the data)
//! Source: slau320aj
bool TapDev430Xv2::ReadWords(address_t address, uint16_t *buf, uint32_t word_count)
{
	uint8_t jtag_id = g_Player.IR_Shift(IR_CNTRL_SIG_CAPTURE);

	// Set PC to 'safe' address
	address_t lPc = ((jtag_id == JTAG_ID99) || (jtag_id == JTAG_ID98))
		? 0x00000004
		: 0;

	TapDev430Xv2::SetPC(address);

	static constexpr TapStep steps[] =
	{
		kTclk1
		// set word read
		, kIrDr16(IR_CNTRL_SIG_16BIT, 0x0501)
		// Set address
		, kIr(IR_ADDR_CAPTURE)
		, kIr(IR_DATA_QUICK)
	};
	g_Player.Play(steps, _countof(steps));

	for (uint32_t i = 0; i < word_count; ++i)
	{
		g_Player.itf_->OnPulseTclk();
		*buf++ = g_Player.DR_Shift16(0);  // Read data from memory.         
	}

	if (lPc)
		TapDev430Xv2::SetPC(lPc);
	g_Player.SetTCLK();
	return true;
}


/**************************************************************************************/
/* EXPERIMENTAL METHOD                                                                */
/**************************************************************************************/

// Source: uif
void TapDev430Xv2::ReadWordsXv2_uif(address_t address, uint16_t *buf, uint32_t len)
{
	// SET PROGRAM COUNTER for QUICK ACCESS
	TapDev430Xv2::SetPC(address);
#if 0
	cntrl_sig_16bit();
	SetReg_16Bits(0x0501);
#else
	g_Player.SetWordReadXv2();			// Set Word read CpuXv2
#endif
	g_Player.IHIL_Tclk(1);
	g_Player.addr_capture();
	// END OF SETTING THE PROGRAM COUNTER
	g_Player.data_quick();

	for (uint32_t i = 0; i < len; ++i)
	{
		g_Player.itf_->OnPulseTclk();
		*buf++ = g_Player.SetReg_16Bits(0);
	}
	// Check save State
	g_Player.cntrl_sig_capture();
	g_Player.SetReg_16Bits(0x0000);

	// SET PROGRAM COUNTER for Backup
	TapDev430Xv2::SetPC(SAFE_PC_ADDRESS);
#if 0
	cntrl_sig_16bit();
	SetReg_16Bits(0x0501);
#else
	g_Player.SetWordReadXv2();			// Set Word read CpuXv2
#endif
	g_Player.IHIL_Tclk(1);
	g_Player.addr_capture();
}



/**************************************************************************************/
/* MCU VERSION-RELATED WRITE MEMORY METHODS                                           */
/**************************************************************************************/

//----------------------------------------------------------------------------
//! \brief This function writes one byte/word at a given address ( <0xA00)
//! \param[in] word address (Address of data to be written)
//! \param[in] word data (shifted data)
//! Source: slau320aj
bool TapDev430Xv2::WriteWord(address_t address, uint16_t data)
{
	// Check Init State at the beginning
	if (g_Player.GetCtrlSigReg() & 0x0301)
	{
#if 0
		ClrTCLK();
		IR_Shift(IR_CNTRL_SIG_16BIT);
		DR_Shift16(0x0500);
		IR_Shift(IR_ADDR_16BIT);
		DR_Shift20(address);

		SetTCLK();
		// New style: Only apply data during clock high phase
		IR_Shift(IR_DATA_TO_ADDR);
		DR_Shift16(data);           // Shift in 16 bits
		ClrTCLK();
		IR_Shift(IR_CNTRL_SIG_16BIT);
		DR_Shift16(0x0501);
		SetTCLK();
		// one or more cycle, so CPU is driving correct MAB
		itf_->OnPulseTclkN();			// F2xxx
#else
		static constexpr TapStep steps[] =
		{
			kTclk0
			// set word read
			, kIrDr16(IR_CNTRL_SIG_16BIT, 0x0500)
			// Set address
			, kIrDr20Argv(IR_ADDR_16BIT)		// dr16(address)
			, kTclk1
			// New style: Only apply data during clock high phase
			, kIrDr16Argv(IR_DATA_TO_ADDR)		// dr16(data)
			, kTclk0
			, kIrDr16(IR_CNTRL_SIG_16BIT, 0x0501)
			, kTclk1
			// one or more cycle, so CPU is driving correct MAB
			, kPulseTclkN
		};
		g_Player.Play(steps, _countof(steps)
			 , address
			 , data
		);
#endif
		// Processor is now again in Init State
		return true;
	}
	else
		return false;
}


//----------------------------------------------------------------------------
//! \brief This function writes an array of words into the target memory.
//! \param[in] word address (Start address of target memory)
//! \param[in] word word_count (Number of words to be programmed)
//! \param[in] word *buf (Pointer to array with the data)
//! Source: slau320aj
bool TapDev430Xv2::WriteWords(address_t address, const uint16_t *buf, uint32_t word_count)
{
	for (uint32_t i = 0; i < word_count; i++)
	{
		if (!TapDev430Xv2::WriteWord(address, *buf++))
			return false;
		address += 2;
	}
	return true;
}



/**************************************************************************************/
/* MCU VERSION-RELATED WRITE MEMORY METHODS                                           */
/**************************************************************************************/

//! \brief Set the start address of the device RAM
static constexpr uint32_t kRamStartAddress = 0x1C00;

static uint16_t FlashWrite_o[] =
{
	0x001C, 0x00EE, 0xBEEF, 0xDEAD, 0xBEEF, 0xDEAD, 0xA508, 0xA508, 0xA500,
	0xA500, 0xDEAD, 0x000B, 0xDEAD, 0x000B, 0x40B2, 0x5A80, 0x015C, 0x40B2,
	0xABAD, 0x018E, 0x40B2, 0xBABE, 0x018C, 0x4290, 0x0140, 0xFFDE, 0x4290,
	0x0144, 0xFFDA, 0x180F, 0x4AC0, 0xFFD6, 0x180F, 0x4BC0, 0xFFD4, 0xB392,
	0x0144, 0x23FD, 0x4092, 0xFFBE, 0x0144, 0x4290, 0x0144, 0xFFB8, 0x90D0,
	0xFFB2, 0xFFB2, 0x2406, 0x401A, 0xFFAA, 0xD03A, 0x0040, 0x4A82, 0x0144,
	0x1F80, 0x405A, 0xFF94, 0x1F80, 0x405B, 0xFF92, 0x40B2, 0xA540, 0x0140,
	0x40B2, 0x0050, 0x0186, 0xB392, 0x0186, 0x27FD, 0x429A, 0x0188, 0x0000,
	0xC392, 0x0186, 0xB392, 0x0144, 0x23FD, 0x1800, 0x536A, 0x1800, 0x835B,
	0x23F0, 0x1F80, 0x405A, 0xFF6C, 0x1F80, 0x405B, 0xFF6A, 0xE0B0, 0x3300,
	0xFF5C, 0xE0B0, 0x3300, 0xFF58, 0x4092, 0xFF52, 0x0140, 0x4092, 0xFF4E,
	0x0144, 0x4290, 0x0144, 0xFF42, 0x90D0, 0xFF42, 0xFF3C, 0x2406, 0xD0B0,
	0x0040, 0xFF38, 0x4092, 0xFF34, 0x0144, 0x40B2, 0xCAFE, 0x018E, 0x40B2,
	0xBABE, 0x018C, 0x3FFF,
};


// Source: slau320aj
bool TapDev430Xv2::WriteFlash(address_t address, const uint16_t *data, uint32_t word_count)
{
	//! \brief Holds the target code for an flash write operation
//! \details This code is modified by the flash write function depending on it's parameters.

	address_t load_addr = kRamStartAddress;			// RAM start address specified in config header file
	address_t start_addr = load_addr + FlashWrite_o[0];	// start address of the program in target RAM

	FlashWrite_o[2] = (uint16_t)(address);				// set write start address
	FlashWrite_o[3] = (uint16_t)(address >> 16);
	FlashWrite_o[4] = (uint16_t)(word_count);			// set number of words to write
	FlashWrite_o[5] = (uint16_t)(word_count >> 16);
	FlashWrite_o[6] = kFctl3Unlock_Xv2;					// FCTL3: lock/unlock INFO Segment A
														// default = locked

	TapDev430Xv2::WriteWords(load_addr, FlashWrite_o, _countof(FlashWrite_o));
	TapDev430Xv2::ReleaseDevice(start_addr);

	{
		uint32_t Jmb = 0;
		uint32_t Timeout = 0;

		do
		{
			Jmb = g_Player.i_ReadJmbOut();
			Timeout++;
		}
		while (Jmb != 0xABADBABE && Timeout < 3000);

		if (Timeout < 3000)
		{
			uint32_t i;

			for (i = 0; i < word_count; i++)
			{
				g_Player.i_WriteJmbIn16(data[i]);
				//usDelay(100);				// delay 100us  - added by GC       
			}
		}
	}
	{
		uint32_t Jmb = 0;
		uint32_t Timeout = 0;

		do
		{
			Jmb = g_Player.i_ReadJmbOut();
			Timeout++;
		}
		while (Jmb != 0xCAFEBABE && Timeout < 3000);
	}

	//TapDev430Xv2::SyncJtag();

	// clear RAM here - init with JMP $
	{
		for (uint32_t i = 0; i < _countof(FlashWrite_o); i++)
		{
			TapDev430Xv2::WriteWord(load_addr, 0x3fff);
			load_addr += 2;
		}
	}
	return true;
}



/**************************************************************************************/
/* MCU VERSION-RELATED FLASH ERASE                                                    */
/**************************************************************************************/


static uint16_t FlashErase_o[] =
{
	0x0016, 0x00B0, 0xBEEF, 0xDEAD, 0xA502, 0xA508, 0xA508, 0xA500, 0xA500,
	0xDEAD, 0x000B, 0x40B2, 0x5A80, 0x015C, 0x4290, 0x0140, 0xFFEE, 0x4290,
	0x0144, 0xFFEA, 0x180F, 0x4AC0, 0xFFE6, 0xB392, 0x0144, 0x23FD, 0x4092,
	0xFFD4, 0x0144, 0x4290, 0x0144, 0xFFCE, 0x90D0, 0xFFC8, 0xFFC8, 0x2406,
	0x401A, 0xFFC0, 0xD03A, 0x0040, 0x4A82, 0x0144, 0x1F80, 0x405A, 0xFFAC,
	0x4092, 0xFFAC, 0x0140, 0x40BA, 0xDEAD, 0x0000, 0xB392, 0x0144, 0x23FD,
	0x1F80, 0x405A, 0xFFA2, 0xE0B0, 0x3300, 0xFF98, 0xE0B0, 0x3300, 0xFF94,
	0x4092, 0xFF8E, 0x0140, 0x4092, 0xFF8A, 0x0144, 0x4290, 0x0144, 0xFF7E,
	0x90D0, 0xFF7E, 0xFF78, 0x2406, 0xD0B0, 0x0040, 0xFF74, 0x4092, 0xFF70,
	0x0144, 0x40B2, 0xCAFE, 0x018E, 0x40B2, 0xBABE, 0x018C, 0x3FFF,
};


// Source: slau320aj
bool TapDev430Xv2::EraseFlash(address_t address, const uint16_t fctl1, const uint16_t fctl3)
{
	address_t loadAddr = kRamStartAddress;			// RAM start address specified in config header file
	address_t startAddr = loadAddr + FlashErase_o[0];	// start address of the program in target RAM

	FlashErase_o[2] = (uint16_t)(address);			// set dummy write address
	FlashErase_o[3] = (uint16_t)(address >> 16);
	FlashErase_o[4] = fctl1;						// set erase mode
	FlashErase_o[5] = fctl3;						// FCTL3: lock/unlock INFO Segment A
													// default = locked

	TapDev430Xv2::WriteWords(loadAddr, (uint16_t *)FlashErase_o, _countof(FlashErase_o));
	TapDev430Xv2::ReleaseDevice(startAddr);

	{
		unsigned long Jmb = 0;
		unsigned long Timeout = 0;

		do
		{
			Jmb = g_Player.i_ReadJmbOut();
			Timeout++;
		}
		while (Jmb != 0xCAFEBABE && Timeout < 3000);
	}

	//TapDev430Xv2::SyncJtag();

	// clear RAM here - init with JMP $
	{
		for (uint32_t i = 0; i < _countof(FlashErase_o); i++)
		{
			TapDev430Xv2::WriteWord(loadAddr, 0x3fff);
			loadAddr += 2;
		}
	}
	return true;
}




/**************************************************************************************/
/* MCU VERSION-RELATED POWER ON RESET                                                 */
/**************************************************************************************/

bool TapDev430Xv2::GetDevice(CoreId &core_id)
{
	core_id.id_data_addr_ = 0x0FF0;
	assert(core_id.IsXv2());
	// Get Core identification info
	core_id.coreip_id_ = g_Player.Play(kIrDr16(IR_COREIP_ID, 0));
	if (core_id.coreip_id_ == 0)
	{
		Error() << "TapDev::GetDeviceXv2: invalid CoreIP ID\n";
		g_TapMcu.failed_ = true;
		/* timeout reached */
		return false;
	}
	// Get device identification pointer
	if (core_id.jtag_id_ == kMsp_95)
		StopWatch().Delay(1500);
	g_Player.IR_Shift(IR_DEVICE_ID);
	uint32_t tmp = g_Player.SetReg_20Bits(0);
	// The ID pointer is an un-scrambled 20bit value
	core_id.ip_pointer_ = ((tmp & 0xFFFF) << 4) + (tmp >> 16);
	if (core_id.ip_pointer_ && (core_id.ip_pointer_ & 1) == 0)
	{
		core_id.id_data_addr_ = core_id.ip_pointer_ + 4;
	}
	return true;
}


#if 0
//----------------------------------------------------------------------------
//! \brief Function to resync the JTAG connection and execute a Power-On-Reset
//! \return true if operation was successful, false otherwise)
bool TapDev430Xv2::SyncJtag()
{
	uint32_t i = 0;

	g_Player.Play(kIrDr16(IR_CNTRL_SIG_16BIT, 0x1501));  // Set device into JTAG mode + read

	uint8_t jtag_id = g_Player.IR_Shift(IR_CNTRL_SIG_CAPTURE);

	if ((jtag_id != JTAG_ID91) && (jtag_id != JTAG_ID99))
	{
		return false;
	}
	// wait for sync
	while (!(g_Player.DR_Shift16(0) & 0x0200) && i < 50)
	{
		i++;
	};
	// continues if sync was successful
	if (i >= 50)
		return false;

	// execute a Power-On-Reset
	if (TapDev430Xv2::ExecutePOR() == false)
		return false;

	return true;
}
#endif

bool TapDev430Xv2::WaitForSynch()
{
	g_Player.IR_Shift(IR_CNTRL_SIG_CAPTURE);
	uint16_t i = 50;
	do
	{
		if ((g_Player.DR_Shift16(0) & 0x0200) != 0)
			return true;
	}
	while (--i > 0);
	return false;
}


// Source UIF
bool TapDev430Xv2::SyncJtagAssertPorSaveContext(CpuContext &ctx, const ChipProfile &prof)
{
	uint16_t address = g_TapMcu.IsFr41xx() ? WDT_ADDR_FR41XX : WDT_ADDR_XV2;
	uint16_t wdtval = WDT_HOLD;

	if (ctx.jtag_id_ == kMsp_99)
	{
		g_Player.Play(kIrDr16(IR_TEST_3V_REG, 0x40A0));
		g_Player.IR_Shift(IR_TEST_REG);
		g_Player.DR_Shift32(0x00010000);
	}
	// -------------------------Power mode handling end ------------------------

	static constexpr TapStep steps_01[] =
	{
		// enable clock control before sync
		// switch all functional clocks to JCK = 1 and stop them
		kIrDr32(IR_EMEX_DATA_EXCHANGE32, GENCLKCTRL + WRITE),
		kDr32(MCLK_SEL3 + SMCLK_SEL3 + ACLK_SEL3 + STOP_MCLK + STOP_SMCLK + STOP_ACLK),
		// enable Emulation Clocks
		kIrDr16(IR_EMEX_WRITE_CONTROL, EMU_CLK_EN + EEM_EN),

		// release RW and BYTE control signals in low byte, set TCE1 & CPUSUSP(!!) & RW
		kIrDr16(IR_CNTRL_SIG_16BIT, 0x1501),
	};
	g_Player.Play(steps_01, _countof(steps_01));

	if (!WaitForSynch())
		return false;

	static constexpr TapStep steps_02[] =
	{
		// provide one more clock to empty the pipe
		kPulseTclkN,
		// release CPUFLUSH(=CPUSUSP) signal and apply POR signal
		kIrDr16(IR_CNTRL_SIG_16BIT, 0x0C01),
		kDelay1ms(40),
		// release POR signal again
		kDr16(0x0401),	// disable fetch of CPU // changed from 401 to 501
	};
	g_Player.Play(steps_02, _countof(steps_02));
	if (ctx.jtag_id_ == kMsp_91)
	{
		static constexpr TapStep steps[] =
		{
			// Force PC so safe value memory location, JMP $
			kIr(IR_DATA_16BIT)
			, kPulseTclkN
			, kPulseTclkN
			, kDr16(SAFE_PC_ADDRESS)
			// drive safe address into pc end
			, kPulseTclkN
			, kPulseTclkN	// required for version 91 only
			, kIr(IR_DATA_CAPTURE)
		};
		g_Player.Play(steps, _countof(steps));
	}
	else if (ctx.jtag_id_ == kMsp_98
				|| ctx.jtag_id_ == kMsp_99)
	{
		static constexpr TapStep steps[] =
		{
			// Force PC so safe value memory location, JMP $
			kIr(IR_DATA_16BIT)
			, kPulseTclkN
			, kPulseTclkN
			, kDr16(SAFE_PC_ADDRESS)
			// drive safe address into pc end
			, kPulseTclkN
			, kIr(IR_DATA_CAPTURE)
		};
		g_Player.Play(steps, _countof(steps));
	}
	else
	{
		static constexpr TapStep steps[] =
		{
			kPulseTclkN
			, kPulseTclkN
			, kPulseTclkN
		};
		g_Player.Play(steps, _countof(steps));
	}

	static constexpr TapStep steps_03[] =
	{
		// TWO more to release CPU internal POR delay signals
		kPulseTclkN
		, kPulseTclkN
		// set CPUFLUSH signal
		, kIrDr16(IR_CNTRL_SIG_16BIT, 0x0501)
		, kPulseTclkN
		// set EEM FEATURE enable now!!!
		, kIrDr16(IR_EMEX_WRITE_CONTROL, EMU_FEAT_EN + EMU_CLK_EN + CLEAR_STOP)
		// Check that sequence exits on Init State
		, kIrDr16(IR_CNTRL_SIG_CAPTURE, 0x0000)
	};
	g_Player.Play(steps_03, _countof(steps_03));

	// hold Watchdog Timer
	ctx.wdt_ = ReadWord(address);
	wdtval |= ctx.wdt_;				// set original bits in addition to stop bit
	WriteWord(address, wdtval);

	// Capture MAB - the actual PC value is (MAB - 4)
	ctx.pc_ = g_Player.Play(kIrDr20(IR_ADDR_CAPTURE, 0));

	/*****************************************/
	/* Note 1495, 1637 special handling      */
	/*****************************************/
	if (((ctx.pc_ & 0xFFFF) == 0xFFFE) 
		|| (ctx.jtag_id_ == kMsp_91)
		|| (ctx.jtag_id_ == kMsp_98)
		|| (ctx.jtag_id_ == kMsp_99))
	{
		ctx.pc_ = ReadWord(0xFFFE);
	}
	else
	{
		ctx.pc_ -= 4;
	}
	// Status Register should be always 0 after a POR
	ctx.sr_ = 0;


	if (WaitForSynch())
	{
		const ChipInfoDB::EemTimer *eem = prof.eem_timers_;
		if (eem != NULL)
		{
			/*
			** Code refactored from MSPFET firmware. No documentation could be found
			** for ETKEYSEL / ETCLKSEL. Best source of information is 'modules.h'.
			*/
			while (!eem->IsEofMark())
			{
				// check if module clock control is enabled for corresponding module
				uint16_t v = (ctx.eem_mask_ & (1UL << eem->idx_)) != 0;
				WriteWord(ETKEYSEL, ETKEY | eem->value_);
				WriteWord(ETCLKSEL, v);
				++eem;
			}
		}
	}
	static constexpr TapStep steps_04[] =
	{
		// switch back system clocks to original clock source but keep them stopped
		kIrDr32(IR_EMEX_DATA_EXCHANGE32, GENCLKCTRL + WRITE)
		, kDr32(MCLK_SEL0 + SMCLK_SEL0 + ACLK_SEL0 + STOP_MCLK + STOP_SMCLK + STOP_ACLK)
	};
	g_Player.Play(steps_04, _countof(steps_04));

	// reset Vacant Memory Interrupt Flag inside SFRIFG1
	if (ctx.jtag_id_ == kMsp_91)
	{
		uint16_t specialFunc = ReadWord(0x0102);
		if (specialFunc & 0x8)
		{
			SetPC(SAFE_PC_ADDRESS);

			static constexpr TapStep steps_05[] =
			{
				kIrDr16(IR_CNTRL_SIG_16BIT, 0x0501)
				, kTclk1
				, kIr(IR_ADDR_CAPTURE)
			};
			g_Player.Play(steps_05, _countof(steps_05));

			specialFunc &= ~0x8;
			WriteWord(0x0102, specialFunc);
		}
	}
	return true; // return status OK
}


//----------------------------------------------------------------------------
//! \brief Function to execute a Power-On Reset (POR) using JTAG CNTRL SIG 
//! register
//! \return word (STATUS_OK if target is in Full-Emulation-State afterwards,
//! STATUS_ERROR otherwise)
//! Source: slau320aj
bool TapDev430Xv2::ExecutePOR()
{
#if 0
	uint16_t id = 0;

	id = IR_Shift(IR_CNTRL_SIG_CAPTURE);

	// provide one clock cycle to empty the pipe
	itf_->OnPulseTclkN();

	// prepare access to the JTAG CNTRL SIG register
	IR_Shift(IR_CNTRL_SIG_16BIT);
	// release CPUSUSP signal and apply POR signal
	DR_Shift16(0x0C01);
	// release POR signal again
	DR_Shift16(0x0401);

	itf_->OnPulseTclkN();
	itf_->OnPulseTclkN();
	itf_->OnPulseTclkN();

	// two more to release CPU internal POR delay signals
	itf_->OnPulseTclkN();
	itf_->OnPulseTclkN();

	// now set CPUSUSP signal again
#if 0
	IR_Shift(IR_CNTRL_SIG_16BIT);
	DR_Shift16(0x0501);
#else
	SetWordReadXv2();			// Set Word read CpuXv2
#endif
	// and provide one more clock
	itf_->OnPulseTclkN();
#else
	static constexpr TapStep steps[] =
	{
		kIr(IR_CNTRL_SIG_CAPTURE)
		// provide one clock cycle to empty the pipe
		, kPulseTclkN
		// prepare access to the JTAG CNTRL SIG register
		// release CPUSUSP signal and apply POR signal
		, kIrDr16(IR_CNTRL_SIG_16BIT, 0x0C01)
		// release POR signal again
		, kDr16(0x0401)
		, kPulseTclkN
		, kPulseTclkN
		, kPulseTclkN
		// two more to release CPU internal POR delay signals
		, kPulseTclkN
		, kPulseTclkN
		// now set CPUSUSP signal again
		, kIrDr16(IR_CNTRL_SIG_16BIT, 0x0501)
		// and provide one more clock
		, kPulseTclkN
	};
	g_Player.Play(steps, _countof(steps));
#endif
	// the CPU is now in 'Full-Emulation-State'

	// disable Watchdog Timer on target device now by setting the HOLD signal
	// in the WDT_CNTRL register
	TapDev430Xv2::WriteWord(0x015C, 0x5A80);

	// Check if device is in Full-Emulation-State again and return status
	if (g_Player.GetCtrlSigReg() & 0x0301)
		return true;

	return false;
}



/**************************************************************************************/
/* MCU VERSION-RELATED DEVICE RELEASE                                                 */
/**************************************************************************************/

// Source: slau320aj
void TapDev430Xv2::ReleaseDevice(address_t address)
{
	switch (address)
	{
	case V_BOR:
		// perform a BOR via JTAG - we loose control of the device then...
		g_Player.Play(kIrDr16(IR_TEST_REG, 0x0200));
		MicroDelay::Delay(5000);			// wait some time before doing any other action
		// JTAG control is lost now - GetDevice() needs to be called again to gain control.
		break;

	case V_RESET:
		g_Player.Play(kIrDr16(IR_CNTRL_SIG_16BIT, 0x0C01));	// Perform a reset
		g_Player.DR_Shift16(0x0401);
		g_Player.IR_Shift(IR_CNTRL_SIG_RELEASE);
		break;

	case V_RUNNING:
		g_Player.IR_Shift(IR_CNTRL_SIG_RELEASE);
		break;

	default:
		TapDev430Xv2::SetPC(address);	// Set target CPU's PC
		// prepare release & release
#if 0
		SetTCLK();
		IR_Shift(IR_CNTRL_SIG_16BIT);
		DR_Shift16(0x0401);
		IR_Shift(IR_ADDR_CAPTURE);
		IR_Shift(IR_CNTRL_SIG_RELEASE);
#else
		static constexpr TapStep steps[] =
		{
			kTclk1
			, kIrDr16(IR_CNTRL_SIG_16BIT, 0x0401)
			, kIr(IR_ADDR_CAPTURE)
			, kIr(IR_CNTRL_SIG_RELEASE)
		};
		g_Player.Play(steps, _countof(steps));
#endif
		break;
	}
}

