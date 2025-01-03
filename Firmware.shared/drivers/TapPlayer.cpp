#include "stdproj.h"
#include "TapPlayer.h"
#include <cstdarg>


TapPlayer g_Player;



// This was designed for a command that fits a register
static_assert(sizeof(TapStep) == sizeof(uint32_t));
// This was designed for a command that fits a register
static_assert(sizeof(TapStep2) == sizeof(uint32_t));
// This was designed for a command that fits a register
static_assert(sizeof(TapStep3) == sizeof(uint32_t));
// This was designed for a command that fits a register
static_assert(sizeof(TapStep4) == sizeof(uint32_t));


uint32_t TapPlayer::Play(const TapStep cmd)
{
	switch (cmd.s2.cmd)
	{
	case cmdIrShift:
	{
		itf_->OnTclk((DataClk)cmd.s4.arg4a);
		uint8_t res = itf_->OnIrShift(cmd.s4.arg16);
		itf_->OnTclk((DataClk)cmd.s4.arg4b);
		return res;
	}
	case cmdIrShift8:
		itf_->OnIrShift(cmd.s3.arg8);
		return itf_->OnDrShift8(cmd.s3.arg16);
	case cmdIrShift16:
		itf_->OnIrShift(cmd.s3.arg8);
		return itf_->OnDrShift16(cmd.s3.arg16);
	case cmdIrShift20:
		itf_->OnIrShift(cmd.s3.arg8);
		return itf_->OnDrShift20((uint32_t)cmd.s3.arg16);
	case cmdIrShift32:
		itf_->OnIrShift(((const TapStep3 &)cmd).arg8);
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
			*p = itf_->OnIrShift(cmd.s2.arg);
			break;
		}
		case cmdIrShift16_argv:
			{
				DataClk clk = (DataClk)cmd.s4.arg4a;
				if (clk != kdNone)
					itf_->OnTclk(clk);
			}
			itf_->OnIrShift(cmd.s4.arg16);
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
			itf_->OnIrShift(cmd.s2.arg);
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
			itf_->OnIrShift(cmd.s2.arg);
			itf_->OnDrShift32(va_arg(args, uint32_t));
			break;
		case cmdDrShift32_argv:
			itf_->OnDrShift32(va_arg(args, uint32_t));
			break;
		case cmdDrShift32_argv_p:
		{
			uint32_t *p = (uint32_t *)va_arg(args, uint32_t *);
			*p = itf_->OnDrShift20(cmd.s2.arg);
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
		default:
			Play(cmd);
			break;
		}
	}
}


JtagId TapPlayer::SetJtagRunReadLegacy()
{
	itf_->OnIrShift(IR_CNTRL_SIG_16BIT);
	itf_->OnDrShift16(0x2401);
	return (JtagId)(itf_->OnIrShift(IR_CNTRL_SIG_CAPTURE));
}


/* Release the target CPU from the controlled stop state */
void TapPlayer::ReleaseCpu()
{
	static const TapStep steps[] =
	{
		kTclk0
		/* clear the HALT_JTAG bit */
		, kIrDr16(IR_CNTRL_SIG_16BIT, 0x2401)
		, kIr(IR_ADDR_CAPTURE)
		, kTclk1
	};
	Play(steps, _countof(steps));
}

