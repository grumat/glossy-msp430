/*
# TimSbw — Spy-Bi-Wire Transport (Timer+DMA encoder)

Naming: this is the **timdma** transport model — the scan is driven by a TIM1
single-shot whose compare channels fan out DMA requests; there is no software
"dual trigger" critical section coordinating two competing peripherals (that is
the defining trait of the *dtrig* model used by DtrigJtag). A future SBW *dtrig*
variant is sketched in .claude/docs/drivers/DTRIG_SBW_SPI_ALT.md.

## Two sibling classes (split per the WaveJtag.h precedent)

Two dedicated classes, as WaveJtag.h splits Generator (single-port) vs
GeneratorSTLinkPWM (split-port):

  - **TimSbwSTLink** — the single-pin / un-buffered path (STLinkV2 PB14):
    direction is a *separate* full-CRH DMA, read-back on PB12. The only SBW path
    built today (bluepill is OPT_SBW_IMPL_OFF, g431 builds no SBW). Acquires a full
    device identify up to the 1.5 MHz grade.
  - **TimSbw** — the buffered/mux ("direction-switch") path: a PLACEHOLDER, not
    implemented (every buffered board is OPT_SBW_IMPL_OFF).

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

  - TIM1 (advanced timer) drives SBWCLK on a CHN output. Period = one SBWCLK cycle
    (kMult timer ticks; default 24 — even, so kPhaseClk_=12 is a clean 50 % duty).
    RCR = kSbwCycles − 1 sizes the whole scan as a single shot, same trick as DtrigJtag.
  - Compare channels trigger DMA within each cycle:
    * data BSRR + direction CRH DMAs — early (kPhaseData_/kPhaseDir_), valid before the
      SBWCLK fall (the capture). DirDma is higher-priority (dir-first turnaround).
    * IDR DMA — samples in the low phase, frequency-compensated to land late (max TDO
      settle); software keeps every 3rd sample (TDO).

## Per-cycle timing

SBWCLK is PWM (≈0 lag); the SBWDIO data/direction writes and the IDR sample are DMA, each
landing a fixed L (135–180 ns, GD32F103) after its compare. Data and direction are written
early so they settle before the SBWCLK fall; the TDO read is placed late in the low phase,
where the target drives ≈30 ns after the fall and holds to the rising edge (see kPhase*_ and
the SBW_Speed_* grades in platform.h).

## Sovereign Init contract

`Init()` claims every resource it needs unconditionally — see "Init() is
sovereign" in TIM_SBW_DRIVER.md.
*/

#pragma once

#include "JtagFrame.h"

// Compare→DMA→register latency band (ns) — places the TDO sample (see kPhaseSample_).
// Normally provided by stdproj.h/platform.h (per-MCU, from OPT_TEST_TIM_DMA_TIMING);
// these fallbacks keep TimSbw.h self-contained. GD32F103 measured 135..180 ns.
#ifndef OPT_SBW_DMA_LAT_MAX_NS
#  define OPT_SBW_DMA_LAT_MAX_NS 180
#endif
#ifndef OPT_SBW_DMA_LAT_MIN_NS
#  define OPT_SBW_DMA_LAT_MIN_NS 135
#endif


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
 * This is the single-pin / un-buffered path ONLY: direction is always a separate
 * full-CRH DMA and the data BSRR words carry the data bit alone. (The buffered/mux
 * path that folded the direction bit into the data BSRR lives in the sibling
 * placeholder class TimSbw — see the file header. There is no `kSeparateDirDma`
 * switch here; that distinction IS the class split.)
 */
template <
	typename SysClk, const Bmt::Timer::Unit kTim, const Bmt::Timer::Channel kSbwClk, const Bmt::Timer::Channel kSbwDataTrig, const Bmt::Timer::Channel kSbwSampleTrig, typename SbwDioOut, typename SbwDioIn, typename DirPolicy, const uint32_t kFreq, JtagFrame::Scan kScan, JtagFrame::NumBits kNumBits, const bool kCmpComplementary = true, const Bmt::Timer::Channel kSbwDirTrig = Bmt::Timer::Channel::k3, const uint16_t kMult = 24 ///< timer ticks per SBW wire-cycle (even → clean kPhaseClk_=12; see kTimerMultiplier_)
	,
	const uint16_t kLatMaxNs = OPT_SBW_DMA_LAT_MAX_NS ///< worst-case compare→DMA→reg latency (ns); places the TDO sample
	>
class TimSbwSTLink
{
  public:
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
	//       grade rises the tick shrinks but that latency does not, so the early
	//       data/dir writes (placed near the start of the cycle) must still land
	//       before the SBWCLK fall; a bigger multiplier widens that head-room.
	// Default 24 (the kMult template default): even, so kPhaseClk_ = 12 gives a clean
	// 50 % duty and the cycle counts in easy multiples. It is bench-proven to the top
	// SBW grade (1.4 MHz acquires a full G2xx3 identify). The OPT_SBW_TDO_SETTLE_SWEEP
	// bench mode instantiates with a much larger kMult (e.g. 64) at a low wire frequency
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
	// low phase = TDO valid, next rising edge = slot end. kPhaseData_/kPhaseDir_ set the
	// bus up before the fall; kPhaseSample_ reads inside the low phase.
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
	// Direction trigger — fires the separate full-CRH direction DMA.
	using DirTrigger = Bmt::Timer::AnyOutputChannel<
		CycleTimer, kSbwDirTrig,
		Bmt::Timer::OutMode::kFrozen,
		Bmt::Timer::Output::kDisabled, Bmt::Timer::Output::kDisabled>;
	using SampleTrigger = Bmt::Timer::AnyOutputChannel<
		CycleTimer, kSbwSampleTrig,
		Bmt::Timer::OutMode::kFrozen,
		Bmt::Timer::Output::kDisabled, Bmt::Timer::Output::kDisabled>;

