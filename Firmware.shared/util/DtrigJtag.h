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
A critical section in Start() launches TIM1::CounterResume() and SpiTxDma::Enable()
atomically so their first edges are synchronized. The cnt_offset parameter (calibrated
per speed grade with a logic analyzer) pre-loads TIM1 CNT to align TMS toggle events
within each SPI bit cell, compensating for latency differences between the DMA engines.

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
		,
		kCmpComplementary ? Timer::OutMode::kForceInactive : Timer::OutMode::kForceActive
		,
		kCmpComplementary ? Timer::Output::kDisabled : Timer::Output::kInverted		///< main CH
		,
		kCmpComplementary ? Timer::Output::kEnabled  : Timer::Output::kDisabled		///< complementary CHN
		,
		false ///< no CCR preload
		,
		false ///< no fast enable
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
		,
		Timer::OutMode::kFrozen, Timer::Output::kDisabled, Timer::Output::kDisabled>;

	// ── DMA — CCR2 reloads ────────────────────────────────────────────────────
	/// Triggered by CC3 at count kEntry×8; writes kCcr2ExitStart_ → CCR2
	using DmaCcr2Rld1 = Dma::AnyChannel<
		typename TriggerRld1::DmaChInfo_, Dma::Dir::kMemToPer, Dma::PtrPolicy::kLongPtr ///< source: single uint32_t in flash
		,
		Dma::PtrPolicy::kLongPtr ///< dest: CCR2 register (fixed)
		,
		Dma::Prio::kHigh>;

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

	/// At this CNT PWM trigger TMS high
	static constexpr uint32_t kTmsHigh1 =
		(kScan == JtagFrame::Scan::kGoIdle)
			? 3 * kTimerMultiplier_ // > ARR → no second toggle
		: (kScan == JtagFrame::Scan::kIR)
			? kTimerPeriod_ - 2 * kTimerMultiplier_
			: kTimerPeriod_ - 1 * kTimerMultiplier_;
	/// At this CNT PWM trigger TMS high
	// Note: A half clock cycle was considered so `cnt_offset` can trim on both directions
	static constexpr uint32_t kCntStart_ =
		(kScan == JtagFrame::Scan::kGoIdle)
			? 2 * kTimerMultiplier_
		: (kScan == JtagFrame::Scan::kIR)
			? kTimerPeriod_ - 3 * kTimerMultiplier_ + kTimerMultiplier_ / 2
			: kTimerPeriod_ - 3 * kTimerMultiplier_ + kTimerMultiplier_ / 2;

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
	 * @param tdi_bytes  Output: kSpiBytes bytes for SPI TX
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

		// Initialise all TDI bits to tclk level (maintained through idle, capture, update, RTI)
		__builtin_memset(tdi_bytes, tclk_high ? 0xFF : 0x00, kSpiBytes);

		// Skip entry bits (Sel-DR [+ Sel-IR]) and Capture×2 — TDI = tclk throughout
		// Since TMS timer aligns to the falling edge, DR frames gets a dummy run/idle bit before payload
		uint8_t bit = (uint8_t)kEntry + 2 + (kScan_ == JtagFrame::Scan::kDR);

		// Shift phase: override TDI for actual payload bits (kNumBits - 1 bits in Shift state)
		uint32_t mask = 1U << ((uint8_t)kNumBits_ - 1);
		for (; mask > 1; mask >>= 1)
		{
			SetTdiBit(tdi_bytes, bit, (data_out & mask) != 0);
			++bit;
		}

		// Exit1 — last data bit (TMS=1 is handled by hardware)
		SetTdiBit(tdi_bytes, bit, (data_out & mask) != 0);
		// Update and RTI/padding: TDI remains at tclk level (already initialised)
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

		// TDI=0 throughout (TDO not relevant during reset)
		__builtin_memset(tdi_bytes, 0x00, kSpiBytes);

		CycleTimer::SetPrescaler(CycleTimer::kPrescaler_); // force slowest PSC + SPI BAUD (template is bound to JTCK_Speed_1)
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
	 * @param cnt_offset  TIM1 CNT preset: tunes TMS phase vs SPI bit edges (LA-adjusted)
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

		// CCR2: first toggle at end of entry pulse
		TmsOut::SetCompare((uint16_t)kTmsHigh1);
		if (kScan == JtagFrame::Scan::kDR)
		{
			// CC3 fires at the same time as CCR2 → DMA reloads CCR2 = kExitStart*8
			TriggerRld1::SetCompare((uint16_t)kTmsHigh1);
		}

		if (kScan == JtagFrame::Scan::kGoIdle)
			CycleTimer::SetupRepetition(kTimerPeriod_, 1);
		else
			CycleTimer::SetupRepetition(kTimerPeriod_, 2);
		CycleTimer::ClearStatus();
		// Engage the per-frame PWM mode: PWM2 for CHN, PWM1 for CH. Both yield
		// pin=HIGH while CNT<CCR (entry pulse) and pin=LOW once CNT crosses CCR.
		// See kFramePwmMode_ comment for derivation.
		TmsOut::SetOutputMode(kFramePwmMode_);

		// Arm CCR2-reload DMA channels (2 pulse width)
		tmsHigh2 = kTimerPeriod_ - 2 * kTimerMultiplier_;	// Frame exit in last two bits
		if (kScan == JtagFrame::Scan::kDR)
		{
			DmaCcr2Rld1::Start(&tmsHigh2, TmsOut::GetCcrAddress(), 1);
			TriggerRld1::EnableDma();
		}

		// Arm SPI DMA
		SpiRxDma::SetTransferCount(kSpiBytes);
		SpiTxDma::SetTransferCount(kSpiBytes);
		SpiTxDma::SetSourceAddress(tdi_bytes);
		SpiTxDma::SetDestAddress(&SpiDevice::GetDevice()->DR);

		if (tdo_bytes)
		{
			SpiRxDma::SetDestAddress(tdo_bytes);
			SpiRxDma::SetSourceAddress(&SpiDevice::GetDevice()->DR);
			SpiRxDma::Enable();
		}

		CycleTimer::SetCounter(kCntStart_ + cnt_offset);

		// ── Critical section: start TIM1 and SPI TX DMA together ─────────────
		{
			CriticalSection lock;
			SpiTxDma::EnableFast();				// triggers first SPI byte → SPI starts
			CycleTimer::CounterResumeFast();	// TIM1 begins; TMS=HIGH until count kEntry*8
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
	 * Data bits start at position (kEntry + 2) in the bit stream (MSB first).
	 *
	 * @param tdo_bytes  SPI RX buffer filled by Start()/Wait()
	 * @return           Received payload, MSB aligned to bit (kNumBits-1)
	 */
	static ALWAYS_INLINE uint32_t GetResult(const uint8_t* tdo_bytes)
	{
		// Skip entry bits (Sel-DR [+ Sel-IR]) and Capture×2 — TDI = tclk throughout
		// Since TMS timer aligns to the falling edge, DR frames gets a dummy run/idle bit before payload
		uint8_t bit = (uint8_t)kEntry + 2 + (kScan_ == JtagFrame::Scan::kDR);
		uint32_t mask = 1U << ((uint8_t)kNumBits_ - 1);
		uint32_t result = 0;
		for (uint32_t m = mask; m != 0; m >>= 1, ++bit)
		{
			if (tdo_bytes[bit >> 3] & (uint8_t)(0x80U >> (bit & 7)))
				result |= m;
		}
		return result;
	}

private:
	/// Sets or clears one TDI bit at stream position idx in the SPI byte buffer.
	static ALWAYS_INLINE void SetTdiBit(uint8_t* bytes, uint8_t idx, bool val)
	{
		uint8_t& byte = bytes[idx >> 3];
		uint8_t  mask = (uint8_t)(0x80U >> (idx & 7));
		if (val)
			byte |= mask;
		else
			byte &= ~mask;
	}
};

} // namespace DtrigJtag_ns

// Pull the template into the WaveJtag namespace for uniform usage
namespace WaveJtag
{
	using DtrigJtag_ns::DtrigJtag;
}
