/* MSPDebug - debugging tool for MSP430 MCUs
 * Copyright (C) 2009-2012 Daniel Beer
 * Copyright (C) 2012 Peter Bägel
 * Copyright (C) 2021 Mathias Gruber
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

#pragma once

#include "TapDev.h"

//! JTAG TAP device
class JtagDev : public ITapInterface
{
protected:
	virtual bool OnOpen() override;
	virtual void OnClose() override;
	virtual void OnConnectJtag() override;
	virtual void OnReleaseJtag() override;

	virtual void OnEnterTap() override;
	virtual void OnResetTap() override;

	virtual uint8_t OnIrShift(uint8_t byte) override;
	virtual uint8_t OnDrShift8(uint8_t) override;
	virtual uint16_t OnDrShift16(uint16_t) override;
	virtual uint32_t OnDrShift20(uint32_t) override;
	virtual bool OnInstrLoad() override;

	//virtual void OnClockThroughPsa() override;

	virtual void OnSetTclk() override;
	virtual void OnClearTclk() override;
	virtual void OnPulseTclk() override;
	virtual void OnPulseTclk(int count) override;
	virtual void OnPulseTclkN() override;
	virtual void OnFlashTclk(uint32_t min_pulses) override;

	virtual uint32_t OnReadJmbOut() override;
	virtual bool OnWriteJmbIn16(uint16_t data) override;

	virtual ITapInterface &OnStetupArchitecture(ChipInfoDB::CpuArchitecture arch) override { mspArch_ = arch; return *this; }

private:
	bool IsInstrLoad();

private:
	ChipInfoDB::CpuArchitecture mspArch_;
};

