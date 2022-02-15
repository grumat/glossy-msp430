#pragma once

#include "TapDev430Xv2.h"

class TapDev430Xv2_1377 : public TapDev430Xv2
{
public:
	// Reads a CPU register value
	virtual uint32_t GetReg(uint8_t reg) override;
};

