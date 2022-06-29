#pragma once


template <const int SZ_>
class Fifo
{
public:
	/// Fixed buffer size
	static constexpr size_t kBufSize = SZ_;

	/// Reset/empties the FIFO
	void Reset() { m_nPut = m_nGet = 0; }
	/// Checks if FIFO is full
	bool IsFull() const { return GetCount() >= kBufSize - 1; }
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
		return kBufSize - GetCount();
	}

	///  Puts a char into the FIFO
	bool Put(char ch)
	{
		// Can we add more chars?
		if (IsFull())
			return false;
		// Put char into buffer and increment index
		m_Data[m_nPut++] = ch;
		// Check for index wrap around
		if (m_nPut >= kBufSize)
			m_nPut = 0;
		return true;
	}

	/// Retrieves the next char in the FIFO (or -1)
	int Get()
	{
		// Something there to read?
		if (m_nPut == m_nGet)
			return -1;
		// Take char at read pos and increment index
		int ch = m_Data[m_nGet++];
		// Check for index wrap around
		if (m_nGet >= kBufSize)
			m_nGet = 0;
		return ch;
	}

private:
	// Char buffer
	char m_Data[kBufSize];
	// Index where data flows in
	volatile int m_nPut;
	// Index where data flows out
	volatile int m_nGet;
};

