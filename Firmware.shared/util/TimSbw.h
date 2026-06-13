/*
# TimSbw — Spy-Bi-Wire Transport (Timer+DMA encoder)

Naming: this is the **timdma** transport model — the scan is driven by a TIM1
single-shot whose compare channels fan out DMA requests; there is no software
"dual trigger" critical section coordinating two competing peripherals (that is
the defining trait of the *dtrig* model used by DtrigJtag). A future SBW *dtrig*
variant is sketched in .claude/docs/drivers/DTRIG_SBW_SPI_ALT.md.

## Two sibling classes (split per the WaveJtag.h precedent)

The old single `TimSbw<… kSeparateDirDma …>` template is split into two dedicated
classes, exactly as WaveJtag.h splits Generator (single-port) vs
GeneratorSTLinkPWM (split-port):

  - **TimSbwSTLink** — the single-pin / un-buffered path (STLinkV2 PB14):
    direction is a *separate* full-CRH DMA, read-back on PB12. This is the only
    SBW path built today (bluepill is OPT_SBW_IMPL_OFF, g431 builds no SBW), so
    it carries the live, bench-proven implementation.
  - **TimSbw** — the buffered/mux ("direction-switch") path. Reduced to a
    PLACEHOLDER pending a fresh redesign around the latency-aligned late-write
    scheme (see .claude/docs/drivers/SBW_SPEED_TIMING_MODEL.md). The legacy
    folded-BSRR implementation is recoverable from git history.

The structural split preserves today's early-write timing behaviour; the v2
late-write scheme lands in TimSbwSTLink only after bench validation.

Companion to DtrigJtag. Design doc: .claude/docs/drivers/TIM_SBW_DRIVER.md.

## SBW frame model

Each logical JTAG bit becomes a 3-phase wire frame, clocked by SBWCLK rising
edges:

  Cycle 3k+0 : drive SBWDIO with TMS_k                              (output)
  Cycle 3k+1 : drive SBWDIO with TDI_k                              (output)
  Cycle 3k+2 : release SBWDIO via SBWO mux, sample TDO_k near falling edge

So `kJtagBits` logical JTAG bits expand to `3 × kJtagBits` SBWCLK cycles.

## Buffered-board fast path

When the data pin (SBWDIO_Out) and the direction-mux pin (SBWO) share a GPIO
port — which is the case for every buffered board today (PA7+PA9 on
Bluepill/BlackPill-BMP, PA7+PA10 on Nucleo-G431/L432) — both fold into the
same BSRR register. A single DMA stream writes one composite BSRR word per
SBWCLK cycle, simultaneously updating the data bit *and* the direction-mux
bit. The DirPolicy contract still applies for the dir-bit's set/reset
values; the encoder reads them at RenderTransaction time and merges them
into the data BSRR words.

This collapses the original 3-DMA-channel plan (data BSRR / dir BSRR / IDR
sample) down to **2 channels**: one composite BSRR DMA + one IDR sample DMA.

## Resource model

  - TIM1 (advanced timer) drives SBWCLK on a CHN output. Period = one
    SBWCLK cycle (8 timer ticks for trim margin). RCR = kSbwCycles − 1
    sizes the whole scan as a single shot, same trick as DtrigJtag.
  - Two compare channels at sub-cycle positions trigger:
    * BSRR DMA — writes the composite (data | dir) word at tick 0 of each
      cycle, before the SBWCLK rising edge
    * IDR DMA — samples GPIOA->IDR near the falling edge; software keeps
      every 3rd sample (the TDO-phase one)

## Sovereign Init contract

`Init()` claims every resource it needs unconditionally — see "Init() is
sovereign" in TIM_SBW_DRIVER.md.
*/

#pragma once

#include "JtagFrame.h"


namespace TimSbw_ns
{


/**
 * TimSbwSTLink — Spy-Bi-Wire frame generator (Timer+DMA model), single-pin path.
 *
 * Un-buffered single-pin board (STLinkV2 PB14): SBWDIO drives on one pin and is
 * released to Hi-Z each TDO slot so the target answers; the bus is read back on a
 * separate echo pin. Direction is a *separate* full-CRH DMA writing the whole
 * mode register once per cycle from a data-independent 3-word script, so the
 * data BSRR words carry only the data bit. (The buffered/mux fold-into-BSRR path
 * lives in the sibling placeholder class TimSbw — see the file header.)
 *
 * @tparam SysClk         System clock type (provides APB2 frequency)
 * @tparam kTim           Advanced timer unit (TIM1 on F1xx — needs RCR)
 * @tparam kSbwClk        Timer channel driving SBWCLK PWM (CH or CHN)
 * @tparam kSbwDataTrig   Compare-only channel triggering the data BSRR DMA
 * @tparam kSbwSampleTrig Compare-only channel triggering IDR sample DMA
 * @tparam SbwDioOut      Bmt::Gpio output pin alias for SBWDIO data drive
 *                        (must expose `kPin_` and `Io()`)
 * @tparam SbwDioIn       Bmt::Gpio input pin alias for the SBWDIO read-back. The
 *                        sample port and bit are derived from it directly:
 *                        kSbwIdrBit = SbwDioIn::kPin_, sampled IDR = SbwDioIn::Io().IDR.
 *                        (Single source of truth — no separate port helper / bit const.)
 * @tparam DirPolicy      See TIM_SBW_DRIVER.md "DirPolicy contract". Yields the full
 *                        "drive output" / "release input" mode-register words and
 *                        DirRegister() = &GPIOx->CRH for the separate direction DMA.
 * @tparam kFreq          SBWCLK wire frequency in Hz. Silicon max is 5 MHz, but
 *                        the practical ceiling is target-RC-bound (~1.2 MHz on the
 *                        proto targets) — see the SBW_Speed_* grades in platform.h.
 * @tparam kScan          DR, IR, or GoIdle (same enum as DtrigJtag)
 * @tparam kNumBits       Payload width (8 / 16 / 20 / 32, or GoIdle sentinel)
 * @tparam kCmpComplementary  true → SBWCLK on CHN; false → on regular CH
 * @tparam kSbwDirTrig    Compare-only channel that triggers the direction DMA;
 *                        must map to a DMA channel distinct from the data and
 *                        sample DMAs.
 *
 * NOTE: this class is the former `TimSbw<… kSeparateDirDma=true …>`; the bool
 * template parameter is gone, replaced by the hardwired member below. The
 * `if constexpr (kSeparateDirDma)` branches in the body therefore all take the
 * single-pin (true) path; the buffered (false) branches are dead-code-eliminated
 * and kept only until a follow-up strips them.
 */
template <
	typename SysClk
	, const Bmt::Timer::Unit kTim
	, const Bmt::Timer::Channel kSbwClk
	, const Bmt::Timer::Channel kSbwDataTrig
	, const Bmt::Timer::Channel kSbwSampleTrig
	, typename SbwDioOut
	, typename SbwDioIn
	, typename DirPolicy
	, const uint32_t kFreq
	, JtagFrame::Scan kScan
	, JtagFrame::NumBits kNumBits
	, const bool kCmpComplementary = true
	, const Bmt::Timer::Channel kSbwDirTrig = Bmt::Timer::Channel::k3
	, const uint16_t kMult = 8			///< timer ticks per SBW wire-cycle (see kTimerMultiplier_)
>
class TimSbwSTLink
{
public:
  /// Single-pin path: direction is always a separate full-CRH DMA. (Former
  /// template bool, now fixed — see the class note above.)
  static constexpr bool kSeparateDirDma = true;
  static constexpr uint8_t kSbwIdrBit = SbwDioIn::kPin_;
  
