
#include "stdproj.h"

#if OPT_INCLUDE_SBW_TIM_

#include "SbwDev.h"
#include "util/WaveSet.h"
#include "util/TimSbw.h"

using namespace Bmt::Dma;
using namespace Bmt::Timer;
using namespace JtagFrame;
using namespace WaveJtag;


// NOTE: there is deliberately NO per-grade CNT-offset table here. DtrigJtag has
// one because it phase-aligns TWO peripherals (SPI burst vs TIM1 TMS); TimSbw
// has a single timer driving every channel, so a CNT preset only deforms the
// first cycle and cannot trim anything. The real speed-vs-latency compensation
// (constant DMA latency growing as a fraction of a shrinking period) lives in
// the per-grade PHASE values inside TimSbw (kPhaseData_/kPhaseDir_), not here —
// to be sized from the bench when the multi-grade speed study runs (Issue 5).


// ── TimSbw type aliases ─────────────────────────────────────────────────────
//
// Every board-specific knob comes from the target's platform.h via the SBW
// contract: SBWDIO (output pin), SbwIdrPort + kSbwIdrBit (read-back), SbwDirPolicy
// (direction), the kWaveSbw* timer/channel constants, kWaveSbwCmpComplementary,
// kWaveSbwSeparateDirDma, and kWaveSbwDirTrig. This TU is transport-only and
// stays board-agnostic.

template <uint32_t Freq, Scan S, NumBits N>
using TimSbwImpl = TimSbw<
	  SysClk
	, kWaveSbwTimer
	, kWaveSbwClk
	, kWaveSbwDataTrig
	, kWaveSbwSampleTrig
	, SBWDIO					///< output data pin — must expose kPin_/kPortBase_
	, SbwIdrPort				///< GPIO holding the read-back (SBWDIO_In) bit
	, kSbwIdrBit
	, SbwDirPolicy				///< mux-BSRR (buffered) or full-CRH (single-pin)
	, Freq, S, N
	, kWaveSbwCmpComplementary
	, kWaveSbwSeparateDirDma
	, kWaveSbwDirTrig
>;

// Per-scan aliases at the slowest speed grade. ApplySpeed() bumps the actual
// TIM1 prescaler at runtime; the template parameter just picks the Init()
// baseline for the prescaler computation.
using TimSbwGoIdle = TimSbwImpl<SBW_Speed_1, Scan::kGoIdle, NumBits::kGoIdle>;
using TimSbwIr8    = TimSbwImpl<SBW_Speed_1, Scan::kIR,     NumBits::k8>;
using TimSbwDr8    = TimSbwImpl<SBW_Speed_1, Scan::kDR,     NumBits::k8>;
using TimSbwDr16   = TimSbwImpl<SBW_Speed_1, Scan::kDR,     NumBits::k16>;
using TimSbwDr20   = TimSbwImpl<SBW_Speed_1, Scan::kDR,     NumBits::k20>;
using TimSbwDr32   = TimSbwImpl<SBW_Speed_1, Scan::kDR,     NumBits::k32>;

// Per-grade Init-only instantiations: each sets TIM1 PSC for its grade.
using TimSbwInit_1 = TimSbwImpl<SBW_Speed_1, Scan::kDR, NumBits::k8>;
using TimSbwInit_2 = TimSbwImpl<SBW_Speed_2, Scan::kDR, NumBits::k8>;
using TimSbwInit_3 = TimSbwImpl<SBW_Speed_3, Scan::kDR, NumBits::k8>;
using TimSbwInit_4 = TimSbwImpl<SBW_Speed_4, Scan::kDR, NumBits::k8>;


// ── Static storage (mirrors JtagDev.dtrig.cpp s_have_in_flight_) ──

// True if there is an SBW frame DMA in flight that must be drained before the
// next Start(). Set inside each OnXxxShift() and cleared by SbwWaitTransfer().
// SBW and JTAG cannot be active at the same time, so this flag is independent
// of JtagDev's s_have_in_flight_.
static bool s_sbw_have_in_flight_ = false;

// SBW carries TCLK as the TDI bit value in the per-bit frame; OnSetTclk /
// OnClearTclk update this latch and the next shift fills head/tail bits with
// it (same role JTCLK::IsHigh() plays for JtagDev::OnIrShift).
static bool s_sbw_tclk_high = true;

