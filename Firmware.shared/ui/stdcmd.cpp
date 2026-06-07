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

#include "stdcmd.h"
#include "cmddb.h"


// Context for the grouped 'help' listing: print every command whose 'states'
// field exactly equals 'match', through the single shared MonitorStream (a fresh
// MonitorStream would reset the shared buffer — see OutStream ctor).
struct help_group_ctx
{
	MonitorStream	*strm;
	uint8_t			match;
};

static int help_print_group(void *user_data, const struct cmddb_record *rec)
{
	const help_group_ctx *c = (const help_group_ctx *)user_data;
	if (rec->states == c->match)
		*c->strm << "  " << rec->name << '\n';
	return 0;
}

int cmd_help(char **arg)
{
	const char *topic = get_arg(arg);
	MonitorStream strm;

	if (topic)
	{
		struct cmddb_record cmd;
		//struct opdb_key key;

		if (!cmddb_get(topic, &cmd))
		{
			strm << "COMMAND: " << cmd.name << "\n\n" << cmd.help << '\n';
			return 0;
		}

		Error() << "help: unknown command: " << topic << '\n';
		return -1;
	}
	else
	{
		// List commands grouped by the connection state in which they are
		// valid (see CmdState in cmddb.h): "any" (both), pre-connect only, and
		// post-connect only. All output goes through the one 'strm' instance.
		strm << "Available commands:\n";

		help_group_ctx any_grp  = { &strm, kCmdAny };
		help_group_ctx pre_grp  = { &strm, kCmdPre };
		help_group_ctx post_grp = { &strm, kCmdPost };

		strm << "\n any state:\n";
		cmddb_enum(help_print_group, &any_grp);
		strm << "\n before connect:\n";
		cmddb_enum(help_print_group, &pre_grp);
		strm << "\n after connect (target attached):\n";
		cmddb_enum(help_print_group, &post_grp);

		strm << "\nType \"help <topic>\" for more information.\n";
	}

	return 0;
}

