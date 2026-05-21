
#include "stdproj.h"

#if OPT_INCLUDE_SBW_DTRIG_

#include "SbwDev.h"
#include "util/WaveSet.h"
#include "util/DtrigSbw.h"

using namespace Bmt::Dma;
using namespace Bmt::Timer;
using namespace JtagFrame;
using namespace WaveJtag;


// ── CNT offsets per speed grade (mirror DtrigJtag layout) ─────────────────────
// TODO (Issue 5): LA-calibrate these on real silicon. Defaults are safe starting
// points; the actual values depend on the SBW pin layout and DMA latency.
static constexpr uint16_t kDtrigSbwCntOffset_R = 0;	///< GoIdle frame
static constexpr uint16_t kDtrigSbwCntOffset_1 = 0;	///< 0.625 MHz
static constexpr uint16_t kDtrigSbwCntOffset_2 = 0;	///< 1.25 MHz
static constexpr uint16_t kDtrigSbwCntOffset_3 = 0;	///< 2.5 MHz
static constexpr uint16_t kDtrigSbwCntOffset_4 = 0;	///< 5 MHz (MSP430 ceiling)


// ── DtrigSbw type aliases ─────────────────────────────────────────────────────

/// Bit position of SBWDIO_In inside its GPIO->IDR register. SBWDIO_In = JTDO = PA6.
static constexpr uint8_t kSbwIdrBit_ = 6;

template <uint32_t Freq, Scan S, NumBits N>
using DtrigSbwImpl = DtrigSbw<
	  SysClk
	, kWaveSbwTimer
	, kWaveSbwClk
	, kWaveSbwDataTrig
	, kWaveSbwSampleTrig
	, SBWDIO					///< output data pin (PA7) — must expose kPin_
	, SbwIdrPort_A				///< GPIO holding the input bit
	, kSbwIdrBit_
	, DirPolicy_PA9_BsrrMux		///< PA9 hardware mux folded into BSRR
	, Freq, S, N
	, kWaveSbwCmpComplementary
>;

// Per-scan aliases at the slowest speed grade. ApplySpeed() bumps the actual
// TIM1 prescaler at runtime; the template parameter just picks the Init()
// baseline for the prescaler computation.
using DtrigSbwGoIdle = DtrigSbwImpl<SBW_Speed_1, Scan::kGoIdle, NumBits::kGoIdle>;
using DtrigSbwIr8    = DtrigSbwImpl<SBW_Speed_1, Scan::kIR,     NumBits::k8>;
using DtrigSbwDr8    = DtrigSbwImpl<SBW_Speed_1, Scan::kDR,     NumBits::k8>;
using DtrigSbwDr16   = DtrigSbwImpl<SBW_Speed_1, Scan::kDR,     NumBits::k16>;
using DtrigSbwDr20   = DtrigSbwImpl<SBW_Speed_1, Scan::kDR,     NumBits::k20>;
using DtrigSbwDr32   = DtrigSbwImpl<SBW_Speed_1, Scan::kDR,     NumBits::k32>;

// Per-grade Init-only instantiations: each sets TIM1 PSC for its grade.
using DtrigSbwInit_1 = DtrigSbwImpl<SBW_Speed_1, Scan::kDR, NumBits::k8>;
using DtrigSbwInit_2 = DtrigSbwImpl<SBW_Speed_2, Scan::kDR, NumBits::k8>;
using DtrigSbwInit_3 = DtrigSbwImpl<SBW_Speed_3, Scan::kDR, NumBits::k8>;
using DtrigSbwInit_4 = DtrigSbwImpl<SBW_Speed_4, Scan::kDR, NumBits::k8>;


// ── Static storage (mirrors JtagDev.dtrig.cpp s_cnt_offset / s_have_in_flight_) ──

// Active-grade CNT offset, updated by SetSpeed().
static uint16_t s_sbw_cnt_offset = kDtrigSbwCntOffset_1;

// True if there is an SBW frame DMA in flight that must be drained before the
// next Start(). Set inside each OnXxxShift() and cleared by SbwWaitTransfer().
// SBW and JTAG cannot be active at the same time, so this flag is independent
// of JtagDev's s_have_in_flight_.
static bool s_sbw_have_in_flight_ = false;

// SBW carries TCLK as the TDI bit value in the per-bit frame; OnSetTclk /
// OnClearTclk update this latch and the next shift fills head/tail bits with
// it (same role JTCLK::IsHigh() plays for JtagDev::OnIrShift).
static bool s_sbw_tclk_high = true;

// Per-scan direction script. Two CRx flips per frame (output→input at the
// start of the first TDO-sample cycle, input→output after the last cycle).
// Precomputed at Init() time once the DirPolicy is known.
// TODO (Issue 2): fill in from DirPolicy::kOutputCrx / kInputCrx.
static uint32_t s_sbw_dir_script[2] = { 0, 0 };


