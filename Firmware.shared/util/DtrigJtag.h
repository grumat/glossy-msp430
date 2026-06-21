/*
# DtrigJtag — Double-Trigger SPI+TIM1 JTAG Frame Generator (PWM-TMS edition)

## Concept
Synchronises SPI1 and TIM1 to act as a combined JTAG driver for hardware where the
JTAG signals are split across GPIO ports (e.g. ST-Link V2 clone, BluePill). Achieves
9 MHz JTAG clock on STM32F1 by running both peripherals off the same APB2 prescaler
and coupling them via a critical section for phase-locked startup.

  - SPI1 SCK  (PA5) = JTCK    — SPI clock, drives JTCK
  - SPI1 MOSI (PA7) = JTDI    — shift register output, byte-level DMA (one 8-bit transaction per JTCK byte)
  - SPI1 MISO (PA6) = JTDO    — shift register input, byte-level DMA
  - TIM1 CHx  (varies) = JTMS — output driven by hardware PWM, zero per-bit DMA cost

The TMS pin can sit on either a regular CH (e.g. BluePill PA10 = TIM1_CH3) or a
complementary CHN (e.g. STLinkV2 PB14 = TIM1_CH2N). Selection is via the
`kCmpComplementary` template flag — see the parameter docs below.

## TMS Generation — PWM + One CCR Reload

TIM1 runs as a single long shot covering the full frame (kTotalClocks × 8 timer ticks,
ARR = kTotalClocks × 8 − 1). The timer multiplier = 8 gives one timer tick per JTCK
cycle. Both CH and CHN paths arrive at the same TMS waveform (HIGH for the entry
pulse, LOW for the shift portion) but via different combinations:

  CHN config (kCmpComplementary=true) : CHN polarity active-high, PWM2.
                                        CHN inherently inverts OCREF (dead-time
                                        generator), so CHN_pin = NOT_OCREF.
                                        PWM2 + CNT<CCR → OCREF=0 → pin=HIGH.
  CH  config (kCmpComplementary=false): CH  with Output::kInverted (CCxP=1), PWM1.
                                        PWM1 + CNT<CCR → OCREF=1.
                                        Empirically on STM32F103 TIM1_CH3, the pin
                                        with CCxP=1 ends up reflecting OCREF (not
                                        NOT_OCREF as the RM would suggest); whatever
                                        the silicon actually does, this combination
                                        is what produces TMS=HIGH for the entry pulse
                                        on a bluepill.

OCREF state is "frozen" while the timer is stopped between frames, and the AF pin
shows that frozen value the moment GPIO→AF takeover happens. The Setup-time OutMode
(kForceInactive for CHN, kForceActive for CH) pins OCREF at the value that yields
pin=HIGH idle, so the takeover is glitch-free and the entry pulse is HIGH from the
very first JTCK cycle.

Three toggle events are armed via CCR2, reloaded mid-frame by two auxiliary DMA channels:

  Event 1 — count kEntry×8       : toggle → TMS LOW   (end of entry pulse)
             CC3 DMA fires simultaneously → reloads CCR2 = kExitStart×8
  Event 2 — count kExitStart×8   : toggle → TMS HIGH  (exit pulse active: Exit1 + Update)
             CC4 DMA fires simultaneously → reloads CCR2 = kRtiStart×8
  Event 3 — count kRtiStart×8    : toggle → TMS LOW   (RTI, end of frame)

For GoIdle: kEntry=6, kExitStart=kRtiStart=kTotalClocks (> ARR) → only one toggle, no exit.

## DMA budget vs old DtrigJtag
  Old:  1 DMA per JTCK cycle for TMS + 1 DMA per 8 cycles for SPI byte
  New:  2 single-element DMA writes for TMS + 1 DMA per 8 cycles for SPI byte

## Synchronisation — Critical Section & Phase-Locking
SPI baud = APB2 / N, TIM1 period = N APB2 cycles (both 8-count per JTCK cycle).
A critical section in Start() resumes TIM1 first, then preloads SPI DR (which
starts SCK on the next APB cycle), then arms the TX DMA for bytes 1..N-1.
Timer-before-SPI gives OCxREF one or more CK_INT ticks to latch the entry-pulse
state before SPI emits its first SCK edge — required at high JTAG speeds where
the OCxREF settling window is shorter than one TCK cycle.

The `cnt_offset` parameter (calibrated per speed grade with a logic analyzer)
trims TIM1 CNT to align TMS toggles within each SPI bit cell, compensating for
fixed-CPU-cycle latency between the two starts.

Direction convention: `SetCounter(kCntStart_ - cnt_offset)`.
Increasing `cnt_offset` shifts the TMS waveform to the *right* (later in time)
in a logic-analyzer view — natural reading direction.

## TMS pin mode
  Start() : the JTMS pin (PB14 on STLinkV2, PA10 on BluePill) is switched to TIM1
            alt-function by the platform's JTMS_PWM::SetupPinMode().
  Wait()  : pin stays in AF, TMS remains LOW (RTI) between frames.
  Bit-bang: caller must call JTMS::SetupPinMode() (restores GPIO) before GPIO bit-bang ops.
*/