	// ── Per-cycle phases (compare values within the kCycleTicks_-tick cycle) ──
	// SBWCLK is a PWM output (≈0 lag); the SBWDIO data BSRR and direction CRH are DMA, each
	// landing a fixed L (135–180 ns, GD32F103) after its trigger compare. Data and direction
	// are written early so they are valid before the SBWCLK fall (the capture edge).
	//   data(kPhaseData_) → dir(kPhaseDir_) → fall/capture(kPhaseClk_) → sample(kPhaseSample_) → rise
	static constexpr uint16_t kPhaseData_ = 1;				 // SBWDIO level
	static constexpr uint16_t kPhaseDir_  = 1;				 // direction — same tick; DirDma wins by priority
	static constexpr uint16_t kPhaseClk_  = kCycleTicks_ / 2; // capture (fall) at 50 %

	/// Latency `ns` in whole timer ticks for THIS grade (tick = 1/(kFreq·kCycleTicks_)).
	static constexpr uint16_t LatTicks(uint32_t ns)
	{
		return (uint16_t)(((uint64_t)ns * (uint64_t)kFreq * (uint64_t)kCycleTicks_
						   + 999999999ull) / 1000000000ull);
	}
	// TDO sample placed as late as the IDR latch allows: kCycleTicks_ - LatTicks(L_max + guard),
	// so (compare + L) clears the rising edge by ≈guard ns at every grade. The MSP430 drives TDO
	// ≈30 ns after the fall and holds it to the rising edge, so the latest read maximises settle
	// for an RC-loaded link with no early-release risk. Frequency-compensated by the LatTicks
	// term: late at slow grades, mid-low at fast. This is the per-grade BUILD-TIME seed; the LIVE
	// compare lives in the SampleTrigger CCR, written only by ApplySpeed(). Scan instances are
	// built at the slowest grade and run faster via PSC, so their seed is wrong for the active
	// grade — only ApplySpeed() (routed to the matching TimSbwInit_N) must touch the CCR.
	static constexpr uint16_t kSampleGuardNs_ = 60;		// > 45 ns latch jitter
	static constexpr uint16_t kPhaseSample_ =
		(kCycleTicks_ > LatTicks(kLatMaxNs + kSampleGuardNs_) + kPhaseClk_ + 1u)
			? (uint16_t)(kCycleTicks_ - LatTicks(kLatMaxNs + kSampleGuardNs_))
			: (uint16_t)(kPhaseClk_ + (kCycleTicks_ - kPhaseClk_) / 2u);	// fallback: mid-low

	static_assert(kPhaseClk_ != 0, "SBWCLK duty of 0 produces no clock edge");
	static_assert(kPhaseSample_ > kPhaseClk_ && kPhaseSample_ < kCycleTicks_,
				  "TDO sample must land in the low phase, before the rising edge");

	// ── DMA ──────────────────────────────────────────────────────────────────
	// Strict priority: DataDma (kVeryHigh) > DirDma (kHigh) > SampleDma (kMedium).
	// The compares already separate the requests in time (data@kPhaseData_ <
	// dir@kPhaseDir_ < sample@kPhaseSample_); priority only backstops a fast grade where
	// the inter-phase gap shrinks below the DMA latency. The order mirrors the intra-cycle
	// order: DATA first (the SBWDIO level must be latched before the driver is enabled —
	// enabling CRH over a stale ODR glitches the bus), DIR next (settle before the capture
	// edge), SAMPLE last (only meaningful once data + dir are valid).

	/// Data BSRR DMA: writes one uint32_t per cycle to the SBWDIO_Out port's BSRR
	/// (one word per wire cycle, the full kSbwCycles stream). Source increments, dest fixed.
	using DataDma = Bmt::Dma::AnyChannel<
		typename DataTrigger::DmaChInfo_,
		Bmt::Dma::Dir::kMemToPer,
		Bmt::Dma::PtrPolicy::kLongPtrInc, // 32-bit source, incrementing
		Bmt::Dma::PtrPolicy::kLongPtr,	  // 32-bit dest, fixed
		Bmt::Dma::Prio::kHigh>;			  // top rank — the first operation of every cycle

	/// Direction DMA (single-pin path only): writes one full mode-register word
	/// (e.g. GPIOB->CRH) per cycle from the 3-word dir-script. CIRCULAR: the direction
	/// pattern has period 3 (one JTAG bit = drive/drive/release across TMS/TDI/TDO), so
	/// a 3-word buffer wrapped every 3 transfers feeds the whole scan — no per-scan
	/// full-length buffer needed. The buffer is [drive, drive, release] aligned to the
	/// TMS/TDI/TDO slots (early write — fires the same cell it configures). See
	/// RenderDirScript().
	using DirDma = Bmt::Dma::AnyChannel<
		typename DirTrigger::DmaChInfo_,
		Bmt::Dma::Dir::kMemToPerCircular, // wraps every kSbwScriptLen_ transfers
		Bmt::Dma::PtrPolicy::kLongPtrInc, // 32-bit source script, incrementing
		Bmt::Dma::PtrPolicy::kLongPtr,	  // 32-bit dest (CRH), fixed
		Bmt::Dma::Prio::kVeryHigh>;		  // mid rank — follows the data level, before the read

	/// IDR sample DMA: reads GPIOx->IDR every cycle into the sample buffer.
	using SampleDma = Bmt::Dma::AnyChannel<
		typename SampleTrigger::DmaChInfo_,
		Bmt::Dma::Dir::kPerToMem,
		Bmt::Dma::PtrPolicy::kLongPtr,	  // 32-bit source IDR, fixed
		Bmt::Dma::PtrPolicy::kLongPtrInc, // 32-bit dest, incrementing
		Bmt::Dma::Prio::kMedium>;		  // lowest rank: Makes only sense when direction is valid

	/// Direction script period: one SBW JTAG bit = 3 wire cycles (TMS, TDI, TDO). The
	/// direction is data-independent (depends only on board DirPolicy + slot type), so a
	/// single 3-word period feeds the CIRCULAR DirDma, which replays it kJtagBits times.
	static constexpr uint16_t kSbwScriptLen_ = 3;

