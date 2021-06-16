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
#include "reader.h"
#include "cmddb.h"

static int in_reader_loop;

static int do_command(char *arg, int interactive)
{
	const char *cmd_text;
	int len = strlen(arg);

	while (len && isspace(arg[len - 1]))
		len--;
	arg[len] = 0;

	cmd_text = get_arg(&arg);
	if (cmd_text)
	{
		struct cmddb_record cmd;
#if 0
		char translated[1024];
		if (translate_alias(cmd_text, arg,
				    translated, sizeof(translated)) < 0)
			return -1;

		arg = translated;
		cmd_text = get_arg(&arg);
#endif

		/* Allow ^[# to stash a command in history without
		 * attempting to execute */
		if (*cmd_text == '#')
			return 0;

		if (!cmddb_get(cmd_text, &cmd)) {
			int old = in_reader_loop;
			int ret;

			in_reader_loop = interactive;
			ret = cmd.func(&arg);
			in_reader_loop = old;

			return ret;
		}

		MonitorStream() << "unknown command: " << cmd_text << " (try \"help\")\n";
		return -1;
	}

	return 0;
}

int process_command(char *cmd)
{
	return do_command(cmd, 0);
}