  // ── Bit-count constants ──────────────────────────────────────────────────
  static constexpr JtagFrame::Scan kScan_ = kScan;
  static constexpr JtagFrame::NumBits kNumBits_ = kNumBits;

  /// JTAG-level bits required for the selected scan (matches DtrigJtag).
  static constexpr uint8_t kJtagBits =
	  (kScan == JtagFrame::Scan::kGoIdle)
		  ? 8
		  : (5 + (uint8_t)kNumBits + (uint8_t)kScan);

  /// SBW wire cycles per scan = 3 × JTAG bits.
  static constexpr uint16_t kSbwCycles = 3u * kJtagBits;

  /// Entry-pulse length in JTAG bits (TMS=1 prefix). Mirrors DtrigJtag.
  static constexpr uint8_t kEntry =
	  (kScan == JtagFrame::Scan::kGoIdle) ? 6 : (kScan == JtagFrame::Scan::kIR) ? 2
																				: 1;

  /// Index of the first payload bit and the bit beyond the payload.
  ///
  /// The payload does NOT start right after the entry pulse: between Select-IR/DR
  /// and the first valid shift cell sit TWO navigation cells — Capture (TMS=0) and
  /// the Capture→Shift transition (TMS=0). The TAP graph is: two TMS=1 (entry),
  /// two TMS=0 (Capture + Shift-entry), THEN payload. So the first cell whose TDO
  /// carries shifted data is kEntry+2, not kEntry. (Reading from kEntry sampled the
  /// Capture/Shift-entry cells as if they were data — the window was two cells too
  /// early, which decoded the F5418A's 0x91 ID as 0x64.) The last payload cell
  /// doubles as Exit1 (TMS=1, see TmsBit) — that final shift carries the last bit,
  /// which is why the IR frame is one cell longer than DR.
  static constexpr uint8_t kFirstPayloadBit = kEntry + 2u;
  static constexpr uint8_t kPastPayloadBit = kFirstPayloadBit + (uint8_t)kNumBits;
  static_assert(kScan == JtagFrame::Scan::kGoIdle || (kPastPayloadBit + 1u) <= kJtagBits,
				"payload window + Exit1/Update overruns kJtagBits");