// NOTE: the per-cycle direction waveform (out/out/in) is data-independent and
// now lives inside TimSbw as a static script rendered once at Init() — see
// TimSbw::RenderDirScript(). On the single-pin board (kWaveSbwSeparateDirDma)
// it is DMA'd to GPIOB->CRH; on buffered boards the dir bit is folded into the
// data BSRR words. No driver-side direction script is needed here.


/// Drain the most-recently-issued SBW shift's DMAs, if one is in flight.
/// Called from `JtagPending::operator T()` / `Get()` and from the start of
/// each shift body (to wait on the previous frame). Idempotent.
void SbwWaitTransfer()
{
	if (!s_sbw_have_in_flight_)
		return;
	// Every TimSbwXxx::Wait() shares the same TIM1 auto-stop flag and the
	// same IDR DMA channel, so any one of them drains the in-flight frame.
	TimSbwDr8::Wait();
	s_sbw_have_in_flight_ = false;
}


// ── Constructors ──────────────────────────────────────────────────────────────

SbwDev::SbwDev()   {}


// ── SbwDev virtual method implementations ─────────────────────────────────────

bool SbwDev::OnOpen()
{
	// Sovereign Init: unconditionally reclaim TIM1, GPIO and DMA channels
	// regardless of whether JtagDev was the previous owner. See
	// "Init() is sovereign" in TIM_SBW_DRIVER.md.
	s_sbw_have_in_flight_ = false;
	s_sbw_tclk_high = true;
	TimSbwDr8::ReleaseDma();		// clear stale DMA EN bits before re-Setup
	TimSbwInit_1::Init();			// GPIO AF, TIM1, both DMAs, DirPolicy
	return true;
}


void SbwDev::OnClose()
{
	SbwWaitTransfer();				// don't tear down mid-DMA
	TimSbwDr8::ReleaseDma();
	SbwBusOff();					// return SBW pins to Hi-Z
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
	SbwBusOn();				// board-specific SBW pin bring-up (e.g. release the
							// pin shorted to the SBWCLK trace; park data pin)
	SetBusState(BusState::sbw);
	StopWatch().Delay<Msec(10)>();
}


void SbwDev::SetSpeed(BusSpeed speed)
{
	// BRING-UP: force the slowest grade (~500 kHz on the wire) regardless of the
	// requested bus speed. SBW is validated at one slow speed first; the
	// multi-grade speed study (and the per-grade PHASE compensation that hides
	// DMA latency at the fast grades) is a follow-up. Delete the line below to
	// re-enable runtime speed selection once slow SBW works on a target.
	speed = BusSpeed::kSlowest;
	switch (speed)
	{
	case BusSpeed::kSlowest: TimSbwInit_1::ApplySpeed(); break;
	case BusSpeed::kSlow:    TimSbwInit_2::ApplySpeed(); break;
	case BusSpeed::kMedium:  TimSbwInit_3::ApplySpeed(); break;
	case BusSpeed::kFast:
	case BusSpeed::kFastest: TimSbwInit_4::ApplySpeed(); break;
	}
}


void SbwDev::OnReleaseJtag()
{
	SbwWaitTransfer();				// drain any in-flight frame before touching pins
	// Clean SBW exit (SLAU320AJ §2.4.1): drop the interface by holding
	// TEST/SBWTCK (PB13) LOW for > 100 µs — no special waveform, just the
	// documented low hold. (NOT the 7 µs figure, which is the in-frame per-cycle
	// low-phase ceiling — a different rule; see SBW_PIN_ROLES_AND_FUSE.md §2.1.)
	// PB13 is in TIM1_CH1N AF after entry, so flip it back to a GPIO output to
	// drive the static low; the frame timer is already idle (drained above), so
	// no AF/DMA contention and no critical section (the hold is a floor, not a
	// to-the-µs window — longer is fine).
	SBWTEST_Bb::SetupPinMode();		// PB13: TIM1_CH1N AF → GPIO push-pull output
	SBWTEST_Bb::SetLow();			// TEST/SBWTCK low
	StopWatch().Delay<Usec(150)>();	// > 100 µs → SBW logic deactivates
	SbwBusOff();					// return SBW pins to Hi-Z
	SetBusState(BusState::off);
	StopWatch().Delay<Msec(10)>();
}


