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
#include "util/vector.h"


static int push_command_name(void *user_data, const struct cmddb_record *rec)
{
	return vector_push((struct vector *)user_data, &rec->name, 1);
}

int cmd_help(char **arg)
{
	const char *topic = get_arg(arg);
	MonitorStream strm;

	if (topic)
	{
		struct cmddb_record cmd;
		//struct opdb_key key;

		if (!cmddb_get(topic, &cmd)) {
			strm << "COMMAND: " << cmd.name << "\n\n" << cmd.help << '\n';
			return 0;
		}

		Error() << "help: unknown command: " << topic << '\n';
		return -1;
	} else {
		struct vector v;

		vector_init(&v, sizeof(const char *));

		if (!cmddb_enum(push_command_name, &v))
		{
			strm << "Available commands:\n";
			namelist_print(&v);
			strm << '\n';
		}
		else
		{
			strm << "help: can't allocate memory for command list";
		}

		vector_destroy(&v);

		strm << "Type \"help <topic>\" for more information.\n";
	}

	return 0;
}