	/// The one direction period [drive, drive, release] aligned to the TMS/TDI/TDO slots
	/// (early write — each cycle writes its own cell's direction). The CIRCULAR DirDma
	/// replays it for the whole scan and ends on a release, so the bus is left Hi-Z.
	///
	/// PER-INSTANTIATION static; rendered lazily on first Start (scan instances never get
	/// Init(), so an un-rendered all-zero CRH word would wipe the SBWCLK AF bits).
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
	static_assert(DirDma::kChan_ != DataDma::kChan_ && DirDma::kChan_ != SampleDma::kChan_,
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
		DirPolicy::Init();
		DirTrigger::Setup();
		DirDma::Setup();
		RenderDirScript(); // fill the static (data-independent) dir script
		SetPhases();	   // place SBWCLK duty + grade-independent data/dir compares
		ApplySpeed();	   // seed PSC + the per-grade TDO sample compare (sole sample owner)
	}

	/// Apply the grade-INDEPENDENT per-cycle compares: SBWCLK duty, data and direction.
	/// All three are fixed cycle fractions (the kCycleTicks_ geometry is identical at every
	/// grade), so they are written once at Init() and never per-grade. The TDO sample is the
	/// only frequency-dependent compare; it is owned solely by ApplySpeed() and is NOT set
	/// here — so a stray Init()/SetPhases() on a scan instance cannot push its build-time
	/// (slowest-grade) seed into the live SampleTrigger CCR.
	static ALWAYS_INLINE void SetPhases()
	{
		SbwClkOut::SetCompare(kPhaseClk_);
		DataTrigger::SetCompare(kPhaseData_);		// data BSRR — early
		DirTrigger::SetCompare(kPhaseDir_);			// direction CRH — just after data
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
		DirDma::Setup();
	}

	static ALWAYS_INLINE void ReleaseDma()
	{
		DataDma::Disable();
		SampleDma::Disable();
		DirDma::Disable();
	}

	/// Render the data-independent 3-word direction period for the CIRCULAR DirDma.
	/// Early write — each cycle fires the same cell it configures, so the words align
	/// directly with the slot order: drive the TMS and TDI slots, release the TDO slot.
	///   index 0 → TMS slot : drive
	///   index 1 → TDI slot : drive
	///   index 2 → TDO slot : release (hand the bus to the target)
	/// The words come from DirPolicy (full mode-register values on the single-pin
	/// path). Called from Init() and (lazily) from Start().
	static void RenderDirScript()
	{
		const uint32_t drive   = DirPolicy::DriveOutput()[0];
		const uint32_t release = DirPolicy::DriveInput()[0];
		s_dir_script_[0] = drive;	// TMS slot
		s_dir_script_[1] = drive;	// TDI slot
		s_dir_script_[2] = release;	// TDO slot — release the bus for the target
	}

	/// Apply a new speed grade — TIM PSC and the per-grade TDO sample compare. The sample is
	/// the only frequency-dependent phase (it tracks the absolute DMA latency, not a cycle
	/// fraction); clock, data and direction are fixed fractions set once by SetPhases(). This
	/// is the SOLE writer of the SampleTrigger CCR: SetSpeed() routes to the grade's
	/// TimSbwInit_N, whose kPhaseSample_ matches that kFreq, so the single shared timer channel
	/// carries the active grade's compensated sample while the scan instances (all built at the
	/// slowest grade) stay frequency-agnostic and never push their own seed.
	static ALWAYS_INLINE void ApplySpeed()
	{
		CycleTimer::SetPrescaler(CycleTimer::kPrescaler_);
		SampleTrigger::SetCompare(kPhaseSample_);
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

		// Single-pin path: the bus direction is a separate CRH DMA (s_dir_script_), so
		// each BSRR word carries only the SBWDIO data bit.
		const uint32_t fill_bsrr = DataBsrr(tclk_high);

		// Payload window: MSB of data_out goes into JTAG bit kFirstPayloadBit.
		// Bit i ∈ [kFirstPayloadBit, kPastPayloadBit) reads data_out's bit
		// (kNumBits - 1 - (i - kFirstPayloadBit)).
		uint32_t shift_reg = data_out << (32u - (uint32_t)kNumBits);
		for (uint8_t i = 0; i < kJtagBits; ++i)
		{
			// Cycle 3i+0 (TMS phase)
			bsrr_script[3 * i + 0] = DataBsrr(TmsBit(i));

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
			bsrr_script[3 * i + 1] = tdi_bsrr;

			// Cycle 3i+2 (TDO phase) — the bus is released via the dir DMA, so the
			// target drives TDO and this ODR value is never driven onto the wire. Use a
			// neutral constant (the TDI fill level), NOT the next bit's TMS: the staging
			// experiment showed the staged value leaking into the read.
			bsrr_script[3 * i + 2] = fill_bsrr;
		}
	}