  // ── Timer ────────────────────────────────────────────────────────────────
  // kTimerMultiplier_ ticks per SBW wire-cycle. This is NOT the DtrigJtag
  // "trim resolution vs a fixed dual-trigger launch": TimSbw has no second
  // (SPI) trigger source — SBWCLK, the BSRR/CRH DMAs and the IDR sample are all
  // compare channels of THIS one timer, so there is no fixed offset to slide
  // against. The multiplier exists for two SBW-specific reasons instead:
  //   (a) intra-cycle phase resolution — one SBW bit-cell is an ordered
  //       sequence (data setup → SBWCLK fall/capture → TDO-valid low phase →
  //       sample); those four events cannot be placed inside a 1-tick cycle.
  //   (b) DMA-latency headroom — the DMA request→transfer latency is a fixed
  //       number of AHB cycles, independent of the prescaler. As the speed
  //       grade rises the tick shrinks but that latency does not, so it eats a
  //       growing fraction of the cycle (this is the real speed ceiling). The
  //       multiplier is the budget that hides it (see kPhaseData_). Size it
  //       from the bench — how many ticks kPhaseData_ needs at the top grade —
  //       not from the inherited 8.
  // Default 8 (the kMult template default); the OPT_SBW_TDO_SETTLE_SWEEP bench
  // mode instantiates with a much larger kMult (e.g. 64) at a low wire frequency
  // to get fine sub-tick resolution on the TDO settle measurement.
  static constexpr uint16_t kTimerMultiplier_ = kMult;
  static constexpr uint16_t kCycleTicks_ = kTimerMultiplier_; ///< ticks per SBW wire-cycle
  using MasterClock = Bmt::Timer::InternalClock_Hz<kTim, SysClk, kCycleTicks_ * kFreq>;
  // STM32 ARR is period-1, so ARR = kCycleTicks_-1 makes the cycle EXACTLY
  // kCycleTicks_ ticks and the wire clock MasterClock/kCycleTicks_ = kFreq
  // exactly. (An earlier version passed kCycleTicks_ itself as ARR, giving a
  // 9-tick cycle and kFreq·8/9 on the wire — which platform.h then had to
  // pre-divide away with a 562.5k×8/9 fudge. Frequency precision was never
  // required (the MSP430 spec gives only a max and a range), but matching ARR
  // to the clock-multiply keeps the duty a round 50 % and stops the period
  // comments lying.)
  static constexpr uint16_t kTimerReload_ = kCycleTicks_ - 1u; ///< ARR value (= period-1)
  // One-pulse (kSingleShot) + repetition counter: RCR = kSbwCycles-1 sizes the
  // whole scan as a single shot — one timer period per SBWCLK cycle, CEN
  // auto-clears after kSbwCycles overflows. Same trick DtrigJtag uses (rep=1/2).
  // kBuffered=false (ARPE=0) because ARPE=1 broke the RCR→REP reload via the
  // software UG on this Geehy GD32F103; kStrictUpdate=true (URS=1) matches.
  //
  // (History: the earlier "SBWCLK = one pulse over the whole scan" symptom was
  // NOT OPM gating the output — it was the per-instance s_dir_script_ being
  // un-rendered, so DirDma wrote an all-zero CRH that cleared PB13's AF bits.
  // Fixed by the lazy RenderDirScript() in Start(); see s_dir_script_ note. With
  // that fixed, the counter runs all kSbwCycles periods (proven by the frozen
  // DMA-trigger channels firing kSbwCycles times) and the PWM output follows.)
  using CycleTimer = Bmt::Timer::Any<MasterClock, Bmt::Timer::Mode::kSingleShot, kTimerReload_, false, true>;

  // SBWCLK PWM channel. Per TI SLAU320 §2.2.3 the SBWTCK line idles HIGH and each
  // slot is a LOW-going pulse. NOTE: the target captures TMS/TDI on the SBWTCK
  // *falling* edge (SLAU320 §2.2.3.1 — NOT the rising edge), and during a TDO slot
  // the slave drives the bus only in the LOW phase (slave starts on the falling
  // edge, master reads in the low phase, master re-enables its driver on the
  // rising edge). So per cycle: rising edge = slot start (data set up while high),
  // CCR fall = the TMS/TDI capture edge AND the start of the TDO drive window,
  // low phase = TDO valid, next rising edge = slot end. kPhaseData_ (CNT1) sets the
  // bus up before the CCR fall; kPhaseSample_ must read inside the low phase.
  // PWM1 (active-high while CNT < CCR) gives idle-HIGH: the pin is high at the start
  // of each cycle and at the parked CNT=0 between frames, dips low at CCR, rises at
  // the cycle boundary. On the complementary output (kCmpComplementary) CCxNP=0
  // means CHN just follows OCxREF, so the SAME PWM1 mode idles-high on either the
  // main or the CHN pin. (Was PWM2 on CHN, which parked LOW between frames — the
  // bench showed SBWCLK idling low, which risks the target's SBW inactivity timeout.)
  //
  // kPreloadEnable (OC1PE) is FALSE on purpose. The duty (CCR) is constant for
  // the whole scan, so the only thing CCR preload does here is defer the active
  // CCR load to an update event — and with RCR=kSbwCycles-1 the UEV only fires
  // once per scan (after the LAST period). On this Geehy GD32F103 that left the
  // PWM output firing a single pulse and then freezing for the remaining periods
  // (the counter + the FROZEN DMA-trigger channels, which use OC1PE=0, still ran
  // all kSbwCycles periods — that asymmetry is what pinpointed OC1PE). With
  // OC1PE=0 the compare is re-evaluated combinationally every period, so SBWCLK
  // toggles on all kSbwCycles periods. (SetCompare in SetPhases also writes the
  // active CCR directly, needing no UEV to take effect.) Same lesson as ARPE:
  // this clone misbehaves when a buffered register's reload is deferred by RCR.
  using SbwClkOut = Bmt::Timer::AnyOutputChannel<
	  CycleTimer, kSbwClk,
	  Bmt::Timer::OutMode::kPWM1, // idle-high, low-going pulse per bit (see note)
	  kCmpComplementary ? Bmt::Timer::Output::kDisabled : Bmt::Timer::Output::kEnabled,
	  kCmpComplementary ? Bmt::Timer::Output::kEnabled : Bmt::Timer::Output::kDisabled,
	  false, // OC1PE preload OFF — see note above (constant duty; RCR defers UEV)
	  false>;

  // Compare-only triggers (Frozen mode, no pin output, just DMA requests).
  using DataTrigger = Bmt::Timer::AnyOutputChannel<
	  CycleTimer, kSbwDataTrig,
	  Bmt::Timer::OutMode::kFrozen,
	  Bmt::Timer::Output::kDisabled, Bmt::Timer::Output::kDisabled>;
  // Single-pin direction trigger (only configured when kSeparateDirDma).
  using DirTrigger = Bmt::Timer::AnyOutputChannel<
	  CycleTimer, kSbwDirTrig,
	  Bmt::Timer::OutMode::kFrozen,
	  Bmt::Timer::Output::kDisabled, Bmt::Timer::Output::kDisabled>;
  using SampleTrigger = Bmt::Timer::AnyOutputChannel<
	  CycleTimer, kSbwSampleTrig,
	  Bmt::Timer::OutMode::kFrozen,
	  Bmt::Timer::Output::kDisabled, Bmt::Timer::Output::kDisabled>;

