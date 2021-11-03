#include "stdproj.h"

#include "TapDev.h"
#include "eem_defs.h"



/**************************************************************************************/
/* TRAITS FUNCTION TABLE                                                              */
/**************************************************************************************/

const TapDev::CpuTraitsFuncs TapDev::msp430Xv2_1377_ =
{
	.fnSetPC = &TapDev::SetPcXv2_slau320aj
	, .fnSetReg = &TapDev::SetRegXv2_uif
	, true
	, .fnGetReg = &TapDev::GetRegXv2_uif_1377
	//
	, .fnReadWord = &TapDev::ReadWordXv2_slau320aj
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
/* MCU VERSION-RELATED REGISTER GET/SET METHODS                                       */
/**************************************************************************************/

uint32_t TapDev::GetRegXv2_uif_1377(uint8_t reg)
{
	const uint16_t Mova = 0x0060
		| ((uint16_t)reg << 8) & 0x0F00;

	JtagId jtagId = cntrl_sig_capture();
	constexpr uint16_t jmbAddr = 0x0ff6;	// a harmless bus address

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
		, kSetWordReadXv2_						// Set Word read CpuXv2
		, kIr(IR_DATA_CAPTURE)
		, kTclk1
		, kDr16_ret(0)							// Rx_l = dr16(0)
		, kPulseTclkN
		, kDr16_ret(0)							// Rx_h = dr16(0)
		, kPulseTclkN
		, kPulseTclkN
		, kPulseTclkN
		, kTclk0
		, kIr(IR_DATA_CAPTURE)
		, kTclk1
	};
	Play(steps, _countof(steps)
		 , Mova
		 , jmbAddr
		 , &Rx_l
		 , &Rx_h
	);

	if (jtagId == kMsp_91
		|| jtagId == kMsp_98
		|| jtagId == kMsp_99)
	{
		// Set PC to "safe" address
		SetPcXv2_slau320aj(SAFE_PC_ADDRESS);
		SetWordReadXv2();			// Set Word read CpuXv2
		IHIL_Tclk(1);
		addr_capture();
	}
	itf_->OnReadJmbOut();

	return (((uint32_t)Rx_h << 16) | Rx_l) & 0xfffff;
}

