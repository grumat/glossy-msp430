#pragma once

#if OPT_TMS_AUTO_SHAPER

template <
	typename SysClk							///< System clock that drives timers
	, const Timer::Unit kTimer				///< Timer used for TMS shape generation
	, const Timer::ExtClk kClkSource	///< Timer channel used for input of the CLK signal
	, const Timer::Channel kTmsOut			///< Timer channel used for output of the TMS
	, const uint32_t kClkSpeed1				///< Slowest clock speed
	, const uint32_t kClkSpeed2				///< Slow clock speed
	, const uint32_t kClkSpeed3				///< Medium clock speed
	, const uint32_t kClkSpeed4				///< Fast clock speed
	, const uint32_t kClkSpeed5				///< Fastest clock speed
>
class TmsAutoShaper
{
public:
	typedef Timer::ExternalClock<
		kTimer
		, kClkSource
		, kClkSpeed1			///< 562.5 khz (72 Mhz / 256)
		, 1						///< No prescaler for JCLK clock
		, 0						///< Input filter selection (fastest produces ~60ns delay)
	> Clk1_;
	typedef Timer::Any
	<
		Clk1_					///< Don't care as all time bases use same prescaler
		, Timer::Mode::kSingleShot	///< Single shot timer
		, 65535					///< Don't care
		, false					///< No register buffering as DMA will modify on the fly
	> TimConf1_;
	typedef Timer::ExternalClock<
		kTimer
		, kClkSource			///< Timer Input 1 (PA8)
		, kClkSpeed2			///< 1.125 MHz (72 Mhz / 128)
		, 1						///< No prescaler for JCLK clock
		, 0						///< Input filter selection (fastest produces ~60ns delay)
	> Clk2_;
	typedef Timer::Any
	<
		Clk2_					///< Don't care as all time bases use same prescaler
		, Timer::Mode::kSingleShot	///< Single shot timer
		, 65535					///< Don't care
		, false					///< No register buffering as DMA will modify on the fly
	> TimConf2_;
	typedef Timer::ExternalClock<
		kTimer
		, kClkSource
		, kClkSpeed3			///< 2.25 MHz (72 Mhz / 64)
		, 1						///< No prescaler for JCLK clock
		, 0						///< Input filter selection (fastest produces ~60ns delay)
	> Clk3_;
	typedef Timer::Any
	<
		Clk3_					///< Don't care as all time bases use same prescaler
		, Timer::Mode::kSingleShot	///< Single shot timer
		, 65535					///< Don't care
		, false					///< No register buffering as DMA will modify on the fly
	> TimConf3_;
	typedef Timer::ExternalClock<
		kTimer
		, kClkSource
		, kClkSpeed4			///< 4.5 MHz (72 Mhz / 32)
		, 1						///< No prescaler for JCLK clock
		, 0						///< Input filter selection (fastest produces ~60ns delay)
	> Clk4_;
	typedef Timer::Any
	<
		Clk4_					///< Don't care as all time bases use same prescaler
		, Timer::Mode::kSingleShot	///< Single shot timer
		, 65535					///< Don't care
		, false					///< No register buffering as DMA will modify on the fly
	> TimConf4_;
	typedef Timer::ExternalClock<
		kTimer
		, kClkSource
		, kClkSpeed5			///< 9.0 MHz (72 Mhz / 16)
		, 1						///< No prescaler for JCLK clock
		, 0						///< Input filter selection (fastest produces ~60ns delay)
	> Clk5_;
	typedef Timer::Any
	<
		Clk5_					///< Don't care as all time bases use same prescaler
		, Timer::Mode::kSingleShot	///< Single shot timer
		, 65535					///< Don't care
		, false					///< No register buffering as DMA will modify on the fly
	> TimConf5_;
	
	typedef Timer::AnyOutputChannel
	<
		TimConf1_				///< Associate timer class to the output
		, Timer::Channel::k3		///< Channel 3 is out output (PA9)
		, Timer::OutMode::kForceInactive	///< TMS level defaults to low
		, Timer::Output::kEnabled	///< Active High output
		, Timer::Output::kDisabled		///< No negative output
		, false					///< No preload
		, false					///< Fast mode has no effect in timer pulse mode
	> TmsOutCh_;
	
	typedef Dma::AnyChannel
	<
		TmsOutCh_::DmaInstance_
		, TmsOutCh_::DmaCh_
		, Dma::Dir::kMemToPer
		, Dma::PtrPolicy::kShortPtrInc
		, Dma::PtrPolicy::kShortPtr
		, Dma::Prio::kVeryHigh
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
		TmsOutCh_::SetOutputMode(Timer::OutMode::kForceActive);
		if (restore)
			TmsOutCh_::SetOutputMode(Timer::OutMode::kToggle);
	}
	// Positive pulse on TMS line
	static ALWAYS_INLINE void Pulse(const bool restore = true)
	{
		TmsOutCh_::SetOutputMode(Timer::OutMode::kForceActive);
		TmsOutCh_::SetOutputMode(Timer::OutMode::kForceInactive);
		if (restore)
			TmsOutCh_::SetOutputMode(Timer::OutMode::kToggle);
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