#pragma once

#include "JtagFrame.h"


namespace DtrigJtag_ns
{


/**
 * DtrigJtag — synchronised SPI + TIM1 JTAG generator with hardware TMS.
 *
 * The TMS channel is driven in PWM mode for zero-DMA pulse generation. An auxiliary
 * compare channel (kTmsRld1) triggers a single-element DMA write that reloads the
 * TMS CCR at the entry→shift transition (and at the exit→RTI transition for IR/DR).
 *
 * The TMS output can be on either the regular CH or the complementary CHN of the
 * timer, selected at compile time by `kCmpComplementary`. The two paths differ only
 * in which output is enabled and whether OCREF runs in PWM1 or PWM2 — both yield
 * the same TMS-HIGH-during-entry, TMS-LOW-after-toggle waveform.
 *
 * @tparam SysClk            System clock type (provides APB2 frequency)
 * @tparam kTim              Advanced timer unit (must have repetition counter; TIM1 on F1xx)
 * @tparam kTms              Timer channel that drives JTMS (CH or CHN; see kCmpComplementary)
 * @tparam kTmsRld1          Compare-only channel for the CCR-reload DMA trigger
 * @tparam SpiDevice         SPI peripheral type (provides DmaChInfoTx_, DmaChInfoRx_)
 * @tparam kFreq             JTAG clock frequency; must equal SpiDevice baud rate
 * @tparam kScan             DR, IR, or GoIdle
 * @tparam kNumBits          Payload width (8 / 16 / 20 / 32, or GoIdle sentinel)
 * @tparam kCmpComplementary true  → TMS sits on CHN (e.g. STLinkV2 PB14 = TIM1_CH2N).
 *                           false → TMS sits on regular CH (e.g. BluePill PA10 = TIM1_CH3).
 */
template <
	typename SysClk
	, const Timer::Unit kTim
	, const Timer::Channel kTms		///< Channel of the JTMS pin (CH or CHN, per kCmpComplementary)
	, const Timer::Channel kTmsRld1	///< Compare-only channel: DMA trigger reloads TMS CCR at entry end
	, typename SpiDevice
	, const uint32_t kFreq
	, JtagFrame::Scan kScan
	, JtagFrame::NumBits kNumBits
	, const bool kCmpComplementary = true	///< true: TMS on CHN (default, STLinkV2); false: TMS on CH (BluePill)
>
class DtrigJtag
{
public:
	// The Spi data type
	using SpiDevice_ = SpiDevice;
	// ── Timer ────────────────────────────────────────────────────────────────
	// Timer runs a factor faster than SPI bit rate, so we have room to trim latencies
	static constexpr uint16_t kTimerMultiplier_ = 8;
	/// TIM1 input clock = 8 × kFreq: one 8-count period = one JTCK cycle
	using MasterClock = Timer::InternalClock_Hz<kTim, SysClk, kTimerMultiplier_ * kFreq>;
	/// Period is falling edge to falling edge of TMS fix. But we have to adjust initial CNT for first TMS pulse.
	static constexpr uint16_t kTimerPeriod_ = (kScan == JtagFrame::Scan::kGoIdle)
												? 8 * kTimerMultiplier_
												: (3 + (uint16_t)kNumBits) * kTimerMultiplier_;
	/// Single-shot; ARR=kTimerPeriod_ sets the prescaler in Init().  Start() overrides ARR per frame.
	using CycleTimer = Timer::Any<MasterClock, Timer::Mode::kSingleShot, kTimerPeriod_, false, true>;

