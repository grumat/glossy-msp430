#pragma once

#include "clocks.h"
#include "mcu-system.h"
#include "exti.h"


extern "C" void SysTick_Handler(void);

enum SysTickPollType
{
	kPollRun		//!< Continuous polling during delay
	, kPollWfi		//!< Sleep until interrupted
	, kPollWfe		//!< Sleep until event occurs
};

template<
	typename SysClk
	, const uint32_t kFrequency = 1000
	, const SysTickPollType kSysTickPollType = kPollRun
>
class ALIGNED SysTickTemplate
{
public:
	static constexpr uint32_t kFrequency_ = kFrequency;
	static constexpr uint32_t kReload_ = SysClk::kFrequency_ / kFrequency;
	static constexpr SysTickPollType kSysTickPollType_ = kSysTickPollType;

	static inline volatile uint32_t sys_tick_;

	ALWAYS_INLINE static void Init(void)
	{
		sys_tick_ = 0;
		SysTick->LOAD = kReload_;
		SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
		EnableIRQ();
	}
	ALWAYS_INLINE static void EnableIRQ(void)
	{
		SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
	}
	ALWAYS_INLINE static void DisableIRQ(void)
	{
		SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
	}
protected:
	friend void SysTick_Handler(void);
	ALWAYS_INLINE static void Handler()
	{
		++sys_tick_;
	}

public:
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
			while (GetEllapsedTime() < ms)
			{
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
	typename SysClk
	, const uint32_t kFrequency = 1000UL
	, const uint32_t kSoftFreq = 200UL
	, const SysTickPollType kSysTickPollType = kPollWfi
>
class ALIGNED SysTickExTemplate : public SysTickTemplate<SysClk, kFrequency, kSysTickPollType>
{
public:
	typedef SysTickTemplate<SysClk, kFrequency, kSysTickPollType> Base;
	static constexpr uint32_t kFrequency_ = kSoftFreq;
	static inline volatile uint32_t soft_tick_;

	ALWAYS_INLINE static void Init(void)
	{
		// A soft timer shall be slower as the tick timer
		static_assert(kSoftFreq < kFrequency, "Soft timer shall be slower than the hardware timer that drives it");
		Base::Init();
	}

protected:
	static inline uint32_t soft_counter_;
	friend void SysTick_Handler(void);
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

