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

