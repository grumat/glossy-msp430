/* MSPDebug - debugging tool for MSP430 MCUs
 * Copyright (C) 2009-2012 Daniel Beer
 * Copyright (C) 2012-2015 Peter Bägel
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

/* jtag functions are taken from TIs SLAA149–September 2002
 *
 * breakpoint implementation influenced by a posting of Ruisheng Lin
 * to Travis Goodspeed at 2012-09-20 found at:
 * http://sourceforge.net/p/goodfet/mailman/message/29860790/
 *
 * 2012-10-03 Peter Bägel (DF5EQ)
 * 2012-10-03   initial release              Peter Bägel (DF5EQ)
 * 2014-12-26   jtag_single_step added       Peter Bägel (DF5EQ)
 *              jtag_read_reg    corrected
 *              jtag_write_reg   corrected
 * 2015-02-21   jtag_set_breakpoint added    Peter Bägel (DF5EQ)
 *              jtag_cpu_state      added
 * 2020-06-01   jtag_read_reg    corrected   Gabor Mayer (HG5OAP)
 *              jtag_write_reg   corrected
 */

#include "stdproj.h"

#include "TapDev.h"
#include "eem_defs.h"


/**************************************************************************************/
/* GENERAL SUPPORT FUNCTIONS                                                          */
/**************************************************************************************/


bool TapDev::Open(ITapInterface &itf)
{
	itf_ = &itf;
	traits_ = &msp430legacy_;
	failed_ = !itf.OnOpen();
	altromaddr_cpuread_ = true;
	fast_flash_ = false;
	return (failed_ == false);
}


void TapDev::Close()
{
	itf_->OnClose();
	traits_ = &msp430legacy_;
}


/* Take target device under JTAG control.
 * Disable the target watchdog.
 * return: 0 - fuse is blown
 *        >0 - jtag id
 */
TapDev::JtagId TapDev::Init()
{
	failed_ = true;
	altromaddr_cpuread_ = true;
	fast_flash_ = false;
	jtag_id_ = kInvalid;
	coreip_id_ = 0;
	id_data_addr_ = 0x0FF0;
	ip_pointer_ = 0;
	itf_->OnEnterTap();

	/* Check fuse */
	if (IsFuseBlown())
	{
		Error() << "jtag_init: fuse is blown\n";
		return kInvalid;
	}

	altromaddr_cpuread_ = false;
	// Set device into JTAG mode
	GetDevice();
	if (IsMSP430() == false)
	{
		Error() << "jtag_init: invalid jtag_id: 0x" << f::X<2>(jtag_id_) << '\n';
		jtag_id_ = kInvalid;
		return kInvalid;
	}

	return jtag_id_;
}


bool TapDev::StartMcu(ChipInfoDB::CpuArchitecture arch, bool fast_flash)
{
	itf_ = &itf_->OnStetupArchitecture(arch);
	fast_flash_ = fast_flash;

	switch (arch)
	{
	case ChipInfoDB::kCpuXv2:
		traits_ = &msp430Xv2_;
		break;
	case ChipInfoDB::kCpuX:
		traits_ = &msp430X_;
		break;
	default:
		traits_ = &msp430legacy_;
		break;
	}

	/* Perform PUC, includes target watchdog disable */
	if (ExecutePOR() == false)
	{
		Error() << "jtag_init: PUC failed\n";
		return false;
	}

	failed_ = false;
	return true;
}


TapDev::JtagId TapDev::GetDevice()
{
	unsigned int loop_counter;

	/* Set device into JTAG mode + read */
#if 0
	itf_->OnIrShift(IR_CNTRL_SIG_16BIT);
	itf_->OnDrShift16(0x2401);
#else
	SetJtagRunRead();		// JTAG mode + CPU run + read
#endif

	/* Wait until CPU is synchronized,
	 * timeout after a limited number of attempts
	 */
	jtag_id_ = cntrl_sig_capture();
	if (!IsMSP430())
	{
		failed_ = true;
		/* timeout reached */
		return kInvalid;
	}
	for (loop_counter = 50; loop_counter > 0; loop_counter--)
	{
		if ((itf_->OnDrShift16(0x0000) & 0x0200) == 0x0200)
			break;
	}

	if (loop_counter == 0)
	{
		Error() << "TapDev::GetDevice: timed out\n";
		failed_ = true;
		/* timeout reached */
		return kInvalid;
	}

	id_data_addr_ = 0x0FF0;
	if (IsXv2())
	{
		// Get Core identification info
#if 0
		core_ip_pointer();
		coreip_id_ = SetReg_16Bits(0);
#else
		coreip_id_ = Play(kIrDr16(IR_COREIP_ID, 0));
#endif
		// Get device identification pointer
		if (jtag_id_ == kMsp_95)
			StopWatch().Delay(1500);
		device_ip_pointer();
		ip_pointer_ = SetReg_20Bits(0);
		// The ID pointer is an un-scrambled 20bit value
		ip_pointer_ = ((ip_pointer_ & 0xFFFF) << 4) + (ip_pointer_ >> 16);
		if (ip_pointer_ && (ip_pointer_ & 1) == 0)
		{
			id_data_addr_ = ip_pointer_ + 4;
		}
	}
	else
	{
		coreip_id_ = 0;
		ip_pointer_ = 0;
	}
	RedGreenOn();
	return jtag_id_;
}


bool TapDev::ReadChipId(void *buf, uint32_t size)
{
	uint32_t words = size >> 1;
	// function table is not ready yet, so bypass it
	if(IsXv2())
	{
		/*
		** MSP430F5418A does not like any read on this area without the use of the PC, so
		** we need to use the IR_DATA_QUICK to read this area.
		** This is nowhere described and costs me many wasted hours...
		*/
		ReadWordsXv2_slau320aj(id_data_addr_, (uint16_t *)buf, words < 8 ? words : 8);
		// Now we should get a valid read
		return ReadWordsXv2_slau320aj(id_data_addr_, (uint16_t *)buf, words);
	}
	else
	{
		ReadWords_slau320aj(id_data_addr_, (uint16_t *)buf, words < 8 ? words : 8);
		// Now we should get a valid read
		return ReadWords_slau320aj(id_data_addr_, (uint16_t *)buf, words);
	}
}


/*!
Set target CPU JTAG state machine into the instruction fetch state

\return: 1 - instruction fetch was set; 0 - otherwise
*/
TapDev::JtagId TapDev::SetInstructionFetch()
{
	for(int retries = 5; retries > 0; --retries)
	{
		/* Set device into JTAG mode + read */
#if 0
		itf_->OnIrShift(IR_CNTRL_SIG_16BIT);
		itf_->OnDrShift16(0x2401);
#else
		SetJtagRunRead();		// JTAG mode + CPU run + read
#endif

		JtagId jtag_id = (JtagId)itf_->OnIrShift(IR_CNTRL_SIG_CAPTURE);
		/* Wait until CPU is in instruction fetch state
		 * timeout after limited attempts
		 */
		for (int loop_counter = 30; loop_counter > 0; loop_counter--)
		{
			if ((itf_->OnDrShift16(0x0000) & 0x0080) == 0x0080)
				return jtag_id;
			/*
			The TCLK pulse before OnDrShift16 leads to problems at MEM_QUICK_READ,
			it's from SLAU265
			*/
			itf_->OnPulseTclkN();
		}
	}

	Error() << "SetInstructionFetch: failed\n";
	failed_ = true;

	return kInvalid;
}