	/// TMS PWM channel.
	///   CHN path  (kCmpComplementary=true,  STLinkV2 PB14 = TIM1_CH2N): Output::kEnabled
	///                                       on the CHN side. Dead-time generator inverts
	///                                       OCREF so CHN_pin = NOT_OCREF. Combined with
	///                                       PWM2 below, pin = HIGH while CNT<CCR.
	///   CH  path  (kCmpComplementary=false, BluePill PA10 = TIM1_CH3 ): Output::kInverted
	///                                       on the main CH side (CCxP=1). Combined with
	///                                       PWM1 below, the pin sits HIGH while CNT<CCR
	///                                       on F103. NOTE: the RM says CCxP=1 should
	///                                       invert the OC3 pin, but empirically with this
	///                                       configuration the F103 silicon delivers the
	///                                       waveform we want; we don't have a clean
	///                                       explanation for why CCxP=1 is required here
	///                                       rather than CCxP=0, but the analyzer is
	///                                       definitive (see DEBUG dump in Init()).
	/// Initial OutMode is kForceInactive (CHN path: OCREF=0, CHN_pin=HIGH) /
	/// kForceActive (CH path: OCREF=1, CH_pin=HIGH on this F103). The AF takeover at the
	/// start of bit-bang→timer handoff sees a stable HIGH on the JTMS pin regardless of
	/// the previous frame's residue.
	using TmsOut = Timer::AnyOutputChannel<
		CycleTimer, kTms
		, kCmpComplementary ? Timer::OutMode::kForceInactive : Timer::OutMode::kForceActive
		, kCmpComplementary ? Timer::Output::kDisabled : Timer::Output::kInverted		///< main CH
		, kCmpComplementary ? Timer::Output::kEnabled  : Timer::Output::kDisabled		///< complementary CHN
		, false ///< no CCR preload
		, false ///< no fast enable
		>;

	/// PWM mode used during a frame.
	///   CHN path: PWM2 — OCREF=0 while CNT<CCR; CHN inherently inverts → pin=HIGH for entry.
	///   CH  path: PWM1 — OCREF=1 while CNT<CCR. With Output::kInverted (CCxP=1) on F103
	///             this combination produces pin=HIGH for entry empirically; see the
	///             TmsOut comment above.
	/// Both modes flip OCREF to the opposite state once CNT crosses CCR.
	static constexpr Timer::OutMode kFramePwmMode_ =
		kCmpComplementary ? Timer::OutMode::kPWM2 : Timer::OutMode::kPWM1;

	/// OutMode used between frames to hold the AF pin at HIGH (entry-pulse state).
	/// Same as the Setup initial mode; see TmsOut comment above.
	static constexpr Timer::OutMode kIdleOutMode_ =
		kCmpComplementary ? Timer::OutMode::kForceInactive : Timer::OutMode::kForceActive;

	/// CH3: compare-only (Frozen mode, no pin), generates CC3 DMA request at entry end
	using TriggerRld1 = Timer::AnyOutputChannel<
		CycleTimer, kTmsRld1 ///< Channel::k3 → DMA1_CH6
		, Timer::OutMode::kFrozen, Timer::Output::kDisabled, Timer::Output::kDisabled>;

	// ── DMA — CCR2 reloads ────────────────────────────────────────────────────
	/// Triggered by CC3 at count kEntry×8; writes kCcr2ExitStart_ → CCR2
	using DmaCcr2Rld1 = Dma::AnyChannel<
		typename TriggerRld1::DmaChInfo_, Dma::Dir::kMemToPer, Dma::PtrPolicy::kLongPtr ///< source: single uint32_t in flash
		, Dma::PtrPolicy::kLongPtr ///< dest: CCR2 register (fixed)
		, Dma::Prio::kHigh>;

