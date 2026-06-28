#pragma once

#include "util/GdbData.h"

//! Output sink for the GDB 'monitor' (qRcmd) console. Buffers up to one GDB
//! transfer's worth of text (GDB_MAX_XFER) that Gdb::MonitorCommand later wraps
//! into a GdbData response (see Gdb.cpp). Models the OutStream<> sink concept
//! (kEnabled_/Init/PutChar/Flush); like GdbOutBuffer it is a single static
//! buffer, so only one logical monitor frame is filled at a time.
class MonitorBuf
{
public:
	//! Controls binary code generation (OutStream<> sink concept).
	static constexpr bool kEnabled_ = true;

	ALWAYS_INLINE static void Init() { len = 0; }
	ALWAYS_INLINE static void Flush() { }
	ALWAYS_INLINE static void PutChar(char ch)
	{
		if (len < GDB_MAX_XFER)
			buf[len++] = ch;
	}

	static inline char buf[GDB_MAX_XFER];
	static inline int len;
};

using MonitorStream = OutStream<MonitorBuf>;
