#pragma once
/*!
\file stream.h

Elements on this file allows for data formatting and output the result in a stream. It is usually 
far more efficient than printf() formatting.

Example:
\code
typedef SwoChannel<0> Debug_;
typedef SwoTraceSetup <SysClk, kAsynchronous, 720000, Debug_> SwoTrace;
// A stream object for the trace output
typedef OutStream<Debug_> Debug;

void MySystemInit()
{
	/// ...
	SwoTrace::Init();
	/// ...
}

void MyExampleFunction()
{
	using namespace f;
	Debug() << "My byte value is: 0x" << X<2>(myByte) << '\n'
		<< "And my memory size is " << K(ComputeMemorySize()) << '\n'
		<< "This shows another way to specify width: " << Xw(myByte, 2) << '\n'
		;
	// add more code
}
\endcode
*/

namespace Bmt
{

/// Stream format utilities
namespace f
{

/// POD data-type to format in Hex (produces optimal code)
template<const int W> 
struct X
{
	ALWAYS_INLINE X(const uint32_t n) : n_(n) {}
	const uint32_t n_;
};

/// Value and Width to format string in Hex (suboptimal)
struct Xw
{
	ALWAYS_INLINE Xw(const uint32_t n, const int w) : n_(n), w_(w) {}
	const uint32_t n_;
	const int w_;
};

/// POD data-type to format in Decimal with leading '0' (produces optimal code)
template<const int W>
struct N
{
	ALWAYS_INLINE N(const int32_t n) : n_(n) {}
	const int32_t n_;
};

/// Value and Width to format string in Decimal with leading '0' (suboptimal)
struct Nw
{
	ALWAYS_INLINE Nw(const int32_t n, const int w) : n_(n), w_(w) {}
	const int32_t n_;
	const int w_;
};

/// POD data-type to left/right align string (produces optimal code)
template<const int W>
struct S
{
	ALWAYS_INLINE S(const char *s) : s_(s) {}
	const char *s_;
};

/// Value and Width to left/right align string (suboptimal)
struct Sw
{
	ALWAYS_INLINE Sw(const char *s, const int w) : s_(s), w_(w) {}
	const char *s_;
	const int w_;
};

/// POD data-type to max string with ellipsis (produces optimal code)
template<const int W>
struct M
{
	ALWAYS_INLINE M(const char* s) : s_(s) {}
	const char* s_;
};

/// Value and Width to max string with ellipsis (suboptimal)
struct Mw
{
	ALWAYS_INLINE Mw(const char* s, const int w) : s_(s), w_(w) {}
	const char* s_;
	const int w_;
};

/// POD data-type to format value in byte units (KB, MB, GB, ...)
struct K
{
	ALWAYS_INLINE K(const uint32_t n) : n_(n) {}
	const uint32_t n_;
};

/// Put char function
typedef void (* PutC_Fn)(char ch);

// static helpers used to lower code footprint of templates

//! Write string to a static char target
void PutString(PutC_Fn, const char *);
//! Left or right align of string
void FormatString(PutC_Fn fn, const char *s, const int w);
//! Limit string to max width and add ellipsis
void MaxString(PutC_Fn fn, const char* s, const int w);
//! Format numbers with minimal width, padding with '0'
void FormatNum(PutC_Fn fn, int32_t v, const int w = 1, const int base = 10);
//! Format numbers with minimal width, padding with '0'
void FormatNum(PutC_Fn fn, uint32_t v, const int w = 1, const int base = 10);
//! Format numbers in KB, MB, GB
void FormatByte(PutC_Fn fn, uint32_t v);
//! On-stack short lived non recursive string filler
void SetInternalFiller(char *buf, size_t size);
//! PutC for short lived non recursive string filler
void SetInternalPutC(char ch);
}

// Forward declaration
template<const size_t W> class StringBuf;

/// Stream Core members. Use OutStream instead.
template <typename PutC>
class OutStream_
{
public:
	/// Self data-type
	typedef OutStream_<PutC> Self;