  // ── Per-cycle phases (compare values within the kCycleTicks_-tick cycle) ──
  // Each SBW wire-cycle spans CNT 0..kCycleTicks_-1. The period is fixed
  // across speed grades (only the prescaler changes), so these phases are
  // speed-independent. They place each intra-cycle event:
  //   • kPhaseClk_    — SBWCLK PWM duty. MUST be non-zero or the clock never
  //                     pulses (a CCR of 0 = no edge); this is why SBWCLK was
  //                     dead before phases were set.
  //   • kPhaseData_   — SBWDIO BSRR/ODR write; FIRST, so the output level is
  //                     latched before the driver is enabled.
  //   • kPhaseDir_    — CRH direction write; STRICTLY AFTER kPhaseData_. The
  //                     order is load-bearing: the pin output = ODR gated by the
  //                     CRH MODE bits, so if the driver is enabled (CRH) before
  //                     ODR holds the new bit (BSRR), the pin briefly drives the
  //                     STALE level → a transient on the wire (worst on the
  //                     input→output turnaround, where ODR is whatever it was
  //                     before the preceding TDO/Hi-Z cycle). On a TDO cycle the
  //                     same write releases the bus to Hi-Z before the target
  //                     drives; data-before-dir is required on drive cycles and
  //                     harmless on release cycles, so it is the universal order.
  //   • kPhaseSample_ — IDR sample; late, after the SBWCLK pulse, while the
  //                     target output is stable.
  // Ordering within a cycle: data(1) → dir(2) → SBWCLK fall/capture(kPhaseClk_)
  // → sample(kPhaseSample_) → rising edge. This order is enforced by the TIMER
  // COMPARE PHASES, NOT by DMA channel priority — firing DataTrigger and
  // DirTrigger at the SAME compare and letting the controller's arbitration
  // pick the winner is opaque and incidental (it falls to hardware channel
  // number once both are the same Prio). The compare separation makes it
  // deterministic; the priority hierarchy (see DataDma/DirDma) only backstops
  // the case where, at a fast grade, the 1-tick gap shrinks below the DMA
  // request→transfer latency and the two requests overlap. These counts are
  // bench STARTING POINTS — LA-tune them on the STLinkV2; the ORDER is the
  // invariant (locked by the static_asserts below), the exact ticks are tuned.
  static constexpr uint16_t kPhaseData_ = 1;
  static constexpr uint16_t kPhaseDir_ = 2;				   // strictly after data — see note
  static constexpr uint16_t kPhaseClk_ = kCycleTicks_ / 2; // CCR=4 → 4/8 = 50 % duty; fall = capture edge
  // TDO read point: SLAU320 §2.2.3.4 (TDO_RD) reads ~one slot-delay AFTER the
  // SBWTCK falling edge (the slave drives only in the low phase and is sampled
  // well before the rising edge, where it releases and the master re-acquires).
  // So sample a couple of ticks past the CCR fall — NOT at kCycleTicks_-1, which
  // sat right before the rising edge in the turnaround window.
  static constexpr uint16_t kPhaseSample_ = kPhaseClk_ + 2; // mid low-phase, after the fall
  static_assert(kPhaseClk_ != 0, "SBWCLK duty of 0 produces no clock edge");
  static_assert(kPhaseSample_ < kCycleTicks_, "TDO sample must land in the low phase, before the rising edge");
  // Bus-write ordering invariant: data level (BSRR) settled → driver enabled
  // (CRH) → capture edge. Violating this drives a stale level transient.
  static_assert(kPhaseData_ < kPhaseDir_, "data (BSRR) must be set up strictly before direction/enable (CRH)");
  static_assert(kPhaseDir_ < kPhaseClk_, "bus direction must settle before the SBWCLK capture (fall) edge");

  // ── DMA ──────────────────────────────────────────────────────────────────
  // Strict priority: DataDma (kVeryHigh) > DirDma (kHigh) > SampleDma (kMedium).
  // Priority only breaks a tie when two requests are pending at the SAME instant;
  // the compare phases (kPhaseData_ < kPhaseDir_ < … < kPhaseSample_) already
  // separate the requests in time, so it is moot at the bring-up grade and only
  // matters at the FAST grades, where the inter-phase gap shrinks below the DMA
  // request→transfer latency and requests overlap. The ranking then follows the
  // intra-cycle order:
  //   • Data (kVeryHigh) — the first event of every cycle; the output level must
  //     be latched before the driver is enabled (see kPhaseDir_ note: enabling
  //     CRH over a stale ODR glitches the bus).
  //   • Dir (kHigh) — follows data and must settle before the SBWCLK capture edge.
  //   • Sample (kMedium) — last; only meaningful once data + dir are valid.

  /// Composite BSRR DMA: writes one uint32_t per cycle to the SBWDIO_Out
  /// port's BSRR. Source increments, destination fixed.
  using DataDma = Bmt::Dma::AnyChannel<
	  typename DataTrigger::DmaChInfo_,
	  Bmt::Dma::Dir::kMemToPer,
	  Bmt::Dma::PtrPolicy::kLongPtrInc, // 32-bit source, incrementing
	  Bmt::Dma::PtrPolicy::kLongPtr,	// 32-bit dest, fixed
	  Bmt::Dma::Prio::kVeryHigh>;		// top rank: Always the first operation of a cycle

