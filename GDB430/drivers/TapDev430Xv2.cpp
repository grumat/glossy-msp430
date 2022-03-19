#include "stdproj.h"

#include "TapDev430Xv2.h"
#include "TapMcu.h"
#include "eem_defs.h"
#include "res/EmbeddedResources.h"
#include "../FuncletsInterface/Interface.h"



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
	return true;
}


bool TapDev430Xv2::GetRegs_Begin()
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


void TapDev430Xv2::GetRegs_End()
{
	SetPC(back_r0_ - 4);
}



/**************************************************************************************/
/* MCU VERSION-RELATED READ MEMORY METHODS                                            */
/**************************************************************************************/

// Source: slau320aj
uint16_t TapDev430Xv2::ReadWord(address_t address)
{
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
	g_Player.SetWordReadXv2();			// Set Word read CpuXv2
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
	g_Player.SetWordReadXv2();			// Set Word read CpuXv2
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
		// Processor is now again in Init State
		return true;
	}
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


// Source: slau320aj
bool TapDev430Xv2::EraseFlash(address_t address, const uint16_t fctl1, const uint16_t fctl3, bool mass_erase)
{
	EraseCtrlXv2 ctrlData;

	ctrlData.addr_ = address;	// set dummy write address
	ctrlData.fctl1_ = fctl1;	// set erase mode
	ctrlData.fctl3_ = fctl3;	// FCTL3: lock/unlock INFO Segment A

	address_t startAddr = kRamStartAddress;			// RAM start address specified in config header file
	address_t ctrlAddr = startAddr + EmbeddedResources::res_EraseXv2_bin.size();

	TapDev430Xv2::WriteWords(startAddr, (uint16_t *)EmbeddedResources::res_EraseXv2_bin.data(), EmbeddedResources::res_EraseXv2_bin.size()/sizeof(uint16_t));
	TapDev430Xv2::WriteWords(ctrlAddr, (uint16_t *)&ctrlData, sizeof(ctrlData) / sizeof(uint16_t));
	// R12 points to the control data
	TapDev430Xv2::SetReg(12, ctrlAddr);
	// Release device and wait for JMB signal
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
		const uint32_t total_size = (EmbeddedResources::res_EraseXv2_bin.size() + sizeof(ctrlData)) / sizeof(uint16_t);
		for (uint32_t i = 0; i < total_size; i++)
		{
			TapDev430Xv2::WriteWord(startAddr, 0x3fff);
			startAddr += 2;
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
		core_id.id_data_addr_ = core_id.ip_pointer_;
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


void TapDev430Xv2::InitDefaultChip(ChipProfile &prof)
{
	prof.DefaultMcuXv2();
}


// Source UIF
bool TapDev430Xv2::SyncJtagAssertPorSaveContext(CpuContext &ctx, const ChipProfile &prof)
{
	const uint16_t address = g_TapMcu.IsFr41xx() ? WDT_ADDR_FR41XX : WDT_ADDR_XV2;

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
		kIrDr32(IR_EMEX_DATA_EXCHANGE32, GENCLKCTRL + kWrite),
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
	uint16_t wdtval = WDT_HOLD | ctx.wdt_;				// set original bits in addition to stop bit
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
		const ChipInfoDB::EtwCodes &eem = prof.eem_timers_;
		/*
		** Code refactored from MSPFET firmware. No documentation could be found
		** for ETKEYSEL / ETCLKSEL. Best source of information is 'modules.h'.
		*/
		for (size_t i = 0; i < _countof(eem.etw_codes_); ++i)
		{
			// check if module clock control is enabled for corresponding module
			uint16_t v = (ctx.eem_clk_ctrl_ & (1UL << i)) != 0;
			WriteWord(ETKEYSEL, ETKEY | eem.etw_codes_[i]);
			WriteWord(ETCLKSEL, v);
		}
	}
	static constexpr TapStep steps_04[] =
	{
		// switch back system clocks to original clock source but keep them stopped
		kIrDr32(IR_EMEX_DATA_EXCHANGE32, GENCLKCTRL + kWrite)
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


void TapDev430Xv2::DisableLpmx5(const ChipProfile &prof)
{
	if (prof.pwr_settings_ == NULL)
		return;
	if (prof.pwr_settings_->test_reg3v_mask_)
	{
		uint16_t reg_3V = g_Player.Play(kIrDr16(IR_TEST_3V_REG, prof.pwr_settings_->test_reg3v_default));

		g_Player.DR_Shift16(reg_3V & ~prof.pwr_settings_->test_reg3v_mask_
							| prof.pwr_settings_->test_reg3v_disable_lpm5_);
		StopWatch().Delay(20);
	}

	if (prof.pwr_settings_->test_reg_mask_)
	{
		g_Player.IR_Shift(IR_TEST_REG);
		uint32_t reg_test = g_Player.DR_Shift32(prof.pwr_settings_->test_reg_default);
		g_Player.DR_Shift32(reg_test & ~prof.pwr_settings_->test_reg_mask_
							| prof.pwr_settings_->test_reg_disable_lpm5_);
		StopWatch().Delay(20);
	}
}


void TapDev430Xv2::SyncJtagXv2()
{
	g_Player.Play(kIrDr16(IR_CNTRL_SIG_16BIT, 0x1501));
	g_Player.IR_Shift(IR_CNTRL_SIG_CAPTURE);
	int i = 50;
	do
	{
		uint16_t lOut = g_Player.DR_Shift16(0);
		if (lOut != 0xFFFF && (lOut & 0x0200) != 0)
			break;
	}
	while (--i >= 0);
}


// Source UIF
bool TapDev430Xv2::SyncJtagConditionalSaveContext(CpuContext &ctx, const ChipProfile &prof)
{
	const uint16_t address = g_TapMcu.IsFr41xx() ? WDT_ADDR_FR41XX : WDT_ADDR_XV2;
	static constexpr uint32_t MaxCyclesForSync = 10000;	// must be defined, dependent on DMA (burst transfer)!!!

	// syncWithRunVarAddress
	ctx.is_running_ = false;
	// -------------------------Power mode handling start ----------------------
	DisableLpmx5(prof);
	// -------------------------Power mode handling end ------------------------

	// read out EEM control register...
	// ... and check if device got already stopped by the EEM
	if (g_Player.Play(kIrDr16(IR_EMEX_READ_CONTROL, 0)) & EEM_STOPPED)
	{
		// do this only if the device is NOT already stopped.
		// read out control signal register first

		// check if CPUOFF bit is set
		if (!(g_Player.Play(kIrDr16(IR_CNTRL_SIG_CAPTURE, 0)) & CNTRL_SIG_CPUOFF) )
		{
			// do the following only if the device is NOT in Low Power Mode
			uint32_t tbValue;
			uint32_t tbCntrl;
			uint32_t tbMask;
			uint32_t tbComb;
			uint32_t tbBreak;

			static constexpr TapStep steps_01[] =
			{
				kIr(IR_EMEX_DATA_EXCHANGE32),
				// Trigger Block 0 value register (val)
				kDr32(MBTRIGxVAL + kRead + TB0), // load val address
				// Trigger Block 0 control register
				kDr32_ret(0),								// shift in dummy 0

				// Trigger Block 0 value register (ctl)
				kDr32(MBTRIGxCTL + kRead + TB0),			// load ctl address
				// Trigger Block 0 control register
				kDr32_ret(0),								// shift in dummy 0

				// Trigger Block 0 value register (msk)
				kDr32(MBTRIGxMSK + kRead + TB0),			// load msk address
				// Trigger Block 0 control register
				kDr32_ret(0),								// shift in dummy 0

				// Trigger Block 0 value register (cmb)
				kDr32(MBTRIGxCMB + kRead + TB0),			// load cmb address
				// Trigger Block 0 control register
				kDr32_ret(0),								// shift in dummy 0

				// Trigger Block 0 value register (break)
				kDr32(BREAKREACT + kRead),					// load break address
				// Trigger Block 0 control register
				kDr32_ret(0),								// shift in dummy 0

				// now configure a trigger on the next instruction fetch
				kDr32(MBTRIGxCTL + kWrite + TB0),
				kDr32(CMP_EQUAL + TRIG_0 + MAB),
				kDr32(MBTRIGxMSK + kWrite + TB0),
				kDr32(MASK_ALL),
				kDr32(MBTRIGxCMB + kWrite + TB0),
				kDr32(EN0),
				kDr32(BREAKREACT + kWrite),
				kDr32(EN0),

				// enable EEM to stop the device
				kDr32(GENCLKCTRL + kWrite),
				kDr32(MCLK_SEL0 + SMCLK_SEL0 + ACLK_SEL0 + STOP_MCLK + STOP_SMCLK + STOP_ACLK),
				kIrDr16(IR_EMEX_WRITE_CONTROL, EMU_CLK_EN + CLEAR_STOP + EEM_EN),
			};
			g_Player.Play(steps_01, _countof(steps_01)
						  , &tbValue
						  , &tbCntrl
						  , &tbMask
						  , &tbComb
						  , &tbBreak
						  );
			{
				int lTimeout = 3000;
				do
				{
					if(!(g_Player.Play(kIrDr16(IR_EMEX_READ_CONTROL, 0)) & EEM_STOPPED))
						break;
				}
				while (--lTimeout < 0);
			}

			// restore the setting of Trigger Block 0 previously stored
			// Trigger Block 0 value register
			static constexpr TapStep steps_02[] =
			{
				kIr(IR_EMEX_DATA_EXCHANGE32),
				kDr32(MBTRIGxVAL + kWrite + TB0),
				kDr32Argv,							// tbValue
				kDr32(MBTRIGxCTL + kWrite + TB0),
				kDr32Argv,							// tbCntrl
				kDr32(MBTRIGxMSK + kWrite + TB0),
				kDr32Argv,							// tbMask
				kDr32(MBTRIGxCMB + kWrite + TB0),
				kDr32Argv,							// tbComb
				kDr32(BREAKREACT + kWrite),
				kDr32Argv,							// tbBreak
			};
			g_Player.Play(steps_02, _countof(steps_02)
						  , tbValue
						  , tbCntrl
						  , tbMask
						  , tbComb
						  , tbBreak
			);
		}
	}
	// End: special handling note 822
	//------------------------------------------------------------------------------

	// enable clock control before sync
	g_Player.Play(kIrDr16(IR_EMEX_WRITE_CONTROL, EMU_CLK_EN + EEM_EN));

	// sync device to JTAG
	SyncJtagXv2();

	// reset CPU stop reaction - CPU is now under JTAG control
	// Note: does not work on F5438 due to note 772, State storage triggers twice on single stepping
	static constexpr TapStep steps_03[] =
	{
		kIrDr16(IR_EMEX_WRITE_CONTROL, EMU_CLK_EN + CLEAR_STOP + EEM_EN),
		kDr16(EMU_CLK_EN + CLEAR_STOP),
		kIrDr16(IR_CNTRL_SIG_16BIT, 0x1501),
		// clock system into Full Emulation State now...
		// while checking control signals CPUSUSP (pipe_empty), CPUOFF and HALT
		kIr(IR_CNTRL_SIG_CAPTURE),
		kDr16_ret(0),
	};
	uint16_t lOut;
	g_Player.Play(steps_03, _countof(steps_03), &lOut);

	///////////////////////////////////////////
	// Note 805: check if wait is set
	if ((lOut & 0x8) == 0x8)
	{
		uint32_t TimeOut = 0;
		// wait until wait is end
		while ((lOut & 0x8) == 0x8 && TimeOut++ < 30000)
		{
			static constexpr TapStep steps[] =
			{
				kPulseTclkN,	// falling + rising edge clock
				kIr(IR_CNTRL_SIG_CAPTURE),
				kDr16_ret(0),
			};
			g_Player.Play(steps, _countof(steps), &lOut);
		}
	}
	// Note 805 end: Florian, 21 Dec 2010
	///////////////////////////////////////////

	bool pipe_empty = false;
	g_Player.IR_Shift(IR_CNTRL_SIG_CAPTURE);
	uint32_t i = 0;
	do 
	{
		g_Player.ClrTCLK();		// provide falling clock edge
		// shift out current control signal register value
		pipe_empty = (g_Player.DR_Shift16(0) & CNTRL_SIG_CPUSUSP) != 0;
		g_Player.SetTCLK();		// provide rising clock edge
	}
	while (!pipe_empty && i < MaxCyclesForSync);

	//! \todo check error condition
	if (i >= MaxCyclesForSync)
	{
		;
	}

	static constexpr TapStep steps_04[] =
	{
		// the interrupts will be disabled now - JTAG takes over control of the control signals
		kIrDr16(IR_CNTRL_SIG_16BIT, 0x0501),

		// provide 1 clock in order to have the data ready in the first transaction slot
		kPulseTclkN,
		kIr(IR_ADDR_CAPTURE),
		kDr20_ret(0),
		// shift out current control signal register value
		kIr(IR_CNTRL_SIG_CAPTURE),
		kDr16_ret(0),
	};
	uint32_t lOut_long;
	g_Player.Play(steps_04, _countof(steps_04)
				  , &lOut_long
				  , &lOut
	);

	bool cpuoff = (lOut & CNTRL_SIG_CPUOFF) != 0;

	ctx.in_interrupt_ = (lOut & 0x4) != 0;	// undocumented!?!

	// adjust program counter according to control signals
	lOut_long -= cpuoff ? 2 : 4;

	/********************************************************/
	/* Note 1495, 1637 special handling for program counter */
	/********************************************************/
	if ((lOut_long & 0xFFFF) == 0xFFFE)
		ctx.pc_ = TapDev430Xv2::ReadWord(0xFFFE);
	else
		ctx.pc_ = lOut_long;

	static constexpr TapStep steps_05[] =
	{
		// set EEM FEATURE enable now!!!
		kIrDr16(IR_EMEX_WRITE_CONTROL, EMU_FEAT_EN + EMU_CLK_EN + CLEAR_STOP),
		// check for Init State
		kIrDr16(IR_CNTRL_SIG_CAPTURE, 0),
	};
	g_Player.Play(steps_05, _countof(steps_05));

	// Hold Watchdog
	uint16_t wdtval = ctx.wdt_ | WDT_PASSWD;
	ctx.wdt_ = (uint8_t)TapDev430Xv2::ReadWord(address);	// save WDT value
	wdtval |= ctx.wdt_;										// adds the WDT stop bit
	TapDev430Xv2::WriteWord(address, wdtval);

	ctx.sr_ = GetReg(2);
	SetReg(2, ctx.sr_ & 0xFFE7);	// clear CPUOFF/GIE bit

	return true;
}


//----------------------------------------------------------------------------
//! \brief Function to execute a Power-On Reset (POR) using JTAG CNTRL SIG 
//! register
//! \return word (STATUS_OK if target is in Full-Emulation-State afterwards,
//! STATUS_ERROR otherwise)
//! Source: slau320aj
bool TapDev430Xv2::ExecutePOR()
{
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
		static constexpr TapStep steps[] =
		{
			kTclk1
			, kIrDr16(IR_CNTRL_SIG_16BIT, 0x0401)
			, kIr(IR_ADDR_CAPTURE)
			, kIr(IR_CNTRL_SIG_RELEASE)
		};
		g_Player.Play(steps, _countof(steps));
		break;
	}
}


static uint16_t get_subid_(const uint8_t *tlv, uint32_t tlv_size)
{
	constexpr uint8_t SUBVERSION_TAG = 0x14;
	constexpr uint8_t UNDEFINED_FF = 0xFF;
	constexpr uint8_t UNDEFINED_00 = 0x00;

	uint32_t pos = 8;
	// Must have at least 2 byte data left
	while (pos + 3 < tlv_size)
	{
		const uint8_t tag = tlv[pos++];
		const uint8_t len = tlv[pos++];
		const uint8_t *value = &tlv[pos];
		pos += len;

		if (tag == SUBVERSION_TAG)
			return ((uint16_t)value[1] << 8) + value[0];
		if ((tag == UNDEFINED_FF) || (tag == UNDEFINED_00) || (tag == SUBVERSION_TAG))
			break;
	}
	return UNDEFINED_00;
}


uint32_t TapDev430Xv2::EemDataExchangeXv2(uint8_t xchange, const CpuContext &ctx)
{
	// read access
	assert((xchange & 0x01) != 0);
	if ((xchange & 0xfe) == MODCLKCTRL0)
		return ctx.eem_clk_ctrl_;
	else
	{
		g_Player.IR_Shift(IR_EMEX_DATA_EXCHANGE32);
		g_Player.SetReg_32Bits(xchange);	// load address
		return g_Player.SetReg_32Bits(0);
	}
}


void TapDev430Xv2::EemDataExchangeXv2(uint8_t xchange, uint32_t data, CpuContext &ctx)
{
	// write access
	assert((xchange & 0x01) == 0);

	if ((xchange & 0xfe) == MODCLKCTRL0)
	{
		ctx.eem_clk_ctrl_ = data;
	}
	else
	{
#if 0
		if ((xchange == 0x9E) && (data & 0x40))
		{
			lastTraceWritePos = 0;
		}
#endif

		g_Player.IR_Shift(IR_EMEX_DATA_EXCHANGE32);
		g_Player.SetReg_32Bits(xchange);	// load address
		g_Player.SetReg_32Bits(data);		// shift in value
	}
}


// Source: UIF
bool TapDev430Xv2::GetDeviceSignature(DieInfo &id, CpuContext &ctx, const CoreId &coreid)
{
	if (coreid.id_data_addr_ == 0)
	{
		return -1;
	}
	union
	{
		uint16_t d16[4];
		uint8_t d8[8];
	} data;
	bool status = false;

	if (!ReadWords(coreid.id_data_addr_, data.d16, _countof(data.d16)))
	{
		SetPC(ctx.pc_);
		return false;
	}
	
	id.mcu_ver_ = data.d16[2];
	id.mcu_sub_ = 0x0000;		// init with zero = no sub id
	id.mcu_rev_ = data.d8[6];	// HW Revision
	id.mcu_cfg_ = data.d8[7];	// SW Revision
	id.mcu_fab_ = 0x55;
	id.mcu_self_ = 0x5555;
	id.mcu_fuse_ = 0x55;

	uint8_t info_len = data.d8[0];
	if ((info_len > 1) && (info_len < 11))
	{
		uint32_t tlv_size = 4 * (1 << data.d8[0]) - 2;
		uint8_t tlv[tlv_size];

		if (!ReadWords(coreid.id_data_addr_, (uint16_t *)tlv, tlv_size / 2))
			goto error_exit;
		id.mcu_sub_ = get_subid_(tlv, tlv_size);
	}

	ctx.eem_version_ = EemDataExchangeXv2(0x87, ctx);

	status = true;
error_exit:
	if (!SetPC(ctx.pc_))
		return false;
	return status;
}