	// ── DMA — SPI TX/RX ──────────────────────────────────────────────────────
	/// Feeds SPI DR with JTDI bytes (one DMA per 8 JTCK cycles)
	using SpiTxDma = Dma::AnyChannel<
		typename SpiDevice::DmaChInfoTx_, Dma::Dir::kMemToPer, Dma::PtrPolicy::kBytePtrInc, Dma::PtrPolicy::kBytePtr, Dma::Prio::kHigh>;

	/// Drains SPI DR into the JTDO receive buffer
	using SpiRxDma = Dma::AnyChannel<
		typename SpiDevice::DmaChInfoRx_, Dma::Dir::kPerToMem, Dma::PtrPolicy::kBytePtr, Dma::PtrPolicy::kBytePtrInc, Dma::Prio::kVeryHigh>;

	// ── Bit-count constants ───────────────────────────────────────────────────
	static constexpr JtagFrame::Scan kScan_ = kScan;
	static constexpr JtagFrame::NumBits kNumBits_ = kNumBits;

	/// Total JTCK cycles required for the selected scan type and payload
	static constexpr uint8_t kBitCount =
		(kScan == JtagFrame::Scan::kGoIdle)
			? 8
			: (5 + (uint8_t)kNumBits + (uint8_t)kScan);
	/// SPI bytes needed (whole bytes only; may include padding clocks)
	static constexpr uint8_t kSpiBytes = (kBitCount + 7) / 8;
	/// Actual JTCK cycles clocked (= kSpiBytes × 8, includes padding)
	static constexpr uint8_t kTotalClocks = kSpiBytes * 8;

	// ── TMS timing constants (in JTCK cycles) ──────────────────────────────
	/// Cycles where TMS=1 for the entry pulse (Sel-DR, Sel-DR+Sel-IR, or test-logic-reset)
	static constexpr uint8_t kEntry =
		(kScan == JtagFrame::Scan::kIR)
			? 2
			: 1;

	/// CCR2 value for the entry pulse (TMS goes LOW once CNT crosses this).
	/// Pulse width in ticks = ARR − CCR + 1 (closed interval), so CCR is set
	/// such that the pulse spans exactly kEntry × kTimerMultiplier_ ticks
	/// (= kEntry full JTCK cycles).  The +1 cancels the closed-interval count.
	/// GoIdle uses a sentinel value chosen so the resulting pulse covers the
	/// entire 6-clock TMS=1 prelude required by SLAU320 TAP-reset.
	static constexpr uint32_t kTmsHigh1 =
		(kScan == JtagFrame::Scan::kGoIdle)
			? 2 * kTimerMultiplier_ // GoIdle: 6 JTCK of TMS=1
		: (kScan == JtagFrame::Scan::kIR)
			? kTimerPeriod_ - 2 * kTimerMultiplier_ + 1	// 2 JTCK pulse (Sel-DR + Sel-IR)
			: kTimerPeriod_ - 1 * kTimerMultiplier_ + 1;	// 1 JTCK pulse (Sel-DR)
	/// Initial CNT value for the timer at frame start.  CNT counts up, wraps
	/// at ARR.  The choice puts the first CCR cross (= entry-pulse end, TMS
	/// falling edge) at a known phase relative to SPI's first SCK edge; the
	/// per-speed `cnt_offset` parameter trims around it.
	///
	/// Direction: `SetCounter(kCntStart_ - cnt_offset)` — increasing
	/// `cnt_offset` lowers the starting CNT, adding more ticks before the CCR
	/// cross, which shifts the TMS waveform to the *right* in the LA. This
	/// matches the intuition you get when scrolling a capture forward.
	static constexpr uint32_t kCntStart_ =
		(kScan == JtagFrame::Scan::kGoIdle)
			? 3 * kTimerMultiplier_
			: kTimerPeriod_ - 10 * kTimerMultiplier_ / 4
			;

	static inline uint32_t tmsHigh2;

	static_assert(kTotalClocks <= 40,
				"Transaction too large for JtagDev read_buf_ (40 words). Increase kPingPongBufSize_.");
	static_assert(kSpiBytes <= 8,
				"Transaction too large for the SPI TX/RX buffers (8 bytes).");

	// ─────────────────────────────────────────────────────────────────────────
	// Init / DMA lifecycle
	// ─────────────────────────────────────────────────────────────────────────