/*!
Set the CPU into a controlled stop state
*/
bool TapDev::HaltCpu()
{
	failed_ = false;
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
	Play(steps, _countof(steps));
#endif
	return true;
}


/*!
This function checks if the JTAG access security fuse is blown.

\return: true - fuse is blown; false - otherwise
*/
bool TapDev::IsFuseBlown()
{
	unsigned int loop_counter;

	/* First trial could be wrong */
	for (loop_counter = 3; loop_counter > 0; loop_counter--)
	{
		if (Play(kIrDr16(IR_CNTRL_SIG_CAPTURE, 0xAAAA)) == 0x5555)
			/* Fuse is blown */
			return true;
	}

	/* Fuse is not blown */
	return false;
}


//----------------------------------------------------------------------------
//! \brief Function to resync the JTAG connection and execute a Power-On-Reset
//! \return true if operation was successful, false otherwise)
bool TapDev::SyncJtag_AssertPor()
{
	uint16_t i = 0;

	Play(kIrDr16(IR_CNTRL_SIG_16BIT, 0x1501));  // Set device into JTAG mode + read

	uint8_t jtag_id = IR_Shift(IR_CNTRL_SIG_CAPTURE);

	if ((jtag_id != JTAG_ID91) && (jtag_id != JTAG_ID99))
	{
		return false;
	}
	// wait for sync
	while (!(DR_Shift16(0) & 0x0200) && i < 50)
	{
		i++;
	};
	// continues if sync was successful
	if (i >= 50)
		return false;

	// execute a Power-On-Reset
	if (ExecutePorXv2_slau320aj() == false)
		return false;

	return true;
}




/**************************************************************************************/
/* MCU VERSION-RELATED REGISTER GET/SET METHODS                                       */
/**************************************************************************************/

//----------------------------------------------------------------------------
//! \brief Load a given address into the target CPU's program counter (PC).
//! \param[in] word address (destination address)
bool TapDev::SetPc_slau320aj(address_t address)
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
	Play(steps, _countof(steps)
		, address
	);
#endif
	return true;
}


//----------------------------------------------------------------------------
//! \brief Load a given address into the target CPU's program counter (PC).
//! \param[in] word address (destination address)
bool TapDev::SetPcX_slau320aj(address_t address)
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
	Play(steps, _countof(steps)
		, (uint16_t)(0x0080 | (((address) >> 8) & 0x0F00))
		, address
	);
#endif
	return true;
}


//----------------------------------------------------------------------------
//! \brief Load a given address into the target CPU's program counter (PC).
//! \param[in] uint32_t address (destination address)
bool TapDev::SetPcXv2_slau320aj(address_t address)
{
	uint16_t Mova;
	uint16_t Pc_l;

	Mova = 0x0080;
	Mova += (uint16_t)((address >> 8) & 0x00000F00);
	Pc_l = (uint16_t)((address & 0xFFFF));

	// Check Full-Emulation-State at the beginning
	if (Play(kIrDr16(IR_CNTRL_SIG_CAPTURE, 0)) & 0x0301)
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
		DR_Shift16(0x4303);
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
			, kDr16(0x4303)
			, kTclk0
			, kIrDr20(IR_ADDR_CAPTURE, 0)
		};
		Play(steps, _countof(steps)
			, Mova
			, Pc_l
		);
#endif
	}
	return true;
}


bool TapDev::SetReg_uif(uint8_t reg, uint32_t value)
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
	Play(
		steps, _countof(steps)
		, (0x4030 | reg)
		, value
	);
#endif
	return true;
}


bool TapDev::SetRegX_uif(uint8_t reg, uint32_t value)
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
	Play(steps, _countof(steps)
		, op
		, value
	);
#endif
	return true;
}


bool TapDev::SetRegXv2_uif(uint8_t reg, uint32_t value)
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
	Play(steps, _countof(steps)
		, mova
		, rx_l
	);
#endif
	return true;
}


uint32_t TapDev::GetReg_uif(uint8_t reg)
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
		, kIrDr16(IR_DATA_16BIT, 0x00fe)
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
	Play(steps, _countof(steps)
		, ((reg << 8) & 0x0F00) | 0x4082
		, &data
	);
	return data;
#endif
}


uint32_t TapDev::GetRegX_uif(uint8_t reg)
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
		, kDr16(0x00fc)
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
	Play(steps, _countof(steps)
		, ((reg << 8) & 0x0F00) | 0x60
		, &rx_l
		, &rx_h
	);
	return ((uint32_t)rx_h << 16) | rx_l;
#endif
}


uint32_t TapDev::GetRegXv2_uif(uint8_t reg)
{
	uint16_t Mova = 0x0060;
	Mova += ((uint16_t)reg << 8) & 0x0F00;

	JtagId jtagId = cntrl_sig_capture();
	const uint16_t jmbAddr = 
		(altromaddr_cpuread_) ? 0x0ff6
		: (jtagId == kMsp_98) ? 0x14c 
		: 0x18c;

#if 0
	IHIL_Tclk(0);
	data_16bit();
	IHIL_Tclk(1);
	SetReg_16Bits(reg);
	cntrl_sig_16bit();
	SetReg_16Bits(0x1401);
	data_16bit();
	itf_->OnPulseTclkN();
	if (altromaddr_cpuread_)
	{
		SetReg_16Bits(0x0ff6);
	}
	else
	{
		SetReg_16Bits(jmbAddr);
	}
	itf_->OnPulseTclkN();
	SetReg_16Bits(0x3ffd);
	IHIL_Tclk(0);
#else
	static constexpr TapStep steps_01[] =
	{
		kTclk0
		, kIr(IR_DATA_16BIT)
		, kTclk1
		, kDr16Argv				// dr16(reg)
		, kIrDr16(IR_CNTRL_SIG_16BIT, 0x1401)
		, kIr(IR_DATA_16BIT)
		, kPulseTclkN
		, kDr16Argv				// dr16(jmbAddr)
		, kPulseTclkN
		, kDr16(0x3ffd)
		, kTclk0
	};
	Play(steps_01, _countof(steps_01)
		, reg
		, jmbAddr
	);
#endif
	if (altromaddr_cpuread_)
	{
#if 0
		cntrl_sig_16bit();
		SetReg_16Bits(0x0501);
#else
		SetWordReadXv2();			// Set Word read CpuXv2
#endif
	}
#if 0
	data_capture();
	IHIL_Tclk(1);
	uint16_t Rx_l = SetReg_16Bits(0);
	itf_->OnPulseTclkN();
	uint16_t Rx_h = SetReg_16Bits(0);
	itf_->OnPulseTclkN();
	itf_->OnPulseTclkN();
	itf_->OnPulseTclkN();
#else
	static constexpr TapStep steps_02[] =
	{
		kIr(IR_DATA_CAPTURE)
		, kTclk1
		, kDr16_ret(0)			// Rx_l = dr16(0)
		, kPulseTclkN
		, kDr16_ret(0)			// Rx_h = dr16(0)
		, kPulseTclkN
		, kPulseTclkN
		, kPulseTclkN
	};
	uint16_t Rx_l = 0xFFFF;
	uint16_t Rx_h = 0xFFFF;
	Play(steps_02, _countof(steps_02)
		, &Rx_l
		, &Rx_h
	);
#endif
	if (!altromaddr_cpuread_)
	{
#if 0
		cntrl_sig_16bit();
		SetReg_16Bits(0x0501);
#else
		SetWordReadXv2();			// Set Word read CpuXv2
#endif
	}
	IHIL_Tclk(0);
	data_capture();
	IHIL_Tclk(1);

	if (jtagId == kMsp_91
		|| jtagId == kMsp_98
		|| jtagId == kMsp_99)
	{
		// Set PC to "safe" address
		SetPcXv2_slau320aj(SAFE_PC_ADDRESS);
#if 0
		cntrl_sig_16bit();
		SetReg_16Bits(0x0501);
#else
		SetWordReadXv2();			// Set Word read CpuXv2
#endif
		IHIL_Tclk(1);
		addr_capture();
	}
	itf_->OnReadJmbOut();

	return (((uint32_t)Rx_h << 16) + Rx_l) &0xfffff;
}





