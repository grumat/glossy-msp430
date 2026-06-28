#pragma once

#include "util/util.h"

// GDB protocol tracing (OPT_TRACE_GDB_PROTO) is a diagnostic toggle; its default
// lives with the other OPT_* switches in stdproj.h so a target's platform.h can
// override it through the established configuration mechanism.

//! Largest GDB packet payload we accept/emit. The "PacketSize" advertised in
//! qSupported is GDB_MAX_XFER * 2 because each payload byte hex-encodes to two
//! characters on the wire.
static constexpr unsigned GDB_MAX_XFER = 1300;
//! Size of the shared GDB frame buffer: payload hex-doubles on the wire (x2)
//! plus slack for '$', '#', the two checksum nibbles and RLE expansion headroom.
static constexpr unsigned GDB_BUF_SIZE = GDB_MAX_XFER * 2 + 64;

//! Single static byte buffer behind every GDB response, modelling the
//! OutStream<> sink concept (kEnabled_/Init/PutChar/Flush). On these
//! RAM-restricted targets (no heap) this one buffer is shared: GdbData::ReadPacket()
//! reads the *incoming* packet into it via GetDataBuffer() and the reply is then
//! built in the same storage. That RX/TX aliasing is a deliberate memory-saving
//! constraint — a command handler must finish parsing its arguments before it
//! constructs a GdbData reply, because GdbData's constructor resets this buffer.
class GdbOutBuffer
{
public:
	//! Controls binary code generation (OutStream<> sink concept).
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
		if (_countof(outbuf_) - outlen_ > 0)
			outbuf_[outlen_++] = ch;
	}
	static void EndOfRLE();
	static inline char outbuf_[GDB_BUF_SIZE];
	static inline int outlen_;
	static inline char last_char_;
	static inline int rle_cnt_;
};

using GdbOutStream = OutStream<GdbOutBuffer>;

//! One GDB response frame, built over the singleton GdbOutBuffer. Because that
//! backing buffer is a single shared static (see GdbOutBuffer), only one GdbData
//! frame may be alive at a time and the constructor resets the buffer — hence the
//! type is non-copyable to keep the "one live frame" contract compiler-checked.
class GdbData : public GdbOutStream
{
public:
	//! RSP acknowledgement state — the QStartNoAckMode handshake. GDB normally
	//! +/- acks every packet; a successful QStartNoAckMode turns acking off after
	//! one final acked reply. Underlying values are pinned so the zero-initialized
	//! power-on state is kDisabled (matching the pre-enum behaviour: silent until
	//! the first host '+').
	enum class AckMode : int8_t
	{
		kDisabled			=  0,	//!< no-ack mode: neither send nor await +/-
		kFinishThenDisable	=  1,	//!< ack this one last reply, then drop to kDisabled
		kEnabled			= -1,	//!< normal RSP: ack received packets and await the host's +/- for our replies
	};

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
	// Non-copyable: the backing buffer is a singleton, so a copy would alias the
	// same global storage and a second live frame would corrupt the first.
	GdbData(const GdbData &) = delete;
	GdbData &operator=(const GdbData &) = delete;
	//! Flushes the packet to the attached GDB instance
	int FlushAck();
	//! Sends binary data in GDB compatible format
	void AppendData(const void *buf, size_t len);

	//! Reads one GDB RSP packet into \p buf (the shared frame buffer; see
	//! GdbOutBuffer), validates the checksum and acks per ack_mode_. Returns the
	//! payload length, or 0 on timeout / bad packet. The RX counterpart to
	//! FlushAck().
	static int ReadPacket(char *buf);

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
	
	//! Public because Gdb's packet layer drives the QStartNoAckMode transitions.
	static AckMode ack_mode_;

protected:
	static int Send(SimpleResponse resp, const char *func = nullptr, const char *arg = nullptr);
	ALWAYS_INLINE static void PacketStart() { GdbOutBuffer::PutChar('$'); }
	void MakeCheckSum();
};
