#pragma once


template <int SZ_>
class Fifo
{
public:
	enum : size_t
	{
		kBufSize = SZ_,
	};

	void Reset() { m_nPut = m_nGet = 0; }
	bool IsFull() const { return GetCount() >= kBufSize - 1; }
	//! Return total bytes in buffer
	ALWAYS_INLINE size_t GetCount() const
	{
		int dif = m_nPut - m_nGet;
		if (dif < 0)
			dif += kBufSize;
		return (size_t)dif;
	}
	//! Return bytes available on buffer
	ALWAYS_INLINE size_t GetFree() const
	{
		return kBufSize - GetCount();
	}

	bool Put(char ch)
	{
		if (IsFull())
			return false;
		m_UartInBuf[m_nPut++] = ch;
		if (m_nPut >= kBufSize)
			m_nPut = 0;
		return true;
	}

	int Get()
	{
		if (m_nPut == m_nGet)
			return -1;
		int ch = m_UartInBuf[m_nGet++];
		if (m_nGet >= kBufSize)
			m_nGet = 0;
		return ch;
	}


private:
	char m_UartInBuf[kBufSize];
	volatile int m_nPut;
	volatile int m_nGet;
};