  /// Direction DMA (single-pin path only): writes one full mode-register word
  /// (e.g. GPIOB->CRH) per cycle from the 3-word dir-script. CIRCULAR: the
  /// drive/drive/release pattern has period 3, so the channel wraps every 3
  /// transfers and replays it for the whole scan. Source increments through the
  /// 3 words, destination fixed (CRH).
  using DirDma = Bmt::Dma::AnyChannel<
	  typename DirTrigger::DmaChInfo_,
	  Bmt::Dma::Dir::kMemToPerCircular, // wraps every kSbwScriptLen_ transfers
	  Bmt::Dma::PtrPolicy::kLongPtrInc, // 32-bit source script, incrementing
	  Bmt::Dma::PtrPolicy::kLongPtr,	// 32-bit dest (CRH), fixed
	  Bmt::Dma::Prio::kHigh>;			// mid rank — must happen before the read cycle

  /// IDR sample DMA: reads GPIOx->IDR every cycle into the sample buffer.
  using SampleDma = Bmt::Dma::AnyChannel<
	  typename SampleTrigger::DmaChInfo_,
	  Bmt::Dma::Dir::kPerToMem,
	  Bmt::Dma::PtrPolicy::kLongPtr,	// 32-bit source IDR, fixed
	  Bmt::Dma::PtrPolicy::kLongPtrInc, // 32-bit dest, incrementing
	  Bmt::Dma::Prio::kMedium>;			// lowest rank: Makes only sense when direction is valid

  /// Direction script period: one SBW JTAG bit = 3 wire cycles (TMS, TDI, TDO),
  /// and the direction is drive / drive / release across them. Data-independent
  /// (depends only on board DirPolicy, not on payload or scan length), so a
  /// single period feeds the CIRCULAR DirDma, which replays it kJtagBits times.
  static constexpr uint16_t kSbwScriptLen_ = 3;

  /// The one direction period (drive, drive, release). PER-INSTANTIATION static,
  /// but tiny and data-independent; rendered lazily on first Start (scan
  /// instances never get Init(), so an un-rendered all-zero CRH word would wipe
  /// the SBWCLK AF bits — same hazard as before, now only 3 words). Only emitted
  /// when kSeparateDirDma (odr-used solely under `if constexpr`).
  static inline uint32_t s_dir_script_[kSbwScriptLen_];
  /// One-time guard: true once s_dir_script_ has been rendered for this instance.
  static inline bool s_dir_rendered_ = false;

  // ── Compile-time checks ──────────────────────────────────────────────────
  static_assert(kSbwCycles <= 128,
				"SBW scan too long for ping-pong buffers — increase OPT_SBW_BUFFER_CNT_");
  static_assert(CycleTimer::HasRepetitionCounter(),
				"TimSbw requires an advanced timer with repetition counter (TIM1)");
  static_assert(DataDma::kChan_ != SampleDma::kChan_,
				"data BSRR DMA and IDR sample DMA must use distinct channels");
  static_assert(!kSeparateDirDma || (DirDma::kChan_ != DataDma::kChan_ && DirDma::kChan_ != SampleDma::kChan_),
				"direction DMA must use a channel distinct from data and sample DMAs");

  // ─────────────────────────────────────────────────────────────────────────
  // Lifecycle
  // ─────────────────────────────────────────────────────────────────────────

  /// Sovereign one-shot init: TIM1 + DMA channels + DirPolicy.
  static ALWAYS_INLINE void Init()
  {
	  CycleTimer::Setup();
	  SbwClkOut::Setup();
	  DataTrigger::Setup();
	  SampleTrigger::Setup();
	  DataDma::Setup();
	  SampleDma::Setup();
	  DirPolicy::Init(); // no-op for mux variants
	  if constexpr (kSeparateDirDma)
	  {
		  DirTrigger::Setup();
		  DirDma::Setup();
		  RenderDirScript(); // fill the static (data-independent) dir script
	  }
	  SetPhases(); // place SBWCLK duty + DMA-trigger compares
	}

	/// Apply the per-cycle SBWCLK duty and DMA-trigger compare phases. Called
	/// from Init(); the values (kPhase*_) are bench-tunable starting points.
	/// Phases are speed-independent, so this need not run again on a grade
	/// change (ApplySpeed touches only PSC).
	static ALWAYS_INLINE void SetPhases()
	{
		SbwClkOut::SetCompare(kPhaseClk_);
		DataTrigger::SetCompare(kPhaseData_);
		SampleTrigger::SetCompare(kPhaseSample_);
		if constexpr (kSeparateDirDma)
			DirTrigger::SetCompare(kPhaseDir_);
	}

	/// Override the TDO sample compare at runtime (the only phase that benefits
	/// from per-grade tuning — see SBW_SPEED_TIMING_MODEL.md). Persists across
	/// Start() (which does not re-run SetPhases), so it stays until the next
	/// Init()/SetPhases(). Used by the OPT_SBW_TDO_SETTLE_SWEEP bench probe to
	/// walk the sample point across the low phase. The effective sample lands
	/// ~L (DMA latency) after this compare.
	static ALWAYS_INLINE void SetSampleCompare(uint16_t cmp)
	{
		SampleTrigger::SetCompare(cmp);
	}

	static ALWAYS_INLINE void SetupDma()
	{
		DataDma::Setup();
		SampleDma::Setup();
		if constexpr (kSeparateDirDma)
			DirDma::Setup();
	}

	static ALWAYS_INLINE void ReleaseDma()
	{
		DataDma::Disable();
		SampleDma::Disable();
		if constexpr (kSeparateDirDma)
			DirDma::Disable();
	}

	/// Render the data-independent direction period: drive output for the TMS
	/// (cycle 0) and TDI (cycle 1) phases, release to input for the TDO (cycle 2)
	/// phase. The CIRCULAR DirDma replays this 3-word period for the whole scan.
	/// The words come from DirPolicy (full mode-register values on the single-pin
	/// path). Called from Init() and (lazily) from Start().
	static void RenderDirScript()
	{
		if constexpr (kSeparateDirDma)
		{
			const uint32_t drive   = DirPolicy::DriveOutput()[0];
			const uint32_t release = DirPolicy::DriveInput()[0];
			s_dir_script_[0] = drive;	// TMS phase
			s_dir_script_[1] = drive;	// TDI phase
			s_dir_script_[2] = release;	// TDO phase — release the bus
		}
	}

