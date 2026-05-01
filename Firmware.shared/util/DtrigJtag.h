/*
# DtrigJtag — Double-Trigger SPI+TIM1 JTAG Frame Generator

## Concept
Synchronises SPI1 and TIM1 to act as a combined JTAG driver for hardware where the
JTAG signals are split across GPIO ports (e.g. ST-Link V2 clone):

  - SPI1 SCK  (PA5) = JTCK    — SPI clock, directly drives JTCK (PA5 and PB13 are wired together on PCB)
  - SPI1 MOSI (PA7) = JTDI    — shift register output, byte-level DMA
  - SPI1 MISO (PA6) = JTDO    — shift register input,  byte-level DMA
  - TIM1 CH? DMA → PB14       = JTMS   — per-bit BSRR writes from a pre-rendered buffer

TIM1 is used exclusively for per-bit TMS DMA triggering.  It does NOT generate JTCK;
the SPI peripheral's own SCK output is the JTAG clock.

## Synchronisation
Both SPI and TIM1 derive from APB2 (72 MHz on STM32F103).  SPI baud = APB2 / N, TIM1
period = N APB2 cycles.  Once started together they are phase-locked and never drift.

A short critical section starts TIM1 (CNT pre-loaded to kCntOffset) and enables SPI TX
DMA in rapid succession.  kCntOffset shifts the phase relationship so that TIM1's
first TMS DMA event coincides with SPI having shifted out the correct bit.  The exact
value for each speed grade is tuned once with a logic analyser.

## Signal timing (8-count cycle, SPI SCK rises at reload)
  Count:  0   1   2   3   4   5   6   7   (reload → 0)
  JTCK    ────────────────────────┐           ┌─── …   (SPI SCK, falls at reload/2 for CPOL=1)
                                  │           │
                                  └───────────┘
  TMS DMA                              ↑ fires at count 6 (2 counts before next SPI rising edge)
  SPI MOSI  stable before rising edge, sampled by target at rising edge

## DMA budget vs GeneratorSTLinkPWM
  GeneratorSTLinkPWM: 3 DMA channels per JTCK cycle (TMS, JTDI, JTDO)
  DtrigJtag:          1 DMA per JTCK cycle (TMS only) + 1 DMA per 8 cycles (SPI byte)
  → significantly lower AHB bus pressure, enabling higher sustainable clock rates.
*/

#pragma once

#include "JtagFrame.h"


namespace DtrigJtag_ns
{


/**
 * DtrigJtag — synchronised SPI + TIM1 JTAG generator.
 *
 * SPI1 SCK (PA5) drives JTCK directly (PA5 is wired to PB13 on the STLinkV2 PCB).
 * TIM1 is used only to trigger per-bit TMS DMA transfers.
 *
 * @tparam SysClk       System clock type (provides APB2 frequency)
 * @tparam kTim         TIM1 unit (must be advanced timer with repetition counter)
 * @tparam kTms         TIM1 channel whose compare event triggers TMS DMA
 * @tparam SpiDevice    SPI peripheral type (exposes DmaInstance_, DmaTxCh_, DmaRxCh_)
 * @tparam kFreq        JTAG clock frequency; must equal SpiDevice's baud rate
 * @tparam kScan        DR, IR, or GoIdle
 * @tparam kNumBits     Payload width (8 / 16 / 20 / 32 bits, or GoIdle sentinel)
 */
template <
	typename SysClk
	, const Timer::Unit kTim
	, const Timer::Channel kTms
	, typename SpiDevice
	, const uint32_t kFreq
	, JtagFrame::Scan kScan
	, JtagFrame::NumBits kNumBits
>
class DtrigJtag
{
public:
	// ── Timer ────────────────────────────────────────────────────────────────
	/// TIM1 input clock = 8 × kFreq so that one period (ARR=7) = one JTCK cycle
	using MasterClock = Timer::InternalClock_Hz<kTim, SysClk, 8 * kFreq>;
	/// Single-shot, ARR=7 (8 counts), repetition counter controls bit count
	using CycleTimer = Timer::Any<MasterClock, Timer::Mode::kSingleShot, 7, false, true>;

