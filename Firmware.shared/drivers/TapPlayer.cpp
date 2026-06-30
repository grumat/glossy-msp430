#include "stdproj.h"
#include "TapPlayer.h"
#include <cstdarg>


TapPlayer gPlayer;



// This was designed for a command that fits a register
static_assert(sizeof(TapStep) == sizeof(uint32_t));
// This was designed for a command that fits a register
static_assert(sizeof(TapStep2) == sizeof(uint32_t));
// This was designed for a command that fits a register
static_assert(sizeof(TapStep3) == sizeof(uint32_t));
// This was designed for a command that fits a register
static_assert(sizeof(TapStep4) == sizeof(uint32_t));


/// Single-command dispatch.
///
/// Async semantics: shift commands return the captured TDO via implicit
/// conversion of the `JtagPending` returned by `OnXxxShift`.  That conversion
/// blocks on the in-flight DMA — so this function inherently resolves every
/// shift it issues.  Call sites that do not care about the return value pay
/// one wait per shift; if that becomes a bottleneck, route those commands
/// through the variadic `Play(cmds[], …)` path instead, where Pendings can
/// be discarded so adjacent shifts overlap.
uint32_t TapPlayer::Play(const TapStep cmd)
{
	switch (cmd.s2.cmd)
	{
	case cmdIrShift:
	{
		itf_->OnTclk((DataClk)cmd.s4.arg4a);
		uint8_t res = itf_->OnIrShift((Ir)cmd.s4.arg16);
		itf_->OnTclk((DataClk)cmd.s4.arg4b);
		return res;
	}
	case cmdIrShift8:
		itf_->OnIrShift((Ir)cmd.s3.arg8);
		return itf_->OnDrShift8(cmd.s3.arg16);
	case cmdIrShift16:
		itf_->OnIrShift((Ir)cmd.s3.arg8);
		return itf_->OnDrShift16(cmd.s3.arg16);
	case cmdIrShift20:
		itf_->OnIrShift((Ir)cmd.s3.arg8);
		return itf_->OnDrShift20((uint32_t)cmd.s3.arg16);
	case cmdIrShift32:
		itf_->OnIrShift((Ir)((const TapStep3 &)cmd).arg8);
		return itf_->OnDrShift32((uint32_t)cmd.s3.arg16);
	case cmdDrShift8:
		return itf_->OnDrShift8(cmd.s2.arg);
	case cmdDrShift16:
		return itf_->OnDrShift16(cmd.s2.arg);
	case cmdDrShift20:
		return itf_->OnDrShift20(cmd.s2.arg);
	case cmdDrShift32:
		return itf_->OnDrShift32(cmd.s2.arg);
	case cmdIrData16:
		return itf_->OnData16(
			(DataClk)cmd.s4.arg4a
			, cmd.s4.arg16
			, (DataClk)cmd.s4.arg4b
			);
	case cmdClrTclk:
		itf_->OnClearTclk();
		break;
	case cmdSetTclk:
		itf_->OnSetTclk();
		break;
	case cmdPulseTclk:
		itf_->OnPulseTclk();
		break;
	case cmdPulseTclkN:
		itf_->OnPulseTclkN();
		break;
	case cmdStrobeN:
		itf_->OnFlashTclk(cmd.s2.arg);
		break;
	case cmdReleaseCpu:
		ReleaseCpu();
		break;
	case cmdDelay1ms:
	{
		StopWatch().Delay(Timer::Msec(cmd.s2.arg));
		break;
	}
	default:
		assert(false);
		break;
	}
	return 0xFFFFFFFF;
}


