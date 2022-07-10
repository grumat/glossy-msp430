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

#include "TapPlayer.h"


#if OPT_JTAG_USING_SPI
union ALIGNED JtagPacketBuffer
{
	uint8_t bytes[sizeof(uint64_t)];
	uint16_t words[sizeof(uint64_t)/ sizeof(uint16_t)];
	uint32_t dwords[sizeof(uint64_t)/ sizeof(uint32_t)];
	uint64_t qword;
};
#endif

//! JTAG TAP device
class JtagDev : public ITapInterface
{
public:
#if OPT_JTAG_USING_SPI
	static JtagPacketBuffer	tx_buf_;
	static JtagPacketBuffer	rx_buf_;
#endif

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
	virtual uint32_t OnDrShift32(uint32_t) override;
	virtual bool OnInstrLoad() override;

	//virtual void OnClockThroughPsa() override;

	virtual void OnSetTclk() override;
	virtual void OnClearTclk() override;
	virtual void OnPulseTclk() override;
	virtual void OnPulseTclk(int count) override;
	virtual void OnPulseTclkN() override;
	virtual void OnFlashTclk(uint32_t min_pulses) override;
	virtual void OnTclk(DataClk tclk) override;
	virtual uint16_t OnData16(DataClk clk0, uint16_t data, DataClk clk1) override;

	virtual uint32_t OnReadJmbOut() override;
	virtual bool OnWriteJmbIn16(uint16_t data) override;

private:
	bool IsInstrLoad();
#if OPT_JTAG_USING_SPI && OPT_JTAG_USING_DMA
	static void IRQHandler() asm(OPT_JTAG_DMA_ISR) OPTIMIZED __attribute__((interrupt("IRQ")));
	friend class DmaMode_;
#endif
};


#if OPT_JTAG_SPEED_SEL

// 2nd Speed grade
class JtagDev_2 : public JtagDev
{
protected:
	virtual bool OnOpen() override;
};

// 3rd Speed grade
class JtagDev_3 : public JtagDev
{
protected:
	virtual bool OnOpen() override;
};

// 4th Speed grade
class JtagDev_4 : public JtagDev
{
protected:
	virtual bool OnOpen() override;
};

// 5th Speed grade
class JtagDev_5 : public JtagDev
{
protected:
	virtual bool OnOpen() override;
	virtual uint8_t OnIrShift(uint8_t byte) override;
	virtual uint8_t OnDrShift8(uint8_t) override;
	virtual uint16_t OnDrShift16(uint16_t) override;
	virtual uint32_t OnDrShift20(uint32_t) override;
	virtual uint32_t OnDrShift32(uint32_t) override;
};

#endif // OPT_JTAG_SPEED_SEL

