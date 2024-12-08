#pragma once


namespace WaveJtag
{
		 

struct Recipe
{
	volatile GPIO_TypeDef *gpio;	// GPIO port
	uint32_t tdo_mask;				// Bit mask to read TDO value
	uint32_t tdi0;					// BSSR value for the TDI reset (target data input)
	uint32_t tdi1;					// BSSR value for the TDI set (target data input)
	uint32_t tms0tck0;				// BSSR value for TMS=0 and TCK=0
	uint32_t tms1tck0;				// BSSR value for TMS=1 and TCK=0
	uint32_t tms1tck1;				// BSSR value for TMS=1 and TCK=1
	uint32_t tck1;					// BSSR value for TCK=1
};


enum class Scan : uint8_t
{
	kDR = 0,
	kIR = 1,
	kGoIdle,				// Makes JTAG state machine exit any pending state and return to IDLE
};


enum class NumBits : uint8_t
{
	kGoIdle = 0,			// Makes JTAG state machine return to IDLE state
	k8 = 8,
	k16 = 16,
	k20 = 20,
	k32 = 32,
};


template <
	typename SysClk							///< System clock that drives timers
	, const Timer::Unit kTimMaster			///< Master timer
	, const Timer::Channel kWriteCh			///< Timer channel for the write cycle
	, const Timer::Channel kRise			///< Timer channel for the rising edge
	, const Timer::Channel kReadCh			///< Timer channel for the read cycle
	, const uint32_t kFreq					///< Frequency of the wave DMA trigger
	, const Scan kScan
	, const NumBits kNumBits
>
class Generator
{
public:
	/// Time Base for the JCLK generation (8 cycles are needed by timer for a complete cycle)
	typedef Timer::InternalClock_Hz<kTimMaster, SysClk, 8 * kFreq> MasterClock;
	/// Generates the beat that issues a DMA request
	typedef Timer::Any<MasterClock, Timer::Mode::kSingleShot, 7, false, true> CycleTimer;

	/// Timer cycle where clock asserts (and other signals are prepared)
	typedef Timer::AnyOutputChannel<CycleTimer, kWriteCh> TriggerWrite;
	/// DMA channel that writes data to BSRR
	typedef Dma::AnyChannel
		<
		typename TriggerWrite::DmaChInfo_
		, Dma::Dir::kMemToPerCircular
		, Dma::PtrPolicy::kLongPtrInc
		, Dma::PtrPolicy::kLongPtr
		, Dma::Prio::kMedium
		> DmaWrData;

	/// Timer cycle where clock rises (and target samples other signals)
	typedef Timer::AnyOutputChannel<CycleTimer, kRise> TriggerRise;
	/// DMA channel that just produces a rising edge (when all data is fetched)
	typedef Dma::AnyChannel
		<
		typename TriggerRise::DmaChInfo_
		, Dma::Dir::kMemToPerCircular
		, Dma::PtrPolicy::kLongPtr
		, Dma::PtrPolicy::kLongPtr
		, Dma::Prio::kHigh
		> DmaRise;