/// Drain the most-recently-issued SBW shift's DMAs, if one is in flight.
/// Called from `JtagPending::operator T()` / `Get()` and from the start of
/// each shift body (to wait on the previous frame). Idempotent.
void SbwWaitTransfer()
{
	if (!s_sbw_have_in_flight_)
		return;
	// Every DtrigSbwXxx::Wait() shares the same TIM1 auto-stop flag and the
	// same IDR DMA channel, so any one of them drains the in-flight frame.
	DtrigSbwDr8::Wait();
	s_sbw_have_in_flight_ = false;
}


// ── Constructors ──────────────────────────────────────────────────────────────

SbwDev::SbwDev()   {}


// ── SbwDev virtual method implementations ─────────────────────────────────────

bool SbwDev::OnOpen()
{
	// Sovereign Init: unconditionally reclaim TIM1, GPIO and DMA channels
	// regardless of whether JtagDev was the previous owner. See
	// "Init() is sovereign" in DTRIG_SBW_DRIVER.md.
	s_sbw_have_in_flight_ = false;
	s_sbw_tclk_high = true;
	s_sbw_cnt_offset = kDtrigSbwCntOffset_1;
	DtrigSbwDr8::ReleaseDma();		// clear stale DMA EN bits before re-Setup
	DtrigSbwInit_1::Init();			// GPIO AF, TIM1, both DMAs, DirPolicy
	return true;
}


void SbwDev::OnClose()
{
	SbwWaitTransfer();				// don't tear down mid-DMA
	DtrigSbwDr8::ReleaseDma();
	SetBusState(BusState::off);
}


void SbwDev::OnConnectJtag(BusSpeed speed)
{
	/*
	Workflow: Open -> ConnectJtag -> EnterTap -> ResetTap -> SBW mode ready
					  \_________/
	*/
	speed_ = speed;
	SetSpeed(speed);
	SetBusState(BusState::sbw);
	StopWatch().Delay<Msec(10)>();
}


void SbwDev::SetSpeed(BusSpeed speed)
{
	switch (speed)
	{
	case BusSpeed::kSlowest: s_sbw_cnt_offset = kDtrigSbwCntOffset_1; DtrigSbwInit_1::ApplySpeed(); break;
	case BusSpeed::kSlow:    s_sbw_cnt_offset = kDtrigSbwCntOffset_2; DtrigSbwInit_2::ApplySpeed(); break;
	case BusSpeed::kMedium:  s_sbw_cnt_offset = kDtrigSbwCntOffset_3; DtrigSbwInit_3::ApplySpeed(); break;
	case BusSpeed::kFast:
	case BusSpeed::kFastest: s_sbw_cnt_offset = kDtrigSbwCntOffset_4; DtrigSbwInit_4::ApplySpeed(); break;
	}
}


void SbwDev::OnReleaseJtag()
{
	SbwWaitTransfer();
	// TODO (Issue 5): drive a final TEST/RST low sequence per slau320 SBW
	// disconnect timing once we have bench access to validate the shape.
	SetBusState(BusState::off);
	StopWatch().Delay<Msec(10)>();
}


void SbwDev::OnResetTap()
{
	SbwWaitTransfer();				// drain any leftover async shift before reset
	uint32_t* tx = buf_.GetNext1();
	DtrigSbwGoIdle::DoGoIdle(tx);
	buf_.Step();
	uint32_t* rx = buf_.GetCurrent2();
	DtrigSbwGoIdle::Start(tx, rx, s_sbw_cnt_offset);
	DtrigSbwGoIdle::Wait();			// reset path is synchronous
	SetSpeed(speed_);
}


// ── Async-shift template (one frame, three DMAs) ──────────────────────────────
//
//   1. RenderTransaction → buf_.GetNext1()    ← CPU work, overlaps with previous DMA
//   2. SbwWaitTransfer()                       ← blocks on previous frame's TC
//   3. buf_.Step()                             ← advance ping-pong
//   4. R::Start(tx, dir, rx, cnt_offset)       ← arm 3 DMAs, kick TIM1, return
//   5. Return JtagPending pointing at the new frame's IDR-sample slot in rx.
//
// The lambda in the returned JtagPending blocks on its own frame's DMA via
// GetResult; if the caller drops the Pending, the next shift's step (2)
// covers the drain.


// rx is reinterpret_cast'd to uint8_t* to fit JtagPending's storage; each
// decode lambda casts it back to const uint32_t* via a void* hop before
// sampling IDR words. The void* hop avoids -Wcast-align warnings — the
// pointer is genuinely 4-byte aligned since it came from a uint32_t buffer.

JtagPending<uint8_t> SbwDev::OnIrShift(uint8_t instruction)
{
	using R = DtrigSbwIr8;
	uint32_t* tx = buf_.GetNext1();
	R::RenderTransaction(tx, s_sbw_tclk_high, instruction);
	SbwWaitTransfer();				// drain previous frame
	buf_.Step();
	uint32_t* rx = buf_.GetCurrent2();
	R::Start(tx, rx, s_sbw_cnt_offset);
	s_sbw_have_in_flight_ = true;
	return { reinterpret_cast<uint8_t*>(rx), +[](const uint8_t* p) -> uint8_t {
		auto q = static_cast<const uint32_t*>(static_cast<const void*>(p));
		return static_cast<uint8_t>(DtrigSbwIr8::GetResult(q));
	} };
}


