#include "stdproj.h"

#include "MonitorCmd.h"
#include "CmdDb.h"
#include "drivers/TapMcu.h"
#include "drivers/TargetPower.h"
#include "drivers/JtagDev.h"
#include "util/dis.h"
#include "util/MonitorBuf.h"
#include "util/expr.h"


// Context for the grouped 'help' listing: print every command whose 'states'
// field exactly equals 'match', through the single shared MonitorStream (a fresh
// MonitorStream would reset the shared buffer — see OutStream ctor).
struct help_group_ctx
{
	MonitorStream	*strm;
	uint8_t			match;
};

static int help_print_group(void *user_data, const CmdDb::Record *rec)
{
	const help_group_ctx *c = static_cast<const help_group_ctx *>(user_data);
	if (rec->states == c->match)
	{
		*c->strm << "  " << f::S<12>(rec->name);
		if (rec->brief)
			*c->strm << rec->brief;
		*c->strm << '\n';
	}
	return 0;
}

int MonitorCmd::Help(char **arg)
{
	const char *topic = get_arg(arg);
	MonitorStream strm;

	if (topic)
	{
		CmdDb::Record cmd;

		if (!CmdDb::Get(topic, &cmd))
		{
			strm << "COMMAND: " << cmd.name << "\n\n" << cmd.help << '\n';
			return 0;
		}

		// Write to the monitor stream (not Error(), which goes to the internal
		// trace channel) so the reply reaches the GDB client.
		strm << "help: unknown command: " << topic << '\n';
		return -1;
	}
	else
	{
		// List commands grouped by the connection state in which they are
		// valid (see CmdDb::State in CmdDb.h): "any" (both), pre-connect only,
		// and post-connect only. All output goes through the one 'strm' instance.
		strm << "Available commands:\n";

		help_group_ctx any_grp  = { &strm, CmdDb::kAny };
		help_group_ctx pre_grp  = { &strm, CmdDb::kPre };
		help_group_ctx post_grp = { &strm, CmdDb::kPost };

		strm << "\n any state:\n";
		CmdDb::Enum(help_print_group, &any_grp);
		strm << "\n before connect:\n";
		CmdDb::Enum(help_print_group, &pre_grp);
		strm << "\n after connect (target attached):\n";
		CmdDb::Enum(help_print_group, &post_grp);

		strm << "\nType \"help <topic>\" for more information.\n";
	}

	return 0;
}


int MonitorCmd::Dispatch(char *line)
{
	// Remove trailing blanks
	int len = strlen(line);
	while (len && isspace(line[len - 1]))
		len--;
	line[len] = 0;

	// Parse first argument
	const char *cmd_text = get_arg(&line);
	if (cmd_text == nullptr)
		return 0;

	// Allow a leading '#' to stash a command in history without executing it
	if (*cmd_text == '#')
		return 0;

	// Translate command name to its table record
	CmdDb::Record cmd;
	if (CmdDb::Get(cmd_text, &cmd))
	{
		MonitorStream() << "unknown command: " << cmd_text << " (try \"help\")\n";
		return -1;
	}

	// Dual-state gate: a command is only dispatched if its declared 'states'
	// includes the current connection state. Otherwise reply with a hint
	// instead of running it (see CmdDb::State in CmdDb.h).
	const bool attached = g_TapMcu.IsAttached();
	const uint8_t need = attached ? CmdDb::kPost : CmdDb::kPre;
	if (!(cmd.states & need))
	{
		if (attached)
			MonitorStream() << cmd.name
				<< ": not available while a target is connected\n";
		else
			MonitorStream() << cmd.name
				<< ": requires a connected target (try \"jtag_scan\" or \"sbw_scan\")\n";
		return -1;
	}

	return cmd.func(&line);
}


static const char *reg_name(msp430_reg_t reg)
{
	const char *name = dis_reg_name(reg);
	return name ? name : "???";
}


void MonitorCmd::ShowRegs(const address_t *regs)
{
	MonitorStream strm;

	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			int k = j * 4 + i;
			strm << (j == 0 ? "    " : " ")
				<< reg_name(static_cast<msp430_reg_t>(k)) << ": " << f::X<4>(regs[k]);
		}
		strm << '\n';
	}
}


int MonitorCmd::Regs(char **arg)
{
	address_t regs[DEVICE_NUM_REGS];

	(void)arg;

	if (!g_TapMcu.GetRegs(regs))
		return -1;

	/* Check for breakpoints */
	for (int i = 0; i < g_TapMcu.GetMaxBreakpoints(); i++)
	{
		const DeviceBreakpoint &bp = g_TapMcu.breakpoints_[BkptId(i)];

		if ((bp.enabled_)
			&& (bp.type_ == DeviceBpType::kBpTypeBreak)
			&& (bp.addr_ == regs[MSP430_REG_PC]))
			Trace() << "Breakpoint " << i << " triggered (0x" << f::X<4>(bp.addr_) << ")\n";
	}

	ShowRegs(regs);

	return 0;
}


