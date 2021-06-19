#pragma once


namespace f
{

// POD data-type to format in Hex (produces optimal code)
template<const int W> 
struct X
{
	ALWAYS_INLINE X(uint32_t n) : n_(n) {}
	uint32_t n_;
};

// Value and Width to format string in Hex (suboptimal)
struct Xw
{
	ALWAYS_INLINE Xw(uint32_t n, int w) : n_(n), w_(w) {}
	uint32_t n_;
	int w_;
};

// POD data-type to format in Decimal with leading '0' (produces optimal code)
template<const int W>
struct N
{
	ALWAYS_INLINE N(int32_t n) : n_(n) {}
	int32_t n_;
};

// Value and Width to format string in Decimal with leading '0' (suboptimal)
struct Nw
{
	ALWAYS_INLINE Nw(int32_t n, int w) : n_(n), w_(w) {}
	int32_t n_;
	int w_;
};

// POD data-type to left/right align string (produces optimal code)
template<const int W>
struct S
{
	ALWAYS_INLINE S(const char *s) : s_(s) {}
	const char *s_;
};

// Value and Width to left/right align string (suboptimal)
struct Sw
{
	ALWAYS_INLINE Sw(const char *s, int w) : s_(s), w_(w) {}
	const char *s_;
	int w_;
};

struct K
{
	ALWAYS_INLINE K(uint32_t n) : n_(n) {}
	uint32_t n_;
};

typedef void (* PutC_Fn)(char ch);
typedef void (* PutS_Fn)(char ch);

// static code used to lower code footprint of templates

//! Write string to a static char target
void PutString(PutS_Fn, const char *);
//! Left or right align of string
void FormatString(PutC_Fn fn, const char *s, const int w);
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

template<const size_t W> class StringBuf;

template <typename PutC>
class OutStream
{
public:
	//! Self data-type
	typedef OutStream<PutC> Self;

	//! ctor
	ALWAYS_INLINE OutStream() { if(PutC::kEnabled_) PutC::Init(); }
	ALWAYS_INLINE ~OutStream() { if (PutC::kEnabled_) PutC::Flush(); }

	//! Write char to the stream
	ALWAYS_INLINE Self operator <<(char ch) { if (PutC::kEnabled_) PutC::PutChar(ch); return *this;}
	ALWAYS_INLINE Self operator <<(const char *s)
	{
		if (PutC::kEnabled_)
			f::PutString(PutC::PutChar, s);
		return *this;
	}
	ALWAYS_INLINE Self operator <<(int32_t n)
	{
		if (PutC::kEnabled_) 
			f::FormatNum(PutC::PutChar, (int32_t)n);
		return *this;
	}
	ALWAYS_INLINE Self operator <<(int n)
	{
		if (PutC::kEnabled_) 
			f::FormatNum(PutC::PutChar, (int32_t)n);
		return *this;
	}
	ALWAYS_INLINE Self operator <<(uint32_t n)
	{
		if (PutC::kEnabled_) 
			f::FormatNum(PutC::PutChar, n);
		return *this;
	}
	ALWAYS_INLINE Self operator <<(f::Sw s)
	{
		if (PutC::kEnabled_)
			f::FormatString(PutC::PutChar, s.s_, s.w_);
		return *this;
	}
	ALWAYS_INLINE Self operator <<(f::Xw n)
	{
		if (PutC::kEnabled_)
			f::FormatNum(PutC::PutChar, n.n_, n.w_, 16);
		return *this;
	}
	ALWAYS_INLINE Self operator <<(f::Nw n)
	{
		if (PutC::kEnabled_)
			f::FormatNum(PutC::PutChar, n.n_, n.w_);
		return *this;
	}
	ALWAYS_INLINE Self operator <<(f::K n)
	{
		if (PutC::kEnabled_)
			f::FormatByte(PutC::PutChar, n.n_);
		return *this;
	}

	template <const int W> ALWAYS_INLINE Self operator <<(f::S<W> s)
	{
		if (PutC::kEnabled_)
			f::FormatString(PutC::PutChar, s.s_, W);
		return *this;
	}

	template <const int W> ALWAYS_INLINE Self operator <<(f::X<W> n)
	{
		if (PutC::kEnabled_)
			f::FormatNum(PutC::PutChar, n.n_, W, 16);
		return *this;
	}

	template <const int W> ALWAYS_INLINE Self operator <<(f::N<W> n)
	{
		if (PutC::kEnabled_)
			f::FormatNum(PutC::PutChar, n.n_, W);
		return *this;
	}
};


class InternalFiller_
{
public:
	// ! Controls binary code generation
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