	/// Apply a new speed grade — TIM PSC only; CR1 / DMA setup unchanged.
	static ALWAYS_INLINE void ApplySpeed()
	{
		CycleTimer::SetPrescaler(CycleTimer::kPrescaler_);
		CycleTimer::TD::GetDevice()->EGR = TIM_EGR_UG;
	}

	// ─────────────────────────────────────────────────────────────────────────
	// Buffer rendering
	// ─────────────────────────────────────────────────────────────────────────

	/// Single-bit BSRR set/reset word for the SBWDIO output pin.
	static ALWAYS_INLINE constexpr uint32_t DataBsrr(bool bit_value)
	{
		constexpr uint32_t kSet   = (1u << SbwDioOut::kPin_);
		constexpr uint32_t kReset = (1u << (SbwDioOut::kPin_ + 16));
		return bit_value ? kSet : kReset;
	}

	/// TMS waveform: 1 for the entry-pulse cycles, 1 at the last shift bit
	/// (Exit1-DR/IR transition) and at Update-DR/IR, 0 elsewhere.
	static ALWAYS_INLINE constexpr bool TmsBit(uint8_t k)
	{
		if (k < kEntry) return true;					// entry pulse
		if (k == kPastPayloadBit - 1) return true;		// last shift bit → Exit1
		if (k == kPastPayloadBit) return true;			// Exit1 → Update
		return false;
	}

	/**
	 * Fills the composite BSRR DMA script for one SBW scan.
	 *
	 * Per JTAG bit i, three uint32_t BSRR words go to bsrr_script[3i..3i+2]:
	 *   3i+0 (TMS):  DataBsrr(TmsBit(i))  | DirPolicy::DriveOutput()[0]
	 *   3i+1 (TDI):  DataBsrr(tdi_i)      | DirPolicy::DriveOutput()[0]
	 *   3i+2 (TDO):  DirPolicy::DriveInput()[0]
	 *                (data pin BSRR irrelevant — buffer is in input mode)
	 *
	 * Payload `data_out` is shifted MSB-first into cycles
	 * [kFirstPayloadBit .. kPastPayloadBit) — i.e. starting two cells after the
	 * entry pulse (past Capture + Shift-entry). Head and tail bits use `tclk_high`
	 * as the TDI fill value (same convention as DtrigJtag).
	 *
	 * @param bsrr_script   Output: kSbwCycles uint32_t BSRR words (4-byte aligned)
	 * @param tclk_high     Current TCLK level (head/tail fill)
	 * @param data_out      Payload to shift; MSB first (same as DtrigJtag)
	 */
	static ALWAYS_INLINE void RenderTransaction(
		uint32_t* bsrr_script,
		bool tclk_high,
		uint32_t data_out)
	{
		static_assert(kScan_ != JtagFrame::Scan::kGoIdle,
			"Use DoGoIdle() for the GoIdle sequence");

		// On the single-pin path the direction lives in a separate register
		// DMA (s_dir_script_), so nothing is OR'd into the data BSRR words.
		const uint32_t dir_out = kSeparateDirDma ? 0u : DirPolicy::DriveOutput()[0];
		const uint32_t dir_in  = kSeparateDirDma ? 0u : DirPolicy::DriveInput()[0];
		const uint32_t fill_bsrr = DataBsrr(tclk_high);

		// Payload window: MSB of data_out goes into JTAG bit kFirstPayloadBit.
		// Bit i ∈ [kFirstPayloadBit, kPastPayloadBit) reads data_out's bit
		// (kNumBits - 1 - (i - kFirstPayloadBit)).
		uint32_t shift_reg = data_out << (32u - (uint32_t)kNumBits);
		for (uint8_t i = 0; i < kJtagBits; ++i)
		{
			// Cycle 3i+0 (TMS phase)
			bsrr_script[3 * i + 0] = DataBsrr(TmsBit(i)) | dir_out;

			// Cycle 3i+1 (TDI phase)
			uint32_t tdi_bsrr;
			if (i >= kFirstPayloadBit && i < kPastPayloadBit)
			{
				tdi_bsrr = DataBsrr((shift_reg & 0x80000000u) != 0);
				shift_reg <<= 1;
			}
			else
			{
				tdi_bsrr = fill_bsrr;
			}
			bsrr_script[3 * i + 1] = tdi_bsrr | dir_out;

			// Cycle 3i+2 (TDO phase) — release the bus
			bsrr_script[3 * i + 2] = dir_in;
		}
	}

	/// GoIdle: 6× TMS=1 then TMS=0 ×2 (padding to 8 = kJtagBits). TDI held high
	/// throughout. Used to drive the TAP to Test-Logic-Reset → Run-Test/Idle
	/// from any prior state.
	static ALWAYS_INLINE void DoGoIdle(uint32_t* bsrr_script)
	{
		static_assert(kScan_ == JtagFrame::Scan::kGoIdle,
			"DoGoIdle() requires a kGoIdle instantiation");

		const uint32_t dir_out  = kSeparateDirDma ? 0u : DirPolicy::DriveOutput()[0];
		const uint32_t dir_in   = kSeparateDirDma ? 0u : DirPolicy::DriveInput()[0];
		const uint32_t tdi_high = DataBsrr(true);

		for (uint8_t i = 0; i < kJtagBits; ++i)
		{
			const bool tms = (i < 6);
			bsrr_script[3 * i + 0] = DataBsrr(tms) | dir_out;
			bsrr_script[3 * i + 1] = tdi_high      | dir_out;
			bsrr_script[3 * i + 2] = dir_in;
		}
	}