void SbwDev::OnResetTap()
{
	SbwWaitTransfer();				// drain any leftover async shift before reset
	uint32_t* tx = buf_.GetNext1();
	TimSbwGoIdle::DoGoIdle(tx);
	buf_.Step();
	uint32_t* rx = buf_.GetCurrent2();
	TimSbwGoIdle::Start(tx, rx);
	TimSbwGoIdle::Wait();			// reset path is synchronous
	SetSpeed(speed_);
}


// ── Async-shift template (one frame, three DMAs) ──────────────────────────────
//
//   1. RenderTransaction → buf_.GetNext1()    ← CPU work, overlaps with previous DMA
//   2. SbwWaitTransfer()                       ← blocks on previous frame's TC
//   3. buf_.Step()                             ← advance ping-pong
//   4. R::Start(tx, rx)                        ← arm 3 DMAs, kick TIM1, return
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
	using R = TimSbwIr8;
	uint32_t* tx = buf_.GetNext1();
	R::RenderTransaction(tx, s_sbw_tclk_high, instruction);
	SbwWaitTransfer();				// drain previous frame
	buf_.Step();
	uint32_t* rx = buf_.GetCurrent2();
	R::Start(tx, rx);
	s_sbw_have_in_flight_ = true;
	return { reinterpret_cast<uint8_t*>(rx), +[](const uint8_t* p) -> uint8_t {
		auto q = static_cast<const uint32_t*>(static_cast<const void*>(p));
		return static_cast<uint8_t>(TimSbwIr8::GetResult(q));
	} };
}


JtagPending<uint8_t> SbwDev::OnDrShift8(uint8_t data)
{
	using R = TimSbwDr8;
	uint32_t* tx = buf_.GetNext1();
	R::RenderTransaction(tx, s_sbw_tclk_high, data);
	SbwWaitTransfer();
	buf_.Step();
	uint32_t* rx = buf_.GetCurrent2();
	R::Start(tx, rx);
	s_sbw_have_in_flight_ = true;
	return { reinterpret_cast<uint8_t*>(rx), +[](const uint8_t* p) -> uint8_t {
		auto q = static_cast<const uint32_t*>(static_cast<const void*>(p));
		return static_cast<uint8_t>(TimSbwDr8::GetResult(q));
	} };
}


JtagPending<uint16_t> SbwDev::OnDrShift16(uint16_t data)
{
	using R = TimSbwDr16;
	uint32_t* tx = buf_.GetNext1();
	R::RenderTransaction(tx, s_sbw_tclk_high, data);
	SbwWaitTransfer();
	buf_.Step();
	uint32_t* rx = buf_.GetCurrent2();
	R::Start(tx, rx);
	s_sbw_have_in_flight_ = true;
	return { reinterpret_cast<uint8_t*>(rx), +[](const uint8_t* p) -> uint16_t {
		auto q = static_cast<const uint32_t*>(static_cast<const void*>(p));
		return static_cast<uint16_t>(TimSbwDr16::GetResult(q));
	} };
}


JtagPending<uint32_t> SbwDev::OnDrShift20(uint32_t data)
{
	using R = TimSbwDr20;
	uint32_t* tx = buf_.GetNext1();
	R::RenderTransaction(tx, s_sbw_tclk_high, data);
	SbwWaitTransfer();
	buf_.Step();
	uint32_t* rx = buf_.GetCurrent2();
	R::Start(tx, rx);
	s_sbw_have_in_flight_ = true;
	// 20-bit DR result needs MSP430 word/byte-swap demuxing (rotate-right-by-4
	// within 20 bits) — same as JtagDev::OnDrShift20 and TI AllShifts() F_ADDR
	// (`((d<<16)+(d>>4)) & 0xFFFFF`). Embed it in the decoder so the Pending value
	// is already de-scrambled.
	return { reinterpret_cast<uint8_t*>(rx), +[](const uint8_t* p) -> uint32_t {
		auto q = static_cast<const uint32_t*>(static_cast<const void*>(p));
		uint32_t d = TimSbwDr20::GetResult(q);
		return ((d << 16) + (d >> 4)) & 0x000FFFFF;
	} };
}


JtagPending<uint32_t> SbwDev::OnDrShift32(uint32_t data)
{
	using R = TimSbwDr32;
	uint32_t* tx = buf_.GetNext1();
	R::RenderTransaction(tx, s_sbw_tclk_high, data);
	SbwWaitTransfer();
	buf_.Step();
	uint32_t* rx = buf_.GetCurrent2();
	R::Start(tx, rx);
	s_sbw_have_in_flight_ = true;
	return { reinterpret_cast<uint8_t*>(rx), +[](const uint8_t* p) -> uint32_t {
		auto q = static_cast<const uint32_t*>(static_cast<const void*>(p));
		return TimSbwDr32::GetResult(q);
	} };
}


