#pragma once


/*!
Ping-pong double-buffer pattern: allows preparation of the next hardware DMA transfer
while the current one is in progress. Critical for JTAG because frame rendering (TDI/TMS
bit assembly) can overlap with hardware execution (SPI clock + TIM1 toggles).

Usage (JTAG dtrig example):
  - OnIrShift(): tx = GetNext(); RenderTransaction(tx, ...); RenderTransaction updates
  - Wait() blocks until DMA completes
  - Step() swaps buffers: next becomes current for DMA, current becomes available for next render
  - Start() reuses next buffer for the next frame while current buffer's DMA is still running

Buffer layout: naturally sized to uintptr_t alignment (8 bytes on 64-bit systems) to
minimize padding. Raw bytes are cast to T* to allow any element type (uint8_t for SPI
bytes, uint16_t for TIM CCR values, etc.). Buffers are kept in SRAM and not const because
DMA write must be atomic (GetNext() for reads, GetCurrent() for fresh DMA receives).

For paired/grouped buffers (TX+RX, or TX+RX+AUX) that must advance in lockstep, use the
AnyPingPongBuffer2 / AnyPingPongBuffer3 variants below — they share a single current_
flag so one Step() atomically swaps all sub-buffers.
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
	static constexpr size_t size_ = count * sizeof(T);

public:
	AnyPingPongBuffer()
	{
		current_ = 0;
	}
	// Read operation happens on the current transfer buffer
	T * GetCurrent() { return reinterpret_cast<T*>(buf_ + (current_ * bufcount_)); }
	// Write operation happens on the next buffer
	T * GetNext() { return reinterpret_cast<T*>(buf_ + ((!current_) * bufcount_)); }
	// Step to the next buffer cycle: invalidates the current read buffer to be used next
	void Step() { current_ = !current_; }

private:
	// Number of items of the buffer
	static constexpr size_t bufcount_
		= sizeof(T) < sizeof(uintptr_t)
		? (size_ + sizeof(uintptr_t) - 1) / sizeof(uintptr_t)
		: size_
		;
	// Both buffers
	uintptr_t buf_[2*bufcount_] ALIGNED;
	bool current_;
};


// ── Paired / grouped ping-pong buffers ───────────────────────────────────────
//
// Most DMA peripherals work with paired buffers (TX + RX), and some setups need a
// third channel (e.g. TX + RX + a TMS/CCR aux stream). The variants below bundle
// 2 or 3 independent buffers behind a single current_ flag so one Step() advances
// all of them atomically — eliminating the easy-to-forget bug where one half of a
// pair gets stepped and the other does not.
//
// Each sub-buffer keeps its own element type and count; only the "which half is
// current" flag is shared.

namespace PingPong_
{

// uintptr_t words required to hold (count × T) bytes, padded up for alignment.
template <typename T, size_t count>
constexpr size_t WordCount()
{
	return (count * sizeof(T) + sizeof(uintptr_t) - 1) / sizeof(uintptr_t);
}

} // namespace PingPong_


/*!
Two parallel ping-pong buffers (typically TX + RX) sharing a single current_ flag.

  GetCurrent1() / GetNext1() — first  sub-buffer (T1[N1])
  GetCurrent2() / GetNext2() — second sub-buffer (T2[N2])
  Step()                     — atomically advances both halves
*/
template <
	typename T1, const size_t count1
	, typename T2, const size_t count2
>
class AnyPingPongBuffer2
{
public:
	static constexpr size_t count1_ = count1;
	static constexpr size_t size1_  = count1 * sizeof(T1);
	static constexpr size_t count2_ = count2;
	static constexpr size_t size2_  = count2 * sizeof(T2);

	AnyPingPongBuffer2() { current_ = 0; }

	T1 * GetCurrent1() { return reinterpret_cast<T1*>(buf1_ + ( current_ * bufcount1_)); }
	T1 * GetNext1()    { return reinterpret_cast<T1*>(buf1_ + ((!current_) * bufcount1_)); }
	T2 * GetCurrent2() { return reinterpret_cast<T2*>(buf2_ + ( current_ * bufcount2_)); }
	T2 * GetNext2()    { return reinterpret_cast<T2*>(buf2_ + ((!current_) * bufcount2_)); }

	// Atomically swaps both sub-buffers in one operation.
	void Step() { current_ = !current_; }

private:
	static constexpr size_t bufcount1_ = PingPong_::WordCount<T1, count1>();
	static constexpr size_t bufcount2_ = PingPong_::WordCount<T2, count2>();

	uintptr_t buf1_[2 * bufcount1_] ALIGNED;
	uintptr_t buf2_[2 * bufcount2_] ALIGNED;
	bool current_;
};


/*!
Three parallel ping-pong buffers (e.g. TX + RX + AUX) sharing a single current_ flag.

  GetCurrent1() / GetNext1() — first  sub-buffer (T1[N1])
  GetCurrent2() / GetNext2() — second sub-buffer (T2[N2])
  GetCurrent3() / GetNext3() — third  sub-buffer (T3[N3])
  Step()                     — atomically advances all three halves
*/
template <
	typename T1, const size_t count1
	, typename T2, const size_t count2
	, typename T3, const size_t count3
>
class AnyPingPongBuffer3
{
public:
	static constexpr size_t count1_ = count1;
	static constexpr size_t size1_  = count1 * sizeof(T1);
	static constexpr size_t count2_ = count2;
	static constexpr size_t size2_  = count2 * sizeof(T2);
	static constexpr size_t count3_ = count3;
	static constexpr size_t size3_  = count3 * sizeof(T3);

	AnyPingPongBuffer3() { current_ = 0; }

	T1 * GetCurrent1() { return reinterpret_cast<T1*>(buf1_ + ( current_ * bufcount1_)); }
	T1 * GetNext1()    { return reinterpret_cast<T1*>(buf1_ + ((!current_) * bufcount1_)); }
	T2 * GetCurrent2() { return reinterpret_cast<T2*>(buf2_ + ( current_ * bufcount2_)); }
	T2 * GetNext2()    { return reinterpret_cast<T2*>(buf2_ + ((!current_) * bufcount2_)); }
	T3 * GetCurrent3() { return reinterpret_cast<T3*>(buf3_ + ( current_ * bufcount3_)); }
	T3 * GetNext3()    { return reinterpret_cast<T3*>(buf3_ + ((!current_) * bufcount3_)); }

	// Atomically swaps all three sub-buffers in one operation.
	void Step() { current_ = !current_; }

private:
	static constexpr size_t bufcount1_ = PingPong_::WordCount<T1, count1>();
	static constexpr size_t bufcount2_ = PingPong_::WordCount<T2, count2>();
	static constexpr size_t bufcount3_ = PingPong_::WordCount<T3, count3>();

	uintptr_t buf1_[2 * bufcount1_] ALIGNED;
	uintptr_t buf2_[2 * bufcount2_] ALIGNED;
	uintptr_t buf3_[2 * bufcount3_] ALIGNED;
	bool current_;
};
