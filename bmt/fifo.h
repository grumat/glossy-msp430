#pragma once


template <const int SZ_>
class Fifo
{
public:
	/// Fixed buffer size
	static constexpr size_t kBufSize = SZ_;

	/// Reset/empties the FIFO
	ALWAYS_INLINE void Reset() { m_nPut = m_nGet = 0; }
	/// Checks if FIFO is full
	ALWAYS_INLINE bool IsFull() const { return GetCount() >= (kBufSize - 1); }
	/// Return total bytes in buffer
	ALWAYS_INLINE size_t GetCount() const
	{
		// Difference between in and out
		int dif = m_nPut - m_nGet;
		// Fix wrap arounds
		if (dif < 0)
			dif += kBufSize;
		return (size_t)dif;
	}
	/// Return bytes available on buffer
	ALWAYS_INLINE size_t GetFree() const
	{
		// Complements the GetCount() method
		return kBufSize - GetCount() - 1;
	}

	///  Puts a char into the FIFO
	bool Put(char ch)
	{
		// Can we add more chars?
		if (IsFull())
			return false;
		// Put char into buffer and increment index
		int tmp = m_nPut;
		m_Data[tmp++] = ch;
		// Check for index wrap around
		if (tmp >= kBufSize)
			tmp = 0;
		m_nPut = tmp;
		return true;
	}

	/// Retrieves the next char in the FIFO (or -1)
	int Get()
	{
		// Something there to read?
		if (m_nPut == m_nGet)
			return -1;
		// Take char at read pos and increment index
		int tmp = m_nGet;
		int ch = m_Data[tmp++];
		// Check for index wrap around
		if (tmp >= kBufSize)
			tmp = 0;
		m_nGet = tmp;
		return ch;
	}

private:
	// Index where data flows in
	volatile int m_nPut;
	// Index where data flows out
	volatile int m_nGet;
	// Char buffer
	char m_Data[kBufSize];
};

