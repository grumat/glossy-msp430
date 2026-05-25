/*
# TimSbw — Spy-Bi-Wire Transport (Timer+DMA encoder)

Naming: this is the **timdma** transport model — the scan is driven by a TIM1
single-shot whose compare channels fan out DMA requests; there is no software
"dual trigger" critical section coordinating two competing peripherals (that is
the defining trait of the *dtrig* model used by DtrigJtag). A future SBW *dtrig*
variant is sketched in .claude/docs/drivers/DTRIG_SBW_SPI_ALT.md.

Companion to DtrigJtag. Design doc: .claude/docs/drivers/TIM_SBW_DRIVER.md.

## SBW frame model

Each logical JTAG bit becomes a 3-phase wire frame, clocked by SBWCLK rising
edges:

  Cycle 3k+0 : drive SBWDIO with TMS_k                              (output)
  Cycle 3k+1 : drive SBWDIO with TDI_k                              (output)
  Cycle 3k+2 : release SBWDIO via SBWO mux, sample TDO_k near falling edge

So `kJtagBits` logical JTAG bits expand to `3 × kJtagBits` SBWCLK cycles.

## Buffered-board fast path (Option 1 / strategy B)

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
 * TimSbw — Spy-Bi-Wire frame generator (Timer+DMA model).
 *
 * Buffered-board fast path: data + direction folded into one composite BSRR
 * DMA. For STLinkV2 (un-buffered single-pin bit-band path) a separate
 * variant or template specialisation is required and not implemented here;
 * see TIM_SBW_DRIVER.md.
 *
 * @tparam SysClk         System clock type (provides APB2 frequency)
 * @tparam kTim           Advanced timer unit (TIM1 on F1xx — needs RCR)
 * @tparam kSbwClk        Timer channel driving SBWCLK PWM (CH or CHN)
 * @tparam kSbwDataTrig   Compare-only channel triggering composite BSRR DMA
 * @tparam kSbwSampleTrig Compare-only channel triggering IDR sample DMA
 * @tparam SbwDioOut      Bmt::Gpio output pin alias for SBWDIO data drive
 *                        (must expose `static constexpr uint8_t kPin_`)
 * @tparam SbwIdrPort     Pointer-returning helper: `static GPIO_TypeDef*
 *                        Get() { return GPIOA; }` — the GPIO holding the
 *                        SBWDIO_In sample bit
 * @tparam SbwIdrBit      Bit position of SBWDIO_In inside that GPIO->IDR
 * @tparam DirPolicy      See TIM_SBW_DRIVER.md "DirPolicy contract".
 *                        On the buffered fast path only `DriveOutput()[0]`
 *                        and `DriveInput()[0]` are consumed.
 * @tparam kFreq          SBWCLK frequency in Hz (5 MHz MSP430 ceiling)
 * @tparam kScan          DR, IR, or GoIdle (same enum as DtrigJtag)
 * @tparam kNumBits       Payload width (8 / 16 / 20 / 32, or GoIdle sentinel)
 * @tparam kCmpComplementary  true → SBWCLK on CHN; false → on regular CH
 * @tparam kSeparateDirDma  false (buffered/mux boards): the direction bit is
 *                        folded into the data BSRR words and DirPolicy yields
 *                        BSRR set/reset words. true (single-pin boards, e.g.
 *                        STLinkV2 PB14): direction is a *separate* DMA that
 *                        writes a full mode register (e.g. GPIOB->CRH) once per
 *                        cycle from a data-independent static script; DirPolicy
 *                        then yields the full "drive output" / "release input"
 *                        register words and DirRegister() = &GPIOx->CRH.
 * @tparam kSbwDirTrig    Compare-only channel that triggers the direction DMA.
 *                        Only used when kSeparateDirDma; must map to a DMA
 *                        channel distinct from the data and sample DMAs.
 */
template <
	typename SysClk
	, const Bmt::Timer::Unit kTim
	, const Bmt::Timer::Channel kSbwClk
	, const Bmt::Timer::Channel kSbwDataTrig
	, const Bmt::Timer::Channel kSbwSampleTrig
	, typename SbwDioOut
	, typename SbwIdrPort
	, const uint8_t kSbwIdrBit
	, typename DirPolicy
	, const uint32_t kFreq
	, JtagFrame::Scan kScan
	, JtagFrame::NumBits kNumBits
	, const bool kCmpComplementary = true
	, const bool kSeparateDirDma = false
	, const Bmt::Timer::Channel kSbwDirTrig = Bmt::Timer::Channel::k3
