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

#pragma once

#include "util/gdb_proto.h"


struct vector;


class MonitorBuf
{
public:
	// ! Controls binary code generation
	static constexpr bool kEnabled_ = true;

	ALWAYS_INLINE static void Init() { len = 0; }
	ALWAYS_INLINE static void Flush() { }
	//int	trunc;
	ALWAYS_INLINE static void PutChar(char ch)
	{
		if (len < GDB_MAX_XFER)
			buf[len++] = ch;
	}

	static inline char buf[GDB_MAX_XFER];
	static inline int len;
};

typedef OutStream<MonitorBuf> MonitorStream;


/* Colorized register dump */
void show_regs(const address_t *regs);

/* Name lists. This function is used for printing multi-column sorted
 * lists of constant strings. Expected is a vector of const char *.
 */
void namelist_print(struct vector *v);

