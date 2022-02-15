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


int process_command(char *arg)
{
	// Remove trailing blanks
	int len = strlen(arg);
	while (len && isspace(arg[len - 1]))
		len--;
	arg[len] = 0;

	// Parse first argument
	const char *cmd_text = get_arg(&arg);
	if (cmd_text)
	{
		struct cmddb_record cmd;

		/* Allow ^[# to stash a command in history without
		 * attempting to execute */
		if (*cmd_text == '#')
			return 0;
		// Translate command to function pointer to be executed
		if (!cmddb_get(cmd_text, &cmd))
			return cmd.func(&arg);

		MonitorStream() << "unknown command: " << cmd_text << " (try \"help\")\n";
		return -1;
	}

	return 0;
}

