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
	// Element count shared by every sub-buffer (TX, RX, [AUX]).
	static constexpr size_t kBufSize_ = OPT_BUFFER_CNT_;
#if OPT_BUFFER_LAYOUT_ == OPT_BUFFER_LAYOUT_PAIR
	// Combined TX+RX ping-pong; one Step() advances both halves atomically.
	//   buf_.GetNext1()    → TX render target (next frame)
	//   buf_.GetCurrent1() → TX live (DMA source)
	//   buf_.GetNext2()    → RX target for the next DMA receive
	//   buf_.GetCurrent2() → RX result of the most recently completed frame
	static AnyPingPongBuffer2<FrameBufEleType, kBufSize_, FrameBufEleType, kBufSize_> buf_;
#elif OPT_BUFFER_LAYOUT_ == OPT_BUFFER_LAYOUT_TRIPLE
	// Combined TX+RX+AUX ping-pong; one Step() advances all three halves atomically.
	//   buf_.GetNext3()    → AUX render target (TMS bit stream, next frame)
	//   buf_.GetCurrent3() → AUX live (DMA source)
	static AnyPingPongBuffer3<FrameBufEleType, kBufSize_, FrameBufEleType, kBufSize_, uint32_t, kBufSize_> buf_;
#else
#	error Unsupported OPT_BUFFER_LAYOUT_ value
#endif

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