/**************************************************************************************/
/* MCU VERSION-RELATED READ MEMORY METHODS                                            */
/**************************************************************************************/

uint16_t TapDev::ReadWord_slau320aj(address_t address)
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
	Play(steps, _countof(steps)
		, address
		, &content
	);
#endif
	return content;
}


bool TapDev::ReadWords_slau320aj(address_t address, uint16_t *buf, uint32_t word_count)
{
	if (!HaltCpu())
		return false;
	itf_->OnClearTclk();
	SetWordRead();					// Set RW to read: ir_dr16(IR_CNTRL_SIG_16BIT, 0x2409);
	for (uint32_t i = 0; i < word_count; ++i)
	{
		// Set address
		itf_->OnIrShift(IR_ADDR_16BIT);
		itf_->OnDrShift16(address);
		itf_->OnIrShift(IR_DATA_TO_ADDR);
		itf_->OnPulseTclk();
		// Fetch 16-bit data
		*buf++ = itf_->OnDrShift16(0x0000);
		address += 2;
	}
	//itf_->OnSetTclk(); /* is also the first instruction in ReleaseCpu() */
	ReleaseCpu();
	return true;
}


uint16_t TapDev::ReadWordX_slau320aj(address_t address)
{
	if (!HaltCpu())
		return 0xFFFF;
#if 1
	itf_->OnClearTclk();
	SetWordRead();					// Set RW to read: ir_dr16(IR_CNTRL_SIG_16BIT, 0x2409);
	// Set address
	itf_->OnIrShift(IR_ADDR_16BIT);
	itf_->OnDrShift20(address);
	itf_->OnIrShift(IR_DATA_TO_ADDR);
	itf_->OnPulseTclk();
	// Fetch 16-bit data
	uint16_t content = itf_->OnDrShift16(0x0000);
	ReleaseCpu();
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

//#error stop here

bool TapDev::ReadWordsX_slau320aj(address_t address, uint16_t *buf, uint32_t word_count)
{
	if (!HaltCpu())
		return false;
	itf_->OnClearTclk();
	SetWordRead();					// Set RW to read: ir_dr16(IR_CNTRL_SIG_16BIT, 0x2409);
	for (uint32_t i = 0; i < word_count; ++i)
	{
		// Set address
		itf_->OnIrShift(IR_ADDR_16BIT);
		itf_->OnDrShift20(address);
		itf_->OnIrShift(IR_DATA_TO_ADDR);
		itf_->OnPulseTclk();
		// Fetch 16-bit data
		*buf++ = itf_->OnDrShift16(0x0000);
		address += 2;
	}
	ReleaseCpu();
	return true;
}


uint16_t TapDev::ReadWordXv2_slau320aj(address_t address)
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
	Play(steps, _countof(steps)
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
bool TapDev::ReadWordsXv2_slau320aj(address_t address, uint16_t *buf, uint32_t word_count)
{
	SetPcXv2_slau320aj(address);

	address_t lPc = 0;

	// Set PC to 'safe' address
	if ((IR_Shift(IR_CNTRL_SIG_CAPTURE) == JTAG_ID99) || (IR_Shift(IR_CNTRL_SIG_CAPTURE) == JTAG_ID98))
	{
		lPc = 0x00000004;
	}
	SetTCLK();
	uint32_t retry = 50;
	for (;;)
	{
#if 0
		IR_Shift(IR_CNTRL_SIG_16BIT);
		DR_Shift16(0x0501);
#else
		SetWordReadXv2();			// Set Word read CpuXv2
#endif
		//__NOP();
		if ((Play(kIrDr16(IR_CNTRL_SIG_CAPTURE, 0)) & 0x0301) == 0x0301)
			break;
		// too many retries
		if (--retry == 0)
			return false;
	}
	IR_Shift(IR_ADDR_CAPTURE);

	IR_Shift(IR_DATA_QUICK);

	for (uint32_t i = 0; i < word_count; ++i)
	{
		itf_->OnPulseTclk();
		*buf++ = DR_Shift16(0);  // Read data from memory.         
	}

	if (lPc)
	{
		SetPcXv2_slau320aj(lPc);
	}
	SetTCLK();
	return true;
}




/**************************************************************************************/
/* MCU VERSION-RELATED WRITE MEMORY METHODS                                           */
/**************************************************************************************/

//----------------------------------------------------------------------------
//! \brief This function writes one byte/word at a given address ( <0xA00)
//! \param[in] word address (Address of data to be written)
//! \param[in] word data (shifted data)
bool TapDev::WriteWord_slau320aj(address_t address, uint16_t data)
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
	Play(steps, _countof(steps)
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
bool TapDev::WriteWords_slau320aj(address_t address, const uint16_t *buf, uint32_t word_count)
{
	// Initialize writing:
	if (!SetPc_slau320aj(address - 4)
		|| !HaltCpu())
		return false;

	ClrTCLK();
	SetWordWrite();					// Set RW to write: ir_dr16(IR_CNTRL_SIG_16BIT, 0x2408);
	IR_Shift(IR_DATA_QUICK);
	for (uint32_t i = 0; i < word_count; i++)
	{
		DR_Shift16(buf[i]);			// Shift in the write data
		itf_->OnPulseTclk();		// Increment PC by 2
	}
	ReleaseCpu();
	return true;
}


//----------------------------------------------------------------------------
//! \brief This function writes one byte/word at a given address ( <0xA00)
//! \param[in] word address (Address of data to be written)
//! \param[in] word data (shifted data)
bool TapDev::WriteWordX_slau320aj(address_t address, uint16_t data)
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
	Play(steps, _countof(steps)
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
bool TapDev::WriteWordsX_slau320aj(address_t address, const uint16_t *buf, uint32_t word_count)
{
	uint32_t i;

	// Initialize writing:
	if (!SetPcX_slau320aj(address - 4)
		|| !HaltCpu())
		return false;

	ClrTCLK();
	SetWordWrite();					// Set RW to write: ir_dr16(IR_CNTRL_SIG_16BIT, 0x2408);
	IR_Shift(IR_DATA_QUICK);
	for (i = 0; i < word_count; i++)
	{
		DR_Shift16(buf[i]);			// Shift in the write data
		itf_->OnPulseTclk();		// Increment PC by 2
	}
	ReleaseCpu();
	return true;
}


//----------------------------------------------------------------------------
//! \brief This function writes one byte/word at a given address ( <0xA00)
//! \param[in] word address (Address of data to be written)
//! \param[in] word data (shifted data)
bool TapDev::WriteWordXv2_slau320aj(address_t address, uint16_t data)
{
	// Check Init State at the beginning
	IR_Shift(IR_CNTRL_SIG_CAPTURE);
	if (DR_Shift16(0) & 0x0301)
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
		Play(steps, _countof(steps)
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
bool TapDev::WriteWordsXv2_slau320aj(address_t address, const uint16_t *buf, uint32_t word_count)
{
	for (uint32_t i = 0; i < word_count; i++)
	{
		if (!WriteWordXv2_slau320aj(address, *buf++))
			return false;
		address += 2;
	}
	return true;
}


static constexpr uint16_t SegmentInfoAKey = 0xA500;


bool TapDev::WriteFlash_slau320aj(address_t address, const uint16_t *buf, uint32_t word_count)
{
	address_t addr = address;				// Address counter
	if (!HaltCpu())
		return false;

#if 0
	uint16_t FCTL3_val = SegmentInfoAKey;   // SegmentInfoAKey holds Lock-Key for Info
											// Seg. A 

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
	DR_Shift16(FCTL3_val);		// Clear FCTL3; F2xxx: Unlock Info-Seg.
								// A by toggling LOCKA-Bit if required,
	SetTCLK();

	ClrTCLK();
	IR_Shift(IR_CNTRL_SIG_16BIT);
#else
	static constexpr TapStep steps_01[] =
	{
		kTclk0
		, kIrDr16(IR_CNTRL_SIG_16BIT, 0x2408)	// Set RW to write
		, kIrDr16(IR_ADDR_16BIT, 0x0128)		// FCTL1 register
		, kIrDr16(IR_DATA_TO_ADDR, 0xA540)		// Enable FLASH write
		, kTclk1

		, kTclk0
		, kIrDr16(IR_ADDR_16BIT, 0x012A)		// FCTL2 register
		, kIrDr16(IR_DATA_TO_ADDR, 0xA540)		// Select MCLK as source, DIV=1
		, kTclk1

		, kTclk0
		, kIrDr16(IR_ADDR_16BIT, 0x012C)		// FCTL3 register
		, kIrDr16(IR_DATA_TO_ADDR, SegmentInfoAKey)	// Clear FCTL3; F2xxx: Unlock Info-Seg.
													// A by toggling LOCKA-Bit if required,
		, kTclk1

		, kTclk0
		, kIr(IR_CNTRL_SIG_16BIT)
	};
	Play(steps_01, _countof(steps_01));
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
		Play(steps_02, _countof(steps_02)
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
	DR_Shift16(0xA500);			// Disable FLASH write
	SetTCLK();

	// set LOCK-Bits again
	ClrTCLK();
	IR_Shift(IR_ADDR_16BIT);
	DR_Shift16(0x012C);			// FCTL3 address
	IR_Shift(IR_DATA_TO_ADDR);
	DR_Shift16(FCTL3_val | 0x0010);		// Lock Inf-Seg. A by toggling LOCKA and set LOCK again
	SetTCLK();
	ReleaseCpu();
#else
	static constexpr TapStep steps_03[] =
	{
		kIrDr16(IR_CNTRL_SIG_16BIT, 0x2408)		// Set RW to write
		, kIrDr16(IR_ADDR_16BIT, 0x0128)		// FCTL1 register
		, kIrDr16(IR_DATA_TO_ADDR, 0xA500)		// Enable FLASH write
		, kTclk1

		, kTclk0
		, kIrDr16(IR_ADDR_16BIT, 0x012C)		// FCTL3 register
		// Lock Inf-Seg. A by toggling LOCKA and set LOCK again
		, kIrDr16(IR_DATA_TO_ADDR, SegmentInfoAKey | 0x0010)
		, kTclk1
		, kReleaseCpu
	};
	Play(steps_03, _countof(steps_03));
#endif

	return true;
}


bool TapDev::WriteFlashX_slau320aj(address_t address, const uint16_t *buf, uint32_t word_count)
{
	uint32_t addr = address;				// Address counter
	if (!HaltCpu())
		return false;

#if 0
	uint16_t FCTL3_val = SegmentInfoAKey;	// Lock/Unlock SegA InfoMem Seg.A, def=locked

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
	DR_Shift16(FCTL3_val);		// Clear FCTL3; F2xxx: Unlock Info-Seg.
								// A by toggling LOCKA-Bit if required,
	SetTCLK();

	ClrTCLK();
	IR_Shift(IR_CNTRL_SIG_16BIT);
#else
	static constexpr TapStep steps_01[] =
	{
		kTclk0
		, kIrDr16(IR_CNTRL_SIG_16BIT, 0x2408)	// Set RW to write
		, kIrAddr20(0x0128)						// FCTL1 register
		, kIrDr16(IR_DATA_TO_ADDR, 0xA540)		// Enable FLASH write
		, kTclk1

		, kTclk0
		, kIrAddr20(0x012A)						// FCTL2 register
		, kIrDr16(IR_DATA_TO_ADDR, 0xA540)		// Select MCLK as source, DIV=1
		, kTclk1

		, kTclk0
		, kIrAddr20(0x012C)						// FCTL3 register
		, kIrDr16(IR_DATA_TO_ADDR, SegmentInfoAKey)	// Clear FCTL3; F2xxx: Unlock Info-Seg.
													// A by toggling LOCKA-Bit if required,
		, kTclk1

		, kTclk0
		, kIr(IR_CNTRL_SIG_16BIT)
	};
	Play(steps_01, _countof(steps_01));
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
		Play(steps_02, _countof(steps_02)
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
	DR_Shift16(0xA500);			// Disable FLASH write
	SetTCLK();

	// set LOCK-Bits again
	ClrTCLK();
	IR_Shift(IR_ADDR_16BIT);
	DR_Shift20(0x012C);			// FCTL3 address
	IR_Shift(IR_DATA_TO_ADDR);
	DR_Shift16(SegmentInfoAKey | 0x0010);	// Lock Inf-Seg. A by toggling LOCKA and set LOCK again
	SetTCLK();
	ReleaseCpu();
#else
	static constexpr TapStep steps_03[] =
	{
		kIrDr16(IR_CNTRL_SIG_16BIT, 0x2408)		// Set RW to write
		, kIrAddr20(0x0128)						// FCTL1 register
		, kIrDr16(IR_DATA_TO_ADDR, 0xA500)		// Enable FLASH write
		, kTclk1

		, kTclk0
		, kIrAddr20(0x012C)						// FCTL3 register
		// Lock Inf-Seg. A by toggling LOCKA and set LOCK again
		, kIrDr16(IR_DATA_TO_ADDR, SegmentInfoAKey | 0x0010)
		, kTclk1
		, kReleaseCpu
	};
	Play(steps_03, _countof(steps_03));
#endif

	return true;
}


//! \brief Set the start address of the device RAM
static constexpr uint32_t RAM_START_ADDRESS = 0x1C00;
static constexpr uint16_t SegmentInfoAKey5xx = 0xA548;

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

bool TapDev::WriteFlashXv2_slau320aj(address_t address, const uint16_t *data, uint32_t word_count)
{
	//! \brief Holds the target code for an flash write operation
//! \details This code is modified by the flash write function depending on it's parameters.

	address_t load_addr = RAM_START_ADDRESS;			// RAM start address specified in config header file
	address_t start_addr = load_addr + FlashWrite_o[0];	// start address of the program in target RAM

	FlashWrite_o[2] = (uint16_t)(address);				// set write start address
	FlashWrite_o[3] = (uint16_t)(address >> 16);
	FlashWrite_o[4] = (uint16_t)(word_count);			// set number of words to write
	FlashWrite_o[5] = (uint16_t)(word_count >> 16);
	FlashWrite_o[6] = SegmentInfoAKey5xx;				// FCTL3: lock/unlock INFO Segment A
														// default = locked

	WriteWordsXv2_slau320aj(load_addr, FlashWrite_o, _countof(FlashWrite_o));
	ReleaseDeviceXv2_slau320aj(start_addr);

	{
		uint32_t Jmb = 0;
		uint32_t Timeout = 0;

		do
		{
			Jmb = i_ReadJmbOut();
			Timeout++;
		}
		while (Jmb != 0xABADBABE && Timeout < 3000);

		if (Timeout < 3000)
		{
			uint32_t i;

			for (i = 0; i < word_count; i++)
			{
				i_WriteJmbIn16(data[i]);
				//usDelay(100);				// delay 100us  - added by GC       
			}
		}
	}
	{
		uint32_t Jmb = 0;
		uint32_t Timeout = 0;

		do
		{
			Jmb = i_ReadJmbOut();
			Timeout++;
		}
		while (Jmb != 0xCAFEBABE && Timeout < 3000);
	}

	SyncJtag_AssertPor();

	// clear RAM here - init with JMP $
	{
		for (uint32_t i = 0; i < _countof(FlashWrite_o); i++)
		{
			WriteWordXv2_slau320aj(load_addr, 0x3fff);
			load_addr += 2;
		}
	}
	return true;
}



/**************************************************************************************/
/* MCU VERSION-RELATED FLASH ERASE                                                    */
/**************************************************************************************/

bool TapDev::EraseFlash_slau320aj(address_t address, EraseMode mode)
{
	uint32_t strobe_amount = 4820;			// default for Segment Erase
	uint32_t loop_cnt = 1;					// erase cycle repeating for Mass Erase
	constexpr uint16_t FCTL3_val = SegmentInfoAKey;	// SegmentInfoAKey holds Lock-Key for Info
											// Seg. A     

	if ((mode == kMassEraseJtag) || (mode == kMainEraseJtag))
	{
		if (fast_flash_)
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
		DR_Shift16(mode);		// Enable erase mode
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
		DR_Shift16(FCTL3_val);		// Clear FCTL3; F2xxx: Unlock Info-Seg. A by toggling LOCKA-Bit if required,
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
		DR_Shift16(0xA500);			// Disable erase
		SetTCLK();
#else
		static constexpr TapStep steps_01[] =
		{
			kTclk0
			, kIrDr16(IR_CNTRL_SIG_16BIT, 0x2408)
			, kIrDr16(IR_ADDR_16BIT, 0x0128)		// FCTL1 address
			, kIrDr16Argv(IR_DATA_TO_ADDR)			// Enable erase "mode"
			, kTclk1

			, kTclk0
			, kIrDr16(IR_ADDR_16BIT, 0x012A)		// FCTL2 address
			, kIrDr16(IR_DATA_TO_ADDR, 0xA540)		// MCLK is source, DIV=1
			, kTclk1

			, kTclk0
			, kIrDr16(IR_ADDR_16BIT, 0x012C)		// FCTL3 address
			, kIrDr16(IR_DATA_TO_ADDR, FCTL3_val)	// Clear FCTL3; F2xxx: Unlock Info-Seg. A 
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
			, kIrDr16(IR_DATA_TO_ADDR, 0xA500)		// Disable erase
			, kTclk1
		};
		Play(steps_01, _countof(steps_01)
			, mode
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
	DR_Shift16(FCTL3_val | 0x0010);	// Lock Inf-Seg. A by toggling LOCKA (F2xxx) and set LOCK again
	SetTCLK();

	ReleaseCpu();
#else
	static constexpr TapStep steps_02[] =
	{
		kTclk0
		, kIrDr16(IR_ADDR_16BIT, 0x012C)				// FCTL3 address
		, kIrDr16(IR_DATA_TO_ADDR, FCTL3_val | 0x0010)	// Lock Inf-Seg. A by toggling LOCKA (F2xxx) and set LOCK again
		, kTclk1
		, kReleaseCpu
	};
	Play(steps_02, _countof(steps_02));
#endif
	return true;
}


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


bool TapDev::EraseFlashX_slau320aj(address_t address, EraseMode mode)
{
	uint32_t strobe_amount = 4820;			// default for Segment Erase
	uint32_t loop_cnt = 1;					// erase cycle repeating for Mass Erase
	constexpr uint16_t FCTL3_val = SegmentInfoAKey;	// Lock/Unlock SegA InfoMem Seg.A, def=locked

	if ((mode == kMassEraseJtag) 
		|| (mode == kMainEraseJtag) 
		|| (mode == kAllMainEraseJtag) 
		|| (mode == kGlobEraseJtag)
		)
	{
		if (fast_flash_)
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
		DR_Shift16(mode);			// Enable erase mode
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
		DR_Shift16(FCTL3_val);		// Clear FCTL3; F2xxx: Unlock Info-Seg. A by toggling LOCKA-Bit if required,
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
		DR_Shift16(0xA500);			// Disable erase
		SetTCLK();
#else
		static constexpr TapStep steps_01[] =
		{
			kTclk0
			, kIrDr16(IR_CNTRL_SIG_16BIT, 0x2408)
			, kIrDr20(IR_ADDR_16BIT, 0x0128)		// FCTL1 address
			, kIrDr16Argv(IR_DATA_TO_ADDR)			// Enable erase "mode"
			, kTclk1

			, kTclk0
			, kIrDr20(IR_ADDR_16BIT, 0x012A)		// FCTL2 address
			, kIrDr16(IR_DATA_TO_ADDR, 0xA540)		// MCLK is source, DIV=1
			, kTclk1

			, kTclk0
			, kIrDr20(IR_ADDR_16BIT, 0x012C)		// FCTL3 address
			, kIrDr16(IR_DATA_TO_ADDR, FCTL3_val)	// Clear FCTL3; F2xxx: Unlock Info-Seg. A 
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
			, kIrDr16(IR_DATA_TO_ADDR, 0xA500)		// Disable erase
			, kTclk1
		};
		Play(steps_01, _countof(steps_01)
			, mode
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
	DR_Shift16(FCTL3_val | 0x0010);	// Lock Inf-Seg. A by toggling LOCKA (F2xxx) and set LOCK again
	SetTCLK();

	ReleaseCpu();
#else
	static constexpr TapStep steps_02[] =
	{
		kTclk0
		, kIrDr20(IR_ADDR_16BIT, 0x012C)				// FCTL3 address
		, kIrDr16(IR_DATA_TO_ADDR, FCTL3_val | 0x0010)	// Lock Inf-Seg. A by toggling LOCKA (F2xxx) and set LOCK again
		, kTclk1
		, kReleaseCpu
	};
	Play(steps_02, _countof(steps_02));
#endif
	return true;
}


bool TapDev::EraseFlashXv2_slau320aj(address_t address, EraseMode mode)
{
	address_t loadAddr = RAM_START_ADDRESS;			// RAM start address specified in config header file
	address_t startAddr = loadAddr + FlashErase_o[0];	// start address of the program in target RAM

	FlashErase_o[2] = (uint16_t)(address);			// set dummy write address
	FlashErase_o[3] = (uint16_t)(address >> 16);
	FlashErase_o[4] = mode;							// set erase mode
	FlashErase_o[5] = SegmentInfoAKey5xx;			// FCTL3: lock/unlock INFO Segment A
													// default = locked

	WriteWordsXv2_slau320aj(loadAddr, (uint16_t *)FlashErase_o, _countof(FlashErase_o));
	ReleaseDeviceXv2_slau320aj(startAddr);

	{
		unsigned long Jmb = 0;
		unsigned long Timeout = 0;

		do
		{
			Jmb = i_ReadJmbOut();
			Timeout++;
		}
		while (Jmb != 0xCAFEBABE && Timeout < 3000);
	}

	SyncJtag_AssertPor();

	// clear RAM here - init with JMP $
	{
		for (uint32_t i = 0; i < _countof(FlashErase_o); i++)
		{
			WriteWord_slau320aj(loadAddr, 0x3fff);
			loadAddr += 2;
		}
	}
	return true;
}





/**************************************************************************************/
/* MCU VERSION-RELATED POWER ON RESET                                                 */
/**************************************************************************************/

bool TapDev::ExecutePor_slau320aj()
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
	Play(steps, _countof(steps)
		, &jtag_ver
	);
#endif

	WriteWord_slau320aj(0x0120, 0x5A80);	// Disable Watchdog on target device

	if (jtag_ver != kMspStd)
		return false;
	return true;
}


bool TapDev::ExecutePorX_slau320aj()
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
	Play(steps, _countof(steps)
		, &jtag_ver
	);
#endif

	WriteWordX_slau320aj(0x0120, 0x5A80);	// Disable Watchdog on target device

	if (jtag_ver != kMspStd)
		return false;
	return true;
}


//----------------------------------------------------------------------------
//! \brief Function to execute a Power-On Reset (POR) using JTAG CNTRL SIG 
//! register
//! \return word (STATUS_OK if target is in Full-Emulation-State afterwards,
//! STATUS_ERROR otherwise)
bool TapDev::ExecutePorXv2_slau320aj()
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
	Play(steps, _countof(steps));
#endif
	// the CPU is now in 'Full-Emulation-State'

	// disable Watchdog Timer on target device now by setting the HOLD signal
	// in the WDT_CNTRL register
	WriteWordXv2_slau320aj(0x015C, 0x5A80);

	// Check if device is in Full-Emulation-State again and return status
	if (Play(kIrDr16(IR_CNTRL_SIG_CAPTURE, 0)) & 0x0301)
		return true;

	return false;
}




/**************************************************************************************/
/* MCU VERSION-RELATED DEVICE RELEASE                                                 */
/**************************************************************************************/

void TapDev::ReleaseDevice_slau320aj(address_t address)
{
	switch (address)
	{
	case V_RUNNING: /* Nothing to do */
		break;
	case V_BOR:
	case V_RESET: /* Perform reset */
		/* issue reset */
		Play(kIrDr16(IR_CNTRL_SIG_16BIT, 0x2C01));
		itf_->OnDrShift16(0x2401);
		break;
	default: /* Set target CPU's PC */
		(this->*traits_->fnSetPC)(address);
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
	Play(steps, _countof(steps));
#endif
}


void TapDev::ReleaseDeviceXv2_slau320aj(address_t address)
{
	switch (address)
	{
	case V_BOR:
		// perform a BOR via JTAG - we loose control of the device then...
		Play(kIrDr16(IR_TEST_REG, 0x0200));
		MicroDelay::Delay(5000);			// wait some time before doing any other action
		// JTAG control is lost now - GetDevice() needs to be called again to gain control.
		break;

	case V_RESET:
		Play(kIrDr16(IR_CNTRL_SIG_16BIT, 0x0C01));	// Perform a reset
		DR_Shift16(0x0401);
		IR_Shift(IR_CNTRL_SIG_RELEASE);
		break;

	case V_RUNNING:
		IR_Shift(IR_CNTRL_SIG_RELEASE);
		break;

	default:
		SetPcXv2_slau320aj(address);	// Set target CPU's PC
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
		Play(steps, _countof(steps));
#endif
		break;
	}
}




/**************************************************************************************/
/* TRAITS FUNCTION TABLE                                                              */
/**************************************************************************************/

const TapDev::CpuTraitsFuncs TapDev::msp430legacy_ =
{
	.fnSetPC = &TapDev::SetPc_slau320aj
	, .fnSetReg = &TapDev::SetReg_uif
	, .fnGetReg = &TapDev::GetReg_uif
	//
	, .fnReadWord = &TapDev::ReadWord_slau320aj
	, .fnReadWords = &TapDev::ReadWords_slau320aj
	//
	, .fnWriteWord = &TapDev::WriteWord_slau320aj
	, .fnWriteWords = &TapDev::WriteWords_slau320aj
	, .fnWriteFlash = &TapDev::WriteFlash_slau320aj
	//
	, .fnEraseFlash = &TapDev::EraseFlash_slau320aj
	//
	, .fnExecutePOR = &TapDev::ExecutePor_slau320aj
	, .fnReleaseDevice = &TapDev::ReleaseDevice_slau320aj
};

const TapDev::CpuTraitsFuncs TapDev::msp430X_ =
{
	.fnSetPC = &TapDev::SetPcX_slau320aj
	, .fnSetReg = &TapDev::SetRegX_uif
	, .fnGetReg = &TapDev::GetRegX_uif
	//
	, .fnReadWord = &TapDev::ReadWord_slau320aj
	, .fnReadWords = &TapDev::ReadWordsX_slau320aj
	//
	, .fnWriteWord = &TapDev::WriteWordX_slau320aj
	, .fnWriteWords = &TapDev::WriteWordsX_slau320aj
	, .fnWriteFlash = &TapDev::WriteFlashX_slau320aj
	//
	, .fnEraseFlash = &TapDev::EraseFlashX_slau320aj
	//
	, .fnExecutePOR = &TapDev::ExecutePorX_slau320aj
	, .fnReleaseDevice = &TapDev::ReleaseDevice_slau320aj
};

const TapDev::CpuTraitsFuncs TapDev::msp430Xv2_ =
{
	.fnSetPC = &TapDev::SetPcXv2_slau320aj
	, .fnSetReg = &TapDev::SetRegXv2_uif
	, .fnGetReg = &TapDev::GetRegXv2_uif
	//
	, .fnReadWord = &TapDev::ReadWord_slau320aj
	, .fnReadWords = &TapDev::ReadWordsXv2_slau320aj
	//
	, .fnWriteWord = &TapDev::WriteWordXv2_slau320aj
	, .fnWriteWords = &TapDev::WriteWordsXv2_slau320aj
	, .fnWriteFlash = &TapDev::WriteFlashXv2_slau320aj
	//
	, .fnEraseFlash = &TapDev::EraseFlashXv2_slau320aj
	//
	, .fnExecutePOR = &TapDev::ExecutePorXv2_slau320aj
	, .fnReleaseDevice = &TapDev::ReleaseDeviceXv2_slau320aj
};




/**************************************************************************************/
/* EXPERIMENTAL METHODS                                                               */
/**************************************************************************************/


void TapDev::ReadWordsXv2_uif(address_t address, uint16_t *buf, uint32_t len)
{
	// SET PROGRAM COUNTER for QUICK ACCESS
	SetPcXv2_slau320aj(address);
#if 0
	cntrl_sig_16bit();
	SetReg_16Bits(0x0501);
#else
	SetWordReadXv2();			// Set Word read CpuXv2
#endif
	IHIL_Tclk(1);
	addr_capture();
	// END OF SETTING THE PROGRAM COUNTER
	data_quick();

	for (uint32_t i = 0; i < len; ++i)
	{
		itf_->OnPulseTclk();
		*buf++ = SetReg_16Bits(0);
	}
	// Check save State
	cntrl_sig_capture();
	SetReg_16Bits(0x0000);

	// SET PROGRAM COUNTER for Backup
	SetPcXv2_slau320aj(SAFE_PC_ADDRESS);
#if 0
	cntrl_sig_16bit();
	SetReg_16Bits(0x0501);
#else
	SetWordReadXv2();			// Set Word read CpuXv2
#endif
	IHIL_Tclk(1);
	addr_capture();
}



/**************************************************************************************/
/* DISPATCHER METHODS                                                                 */
/**************************************************************************************/


/*!
Reads one byte/word from a given address.

\param format : 8-byte, 16-word
\param address: address of memory
\return : content of memory
*/
void TapDev::ReadWords(address_t address, uint16_t *buf, uint32_t word_count)
{
	failed_ = false;

	// 16-bit aligned address required
	assert((address & 1) == 0);
	// At least one word is required
	assert(word_count > 0);

	(this->*traits_->fnReadWords)(address, buf, word_count);
}


uint16_t TapDev::ReadWord(address_t address)
{
	failed_ = false;

	// 16-bit aligned address required
	assert((address & 1) == 0);

	return (this->*traits_->fnReadWord)(address);
}


bool TapDev::WriteWord(address_t address, uint16_t data)
{
	failed_ = false;

	// 16-bit aligned address required
	assert((address & 1) == 0);

	return (this->*traits_->fnWriteWord)(address, data);
}


/*!
Writes an array of words into target memory.

\param address: address to write to
\param length: number of word to write
\param data: data to write
*/
bool TapDev::WriteMem(address_t address, const uint16_t *data, uint32_t length)
{
	failed_ = false;

	// 16-bit aligned address required
	assert((address & 1) == 0);

	return (this->*traits_->fnWriteWords)(address, data, length);
}


bool TapDev::SetPC(address_t address)
{
	return (this->*traits_->fnSetPC)(address);
}

/* Programs/verifies an array of words into a FLASH by using the
 * FLASH controller. The JTAG FLASH register isn't needed.
 * address: start in FLASH
 * word_count   : number of words
 * data         : pointer to data
 */
bool TapDev::WriteFlash(address_t address, const uint16_t *data, uint32_t word_count)
{
	RedLedOff();
	bool res = (this->*traits_->fnWriteFlash)(address, data, word_count);
	RedLedOn();
	return res;
}


/*!
Performs a mass erase (with and w/o info memory) or a segment erase of a
FLASH module specified by the given mode and address. Large memory devices
get additional mass erase operations to meet the spec.
\param erase_mode: ERASE_MASS, ERASE_MAIN, ERASE_SGMT
\param erase_address: address within the selected segment
*/
void TapDev::EraseFlash(address_t erase_address, EraseMode erase_mode)
{
	RedLedOff();

	(this->*traits_->fnEraseFlash)(erase_address, erase_mode);

	RedLedOn();
}

/*!
Release the target device from JTAG control.

\param address: 0xFFFE - perform Reset, load Reset Vector into PC
	0xFFFF - start execution at current PC position
	other  - load Address into PC
*/
void TapDev::ReleaseDevice(address_t address)
{
	RedGreenOff();

	/* delete all breakpoints */
	if (address == V_RESET)
		SetBreakpoint(-1, 0);

	(this->*traits_->fnReleaseDevice)(address);
}


/*!
Writes a value into a register of the target CPU
*/
bool TapDev::WriteReg(int reg, address_t value)
{
	return (this->*traits_->fnSetReg)(reg, value);
}









#if 0
/*!
Compares the computed PSA (Pseudo Signature Analysis) value to the PSA
value shifted out from the target device. It is used for very fast data
block write or erasure verification.

\param address: start of data
\param word_count: number of data
\param data: pointer to data, 0 for erase check
\return: 1 - comparison was successful; 0 - otherwise
*/
bool TapDev::VerifyPsa(uint32_t start_address, uint32_t length, const uint16_t *data)
{
	unsigned int psa_value;
	unsigned int index;

	/* Polynom value for PSA calculation */
	unsigned int polynom = 0x0805;
	/* Start value for PSA calculation */
	unsigned int psa_crc = start_address - 2;

	ExecutePOR();
	itf_->OnIrShift(IR_CNTRL_SIG_16BIT);
	itf_->OnDrShift16(0x2401);
	SetInstructionFetch();
	itf_->OnIrShift(IR_DATA_16BIT);
	itf_->OnDrShift16(0x4030);
	itf_->OnPulseTclk();
	itf_->OnDrShift16(start_address - 2);
	itf_->OnPulseTclk();
	itf_->OnPulseTclk();
	itf_->OnPulseTclk();
	itf_->OnIrShift(IR_ADDR_CAPTURE);
	itf_->OnDrShift16(0x0000);
	itf_->OnIrShift(IR_DATA_PSA);

	for (index = 0; index < length; index++)
	{
		/* Calculate the PSA value */
		if ((psa_crc & 0x8000) == 0x8000)
		{
			psa_crc ^= polynom;
			psa_crc <<= 1;
			psa_crc |= 0x0001;
		}
		else
			psa_crc <<= 1;

		if (data == 0)
			/* use erase check mask */
			psa_crc ^= 0xFFFF;
		else
			/* use data */
			psa_crc ^= data[index];

		/* Clock through the PSA */
		itf_->OnClockThroughPsa();
	}

	/* Read out the PSA value */
	itf_->OnIrShift(IR_SHIFT_OUT_PSA);
	psa_value = itf_->OnDrShift16(0x0000);
	itf_->OnSetTclk();

	return (psa_value == psa_crc) ? 1 : 0;
}
#endif

/*----------------------------------------------------------------------------*/
void TapDev::SingleStep()
{
	unsigned int loop_counter;

	/* CPU controls RW & BYTE */
	Play(kIrDr16(IR_CNTRL_SIG_16BIT, 0x3401));

	/* clock CPU until next instruction fetch cycle  */
	/* failure after 10 clock cycles                 */
	/* this is more than for the longest instruction */
	itf_->OnIrShift(IR_CNTRL_SIG_CAPTURE);
	for (loop_counter = 10; loop_counter > 0; loop_counter--)
	{
		itf_->OnPulseTclkN();
		if ((itf_->OnDrShift16(0x0000) & 0x0080) == 0x0080)
		{
			break;
		}
	}

	/* JTAG controls RW & BYTE */
#if 0
	itf_->OnIrShift(IR_CNTRL_SIG_16BIT);
	itf_->OnDrShift16(0x2401);
#else
	SetJtagRunRead();		// JTAG mode + CPU run + read
#endif

	if (loop_counter == 0)
	{
		/* timeout reached */
		Error() << "pif: single step failed\n";
		failed_ = true;
	}
}

/*----------------------------------------------------------------------------*/
bool TapDev::SetBreakpoint(int bp_num, address_t bp_addr)
{
	/* The breakpoint logic is explained in 'SLAU414c EEM.pdf' */
	/* A good overview is given with Figure 1-1                */
	/* MBx           is TBx         in EEM_defs.h              */
	/* CPU Stop      is BREAKREACT  in EEM_defs.h              */
	/* State Storage is STOR_REACT  in EEM_defs.h              */
	/* Cycle Counter is EVENT_REACT in EEM_defs.h              */

	unsigned int breakreact;

	if (bp_num >= 8)
	{
		/* there are no more than 8 breakpoints in EEM */
		Error() << "jtag_set_breakpoint: failed setting "
				   "breakpoint " << bp_num << " at " << f::X<4>(bp_addr) << '\n';
		failed_ = true;
		return false;
	}

	if (bp_num < 0)
	{
		/* disable all breakpoints by deleting the BREAKREACT
		 * register */
		Play(kIrDr16(IR_EMEX_DATA_EXCHANGE, BREAKREACT + WRITE));
		itf_->OnDrShift16(0x0000);
		return true;
	}

	/* set breakpoint */
	Play(kIrDr16(IR_EMEX_DATA_EXCHANGE, GENCTRL + WRITE));
	itf_->OnDrShift16(EEM_EN + CLEAR_STOP + EMU_CLK_EN + EMU_FEAT_EN);

	itf_->OnIrShift(IR_EMEX_DATA_EXCHANGE); //repeating may not needed
	itf_->OnDrShift16(8 * bp_num + MBTRIGxVAL + WRITE);
	itf_->OnDrShift16(bp_addr);

	itf_->OnIrShift(IR_EMEX_DATA_EXCHANGE); //repeating may not needed
	itf_->OnDrShift16(8 * bp_num + MBTRIGxCTL + WRITE);
	itf_->OnDrShift16(MAB + TRIG_0 + CMP_EQUAL);

	itf_->OnIrShift(IR_EMEX_DATA_EXCHANGE); //repeating may not needed
	itf_->OnDrShift16(8 * bp_num + MBTRIGxMSK + WRITE);
	itf_->OnDrShift16(NO_MASK);

	itf_->OnIrShift(IR_EMEX_DATA_EXCHANGE); //repeating may not needed
	itf_->OnDrShift16(8 * bp_num + MBTRIGxCMB + WRITE);
	itf_->OnDrShift16(1 << bp_num);

	/* read the actual setting of the BREAKREACT register         */
	/* while reading a 1 is automatically shifted into LSB        */
	/* this will be undone and the bit for the new breakpoint set */
	/* then the updated value is stored back                      */
	itf_->OnIrShift(IR_EMEX_DATA_EXCHANGE); //repeating may not needed
	breakreact = itf_->OnDrShift16(BREAKREACT + READ);
	breakreact += itf_->OnDrShift16(0x000);
	breakreact = (breakreact >> 1) | (1 << bp_num);
	itf_->OnDrShift16(BREAKREACT + WRITE);
	itf_->OnDrShift16(breakreact);
	return true;
}

/*----------------------------------------------------------------------------*/
bool TapDev::GetCpuState()
{
	itf_->OnIrShift(IR_EMEX_READ_CONTROL);

	if ((itf_->OnDrShift16(0x0000) & 0x0080) == 0x0080)
	{
		return true; /* halted */
	}
	else
	{
		return false; /* running */
	}
}

/*----------------------------------------------------------------------------*/
int TapDev::GetConfigFuses()
{
	itf_->OnIrShift(IR_CONFIG_FUSES);

	return itf_->OnDrShift8(0);
}

