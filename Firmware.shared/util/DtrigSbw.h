/*
# DtrigSbw — Autonomous Spy-Bi-Wire Transport (DRAFT SKELETON)

Companion to DtrigJtag. Design doc: .claude/docs/drivers/DTRIG_SBW_DRIVER.md.

**Status: DRAFT.** This header sketches the template surface and the SBW frame
mechanics. Method bodies are intentionally left as stubs / TODOs so the layout
can be reviewed before implementation. Do not include from production TUs yet
— guard with `OPT_INCLUDE_SBW_DTRIG_` when wiring it up.

## SBW frame model

Each logical JTAG bit becomes a 3-phase wire frame, clocked by SBWCLK rising
edges:

  Cycle 3k+0 : drive SBWDIO with TMS_k     (output)
  Cycle 3k+1 : drive SBWDIO with TDI_k     (output)
  Cycle 3k+2 : release SBWDIO, sample TDO  (input)

So `kBitCount` logical JTAG bits expand to `3 × kBitCount` SBWCLK cycles.

## Resource model

- TIM1 (advanced timer) drives SBWCLK on a CHN output (PB13 = TIM1_CH1N on
  STLinkV2). Period = one SBWCLK cycle. Repetition counter sizes the whole
  scan as a single-shot, same trick as DtrigJtag.
- TIM1 CC events trigger three DMA channels per scan:
    1. BSRR DMA — writes data bit to SBWDIO_PIN every cycle
    2. CRH/CRL DMA — flips SBWDIO pin direction at the TDO phase boundary
    3. IDR DMA — samples GPIOx->IDR every cycle (we discard 2/3 of samples
       in GetResult to keep TIM1's RCR free for the single-shot length)

## Sovereign Init contract

`Init()` claims every resource it needs unconditionally:
  - force GPIO pins (SBWCLK, SBWDIO) to configured AF/output state
  - force-disable then re-Setup() all three DMA channels
  - reset TIM1 channel configuration

There is no claim/release dance vs JtagDev. Switching mode = call the other
mode's Init(). See DTRIG_SBW_DRIVER.md "Resource-ownership rules".
*/

#pragma once

#include "JtagFrame.h"


namespace DtrigSbw_ns
{


/**
 * DtrigSbw — autonomous Spy-Bi-Wire frame generator.
 *
 * Mirrors the surface of `DtrigJtag<>` so `TapDev430*` can sit on either
 * transport without rewriting the protocol layer. The encoder differs: each
 * JTAG bit expands to 3 SBWCLK cycles, with mid-frame data-pin direction
 * flips driven by DMA.
 *
 * @tparam SysClk         System clock type (provides APB2 frequency)
 * @tparam kTim           Advanced timer unit (TIM1 on F1xx — needs RCR)
 * @tparam kSbwClk        Timer channel driving SBWCLK PWM (CH or CHN)
 * @tparam kSbwDataTrig   Compare-only channel: triggers SBWDIO BSRR DMA
 * @tparam kSbwDirTrig    Compare-only channel: triggers SBWDIO CRx-register DMA
 * @tparam kSbwSampleTrig Compare-only channel: triggers IDR sample DMA
 * @tparam SbwDioPin      Bmt::Gpio pin alias for the bidirectional SBWDIO line
 * @tparam DirPolicy      Strategy class for direction control. Required API:
 *                          static constexpr uint32_t kOutputCrx;  // GPIOx->CR? value for output
 *                          static constexpr uint32_t kInputCrx;   // GPIOx->CR? value for input
 *                          static void* CrxAddress();             // &GPIOx->CRL or CRH
 *                        Strategy A (single-pin) targets one CRx register.
 *                        Strategy B (mux pin) would replace this with a BSRR
 *                        bit on a separate direction-select GPIO.
 * @tparam kFreq          SBWCLK frequency (Hz). 1–2 MHz is the practical
 *                        sweet spot on F103; 5 MHz is the MSP430 ceiling.
 * @tparam kScan          DR, IR, or GoIdle (same enum as DtrigJtag)
 * @tparam kNumBits       Payload width (8 / 16 / 20 / 32, or GoIdle sentinel)
 * @tparam kCmpComplementary  true → SBWCLK on CHN (STLinkV2 PB13 = TIM1_CH1N).
 *                            false → SBWCLK on regular CH (future boards).
 */
template <
	typename SysClk
	, const Bmt::Timer::Unit kTim
	, const Bmt::Timer::Channel kSbwClk
	, const Bmt::Timer::Channel kSbwDataTrig
	, const Bmt::Timer::Channel kSbwDirTrig
	, const Bmt::Timer::Channel kSbwSampleTrig
	, typename SbwDioPin
	, typename DirPolicy
	, const uint32_t kFreq
	, JtagFrame::Scan kScan
	, JtagFrame::NumBits kNumBits
	, const bool kCmpComplementary = true
>
class DtrigSbw
{
public:
	// ── Bit-count constants (mirror DtrigJtag) ───────────────────────────────
	static constexpr JtagFrame::Scan kScan_       = kScan;
	static constexpr JtagFrame::NumBits kNumBits_ = kNumBits;

