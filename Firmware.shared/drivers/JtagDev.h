#pragma once

#include "TapPlayer.h"
#include <util/PingPongBuffer.h>


/// Concrete ITapInterface backend.
///
/// `JtagDev.dtrig.cpp` is the only translation unit, selected at compile time
/// by `OPT_JTAG_IMPLEMENTATION == OPT_JTAG_IMPL_DTRIG`. Method semantics match
/// the `ITapInterface` contract; the comments below only document
/// backend-specific behaviour worth knowing at the call site. See
/// ITapInterface for the per-method protocol description.
///
/// Async note: the `OnXxxShift()` methods return a `JtagPending<T>` (see
/// `util/JtagPending.h`); they kick off the SPI RX DMA and return immediately.
/// The previous shift's DMA is drained inside the new shift before the new
/// frame is started, so adjacent shifts overlap render(N+1) with DMA(N).
class JtagDev : public ITapInterface
{
public:
	JtagDev();
	/// Element count shared by every sub-buffer (TX, RX, [AUX]). Sized by the
	/// active OPT_BUFFER_CNT_; large enough to hold the longest JTAG frame
	/// the selected backend needs to render in one go.
	static constexpr size_t kBufSize_ = OPT_BUFFER_CNT_;
#if OPT_BUFFER_LAYOUT_ == OPT_BUFFER_LAYOUT_PAIR
	// Combined TX+RX ping-pong; one Step() advances both halves atomically.
	//   buf.GetNext1()    → TX render target (next frame)
	//   buf.GetCurrent1() → TX live (DMA source)
	//   buf.GetNext2()    → RX target for the next DMA receive
	//   buf.GetCurrent2() → RX result of the most recently completed frame
	static AnyPingPongBuffer2<FrameBufEleType, kBufSize_, FrameBufEleType, kBufSize_> buf;
#elif OPT_BUFFER_LAYOUT_ == OPT_BUFFER_LAYOUT_TRIPLE
	// Combined TX+RX+AUX ping-pong; one Step() advances all three halves atomically.
	//   buf.GetNext3()    → AUX render target (TMS bit stream, next frame)
	//   buf.GetCurrent3() → AUX live (DMA source)
	static AnyPingPongBuffer3<FrameBufEleType, kBufSize_, FrameBufEleType, kBufSize_, uint32_t, kBufSize_> buf;
#else
#	error Unsupported OPT_BUFFER_LAYOUT_ value
#endif

protected:
	/// Bring the backend's peripherals up (timers, DMA, SPI, GPIO AF). When
	/// `OPT_TEST_WITH_LOGIC_ANALYZER_` is enabled, dispatches to
	/// DoLogicAnalyzerTest() at the end so a logic analyzer can capture the
	/// reference IR/DR/TCLK waveform sequence; otherwise that helper is not
	/// compiled in.
	virtual bool OnOpen() override;

#if OPT_TEST_WITH_LOGIC_ANALYZER_
	/// Bench-only: drive a fixed IR/DR/TCLK sequence at the slowest grade so
	/// a logic analyzer can capture and validate the waveform. Tri-states the
	/// JTAG bus and halts in __WFI() — never returns.
	[[noreturn]] void DoLogicAnalyzerTest();
#endif
	/// Tear the backend down — DMA off, SPI off, JTAG bus drivers tri-stated.
	virtual void OnClose() override;
	/// Acquire the JTAG bus and store the requested `speed` in `speed_`.
	/// On the SPI backend this also latches `s_anticipate_clock` so subsequent
	/// IR/DR shifts compensate for the timer delay at top speed. Dtrig also
	/// reprograms the TIM1 / SPI dividers via SetSpeed().
	virtual void OnConnectJtag(BusSpeed speed) override;
	/// Close state: settle MOSI high, pull TEST/TMS to the idle level, then have
	/// the buffers *drive* the JTAG idle (JtagOff) — buffers stay enabled.
	virtual void OnReleaseJtag() override;
	/// Init state: release the buffers to Hi-Z (DriverOff, BusState::kStandby). Called
	/// from OnClose(); not used inside the acquisition retry loop.
	virtual void OnReleaseDriver() override;

	/// Apply the slau320aj fuse-check / TAP-entry pulse sequence on RST/TEST.
	virtual void OnEnterTap(bool rst_low) override;
	/// Force the TAP back to Test-Logic-Reset using the TMS auto-shaper, then
	/// return to Run-Test/Idle. Backends switch the TMS pin to the GPIO
	/// bit-bang path for the duration.
	virtual void OnResetTap() override;

	/// IR shift; backends render the frame into `buf.GetNext1()`, kick off
	/// the DMA, and return a `JtagPending` pointing at the in-flight frame's
	/// RX slot in `buf.GetCurrent2()`. Resolution waits on the DMA TC and
	/// decodes the captured TDO value.
	virtual JtagPending<uint8_t>  OnIrShift(Ir instr) override;
	virtual JtagPending<uint8_t>  OnDrShift8(uint8_t) override;
	virtual JtagPending<uint16_t> OnDrShift16(uint16_t) override;
	virtual JtagPending<uint32_t> OnDrShift20(uint32_t) override;
	virtual JtagPending<uint32_t> OnDrShift32(uint32_t) override;
	/// Polls IsInstrLoad() up to 10 times, pulsing TCLK between attempts.
	virtual bool OnInstrLoad() override;

	//virtual void OnClockThroughPsa() override;

	/// TCLK helpers. On SPI/Dtrig backends these flip PA5 between SPI-AF and
	/// GPIO-output via the HwMode FSM (`AcquireSpiClkMuted()`), so back-to-back
	/// TCLK calls do not pay the pin-mode flip twice per byte.
	virtual void OnSetTclk() override;
	virtual void OnClearTclk() override;
	virtual void OnPulseTclk() override;
	virtual void OnPulseTclkN() override;
	/// Flash-strobe path: drives JTDI at ~470 kHz (Gen1/Gen2 erase/write rate)
	/// via the JtclkWaveGen DMA timer chain.
	virtual void OnFlashTclk(uint32_t min_pulses) override;
	virtual void OnTclk(DataClk tclk) override;
	/// Composite IR(DATA_16BIT) + TCLK + DR16 + TCLK; the workhorse of the
	/// MSP430 data-bus access primitives.
	virtual uint16_t OnData16(DataClk clk0, uint16_t data, DataClk clk1) override;

	virtual uint32_t OnReadJmbOut() override;
	virtual bool OnWriteJmbIn16(uint16_t data) override;

protected:
	/// Currently active bus-speed grade, latched by OnConnectJtag().
	BusSpeed speed_{BusSpeed::kSlowest};
	/// Reprogram the timer/SPI dividers for the requested grade. Dtrig backend
	/// also updates `s_cnt_offset` to match the chosen DtrigInit_N::ApplySpeed().
	void SetSpeed(BusSpeed speed);

private:
	/// One-shot probe: shifts Ir::kCntrlSigCapture and checks the
	/// (kRead | kInstrLoad) flag pair in the returned control-signal word.
	bool IsInstrLoad();
};

