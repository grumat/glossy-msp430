/* MSPDebug - debugging tool for MSP430 MCUs
 * Copyright (C) 2009, 2010 Daniel Beer
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "stdproj.h"

#include "devcmd.h"
#include "cmddb.h"


const struct cmddb_record commands[] = {
	{
		.name = "help",
		.func = cmd_help,
		.help =
"help [command]\n"
"    Without arguments, displays a list of commands. With a command\n"
"    name as an argument, displays help for that command.\n",
		.brief = "list commands, or show help for one",
	},
	{
		.name = "regs",
		.func = cmd_regs,
		.help =
"regs\n"
"    Read and display the current register contents.\n",
		.brief = "show CPU registers",
		.states = kCmdPost,
	},
	{
		.name = "reset",
		.func = cmd_reset,
		.help =
 "reset\n"
 "    Reset (and halt) the CPU.\n",
		.brief = "reset and halt the CPU",
		.states = kCmdPost,
	},
	{
		.name = "erase",
		.func = cmd_erase,
		.help =
"erase [all|segment] [address]\n"
"erase segrange <address> <size> <seg-size>\n"
"erase infoa\n"
"    Erase the device under test. With no arguments, erases all of main\n"
"    memory. Specify arguments to perform a mass erase, clear protected\n"
"    INFO A or to erase individual segments. The \"segrange\" mode is used\n"
"    to erase an address range via a series of segment erases.\n",
		.brief = "erase flash (all/segment/range)",
		.states = kCmdPost,
	},
	{
		.name = "run",
		.func = cmd_run,
		.help =
"run\n"
"    Run the CPU to until a breakpoint is reached or the command is\n"
"    interrupted.\n",
		.brief = "run until breakpoint or interrupt",
		.states = kCmdPost,
	},
	{
		.name = "set",
		.func = cmd_set,
		.help =
"set <register> <value>\n"
"    Change the value of a CPU register.\n",
		.brief = "set a CPU register value",
		.states = kCmdPost,
	},
	{
		.name = "version",
		.func = cmd_version,
		.help =
"version\n"
"    Show firmware version, compiled transports and the active transport.\n",
		.brief = "show firmware version and transports",
	},
	{
		.name = "jtag_scan",
		.func = cmd_jtag_scan,
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
		.func = cmd_sbw_scan,
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
		.func = cmd_chipinfo,
		.help =
"chipinfo\n"
"    Show the connected device and its full memory map (the GDB/MSP430\n"
"    substitute for an ARM-style memory-map XML).\n",
		.brief = "show device and memory map",
		.states = kCmdPost,
	},
	{
		.name = "power",
		.func = cmd_power,
		.help =
"power [auto|off|<millivolts>]\n"
"    Without arguments (or 'auto'), report the measured target voltage.\n"
"    With a value, set the target supply on probes with a controllable\n"
"    output; 'off' removes it. Fixed-supply probes report sense only.\n",
		.brief = "read or set target supply voltage",
	},
};

int cmddb_get(const char *name, struct cmddb_record *ret)
{
	int len = strlen(name);
	int i;
	const struct cmddb_record *found = NULL;

	/* First look for an exact match */
	for (i = 0; i < ARRAY_LEN(commands); i++) {
		const struct cmddb_record *r = &commands[i];

		if (!strcasecmp(r->name, name)) {
			found = r;
			goto done;
		}
	}

	/* Allow partial matches if unambiguous */
	for (i = 0; i < ARRAY_LEN(commands); i++) {
		const struct cmddb_record *r = &commands[i];

		if (!strncasecmp(r->name, name, len)) {
			if (found)
				return -1;
			found = r;
		}
	}

	if (!found)
		return -1;

done:
	memcpy(ret, found, sizeof(*ret));
	return 0;
}

int cmddb_enum(cmddb_enum_func_t func, void *user_data)
{
	int i;

	for (i = 0; i < ARRAY_LEN(commands); i++)
		if (func(user_data, &commands[i]) < 0)
			return -1;

	return 0;
}
