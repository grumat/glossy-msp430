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

#include "util/dis.h"
#include "util/output_util.h"
#include "vector.h"


static const char *reg_name(const msp430_reg_t reg)
{
	const char *name = dis_reg_name(reg);

	if (!name)
		return "???";
	return name;
}


void show_regs(const address_t *regs)
{
	int i;
	MonitorStream strm;

	for (i = 0; i < 4; i++)
	{
		int j;

		for (j = 0; j < 4; j++)
		{
			int k = j * 4 + i;
			if(j == 0)
				strm << "    ";
			else
				strm << ' ';

			strm << reg_name((msp430_reg_t)k) << ": " << f::X<4>(regs[k]);
		}

		strm << '\n';
	}
}


/************************************************************************
 * Name lists
 */

static int namelist_cmp(const void *a, const void *b)
{
	return strcasecmp(*(const char **)a, *(const char **)b);
}

void namelist_print(struct vector *v)
{
	int i;
	int max_len = 0;
	int rows, cols;
	MonitorStream strm;

	qsort(v->ptr, v->size, v->elemsize, namelist_cmp);

	for (i = 0; i < v->size; i++) {
		const char *text = VECTOR_AT(*v, i, const char *);
		int len = strlen(text);

		if (len > max_len)
			max_len = len;
	}

	max_len += 2;
	cols = 72 / max_len;
	rows = (v->size + cols - 1) / cols;

	for (i = 0; i < rows; i++) {
		int j;

		strm << "    ";
		for (j = 0; j < cols; j++) {
			int k = j * rows + i;
			const char *text;

			if (k >= v->size)
				break;

			text = VECTOR_AT(*v, k, const char *);
			strm << text;
			for (k = strlen(text); k < max_len; k++)
				strm << ' ';
		}
		strm << '\n';
	}
}
