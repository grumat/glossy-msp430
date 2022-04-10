#include "stdproj.h"

#include "TapDev430Xv2_1377.h"
#include "TapMcu.h"
#include "eem_defs.h"



/**************************************************************************************/
/* MCU VERSION-RELATED REGISTER GET/SET METHODS                                       */
/**************************************************************************************/

// Source: uif
uint32_t TapDev430Xv2_1377::GetReg(uint8_t reg)
{
	const uint16_t Mova = 0x0060
		| ((uint16_t)reg << 8) & 0x0F00;

	JtagId jtagId = g_Player.cntrl_sig_capture();
	constexpr uint16_t jmbAddr = 0x0ff6;	// a harmless bus address

	uint16_t Rx_l = 0xFFFF;
	uint16_t Rx_h = 0xFFFF;
	static constexpr TapStep steps[] =
	{
		kTclk0
		, kIrData16Argv(kdTclk1)				// kIr(IR_DATA_16BIT) + kTclk1 + kDr16(Mova)
		, kIrDr16(IR_CNTRL_SIG_16BIT, 0x1401)	// RD + JTAGCTRL + RELEASE_LBYTE:01
		, kIrData16Argv(kdTclkN, kdTclkN)		// kIr(IR_DATA_16BIT) + kPulseTclkN + kDr16(jmbAddr) + kPulseTclkN
		, kDr16(0x3ffd)							// jmp $-4
		, kTclk0
		, TapPlayer::kSetWordReadXv2_			// Set Word read CpuXv2
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
	g_Player.Play(steps, _countof(steps)
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
		TapDev430Xv2_1377::SetPC(SAFE_PC_ADDRESS);
		g_Player.SetWordReadXv2();			// Set Word read CpuXv2
		g_Player.IHIL_Tclk(1);
		g_Player.addr_capture();
	}
	g_Player.itf_->OnReadJmbOut();

	return (((uint32_t)Rx_h << 16) | Rx_l) & 0xfffff;
}