	/// Compare event at count 6 triggers one TMS DMA transfer per JTCK cycle
	using TriggerTms = Timer::AnyOutputChannel<CycleTimer, kTms>;

	// ── DMA — TMS ────────────────────────────────────────────────────────────
	/// Writes one uint32_t BSRR value per JTCK cycle to JTMS GPIO port
	using DmaTms = Dma::AnyChannel
		<
		typename TriggerTms::DmaChInfo_
		, Dma::Dir::kMemToPer
		, Dma::PtrPolicy::kLongPtrInc	///< Advances through tms_buf[]
		, Dma::PtrPolicy::kLongPtr		///< BSRR is a fixed peripheral address
		, Dma::Prio::kMedium
		>;

	// ── DMA — SPI TX/RX ──────────────────────────────────────────────────────
	/// Feeds SPI DR with JTDI bytes; fires once per byte (every 8 JTCK cycles)
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
	static constexpr uint8_t kSpiBytes = (kBitCount + 7) / 8;
	/// Actual JTCK cycles clocked (= kSpiBytes × 8, includes padding)
	static constexpr uint8_t kTotalClocks = kSpiBytes * 8;

	static_assert(kTotalClocks <= 40,
		"Transaction too large for JtagDev::read_buf_ (40 words). Increase kPingPongBufSize_.");
	static_assert(kSpiBytes <= 8,
		"Transaction too large for the SPI tx/rx buffers (8 bytes).");

	// ─────────────────────────────────────────────────────────────────────────
	// Init / DMA lifecycle
	// ─────────────────────────────────────────────────────────────────────────

	/// One-time hardware initialisation for this speed grade.
	/// Must be called in OnOpen(); sets TIM1 prescaler and SPI baud rate.
	static ALWAYS_INLINE void Init()
	{
		static_assert(CycleTimer::HasRepetitionCounter(),
			"DtrigJtag requires an advanced timer (TIM1) with a repetition counter");
		static_assert(DmaTms::kDma_ != SpiTxDma::kDma_ || DmaTms::kChan_ != SpiTxDma::kChan_,
			"TMS DMA channel conflicts with SPI TX DMA channel — choose a different kTms");
		static_assert(DmaTms::kDma_ != SpiRxDma::kDma_ || DmaTms::kChan_ != SpiRxDma::kChan_,
			"TMS DMA channel conflicts with SPI RX DMA channel — choose a different kTms");

		CycleTimer::Init();
		TriggerTms::Setup();
		TriggerTms::SetCompare(1);		// TMS DMA 2 counts before SPI rising edge

		DmaTms::Init();
		SpiTxDma::Init();
		SpiRxDma::Setup();

		SpiDevice::Init();
		SpiDevice::EnableDma();		// must follow Init(): Init() writes CR2 and would clear TXDMAEN/RXDMAEN
	}

	/// Re-arms DMA channels after they were released for repurposing (e.g. JtclkWaveGen)
	static ALWAYS_INLINE void SetupDma()
	{
		DmaTms::Setup();
		SpiTxDma::Setup();
		SpiRxDma::Setup();
	}

	/// Releases DMA channels so other peripherals (e.g. JtclkWaveGen) may use them
	static ALWAYS_INLINE void ReleaseDma()
	{
		DmaTms::Disable();
		SpiTxDma::Disable();
		SpiRxDma::Disable();
	}

	// ─────────────────────────────────────────────────────────────────────────
	// Buffer rendering
	// ─────────────────────────────────────────────────────────────────────────