int MonitorCmd::Reset(char **arg)
{
	(void)arg;

	return g_TapMcu.SoftReset();
}


enum EraseType
{
	kEraseAll,
	kEraseMain,
	kEraseSegment,
	kEraseInfoA,
	kEraseRange,
};


int MonitorCmd::Erase(char **arg)
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
		else if (!strcasecmp(type_text, "infoa"))
		{
			type = kEraseInfoA;
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

	if (!g_TapMcu.Halt())
		return -1;

	bool res = true;
	Trace() << "Erasing...\n";
	switch (type)
	{
	case kEraseSegment:
		res = g_TapMcu.EraseSegment(segment);
		break;
	case kEraseInfoA:
		res = g_TapMcu.EraseInfoA();
		break;
	case kEraseRange:
		res = g_TapMcu.EraseRange(segment, total_size);
		break;
	case kEraseAll:
		res = g_TapMcu.EraseAll();
		break;
	case kEraseMain:
	default:
		res = g_TapMcu.EraseMain();
	}
	return (res - 1);
}


int MonitorCmd::Run(char **arg)
{
	device_status_t status;
	address_t regs[DEVICE_NUM_REGS];

	(void)arg;

	if (!g_TapMcu.GetRegs(regs))
	{
		Error() << "warning: device: can't fetch registers\n";
	}
	else
	{
		int i;

		for (i = 0; i < g_TapMcu.GetMaxBreakpoints(); i++)
		{
			const DeviceBreakpoint *bp = &g_TapMcu.breakpoints_[BkptId(i)];

			if ((bp->enabled_)
				&& bp->type_ == DeviceBpType::kBpTypeBreak
				&& bp->addr_ == regs[MSP430_REG_PC])
				break;
		}

		if (i < g_TapMcu.GetMaxBreakpoints())
		{
			Trace() << "Stepping over breakpoint #" << i << " at 0x" << f::X<4>(regs[0]) << '\n';
			g_TapMcu.SingleStep();
		}
	}

	if (g_TapMcu.Run() < 0)
	{
		Error() << "run: failed to start CPU\n";
		return -1;
	}

	do
	{
		status = g_TapMcu.Poll();
	}
	while (status == DEVICE_STATUS_RUNNING);

	if (status == DEVICE_STATUS_INTR)
		Trace() << '\n';

	if (status == DEVICE_STATUS_ERROR)
		return -1;

	if (!g_TapMcu.Halt())
		return -1;

	return Regs(nullptr);
}


int MonitorCmd::Set(char **arg)
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

	if (!g_TapMcu.GetRegs(regs))
		return -1;
	regs[reg] = value;
	if (g_TapMcu.SetRegs(regs) < 0)
		return -1;

	ShowRegs(regs);
	return 0;
}


// ── Monitor menu (#46): version / speed / scan / chipinfo ────────────────────

int MonitorCmd::Version(char **arg)
{
	(void)arg;
	MonitorStream strm;
	strm << "Glossy MSP430 " GLOSSY_FW_VERSION "\n";
	strm << "transports: JTAG";
#if OPT_INCLUDE_SBW_TIM_
	strm << " SBW";
#endif
	strm << "\nactive transport: "
		<< (g_TapMcu.GetTransport() == TapMcu::Transport::kSbw ? "SBW" : "JTAG")
		<< "\n";
	return 0;
}


// Bus-speed grades, mapped to the per-transport rate constants from platform.h.
// The five entries match the BusSpeed enum order (kSlowest..kFastest).
struct SpeedGrade
{
	const char	*name;
	BusSpeed	grade;
	uint32_t	jtag_hz;
	uint32_t	sbw_hz;
};

// SBW rate constants only exist on targets that compile the SBW driver; when it
// is off the transport can never be SBW, so the column is unused — fall back to 0.
#if OPT_INCLUDE_SBW_TIM_
#	define GLOSSY_SBW_HZ(n)	SBW_Speed_##n
#else
#	define GLOSSY_SBW_HZ(n)	0u
#endif

static const SpeedGrade kSpeedGrades[] =
{
	{ "slowest", BusSpeed::kSlowest, JTCK_Speed_1, GLOSSY_SBW_HZ(1) },
	{ "slow",    BusSpeed::kSlow,    JTCK_Speed_2, GLOSSY_SBW_HZ(2) },
	{ "medium",  BusSpeed::kMedium,  JTCK_Speed_3, GLOSSY_SBW_HZ(3) },
	{ "fast",    BusSpeed::kFast,    JTCK_Speed_4, GLOSSY_SBW_HZ(4) },
	{ "fastest", BusSpeed::kFastest, JTCK_Speed_5, GLOSSY_SBW_HZ(5) },
};

#undef GLOSSY_SBW_HZ


// Look up a speed grade by name (case-insensitive). Returns nullptr if unknown.
static const SpeedGrade *find_speed_grade(const char *name)
{
	for (const SpeedGrade &g : kSpeedGrades)
		if (strcasecmp(name, g.name) == 0)
			return &g;
	return nullptr;
}


