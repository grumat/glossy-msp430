
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
	issue_1377_ = true;
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
	issue_1377_ = true;
	fast_flash_ = false;
	jtag_id_ = kInvalid;
	coreip_id_ = 0;
	id_data_addr_ = 0x0FF0;
	ip_pointer_ = 0;
	traits_ = &msp430legacy_;

	// see GetCoreID()
	size_t tries = 0;
	for (tries = 0; tries < kMaxEntryTry; ++tries)
	{
		// release JTAG/TEST signals to safely reset the test logic
		itf_->OnReleaseJtag();
		// establish the physical connection to the JTAG interface
		itf_->OnConnectJtag();
		// Apply again 4wire/SBW entry Sequence.
		itf_->OnEnterTap();
		// reset TAP state machine -> Run-Test/Idle
		itf_->OnResetTap();
		// shift out JTAG ID
		jtag_id_ = (JtagId)IR_Shift(IR_CNTRL_SIG_CAPTURE);
		// break if a valid JTAG ID is being returned
		if (IsMSP430())
			break;
	}
	if (tries == kMaxEntryTry)
	{
		Error() << "jtag_init: no device found\n";
		jtag_id_ = kInvalid;
		return kInvalid;
	}

	// Check fuse
	if (IsFuseBlown())
	{
		Error() << "jtag_init: fuse is blown\n";
		return kInvalid;
	}

	// Set device into JTAG mode
	if(IsXv2())
		GetDeviceXv2();
	else
		GetDevice();
	if (IsMSP430() == false)
	{
		Error() << "jtag_init: invalid jtag_id: 0x" << f::X<2>(jtag_id_) << '\n';
		jtag_id_ = kInvalid;
		return kInvalid;
	}
	if (IsXv2())
		SyncJtag_AssertPor();
	else
		ExecutePOR();

	return jtag_id_;
}


bool TapDev::StartMcu(ChipInfoDB::CpuArchitecture arch, bool fast_flash, bool issue_1377)
{
	itf_ = &itf_->OnStetupArchitecture(arch);
	fast_flash_ = fast_flash;
	issue_1377_ = issue_1377;

	switch (arch)
	{
	case ChipInfoDB::kCpuXv2:
		traits_ = issue_1377 ? &msp430Xv2_1377_ : &msp430Xv2_;
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
		//ReadWordsXv2_slau320aj(id_data_addr_, (uint16_t *)buf, words < 8 ? words : 8);
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
This function checks if the JTAG access security fuse is blown.

\return: true - fuse is blown; false - otherwise
*/
bool TapDev::IsFuseBlown()
{
	unsigned int loop_counter;

	// First trial could be wrong
	for (loop_counter = 3; loop_counter > 0; loop_counter--)
	{
		if (Play(kIrDr16(IR_CNTRL_SIG_CAPTURE, 0xAAAA)) == 0x5555)
			return true;	// Fuse is blown
	}
	return false;			// Fuse is not blown
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
void TapDev::EraseFlash(address_t erase_address, EraseModeFctl erase_mode)
{
	(this->*traits_->fnEraseFlash)(erase_address, (uint16_t)erase_mode, (uint16_t)(erase_mode >> 16));
}

/*!
Release the target device from JTAG control.

\param address: 0xFFFE - perform Reset, load Reset Vector into PC
	0xFFFF - start execution at current PC position
	other  - load Address into PC
*/
void TapDev::ReleaseDevice(address_t address)
{
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

