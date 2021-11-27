#include "stdproj.h"
#include "TapPlayer.h"
#include <cstdarg>


TapPlayer g_Player;


struct TapStep3
{
	TapCmd cmd : 8;
	uint32_t arg8 : 8;
	uint32_t arg16 : 16;
};

// This was designed for a command that fits a register
static_assert(sizeof(TapStep) == sizeof(uint32_t));


uint32_t TapPlayer::Play(const TapStep cmd)
{
	switch (cmd.cmd)
	{
	case cmdIrShift:
		return itf_->OnIrShift(cmd.arg);
	case cmdIrShift8:
		itf_->OnIrShift(((const TapStep3&)cmd).arg8);
		return itf_->OnDrShift8(((const TapStep3&)cmd).arg16);
	case cmdIrShift16:
		itf_->OnIrShift(((const TapStep3&)cmd).arg8);
		return itf_->OnDrShift16(((const TapStep3&)cmd).arg16);
	case cmdIrShift20:
		itf_->OnIrShift(((const TapStep3&)cmd).arg8);
		return itf_->OnDrShift20((uint32_t)((const TapStep3&)cmd).arg16);
	case cmdDrShift8:
		return itf_->OnDrShift8(cmd.arg);
	case cmdDrShift16:
		return itf_->OnDrShift16(cmd.arg);
	case cmdDrShift20:
		return itf_->OnDrShift20(cmd.arg);
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
		itf_->OnFlashTclk(cmd.arg);
		break;
	case cmdReleaseCpu:
		ReleaseCpu();
		break;
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
		switch (cmd.cmd)
		{
		case cmdIrShift_argv_p:
		{
			uint8_t* p = (uint8_t*)va_arg(args, uint8_t*);
			*p = itf_->OnIrShift(cmd.arg);
			break;
		}
		case cmdIrShift16_argv:
			itf_->OnIrShift(cmd.arg);
			itf_->OnDrShift16((uint16_t)va_arg(args, uint32_t));
			break;
		case cmdDrShift16_argv_p:
		{
			uint16_t* p = (uint16_t*)va_arg(args, uint16_t*);
			*p = itf_->OnDrShift16(cmd.arg);
			break;
		}
		case cmdIrShift20_argv:
			itf_->OnIrShift(cmd.arg);
			itf_->OnDrShift20(va_arg(args, uint32_t));
			break;
		case cmdDrShift16_argv:
			itf_->OnDrShift16((uint16_t)va_arg(args, uint32_t));
			break;
		case cmdDrShift20_argv:
			itf_->OnDrShift20(va_arg(args, uint32_t));
			break;
		case cmdStrobe_argv:
			itf_->OnFlashTclk(va_arg(args, uint32_t));
			break;
		default:
			Play(cmd);
			break;
		}
	}
}


/* Release the target CPU from the controlled stop state */
void TapPlayer::ReleaseCpu()
{
#if 0
	itf_->OnClearTclk();

	/* clear the HALT_JTAG bit */
	itf_->OnIrShift(IR_CNTRL_SIG_16BIT);
	itf_->OnDrShift16(0x2401);
	itf_->OnIrShift(IR_ADDR_CAPTURE);
	itf_->OnSetTclk();
#else
	static const TapStep steps[] =
	{
		kTclk0
		/* clear the HALT_JTAG bit */
		, kIrDr16(IR_CNTRL_SIG_16BIT, 0x2401)
		, kIr(IR_ADDR_CAPTURE)
		, kTclk1
	};
	Play(steps, _countof(steps));
#endif
}