	/// Timer cycle after clock rises dat input is sampled
	typedef Timer::AnyOutputChannel<CycleTimer, kReadCh> TriggerRead;
	/// DMA channel that reads IDR
	typedef Dma::AnyChannel
		<
		typename TriggerRead::DmaChInfo_
		, Dma::Dir::kPerToMemCircular
		, Dma::PtrPolicy::kLongPtr
		, Dma::PtrPolicy::kLongPtrInc
		, Dma::Prio::kHigh
		> DmaRdData;
	// Type of JTAG scan
	static constexpr Scan kScan_ = kScan;
	// Bit count
	static constexpr NumBits kNumBits_ = kNumBits;

public:
	/// Hardware initialization
	static ALWAYS_INLINE void Init()
	{
		// Repetition counter is only available on the advanced timer
		static_assert(CycleTimer::HasRepetitionCounter(), "Slave timer needs to be the advanced timer (TIM1)");
		static_assert(DmaWrData::kChan_ != DmaRise::kChan_, "Selected channels are sharing the same DMA channel (HW limitation)");
		static_assert(DmaWrData::kChan_ != DmaRdData::kChan_, "Selected channels are sharing the same DMA channel (HW limitation)");
		static_assert(DmaRise::kChan_ != DmaRdData::kChan_, "Selected channels are sharing the same DMA channel (HW limitation)");

		CycleTimer::Init();				// Timer generates time base
		DmaWrData::Init();
		DmaRise::Setup();
		DmaRdData::Setup();
		TriggerWrite::Setup();
		TriggerRise::Setup();
		TriggerRead::Setup();
		TriggerWrite::SetCompare(1);
		TriggerRise::SetCompare(5);
		TriggerRead::SetCompare(7);
	}
	// Restores DMA Setup for this use (when sharing DMA with other peripherals)
	static ALWAYS_INLINE void SetupDma()
	{
		DmaWrData::Setup();
		DmaRise::Setup();
		DmaRdData::Setup();
	}
	// Restores DMA channels for repurpose, when sharing DMA with other peripherals
	static ALWAYS_INLINE void ReleaseDma()
	{
		DmaWrData::Disable();
		DmaRise::Disable();
		DmaRdData::Disable();
	}
	/// Prepares a buffer with a JTAG transaction
	static ALWAYS_INLINE void RenderTransaction(
		uint32_t *buffer
		, const Recipe &recipe
		, const uint32_t tclk_level			// BSSR value for the initial TCLK level
		, const uint32_t data_out
		)
	{
		static_assert(kScan_ != Scan::kGoIdle && kNumBits_ != NumBits::kGoIdle, "This method cannot be used to render kGoIdle");
		
		// Select DR-Scan
		*buffer++ = recipe.tms1tck0 | tclk_level;
		if (kScan_ == Scan::kIR)
		{
			// Select IR-Scan
			*buffer++ = recipe.tms1tck0 | tclk_level;
		}
		// Capture-DR / Capture-IR
		*buffer++ = recipe.tms0tck0 | tclk_level;
		*buffer++ = recipe.tms0tck0 | tclk_level;
		uint32_t mask = 0x0001U << ((uint8_t)kNumBits_ - 1);
		for (; mask > 1; mask >>= 1)
		{
			// Shift-DR / Shift-IR
			if ((data_out & mask) != 0)
				*buffer++ = recipe.tms0tck0 | recipe.tdi1;
			else
				*buffer++ = recipe.tms0tck0 | recipe.tdi0;
		}
		// Exit1-DR / Exit1-IR; The last bit
		if ((data_out & mask) != 0)
			*buffer++ = recipe.tms1tck0 | recipe.tdi1;
		else
			*buffer++ = recipe.tms1tck0 | recipe.tdi0;
		// Update-DR / Update-IR
		*buffer++ = recipe.tms1tck0 | tclk_level;
		// Run-test/IDLE
		*buffer++ = recipe.tms0tck0 | tclk_level;
	}
	static constexpr ALWAYS_INLINE uint8_t GetCount()
	{
		if (kScan_ == Scan::kGoIdle)
			return 8;
		else
			return (5 + (uint8_t)kNumBits_ + (uint8_t)kScan_);
	}
	static ALWAYS_INLINE void Start(uint32_t *buffer, const Recipe &recipe)
	{
		uint16_t cnt = GetCount();
		DmaWrData::Start(buffer, &recipe.gpio->BSRR, cnt);
		DmaRise::Start(&recipe.tck1, &recipe.gpio->BSRR, cnt);
		DmaRdData::Start(&recipe.gpio->IDR, buffer, cnt);
		CycleTimer::SetupRepetition(cnt);
		TriggerWrite::EnableDma();
		TriggerRise::EnableDma();
		TriggerRead::EnableDma();
		CycleTimer::CounterResume();
	}
	static ALWAYS_INLINE void Wait()
	{
		while (CycleTimer::IsTimerEnabled())
			;
		/* JTAG state = Run-Test/Idle */
		TriggerWrite::DisableDma();
		TriggerRise::DisableDma();
		TriggerRead::DisableDma();
	}
	static ALWAYS_INLINE uint32_t GetResult(uint32_t *buffer, const Recipe &recipe)
	{
		size_t pos = 3 + (uint8_t)kScan_;
		uint32_t mask = 0x0001U << ((uint8_t)kNumBits_ - 1);
		uint32_t data = 0;
		for (; mask != 0; mask >>= 1, ++pos)
		{
			if ((buffer[pos] & recipe.tdo_mask) != 0)
				data |= mask;
		}
		return data;
	}
	// Prepare a buffer to reset state of the JTAG state machine
	static ALWAYS_INLINE void DoGoIdle(
		uint32_t *buffer,
		const Recipe &recipe)
	{
		static_assert(kScan_ == Scan::kGoIdle && kNumBits_ == NumBits::kGoIdle, "This method cannot be used to render kGoIdle");
		
		uint32_t *wave = buffer;
		// First bit is dummy, just to ensure TMS is up without a clock change
		*wave++ = recipe.tms1tck1;
		*wave++ = recipe.tms1tck0;
		*wave++ = recipe.tms1tck0;
		*wave++ = recipe.tms1tck0;
		*wave++ = recipe.tms1tck0;
		*wave++ = recipe.tms1tck0;
		*wave++ = recipe.tms1tck0;
		*wave++ = recipe.tms0tck0;
		uint32_t tmp;
		// Change default prescaler to a slower one
		if (kScan_ == Scan::kGoIdle)
		{
			tmp = CycleTimer::GetPrescaler();
			CycleTimer::SetPrescaler(CycleTimer::kPrescaler_);
		}
		Start(buffer, recipe);
		Wait();
		if (kScan_ == Scan::kGoIdle)
			CycleTimer::SetPrescaler(tmp);
	}
};


}	// namespace WaveJtag
