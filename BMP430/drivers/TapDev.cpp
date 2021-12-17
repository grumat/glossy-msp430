
#include "stdproj.h"

#include "TapDev.h"
#include "eem_defs.h"

#include "TapDev430X.h"
#include "TapDev430Xv2_1377.h"


TapDev430 msp430legacy_;
TapDev430X msp430X_;
TapDev430Xv2 msp430Xv2_;
TapDev430Xv2_1377 msp430Xv2_1377_;


// Singleton for the JTAG device and helper functions
TapDev g_JtagDev;


/**************************************************************************************/
/* GENERAL SUPPORT FUNCTIONS                                                          */
/**************************************************************************************/


bool TapDev::Open(ITapInterface &itf)
{
	g_Player.itf_ = &itf;
	traits_ = &msp430legacy_;
	failed_ = !itf.OnOpen();
	issue_1377_ = true;
	fast_flash_ = false;
	return (failed_ == false);
}


void TapDev::Close()
{
	g_Player.itf_->OnClose();
	traits_ = &msp430legacy_;
}


/* Take target device under JTAG control.
 * Disable the target watchdog.
 * return: 0 - fuse is blown
 *        >0 - jtag id
 */
JtagId TapDev::Init()
{
	failed_ = true;
	issue_1377_ = true;
	fast_flash_ = false;
	core_id_.Init();
	traits_ = &msp430legacy_;

	// see GetCoreID()
	size_t tries = 0;
	for (tries = 0; tries < kMaxEntryTry; ++tries)
	{
		// release JTAG/TEST signals to safely reset the test logic
		g_Player.itf_->OnReleaseJtag();
		// establish the physical connection to the JTAG interface
		g_Player.itf_->OnConnectJtag();
		__NOP();
		// Apply again 4wire/SBW entry Sequence.
		g_Player.itf_->OnEnterTap();
		// reset TAP state machine -> Run-Test/Idle
		g_Player.itf_->OnResetTap();
		// shift out JTAG ID
		core_id_.jtag_id_ = (JtagId)g_Player.IR_Shift(IR_CNTRL_SIG_CAPTURE);
		__NOP();
		// break if a valid JTAG ID is being returned
		if (core_id_.IsMSP430())
			break;
	}
	if (tries == kMaxEntryTry)
	{
		Error() << "jtag_init: no device found\n";
		core_id_.jtag_id_ = kInvalid;
		return kInvalid;
	}

	// Check fuse
	if (IsFuseBlown())
	{
		Error() << "jtag_init: fuse is blown\n";
		return kInvalid;
	}
	/*
	** Before a database lookup we cannot be more specific, so load a general 
	** compatible function set.
	*/
	traits_ = core_id_.IsXv2() ? (ITapDev *)&msp430Xv2_ : (ITapDev *)&msp430legacy_;
	// Capture device into JTAG mode
	if (!traits_->GetDevice(core_id_))
	{
		Error() << "jtag_init: invalid jtag_id: 0x" << f::X<2>(core_id_.jtag_id_) << '\n';
		core_id_.Init();
		return kInvalid;
	}
	// Forward detect CPUX devices, before database lookup
	if (core_id_.coreip_id_ == 0 && ChipProfile::IsCpuX_ID(core_id_.device_id_))
		traits_ = &msp430X_;
	//traits_->SyncJtag();
	cpu_ctx_.jtag_id_ = core_id_.jtag_id_;
	__NOP();
	ChipProfile tmp;
	tmp.DefaultMcuXv2();
	traits_->SyncJtagAssertPorSaveContext(cpu_ctx_, tmp);

	return core_id_.jtag_id_;
}