	/// One-time hardware initialisation.  Sets TIM1 prescaler and SPI baud rate.
	static ALWAYS_INLINE void Init()
	{
		static_assert(CycleTimer::HasRepetitionCounter(),
					"DtrigJtag requires an advanced timer with repetition counter (e.g. TIM1)");
		static_assert(DmaCcr2Rld1::kChan_ != SpiTxDma::kChan_,
					"kTmsRld1 DMA conflicts with SPI TX DMA — choose a different kTmsRld1 channel");
		static_assert(DmaCcr2Rld1::kChan_ != SpiRxDma::kChan_,
					"kTmsRld1 DMA conflicts with SPI RX DMA — choose a different kTmsRld1 channel");

		CycleTimer::Setup();	// sets PSC, CR1 mode bits; ARR is set per-frame in Start()
		TmsOut::Setup();		// toggle mode, CH2N enabled, MOE=1
		TriggerRld1::Setup(); // CH3 frozen (no pin), DMA request enabled in Start()

		DmaCcr2Rld1::Setup();
		SpiTxDma::Setup();
		SpiRxDma::Setup();

		SpiDevice::Setup();
		SpiDevice::EnableDma(); // must follow Init(): Init() writes CR2 and clears TXDMAEN/RXDMAEN
	}

	/// Re-arms DMA channels after they were released (e.g. for JtclkWaveGen)
	static ALWAYS_INLINE void SetupDma()
	{
		DmaCcr2Rld1::Setup();
		SpiTxDma::Setup();
		SpiRxDma::Setup();
	}

	/// Releases DMA channels for repurposing (e.g. JtclkWaveGen)
	static ALWAYS_INLINE void ReleaseDma()
	{
		DmaCcr2Rld1::Disable();
		SpiTxDma::Disable();
		SpiRxDma::Disable();
	}

	/// Applies speed-sensitive registers only (TIM1 PSC and SPI CR1[BR]).
	/// Call this instead of Init() when switching speed grades at runtime.
	/// CR1 mode bits and DMA setup are left untouched.
	static ALWAYS_INLINE void ApplySpeed()
	{
		CycleTimer::SetPrescaler(CycleTimer::kPrescaler_);
		CycleTimer::TD::GetDevice()->EGR = TIM_EGR_UG;		// latch new PSC immediately
		SpiDevice::SetupSpeed();	// writes CR1 without SPE — disables SPI temporarily
		SpiDevice::Enable();		// restore SPE
	}

	// ─────────────────────────────────────────────────────────────────────────
	// Buffer rendering
	// ─────────────────────────────────────────────────────────────────────────