>
class TimSbw
{
public:
	// ── Bit-count constants ──────────────────────────────────────────────────
	static constexpr JtagFrame::Scan    kScan_    = kScan;
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
		(kScan == JtagFrame::Scan::kGoIdle) ? 6 :
		(kScan == JtagFrame::Scan::kIR)     ? 2 : 1;

	/// Index of the first payload bit and the bit beyond the payload.
	static constexpr uint8_t kFirstPayloadBit = kEntry;
	static constexpr uint8_t kPastPayloadBit  = kEntry + (uint8_t)kNumBits;

	// ── Timer ────────────────────────────────────────────────────────────────
	static constexpr uint16_t kTimerMultiplier_ = 8;
	using MasterClock = Bmt::Timer::InternalClock_Hz<kTim, SysClk, kTimerMultiplier_ * kFreq>;
	static constexpr uint16_t kTimerPeriod_ = kTimerMultiplier_;
	using CycleTimer = Bmt::Timer::Any<MasterClock, Bmt::Timer::Mode::kSingleShot, kTimerPeriod_, true, true>;

	// SBWCLK PWM channel (50% duty, CHN with PWM2 inverts so pin is high in
	// the first half — gives a rising edge mid-cycle that the target latches
	// MTS/TDI on).
	using SbwClkOut = Bmt::Timer::AnyOutputChannel<
		CycleTimer, kSbwClk,
		kCmpComplementary ? Bmt::Timer::OutMode::kPWM2 : Bmt::Timer::OutMode::kPWM1,
		kCmpComplementary ? Bmt::Timer::Output::kDisabled : Bmt::Timer::Output::kEnabled,
		kCmpComplementary ? Bmt::Timer::Output::kEnabled  : Bmt::Timer::Output::kDisabled,
		true,  // preload CCR (PWM)
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
	// Single-pin direction trigger (only configured when kSeparateDirDma).
	using DirTrigger = Bmt::Timer::AnyOutputChannel<
		CycleTimer, kSbwDirTrig,
		Bmt::Timer::OutMode::kFrozen,
		Bmt::Timer::Output::kDisabled, Bmt::Timer::Output::kDisabled>;

	// ── DMA ──────────────────────────────────────────────────────────────────
	/// Composite BSRR DMA: writes one uint32_t per cycle to the SBWDIO_Out
	/// port's BSRR. Source increments, destination fixed.
	using DataDma = Bmt::Dma::AnyChannel<
		typename DataTrigger::DmaChInfo_,
		Bmt::Dma::Dir::kMemToPer,
		Bmt::Dma::PtrPolicy::kLongPtrInc,	// 32-bit source, incrementing
		Bmt::Dma::PtrPolicy::kLongPtr,		// 32-bit dest, fixed
		Bmt::Dma::Prio::kHigh>;

	/// IDR sample DMA: reads GPIOx->IDR every cycle into the sample buffer.
	using SampleDma = Bmt::Dma::AnyChannel<
		typename SampleTrigger::DmaChInfo_,
		Bmt::Dma::Dir::kPerToMem,
		Bmt::Dma::PtrPolicy::kLongPtr,		// 32-bit source IDR, fixed
		Bmt::Dma::PtrPolicy::kLongPtrInc,	// 32-bit dest, incrementing
		Bmt::Dma::Prio::kVeryHigh>;

	/// Direction DMA (single-pin path only): writes one full mode-register word
	/// (e.g. GPIOB->CRH) per cycle from the static dir-script. Source
	/// increments, destination fixed — the structural twin of DataDma.
	using DirDma = Bmt::Dma::AnyChannel<
		typename DirTrigger::DmaChInfo_,
		Bmt::Dma::Dir::kMemToPer,
		Bmt::Dma::PtrPolicy::kLongPtrInc,	// 32-bit source script, incrementing
		Bmt::Dma::PtrPolicy::kLongPtr,		// 32-bit dest (CRH), fixed
		Bmt::Dma::Prio::kHigh>;