bool TapDev::StartMcu(ChipInfoDB::CpuArchitecture arch, bool fast_flash, bool issue_1377)
{
	g_Player.itf_ = &g_Player.itf_->OnStetupArchitecture(arch);
	fast_flash_ = fast_flash;
	issue_1377_ = issue_1377;

	switch (arch)
	{
	case ChipInfoDB::kCpuXv2:
		traits_ = issue_1377 ? (ITapDev*)&msp430Xv2_1377_ : (ITapDev *)&msp430Xv2_;
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
	if(core_id_.IsXv2())
	{
		/*
		** MSP430F5418A does not like any read on this area without the use of the PC, so
		** we need to use the IR_DATA_QUICK to read this area.
		** This is nowhere described and costs me many wasted hours...
		*/
		//ReadWordsXv2_slau320aj(id_data_addr_, (uint16_t *)buf, words < 8 ? words : 8);
		// Now we should get a valid read
		return msp430Xv2_.ReadWords(core_id_.id_data_addr_, (uint16_t *)buf, words);
		//return ReadWordsXv2_slau320aj(id_data_addr_, (uint16_t *)buf, words);
	}
	else
	{
		//msp430legacy_.ReadWords(id_data_addr_, (uint16_t *)buf, words < 8 ? words : 8);
		//ReadWords_slau320aj(id_data_addr_, (uint16_t *)buf, words < 8 ? words : 8);
		// Now we should get a valid read
		return msp430legacy_.ReadWords(core_id_.id_data_addr_, (uint16_t *)buf, words);
		//return ReadWords_slau320aj(id_data_addr_, (uint16_t *)buf, words);
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
		if (g_Player.Play(kIrDr16(IR_CNTRL_SIG_CAPTURE, 0xAAAA)) == 0x5555)
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

	traits_->ReadWords(address, buf, word_count);
}


uint16_t TapDev::ReadWord(address_t address)
{
	failed_ = false;

	// 16-bit aligned address required
	assert((address & 1) == 0);

	return traits_->ReadWord(address);
}


bool TapDev::WriteWord(address_t address, uint16_t data)
{
	failed_ = false;

	// 16-bit aligned address required
	assert((address & 1) == 0);

	return traits_->WriteWord(address, data);
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

	return traits_->WriteWords(address, data, length);
}


bool TapDev::SetPC(address_t address)
{
	return traits_->SetPC(address);
}

/* Programs/verifies an array of words into a FLASH by using the
 * FLASH controller. The JTAG FLASH register isn't needed.
 * address: start in FLASH
 * word_count   : number of words
 * data         : pointer to data
 */
bool TapDev::WriteFlash(address_t address, const uint16_t *data, uint32_t word_count)
{
	bool res = traits_->WriteFlash(address, data, word_count);
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
	traits_->EraseFlash(erase_address, (uint16_t)erase_mode, (uint16_t)(erase_mode >> 16));
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

	traits_->ReleaseDevice(address);
}


/*!
Writes a value into a register of the target CPU
*/
bool TapDev::WriteReg(int reg, address_t value)
{
	if(reg == 0)
		return traits_->SetPC(value);
	return traits_->SetReg(reg, value);
}










/**************************************************************************************/
/* SUPPORT METHODS                                                                    */
/**************************************************************************************/


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
	g_Player.Play(kIrDr16(IR_CNTRL_SIG_16BIT, 0x3401));

	/* clock CPU until next instruction fetch cycle  */
	/* failure after 10 clock cycles                 */
	/* this is more than for the longest instruction */
	g_Player.itf_->OnIrShift(IR_CNTRL_SIG_CAPTURE);
	for (loop_counter = 10; loop_counter > 0; loop_counter--)
	{
		g_Player.itf_->OnPulseTclkN();
		if ((g_Player.itf_->OnDrShift16(0x0000) & 0x0080) == 0x0080)
		{
			break;
		}
	}

	/* JTAG controls RW & BYTE */
#if 0
	itf_->OnIrShift(IR_CNTRL_SIG_16BIT);
	itf_->OnDrShift16(0x2401);
#else
	g_Player.Play(TapPlayer::kSetJtagRunRead_);		// JTAG mode + CPU run + read
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
		g_Player.Play(kIrDr16(IR_EMEX_DATA_EXCHANGE, BREAKREACT + WRITE));
		g_Player.itf_->OnDrShift16(0x0000);
		return true;
	}

	/* set breakpoint */
	g_Player.Play(kIrDr16(IR_EMEX_DATA_EXCHANGE, GENCTRL + WRITE));
	g_Player.itf_->OnDrShift16(EEM_EN + CLEAR_STOP + EMU_CLK_EN + EMU_FEAT_EN);

	g_Player.itf_->OnIrShift(IR_EMEX_DATA_EXCHANGE); //repeating may not needed
	g_Player.itf_->OnDrShift16(8 * bp_num + MBTRIGxVAL + WRITE);
	g_Player.itf_->OnDrShift16(bp_addr);

	g_Player.itf_->OnIrShift(IR_EMEX_DATA_EXCHANGE); //repeating may not needed
	g_Player.itf_->OnDrShift16(8 * bp_num + MBTRIGxCTL + WRITE);
	g_Player.itf_->OnDrShift16(MAB + TRIG_0 + CMP_EQUAL);

	g_Player.itf_->OnIrShift(IR_EMEX_DATA_EXCHANGE); //repeating may not needed
	g_Player.itf_->OnDrShift16(8 * bp_num + MBTRIGxMSK + WRITE);
	g_Player.itf_->OnDrShift16(NO_MASK);

	g_Player.itf_->OnIrShift(IR_EMEX_DATA_EXCHANGE); //repeating may not needed
	g_Player.itf_->OnDrShift16(8 * bp_num + MBTRIGxCMB + WRITE);
	g_Player.itf_->OnDrShift16(1 << bp_num);

	/* read the actual setting of the BREAKREACT register         */
	/* while reading a 1 is automatically shifted into LSB        */
	/* this will be undone and the bit for the new breakpoint set */
	/* then the updated value is stored back                      */
	g_Player.itf_->OnIrShift(IR_EMEX_DATA_EXCHANGE); //repeating may not needed
	breakreact = g_Player.itf_->OnDrShift16(BREAKREACT + READ);
	breakreact += g_Player.itf_->OnDrShift16(0x000);
	breakreact = (breakreact >> 1) | (1 << bp_num);
	g_Player.itf_->OnDrShift16(BREAKREACT + WRITE);
	g_Player.itf_->OnDrShift16(breakreact);
	return true;
}

/*----------------------------------------------------------------------------*/
bool TapDev::GetCpuState()
{
	g_Player.itf_->OnIrShift(IR_EMEX_READ_CONTROL);

	if ((g_Player.itf_->OnDrShift16(0x0000) & 0x0080) == 0x0080)
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
	g_Player.itf_->OnIrShift(IR_CONFIG_FUSES);

	return g_Player.itf_->OnDrShift8(0);
}

