/*
# TimDmaTiming — Timer→DMA latency bench probe (driver-decoupled)

A **single-shot, logic-analyzer-only** waveform generator used to measure the fixed
timer-compare → DMA-request → register-write latency of a given MCU. It deliberately
mirrors the `TimSbw` timer+DMA model (TIM1 advanced timer, PWM clock channel, frozen
compare channels fanning out DMA requests writing GPIO `BSRR`) but carries **no include
or type dependency on TimSbw.h** — it is a standalone reference so the same numbers can
be taken on the STLinkV2 Geehy part today and on the genuine STM32F103 / STM32G431 later
without dragging the SBW protocol layer along.

## Why this exists

`TimSbw::kTimerMultiplier_` (8 ticks/cycle) is a budget that hides the DMA
request→transfer latency (see the long note in `Firmware.shared/util/TimSbw.h`). That
budget is currently an inherited guess. This probe makes the latency directly visible on
a logic analyzer so the multiplier, the per-cycle phase constants and the SBW speed
grades can be set from measured data instead of folklore.

## Waveform (one of `kReps` = 100 repetitions)

```
            ____            anchor = SBWCLK falling edge (timer compare @ kPhaseClk_)
SBWCLK: ____|   |________
                 _
SBWDIO: ________| |_______
```

Both the SBWCLK PWM falling edge and the two DMA-trigger compares are placed at the
**same** counter value `kPhaseClk_`, so the two BSRR DMA requests fire concurrently with
the clock edge. One DMA channel streams a `set` word (SBWDIO rises), the other a `reset`
word (SBWDIO falls); the controller services them sequentially, so:

  - **clk-fall → SBWDIO first edge** = compare → DMA-request → BSRR-write latency
    (the "anchor lag");
  - **SBWDIO first edge → second edge** = gap between the two concurrently-triggered
    DMA channels.

Both DMA channels carry the **same software priority**, so the DMA controller's hardware
channel-number arbitration decides which is serviced first. Swapping which physical
channel plays the set/reset role (`kSwapMode == 2`) exposes that natural priority:

  - `kSwapMode == 1` (normal): channel A → set, channel B → reset. SBWDIO idles low,
    pulses HIGH. (When A is the lower-numbered DMA channel it wins arbitration and the
    rise leads.)
  - `kSwapMode == 2` (swapped): channel A → reset, channel B → set. SBWDIO idles high,
    pulses LOW. The host analysis script handles either polarity.

The statistics (min / max / mean over the 100 reps, normal vs swapped) are computed
off-device from the LA capture by `tools/tim_dma_timing.py`; this firmware's only job is
to emit a clean, deterministic burst. See `.claude/docs/drivers/TIM_DMA_TIMING_PROBE.md`.

## Resource model

Reuses the target's SBW assignments (TIM1, the SBWCLK PWM channel, two of the SBW
frozen-compare DMA-trigger channels, SBWDIO/PB14, SBWCLK/PB13). TIM1, DMA1 and the GPIO
ports are already clocked at boot by the platform `PeripheralEnabler`, so no driver setup
is required — the probe runs straight out of `main()`.

## Usage

Include from a `.cpp` (so `WATCHPOINT`/`Trace` from `stdproj.h` are already defined —
NOT from `platform.h`, which is parsed before them). Instantiate with the platform's SBW
constants and call `Run()` once; it never returns (single-shot, like the LA test).
*/

#pragma once

#ifndef STDPROJ_H__INCLUDED__
#	error Include stdproj.h before util/TimDmaTiming.h (needs bmt + WATCHPOINT/Trace)
#endif


