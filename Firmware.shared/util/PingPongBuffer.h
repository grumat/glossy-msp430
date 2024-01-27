#pragma once


/*!
This template class implements a typical ping/pong buffer, which allows to prepare the 
next transfer, while the current is happening. This helps optimize transfer rates, 
because you can prepare the next transfer while an autonomous hardware exchanges the 
current buffer.
*/
template <
	typename T				// The data-type of each element
	, const size_t count	// The number of element on one of the IO buffers
>
class AnyPingPongBuffer
{
public:
	// Number of items of the buffer
	static constexpr size_t count_ = count;

public:
	void Init()
	{
		current_ = 0;
	}
	// Read operation happens on the current transfer buffer
	T *GetCurrent() { return buf_[current_]; }
	// Write operation happens on the next buffer
	T *GetNext() { return buf_[!current_]; }
	// Step to the next buffer cycle: invalidates the current read buffer to be used next
	void Step() { current_ = !current_; }

private:
	// Both buffers
	static inline T buf_[count_][2];
	static inline bool current_;
};

