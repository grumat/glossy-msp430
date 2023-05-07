#include "stdproj.h"
#include <stdarg.h>
#include <stdio.h>

#include "gdb_proto.h"
#include "util.h"


int GdbData::send_ack_;

void GdbOutBuffer::PutChar(char ch)
{
	if ((rle_cnt_ > 0) && (last_char_ != ch))
		EndOfRLE();
	last_char_ = ch;
	++rle_cnt_;
}


/*!
Response data can be run-length encoded to save space. Run-length encoding replaces runs of 
identical characters with one instance of the repeated character, followed by a '*' and a repeat 
count. The repeat count is itself sent encoded, to avoid binary characters in data: a value of n 
is sent as n+29. For a repeat count greater or equal to 3, this produces a printable ASCII 
character, e.g. a space (ASCII code 32) for a repeat count of 3. (This is because run-length 
encoding starts to win for counts 3 or more.) Thus, for example, "0* " is a run-length encoding 
of "0000": the space character after '*' means repeat the leading 0 32 - 29 = 3 more times.

The printable characters '#' and '$' or with a numeric value greater than 126 must not be used. 
Runs of six repeats ('#') or seven repeats ('$') can be expanded using a repeat count of only 
five ('"'). For example, "00000000" can be encoded as '0*"00'. 
*/
void GdbOutBuffer::EndOfRLE()
{
	static constexpr char kAsciiOffset = 29;	// offset for RLE chars
	static constexpr char kAsciiMax = '\x7e';	// ASCII 126
	
	/*
	** The printable characters '#' and '$' or with a numeric value greater than 126 must not 
	** be used.
	*/
	if (rle_cnt_ > 400)
		__NOP();
	
	// Output runs larger than 1+97 chars
	while (rle_cnt_ >= (kAsciiMax - kAsciiOffset + 1))
	{
		if (ARRAY_LEN(outbuf_) - outlen_ < 2)
			return;
		PutRawChar(last_char_);		// 1 + ...
		PutRawChar('*');
		PutRawChar(kAsciiMax);
		rle_cnt_ -= (1 + kAsciiMax - kAsciiOffset);
	}
	// Output runs shorter than 126 chars
	while (rle_cnt_ > 0)
	{
		PutRawCharSafe(last_char_);
		--rle_cnt_;
		// This is because run-length encoding starts to win for counts 3 or more.
		if (rle_cnt_ < 3)
			continue;
		else if (rle_cnt_ == ('#' - kAsciiOffset)
			|| rle_cnt_ == ('$' - kAsciiOffset))
		{
			/*
			** Runs of six repeats ('#') or seven repeats ('$') can be expanded using a repeat 
			** count of only five ('"').
			*/
			PutRawChar('*');
			PutRawChar('"');
			rle_cnt_ -= 5;
		}
		else
		{
			PutRawChar('*');
			PutRawChar((char)(rle_cnt_ + kAsciiOffset));
			rle_cnt_ = 0;
		}
	}
}


void GdbData::AppendData(const void *buf, size_t len)
{
	const uint8_t *b = (uint8_t *)buf;
	for (size_t i = 0; i < len; ++i)
		*this << f::X<2>(b[i]);
}


void GdbData::MakeCheckSum()
{
	// RLE inside the payload only
	GdbOutBuffer::EndOfRLE();
	uint8_t c = GdbOutBuffer::GetCheckSum();
	*this << '#' << f::X<2>(c);
}


int GdbData::FlushAck()
{
	int c;

	MakeCheckSum();
	// Flush RLE because it always keeps pending chars
	GdbOutBuffer::EndOfRLE();
	
	GdbOutBuffer::outbuf_[GdbOutBuffer::outlen_] = 0;
#if OPT_TRACE_GDB_PROTO
	Debug() << "-> " << f::M<80>(GdbOutBuffer::outbuf_) << '\n';
#endif

	do
	{
		gUartGdb.PutBuf(GdbOutBuffer::outbuf_, GdbOutBuffer::outlen_);
		// No ACK required?
		if (GdbData::send_ack_ == 0)
			break;

		StopWatch sw;
		do
		{
			if (sw.GetElapsedTicks() > TickTimer::M2T<5000>::kTicks)
				return -1;
			c = gUartGdb.GetChar();
		}
		while (c != '+' && c != '-');
	}
	while (c != '+');
	if (GdbData::send_ack_ > 0)
		GdbData::send_ack_ = 0;

	GdbOutBuffer::outlen_ = 0;
	return 0;
}


int GdbData::Send(const char *msg)
{
	GdbData response;
	response << msg;
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
	while (sw.GetElapsedTicks() < TickTimer::M2T<100>::kTicks);
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
		if (c == '+')
			GdbData::send_ack_ = -1;
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

#if OPT_TRACE_GDB_PROTO
	Debug() << "<- $" << f::M<80>(buf) << '#' << f::X<2>(cksum_recv) << '\n';
#endif

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
	if (GdbData::send_ack_ != 0)
		gUartGdb.PutChar('+');
	return len;
}
