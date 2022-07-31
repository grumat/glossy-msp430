#include "stdproj.h"
#include <stdarg.h>
#include <stdio.h>

#include "gdb_proto.h"
#include "util.h"


void GdbData::AppendData(const void *buf, size_t len)
{
	const uint8_t *b = (uint8_t *)buf;
	for (size_t i = 0; i < len; ++i)
		GdbOutStream() << f::X<2>(b[i]);
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


int GdbData::Send(GdbData::SimpleResponse resp, const char *func, const char *arg)
{
	static const char *responses[] = 
	{
		"",
		"OK",
		"W00",		// kNotAttached
		"X1D",		// kNotAttached2	????
		"E3D",		// kMissingArg: ENODATA
		"E16",		// kInvalidArg: EINVAL
		"E05",		// kJtagError: EIO
	};

	if (func)
	{
		switch (resp)
		{
		case SimpleResponse::kUnsupported:
			if (func != NULL)
				Error() << func << ": unknown command: " << arg << '\n';
			break;
		case SimpleResponse::kInvalidArg:
			Error() << func << ": invalid argument: " << arg << '\n';
			break;
		case SimpleResponse::kNotAttached:
			Error() << func << ": MCU is not attached!\n";
			break;
		case SimpleResponse::kProcExit:
			Error() << func << ": Process exited!\n";
			break;
		case SimpleResponse::kJtagError:
			if(arg)
				Error() << func << ": JTAG Error: " << arg << '\n';
			else
				Error() << func << ": JTAG Error\n";
			break;
		}
	}
	static_assert(kLast_ == _countof(responses), "Response array count does not match to associated enum range");
	return Send(responses[resp]);
}


int GdbData::InvalidArg(const char *func, char ch)
{
	char buf[2];
	buf[0] = ch;
	buf[1] = 0;
	return Send(SimpleResponse::kInvalidArg, func, buf);
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
	while (sw.GetEllapsedTime() < 100);
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
