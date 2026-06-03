
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

// Tracks the current TCLK level. Two roles:
//  (1) shift head/tail fill — RenderTransaction drives the non-payload (RTI) TDI
//      cells at this level, so a shift leaves TCLK where the logical state expects
//      (same role JTCLK::IsHigh() plays for JtagDev::OnIrShift); and
//  (2) idempotency guard for the bit-banged level-set strobes — OnSetTclk /
//      OnClearTclk only emit a strobe (which always makes a TDI-slot edge) when the
//      level actually changes, so a redundant set/clear cannot inject a spurious
//      CPU clock. The bit-bang TCLK helpers (below) keep this latch in sync.
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

JtagPending<uint8_t> SbwDev::OnIrShift(Ir instruction)
{
	using R = TimSbwIr8;
	uint32_t* tx = buf_.GetNext1();
	R::RenderTransaction(tx, s_sbw_tclk_high, E2I(instruction));
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


// ── TCLK helpers (bit-banged Run-Test/Idle strobes) ─────────────────────────────
// SBW has no dedicated TCLK pin: while the TAP is in Run-Test/Idle the TDI slot
// drives the target's TCLK input (SLAU320AJ §2.2.3.5.1). Crucially, the RTI TCLK
// sync logic (Fig. 2-11) needs the SBWTDIO *edge* placed inside the slot — the TMS
// slot must use the TMSLDH pattern (SBWTDIO low at the SBWTCK falling edge so TMS=0
// keeps the TAP in RTI, then brought HIGH in the SBWTCK low phase before the slot
// ends, §2.2.3.2.3). The earlier DMA strobe drove a STATIC level per slot — the TDI
// levels read back correct on SWO, but with no intra-slot edge the sync logic never
// produced a TCLK edge, so the CPU never clocked and Ir::kDataQuick / SetPC silently
// failed (constant reads). A DMA channel writing one BSRR word per cycle cannot make
// the two edges TMSLDH needs, so TCLK is bit-banged here instead — it is low-rate and
// synchronous, so exact GPIO edge control is the right tool.
//
// Bus during a strobe: SBWTCK (PB13) is flipped from TIM1_CH1N AF to GPIO, SBWTDIO
// (PB14) is driven as an output, and the sequence runs with interrupts off (an ISR
// must not stretch an SBWTCK low phase past the 7 µs SBW limit, §2.2.3). PB13 is
// handed back to TIM1 afterwards for the DMA shift frames.
//
// Reconstruction (no TI SetTCLK_sbw/ClrTCLK_sbw source in-tree): derived from the
// slot macros + §2.2.3.5.1; the OnPulseTclk/OnPulseTclkN polarity matches the
// 4-wire JtagDev (0xF0 → ends low / 0x0F → ends high). Each TCLK bit also releases
// SBWTDIO in its TDO slot (SbwBbTdoSlot), the half-duplex turnaround the DMA frame
// engine does per cycle — without it the host fights the target's TDO driver.

/// Inter-edge settle. Keep the SBWTCK low phase well under 7 µs (we are inside a
/// CriticalSection, so no ISR can stretch it).
static ALWAYS_INLINE void SbwBbSettle() { StopWatch().Delay<Usec(1)>(); }

/// Half-duplex turnaround (PB14 only — leaves the GPIO-driven SBWTCK on PB13
/// untouched, unlike the full-CRH SbwDirPolicy used by the DMA frames).
static ALWAYS_INLINE void SbwBbDioDrive()   { SbwDioDrive_Bb::SetupPinMode(); }
static ALWAYS_INLINE void SbwBbDioRelease() { SbwDioRelease_Bb::SetupPinMode(); }

/// TDO slot of a TCLK bit: release SBWTDIO so the target drives the bus for one
/// SBWTCK cycle, then re-drive it as output for the next bit's TMS slot. The DMA
/// shift frames flip direction out/out/in every cycle (RenderDirScript) — the
/// bit-bang strobe must do the same or the host fights the target's TDO driver
/// through the level translator. We don't sample TDO here; the release is purely
/// the bus turnaround the SBW slot machine expects.
static ALWAYS_INLINE void SbwBbTdoSlot()
{
	SbwBbDioRelease();							// PB14 → floating input; target owns the bus
	SBWTEST_Bb::SetLow();	SbwBbSettle();		// SBWTCK ↓
	SBWTEST_Bb::SetHigh();	SbwBbSettle();		// SBWTCK ↑
	SbwBbDioDrive();							// PB14 → output for the next TMS slot
}

/// TMS slot, plain low (TMSL): SBWTDIO low across the whole slot → TMS captured 0.
static ALWAYS_INLINE void SbwBbTmsL()
{
	SBWDIO::SetLow();		SbwBbSettle();
	SBWTEST_Bb::SetLow();	SbwBbSettle();		// SBWTCK ↓ — TMS = 0
	SBWTEST_Bb::SetHigh();	SbwBbSettle();
}

/// TMS slot, TMSLDH (SLAU320AJ §2.2.3.2.3): SBWTDIO low at the SBWTCK falling edge
/// (TMS = 0, stay in RTI), then brought HIGH in the SBWTCK low phase before the slot
/// ends. This is the sync rising edge required so a following TDIL makes a real TCLK
/// falling edge (§2.2.3.5.1) — the thing the DMA strobe could not produce.
static ALWAYS_INLINE void SbwBbTmsLdh()
{
	SBWDIO::SetLow();		SbwBbSettle();
	SBWTEST_Bb::SetLow();	SbwBbSettle();		// SBWTCK ↓ — TMS = 0
	SBWDIO::SetHigh();							// SBWTDIO ↑ in the SBWTCK low phase
	SBWTEST_Bb::SetHigh();	SbwBbSettle();
}

/// TDI slot: drive SBWTDIO = TCLK level, captured on the SBWTCK falling edge.
static ALWAYS_INLINE void SbwBbTdi(bool level)
{
	if (level) SBWDIO::SetHigh(); else SBWDIO::SetLow();
	SbwBbSettle();
	SBWTEST_Bb::SetLow();	SbwBbSettle();		// SBWTCK ↓ — TCLK captured
	SBWTEST_Bb::SetHigh();	SbwBbSettle();
}

/// One TCLK bit. The TMS slot must MAINTAIN the *current* TCLK level so a held line
/// is never glitched (TI _hil_2w_Tclk, hil_2w.c: TMSLDH when currently high, TMSL when
/// currently low); the TDI slot then drives the NEW level, where the intended edge is
/// captured. Picking the TMS variant from the transition *direction* instead (the old
/// SbwBbSetTclk=TmsL / SbwBbClrTclk=TmsLdh hardcode) injects a SPURIOUS TMS-slot edge
/// whenever a pulse is entered from the "wrong" level — e.g. OnPulseTclkN entered LOW
/// (SetPC's first kdTclkN) ran TMSLDH on a low line → a phantom rising clock that
/// corrupted MOVA #imm,PC and left raw[0..]=0x00f0/0x3fff (FR5994 golden-reference
/// diff). s_sbw_tclk_high is the live level (also the DMA shift head/tail fill).
static ALWAYS_INLINE void SbwBbTclkBit(bool level)
{
	if (s_sbw_tclk_high) SbwBbTmsLdh();		// currently high → hold high through TMS
	else                 SbwBbTmsL();		// currently low  → hold low through TMS
	SbwBbTdi(level);						// TDI slot: drive the new level (edge here)
	SbwBbTdoSlot();
	s_sbw_tclk_high = level;
}
static ALWAYS_INLINE void SbwBbSetTclk() { SbwBbTclkBit(true); }		// → TCLK high
static ALWAYS_INLINE void SbwBbClrTclk() { SbwBbTclkBit(false); }	// → TCLK low

/// Run a bit-banged TCLK strobe: take SBWTCK (PB13) off TIM1 AF, drive the RTI bit
/// sequence with interrupts off, then hand PB13 back to TIM1_CH1N for the next frame.
template <typename BitSeq>
static void SbwTclkStrobe(BitSeq bits)
{
	SbwWaitTransfer();				// no DMA frame may be live on TIM1/PB13
	// Pre-load ODR=1 BEFORE flipping the mux. SetHigh() is a BSRR write that sets
	// the ODR bit while PB13 is still in TIM1_CH1N AF (harmless — the timer drives
	// the pin until SetupPinMode runs). PB13's PWM idles HIGH but the GPIO ODR was
	// last left LOW, so doing SetupPinMode first would drive the pin to the stale
	// ODR=0 the instant the mux switches — a spurious SBWCLK low/high glitch (a
	// "dummy" clock edge) that advances the target's SBW slot machine before the
	// real strobe. Setting ODR high first makes the AF→GPIO transition high→high.
	SBWTEST_Bb::SetHigh();			// ODR=1 first (no pin effect under AF)
	SBWTEST_Bb::SetupPinMode();		// PB13: TIM1_CH1N AF → GPIO output, already HIGH
	SBWDIO::SetupPinMode();			// PB14: ensure driven output for the bit-bang
	{
		CriticalSection lock;		// 7 µs low-phase rule — no ISR mid-strobe
		bits();
	}
	SbwClkToAf::Setup();			// PB13 → TIM1_CH1N AF (SBWCLK) for DMA frames
}


void SbwDev::OnSetTclk()
{
	// Idempotent — strobe ONLY on an actual low→high transition. Unlike the
	// 4-wire driver (where SetTCLK just drives a level), the bit-bang SetTCLK
	// always manufactures a TDI-slot rising edge, so re-asserting an already-high
	// TCLK would inject a SPURIOUS clock into the CPU (an extra SetPC step / extra
	// Ir::kDataQuick increment). s_sbw_tclk_high tracks the level (it is also the
	// shift head/tail fill), so it is the correct guard.
	if (!s_sbw_tclk_high)
		SbwTclkStrobe([]{ SbwBbSetTclk(); });	// SbwBbTclkBit maintains s_sbw_tclk_high
}


void SbwDev::OnClearTclk()
{
	// Idempotent — strobe ONLY on an actual high→low transition (see OnSetTclk):
	// re-asserting an already-low TCLK would inject a spurious falling clock.
	if (s_sbw_tclk_high)
		SbwTclkStrobe([]{ SbwBbClrTclk(); });	// SbwBbTclkBit maintains s_sbw_tclk_high
}


void SbwDev::OnPulseTclk()
{
	// Set (→1) then Clr (→0): ends LOW — same polarity as the 4-wire JtagDev::OnPulseTclk
	// (0xF0 = high then low). The trailing falling edge is the one Ir::kDataQuick
	// increments the PC on (§2.2.4.2.3). SbwBbTclkBit now maintains the level through the
	// TMS slot, so entering already-high emits just the falling edge (no phantom rising).
	SbwTclkStrobe([]{ SbwBbSetTclk(); SbwBbClrTclk(); });
}


void SbwDev::OnPulseTclkN()
{
	// Clr (→0) then Set (→1): ends HIGH — same polarity as the 4-wire
	// JtagDev::OnPulseTclkN (0x0F = low then high). Two 3-slot bits, so the strobe stays
	// aligned to the SBW slot machine. SbwBbTclkBit maintains the level through the TMS
	// slot, so entering already-low emits just the rising edge (no phantom falling) —
	// this is the SetPC MOVA case that previously corrupted the program counter.
	SbwTclkStrobe([]{ SbwBbClrTclk(); SbwBbSetTclk(); });
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
