#pragma once

#include "clocks.h"
#include "mcu-system.h"
#include "exti.h"

/// Application defined `tick handler` defined on the App
extern "C" void SysTick_Handler(void);

/// How a time delay shall be done
enum SysTickPollType
{
	kPollRun		//!< Continuous polling during delay
	, kPollWfi		//!< Sleep until interrupted
	, kPollWfe		//!< Sleep until event occurs
};

/// Tick timer class template
template<
	typename SysClk						///< System clock
	, const uint32_t kFrequency = 1000	///< Tick frequency
	, const SysTickPollType kSysTickPollType = kPollRun	/// How to perform delays
>
class ALIGNED SysTickTemplate
{
public:
	/// Constant with the Tick frequency
	static constexpr uint32_t kFrequency_ = kFrequency;
	/// Timer reload constant
	static constexpr uint32_t kReload_ = SysClk::kFrequency_ / kFrequency;
	/// How to poll delays
	static constexpr SysTickPollType kSysTickPollType_ = kSysTickPollType;

	/// The tick counter value
	static inline volatile uint32_t sys_tick_;

	/// Initialize the tick timer
	ALWAYS_INLINE static void Init(void)
	{
		sys_tick_ = 0;
		SysTick->LOAD = kReload_;
		SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
		EnableIRQ();
	}
	/// Enables the ISR driven by the timer interrupt request
	ALWAYS_INLINE static void EnableIRQ(void)
	{
		SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
	}
	/// Disables timer interrupt request
	ALWAYS_INLINE static void DisableIRQ(void)
	{
		SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
	}
protected:
	/// ISR can access this class
	friend void SysTick_Handler(void);
	/// Simple tick counter is implemented here
	ALWAYS_INLINE static void Handler()
	{
		++sys_tick_;
	}

public:
	/// A stopwatch class using the tick timer
	class StopWatch
	{
	public:
		/// ctor starts the stop watch
		ALWAYS_INLINE StopWatch() { Start(); }
		// Start the stop watch at the moment t0 expressed in milliseconds (i.e.
		// calling Time() immediately afterwards returns t0). This can be used to
		// restart an existing stopwatch.
		ALWAYS_INLINE void Start()
		{
			t0_ = sys_tick_;
		}
		/// Elapsed time in ms
		ALWAYS_INLINE uint32_t GetEllapsedTime() const
		{
			return sys_tick_ - t0_;
		}
		/// Delay CPU
		void Delay(uint32_t ms)
		{
			/// Hold CPU flow as long as time has not ellapsed
			while (GetEllapsedTime() < ms)
			{
				/// Halt the CPU for low power use
				if (kSysTickPollType_ == kPollWfi)
					__WFI();
				else if (kSysTickPollType_ == kPollWfe)
					__WFE();
			}
		}

	private:
		// The clock value when the stop watch was last started. Its units vary
		// depending on the platform.
		uint32_t t0_;
	};
};


/*!
A Tick timer class that handles two counters. A tick counter and a slower soft 
counter typically used to control lower priority tasks.
*/
template<
	typename SysClk			///< System clock
	, const uint32_t kFrequency = 1000UL	///< Frequency of the tick counter
	, const uint32_t kSoftFreq = 200UL		///< Frequency of the soft-generated sub-frequency
	, const SysTickPollType kSysTickPollType = kPollWfi	///< How to halt the CPU
>
class ALIGNED SysTickExTemplate : public SysTickTemplate<SysClk, kFrequency, kSysTickPollType>
{
public:
	/// Base class
	typedef SysTickTemplate<SysClk, kFrequency, kSysTickPollType> Base;
	/// Soft frequency
	static constexpr uint32_t kFrequency_ = kSoftFreq;
	/// Soft generated tick counter
	static inline volatile uint32_t soft_tick_;

	/// Initialize the timer
	ALWAYS_INLINE static void Init(void)
	{
		// A soft timer shall be slower as the tick timer
		static_assert(kSoftFreq < kFrequency, "Soft timer shall be slower than the hardware timer that drives it");
		Base::Init();
	}

protected:
	/// Counter for the soft timer
	static inline uint32_t soft_counter_;
	/// Tick handler has full access this class
	friend void SysTick_Handler(void);
	/// This implements the ISR Handler
	ALWAYS_INLINE static void Handler()
	{
		Base::Handler();
		soft_counter_ += kFrequency_;
		if (soft_counter_ >= Base::kFrequency_)
		{
			// remark: one of them will be optimized out by compiler
			if (Base::kFrequency_ % kFrequency_ == 0)
			{
				soft_counter_ = 0;
			}
			else
			{
				// Remove keeping remainder
				soft_counter_ -= Base::kFrequency_;
			}
			// update soft tick
			++soft_tick_;
		}
	}
};