JtagPending<uint8_t> SbwDev::OnDrShift8(uint8_t data)
{
	using R = DtrigSbwDr8;
	uint32_t* tx = buf_.GetNext1();
	R::RenderTransaction(tx, s_sbw_tclk_high, data);
	SbwWaitTransfer();
	buf_.Step();
	uint32_t* rx = buf_.GetCurrent2();
	R::Start(tx, rx, s_sbw_cnt_offset);
	s_sbw_have_in_flight_ = true;
	return { reinterpret_cast<uint8_t*>(rx), +[](const uint8_t* p) -> uint8_t {
		auto q = static_cast<const uint32_t*>(static_cast<const void*>(p));
		return static_cast<uint8_t>(DtrigSbwDr8::GetResult(q));
	} };
}


JtagPending<uint16_t> SbwDev::OnDrShift16(uint16_t data)
{
	using R = DtrigSbwDr16;
	uint32_t* tx = buf_.GetNext1();
	R::RenderTransaction(tx, s_sbw_tclk_high, data);
	SbwWaitTransfer();
	buf_.Step();
	uint32_t* rx = buf_.GetCurrent2();
	R::Start(tx, rx, s_sbw_cnt_offset);
	s_sbw_have_in_flight_ = true;
	return { reinterpret_cast<uint8_t*>(rx), +[](const uint8_t* p) -> uint16_t {
		auto q = static_cast<const uint32_t*>(static_cast<const void*>(p));
		return static_cast<uint16_t>(DtrigSbwDr16::GetResult(q));
	} };
}


JtagPending<uint32_t> SbwDev::OnDrShift20(uint32_t data)
{
	using R = DtrigSbwDr20;
	uint32_t* tx = buf_.GetNext1();
	R::RenderTransaction(tx, s_sbw_tclk_high, data);
	SbwWaitTransfer();
	buf_.Step();
	uint32_t* rx = buf_.GetCurrent2();
	R::Start(tx, rx, s_sbw_cnt_offset);
	s_sbw_have_in_flight_ = true;
	return { reinterpret_cast<uint8_t*>(rx), +[](const uint8_t* p) -> uint32_t {
		auto q = static_cast<const uint32_t*>(static_cast<const void*>(p));
		return DtrigSbwDr20::GetResult(q);
	} };
}


JtagPending<uint32_t> SbwDev::OnDrShift32(uint32_t data)
{
	using R = DtrigSbwDr32;
	uint32_t* tx = buf_.GetNext1();
	R::RenderTransaction(tx, s_sbw_tclk_high, data);
	SbwWaitTransfer();
	buf_.Step();
	uint32_t* rx = buf_.GetCurrent2();
	R::Start(tx, rx, s_sbw_cnt_offset);
	s_sbw_have_in_flight_ = true;
	return { reinterpret_cast<uint8_t*>(rx), +[](const uint8_t* p) -> uint32_t {
		auto q = static_cast<const uint32_t*>(static_cast<const void*>(p));
		return DtrigSbwDr32::GetResult(q);
	} };
}


// ── TCLK helpers ──────────────────────────────────────────────────────────────
// SBW does not have a dedicated TCLK pin — TCLK is the TDI value held by the
// per-bit SBW frame. OnSet/OnClearTclk update the latch s_sbw_tclk_high which
// the next RenderTransaction folds into the head/tail fill bits. OnPulseTclk
// emits a short SBW frame that toggles TCLK then returns it to its prior value.

void SbwDev::OnSetTclk()
{
	s_sbw_tclk_high = true;
	// TODO (Issue 2): if the bus is idle, drive a settle frame so the wire
	// reflects the latched TCLK level before the next shift starts.
}


void SbwDev::OnClearTclk()
{
	s_sbw_tclk_high = false;
	// TODO (Issue 2): same as OnSetTclk — emit a settle frame if needed.
}


void SbwDev::OnPulseTclk()
{
	// TODO (Issue 2): one positive-going TCLK pulse via a 1-bit SBW frame.
	// Net effect on the TCLK latch: unchanged (return to prior level).
}


void SbwDev::OnPulseTclkN()
{
	// TODO (Issue 2): one negative-going TCLK pulse via a 1-bit SBW frame.
}


void SbwDev::OnPulseTclk(int count)
{
	for (int i = 0; i < count; ++i)
		OnPulseTclk();
}


void SbwDev::OnFlashTclk(uint32_t min_pulses)
{
	(void)min_pulses;
	// TODO (Issue 2): drive TCLK at the Gen1/Gen2 flash rate (~470 kHz) via
	// a DMA-driven TIM1 burst. JtagDev uses the JtclkWaveGen path; SBW will
	// need an equivalent that reuses the SBW frame engine.
}


#endif // OPT_INCLUDE_SBW_DTRIG_
