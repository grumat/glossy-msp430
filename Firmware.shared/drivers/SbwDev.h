#pragma once

#include "TapPlayer.h"
#include <util/PingPongBuffer.h>


#if OPT_INCLUDE_SBW_TIM_

/// Concrete ITapInterface backend for Spy-Bi-Wire (SBW).
///
/// `SbwDev.tim.cpp` is the only translation unit, selected at compile time
/// by `OPT_SBW_IMPLEMENTATION == OPT_SBW_IMPL_TIM`. Method semantics match
/// the `ITapInterface` contract; the comments below only document
/// backend-specific behaviour worth knowing at the call site. See
/// ITapInterface for the per-method protocol description.
///
/// SBW frame model: each logical JTAG bit expands to 3 SBWCLK cycles
/// (TMS, TDI, TDO-sample). See `Firmware.shared/util/TimSbw.h` for the
/// encoding details. The wire-level driver is single-pin bidirectional on
/// SBWTDIO; mid-frame turnaround is driven from DMA scripts.
///
/// Async note: the `OnXxxShift()` methods return a `JtagPending<T>` (see
/// `util/JtagPending.h`); they kick off the SBW transport DMA and return
/// immediately. The previous shift's DMA is drained inside the new shift
/// before the new frame is started, so adjacent shifts overlap render(N+1)
/// with DMA(N) — same contract as JtagDev.
///
/// Resource model: SbwDev and JtagDev share TIM1, GPIO and DMA channels and
/// cannot run concurrently. TapMcu::Open() picks one protocol for the session
/// and calls exactly one driver's Init() — see "Init() is sovereign" in
/// `.claude/docs/drivers/TIM_SBW_DRIVER.md`.
class SbwDev : public ITapInterface
{
public:
	SbwDev();
	/// Element count shared by every sub-buffer (BSRR script, IDR samples,
	/// direction script). Sized by OPT_SBW_BUFFER_CNT_; large enough to hold
	/// `3 × kJtagBitsMax` SBWCLK cycles for the longest scan.
	static constexpr size_t kBufSize_ = OPT_SBW_BUFFER_CNT_;
	// Combined BSRR-script + IDR-sample ping-pong; one Step() advances both halves.
	//   buf.GetNext1()    → BSRR script render target (next frame)
	//   buf.GetCurrent1() → BSRR script live (DMA source)
	//   buf.GetNext2()    → IDR sample target for the next DMA receive
	//   buf.GetCurrent2() → IDR sample result of the most recently completed frame
	static AnyPingPongBuffer2<uint32_t, kBufSize_, uint32_t, kBufSize_> buf;

protected:
	/// Bring the backend's peripherals up (TIM1, three DMA channels, GPIO AF,
	/// SBWTDIO direction-mux pin). Sovereign: unconditionally reconfigures
	/// every resource it owns regardless of prior owner (JtagDev).
	virtual bool OnOpen() override;

	/// Tear the backend down — DMA off, TIM1 off, SBW bus drivers tri-stated.
	virtual void OnClose() override;
	/// Acquire the SBW bus and store the requested `speed` in `speed_`.
	/// Reprograms TIM1 prescaler via SetSpeed() (5 MHz MSP430 ceiling).
	virtual void OnConnectJtag(BusSpeed speed) override;
	/// Close state: clean SBW exit (TEST low hold), then DRIVE the bus idle level
	/// via the buffers (SbwBusClose) — buffers stay enabled, no Hi-Z.
	virtual void OnReleaseJtag() override;
	/// Init state: release the SBW buffers to Hi-Z (SbwBusOff, BusState::kStandby).
	/// Called from OnClose(); not used inside the acquisition retry loop.
	virtual void OnReleaseDriver() override;

	/// Apply the SBW entry sequence on RST/TEST (reshaped vs 4-wire JTAG —
	/// see slau320 SBW timing diagrams).
	virtual void OnEnterTap(bool rst_low) override;
	/// Force the TAP back to Test-Logic-Reset via the GoIdle SBW frame,
	/// then return to Run-Test/Idle.
	virtual void OnResetTap() override;

	/// IR shift; backend renders the BSRR + direction scripts into
	/// `buf.GetNext1()`, kicks off the three DMAs, and returns a
	/// `JtagPending` pointing at the in-flight frame's sample slot in
	/// `buf.GetCurrent2()`. Resolution waits on DMA TC and decodes TDO.
	virtual JtagPending<uint8_t>  OnIrShift(Ir instr) override;
	virtual JtagPending<uint8_t>  OnDrShift8(uint8_t) override;
	virtual JtagPending<uint16_t> OnDrShift16(uint16_t) override;
	virtual JtagPending<uint32_t> OnDrShift20(uint32_t) override;
	virtual JtagPending<uint32_t> OnDrShift32(uint32_t) override;
	// NOTE: SBW's per-frame RX buffer is uint32_t (IDR samples), but
	// ITapInterface's virtuals return JtagPending<T> which stores a uint8_t*.
	// SbwDev.tim.cpp reinterpret_casts the buffer pointer at construction;
	// the decode lambda casts back to const uint32_t* before reading.
	/// Polls IsInstrLoad() up to 10 times, pulsing TCLK between attempts.
	virtual bool OnInstrLoad() override;

	/// TCLK helpers. On SBW, TCLK is embedded as the TDI value in the
	/// per-bit frame — set/clear updates the latched TCLK state used by
	/// subsequent shifts; pulse emits a short SBW frame to toggle it.
	virtual void OnSetTclk() override;
	virtual void OnClearTclk() override;
	virtual void OnPulseTclk() override;
	virtual void OnPulseTclkN() override;
	/// Flash-strobe path: drives TCLK at the Gen1/Gen2 erase/write rate.
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
	/// Reprogram the TIM1 prescaler for the requested grade via the chosen
	/// TimSbwInit_N::ApplySpeed().
	void SetSpeed(BusSpeed speed);

private:
	/// One-shot probe: shifts Ir::kCntrlSigCapture and checks the
	/// (kRead | kInstrLoad) flag pair in the returned control-signal word.
	bool IsInstrLoad();

#if OPT_SBW_TEST_WITH_LOGIC_ANALYZER_
	/// Bench probe (gated by OPT_STARTUP_SBW_LA_WAVEFORM): the SBW analogue of
	/// JtagDev::DoLogicAnalyzerTest. Called at the end of OnConnectJtag on the
	/// autonomous SBW open; emits a few reference IR/DR frames then the flash
	/// TCLK-strobe burst (OnFlashTclk) for a logic-analyzer capture, then halts.
	[[noreturn]] void DoLogicAnalyzerTest();
#endif

#if OPT_SBW_TDO_SETTLE_SWEEP_
	/// Bench probe (default-off, gated by OPT_SBW_TDO_SETTLE_SWEEP_): enter the TAP,
	/// re-init TIM1 to a high-multiplier / low-frequency geometry, then sweep the
	/// TDO sample compare across the low phase while reading the JTAG ID, tracing
	/// ok/total per phase over TRACESWO to measure T_settle. Never returns (halts).
	/// Called at the end of OnConnectJtag(). See SBW_SPEED_TIMING_MODEL.md.
	[[noreturn]] void DoTdoSettleSweep();
#endif
};

#endif // OPT_INCLUDE_SBW_TIM_