/// Fire-and-forget single-command dispatch for write-only steps.
///
/// Mirrors the literal-shift cases of the variadic `Play(cmds[], …)` but for one
/// command: each shift is issued as a statement-expression so the returned
/// `JtagPending` is discarded and its DMA left in flight (drained by the next
/// shift).  This is the single-command counterpart that `Play(const TapStep cmd)`
/// cannot be — that one returns `uint32_t`, forcing the blocking Pending->T
/// conversion even for a discarded result.
///
/// Only shift commands gain anything; non-shift steps (TCLK/delay/release) and
/// the value-returning `cmdIrData16` (OnData16 blocks internally) delegate to the
/// blocking `Play(cmd)`.
void TapPlayer::PlayAsync(const TapStep cmd)
{
	switch (cmd.s2.cmd)
	{
	case cmdIrShift:
		{
			DataClk clk0 = (DataClk)cmd.s4.arg4a;
			if (clk0 != kdNone)
				itf_->OnTclk(clk0);
		}
		itf_->OnIrShift((Ir)cmd.s4.arg16);
		{
			DataClk clk1 = (DataClk)cmd.s4.arg4b;
			if (clk1 != kdNone)
				itf_->OnTclk(clk1);
		}
		break;
	case cmdIrShift8:
		itf_->OnIrShift((Ir)cmd.s3.arg8);
		itf_->OnDrShift8(cmd.s3.arg16);
		break;
	case cmdIrShift16:
		itf_->OnIrShift((Ir)cmd.s3.arg8);
		itf_->OnDrShift16(cmd.s3.arg16);
		break;
	case cmdIrShift20:
		itf_->OnIrShift((Ir)cmd.s3.arg8);
		itf_->OnDrShift20((uint32_t)cmd.s3.arg16);
		break;
	case cmdIrShift32:
		itf_->OnIrShift((Ir)cmd.s3.arg8);
		itf_->OnDrShift32((uint32_t)cmd.s3.arg16);
		break;
	case cmdDrShift8:
		itf_->OnDrShift8(cmd.s2.arg);
		break;
	case cmdDrShift16:
		itf_->OnDrShift16(cmd.s2.arg);
		break;
	case cmdDrShift20:
		itf_->OnDrShift20(cmd.s2.arg);
		break;
	case cmdDrShift32:
		itf_->OnDrShift32(cmd.s2.arg);
		break;
	default:
		// Non-shift, or value-returning composite (cmdIrData16): no async win.
		Play(cmd);
		break;
	}
}


