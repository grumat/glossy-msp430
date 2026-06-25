#include "stdproj.h"

#include "MonitorCmd.h"
#include "CmdDb.h"


const CmdDb::Record CmdDb::kCommands_[] =
{
	{
		.name = "help",
		.func = MonitorCmd::Help,
		.help =
"help [command]\n"
"    Without arguments, displays a list of commands. With a command\n"
"    name as an argument, displays help for that command.\n",
		.brief = "list commands, or show help for one",
	},
	{
		.name = "regs",
		.func = MonitorCmd::Regs,
		.help =
"regs\n"
"    Read and display the current register contents.\n",
		.brief = "show CPU registers",
		.states = kPost,
	},
	{
		.name = "reset",
		.func = MonitorCmd::Reset,
		.help =
 "reset\n"
 "    Reset (and halt) the CPU.\n",
		.brief = "reset and halt the CPU",
		.states = kPost,
	},
	{
		.name = "erase",
		.func = MonitorCmd::Erase,
		.help =
"erase [all|segment] [address]\n"
"erase segrange <address> <size> <seg-size>\n"
"erase infoa\n"
"    Erase the device under test. With no arguments, erases all of main\n"
"    memory. Specify arguments to perform a mass erase, clear protected\n"
"    INFO A or to erase individual segments. The \"segrange\" mode is used\n"
"    to erase an address range via a series of segment erases.\n",
		.brief = "erase flash (all/segment/range)",
		.states = kPost,
	},
	{
		.name = "run",
		.func = MonitorCmd::Run,
		.help =
"run\n"
"    Run the CPU to until a breakpoint is reached or the command is\n"
"    interrupted.\n",
		.brief = "run until breakpoint or interrupt",
		.states = kPost,
	},
	{
		.name = "set",
		.func = MonitorCmd::Set,
		.help =
"set <register> <value>\n"
"    Change the value of a CPU register.\n",
		.brief = "set a CPU register value",
		.states = kPost,
	},
	{
		.name = "version",
		.func = MonitorCmd::Version,
		.help =
"version\n"
"    Show firmware version, compiled transports and the active transport.\n",
		.brief = "show firmware version and transports",
	},
	{
		.name = "jtag_scan",
		.func = MonitorCmd::JtagScan,
		.help =
"jtag_scan [slowest|slow|medium|fast|fastest]\n"
"    Select 4-wire JTAG and acquire the target. With an optional speed grade,\n"
"    latch that bus speed first (default medium); re-run at different grades\n"
"    to check link stability. On success, reports the speed used, the\n"
"    identified device and hardware-breakpoint count.\n",
		.brief = "acquire target over 4-wire JTAG [speed]",
	},
	{
		.name = "sbw_scan",
		.func = MonitorCmd::SbwScan,
		.help =
"sbw_scan [slowest|slow|medium|fast|fastest]\n"
"    Select 2-wire Spy-Bi-Wire and acquire the target. With an optional speed\n"
"    grade, latch that bus speed first (default medium); re-run at different\n"
"    grades to check link stability. On success, reports the speed used, the\n"
"    identified device and hardware-breakpoint count.\n",
		.brief = "acquire target over 2-wire SBW [speed]",
	},
	{
		.name = "chipinfo",
		.func = MonitorCmd::ChipInfo,
		.help =
"chipinfo\n"
"    Show the connected device and its full memory map (the GDB/MSP430\n"
"    substitute for an ARM-style memory-map XML).\n",
		.brief = "show device and memory map",
		.states = kPost,
	},
	{
		.name = "power",
		.func = MonitorCmd::Power,
		.help =
"power [auto|off|<millivolts>]\n"
"    Without arguments (or 'auto'), report the measured target voltage.\n"
"    With a value, set the target supply on probes with a controllable\n"
"    output; 'off' removes it. Fixed-supply probes report sense only.\n",
		.brief = "read or set target supply voltage",
	},
};


int CmdDb::Get(const char *name, Record *out)
{
	const Record *found = nullptr;

	// First look for an exact match.
	for (const Record &r : kCommands_)
	{
		if (!strcasecmp(r.name, name))
		{
			found = &r;
			break;
		}
	}

	// Otherwise allow a partial match, but only if it is unambiguous.
	if (found == nullptr)
	{
		const size_t len = strlen(name);
		for (const Record &r : kCommands_)
		{
			if (!strncasecmp(r.name, name, len))
			{
				if (found != nullptr)
					return -1;
				found = &r;
			}
		}
	}

	if (found == nullptr)
		return -1;
	
	*out = *found;
	return 0;
}


int CmdDb::Enum(EnumFunc func, void *user_data)
{
	for (const Record &r : kCommands_)
		if (func(user_data, &r) < 0)
			return -1;
	return 0;
}
