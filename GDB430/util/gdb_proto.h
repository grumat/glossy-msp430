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

#ifndef GDB_PROTO_H_
#define GDB_PROTO_H_

#include "util/util.h"

// Adds tracing for GDB protocol
#define OPT_TRACE_GDB_PROTO 0

#define GDB_MAX_XFER    1300
#define GDB_BUF_SIZE	(GDB_MAX_XFER * 2 + 64)

class GdbOutBuffer
{
public:
	// ! Controls binary code generation
	static constexpr bool kEnabled_ = true;

	// Empty placeholder to satisfy OutStream instances
	ALWAYS_INLINE static void Init() { }
	// Sends a byte, eventually applying RLE compression
	static void PutChar(char ch);
	// Empty placeholder to satisfy OutStream instances
	ALWAYS_INLINE static void Flush() { }
	// Access to data buffer
	ALWAYS_INLINE static char *GetDataBuffer() { return outbuf_; }

protected:
	friend class GdbData;

	ALWAYS_INLINE static void Reset()
	{
		outlen_ = 0;
		rle_cnt_ = 0;
	}
	ALWAYS_INLINE static int GetCheckSum()
	{
		int c = 0;
		for (int i = 1; i < outlen_; i++)
			c = (c + outbuf_[i]) & 0xff;
		return c;
	}
	ALWAYS_INLINE static void PutRawChar(char ch)
	{
		outbuf_[outlen_++] = ch;
	}
	ALWAYS_INLINE static void PutRawCharSafe(char ch)
	{
		if (ARRAY_LEN(outbuf_) - outlen_ > 0)
			outbuf_[outlen_++] = ch;
	}
	static void EndOfRLE();
	static inline char outbuf_[GDB_BUF_SIZE];
	static inline int outlen_;
	static inline char last_char_;
	static inline int rle_cnt_;
};

typedef OutStream<GdbOutBuffer> GdbOutStream;

class GdbData : public GdbOutStream
{
public:
	enum SimpleResponse
	{
		kUnsupported	// ""
		, kOk			// "OK"
		, kNotAttached	// "W00"
		, kProcExit		// "X1D"
		, kMissingArg	// "E3D"
		, kInvalidArg	// "E16"
		, kJtagError	// "E05"
		, kLast_
	};

public:
	// Resets buffer to start a new GDB response frame
	ALWAYS_INLINE GdbData()
	{
		GdbOutBuffer::Reset();
		PacketStart();
	}
	//! Flushes the packet to the attached GDB instance
	int FlushAck();
	//! Sends binary data in GDB compatible format
	void AppendData(const void *buf, size_t len);

public:
	static int Send(const char *msg);
	static int OK()
	{
		return Send(SimpleResponse::kOk);
	}
	static int Unsupported()
	{
		return Send(SimpleResponse::kUnsupported);
	}
	static int Unsupported(const char *func, const char *arg)
	{
		return Send(SimpleResponse::kUnsupported, func, arg);
	}
	static int MissingArg(const char *func)
	{
		return Send(SimpleResponse::kMissingArg, func);
	}
	static int InvalidArg(const char *func, const char *arg)
	{
		return Send(SimpleResponse::kInvalidArg, func, arg);
	}
	static int InvalidArg(const char *func, char ch);
	static int NotAttached(const char *func)
	{
		return Send(SimpleResponse::kNotAttached, func);
	}
	static int ProcExited(const char *func)
	{
		return Send(SimpleResponse::kProcExit, func);
	}
	static int ErrorJtag(const char *func)
	{
		return Send(SimpleResponse::kJtagError, func);
	}
	static int ErrorJtag(const char *func, const char *arg)
	{
		return Send(SimpleResponse::kJtagError, func, arg);
	}
	
	static int send_ack_;

protected:
	static int Send(SimpleResponse resp, const char *func = NULL, const char *arg = NULL);
	ALWAYS_INLINE static void PacketStart() { GdbOutBuffer::PutChar('$'); }
	void MakeCheckSum();
};


int gdb_read_packet(char *buf);

#endif