	/**
	 * Fills the SPI TX byte buffer for one JTAG scan.  TMS is handled entirely
	 * by hardware; no TMS buffer is needed.
	 *
	 * SPI byte layout: MSB-first, each bit corresponds to one JTCK cycle.
	 *   Bit position i  →  byte[i/8] bit (7 - i%8)
	 *
	 * Stream layout (uniform across IR/DR, all widths):
	 *   bits [0..3]                             head fill   (TDI = tclk_high)
	 *   bits [4..(4 + kNumBits - 1)]            payload     (MSB first)
	 *   bits [(4 + kNumBits)..(8*kSpiBytes-1)]  tail fill   (TDI = tclk_high)
	 *
	 * Encoding trick (lifted from the deprecated SpiJtagDataShift::Transmit
	 * — see git commit af121e4b for the original): pack the entire frame in
	 * a register with stream bit 0 at register bit 31, OR-in the fill mask,
	 * byte-reverse with the single-cycle Cortex-M `__REV` instruction, then
	 * one store. ~4 instructions for widths ≤ 20 bits, ~10 for 32 bits —
	 * 50–80× faster than a per-bit loop.
	 *
	 * Underlying storage is `uintptr_t[]` (4-byte aligned via PingPongBuffer's
	 * ALIGNED attribute), so `__builtin_memcpy` lowers to a single aligned STR.
	 *
	 * @param tdi_bytes  Output: kSpiBytes bytes for SPI TX (4-byte aligned)
	 * @param tclk_high  Current JTDI/TCLK level (maintained during idle/update bits)
	 * @param data_out   Payload to shift; MSB is sent first
	 */
	static ALWAYS_INLINE void RenderTransaction(
		uint8_t* tdi_bytes
		, bool tclk_high
		, uint32_t data_out
	)
	{
		static_assert(kScan_ != JtagFrame::Scan::kGoIdle,
			"Use DoGoIdle() for the GoIdle sequence");

		if constexpr ((uint8_t)kNumBits_ <= 20)
		{
			// Single uint32_t covers the whole frame (kSpiBytes ≤ 4).
			//   W bit 31         → stream bit 0 (head fill)
			//   W bit (28-1)..28 → stream bits 4 → kNumBits MSBs of payload
			//   ...
			constexpr uint32_t kDataShift  = 28u - (uint32_t)kNumBits_;
			constexpr uint32_t kPayloadMsk = (((uint32_t)1 << (uint32_t)kNumBits_) - 1u) << kDataShift;
			constexpr uint32_t kFillMask   = ~kPayloadMsk;

			uint32_t w = data_out << kDataShift;
			if (tclk_high)
				w |= kFillMask;
			w = __REV(w);
			__builtin_memcpy(tdi_bytes, &w, sizeof(w));
		}
		else
		{
			// kNumBits_ == 32: payload (32) + 4 head fill + 4 tail fill = 40 bits = 5 bytes.
			// Straddles 4-byte boundary; one uint32_t store + one byte store.
			//   W_hi (stream bits 0..31)  : [head fill: 4] [payload[31..4]: 28]
			//   byte4 (stream bits 32..39): [payload[3..0]: 4] [tail fill: 4]
			uint32_t w_hi      = data_out >> 4;
			uint8_t  w_lo_byte = static_cast<uint8_t>((data_out & 0xFu) << 4);
			if (tclk_high)
			{
				w_hi      |= 0xF0000000u;	// head-fill nibble at top of W_hi
				w_lo_byte |= 0x0Fu;			// tail-fill nibble at bottom of byte 4
			}
			w_hi = __REV(w_hi);
			__builtin_memcpy(tdi_bytes, &w_hi, sizeof(w_hi));
			tdi_bytes[4] = w_lo_byte;
		}
	}

	/**
	 * Fills the SPI TX buffer for GoIdle (TAP reset): 6× TMS=1 + 1× TMS=0 + 1 pad.
	 * TMS is driven by TIM1_CH2N (kEntry=6 toggle, no exit pulse).
	 * Uses a slower prescaler (CycleTimer::kPrescaler_) for safe TAP reset timing.
	 */
	static ALWAYS_INLINE void DoGoIdle(
		uint8_t* tdi_bytes
		, uint16_t cnt_offset
	)
	{
		static_assert(kScan_ == JtagFrame::Scan::kGoIdle,
			"DoGoIdle() requires a kGoIdle instantiation");

		// TDI=1 throughout (TDO needs to be high for a valid fuse test that follows this frame)
		__builtin_memset(tdi_bytes, 0xff, kSpiBytes);

		// Force grade-1 on BOTH peripherals before the TAP-reset frame. The
		// previous shift may have ramped SPI baud and TIM PSC up to a faster
		// grade (via SetSpeed); SetPrescaler alone would slow only the timer
		// while SPI keeps clocking fast, splitting TMS and JTCK onto two
		// timebases and derailing the frame. ApplySpeed re-pins both (TIM PSC
		// latched via UG + SPI CR1[BR]) since this template is bound to
		// JTCK_Speed_1.
		ApplySpeed();
		Start(tdi_bytes, nullptr, cnt_offset);
		Wait();
	}

	// ─────────────────────────────────────────────────────────────────────────
	// Execution
	// ─────────────────────────────────────────────────────────────────────────