	// NOTE: TCLK is NOT generated here. An early version rendered an RTI "TCLK
	// frame" through this DMA engine (TMS=0 cells, TDI-slot = TCLK level), but the
	// SBW RTI TCLK sync logic needs the TMSLDH intra-slot edge (SBWTDIO low at the
	// SBWTCK fall, then high before the slot ends — SLAU320AJ §2.2.3.2.3/§2.2.3.5.1),
	// which a one-BSRR-write-per-cycle DMA cannot produce. TCLK is therefore
	// bit-banged in SbwDev (OnSetTclk/OnClearTclk/OnPulseTclk*) — see the TCLK
	// helpers in SbwDev.tim.cpp.

	// ─────────────────────────────────────────────────────────────────────────
	// Execution
	// ─────────────────────────────────────────────────────────────────────────

	/**
	 * Launches a pre-rendered SBW scan: arm the composite BSRR DMA and the IDR
	 * sample DMA, then start TIM1. Unlike the *dtrig* model there is no
	 * competing peripheral to race against here — both DMA channels are armed
	 * before the timer is released, and the first CC trigger only fires once
	 * the timer is running, so no critical section is needed.
	 *
	 * @param bsrr_script  kSbwCycles BSRR words (from RenderTransaction)
	 * @param sample_buf   kSbwCycles slots for IDR DMA destination
	 */
	static OPTIMIZED void Start(
		uint32_t* bsrr_script,
		uint32_t* sample_buf)
	{
		// Lazily render THIS instance's direction script the first time it runs.
		// Each scan type has its own s_dir_script_; only the Init_* instances get
		// Init()/RenderDirScript(), so without this the scan instances would DMA an
		// all-zero CRH word and de-configure PB13 (SBWCLK AF). Data-independent, so
		// rendering once is sufficient.
		if constexpr (kSeparateDirDma)
		{
			if (!s_dir_rendered_)
			{
				RenderDirScript();
				s_dir_rendered_ = true;
			}
		}

		// ARR = kCycleTicks_-1 (one SBWCLK period); RCR = kSbwCycles → auto-stop
		// after the full scan.
		CycleTimer::SetupRepetition(kTimerReload_, kSbwCycles);
		CycleTimer::ClearStatus();

		// Arm composite BSRR DMA: source = bsrr_script, dest = SBWDIO_Out port's BSRR.
		// (On buffered boards data pin and DirPolicy mux pin share the port, so
		//  the same BSRR register receives both data and direction updates.)
		DataDma::SetTransferCount(kSbwCycles);
		DataDma::SetSourceAddress(bsrr_script);
		DataDma::SetDestAddress(&SbwDioOut::Io().BSRR);
		DataDma::Enable();

		// Arm IDR sample DMA: source = SBWDIO_In port's IDR, dest = sample_buf.
		SampleDma::SetTransferCount(kSbwCycles);
		SampleDma::SetSourceAddress(&SbwDioIn::Io().IDR);
		SampleDma::SetDestAddress(sample_buf);
		SampleDma::Enable();

		// Single-pin path: arm the CIRCULAR direction DMA from the 3-word period.
		// dest = DirPolicy::DirRegister() (e.g. &GPIOB->CRH); source wraps over
		// s_dir_script_ every kSbwScriptLen_ transfers. Re-arming here (disable in
		// Wait() → set count → enable) reloads CNDTR and the source pointer to the
		// base, so every scan starts on the TMS phase regardless of how the
		// previous one ended — the predictable-reposition guarantee.
		if constexpr (kSeparateDirDma)
		{
			DirDma::SetTransferCount(kSbwScriptLen_);
			DirDma::SetSourceAddress(s_dir_script_);
			DirDma::SetDestAddress(DirPolicy::DirRegister());
			DirDma::Enable();
		}

		// Enable the timer's per-channel DMA-request generation (DIER.CCxDE).
		// Setup() only configures CCMR/CCER — it does NOT arm the DMA request
		// line, so without this the compare events never trigger the armed DMA
		// channels and no BSRR/CRH/IDR transfers occur (and Wait() then hangs on
		// SampleDma). DtrigJtag does the equivalent via TriggerRld1::EnableDma().
		DataTrigger::EnableDma();
		SampleTrigger::EnableDma();
		if constexpr (kSeparateDirDma)
			DirTrigger::EnableDma();

		// All DMAs are armed above; start CNT at 0 and release the timer. The
		// first CC trigger cannot fire before the timer runs, so no critical
		// section is required (timdma model — see header). CNT MUST start at 0
		// (not kTimerReload_ == ARR, which would make the first SBWCLK cycle a
		// 1-tick runt: immediate overflow, clock already high, one repetition
		// wasted); 0 gives a clean full first cycle.
		//
		// NOTE: there is intentionally no per-grade CNT preset here. A CNT offset
		// only shifts the FIRST cycle (CNT reloads to 0 after each overflow), so
		// it cannot trim the data/clk/sample phase relationship — that is fixed
		// by the CCR values. Unlike DtrigJtag (which slides TIM1 against the SPI
		// burst), TimSbw has a single timer driving every channel; the real
		// speed-vs-DMA-latency compensation belongs in the per-grade PHASE values
		// (kPhaseData_/kPhaseDir_).
		CycleTimer::SetCounter(0);

		CycleTimer::CounterResumeFast();
	}

	/// Wait for the single-shot timer to auto-stop, then drain the sample DMA.
	static ALWAYS_INLINE void Wait()
	{
		// OPM auto-stop: CEN clears itself after the kSbwCycles-th overflow.
		CycleTimer::WaitForAutoStop();

		SampleDma::WaitTransferComplete();
		DataDma::Disable();
		SampleDma::Disable();
		if constexpr (kSeparateDirDma)
			DirDma::Disable();
	}

