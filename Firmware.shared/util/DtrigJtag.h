/*
# DtrigJtag — Double-Trigger SPI+TIM1 JTAG Frame Generator (PWM-TMS edition)

## Concept
Synchronises SPI1 and TIM1 to act as a combined JTAG driver for hardware where the
JTAG signals are split across GPIO ports (e.g. ST-Link V2 clone):

  - SPI1 SCK  (PA5) = JTCK    — SPI clock, drives JTCK (PA5 and PB13 are wired together on PCB)
  - SPI1 MOSI (PA7) = JTDI    — shift register output, byte-level DMA
  - SPI1 MISO (PA6) = JTDO    — shift register input, byte-level DMA
  - TIM1 CH2N (PB14)= JTMS    — toggle output driven by hardware, no per-bit DMA

## TMS Generation — Toggle Mode + Two CCR2 Reloads

TIM1 runs as a single long shot covering the full frame (kTotalClocks × 8 timer ticks,
ARR = kTotalClocks × 8 − 1).  TIM1_CH2N on PB14 is configured in Toggle mode:

  CH2 (OCREF) starts LOW  →  CH2N (TMS) starts HIGH  (entry pulse active)

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

## Synchronisation
SPI baud = APB2 / N, TIM1 period = N APB2 cycles (both 8-count per JTCK cycle).
A critical section launches TIM1 and SPI TX DMA simultaneously.  cnt_offset pre-loads
TIM1 CNT so that TMS toggle events land at the correct phase within each SPI bit cycle.

## PB14 pin mode
  Start() : switches PB14 to TIM1_CH2N AF mode (JTMS_PWM::SetupPinMode)
  Wait()  : PB14 stays in AF, TMS remains LOW (RTI) between frames
  Bit-bang: caller must call JTMS::SetHigh/Low() + JTMS::SetupPinMode() before GPIO use
*/

#pragma once

#include "JtagFrame.h"


namespace DtrigJtag_ns
{


/**
 * DtrigJtag — synchronised SPI + TIM1 JTAG generator with hardware TMS.
 *
 * TIM1_CH2N (PB14 = JTMS) is driven in Toggle mode for zero-DMA TMS generation.
 * Two auxiliary compare channels (kTmsRld1, kTmsRld2) trigger single-element DMA
 * writes that reload CCR2 at the entry→shift and exit→RTI transition points.
 *
 * @tparam SysClk     System clock type (provides APB2 frequency)
 * @tparam kTim       TIM1 unit (advanced timer with complementary outputs)
 * @tparam kTms       TIM1 CH2 — toggle output, CH2N drives JTMS pin (PB14)
 * @tparam kTmsRld1   TIM1 CH3 — compare-only, DMA trigger at end of entry pulse
 * @tparam SpiDevice  SPI peripheral type (DmaChInfoTx_, DmaChInfoRx_)
 * @tparam kFreq      JTAG clock frequency; must equal SpiDevice baud rate
 * @tparam kScan      DR, IR, or GoIdle
 * @tparam kNumBits   Payload width (8 / 16 / 20 / 32, or GoIdle sentinel)
 */
template <
	typename SysClk
	, const Timer::Unit kTim
	, const Timer::Channel kTms		///< CH2: toggle output → CH2N → PB14 (JTMS)
	, const Timer::Channel kTmsRld1	///< CH3: DMA trigger, reloads CCR2 at entry end
	, typename SpiDevice
	, const uint32_t kFreq
	, JtagFrame::Scan kScan
	, JtagFrame::NumBits kNumBits
>
class DtrigJtag
{
public:
	// ── Timer ────────────────────────────────────────────────────────────────
	// Timer runs a factor faster than SPI bit rate, so we have room to trim latencies
	static constexpr uint16_t kTimerMultiplier_ = 8;
	/// TIM1 input clock = 8 × kFreq: one 8-count period = one JTCK cycle
	using MasterClock = Timer::InternalClock_Hz<kTim, SysClk, kTimerMultiplier_ * kFreq>;
	/// Period is falling edge to falling edge of TMS fix. But we have to adjust initial CNT for first TMS pulse.
	static constexpr uint16_t kTimerPeriod_ 
		= (kScan == JtagFrame::Scan::kGoIdle)
		? 8 * kTimerMultiplier_
		: (3 + (uint16_t)kNumBits) * kTimerMultiplier_
		;
	/// Single-shot; ARR=kTimerPeriod_ sets the prescaler in Init().  Start() overrides ARR per frame.
	using CycleTimer = Timer::Any<MasterClock, Timer::Mode::kSingleShot, kTimerPeriod_, false, true>;

