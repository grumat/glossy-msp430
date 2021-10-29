#include "stdproj.h"

#include "devcmd.h"
#include "drivers/TapMcu.h"
#include "drivers/JtagDev.h"
#include "util/dis.h"
#include "util/output_util.h"
#include "util/expr.h"


int cmd_regs(char **arg)
{
	address_t regs[DEVICE_NUM_REGS];
	uint8_t code[16];
	int len = sizeof(code);
	int i;

	(void)arg;

	if (g_tap_mcu.GetRegs(regs) < 0)
		return -1;

	/* Check for breakpoints */
	for (i = 0; i < g_tap_mcu.max_breakpoints; i++)
	{
		const struct device_breakpoint *bp =
			&g_tap_mcu.breakpoints[i];

		if ((bp->flags & DEVICE_BP_ENABLED) &&
			(bp->type == DEVICE_BPTYPE_BREAK) &&
			(bp->addr == regs[MSP430_REG_PC]))
			Trace() << "Breakpoint " << i << " triggered (0x" << f::X<4>(bp->addr) << ")\n";
	}

	show_regs(regs);

	return 0;
}

int cmd_reset(char **arg)
{
	(void)arg;

	return g_tap_mcu.SoftReset();
}


enum EraseType
{
	kEraseAll,
	kEraseMain,
	kEraseSegment,
	kEraseRange,
};


int cmd_erase(char **arg)
{
	const char *type_text = get_arg(arg);
	const char *seg_text = get_arg(arg);
	EraseType type = kEraseMain;
	address_t segment = 0;
	address_t total_size = 0;

	if (seg_text && expr_eval(seg_text, &segment) < 0)
	{
		Error() << "erase: invalid expression: " << seg_text << '\n';
		return -1;
	}

	if (type_text)
	{
		if (!strcasecmp(type_text, "all"))
		{
			type = kEraseAll;
		}
		else if (!strcasecmp(type_text, "segment"))
		{
			type = kEraseSegment;
			if (!seg_text)
			{
				Error() << "erase: expected segment address\n";
				return -1;
			}
		}
		else if (!strcasecmp(type_text, "segrange"))
		{
			const char *total_text = get_arg(arg);
			const char *ss_text = get_arg(arg);

			if (!(total_text && ss_text))
			{
				Error() << "erase: you must specify total and segment sizes\n";
				return -1;
			}

			if (expr_eval(total_text, &total_size) < 0)
			{
				Error() << "erase: invalid expression: " << total_text << '\n';
				return -1;
			}
			type = kEraseRange;
		}
		else
		{
			Error() << "erase: unknown erase type: " << type_text << '\n';
			return -1;
		}
	}

	if (g_tap_mcu.Halt() < 0)
		return -1;

	bool res = true;
	Trace() << "Erasing...\n";
	switch (type)
	{
	case kEraseSegment:
		res = g_tap_mcu.EraseSegment(segment);
		break;
	case kEraseRange:
		res = g_tap_mcu.EraseRange(segment, total_size);
		break;
	case kEraseAll:
		res = g_tap_mcu.EraseAll();
		break;
	case kEraseMain:
	default:
		res = g_tap_mcu.EraseMain();
	}
	return (res - 1);
}


int cmd_run(char **arg)
{
	device_status_t status;
	address_t regs[DEVICE_NUM_REGS];

	(void)arg;

	if (g_tap_mcu.GetRegs(regs) < 0)
	{
		Error() << "warning: device: can't fetch registers\n";
	}
	else
	{
		int i;

		for (i = 0; i < g_tap_mcu.max_breakpoints; i++)
		{
			struct device_breakpoint *bp =
				&g_tap_mcu.breakpoints[i];

			if ((bp->flags & DEVICE_BP_ENABLED) &&
				bp->type == DEVICE_BPTYPE_BREAK &&
				bp->addr == regs[0])
				break;
		}

		if (i < g_tap_mcu.max_breakpoints)
		{
			Trace() << "Stepping over breakpoint #" << i << " at 0x" << f::X<4>(regs[0]) << '\n';
			g_tap_mcu.SingleStep();
		}
	}

	if (g_tap_mcu.Run() < 0)
	{
		Error() << "run: failed to start CPU\n";
		return -1;
	}

	do
	{
		status = g_tap_mcu.Poll();
	}
	while (status == DEVICE_STATUS_RUNNING);

	if (status == DEVICE_STATUS_INTR)
		Trace() << '\n';

	if (status == DEVICE_STATUS_ERROR)
		return -1;

	if (g_tap_mcu.Halt() < 0)
		return -1;

	return cmd_regs(NULL);
}

int cmd_set(char **arg)
{
	char *reg_text = get_arg(arg);
	char *val_text = get_arg(arg);
	int reg;
	address_t value = 0;
	address_t regs[DEVICE_NUM_REGS];

	if (!(reg_text && val_text))
	{
		Error() << "set: must specify a register and a value\n";
		return -1;
	}

	reg = dis_reg_from_name(reg_text);
	if (reg < 0)
	{
		Error() << "set: unknown register: " << reg_text << '\n';
		return -1;
	}

	if (expr_eval(val_text, &value) < 0)
	{
		Error() << "set: can't parse value: " << val_text << '\n';
		return -1;
	}

	if (g_tap_mcu.GetRegs(regs) < 0)
		return -1;
	regs[reg] = value;
	if (g_tap_mcu.SetRegs(regs) < 0)
		return -1;

	show_regs(regs);
	return 0;
}

