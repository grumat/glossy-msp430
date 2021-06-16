#pragma once

#include "timer.h"


template <typename TimerTemplate>
class StopWatchTemplate
{
public:
	typedef uint32_t TypCnt;
	/// ctor starts the stop watch
	ALWAYS_INLINE StopWatchTemplate() { Start(); }
	// Start the stop watch at the moment t0 expressed in milliseconds (i.e.
	// calling Time() immediately afterwards returns t0). This can be used to
	// restart an existing stopwatch.
	ALWAYS_INLINE void Start()
	{
		m_t0 = TimerTemplate::GetCounter();
	}
	ALWAYS_INLINE void Restart()
	{
		m_t0 = TimerTemplate::GetCounter();
	}
	/// Elapsed time in ms
	ALWAYS_INLINE TypCnt GetEllapsedTime() const
	{
		return TimerTemplate::DistanceOf(m_t0);
	}
	/// Delay CPU
	void Delay(uint32_t ms) NO_INLINE
	{
		while(GetEllapsedTime() < ms)
		{ }
	}

private:
	// The clock value when the stop watch was last started. Its units vary
	// depending on the platform.
	TypCnt m_t0;
};