	/**
	 * Starts a pre-rendered transaction.  TIM1 and SPI are launched inside a
	 * critical section so their clocks are phase-locked from the very first bit.
	 *
	 * TMS sequence:
	 *   [0 .. kEntry-1]          TMS=1  (entry pulse, CH2N HIGH from Force-Inactive start)
	 *   [kEntry .. kExitStart-1] TMS=0  (Capture + Shift, after first toggle)
	 *   [kExitStart .. kRtiStart-1] TMS=1 (Exit1 + Update, after second toggle)
	 *   [kRtiStart ..]           TMS=0  (RTI, after third toggle)
	 *
	 * PB14 is switched to TIM1_CH2N AF mode inside this call.  Caller must call
	 * JTMS::SetupPinMode() before any GPIO bit-bang of TMS after Wait() returns.
	 *
	 * @param tdi_bytes   kSpiBytes of SPI TX data (from RenderTransaction)
	 * @param tdo_bytes   kSpiBytes buffer for SPI RX data (nullptr for GoIdle)
	 * @param cnt_offset  TIM1 CNT trim, in timer ticks.  Subtracted from
	 *                    kCntStart_, so increasing values shift TMS to the
	 *                    right (later) on the LA — matches the natural reading
	 *                    direction.  Calibrated per speed grade.
	 */
	static OPTIMIZED void Start(
		uint8_t* tdi_bytes
		, uint8_t* tdo_bytes
		, uint16_t cnt_offset
	)
	{
		// Force OCREF to the entry-pulse state (kForceInactive for CHN path,
		// kForceActive for CH path) before configuring the frame. The previous
		// frame may have left OCREF parked at the opposite value, and OCREF does
		// not re-evaluate while the timer is stopped — so without this, the AF
		// pin would reflect stale OCREF until the first clock tick of the next
		// frame. The kFramePwmMode_ switch below picks up cleanly with CNT<CCR
		// keeping OCREF at the same level.
		TmsOut::SetOutputMode(kIdleOutMode_);

		// CCR2: TMS goes LOW once CNT crosses this value (entry-pulse end).
		// kTmsHigh1 is sized so the closed-interval pulse width equals exactly
		// kEntry × kTimerMultiplier_ ticks — see kTmsHigh1 doc above.
		TmsOut::SetCompare((uint16_t)kTmsHigh1);
		if (kScan == JtagFrame::Scan::kDR)
		{
			// CC3 fires at the same CNT as CCR2 → DMA reloads CCR2 with tmsHigh2
			// (the exit-pulse compare value, set further down).
			TriggerRld1::SetCompare((uint16_t)kTmsHigh1);
		}

		if (kScan == JtagFrame::Scan::kGoIdle)
			CycleTimer::SetupRepetition(kTimerPeriod_, 1);
		else
			CycleTimer::SetupRepetition(kTimerPeriod_, 2);
		CycleTimer::ClearStatus();
		// Engage the per-frame PWM mode: PWM2 for CHN, PWM1 for CH. The mode +
		// CCxP polarity + (CHN-only) dead-time inversion combination is set up
		// so the pin pulses HIGH during CNT ∈ [CCR, ARR] of each cycle and is
		// LOW elsewhere. With CCR placed near ARR, the pulse appears at the
		// *tail* of every cycle — kEntry × kTimerMultiplier_ ticks wide for
		// the entry pulse, then again for the exit pulse after the CCR2
		// reload. See kFramePwmMode_ and TmsOut comments for the per-platform
		// derivation.
		TmsOut::SetOutputMode(kFramePwmMode_);

		// CCR2 value reloaded mid-frame for the exit pulse.  Same closed-interval
		// math as kTmsHigh1: ARR − CCR + 1 = 2 × kTimerMultiplier_ → 2 JTCK pulse
		// covering Exit1-IR/DR + Update.
		tmsHigh2 = kTimerPeriod_ - 2 * kTimerMultiplier_ + 1;
		if (kScan == JtagFrame::Scan::kDR)
		{
			DmaCcr2Rld1::Start(&tmsHigh2, TmsOut::GetCcrAddress(), 1);
			TriggerRld1::EnableDma();
		}

		// Arm SPI DMA.  Byte 0 is preloaded into DR inside the critical section
		// (see below); TX DMA covers bytes 1..N-1 only.  This makes SPI startup
		// deterministic — one CPU cycle from DR write to first SCK — instead
		// of bounded by DMA arbitration latency on EnableFast().
		SpiRxDma::SetTransferCount(kSpiBytes);
		if constexpr (kSpiBytes > 1)
		{
			SpiTxDma::SetTransferCount(kSpiBytes - 1);
			SpiTxDma::SetSourceAddress(tdi_bytes + 1);
			SpiTxDma::SetDestAddress(&SpiDevice::GetDevice()->DR);
		}
		const uint8_t kFirstTxByte = tdi_bytes[0];

		if (tdo_bytes)
		{
			SpiRxDma::SetDestAddress(tdo_bytes);
			SpiRxDma::SetSourceAddress(&SpiDevice::GetDevice()->DR);
			SpiRxDma::Enable();
		}

		CycleTimer::SetCounter(kCntStart_ - cnt_offset);

		// ── Critical section: preload SPI DR and start TIM1 together ─────────
		// WriteChar() spin on TXE is a no-op here: Wait() left SPI idle with DR
		// empty (TXE=1).  Writing DR moves byte 0 to the shift register; SPI
		// begins clocking immediately.  EnableFast() then arms DMA so bytes
		// 1..N-1 flow in via the next TXE assertions.
		// NOTE: this shifts the SPI-to-TIM1 phase relative to the old
		// "EnableFast triggers first byte" path — kDtrigCntOffset_* in
		// JtagDev.dtrig.cpp must be retrimmed with a logic analyzer.
		{
			CriticalSection lock;
			CycleTimer::CounterResumeFast();	// TIM1 begins; TMS=HIGH until count kEntry*8
			SpiDevice::WriteChar(kFirstTxByte);	// SPI starts shifting byte 0
			if constexpr (kSpiBytes > 1)
				SpiTxDma::EnableFast();			// DMA feeds bytes 1..N-1 on TXE
		}
	}