	/// Direction script: one mode-register word per SBW cycle. Data-independent
	/// (the out/out/in turnaround pattern depends only on scan geometry), so it
	/// is rendered once at Init() and reused for every frame. Only emitted when
	/// kSeparateDirDma (odr-used solely under `if constexpr`).
	static inline uint32_t s_dir_script_[kSbwCycles];

	// ── Compile-time checks ──────────────────────────────────────────────────
	static_assert(kSbwCycles <= 128,
		"SBW scan too long for ping-pong buffers — increase OPT_SBW_BUFFER_CNT_");
	static_assert(CycleTimer::HasRepetitionCounter(),
		"TimSbw requires an advanced timer with repetition counter (TIM1)");
	static_assert(DataDma::kChan_ != SampleDma::kChan_,
		"data BSRR DMA and IDR sample DMA must use distinct channels");
	static_assert(!kSeparateDirDma
		|| (DirDma::kChan_ != DataDma::kChan_ && DirDma::kChan_ != SampleDma::kChan_),
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
		DirPolicy::Init();		// no-op for mux variants
		if constexpr (kSeparateDirDma)
		{
			DirTrigger::Setup();
			DirDma::Setup();
			RenderDirScript();	// fill the static (data-independent) dir script
		}
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

	/// Render the data-independent direction script: drive output during the
	/// TMS (3i+0) and TDI (3i+1) cycles, release to input during the TDO
	/// (3i+2) cycle. The two words come from DirPolicy (full mode-register
	/// values on the single-pin path). Called once from Init().
	static void RenderDirScript()
	{
		if constexpr (kSeparateDirDma)
		{
			const uint32_t drive   = DirPolicy::DriveOutput()[0];
			const uint32_t release = DirPolicy::DriveInput()[0];
			for (uint16_t c = 0; c < kSbwCycles; ++c)
				s_dir_script_[c] = ((c % 3u) == 2u) ? release : drive;
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
	 * Payload `data_out` is shifted MSB-first into cycles [kEntry .. kEntry+kNumBits).
	 * Head and tail bits use `tclk_high` as the TDI fill value (same convention as
	 * DtrigJtag).
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
	 * @param cnt_offset   Per-grade TIM CNT trim (LA-calibrated; Issue #5)
	 */
	static OPTIMIZED void Start(
		uint32_t* bsrr_script,
		uint32_t* sample_buf,
		uint16_t cnt_offset)
	{
		// ARR = one SBWCLK period in ticks; RCR = kSbwCycles → auto-stop
		// after the full scan.
		CycleTimer::SetupRepetition(kTimerPeriod_, kSbwCycles);
		CycleTimer::ClearStatus();

		// Arm composite BSRR DMA: source = bsrr_script, dest = SBWDIO_Out port's BSRR.
		// (On buffered boards data pin and DirPolicy mux pin share the port, so
		//  the same BSRR register receives both data and direction updates.)
		DataDma::SetTransferCount(kSbwCycles);
		DataDma::SetSourceAddress(bsrr_script);
		DataDma::SetDestAddress(&reinterpret_cast<GPIO_TypeDef*>(SbwDioOut::kPortBase_)->BSRR);
		DataDma::Enable();

		// Arm IDR sample DMA: source = SBWDIO_In port's IDR, dest = sample_buf.
		SampleDma::SetTransferCount(kSbwCycles);
		SampleDma::SetSourceAddress(&SbwIdrPort::Get()->IDR);
		SampleDma::SetDestAddress(sample_buf);
		SampleDma::Enable();

		// Single-pin path: arm the direction DMA from the static dir-script.
		// dest = DirPolicy::DirRegister() (e.g. &GPIOB->CRH), source increments
		// through s_dir_script_, one full register word per cycle.
		if constexpr (kSeparateDirDma)
		{
			DirDma::SetTransferCount(kSbwCycles);
			DirDma::SetSourceAddress(s_dir_script_);
			DirDma::SetDestAddress(DirPolicy::DirRegister());
			DirDma::Enable();
		}

		// All DMAs are armed above; preset CNT for the per-grade trim and
		// release the timer. The first CC trigger cannot fire before the timer
		// runs, so no critical section is required (timdma model — see header).
		CycleTimer::SetCounter((uint16_t)(kTimerPeriod_ - cnt_offset));
		CycleTimer::CounterResumeFast();
	}

	/// Wait for the single-shot timer to auto-stop, then drain the sample DMA.
	static ALWAYS_INLINE void Wait()
	{
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
}