	/// JTAG-level bits required for the selected scan (same as DtrigJtag).
	static constexpr uint8_t kJtagBits =
		(kScan == JtagFrame::Scan::kGoIdle)
			? 8
			: (5 + (uint8_t)kNumBits + (uint8_t)kScan);

	/// SBW wire cycles per scan = 3 × JTAG bits.
	static constexpr uint16_t kSbwCycles = 3u * kJtagBits;

	// ── Timer (TODO: lock down once channel/DMA layout is final) ─────────────
	// TIM1 period = one SBWCLK cycle. RCR-1 = kSbwCycles - 1 → single shot.
	// kSbwClk in PWM mode produces a 50% duty SBWCLK on its pin.
	// Three compare channels at sub-cycle positions trigger:
	//   - data BSRR DMA (early in the cycle, before SBWDIO rising-edge setup)
	//   - direction CRx DMA (only on TDO-phase boundaries; gated via DMA NDTR
	//     count, no separate enable)
	//   - IDR sample DMA (near the falling edge, after target drives TDO)
	// Concrete TimerInternalClock / Any / AnyOutputChannel typedefs go here.

	// ── DMA (TODO) ───────────────────────────────────────────────────────────
	// SbwDataDma  — mem→periph, source = bsrr_script[kSbwCycles], dest = &GPIO->BSRR
	// SbwDirDma   — mem→periph, source = dir_script[kDirFlips],   dest = DirPolicy::CrxAddress()
	// SbwSampleDma — periph→mem, source = &GPIO->IDR,             dest = sample_buf[kSbwCycles]

	// ── Compile-time checks ──────────────────────────────────────────────────
	static_assert(kSbwCycles <= 128,
		"SBW scan too long for ping-pong buffers — increase kSbwBufSize_ in SbwDev");
	// TODO: add the same DMA-channel-distinct asserts as DtrigJtag once the
	// concrete DMA typedefs are in place.

	// ─────────────────────────────────────────────────────────────────────────
	// Lifecycle
	// ─────────────────────────────────────────────────────────────────────────

	/// One-time hardware init. Sovereign: force-overwrites GPIO modes for
	/// SBWCLK and SBWDIO, force-disables every DMA channel before Setup(),
	/// and resets TIM1 channel configuration. Safe to call after any
	/// JtagDev state.
	static ALWAYS_INLINE void Init()
	{
		// TODO:
		//   CycleTimer::Setup();             // PSC for kFreq, ARR = period
		//   SbwClkOut::Setup();              // PWM2 on kSbwClk (CHN), 50% duty
		//   SbwDataTrigger::Setup();         // Frozen, no pin, DMA on CCx
		//   SbwDirTrigger::Setup();
		//   SbwSampleTrigger::Setup();
		//   SbwDataDma::Setup();
		//   SbwDirDma::Setup();
		//   SbwSampleDma::Setup();
		//   SbwDioPin::SetupPinMode();       // Force output, idle low
	}

	static ALWAYS_INLINE void SetupDma()   { /* TODO: re-arm three DMA channels */ }
	static ALWAYS_INLINE void ReleaseDma() { /* TODO: disable three DMA channels */ }

	// ─────────────────────────────────────────────────────────────────────────
	// Buffer rendering
	// ─────────────────────────────────────────────────────────────────────────

