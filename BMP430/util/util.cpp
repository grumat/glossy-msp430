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
#include <ctype.h>


char *get_arg(char **text)
{
	char *start;
	char *rewrite;
	char *end;
	int qstate = 0;
	int qval = 0;

	if (!text)
		return NULL;

	start = *text;
	while (*start && isspace(*start))
		start++;

	if (!*start)
		return NULL;

	/* We've found the start of the argument. Parse it. */
	end = start;
	rewrite = start;
	while (*end)
	{
		switch (qstate)
		{
		case 0: /* Bare */
			if (isspace(*end))
				goto out;
			else if (*end == '"')
				qstate = 1;
			else if (*end == '\'')
				qstate = 2;
			else
				*(rewrite++) = *end;
			break;

		case 1: /* In quotes */
			if (*end == '"')
				qstate = 0;
			else if (*end == '\'')
				qstate = 3;
			else if (*end == '\\')
				qstate = 4;
			else
				*(rewrite++) = *end;
			break;

		case 2: /* In quote (verbatim) */
		case 3:
			if (*end == '\'')
				qstate -= 2;
			else
				*(rewrite++) = *end;
			break;

		case 4: /* Backslash */
			if (*end == '\\')
				*(rewrite++) = '\\';
			else if (*end == '\'')
				*(rewrite++) = '\'';
			else if (*end == 'n')
				*(rewrite++) = '\n';
			else if (*end == 'r')
				*(rewrite++) = '\r';
			else if (*end == 't')
				*(rewrite++) = '\t';
			else if (*end >= '0' && *end <= '3')
			{
				qstate = 50;
				qval = *end - '0';
			}
			else if (*end == 'x')
			{
				qstate = 60;
				qval = 0;
			}
			else
				*(rewrite++) = *end;

			if (qstate == 4)
				qstate = 1;
			break;

		case 50: /* Octal */
		case 51:
			if (*end >= '0' && *end <= '7')
				qval = (qval << 3) | (*end - '0');

			if (qstate == 51)
			{
				*(rewrite++) = qval;
				qstate = 1;
			}
			else
			{
				qstate++;
			}
			break;

		case 60: /* Hex */
		case 61:
			if (isdigit(*end))
				qval = (qval << 4) | (*end - '0');
			else if (isupper(*end))
				qval = (qval << 4) | (*end - 'A' + 10);
			else if (islower(*end))
				qval = (qval << 4) | (*end - 'a' + 10);

			if (qstate == 61)
			{
				*(rewrite++) = qval;
				qstate = 1;
			}
			else
			{
				qstate++;
			}
			break;
		}
		end++;
	}
out:
	/* Leave the text pointer at the end of the next argument */
	while (*end && isspace(*end))
		end++;

	*rewrite = 0;
	*text = end;
	return start;
}


int hexval(int c)
{
	if(c >= 'a')
		return c - 'a' + 10;
	if (c >= 'A')
		return c - 'A' + 10;
	if (c >= '0')
		return c - '0';
	return 0;
}

