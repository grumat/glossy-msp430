#pragma once

#include "TapDev430Xv2.h"

class TapDev430Xv2_1377 : public TapDev430Xv2
{
	// VIRTUAL DESTRUCTOR IS NOT NECESSARY:
	// Instance of this objetc is **static** and will never be destroyed
	// since there is no "exit program" operation in a firmware.
	// This spares 2K of Flash + some more RAM

public:
	// Reads a CPU register value
	virtual uint32_t GetReg(uint8_t reg) override;
};