	/**
	 * Fills the SPI TX byte buffer and the TMS BSRR buffer for one JTAG scan.
	 *
	 * SPI byte layout: MSB-first, each bit corresponds to one JTCK cycle.
	 *   Bit position i  →  byte[i/8] bit (7 - i%8)
	 *
	 * TMS buffer: one uint32_t BSRR value per JTCK cycle (kTotalClocks entries).
	 *
	 * @param tdi_bytes  Output: kSpiBytes bytes for SPI TX
	 * @param tms_buf    Output: kTotalClocks uint32_t values for TMS GPIO BSRR
	 * @param tclk_high  Current JTDI/TCLK level (maintained during Run-Test/Idle bits)
	 * @param data_out   Payload to shift; MSB is sent first
	 */
	static ALWAYS_INLINE void RenderTransaction(
		uint8_t* tdi_bytes
		, uint32_t* tms_buf
		, bool tclk_high
		, uint32_t data_out
	)
	{
		static_assert(kScan_ != JtagFrame::Scan::kGoIdle,
			"Use DoGoIdle() for the GoIdle sequence");

		constexpr uint32_t tms_set = JTMS::kBitValue_;
		constexpr uint32_t tms_rst = JTMS::kBitValue_ << 16;

		// Initialise all TDI bits to tclk level, all TMS to reset (TMS=0)
		__builtin_memset(tdi_bytes, tclk_high ? 0xFF : 0x00, kSpiBytes);
		for (uint8_t i = 0; i < kTotalClocks; ++i)
			tms_buf[i] = tms_rst;

		uint8_t bit = 0;

		// Select-DR (TMS=1, TDI=tclk already set)
		tms_buf[bit++] = tms_set;

		if (kScan_ == JtagFrame::Scan::kIR)
		{
			// Select-IR (TMS=1, TDI=tclk)
			tms_buf[bit++] = tms_set;
		}

		// Capture ×2 (TMS=0, TDI=tclk) — both already initialised
		bit += 2;

		// Shift phase: override TDI for actual data bits
		uint32_t mask = 1U << ((uint8_t)kNumBits_ - 1);
		for (; mask > 1; mask >>= 1)
		{
			SetTdiBit(tdi_bytes, bit, (data_out & mask) != 0);
			// tms_buf[bit] already tms_rst
			++bit;
		}

		// Exit1 — last data bit with TMS=1
		tms_buf[bit] = tms_set;
		SetTdiBit(tdi_bytes, bit, (data_out & mask) != 0);
		++bit;

		// Update (TMS=1, TDI=tclk)
		tms_buf[bit++] = tms_set;

		// Run-Test/Idle + padding (TMS=0, TDI=tclk) — already initialised
	}

	/**
	 * Fills buffers for the GoIdle (TAP reset) sequence: 6× TMS=1 then 1× TMS=0.
	 * Uses a slower prescaler (CycleTimer::kPrescaler_) for safe TAP reset timing.
	 */
	static ALWAYS_INLINE void DoGoIdle(
		uint8_t* tdi_bytes
		, uint32_t* tms_buf
		, uint16_t cnt_offset
	)
	{
		static_assert(kScan_ == JtagFrame::Scan::kGoIdle,
			"DoGoIdle() requires a kGoIdle instantiation");

		constexpr uint32_t tms_set = JTMS::kBitValue_;
		constexpr uint32_t tms_rst = JTMS::kBitValue_ << 16;

		// TDI=0 throughout (TDO not relevant during reset)
		__builtin_memset(tdi_bytes, 0x00, kSpiBytes);

		// 6 clocks with TMS=1 to guarantee TAP reaches Test-Logic-Reset from any state,
		// followed by 1 clock with TMS=0 to enter Run-Test/Idle, then 1 padding clock.
		for (uint8_t i = 0; i < 6; ++i)
			tms_buf[i] = tms_set;
		tms_buf[6] = tms_rst;
		tms_buf[7] = tms_rst;

		// Use the default (slower) prescaler to ensure safe JTAG reset timing
		CycleTimer::SetPrescaler(CycleTimer::kPrescaler_);
		Start(tdi_bytes, nullptr, tms_buf, cnt_offset);
		Wait();
	}

