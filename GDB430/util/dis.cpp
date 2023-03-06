/* MSPDebug - debugging tool for the eZ430
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

#include "dis.h"


static const char *const msp430_reg_names[] = {
	"Gpio::PC",  "SP",  "SR",  "R3",
	"R4",  "R5",  "R6",  "R7",
	"R8",  "R9",  "R10", "R11",
	"R12", "R13", "R14", "R15"
};

int dis_reg_from_name(const char *name)
{
	int i;

	if (!strcasecmp(name, "pc"))
		return 0;
	if (!strcasecmp(name, "sp"))
		return 1;
	if (!strcasecmp(name, "sr"))
		return 2;

	if (toupper(*name) == 'R')
		name++;

	for (i = 0; name[i]; i++)
		if (!isdigit(name[i]))
			return -1;

	i = atoi(name);
	if (i < 0 || i > 15)
		return -1;

	return i;
}

const char *dis_reg_name(msp430_reg_t reg)
{
	if (reg <= 15)
		return msp430_reg_names[reg];

	return NULL;
}
