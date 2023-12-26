#pragma once


/*!
This template uses PWM to generate the TMS pulse. It combines the double buffer
of timer configuration registers to generate both positive slopes required by
a JTAG IR/DR transfer.
This class assumes that TI1 input of the timer is tied to the CLK output of the 
SPI device. It clocks the internal timer at both edges of this signal.
*/
template <
	typename SysClk						///< System clock that drives timers
	, const Timer::Unit kTimer			///< Timer used for TMS shape generation
	, const Timer::Channel kTmsOut		///< Timer channel used for output of the TMS
	, const uint8_t kFilter				///< A filter for the input
>
class TmsAutoShaper
{
public:
	static constexpr Timer::OutMode kOutMode_ = Timer::OutMode::kPWM2;
	/// Prescaler setup for slow JTAG frequency
	typedef Timer::ExternalClock<
		kTimer			///< Timer unit
		, Timer::ExtClk::kTI1F_ED	///< Both edges are required for accuracy
		, 5000000		///< Unused (don't care)
		, 0				///< No prescaler for JCLK clock
		, kFilter		///< Input filter selection (fastest produces ~60ns delay)
	> Clk;
	/// Timer setup for slow JTAG frequency
	typedef Timer::Any
	<
		Clk							///< Associated prescaler setup
		, Timer::Mode::kUpCounter	///< Count mode for PWM
		, 65535						///< Don't care (used later)
		, true						///< Buffering allows for two independent slopes
		, false						///< Strict update signal changes states
	> Config;

	/// The output channel will produce a compatible TMS signal
	typedef Timer::AnyOutputChannel
	<
		Config						///< Associate timer class to the output
		, kTmsOut					///< Channel 3 is out output (PA10)
		, kOutMode_					///< TMS level defaults to low
		, Timer::Output::kEnabled	///< Active High output
		, Timer::Output::kDisabled	///< No negative output
		, true						///< Buffering allows for two independent slopes
	> TmsOutput;
	
	/// Initializes the TMS output
	static ALWAYS_INLINE void InitOutput()
	{
		TmsOutput::Setup(); // Init() was already done by source of timer
	}
	/// Closing device
	static ALWAYS_INLINE void Close()
	{
		Config::CounterStop();
	}
	/// Forces the TMS level to High
	static ALWAYS_INLINE void Set(const bool restore = true)
	{
		TmsOutput::SetOutputMode(Timer::OutMode::kForceActive);
		if (restore)
			TmsOutput::SetOutputMode(kOutMode_);
	}
	// Positive pulse on TMS line
	static ALWAYS_INLINE void Pulse(const bool restore = true)
	{
		TmsOutput::SetOutputMode(Timer::OutMode::kForceActive);
		TmsOutput::SetOutputMode(Timer::OutMode::kForceInactive);
		if (restore)
			TmsOutput::SetOutputMode(kOutMode_);
	}
	/// Start Timer pre-programming first toggle
	static ALWAYS_INLINE void Start(uint16_t t1, uint16_t p1, uint16_t t2, uint16_t p2)
	{
		TmsOutput::DoublePWM(t1, p1, t2, p2);
	}
	/// Reset TAP mode
	static ALWAYS_INLINE void ConfigForResetTap(bool fAnticipateClk)
	{
		Start(0, 12 - fAnticipateClk, 0xfff, 0xfff);
		// Caller should send now an 0xFF and then Stop()
	}
	/// Stops reading input clock
	static ALWAYS_INLINE void Stop()
	{
		Config::CounterStop();
	}
	
public:
	class SuspendAutoOutput
	{
	public:
		SuspendAutoOutput()
		{
			Config::CounterStop();
		}
		~SuspendAutoOutput()
		{
			Config::CounterResume();
		}
	};
};

