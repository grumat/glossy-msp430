#pragma once

#if OPT_TMS_AUTO_SHAPER

template <
	typename SysClk							///< System clock that drives timers
	, const Tim kTimer				///< Timer used for TMS shape generation
	, const ExtClockSource kClkSource		///< Timer channel used for input of the CLK signal
	, const TimChannel kTmsOut				///< Timer channel used for output of the TMS
	, const uint32_t kClkSpeed1				///< Slowest clock speed
	, const uint32_t kClkSpeed2				///< Slow clock speed
	, const uint32_t kClkSpeed3				///< Medium clock speed
	, const uint32_t kClkSpeed4				///< Fast clock speed
	, const uint32_t kClkSpeed5				///< Fastest clock speed
>
class TmsAutoShaper
{
public:
	typedef ExternalClock<
		kTimer
		, kClkSource
		, kClkSpeed1			///< 562.5 khz (72 Mhz / 256)
		, 1						///< No prescaler for JCLK clock
		, 0						///< Input filter selection (fastest produces ~60ns delay)
	> Clk1_;
	typedef TimerTemplate
	<
		Clk1_					///< Don't care as all time bases use same prescaler
		, TimerMode::kSingleShot	///< Single shot timer
		, 65535					///< Don't care
		, false					///< No register buffering as DMA will modify on the fly
	> TimConf1_;
	typedef ExternalClock<
		kTimer
		, kClkSource			///< Timer Input 1 (PA8)
		, kClkSpeed2			///< 1.125 MHz (72 Mhz / 128)
		, 1						///< No prescaler for JCLK clock
		, 0						///< Input filter selection (fastest produces ~60ns delay)
	> Clk2_;
	typedef TimerTemplate
	<
		Clk2_					///< Don't care as all time bases use same prescaler
		, TimerMode::kSingleShot	///< Single shot timer
		, 65535					///< Don't care
		, false					///< No register buffering as DMA will modify on the fly
	> TimConf2_;
	typedef ExternalClock<
		kTimer
		, kClkSource
		, kClkSpeed3			///< 2.25 MHz (72 Mhz / 64)
		, 1						///< No prescaler for JCLK clock
		, 0						///< Input filter selection (fastest produces ~60ns delay)
	> Clk3_;
	typedef TimerTemplate
	<
		Clk3_					///< Don't care as all time bases use same prescaler
		, TimerMode::kSingleShot	///< Single shot timer
		, 65535					///< Don't care
		, false					///< No register buffering as DMA will modify on the fly
	> TimConf3_;
	typedef ExternalClock<
		kTimer
		, kClkSource
		, kClkSpeed4			///< 4.5 MHz (72 Mhz / 32)
		, 1						///< No prescaler for JCLK clock
		, 0						///< Input filter selection (fastest produces ~60ns delay)
	> Clk4_;
	typedef TimerTemplate
	<
		Clk4_					///< Don't care as all time bases use same prescaler
		, TimerMode::kSingleShot	///< Single shot timer
		, 65535					///< Don't care
		, false					///< No register buffering as DMA will modify on the fly
	> TimConf4_;
	typedef ExternalClock<
		kTimer
		, kClkSource
		, kClkSpeed5			///< 9.0 MHz (72 Mhz / 16)
		, 1						///< No prescaler for JCLK clock
		, 0						///< Input filter selection (fastest produces ~60ns delay)
	> Clk5_;
	typedef TimerTemplate
	<
		Clk5_					///< Don't care as all time bases use same prescaler
		, TimerMode::kSingleShot	///< Single shot timer
		, 65535					///< Don't care
		, false					///< No register buffering as DMA will modify on the fly
	> TimConf5_;
	
	typedef TimerOutputChannel
	<
		TimConf1_				///< Associate timer class to the output
		, TimChannel::k3		///< Channel 3 is out output (PA9)
		, TimOutMode::kTimOutLow	///< TMS level defaults to low
		, TimOutDrive::kTimOutActiveHigh	///< Active High output
		, TimOutDrive::kTimOutInactive		///< No negative output
		, false					///< No preload
		, false					///< Fast mode has no effect in timer pulse mode
	> TmsOutCh_;
	
	typedef DmaChannel
	<
		TmsOutCh_::DmaInstance_
		, TmsOutCh_::DmaCh_
		, kDmaMemToPer
		, kDmaShortPtrInc
		, kDmaShortPtrConst
		, kDmaVeryHighPrio
	> GeneratorDma_;
	
	/// Initializes the TMS output
	static ALWAYS_INLINE void InitOutput()
	{
		TmsOutCh_::Setup();		// Init() was already done by source of timer
		GeneratorDma_::Init();
	}
	/// Closing device
	static ALWAYS_INLINE void Close()
	{
		TimConf1_::CounterStop();
		GeneratorDma_::Stop();
	}
	/// Forces the TMS level to High
	static ALWAYS_INLINE void Set(const bool restore = true)
	{
		TmsOutCh_::SetOutputMode(TimOutMode::kTimOutHigh);
		if (restore)
			TmsOutCh_::SetOutputMode(TimOutMode::kTimOutToggle);
	}
	// Positive pulse on TMS line
	static ALWAYS_INLINE void Pulse(const bool restore = true)
	{
		TmsOutCh_::SetOutputMode(TimOutMode::kTimOutHigh);
		TmsOutCh_::SetOutputMode(TimOutMode::kTimOutLow);
		if (restore)
			TmsOutCh_::SetOutputMode(TimOutMode::kTimOutToggle);
	}
	/// Sets the TMS pulse counter for next toggle
	static ALWAYS_INLINE void NextToggle(uint16_t clks)
	{
		TmsOutCh_::SetCompare(clks);
	}
	/// Starts DMA
	static ALWAYS_INLINE void StartDma(const void *src, const uint16_t cnt)
	{
		GeneratorDma_::Start(src, TmsOutCh_::GetCcrAddress(), cnt);
	}
	/// Start Timer pre-programming first toggle
	static ALWAYS_INLINE void Start(uint16_t next_toggle)
	{
		NextToggle(next_toggle);
		TimConf1_::StartShot();
	}
	/// Stops reading input clock
	static ALWAYS_INLINE void Stop()
	{
		TimConf1_::CounterStop();
	}
	
public:
	class SuspendAutoOutput
	{
	public:
		SuspendAutoOutput()
		{
			TmsOutCh_::DisableDma();
		}
		~SuspendAutoOutput()
		{
			TmsOutCh_::EnableDma();
		}
	};
};

#endif	// OPT_TMS_AUTO_SHAPER
