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

	JtagId jtagId = gPlayer.cntrl_sig_capture();
	constexpr uint16_t jmbAddr = 0x0ff6;	// a harmless bus address

	uint16_t Rx_l = 0xFFFF;
	uint16_t Rx_h = 0xFFFF;
	static constexpr TapStep steps[] =
	{
		kTclk0
		, kIrData16Argv(kdTclk1)				// kIr(Ir::kData16Bit) + kTclk1 + kDr16(Mova)
		, kIrDr16(Ir::kCntrlSig16Bit, 0x1401)	// RD + JTAGCTRL + RELEASE_LBYTE:01
		, kIrData16Argv(kdTclkN, kdTclkN)		// kIr(Ir::kData16Bit) + kPulseTclkN + kDr16(jmbAddr) + kPulseTclkN
		, kDr16(0x3ffd)							// jmp $-4
		, kTclk0
		, kIrDr16(Ir::kCntrlSig16Bit, 0x0501)	// Set Word read CpuXv2
		, kIr(Ir::kDataCapture)
		, kTclk1
		, kDr16_ret(0)							// Rx_l = dr16(0)
		, kPulseTclkN
		, kDr16_ret(0)							// Rx_h = dr16(0)
		, kPulseTclkN
		, kPulseTclkN
		, kPulseTclkN
		, kTclk0
		, kIr(Ir::kDataCapture)
		, kTclk1
	};
	gPlayer.Play(steps, _countof(steps)
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
		gPlayer.Play(kIrDr16(Ir::kCntrlSig16Bit, 0x0501)); // Set Word read CpuXv2
		gPlayer.IHIL_Tclk(1);
		gPlayer.addr_capture();
	}
	gPlayer.pItf->OnReadJmbOut();

	return (((uint32_t)Rx_h << 16) | Rx_l) & 0xfffff;
}

