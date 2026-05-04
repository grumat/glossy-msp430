#pragma once

#include "TapPlayer.h"
#include <util/PingPongBuffer.h>

#if (OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_SPI_DMA) && !(defined OPT_JTAG_DMA_ISR)
#error Platform.h need to define the IRQ handler function in OPT_JTAG_DMA_ISR
#endif


//! JTAG TAP device
class JtagDev : public ITapInterface
{
public:
	JtagDev();
#if OPT_TX_BUFFER_CNT_
	static constexpr size_t kTxBufSize_ = OPT_TX_BUFFER_CNT_;
	// Ping-pong buffer for transmission frame
	static AnyPingPongBuffer<FrameBufEleType, kTxBufSize_> tx_buf_;
#endif	// OPT_TX_BUFFER_CNT_
#if OPT_RX_BUFFER_CNT_
	// Ping-pong buffer for reception frame
	static constexpr size_t kRxBufSize_ = OPT_RX_BUFFER_CNT_;
	static AnyPingPongBuffer<FrameBufEleType, kRxBufSize_> rx_buf_;
#endif	// OPT_RX_BUFFER_CNT_
#if OPT_AUX_BUFFER_CNT_
	static constexpr size_t kAuxBufSize_ = OPT_AUX_BUFFER_CNT_;
	static AnyPingPongBuffer<uint32_t, kAuxBufSize_> aux_buf_;
#endif	// OPT_AUX_BUFFER_CNT_

protected:
	virtual bool OnAnticipateTms() const override;
	virtual bool OnOpen() override;
	virtual void OnClose() override;
	virtual void OnConnectJtag(BusSpeed speed) override;
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
  BusSpeed speed_{BusSpeed::kSlowest};
  void SetSpeed(BusSpeed speed);

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