// ── TCLK helpers ──────────────────────────────────────────────────────────────
// SBW has no dedicated TCLK pin: while the TAP is in Run-Test/Idle the TDI slot
// of each SBW cell drives the target's TCLK input (SLAU320AJ §2.2.3.5.1). So a
// TCLK edge is produced by a real RTI strobe frame (TMS held low throughout,
// TDI slot carrying the TCLK level) — NOT merely a software latch. The latch
// s_sbw_tclk_high still tracks the level so the next shift's head/tail fill
// matches, but the level is now actually clocked onto the wire here. Without
// real edges, IR_DATA_QUICK reads never auto-increment (PC advances on the TCLK
// FALLING edge, §2.2.4.2.3) — every word reads the same location.
//
// Each strobe is synchronous: drain any in-flight async shift, render the RTI
// frame, run it, and Wait. The kGoIdle instance supplies the geometry (TMS-free,
// 24 SBW cycles); TimSbw::DoTclk places the per-cell TDI/TCLK levels.

/// Run one synchronous Run-Test/Idle TCLK strobe frame. (tdi0, tdi1, tdi_rest)
/// pick the per-cell TCLK levels — see TimSbw::DoTclk.
static void SbwTclkFrame(bool tdi0, bool tdi1, bool tdi_rest)
{
	SbwWaitTransfer();				// drain any in-flight shift before reusing TIM1/buf_
	uint32_t* tx = SbwDev::buf_.GetNext1();
	TimSbwGoIdle::DoTclk(tx, tdi0, tdi1, tdi_rest);
	SbwDev::buf_.Step();
	uint32_t* rx = SbwDev::buf_.GetCurrent2();
	TimSbwGoIdle::Start(tx, rx);
	TimSbwGoIdle::Wait();			// synchronous — leaves nothing in flight
}


void SbwDev::OnSetTclk()
{
	SbwTclkFrame(true, true, true);		// TCLK → high on the wire
	s_sbw_tclk_high = true;
}


void SbwDev::OnClearTclk()
{
	SbwTclkFrame(false, false, false);	// TCLK → low on the wire
	s_sbw_tclk_high = false;
}


void SbwDev::OnPulseTclk()
{
	// high → low (falling: PC auto-increment under IR_DATA_QUICK) → high.
	SbwTclkFrame(true, false, true);
	s_sbw_tclk_high = true;				// ends high
}


void SbwDev::OnPulseTclkN()
{
	// low → high (rising) → low. Mirror of OnPulseTclk; bench-unvalidated.
	SbwTclkFrame(false, true, false);
	s_sbw_tclk_high = false;			// ends low
}


void SbwDev::OnPulseTclk(int count)
{
	for (int i = 0; i < count; ++i)
		OnPulseTclk();
}


void SbwDev::OnFlashTclk(uint32_t min_pulses)
{
	(void)min_pulses;
	// TODO (Issue 2): drive TCLK at the Gen1/Gen2 flash rate via a DMA-driven
	// TIM1 burst. JtagDev uses the JtclkWaveGen path; SBW will need an equivalent
	// that reuses the SBW frame engine.
	//
	// IMPORTANT — this TCLK rate is FIXED by the FLASH MEMORY, not by the SBW bus.
	// The MSP430 flash timing generator (FTG) requires f(FTG) within ~257–476 kHz
	// (target ~450 kHz); outside that window the flash program/erase is invalid.
	// Unlike OnPulseTclk()/OnSetTclk() — which strobe at the SBW *bus* grade — the
	// flash strobe MUST target ~450 kHz regardless of the selected bus speed, so
	// it must NOT be derived from the SBW prescaler. Note the current forced
	// kSlowest bus (500 kHz on the wire) already exceeds the 476 kHz flash max, so
	// reusing the per-cell strobe rate here would over-clock the FTG: this path
	// needs its own dedicated timer rate.
	//
	// (Applies to Gen1/Gen2 flash on 1xx/2xx/4xx. On F5xx/F6xx (Xv2) the flash
	// timing comes from the internal MODOSC — SLAU320AJ §2.2.3.5.2 — so TCLK
	// strobing is not used for flash there.)
}


#endif // OPT_INCLUDE_SBW_TIM_