// Shared body for jtag_scan / sbw_scan: optionally latch a bus-speed grade,
// select the transport, (re)acquire the target, and return the one-line device
// summary to the GDB monitor reply.
//
// The optional speed argument replaces the old standalone 'speed' command: an
// operator probes link stability by re-running the scan at successive grades
// (e.g. "jtag_scan medium", "jtag_scan fast"). With no argument the current
// grade is kept (defaults to medium — see TapMcu::speed_).
static int monitor_scan(TapMcu::Transport t, const char *label, char **arg)
{
	const char *a = get_arg(arg);
	if (a != nullptr && *a != 0)
	{
		const SpeedGrade *g = find_speed_grade(a);
		if (g == nullptr)
		{
			MonitorStream() << label << ": unknown speed '" << a
				<< "' (try: slowest slow medium fast fastest)\n";
			return -1;
		}
		g_TapMcu.SetBusSpeed(g->grade);
	}

	if (!g_TapMcu.SetTransport(t))
	{
		// Transport selected but its driver isn't built into this target yet
		// (e.g. SBW on the BluePill variant — coming soon).
		MonitorStream() << label << ": not implemented in this build\n";
		return -1;
	}
	// A scan may switch transports or re-acquire, so drop any current session
	// first (the transport interface owns shared timers/DMA/GPIO).
	if (g_TapMcu.IsAttached())
		g_TapMcu.Close();

	const bool ok = g_TapMcu.Open();
	MonitorStream strm;	// constructed only now: Open() must not touch MonitorBuf
	if (!ok)
	{
		strm << label << ": no target found\n";
		return -1;
	}
	// Echo the grade the link actually came up at, so a stability sweep shows
	// which rate succeeded.
	const bool sbw = (t == TapMcu::Transport::kSbw);
	const BusSpeed cur = g_TapMcu.GetBusSpeed();
	for (const SpeedGrade &g : kSpeedGrades)
	{
		if (g.grade == cur)
		{
			const uint32_t hz = sbw ? g.sbw_hz : g.jtag_hz;
			strm << "speed: " << g.name << " (" << (hz / 1000) << " kHz)\n";
			break;
		}
	}
	g_TapMcu.PrintChipInfo(strm, /*full=*/false);
	return 0;
}


int MonitorCmd::JtagScan(char **arg)
{
	return monitor_scan(TapMcu::Transport::kJtag, "jtag_scan", arg);
}


int MonitorCmd::SbwScan(char **arg)
{
	return monitor_scan(TapMcu::Transport::kSbw, "sbw_scan", arg);
}


int MonitorCmd::ChipInfo(char **arg)
{
	(void)arg;
	// Post-connect command: chip_info_ is guaranteed loaded by the dispatcher's
	// state gate. Full dump = device line + memory map (GDB/MSP430 has no XML
	// memory map, so this is its substitute).
	MonitorStream strm;
	g_TapMcu.PrintChipInfo(strm, /*full=*/true);
	return 0;
}


int MonitorCmd::Power(char **arg)
{
	const char *a = get_arg(arg);
	MonitorStream strm;

	// "auto" on an adjustable-supply probe: capture V2REF and drive the rail from
	// it (valid reading copied/clamped, invalid → safe fallback). Sense-only and
	// fixed-supply probes fall through to the report-only branch below.
	if (strcasecmp(a, "auto") == 0 && TargetPower::HasDrive())
	{
		const uint32_t mv = TargetPower::AutoPower();
		strm << "power: auto -> driving " << mv << " mV\n";
		return 0;
	}

	// No arg or "auto": report the measured target voltage (and supply state).
	if (a == nullptr || *a == 0 || strcasecmp(a, "auto") == 0)
	{
		if (TargetPower::HasSense())
			strm << "target voltage: " << TargetPower::ReadMilliVolts() << " mV\n";
		else
			strm << "power: no voltage sense on this probe\n";
		if (TargetPower::HasDrive())
		{
			const uint32_t sp = TargetPower::DriveMilliVolts();
			if (sp)
				strm << "supply: driving " << sp << " mV\n";
			else
				strm << "supply: off\n";
		}
		else
			strm << "power: fixed supply (no controllable output)\n";
		return 0;
	}

	if (strcasecmp(a, "off") == 0)
	{
		if (!TargetPower::HasDrive())
		{
			strm << "power: fixed supply (cannot switch off)\n";
			return -1;
		}
		TargetPower::Off();
		strm << "power: off\n";
		return 0;
	}

	// Numeric: requested output in millivolts.
	uint32_t mv = 0;
	for (const char *p = a; *p; ++p)
	{
		if (*p < '0' || *p > '9')
		{
			strm << "power: expected a mV value, 'auto' or 'off'\n";
			return -1;
		}
		mv = mv * 10 + (uint32_t)(*p - '0');
	}
	if (!TargetPower::SetMilliVolts(mv))
	{
		strm << "power: this probe has a fixed 3.3 V supply (set unsupported)\n";
		return -1;
	}
	strm << "power set to " << mv << " mV\n";
	return 0;
}