	/// Write char to the stream
	ALWAYS_INLINE Self operator <<(char ch) { if (PutC::kEnabled_) PutC::PutChar(ch); return *this;}
	/// Write a string to the stream
	ALWAYS_INLINE Self operator <<(const char *s)
	{
		// Conditional compilation
		if (PutC::kEnabled_)
			f::PutString(PutC::PutChar, s);
		return *this;
	}
	/// Formats a number and writes it to the stream
	ALWAYS_INLINE Self operator <<(int32_t n)
	{
		// Conditional compilation
		if (PutC::kEnabled_) 
			f::FormatNum(PutC::PutChar, (int32_t)n);
		return *this;
	}
	/// Formats a number and writes it to the stream
	ALWAYS_INLINE Self operator <<(int n)
	{
		// Conditional compilation
		if (PutC::kEnabled_)
			f::FormatNum(PutC::PutChar, (int32_t)n);
		return *this;
	}
	/// Formats a number and writes it to the stream
	ALWAYS_INLINE Self operator <<(uint32_t n)
	{
		// Conditional compilation
		if (PutC::kEnabled_)
			f::FormatNum(PutC::PutChar, n);
		return *this;
	}
	/// Formats a string and writes it to the stream
	ALWAYS_INLINE Self operator <<(f::Sw s)
	{
		// Conditional compilation
		if (PutC::kEnabled_)
			f::FormatString(PutC::PutChar, s.s_, s.w_);
		return *this;
	}
	/// Formats a string and writes it to the stream
	ALWAYS_INLINE Self operator <<(f::Mw s)
	{
		// Conditional compilation
		if (PutC::kEnabled_)
			f::MaxString(PutC::PutChar, s.s_, s.w_);
		return *this;
	}
	/// Formats an Hex number and writes it to the stream
	ALWAYS_INLINE Self operator <<(f::Xw n)
	{
		// Conditional compilation
		if (PutC::kEnabled_)
			f::FormatNum(PutC::PutChar, n.n_, n.w_, 16);
		return *this;
	}
	/// Formats a number with given width and writes it to the stream
	ALWAYS_INLINE Self operator <<(f::Nw n)
	{
		// Conditional compilation
		if (PutC::kEnabled_)
			f::FormatNum(PutC::PutChar, n.n_, n.w_);
		return *this;
	}
	/// Formats a byte size with units and writes it to the stream
	ALWAYS_INLINE Self operator <<(f::K n)
	{
		// Conditional compilation
		if (PutC::kEnabled_)
			f::FormatByte(PutC::PutChar, n.n_);
		return *this;
	}

	/// Formats a string for the given width and send it to the stream
	template <const int W> ALWAYS_INLINE Self operator <<(f::S<W> s)
	{
		// Conditional compilation
		if (PutC::kEnabled_)
			f::FormatString(PutC::PutChar, s.s_, W);
		return *this;
	}

	/// Formats a string for the given width and send it to the stream
	template <const int W> ALWAYS_INLINE Self operator <<(f::M<W> s)
	{
		// Conditional compilation
		if (PutC::kEnabled_)
			f::MaxString(PutC::PutChar, s.s_, W);
		return *this;
	}

	/// Formats an Hex value using the specified width and sends it to the stream
	template <const int W> ALWAYS_INLINE Self operator <<(f::X<W> n)
	{
		// Conditional compilation
		if (PutC::kEnabled_)
			f::FormatNum(PutC::PutChar, n.n_, W, 16);
		return *this;
	}

	/// Formats a number using a specified width and sends it to the stream
	template <const int W> ALWAYS_INLINE Self operator <<(f::N<W> n)
	{
		if (PutC::kEnabled_)
			f::FormatNum(PutC::PutChar, n.n_, W);
		return *this;
	}
};

/// A template class to create an output stream to the specified `putchar` implementation
/*!
A typical case that serves as `putchar` class is the SwoChannel<> template class.
An interesting feature here is that these classes implements a constant that control 
conditional compilation, called \b kEnabled_. Through this option compiler can wipe out 
all tracing code for a build other than the debug.

See the example:
\code
#ifdef _DEBUG
typedef SwoDummyChannel DebugStream;
#else
typedef SwoChannel<0> DebugStream;
#endif
typedef OutStream<DebugStream> Debug;

void MyTestFunc()
{
	// Compiler efficiently discards all code and data necessary for this 
	// trace message on a release build.
	Debug() << "This trace message disappears on release build!!!";
}
\endcode
*/
template <typename PutC>
class OutStream : public OutStream_<PutC>
{
public:
	//! ctor/dtor (run once per stream instance)
	ALWAYS_INLINE OutStream() { if (PutC::kEnabled_) PutC::Init(); }
	ALWAYS_INLINE ~OutStream() { if (PutC::kEnabled_) PutC::Flush(); }
};

class InternalFiller_
{
public:
	//! Controls binary code generation
	static constexpr bool kEnabled_ = true;

	ALWAYS_INLINE static void Init() { }
	ALWAYS_INLINE static void Flush() { }
	ALWAYS_INLINE static void PutChar(char ch) { f::SetInternalPutC(ch); }
};


//! Generic string buffer for formatting
template<const size_t W>
class StringBuf : public OutStream<InternalFiller_>
{
public:
	StringBuf() { f::SetInternalFiller(buf, W); }
	~StringBuf() { f::SetInternalFiller(NULL, 0); }

	ALWAYS_INLINE operator const char *() { return buf; }

private:
	char buf[W];
};


}	// namespace Bmt