	/**
	 * Fills the three DMA scripts for one SBW scan.
	 *
	 * Per JTAG bit i in [0, kJtagBits), writes BSRR words for cycles
	 * (3i+0, 3i+1, 3i+2):
	 *   cycle 3i+0 : TMS bit value (from the TMS waveform for this scan)
	 *   cycle 3i+1 : TDI bit value (from data_out, with tclk_high fill for
	 *                head/tail bits like DtrigJtag's RenderTransaction)
	 *   cycle 3i+2 : don't-care (SBWDIO released to input — value is the
	 *                next bit's TMS so the BSRR write can be skipped if
	 *                the direction flip is timed to this cycle)
	 *
	 * Direction script: 2 flips per scan (output→input at first TDO cycle,
	 * input→output at end of frame).
	 *
	 * @param bsrr_script   Output: kSbwCycles uint32_t for BSRR DMA
	 * @param dir_script    Output: 2 uint32_t for CRx DMA
	 * @param tclk_high     Current TCLK level (head/tail fill)
	 * @param data_out      Payload to shift; MSB first (same as DtrigJtag)
	 */
	static ALWAYS_INLINE void RenderTransaction(
		uint32_t* bsrr_script
		, uint32_t* dir_script
		, bool tclk_high
		, uint32_t data_out
	)
	{
		static_assert(kScan_ != JtagFrame::Scan::kGoIdle,
			"Use DoGoIdle() for the GoIdle sequence");
		(void)bsrr_script; (void)dir_script; (void)tclk_high; (void)data_out;
		// TODO:
		//   - build packed (TMS, TDI) bit-planes using the pack-and-__REV
		//     trick from DtrigJtag for both columns
		//   - expand the two planes into 3 BSRR words per bit via a small
		//     loop or 8-entry LUT (the encoding is regular, so this stays
		//     ~5 instructions per bit)
		//   - direction flips at cycles (3*kFirstShiftBit+2) and
		//     (3*kJtagBits) — precomputed at compile time
	}

	/// GoIdle: 6× TMS=1, 1× TMS=0, TDI=1 throughout (matches DtrigJtag).
	static ALWAYS_INLINE void DoGoIdle(
		uint32_t* bsrr_script
		, uint32_t* dir_script
		, uint16_t cnt_offset
	)
	{
		static_assert(kScan_ == JtagFrame::Scan::kGoIdle,
			"DoGoIdle() requires a kGoIdle instantiation");
		(void)bsrr_script; (void)dir_script; (void)cnt_offset;
		// TODO: render the TAP-reset frame and launch via Start/Wait
	}

	// ─────────────────────────────────────────────────────────────────────────
	// Execution
	// ─────────────────────────────────────────────────────────────────────────

	/**
	 * Launches a pre-rendered SBW scan. TIM1, data DMA, dir DMA and sample
	 * DMA are armed inside a critical section so all three are phase-locked
	 * to the first SBWCLK edge.
	 *
	 * @param bsrr_script  kSbwCycles BSRR words (from RenderTransaction)
	 * @param dir_script   2 CRx words for direction flips
	 * @param sample_buf   kSbwCycles slots for IDR DMA destination
	 * @param cnt_offset   Per-grade TIM1 CNT trim (LA-calibrated)
	 */
	static OPTIMIZED void Start(
		uint32_t* bsrr_script
		, uint32_t* dir_script
		, uint32_t* sample_buf
		, uint16_t cnt_offset
	)
	{
		(void)bsrr_script; (void)dir_script; (void)sample_buf; (void)cnt_offset;
		// TODO:
		//   - set TIM1 ARR = kPeriod, RCR = kSbwCycles - 1
		//   - arm SbwDataDma   (mem→BSRR, kSbwCycles transfers)
		//   - arm SbwDirDma    (mem→CRx, 2 transfers, count-gated)
		//   - arm SbwSampleDma (IDR→mem, kSbwCycles transfers)
		//   - CriticalSection { TIM1->CNT = kCntStart - cnt_offset; TIM1->CR1 |= CEN; }
	}

	/// Wait for single-shot completion + final sample DMA flush.
	static ALWAYS_INLINE void Wait()
	{
		// TODO: WaitForAutoStop, then SbwSampleDma TC poll, then disable all three DMAs
	}

	// ─────────────────────────────────────────────────────────────────────────
	// Result decoding
	// ─────────────────────────────────────────────────────────────────────────

	/**
	 * Extracts the received TDO payload from the IDR sample buffer.
	 *
	 * Only cycles (3i+2) carry valid TDO; the other 2/3 are discarded. The
	 * relevant bit position inside each sampled IDR word is the SBWDIO pin
	 * index. After masking, pack the bits MSB-first into a uint32_t —
	 * symmetric reverse of RenderTransaction's payload-bit positions.
	 *
	 * @param sample_buf  kSbwCycles IDR samples from Start()/Wait()
	 * @return            Received payload, LSB-aligned
	 */
	static ALWAYS_INLINE uint32_t GetResult(const uint32_t* sample_buf)
	{
		(void)sample_buf;
		// TODO:
		//   for (i = 0; i < kJtagBits; ++i) {
		//       uint32_t s = sample_buf[3*i + 2];
		//       out = (out << 1) | ((s >> kSbwDioBit) & 1);
		//   }
		//   apply payload-window mask + shift like DtrigJtag
		return 0u;
	}
};


} // namespace DtrigSbw_ns


namespace WaveJtag
{
	using DtrigSbw_ns::DtrigSbw;
}