	/// Waits for the frame to complete then cleans up DMA.
	/// PB14 stays in TIM1_CH2N AF mode with TMS=LOW (RTI) after this returns.
	static ALWAYS_INLINE void Wait()
	{
		CycleTimer::WaitForAutoStop();
		// SPI may still be clocking the last byte; wait for BSY to clear
		while (SpiDevice::IsBusy())
			;
		// Flush any byte left in RXDR (e.g. DoGoIdle ran without RX DMA active).
		// Reading DR is harmless when RXNE=0 and clears RXNE when 1.
		SpiDevice::DummyRead();
		TriggerRld1::DisableDma();
		DmaCcr2Rld1::Disable();
		SpiTxDma::Disable();
		SpiRxDma::Disable();
	}

	// ─────────────────────────────────────────────────────────────────────────
	// Result decoding
	// ─────────────────────────────────────────────────────────────────────────

	/**
	 * Extracts the received payload from the SPI RX byte buffer.
	 * Symmetric reverse of RenderTransaction's encoding: load the frame into
	 * a register, byte-reverse with `__REV`, shift and mask to extract the
	 * payload bits.  ~4 instructions for widths ≤ 20 bits, ~6 for 32 bits.
	 *
	 * @param tdo_bytes  SPI RX buffer filled by Start()/Wait() (4-byte aligned)
	 * @return           Received payload, LSB-aligned in the returned uint32_t
	 */
	static ALWAYS_INLINE uint32_t GetResult(const uint8_t* tdo_bytes)
	{
		if constexpr ((uint8_t)kNumBits_ <= 20)
		{
			constexpr uint32_t kDataShift  = 28u - (uint32_t)kNumBits_;
			constexpr uint32_t kPayloadMsk = ((uint32_t)1 << (uint32_t)kNumBits_) - 1u;

			uint32_t w;
			__builtin_memcpy(&w, tdo_bytes, sizeof(w));
			w = __REV(w);
			return (w >> kDataShift) & kPayloadMsk;
		}
		else
		{
			// kNumBits_ == 32: same straddled layout as RenderTransaction.
			uint32_t w_hi;
			__builtin_memcpy(&w_hi, tdo_bytes, sizeof(w_hi));
			w_hi = __REV(w_hi);
			// W_hi bits 27..0 = payload[31..4]; byte 4 top nibble = payload[3..0].
			return ((w_hi & 0x0FFFFFFFu) << 4) | (uint32_t)(tdo_bytes[4] >> 4);
		}
	}
};

} // namespace DtrigJtag_ns

// Pull the template into the WaveJtag namespace for uniform usage
namespace WaveJtag
{
	using DtrigJtag_ns::DtrigJtag;
}
