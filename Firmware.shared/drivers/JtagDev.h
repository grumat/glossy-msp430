#pragma once

#include "TapPlayer.h"
#include <util/PingPongBuffer.h>

#if (OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_SPI_DMA) && !(defined OPT_JTAG_DMA_ISR)
#error Platform.h need to define the IRQ handler function in OPT_JTAG_DMA_ISR
#endif

#if defined OPT_INCLUDE_JTAG_SPI_
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
	JtagDev();
#if defined OPT_INCLUDE_JTAG_SPI_
	static JtagPacketBuffer	tx_buf_;
	static JtagPacketBuffer	rx_buf_;
#endif
	static constexpr size_t kPingPongBufSize_ = 40;
	static uint32_t read_buf_[kPingPongBufSize_];

protected:
	virtual bool OnAnticipateTms() const override;
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
	
protected:
	void OpenCommon_1();
	void OpenCommon_2();

private:
	bool IsInstrLoad();
#if OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_SPI_DMA
	static void IRQHandler() asm(OPT_JTAG_DMA_ISR) OPTIMIZED __attribute__((interrupt("IRQ")));
	friend class DmaMode_;
#endif
};

#if OPT_TMS_VERY_HIGH_CLOCK != 9
// Very high SPI clocks, requires this instance for pulse anticipation
class JtagDevVhc : public JtagDev
{
public:
	JtagDevVhc();

protected:
	virtual bool OnAnticipateTms() const override;
	virtual uint8_t OnIrShift(uint8_t byte) override;
	virtual uint8_t OnDrShift8(uint8_t) override;
	virtual uint16_t OnDrShift16(uint16_t) override;
	virtual uint32_t OnDrShift20(uint32_t) override;
	virtual uint32_t OnDrShift32(uint32_t) override;
};
#endif


#if OPT_JTAG_SPEED_SEL

// 2nd Speed grade
class JtagDev_2
#if OPT_TMS_VERY_HIGH_CLOCK <= 2
	: public JtagDevVhc
#else
	: public JtagDev
#endif
{
public:
	JtagDev_2();
protected:
	virtual bool OnOpen() override;
};

// 3rd Speed grade
class JtagDev_3
#if OPT_TMS_VERY_HIGH_CLOCK <= 3
	: public JtagDevVhc
#else
	: public JtagDev
#endif
{
public:
	JtagDev_3();
protected:
	virtual bool OnOpen() override;
};

// 4th Speed grade
class JtagDev_4
#if OPT_TMS_VERY_HIGH_CLOCK <= 4
	: public JtagDevVhc
#else
	: public JtagDev
#endif
{
public:
	JtagDev_4();
protected:
	virtual bool OnOpen() override;
};

class JtagDev_5
#if OPT_TMS_VERY_HIGH_CLOCK <= 5
	: public JtagDevVhc
#else
	: public JtagDev
#endif
{
public:
	JtagDev_5();
protected:
	virtual bool OnOpen() override;
};

#endif // OPT_JTAG_SPEED_SEL