namespace TimDmaTiming_ns
{


/**
 * Timer→DMA latency probe.
 *
 * @tparam SysClk            System clock type (provides APBx timer frequency)
 * @tparam kTim              Advanced timer unit (TIM1 — needs a repetition counter)
 * @tparam kClkCh            Timer channel driving the SBWCLK PWM (CH or CHN)
 * @tparam kTrigACh          First frozen-compare channel → DMA channel A
 * @tparam kTrigBCh          Second frozen-compare channel → DMA channel B
 * @tparam SbwClkPin         Bmt::Gpio pin alias that puts the SBWCLK pin in timer AF
 *                           mode (e.g. TIM1_CH1N_PB13<kAlternate,…>); SetupPinMode()
 *                           is called once.
 * @tparam SbwDio            Bmt::Gpio push-pull output pin alias for the pulsed data
 *                           line (must expose `kPin_` and `Io()`, like TimSbw's
 *                           SbwDioOut). SetupPinMode() drives it to its idle level.
 * @tparam kSwapMode         1 = normal channel order, 2 = swapped (set/reset roles).
 * @tparam kMultiplier       Timer ticks per wire-cycle (even, ≥2). 8 mirrors TimSbw.
 * @tparam kFreq             SBWCLK wire frequency in Hz (use a round value near the SBW
 *                           ceiling, e.g. 1 MHz).
 * @tparam kClkComplementary true → SBWCLK on CHN; false → on the regular CH. Mirrors
 *                           TimSbw's idle-high PWM1 configuration exactly.
 */
template <
	typename SysClk
	, const Bmt::Timer::Unit kTim
	, const Bmt::Timer::Channel kClkCh
	, const Bmt::Timer::Channel kTrigACh
	, const Bmt::Timer::Channel kTrigBCh
	, typename SbwClkPin
	, typename SbwDio
	, const int kSwapMode
	, const uint16_t kMultiplier
	, const uint32_t kFreq
	, const bool kClkComplementary = true
>
class TimDmaTiming
{
public:
	/// Number of pulses per capture (= TIM1 repetition count). ≤255 so it fits RCR.
	static constexpr uint16_t kReps = 100;

	static_assert(kMultiplier >= 2u && (kMultiplier % 2u) == 0u,
				  "multiplier must be even and >= 2 (PWM 50% duty / anchor sits at mult/2)");
	static_assert(kSwapMode == 1 || kSwapMode == 2,
				  "OPT_TEST_TIM_DMA_TIMING must be 1 (normal) or 2 (swapped) when enabled");

	// ── Timer ────────────────────────────────────────────────────────────────
	static constexpr uint16_t kCycleTicks_  = kMultiplier;        ///< ticks per wire-cycle
	static constexpr uint16_t kTimerReload_ = kMultiplier - 1u;   ///< ARR (= period-1 → exact kFreq)
	/// SBWCLK 50 % duty = falling-edge anchor; both DMA triggers share this compare.
	static constexpr uint16_t kPhaseClk_    = kMultiplier / 2u;

	using MasterClock = Bmt::Timer::InternalClock_Hz<kTim, SysClk, (uint32_t)kCycleTicks_ * kFreq>;
	// kSingleShot (OPM) + repetition counter: one timer period per SBWCLK cycle, CEN
	// auto-clears after kReps overflows. kBuffered=false (ARPE=0) / kStrictUpdate=true
	// (URS=1) match TimSbw — the Geehy GD32F103 mis-reloads RCR/CCR with ARPE/OCxPE=1.
	using CycleTimer = Bmt::Timer::Any<MasterClock, Bmt::Timer::Mode::kSingleShot, kTimerReload_, false, true>;

	// SBWCLK PWM channel — idle-high, low-going pulse, falling edge at kPhaseClk_.
	// Configured identically to TimSbw::SbwClkOut (proven idle-high on PB13/CH1N):
	// PWM1 + OCxPE preload OFF (constant duty; RCR would defer the UEV otherwise).
	using SbwClkOut = Bmt::Timer::AnyOutputChannel<
		CycleTimer, kClkCh, Bmt::Timer::OutMode::kPWM1,
		kClkComplementary ? Bmt::Timer::Output::kDisabled : Bmt::Timer::Output::kEnabled,
		kClkComplementary ? Bmt::Timer::Output::kEnabled  : Bmt::Timer::Output::kDisabled,
		false, false>;

	// Frozen compare channels — no pin output, just DMA requests at kPhaseClk_.
	using TrigA = Bmt::Timer::AnyOutputChannel<
		CycleTimer, kTrigACh, Bmt::Timer::OutMode::kFrozen,
		Bmt::Timer::Output::kDisabled, Bmt::Timer::Output::kDisabled>;
	using TrigB = Bmt::Timer::AnyOutputChannel<
		CycleTimer, kTrigBCh, Bmt::Timer::OutMode::kFrozen,
		Bmt::Timer::Output::kDisabled, Bmt::Timer::Output::kDisabled>;