	// ─────────────────────────────────────────────────────────────────────────
	// Execution
	// ─────────────────────────────────────────────────────────────────────────

	/**
	 * Starts a pre-rendered transaction.  TIM1 and SPI are launched inside a
	 * critical section so their clocks are phase-locked from the very first bit.
	 *
	 * SPI1 SCK provides JTCK — no pin-mode switching needed here.
	 *
	 * @param tdi_bytes   kSpiBytes of SPI TX data (from RenderTransaction)
	 * @param tdo_bytes   kSpiBytes buffer for SPI RX data (may be nullptr for GoIdle)
	 * @param tms_buf     kTotalClocks TMS BSRR values (from RenderTransaction)
	 * @param cnt_offset  TIM1 CNT preset: tunes TMS phase vs SPI bit edges (LA-adjusted)
	 */
	static OPTIMIZED void Start(
		uint8_t* tdi_bytes
		, uint8_t* tdo_bytes
		, uint32_t* tms_buf
		, uint16_t cnt_offset
	)
	{
		// Set up DMA transfer sizes
		CycleTimer::ClearStatus();
		CycleTimer::SetupRepetition(kTotalClocks);
		CycleTimer::SetPrescaler(CycleTimer::kPrescaler_);
		DmaTms::SetTransferCount(kTotalClocks);
		SpiRxDma::SetTransferCount(kSpiBytes);
		SpiTxDma::SetTransferCount(kSpiBytes);

		// Point DMA at the caller-supplied buffers
		DmaTms::SetSourceAddress(tms_buf);
		DmaTms::SetDestAddress(&JTMS::Io().BSRR);
		SpiTxDma::SetSourceAddress(tdi_bytes);
		SpiTxDma::SetDestAddress(&SpiDevice::GetDevice()->DR);

		TriggerTms::EnableDma();

		// Arm receive DMA before the critical section
		if (tdo_bytes)
		{
			SpiRxDma::SetDestAddress(tdo_bytes);
			SpiRxDma::SetSourceAddress(&SpiDevice::GetDevice()->DR);
			SpiRxDma::Enable();
		}

		DmaTms::Enable();			// arm TMS DMA (first write at count 6)
		TIM1->CNT = cnt_offset;		// preset phase

		// ── Critical section: start TIM1 and SPI TX DMA together ─────────────
		// Both run at APB2/N and will stay phase-locked once started.
		// cnt_offset shifts TIM1 phase so the first TMS DMA fires at the right
		// moment relative to SPI MOSI.  Tune per speed grade with a logic analyser.
		{
			CriticalSection lock;
			SpiTxDma::Enable();				// triggers first SPI byte transfer → SPI starts
			CycleTimer::CounterResume();	// start TIM1 → TMS DMA trigger generation begins
		}
	}

	/// Waits for TIM1 to complete all kTotalClocks cycles then cleans up DMA.
	static ALWAYS_INLINE void Wait()
	{
		CycleTimer::WaitForAutoStop();
		// SPI may still be clocking the last byte; wait for BSY to clear
		while (SpiDevice::GetDevice()->SR & SPI_SR_BSY)
			;
		TriggerTms::DisableDma();
		DmaTms::Disable();
		SpiTxDma::Disable();
		SpiRxDma::Disable();
		// SPI SCK (= JTCK) is now idle; SPI peripheral keeps it in the CPOL=1 idle state.
	}

	// ─────────────────────────────────────────────────────────────────────────
	// Result decoding
	// ─────────────────────────────────────────────────────────────────────────

	/**
	 * Extracts the received payload from the SPI RX byte buffer.
	 * Data bits start at position (3 + kScan_) in the bit stream (MSB first).
	 *
	 * @param tdo_bytes  SPI RX buffer filled by Start()/Wait()
	 * @return           Received payload, MSB aligned to bit (kNumBits-1)
	 */
	static ALWAYS_INLINE uint32_t GetResult(const uint8_t* tdo_bytes)
	{
		uint8_t  bit  = 3 + (uint8_t)kScan_;
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