	// ─────────────────────────────────────────────────────────────────────────
	// Result decoding
	// ─────────────────────────────────────────────────────────────────────────

	/**
	 * BENCH DIAG, gated by OPT_SBWDEV_DUMP_READ_PHASE (default 0 — see platform.h
	 * / stdproj.h). Dumps the read-phase sample buffer over TRACESWO. SampleDma
	 * captures the WHOLE GPIOB->IDR every cycle,
	 * so this is an internal logic analyzer with exact cycle/phase alignment —
	 * no external probe on the SMD host needed. Three binary lines, one char per
	 * cycle, grouped in 3s = one JTAG bit (TMS TDI TDO):
	 *   bus = PB14 (SBWTDIO, what is on the wire / what the target drives)
	 *   clk = PB13 (SBWCLK at the sample instant — should be 0 = we sampled the
	 *         LOW phase; any 1 means kPhaseSample_ landed in the HIGH phase)
	 *   rd  = PB12 (SWD_IN echo — what the firmware actually feeds GetResult)
	 * Comparing bus vs rd isolates the level-translator path; clk verifies phase.
	 * (TCLK strobes are bit-banged in SbwDev, not DMA frames, so they do not appear
	 * here — observe them on the LA instead.)
	 */
	static void DumpReadPhase(const uint32_t* sample_buf)
	{
		constexpr uint8_t kBusBit = SbwDioOut::kPin_;	// PB14
		constexpr uint8_t kClkBit = 13;					// PB13 SBWCLK (STLinkV2)
		Trace() << "SBW rd s" << (int)(uint8_t)kScan_ << " n" << (int)(uint8_t)kNumBits_
				<< " cyc" << (int)kSbwCycles << " smp" << (int)kPhaseSample_ << "\n bus ";
		for (uint16_t c = 0; c < kSbwCycles; ++c)
			Trace() << (char)('0' + ((sample_buf[c] >> kBusBit) & 1u)) << ((c % 3u == 2u) ? " " : "");
		Trace() << "\n clk ";
		for (uint16_t c = 0; c < kSbwCycles; ++c)
			Trace() << (char)('0' + ((sample_buf[c] >> kClkBit) & 1u)) << ((c % 3u == 2u) ? " " : "");
		Trace() << "\n rd  ";
		for (uint16_t c = 0; c < kSbwCycles; ++c)
			Trace() << (char)('0' + ((sample_buf[c] >> kSbwIdrBit) & 1u)) << ((c % 3u == 2u) ? " " : "");
		Trace() << "\n";
	}

	/**
	 * Extracts the received TDO payload from the IDR sample buffer.
	 *
	 * Only cycles (3i+2) for i ∈ [kFirstPayloadBit, kPastPayloadBit) carry
	 * valid TDO; the rest are discarded. The bit position inside each IDR
	 * sample is kSbwIdrBit. Pack MSB-first into the result word.
	 *
	 * @param sample_buf  kSbwCycles IDR samples (from Start()/Wait())
	 * @return            Received payload, LSB-aligned
	 */
	static ALWAYS_INLINE uint32_t GetResult(const uint32_t* sample_buf)
	{
		if constexpr (kScan_ == JtagFrame::Scan::kGoIdle)
		{
			return 0;
		}
		else
		{
#if OPT_SBWDEV_DUMP_READ_PHASE
			DumpReadPhase(sample_buf);		// BENCH DIAG — gated by OPT_SBWDEV_DUMP_READ_PHASE
#endif
			uint32_t out = 0;
			for (uint8_t i = kFirstPayloadBit; i < kPastPayloadBit; ++i)
			{
				const uint32_t s = sample_buf[3 * i + 2];
				out = (out << 1) | ((s >> kSbwIdrBit) & 1u);
			}
			return out;
		}
	}
};


/**
 * TimSbw — buffered/mux ("direction-switch") SBW path. PLACEHOLDER.
 *
 * This is where the buffered fast path used to live (data + direction folded
 * into one composite BSRR DMA via a shared port mux pin — bluepill PA7+PA9 etc).
 * It was carved out of the old `kSeparateDirDma` switch when TimSbwSTLink became
 * its own class. No target builds it today (every buffered board is
 * OPT_SBW_IMPL_OFF), so it is intentionally reduced to a stub pending a fresh
 * redesign around the latency-aligned late-write scheme — see
 * .claude/docs/drivers/SBW_SPEED_TIMING_MODEL.md. The legacy folded-BSRR
 * implementation is recoverable from git history.
 *
 * Instantiating it is a compile error (dependent static_assert) so the gap is
 * loud rather than silent; reimplement here before re-enabling a buffered board.
 */
template <
	typename SysClk
	, const Bmt::Timer::Unit kTim
	, const Bmt::Timer::Channel kSbwClk
	, const Bmt::Timer::Channel kSbwDataTrig
	, const Bmt::Timer::Channel kSbwSampleTrig
	, typename SbwDioOut
	, typename SbwDioIn
	, typename DirPolicy
	, const uint32_t kFreq
	, JtagFrame::Scan kScan
	, JtagFrame::NumBits kNumBits
	, const bool kCmpComplementary = true
>
class TimSbw
{
	static_assert(sizeof(SysClk) == 0,
		"TimSbw (buffered/mux SBW path) is a placeholder — not yet reimplemented "
		"after the TimSbwSTLink split. Use TimSbwSTLink for the single-pin path, "
		"or reimplement this class (see SBW_SPEED_TIMING_MODEL.md).");
};


} // namespace TimSbw_ns


namespace WaveJtag
{
	using TimSbw_ns::TimSbw;
	using TimSbw_ns::TimSbwSTLink;
}