	// ── DMA ──────────────────────────────────────────────────────────────────
	// Both channels write one fixed BSRR word per trigger (source fixed = kLongPtr,
	// dest fixed = kLongPtr): kReps identical pulses. EQUAL software priority so the
	// controller's hardware channel-number arbitration — the thing under test —
	// orders them; the kSwapMode role swap exposes it.
	using DmaA = Bmt::Dma::AnyChannel<
		typename TrigA::DmaChInfo_,
		Bmt::Dma::Dir::kMemToPer,
		Bmt::Dma::PtrPolicy::kLongPtr,   // fixed source word
		Bmt::Dma::PtrPolicy::kLongPtr,   // fixed dest (BSRR)
		Bmt::Dma::Prio::kHigh>;
	using DmaB = Bmt::Dma::AnyChannel<
		typename TrigB::DmaChInfo_,
		Bmt::Dma::Dir::kMemToPer,
		Bmt::Dma::PtrPolicy::kLongPtr,
		Bmt::Dma::PtrPolicy::kLongPtr,
		Bmt::Dma::Prio::kHigh>;

	static_assert(CycleTimer::HasRepetitionCounter(),
				  "TimDmaTiming requires an advanced timer with a repetition counter (TIM1)");
	static_assert(DmaA::kChan_ != DmaB::kChan_,
				  "the two trigger channels must map to distinct DMA channels");

	// BSRR words for the pulsed data pin. static inline so the DMA has a stable address.
	static inline uint32_t s_set_word_   = (1u << (uint32_t)SbwDio::kPin_);          ///< BSRR set  → high
	static inline uint32_t s_reset_word_ = (1u << ((uint32_t)SbwDio::kPin_ + 16u));  ///< BSRR reset → low

	/// Generate the deterministic 100-pulse burst and park. Never returns.
	static void Run()
	{
		// ── GPIO (clocks already enabled at boot via PeripheralEnabler) ──
		SbwDio::SetupPinMode();     // data pin: push-pull output, idle at its Level::
		SbwClkPin::SetupPinMode();  // SBWCLK pin: timer alternate-function

		// ── Timer + channels ──
		CycleTimer::Setup();        // PSC / ARR / CR1 (OPM, URS)
		SbwClkOut::Setup();         // PWM1 + MOE (advanced timer)
		TrigA::Setup();
		TrigB::Setup();
		DmaA::Setup();
		DmaB::Setup();

		// SBWCLK falling edge AND both DMA-trigger requests all at kPhaseClk_.
		SbwClkOut::SetCompare(kPhaseClk_);
		TrigA::SetCompare(kPhaseClk_);
		TrigB::SetCompare(kPhaseClk_);

		// Size the scan first (ARR + RCR=kReps-1, then UG), clear stale status — same
		// order as TimSbw::Start(): the UG reload happens before the DMA request lines
		// are armed, so it cannot fire a spurious transfer.
		CycleTimer::SetupRepetition(kTimerReload_, (uint8_t)kReps);
		CycleTimer::ClearStatus();

		// Role assignment — kSwapMode swaps which physical channel sets vs resets.
		uint32_t* srcA = (kSwapMode == 2) ? &s_reset_word_ : &s_set_word_;
		uint32_t* srcB = (kSwapMode == 2) ? &s_set_word_   : &s_reset_word_;

		DmaA::SetTransferCount(kReps);
		DmaA::SetSourceAddress(srcA);
		DmaA::SetDestAddress(&SbwDio::Io().BSRR);
		DmaA::Enable();

		DmaB::SetTransferCount(kReps);
		DmaB::SetSourceAddress(srcB);
		DmaB::SetDestAddress(&SbwDio::Io().BSRR);
		DmaB::Enable();

		// Arm the per-channel DMA request lines (DIER.CCxDE).
		TrigA::EnableDma();
		TrigB::EnableDma();
		CycleTimer::SetCounter(0);          // CNT=0 → clean full first cycle

		Trace() << "TIM->DMA probe: mult " << (int)kMultiplier
				<< " freq " << (int)(kFreq / 1000) << "k ccr " << (int)kPhaseClk_
				<< " reps " << (int)kReps << " mode " << (int)kSwapMode << "\n";

		WATCHPOINT();                       // ── arm the logic analyzer here ──
		CycleTimer::CounterResumeFast();    // fire the burst
		CycleTimer::WaitForAutoStop();      // OPM clears CEN after kReps overflows

		DmaA::Disable();
		DmaB::Disable();
		TrigA::DisableDma();
		TrigB::DisableDma();
		SbwDio::SetupPinMode();             // park the data pin at its idle level

		Trace() << "TIM->DMA probe: done\n";
		WATCHPOINT();
		while (true)
			__WFI();
	}
};


} // namespace TimDmaTiming_ns