/// Variadic batch dispatch — the hot path for protocol sequences.
///
/// Async semantics: each non-capturing shift command — both the runtime-argument
/// `_argv` variants AND the literal-data steps (`kIrDr*`, `kDr*`, `kIr`) — issues
/// `itf_->OnXxxShift(...)` as a statement-expression and discards the returned
/// `JtagPending` (no implicit conversion happens for a statement-expression).
/// The DMA is left in flight; the *next* shift's internal `JtagWaitTransfer()`
/// drains it before kicking the new frame — so consecutive shifts overlap
/// render(N+1) with DMA(N) for free.  (The literal-data steps each have an
/// explicit case below; without it they would fall through to `default: Play(cmd)`,
/// whose `uint32_t` return forces the Pending->T conversion to BLOCK — which is
/// why constant sequences historically got no overlap.)
///
/// Capturing variants (`cmdXxx_argv_p`) write through the pointer arg, which
/// triggers implicit conversion and resolves the Pending immediately.  Use
/// these only when the value is actually needed downstream.  `cmdIrData16`
/// (`kIrData16`) also blocks (OnData16 returns a value type), via `default`.
void TapPlayer::Play(const TapStep cmds[], const uint32_t count, ...)
{
	va_list args;
	va_start(args, count);
	for (uint32_t i = 0; i < count; ++i)
	{
		const TapStep &cmd = cmds[i];
		switch (cmd.s2.cmd)
		{
		case cmdIrShift_argv_p:
		{
			uint8_t* p = (uint8_t*)va_arg(args, uint8_t*);
			*p = itf_->OnIrShift((Ir)cmd.s2.arg);
			break;
		}
		case cmdIrShift16_argv:
			{
				DataClk clk = (DataClk)cmd.s4.arg4a;
				if (clk != kdNone)
					itf_->OnTclk(clk);
			}
			itf_->OnIrShift((Ir)cmd.s4.arg16);
			itf_->OnDrShift16((uint16_t)va_arg(args, uint32_t));
			{
				DataClk clk = (DataClk)cmd.s4.arg4b;
				if (clk != kdNone)
					itf_->OnTclk(clk);
			}
			break;
		case cmdDrShift16_argv:
			itf_->OnDrShift16((uint16_t)va_arg(args, uint32_t));
			break;
		case cmdDrShift16_argv_i:
		{
			uint16_t *p = (uint16_t *)va_arg(args, uint16_t *);
			itf_->OnDrShift16(*p);
			break;
		}
		case cmdDrShift16_argv_p:
		{
			uint16_t *p = (uint16_t *)va_arg(args, uint16_t *);
			*p = itf_->OnDrShift16(cmd.s2.arg);
			break;
		}
		case cmdIrShift20_argv:
			itf_->OnIrShift((Ir)cmd.s2.arg);
			itf_->OnDrShift20(va_arg(args, uint32_t));
			break;
		case cmdDrShift20_argv:
			itf_->OnDrShift20(va_arg(args, uint32_t));
			break;
		case cmdDrShift20_argv_p:
		{
			uint32_t *p = (uint32_t *)va_arg(args, uint32_t *);
			*p = itf_->OnDrShift20(cmd.s2.arg);
			break;
		}
		case cmdIrShift32_argv:
			itf_->OnIrShift((Ir)cmd.s2.arg);
			itf_->OnDrShift32(va_arg(args, uint32_t));
			break;
		case cmdDrShift32_argv:
			itf_->OnDrShift32(va_arg(args, uint32_t));
			break;
		case cmdDrShift32_argv_p:
		{
			uint32_t *p = (uint32_t *)va_arg(args, uint32_t *);
			*p = itf_->OnDrShift32(cmd.s2.arg);
			break;
		}
		case cmdIrData16_argv:
		{
			uint16_t val = va_arg(args, uint32_t);
			itf_->OnData16(
				(DataClk)cmd.s4.arg4a,
				val,
				(DataClk)cmd.s4.arg4b);
			break;
		}
		case cmdStrobe_argv:
			itf_->OnFlashTclk(va_arg(args, uint32_t));
			break;

		// ── Non-capturing literal-data shift steps (overlap path) ───────────────
		// These mirror the single-command Play(cmd) dispatch but DISCARD the
		// returned JtagPending (statement-expression, fire-and-forget) instead of
		// returning it. That leaves the frame's DMA in flight so the NEXT shift's
		// internal Wait drains it while the CPU renders — render(N+1) overlaps
		// DMA(N). Previously these fell through to `default: Play(cmd)`, whose
		// uint32_t return forced the JtagPending->T conversion to BLOCK, so
		// constant TapStep sequences (e.g. TapDev430Xv2 steps_01/02/03) got ZERO
		// overlap. The capturing *_argv_p variants above stay blocking (the value
		// is needed); non-shift commands keep falling through to `default`.
		// NOTE cmdIrData16 (kIrData16) is deliberately NOT here — OnData16 returns
		// uint16_t by value and blocks internally on its own OnDrShift16, so it
		// can't overlap without a Pending-returning variant (separate change). It
		// stays on the `default` path.
		case cmdIrShift:
			{
				DataClk clk0 = (DataClk)cmd.s4.arg4a;
				if (clk0 != kdNone)
					itf_->OnTclk(clk0);
			}
			itf_->OnIrShift((Ir)cmd.s4.arg16);
			{
				DataClk clk1 = (DataClk)cmd.s4.arg4b;
				if (clk1 != kdNone)
					itf_->OnTclk(clk1);
			}
			break;
		case cmdIrShift8:
			itf_->OnIrShift((Ir)cmd.s3.arg8);
			itf_->OnDrShift8(cmd.s3.arg16);
			break;
		case cmdIrShift16:
			itf_->OnIrShift((Ir)cmd.s3.arg8);
			itf_->OnDrShift16(cmd.s3.arg16);
			break;
		case cmdIrShift20:
			itf_->OnIrShift((Ir)cmd.s3.arg8);
			itf_->OnDrShift20((uint32_t)cmd.s3.arg16);
			break;
		case cmdIrShift32:
			itf_->OnIrShift((Ir)cmd.s3.arg8);
			itf_->OnDrShift32((uint32_t)cmd.s3.arg16);
			break;
		case cmdDrShift8:
			itf_->OnDrShift8(cmd.s2.arg);
			break;
		case cmdDrShift16:
			itf_->OnDrShift16(cmd.s2.arg);
			break;
		case cmdDrShift20:
			itf_->OnDrShift20(cmd.s2.arg);
			break;
		case cmdDrShift32:
			itf_->OnDrShift32(cmd.s2.arg);
			break;

		default:
			Play(cmd);
			break;
		}
	}
}


JtagId TapPlayer::SetJtagRunReadLegacy()
{
	itf_->OnIrShift(Ir::kCntrlSig16Bit);
	itf_->OnDrShift16(0x2401);
	return (JtagId)(uint8_t)itf_->OnIrShift(Ir::kCntrlSigCapture);
}


/* Release the target CPU from the controlled stop state */
void TapPlayer::ReleaseCpu()
{
	static const TapStep steps[] =
	{
		kTclk0
		/* clear the HALT_JTAG bit */
		, kIrDr16(Ir::kCntrlSig16Bit, 0x2401)
		, kIr(Ir::kAddrCapture)
		, kTclk1
	};
	Play(steps, _countof(steps));
}

