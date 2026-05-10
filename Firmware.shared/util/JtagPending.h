/// JtagPending<T> — POD result handle for an in-flight JTAG shift.
///
/// Returned by every `ITapInterface::OnXxxShift()` call. The shift starts the
/// DMA transfer and returns immediately — the caller decides when (or whether)
/// to wait for the result:
///
///   * **Use immediately**: implicit conversion or `.Get()` blocks until the
///     transfer finishes, then decodes and returns the captured TDO value.
///       uint16_t r = jtag.OnDrShift16(0xA55A);   // legacy synchronous style
///
///   * **Pipeline**: stash the Pending, do other work (often: queue the next
///     shift), then resolve later. The next shift's internal `WaitTransfer()`
///     handles the wait for *this* frame implicitly when overlap is desired.
///       auto p = jtag.OnDrShift16(0);
///       jtag.OnDrShift16(addr);                  // overlaps with p's DMA
///       uint16_t r = p;                          // typically already ready
///
///   * **Discard**: ignore the return value entirely. The next shift waits on
///     this transfer before starting its own; correctness is preserved.
///       jtag.OnDrShift16(0xA55A);                // fire-and-forget
///
/// ## Lifetime constraint — at most one outstanding Pending
///
/// The RX buffer behind a Pending lives in a 2-half ping-pong (`JtagDev::buf_`).
/// After the *next* shift starts, the pong half is still safe; after the
/// shift *after that*, the buffer rotates back and is overwritten. So:
///
///   - Resolving (or discarding) a Pending before issuing more than one
///     subsequent shift is **always** safe.
///   - Holding a Pending across two further shifts is **undefined** — the
///     decoded value will be from a later frame, not the one you wanted.
///
/// Typical call patterns (legacy `T r = jtag.OnXxx(...);`, statement-only
/// fire-and-forget, and the TapPlayer batch loop) all satisfy this naturally.
///
/// ## Size
/// 8 bytes (rx pointer + decoder function pointer). Passes in r0/r1 on
/// Cortex-M; the compiler inlines the construction site, so the cost over a
/// direct synchronous return is one extra register move.
///
/// `WaitTransfer()` is a single static covering whatever DMA the active JtagDev
/// backend uses for shifts — there is only one SPI RX DMA channel per build,
/// so per-Pending wait-function pointers are unnecessary.

#pragma once

#include <cstdint>

/// Defined by the active `JtagDev.<variant>.cpp`. Polls the SPI RX DMA TC
/// flag for the in-flight shift and returns once the frame has fully drained.
/// Idempotent — safe to call when no transfer is in flight (returns
/// immediately because the DMA completion latch is already cleared).
extern void JtagWaitTransfer();


template <typename T>
class JtagPending
{
public:
	using DecodeFn = T (*)(const uint8_t* rx);

	constexpr JtagPending(uint8_t* rx, DecodeFn decode)
		: rx_{rx}, decode_{decode}
	{
	}

	/// Implicit conversion: blocks on the in-flight transfer, then decodes
	/// and returns the captured TDO value. Lets every existing call site
	/// `T r = jtag.OnXxxShift(x);` keep compiling unchanged.
	ALWAYS_INLINE operator T() const
	{
		JtagWaitTransfer();
		return decode_(rx_);
	}

	/// Same as the implicit conversion; useful at sites where an explicit
	/// `.Get()` reads better than relying on conversion context.
	ALWAYS_INLINE T Get() const
	{
		JtagWaitTransfer();
		return decode_(rx_);
	}

	/// No-op marker — the next shift's internal `WaitTransfer()` covers
	/// this frame's DMA before starting its own. Useful for self-documenting
	/// fire-and-forget call sites.
	ALWAYS_INLINE void Discard() const
	{
	}

private:
	uint8_t* rx_;
	DecodeFn decode_;
};
