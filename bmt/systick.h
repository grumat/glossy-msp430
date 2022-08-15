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
			/// Hold CPU flow as long as time has not elapsed
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

/// A type safe uint32_t for systimer tick units
enum SysTickUnits : uint32_t;

/// Use of SysTick as a simple raw timer without interrupts
template<
	typename SysClk			///< System clock
>
class ALIGNED SysTickCounter
{
public:
	/// Use CPUFreq/8 as time base
	static constexpr uint32_t kFrequency_ = SysClk::kFrequency_ / 8;

	/// Initialize the tick timer
	ALWAYS_INLINE static void Init(void)
	{
		SysTick->LOAD = 0x00FFFFFF;
		SysTick->CTRL = SysTick_CTRL_ENABLE_Msk;	// CPU/8
	}
	/// Returns the current counter raw value
	ALWAYS_INLINE static uint32_t GetRawValue()
	{
		return SysTick->VAL;
	}

// Time conversion
public:
	/// Computes the total amount of ticks for the given milliseconds (low performance option when used with constants)
	static SysTickUnits ToTicks(uint32_t ms) NO_INLINE
	{
		const uint32_t ticks = (ms * kFrequency_) / 1000;
		assert(ticks < 0x00FFFF80);
		return (SysTickUnits)ticks;
	}

	/// Conversion from ms to timer ticks
	template<const uint32_t kMS>
	struct M2T
	{
		static constexpr SysTickUnits kTicks = (SysTickUnits)((kMS * kFrequency_) / 1000);
	};

	/// Conversion from us to timer ticks
	template<const uint32_t kUS>
	struct U2T
	{
		static constexpr SysTickUnits kTicks = (SysTickUnits)((kUS * kFrequency_) / (1000 * 1000));
	};

public:
	/// A stopwatch class using the tick counter
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
			t0_ = GetRawValue();
		}
		/// Elapsed time in ms
		ALWAYS_INLINE SysTickUnits GetEllapsedTicks() const
		{
			int dif = t0_ - GetRawValue();	// down-counter
			if (dif < 0)
				dif += 0x01000000;
			return (SysTickUnits)dif;
		}
		/// Delays CPU by a given timer tick value
		void DelayTicks(SysTickUnits ticks) NO_INLINE
		{
			while (GetEllapsedTicks() < ticks)
			{ }
		}
		/// Delay CPU using ms value (worst case scenario)
		void Delay(uint32_t ms) NO_INLINE
		{
			if (ms)
			{
				// Only good for variables; not recommended for constants
				const SysTickUnits ticks = ToTicks(ms);
				while (GetEllapsedTicks() < ticks)
				{ }
			}
		}
		/// Constant delay of CPU in ms (optimized code)
		template<const uint32_t kMS> NO_INLINE void Delay()
		{
			static constexpr SysTickUnits kTicks = M2T<kMS>::kTicks;
			// Delay time to big for timer resolution 
			static_assert(kTicks <= 0x00FFFF80);
			// Hold CPU flow as long as time has not elapsed
			DelayTicks(kTicks);
		}
		/// Constant delay of CPU in us (optimized code)
		template<const uint32_t kUS> ALWAYS_INLINE void DelayUS()
		{
			static constexpr SysTickUnits kTicks = U2T<kUS>::kTicks;
			// Delay time to big for timer resolution 
			static_assert(kTicks <= 0x00FFFF80);
			// Hold CPU flow as long as time has not elapsed
			DelayTicks(kTicks);
		}

	private:
		// The clock value when the stop watch was last started. Its units vary
		// depending on the platform.
		uint32_t t0_;
	};
};
