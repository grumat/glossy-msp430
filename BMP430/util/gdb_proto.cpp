/* MSPDebug - debugging tool for MSP430 MCUs
 * Copyright (C) 2009-2011 Daniel Beer
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
#include <stdarg.h>
#include <stdio.h>

#include "gdb_proto.h"
#include "util.h"


void GdbData::AppendData(const void *buf, size_t len)
{
	const uint8_t *b = (uint8_t *)buf;
	for (size_t i = 0; i < len; ++i)
		*this << f::X<2>(b[i]);
}


void GdbData::MakeCheckSum()
{
	uint8_t c = GdbOutBuffer::GetCheckSum();
	GdbOutStream() << '#' << f::X<2>(c);
}


int GdbData::FlushAck()
{
	int c;

	MakeCheckSum();
	GdbOutBuffer::outbuf_[GdbOutBuffer::outlen_] = 0;
	Debug() << "-> " << GdbOutBuffer::outbuf_ << '\n';

	do
	{
		gUartGdb.PutBuf(GdbOutBuffer::outbuf_, GdbOutBuffer::outlen_);

		StopWatch sw;
		do
		{
			if (sw.GetEllapsedTime() > 5000)
				return -1;
			c = gUartGdb.GetChar();
		}
		while (c != '+' && c != '-');
	}
	while (c != '+');

	GdbOutBuffer::outlen_ = 0;
	return 0;
}


int GdbData::Send(const char *msg)
{
	GdbData response;
	GdbOutStream() << msg;
	return response.FlushAck();
}


static int GetChar()
{
	StopWatch sw;
	do
	{
		int ch = gUartGdb.GetChar();
		if (ch >= 0)
			return ch;
	}
	while (sw.GetEllapsedTime() < 200);
	return -1;
}

int gdb_read_packet(char *buf)
{
	int c;
	int len = 0;
	int cksum_calc = 0;
	int cksum_recv = 0;

	/* Wait for packet start */
	do
	{
		c = GetChar();
		if (c < 0)
			return 0;
	}
	while (c != '$');


	/* Read packet payload */
	while (len + 1 < GDB_BUF_SIZE)
	{
		c = GetChar();
		if (c < 0)
			goto bad_packet;
		if (c == '#')
			break;

		buf[len++] = c;
		cksum_calc = (cksum_calc + c) & 0xff;
	}
	buf[len] = 0;

	/* Read packet checksum */
	c = GetChar();
	if (c < 0)
		goto bad_packet;
	cksum_recv = hexval(c);
	c = GetChar();
	if (c < 0)
		goto bad_packet;
	cksum_recv = (cksum_recv << 4) | hexval(c);

	Debug() << "<- $" << buf << '#' << f::X<2>(cksum_recv) << '\n';

	if (cksum_recv != cksum_calc)
	{
bad_packet:
		Error() << "gdb: bad checksum (calc = 0x" << f::X<2>(cksum_calc)
			<< ", recv = 0x" << f::X<2>(cksum_recv) << ")\n";
		Error() << "gdb: packet data was: " << buf << '\n';
		gUartGdb.PutChar('-');
		return 0;
	}

	/* Send acknowledgement */
	gUartGdb.PutChar('+');
	return len;
}