	/// CH2 in Toggle mode, CH2N output enabled → PB14 (JTMS).
	/// OCREF starts LOW (via Force Inactive before each frame), so CH2N starts HIGH (TMS=1).
	using TmsCh2N = Timer::AnyOutputChannel
		<
		CycleTimer
		, kTms						///< Channel::k2
		, Timer::OutMode::kSetActive
		, Timer::Output::kDisabled	///< CH2 main output not used
		, Timer::Output::kEnabled	///< CH2N drives PB14
		, false						///< no CCR2 preload
		, false						///< no fast enable
		>;

	/// CH3: compare-only (Frozen mode, no pin), generates CC3 DMA request at entry end
	using TriggerRld1 = Timer::AnyOutputChannel
		<
		CycleTimer
		, kTmsRld1					///< Channel::k3 → DMA1_CH6
		, Timer::OutMode::kFrozen
		, Timer::Output::kDisabled
		, Timer::Output::kDisabled
		>;

	// ── DMA — CCR2 reloads ────────────────────────────────────────────────────
	/// Triggered by CC3 at count kEntry×8; writes kCcr2ExitStart_ → CCR2
	using DmaCcr2Rld1 = Dma::AnyChannel
		<
		typename TriggerRld1::DmaChInfo_
		, Dma::Dir::kMemToPer
		, Dma::PtrPolicy::kLongPtr		///< source: single uint32_t in flash
		, Dma::PtrPolicy::kLongPtr		///< dest: CCR2 register (fixed)
		, Dma::Prio::kHigh
		>;

	// ── DMA — SPI TX/RX ──────────────────────────────────────────────────────
	/// Feeds SPI DR with JTDI bytes (one DMA per 8 JTCK cycles)
	using SpiTxDma = Dma::AnyChannel
		<
		typename SpiDevice::DmaChInfoTx_
		, Dma::Dir::kMemToPer
		, Dma::PtrPolicy::kBytePtrInc
		, Dma::PtrPolicy::kBytePtr
		, Dma::Prio::kHigh
		>;

	/// Drains SPI DR into the JTDO receive buffer
	using SpiRxDma = Dma::AnyChannel
		<
		typename SpiDevice::DmaChInfoRx_
		, Dma::Dir::kPerToMem
		, Dma::PtrPolicy::kBytePtr
		, Dma::PtrPolicy::kBytePtrInc
		, Dma::Prio::kVeryHigh
		>;

	// ── Bit-count constants ───────────────────────────────────────────────────
	static constexpr JtagFrame::Scan    kScan_    = kScan;
	static constexpr JtagFrame::NumBits kNumBits_ = kNumBits;

	/// Total JTCK cycles required for the selected scan type and payload
	static constexpr uint8_t kBitCount =
		(kScan == JtagFrame::Scan::kGoIdle)
		? 8
		: (5 + (uint8_t)kNumBits + (uint8_t)kScan);
	/// SPI bytes needed (whole bytes only; may include padding clocks)
	static constexpr uint8_t kSpiBytes    = (kBitCount + 7) / 8;
	/// Actual JTCK cycles clocked (= kSpiBytes × 8, includes padding)
	static constexpr uint8_t kTotalClocks = kSpiBytes * 8;

	// ── TMS timing constants (in JTCK cycles) ──────────────────────────────
	/// Cycles where TMS=1 for the entry pulse (Sel-DR, Sel-DR+Sel-IR, or test-logic-reset)
	static constexpr uint8_t kEntry =
		(kScan == JtagFrame::Scan::kIR)
		? 2
		: 1
		;