	/// GoIdle: 6× TMS=1 then TMS=0 ×2 (padding to 8 = kJtagBits). TDI held high
	/// throughout. Used to drive the TAP to Test-Logic-Reset → Run-Test/Idle
	/// from any prior state.
	static ALWAYS_INLINE void DoGoIdle(uint32_t* bsrr_script)
	{
		static_assert(kScan_ == JtagFrame::Scan::kGoIdle,
			"DoGoIdle() requires a kGoIdle instantiation");

		const uint32_t tdi_high = DataBsrr(true);

		for (uint8_t i = 0; i < kJtagBits; ++i)
		{
			const bool tms = (i < 6);
			bsrr_script[3 * i + 0] = DataBsrr(tms);
			bsrr_script[3 * i + 1] = tdi_high;
			bsrr_script[3 * i + 2] = tdi_high;	// TDO phase released — neutral fill (see RenderTransaction)
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
		if (!s_dir_rendered_)
		{
			RenderDirScript();
			s_dir_rendered_ = true;
		}

		// ARR = kCycleTicks_-1 (one SBWCLK period); RCR = kSbwCycles → auto-stop
		// after the full scan.
		CycleTimer::SetupRepetition(kTimerReload_, kSbwCycles);
		CycleTimer::ClearStatus();

		// Arm data BSRR DMA: one word per cycle, full kSbwCycles stream, source = the
		// rendered script, dest = the SBWDIO_Out port's BSRR. Each cycle writes its own
		// cell's data, set up before the capture edge.
		DataDma::SetTransferCount(kSbwCycles);
		DataDma::SetSourceAddress(bsrr_script);
		DataDma::SetDestAddress(&SbwDioOut::Io().BSRR);
		DataDma::Enable();

		// Arm IDR sample DMA: source = SBWDIO_In port's IDR, dest = sample_buf. Reads
		// slot c during cycle c (full count).
		SampleDma::SetTransferCount(kSbwCycles);
		SampleDma::SetSourceAddress(&SbwDioIn::Io().IDR);
		SampleDma::SetDestAddress(sample_buf);
		SampleDma::Enable();

		// Single-pin path: arm the CIRCULAR direction DMA from the 3-word period.
		// dest = DirPolicy::DirRegister() (e.g. &GPIOB->CRH); the source wraps over
		// s_dir_script_ every kSbwScriptLen_ transfers and replays it for the whole
		// scan. Re-arming here (disabled in Wait() → set count → enable) reloads CNDTR
		// and the source pointer to the base, so every scan starts on the same phase.
		DirDma::SetTransferCount(kSbwScriptLen_);
		DirDma::SetSourceAddress(s_dir_script_);
		DirDma::SetDestAddress(DirPolicy::DirRegister());
		DirDma::Enable();

		// Enable the timer's per-channel DMA-request generation (DIER.CCxDE).
		// Setup() only configures CCMR/CCER — it does NOT arm the DMA request
		// line, so without this the compare events never trigger the armed DMA
		// channels and no BSRR/CRH/IDR transfers occur (and Wait() then hangs on
		// SampleDma). DtrigJtag does the equivalent via TriggerRld1::EnableDma().
		DataTrigger::EnableDma();
		SampleTrigger::EnableDma();
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
		// it cannot trim the data/dir/clk/sample phase relationship — that is fixed
		// by the CCR values (kPhaseData_ < kPhaseDir_ < kPhaseClk_ < kPhaseSample_).
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
		// smp = the LIVE SampleTrigger CCR (set per grade by ApplySpeed), not the build-time
		// kPhaseSample_ seed — the active grade's value, valid for any scan instance.
		Trace() << "SBW rd s" << (int)(uint8_t)kScan_ << " n" << (int)(uint8_t)kNumBits_
				<< " cyc" << (int)kSbwCycles << " smp" << (int)SampleTrigger::GetCapture() << "\n bus ";
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
 * TimSbwTclk — Spy-Bi-Wire flash TCLK-strobe burst generator (single-pin path).
 *
 * Drives the Gen1/Gen2 flash "TCLK strobe" pulse train of SLAU320AJ §2.2.3.5.2
 * (Fig 2-12). While SBWTCK is held STATIC HIGH the target gates SBWTDIO straight
 * through to the CPU's TCLK, so a clock on SBWTDIO IS the TCLK — this class clocks
 * SBWDIO via a TIM1 compare → DMA → GPIO BSRR at the flash timing-generator rate.
 * That rate is FIXED BY THE FLASH (f(FTG) ≈ 257–476 kHz, target ~450 kHz), NOT by
 * the SBW bus-speed grade: every SBW grade sits outside the FTG window, so this
 * path runs its own dedicated timer rate.
 *
 * Scope: this class is ONLY the TDI-slot pulse engine. The bracketing bit-bang TMS
 * slot (enter, stay in Run-Test/Idle) and TDO slot (half-duplex turnaround), and the
 * AF→GPIO handoff that parks SBWTCK static-high, live in SbwDev::OnFlashTclk.
 *
 * Resource model: reuses TIM1 and ONE of TimSbwSTLink's compare/DMA channels — the
 * data-BSRR channel (kDataTrig → its DMA). SBW shift frames and the flash burst are
 * mutually exclusive (one TAP transaction at a time), so sharing the channel costs
 * nothing and leaves the other DMA channels free for future high-bandwidth features.
 * SbwDev re-establishes the shift-frame TIM1/DMA configuration after the burst.
 *
 * Pulse count: min_pulses reaches the thousands for mass/segment erase (4820 / 5300),
 * far over TIM1's 8-bit RCR, so the burst is emitted in ≤127-TCLK-cycle one-shot
 * chunks. The inter-chunk re-arm holds SBWTDIO at its idle level for <0.5 µs,
 * stretching one TCLK period to ~380 kHz — still inside the FTG window (slack down to
 * 257 kHz is 1.76 µs), so the stream stays valid across chunk boundaries.
 *
 * @tparam SysClk    System clock type (provides the TIM1 input frequency)
 * @tparam kTim      Advanced timer unit (TIM1 on F1xx — needs a repetition counter)
 * @tparam kDataTrig Compare-only channel whose DMA writes the SBWDIO BSRR (CH2→DMA1_CH3)
 * @tparam SbwDioOut Bmt::Gpio push-pull output alias for SBWDIO (exposes kPin_, Io())
 * @tparam kFreq     FTG target in Hz. InternalClock_Hz rounds the TIM1 prescaler to
 *                   nearest, so the achieved rate snaps to the 72 MHz / (PSC+1) / kMult
 *                   / 2 grid. At kFreq=470000, kMult=4 the prescaler rounds to 19 →
 *                   72 MHz/19/4/2 = 473.7 kHz: the nearest grid point AT/BELOW the
 *                   476 kHz FTG ceiling (the kMult=8 grid offers only 450 or 500 kHz —
 *                   500 is over the ceiling, so it can't reach 470). Crystal-derived
 *                   72 MHz (±tens of ppm) keeps 473.7 kHz safely under 476.
 * @tparam kMult     Timer ticks per TCLK HALF-cycle (two BSRR writes per TCLK period);
 *                   even → kPhase_ gives a clean 50 % duty.
 */
template <
	typename SysClk
	, const Bmt::Timer::Unit kTim
	, const Bmt::Timer::Channel kDataTrig
	, typename SbwDioOut
	, const uint32_t kFreq = 470000
	, const uint16_t kMult = 4
>
class TimSbwTclk
{
public:
	// Two BSRR writes per TCLK period (one per half-cycle), so the timer overflows at
	// 2×kFreq and each overflow's compare fires one alternating BSRR write.
	static constexpr uint16_t kCycleTicks_  = kMult;
	static constexpr uint16_t kTimerReload_ = kMult - 1u;	///< ARR (= period-1)
	static constexpr uint16_t kPhase_       = kMult / 2u;	///< compare mid-half-cycle
	/// Max whole TCLK cycles per one-shot chunk: 2 periods/cycle and RCR (rep) is 8-bit.
	static constexpr uint16_t kMaxCyclesPerChunk = 127;		///< 254 periods ≤ 255

	using MasterClock = Bmt::Timer::InternalClock_Hz<kTim, SysClk, (uint32_t)kCycleTicks_ * 2u * kFreq>;
	using CycleTimer  = Bmt::Timer::Any<MasterClock, Bmt::Timer::Mode::kSingleShot, kTimerReload_, false, true>;

	// ── MSP430 half-wave floor ───────────────────────────────────────────────────
	// The flash timing generator accepts f(FTG) only up to kMaxFtgHz; its half-wave
	// 1/(2·kMaxFtgHz) is therefore the SHORTEST half-wave the part tolerates — NEITHER
	// the high nor the low TCLK phase may ever be shorter (TI flash spec).
	//
	// Every TCLK half-wave here equals exactly ONE timer overflow period (kMult ticks):
	// the two alternating BSRR writes are one timer period apart, so the half-wave is set
	// by the crystal-exact timer, NOT by the absolute DMA lag — the lag delays both edges
	// equally and cancels from the interval. The only residual term is the timer→DMA
	// request JITTER, bench-measured at ~45 ns on the Geehy GD32F103 (lag 145 ns avg /
	// 180 ns worst; OPT_TEST_TIM_DMA_TIMING → TIM_DMA_TIMING_PROBE.md). That jitter is the
	// characterised, accepted safety margin on the half-wave; the design rule enforced
	// below is nominal half-wave (= the timer period) ≥ the FTG max-frequency half-wave.
	// The chunk-boundary idle only LENGTHENS a half-wave, never shortens it. The half-wave
	// is kMult / MasterClock::kFrequency_ s, so the floor reduces to a counting-frequency
	// ceiling checked at compile time below.
	static constexpr uint32_t kMaxFtgHz = 476000;
	static_assert(MasterClock::kFrequency_ <= 2u * (uint32_t)kMult * kMaxFtgHz,
				  "TCLK half-wave is shorter than the FTG max-frequency half-wave — "
				  "lower kFreq or raise kMult");

	/// Frozen compare channel — no pin output, just one DMA request per half-cycle.
	using DataTrigger = Bmt::Timer::AnyOutputChannel<
		CycleTimer, kDataTrig, Bmt::Timer::OutMode::kFrozen,
		Bmt::Timer::Output::kDisabled, Bmt::Timer::Output::kDisabled>;

	/// CIRCULAR 2-word BSRR DMA: the [away, back] pair is replayed for the whole chunk,
	/// synthesising the alternating square wave from a 2-word buffer (no per-length buffer).
	using DataDma = Bmt::Dma::AnyChannel<
		typename DataTrigger::DmaChInfo_,
		Bmt::Dma::Dir::kMemToPerCircular,
		Bmt::Dma::PtrPolicy::kLongPtrInc,	// 2-word source, wraps every 2 transfers
		Bmt::Dma::PtrPolicy::kLongPtr,		// fixed dest (BSRR)
		Bmt::Dma::Prio::kHigh>;

	static_assert(CycleTimer::HasRepetitionCounter(),
				  "TimSbwTclk requires an advanced timer with a repetition counter (TIM1)");

	static constexpr uint32_t kSet_   = (1u << (uint32_t)SbwDioOut::kPin_);			///< BSRR set  → high
	static constexpr uint32_t kReset_ = (1u << ((uint32_t)SbwDioOut::kPin_ + 16u));	///< BSRR reset → low

	/// 2-word alternating BSRR wave [away, back]. Rendered per call from the maintained
	/// TCLK level so the burst pulses AWAY from that level and returns to it — exactly
	/// `pulses` whole TCLK periods, end level == start level (no net TCLK edge).
	/// static inline so the DMA has a stable source address.
	static inline uint32_t s_wave_[2];

	/// Sovereign timer/DMA setup for the burst: claims the TIM1 timebase (PSC/ARR/OPM),
	/// the data compare channel and its DMA. SbwDev restores the shift configuration after.
	static void Init()
	{
		CycleTimer::Setup();
		DataTrigger::Setup();
		DataDma::Setup();
		DataTrigger::SetCompare(kPhase_);
	}

	static ALWAYS_INLINE void ReleaseDma() { DataDma::Disable(); }

	/**
	 * Emit `pulses` whole TCLK periods on SBWDIO, pulsing AWAY from `level` and back.
	 *
	 * Assumes SBWDIO already rests at `level` (the bracketing TMS slot left it there)
	 * and SBWTCK is held static high by the caller. Each chunk is an even number of
	 * timer periods, so it begins and ends on `level`; chained chunks therefore stay
	 * phase-continuous and the whole burst leaves TCLK exactly where it started.
	 */
	static void Emit(uint32_t pulses, bool level)
	{
		if (pulses == 0)
			return;
		// high level → dip low then back high ([low,high]); low level → bump high then
		// back low ([high,low]). First write is always AWAY, second returns to `level`.
		s_wave_[0] = level ? kReset_ : kSet_;	// away from level
		s_wave_[1] = level ? kSet_   : kReset_;	// back to level

		while (pulses)
		{
			const uint16_t c = (pulses > kMaxCyclesPerChunk)
							 ? kMaxCyclesPerChunk : (uint16_t)pulses;
			const uint8_t periods = (uint8_t)(c * 2u);	// 2 BSRR writes per TCLK period

			// Re-arm the circular 2-word source at its base so every chunk starts on the
			// AWAY half (deterministic phase), then OPM-size it to exactly `periods`.
			DataDma::SetTransferCount(2);
			DataDma::SetSourceAddress(s_wave_);
			DataDma::SetDestAddress(&SbwDioOut::Io().BSRR);
			DataDma::Enable();
			DataTrigger::EnableDma();

			CycleTimer::SetupRepetition(kTimerReload_, periods);	// ARR + RCR = periods-1
			CycleTimer::ClearStatus();
			CycleTimer::SetCounter(0);								// CNT=0 → clean first half-cycle
			CycleTimer::CounterResumeFast();
			CycleTimer::WaitForAutoStop();							// OPM clears CEN after `periods`

			DataTrigger::DisableDma();
			DataDma::Disable();
			pulses -= c;
		}
	}
};


/**
 * TimSbw — Spy-Bi-Wire frame generator (Timer+DMA model), buffered/mux path.
 *
 * Buffered board (BluePill-G431 jiga): SBWDIO is a level-converter buffer whose
 * direction is selected by a single GPIO bit (SBW_RD), and the data-drive pin and
 * the SBW_RD pin sit on the SAME GPIO port. So data + direction fold into ONE
 * composite BSRR word per wire cycle (the "buffered fast path" in the file header):
 * a single DMA stream writes data-bit | dir-bit to the shared port's BSRR. The
 * read-back is a SEPARATE physical input pin (SbwDioIn), sampled from the same
 * port's IDR. This collapses to TWO DMA channels — composite BSRR + IDR sample —
 * versus the single-pin TimSbwSTLink's three (data / separate-CRH dir / sample).
 *
 * The template parameter list is IDENTICAL to TimSbwSTLink so SbwDev can select
 * between the two with one argument set (std::conditional on the platform's
 * kWaveSbwSeparateDirDma). kSbwDirTrig is accepted but UNUSED here — there is no
 * separate direction channel; the distinction IS the class split. kDirPolicy must
 * describe a direction bit on the SAME port as SbwDioOut (its DriveOutput()[0] /
 * DriveInput()[0] BSRR words are OR-ed straight into the data words).
 *
 * Frame model, per-cycle timing, SBWCLK polarity and the TDO sample placement are
 * all identical to TimSbwSTLink — see that class and the file header. The ONLY
 * differences are (a) direction is folded into the data BSRR rather than driven by
 * a separate CRH DMA, and (b) the data pin keeps a fixed push-pull mode throughout
 * (the external buffer, not a CRH rewrite, performs the turnaround).
 */
template <
	typename SysClk, const Bmt::Timer::Unit kTim, const Bmt::Timer::Channel kSbwClk, const Bmt::Timer::Channel kSbwDataTrig, const Bmt::Timer::Channel kSbwSampleTrig, typename SbwDioOut, typename SbwDioIn, typename DirPolicy, const uint32_t kFreq, JtagFrame::Scan kScan, JtagFrame::NumBits kNumBits, const bool kCmpComplementary = true, const Bmt::Timer::Channel kSbwDirTrig = Bmt::Timer::Channel::k3 /* UNUSED on the buffered path */, const uint16_t kMult = 24
	,
	const uint16_t kLatMaxNs = OPT_SBW_DMA_LAT_MAX_NS
	>
class TimSbw
{
  public:
	static constexpr uint8_t kSbwIdrBit = SbwDioIn::kPin_;

	// ── Bit-count constants (identical to TimSbwSTLink) ──────────────────────
	static constexpr JtagFrame::Scan kScan_ = kScan;
	static constexpr JtagFrame::NumBits kNumBits_ = kNumBits;

	static constexpr uint8_t kJtagBits =
		(kScan == JtagFrame::Scan::kGoIdle)
			? 8
			: (5 + (uint8_t)kNumBits + (uint8_t)kScan);
	static constexpr uint16_t kSbwCycles = 3u * kJtagBits;
	static constexpr uint8_t kEntry =
		(kScan == JtagFrame::Scan::kGoIdle) ? 6 : (kScan == JtagFrame::Scan::kIR) ? 2
																				  : 1;
	static constexpr uint8_t kFirstPayloadBit = kEntry + 2u;
	static constexpr uint8_t kPastPayloadBit = kFirstPayloadBit + (uint8_t)kNumBits;
	static_assert(kScan == JtagFrame::Scan::kGoIdle || (kPastPayloadBit + 1u) <= kJtagBits,
				  "payload window + Exit1/Update overruns kJtagBits");

	// ── Timer (identical geometry to TimSbwSTLink) ───────────────────────────
	static constexpr uint16_t kTimerMultiplier_ = kMult;
	static constexpr uint16_t kCycleTicks_ = kTimerMultiplier_;
	using MasterClock = Bmt::Timer::InternalClock_Hz<kTim, SysClk, kCycleTicks_ * kFreq>;
	static constexpr uint16_t kTimerReload_ = kCycleTicks_ - 1u;
	using CycleTimer = Bmt::Timer::Any<MasterClock, Bmt::Timer::Mode::kSingleShot, kTimerReload_, false, true>;

	// SBWCLK PWM channel — idle-high, low-going pulse per bit (see TimSbwSTLink note).
	using SbwClkOut = Bmt::Timer::AnyOutputChannel<
		CycleTimer, kSbwClk,
		Bmt::Timer::OutMode::kPWM1,
		kCmpComplementary ? Bmt::Timer::Output::kDisabled : Bmt::Timer::Output::kEnabled,
		kCmpComplementary ? Bmt::Timer::Output::kEnabled : Bmt::Timer::Output::kDisabled,
		false,
		false>;

	// Compare-only triggers (Frozen mode, no pin output, just DMA requests).
	using DataTrigger = Bmt::Timer::AnyOutputChannel<
		CycleTimer, kSbwDataTrig,
		Bmt::Timer::OutMode::kFrozen,
		Bmt::Timer::Output::kDisabled, Bmt::Timer::Output::kDisabled>;
	using SampleTrigger = Bmt::Timer::AnyOutputChannel<
		CycleTimer, kSbwSampleTrig,
		Bmt::Timer::OutMode::kFrozen,
		Bmt::Timer::Output::kDisabled, Bmt::Timer::Output::kDisabled>;

	// ── Per-cycle phases ─────────────────────────────────────────────────────
	// One composite write per cycle (data | dir), placed early so it settles before
	// the SBWCLK fall; the TDO sample lands late in the low phase. No separate dir
	// phase — direction rides the data word.
	static constexpr uint16_t kPhaseData_ = 1;					 // composite data|dir BSRR
	static constexpr uint16_t kPhaseClk_  = kCycleTicks_ / 2;	 // capture (fall) at 50 %

	static constexpr uint16_t LatTicks(uint32_t ns)
	{
		return (uint16_t)(((uint64_t)ns * (uint64_t)kFreq * (uint64_t)kCycleTicks_
						   + 999999999ull) / 1000000000ull);
	}
	static constexpr uint16_t kSampleGuardNs_ = 60;
	static constexpr uint16_t kPhaseSample_ =
		(kCycleTicks_ > LatTicks(kLatMaxNs + kSampleGuardNs_) + kPhaseClk_ + 1u)
			? (uint16_t)(kCycleTicks_ - LatTicks(kLatMaxNs + kSampleGuardNs_))
			: (uint16_t)(kPhaseClk_ + (kCycleTicks_ - kPhaseClk_) / 2u);

	static_assert(kPhaseClk_ != 0, "SBWCLK duty of 0 produces no clock edge");
	static_assert(kPhaseSample_ > kPhaseClk_ && kPhaseSample_ < kCycleTicks_,
				  "TDO sample must land in the low phase, before the rising edge");

	// ── DMA — TWO channels (composite BSRR + IDR sample) ─────────────────────
	/// Composite BSRR DMA: one uint32_t per cycle (data bit | direction bit) to the
	/// shared port's BSRR. Source increments over the full kSbwCycles stream; dest fixed.
	using DataDma = Bmt::Dma::AnyChannel<
		typename DataTrigger::DmaChInfo_,
		Bmt::Dma::Dir::kMemToPer,
		Bmt::Dma::PtrPolicy::kLongPtrInc,
		Bmt::Dma::PtrPolicy::kLongPtr,
		Bmt::Dma::Prio::kHigh>;

	/// IDR sample DMA: reads the read-back port's IDR every cycle into the sample buffer.
	using SampleDma = Bmt::Dma::AnyChannel<
		typename SampleTrigger::DmaChInfo_,
		Bmt::Dma::Dir::kPerToMem,
		Bmt::Dma::PtrPolicy::kLongPtr,
		Bmt::Dma::PtrPolicy::kLongPtrInc,
		Bmt::Dma::Prio::kMedium>;

	// ── Compile-time checks ──────────────────────────────────────────────────
	static_assert(kSbwCycles <= 128,
				  "SBW scan too long for ping-pong buffers — increase OPT_SBW_BUFFER_CNT_");
	static_assert(CycleTimer::HasRepetitionCounter(),
				  "TimSbw requires an advanced timer with repetition counter (TIM1)");
	static_assert(DataDma::kChan_ != SampleDma::kChan_,
				  "data BSRR DMA and IDR sample DMA must use distinct channels");

	// ─────────────────────────────────────────────────────────────────────────
	// Lifecycle
	// ─────────────────────────────────────────────────────────────────────────

	/// Sovereign one-shot init: TIM1 + the two DMA channels. No separate direction
	/// DMA on this path (direction is folded into the data BSRR by RenderTransaction).
	static ALWAYS_INLINE void Init()
	{
		CycleTimer::Setup();
		SbwClkOut::Setup();
		DataTrigger::Setup();
		SampleTrigger::Setup();
		DataDma::Setup();
		SampleDma::Setup();
		DirPolicy::Init();		// no-op for the constexpr BSRR-mux policy
		SetPhases();
		ApplySpeed();
	}

	/// Grade-independent compares: SBWCLK duty and the composite data|dir write.
	static ALWAYS_INLINE void SetPhases()
	{
		SbwClkOut::SetCompare(kPhaseClk_);
		DataTrigger::SetCompare(kPhaseData_);
	}

	/// Override the TDO sample compare at runtime (the only grade-dependent phase).
	static ALWAYS_INLINE void SetSampleCompare(uint16_t cmp)
	{
		SampleTrigger::SetCompare(cmp);
	}

	static ALWAYS_INLINE void SetupDma()
	{
		DataDma::Setup();
		SampleDma::Setup();
	}

	static ALWAYS_INLINE void ReleaseDma()
	{
		DataDma::Disable();
		SampleDma::Disable();
	}

	/// Apply a new speed grade — TIM PSC and the per-grade TDO sample compare (the
	/// sole frequency-dependent phase; see TimSbwSTLink::ApplySpeed).
	static ALWAYS_INLINE void ApplySpeed()
	{
		CycleTimer::SetPrescaler(CycleTimer::kPrescaler_);
		SampleTrigger::SetCompare(kPhaseSample_);
		CycleTimer::TD::GetDevice()->EGR = TIM_EGR_UG;
	}

	// ─────────────────────────────────────────────────────────────────────────
	// Buffer rendering
	// ─────────────────────────────────────────────────────────────────────────

	/// Single-bit BSRR set/reset word for the SBWDIO data output pin.
	static ALWAYS_INLINE constexpr uint32_t DataBsrr(bool bit_value)
	{
		constexpr uint32_t kSet   = (1u << SbwDioOut::kPin_);
		constexpr uint32_t kReset = (1u << (SbwDioOut::kPin_ + 16));
		return bit_value ? kSet : kReset;
	}

	/// TMS waveform (identical to TimSbwSTLink): 1 for the entry-pulse cycles, 1 at
	/// the last shift bit (Exit1) and at Update, 0 elsewhere.
	static ALWAYS_INLINE constexpr bool TmsBit(uint8_t k)
	{
		if (k < kEntry) return true;
		if (k == kPastPayloadBit - 1) return true;
		if (k == kPastPayloadBit) return true;
		return false;
	}

	/**
	 * Fills the composite BSRR DMA script for one SBW scan. Per JTAG bit i, three
	 * uint32_t words go to bsrr_script[3i..3i+2], each folding the data bit and the
	 * direction bit (both on the shared port) into ONE BSRR write:
	 *   3i+0 (TMS):  DataBsrr(TmsBit(i)) | DirPolicy::DriveOutput()[0]
	 *   3i+1 (TDI):  DataBsrr(tdi_i)     | DirPolicy::DriveOutput()[0]
	 *   3i+2 (TDO):  fill                | DirPolicy::DriveInput()[0]
	 *                (buffer is in read mode; the data ODR is don't-care, the
	 *                 external buffer disconnects the drive pin from the bus)
	 *
	 * Payload `data_out` is shifted MSB-first into cycles [kFirstPayloadBit ..
	 * kPastPayloadBit). Head/tail bits use `tclk_high` as the TDI fill (same as
	 * DtrigJtag / TimSbwSTLink).
	 */
	static ALWAYS_INLINE void RenderTransaction(
		uint32_t* bsrr_script,
		bool tclk_high,
		uint32_t data_out)
	{
		static_assert(kScan_ != JtagFrame::Scan::kGoIdle,
			"Use DoGoIdle() for the GoIdle sequence");

		const uint32_t dir_out = DirPolicy::DriveOutput()[0];
		const uint32_t dir_in  = DirPolicy::DriveInput()[0];
		const uint32_t fill_bsrr = DataBsrr(tclk_high);

		uint32_t shift_reg = data_out << (32u - (uint32_t)kNumBits);
		for (uint8_t i = 0; i < kJtagBits; ++i)
		{
			// Cycle 3i+0 (TMS phase) — drive
			bsrr_script[3 * i + 0] = DataBsrr(TmsBit(i)) | dir_out;

			// Cycle 3i+1 (TDI phase) — drive
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

			// Cycle 3i+2 (TDO phase) — release to read: dir flips, data ODR is a
			// neutral constant (the buffer disconnects the drive pin from the bus).
			bsrr_script[3 * i + 2] = fill_bsrr | dir_in;
		}
	}

	/// GoIdle: 6× TMS=1 then TMS=0 ×2 (pad to 8). TDI held high; direction follows
	/// the same drive/drive/release period as a shift bit so the TDO slot is released.
	static ALWAYS_INLINE void DoGoIdle(uint32_t* bsrr_script)
	{
		static_assert(kScan_ == JtagFrame::Scan::kGoIdle,
			"DoGoIdle() requires a kGoIdle instantiation");

		const uint32_t dir_out  = DirPolicy::DriveOutput()[0];
		const uint32_t dir_in   = DirPolicy::DriveInput()[0];
		const uint32_t tdi_high = DataBsrr(true);

		for (uint8_t i = 0; i < kJtagBits; ++i)
		{
			const bool tms = (i < 6);
			bsrr_script[3 * i + 0] = DataBsrr(tms) | dir_out;
			bsrr_script[3 * i + 1] = tdi_high     | dir_out;
			bsrr_script[3 * i + 2] = tdi_high     | dir_in;	// TDO slot released (read)
		}
	}

	// NOTE: TCLK is NOT generated here (same as TimSbwSTLink) — it is bit-banged in
	// SbwDev. See the TimSbwSTLink TCLK note and SbwDev.tim.cpp.

	// ─────────────────────────────────────────────────────────────────────────
	// Execution
	// ─────────────────────────────────────────────────────────────────────────

	/**
	 * Launches a pre-rendered SBW scan: arm the composite BSRR DMA and the IDR
	 * sample DMA, then start TIM1. No competing peripheral, so no critical section.
	 */
	static OPTIMIZED void Start(
		uint32_t* bsrr_script,
		uint32_t* sample_buf)
	{
		CycleTimer::SetupRepetition(kTimerReload_, kSbwCycles);
		CycleTimer::ClearStatus();

		// Composite BSRR DMA: one data|dir word per cycle to the shared port's BSRR.
		DataDma::SetTransferCount(kSbwCycles);
		DataDma::SetSourceAddress(bsrr_script);
		DataDma::SetDestAddress(&SbwDioOut::Io().BSRR);
		DataDma::Enable();

		// IDR sample DMA: read the read-back port's IDR each cycle.
		SampleDma::SetTransferCount(kSbwCycles);
		SampleDma::SetSourceAddress(&SbwDioIn::Io().IDR);
		SampleDma::SetDestAddress(sample_buf);
		SampleDma::Enable();

		// Enable the timer's per-channel DMA-request generation (DIER.CCxDE).
		DataTrigger::EnableDma();
		SampleTrigger::EnableDma();

		// CNT MUST start at 0 for a clean full first cycle (see TimSbwSTLink note).
		CycleTimer::SetCounter(0);
		CycleTimer::CounterResumeFast();
	}

	/// Wait for the single-shot timer to auto-stop, then drain the sample DMA.
	static ALWAYS_INLINE void Wait()
	{
		CycleTimer::WaitForAutoStop();
		SampleDma::WaitTransferComplete();
		DataDma::Disable();
		SampleDma::Disable();
	}

	// ─────────────────────────────────────────────────────────────────────────
	// Result decoding
	// ─────────────────────────────────────────────────────────────────────────

	/// BENCH DIAG, gated by OPT_SBWDEV_DUMP_READ_PHASE. Dumps the bus-drive bit and
	/// the read-back bit per cycle over TRACESWO (grouped in 3s = one JTAG bit).
	static void DumpReadPhase(const uint32_t* sample_buf)
	{
		constexpr uint8_t kBusBit = SbwDioOut::kPin_;
		Trace() << "SBW rd s" << (int)(uint8_t)kScan_ << " n" << (int)(uint8_t)kNumBits_
				<< " cyc" << (int)kSbwCycles << " smp" << (int)SampleTrigger::GetCapture() << "\n bus ";
		for (uint16_t c = 0; c < kSbwCycles; ++c)
			Trace() << (char)('0' + ((sample_buf[c] >> kBusBit) & 1u)) << ((c % 3u == 2u) ? " " : "");
		Trace() << "\n rd  ";
		for (uint16_t c = 0; c < kSbwCycles; ++c)
			Trace() << (char)('0' + ((sample_buf[c] >> kSbwIdrBit) & 1u)) << ((c % 3u == 2u) ? " " : "");
		Trace() << "\n";
	}

	/// Extracts the received TDO payload from the IDR sample buffer. Only cycles
	/// (3i+2) for i ∈ [kFirstPayloadBit, kPastPayloadBit) carry valid TDO; pack
	/// MSB-first. (Identical to TimSbwSTLink.)
	static ALWAYS_INLINE uint32_t GetResult(const uint32_t* sample_buf)
	{
		if constexpr (kScan_ == JtagFrame::Scan::kGoIdle)
		{
			return 0;
		}
		else
		{
#if OPT_SBWDEV_DUMP_READ_PHASE
			DumpReadPhase(sample_buf);
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


} // namespace TimSbw_ns


namespace WaveJtag
{
	using TimSbw_ns::TimSbw;
	using TimSbw_ns::TimSbwSTLink;
	using TimSbw_ns::TimSbwTclk;
}