	/// At this CNT PWM trigger TMS high
	static constexpr uint32_t kTmsHigh1 =
		(kScan == JtagFrame::Scan::kGoIdle)
		? 3 * kTimerMultiplier_					// > ARR → no second toggle
		: (kScan == JtagFrame::Scan::kIR)
		? kTimerPeriod_ - 2 * kTimerMultiplier_
		: kTimerPeriod_ - 1 * kTimerMultiplier_
		;
	/// At this CNT PWM trigger TMS high
	// Note: A half clock cycle was considered so `cnt_offset` can trim on both directions
	static constexpr uint32_t kCntStart_ =
		(kScan == JtagFrame::Scan::kGoIdle)
		? 2 * kTimerMultiplier_
		: (kScan == JtagFrame::Scan::kIR)
		? kTimerPeriod_ - 3 * kTimerMultiplier_ + kTimerMultiplier_/2
		: kTimerPeriod_ - 3 * kTimerMultiplier_ + kTimerMultiplier_/2
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
			"DtrigJtag requires TIM1 (advanced timer with complementary CH2N output)");
		static_assert(DmaCcr2Rld1::kChan_ != SpiTxDma::kChan_,
			"kTmsRld1 DMA conflicts with SPI TX DMA — choose a different kTmsRld1 channel");
		static_assert(DmaCcr2Rld1::kChan_ != SpiRxDma::kChan_,
			"kTmsRld1 DMA conflicts with SPI RX DMA — choose a different kTmsRld1 channel");

		CycleTimer::Init();			// sets PSC, CR1 mode bits; ARR is set per-frame in Start()
		TmsCh2N::Setup();			// toggle mode, CH2N enabled, MOE=1
		TriggerRld1::Setup();		// CH3 frozen (no pin), DMA request enabled in Start()

		DmaCcr2Rld1::Init();
		SpiTxDma::Init();
		SpiRxDma::Setup();

		SpiDevice::Init();
		SpiDevice::EnableDma();		// must follow Init(): Init() writes CR2 and clears TXDMAEN/RXDMAEN
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
		// Since TMS timer aligns to the falling edge, DR frames have to get one more bit
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

		// Use the default (slower) prescaler for safe JTAG reset timing
		//CycleTimer::SetPrescaler(CycleTimer::kPrescaler_);
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
		// CCR2: first toggle at end of entry pulse
		TmsCh2N::SetCompare((uint16_t)kTmsHigh1);
		if (kScan == JtagFrame::Scan::kDR)
		{
			// CC3 fires at the same time as CCR2 → DMA reloads CCR2 = kExitStart*8
			TriggerRld1::SetCompare((uint16_t)kTmsHigh1);
		}

		// Switch PB14 to TIM1_CH2N AF (TMS immediately HIGH; idempotent if already AF)
		//JTMS_PWM::SetupPinMode();
		// Switch to Toggle mode (OCREF retained at 0; toggles on CCR2 match)
		//TmsCh2N::SetOutputMode(Timer::OutMode::kPWM1);

		if (kScan == JtagFrame::Scan::kGoIdle)
			CycleTimer::SetupRepetition(kTimerPeriod_, 1);
		else
			CycleTimer::SetupRepetition(kTimerPeriod_, 2);
		CycleTimer::ClearStatus();
		// Force CH2 inactive (OCREF=0 → CH2N=HIGH → TMS=1 for entry pulse)
		TmsCh2N::SetOutputMode(Timer::OutMode::kPWM2);

		// Arm CCR2-reload DMA channels (2 pulse width)
		tmsHigh2 = kTimerPeriod_ - 2 * kTimerMultiplier_;	// Frame exit in last two bits
		if (kScan == JtagFrame::Scan::kDR)
		{
			DmaCcr2Rld1::Start(&tmsHigh2, TmsCh2N::GetCcrAddress(), 1);
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
			SpiTxDma::Enable();				// triggers first SPI byte → SPI starts
			CycleTimer::CounterResume();	// TIM1 begins; TMS=HIGH until count kEntry*8
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
		uint8_t  bit  = (uint8_t)kEntry + 2;
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
