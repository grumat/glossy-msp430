#include "otherlibs.h"
#include "stream.h"

namespace f
{


void PutString(PutC_Fn fn, const char *s)
{
	while (*s)
		(fn)(*s++);
}


void FormatString(PutC_Fn fn, const char *s, const int w)
{
	size_t len = strlen(s);
	if (w < 0)
	{
		int aw = -w;
		// '0' padding
		for (size_t l = len; l < aw; ++l)
			(fn)(' ');
	}
	// Flush digits
	while (*s)
		(fn)(*s++);
	if (w > 0)
	{
		// '0' padding
		for (size_t l = len; l < w; ++l)
			(fn)(' ');
	}
}


void MaxString(PutC_Fn fn, const char* s, const int w)
{
	int cnt = 0;
	// Flush digits
	while (*s)
	{
		if (cnt == w)
		{
			cnt = 3;
			while(cnt--)
				(fn)('.');
			break;
		}
		(fn)(*s++);
		++cnt;
	}
}


void FormatNum(PutC_Fn fn, int32_t v, const int w, const int base)
{
	char buf[16];
	__itoa(v, buf, base);
	if (w > 1)
	{
		// '0' padding
		for (size_t l = strlen(buf); l < w; ++l)
			(fn)('0');
	}
	// Flush digits
	const char *s = buf;
	while (*s)
		(fn)(*s++);
}


void FormatNum(PutC_Fn fn, uint32_t v, const int w, const int base)
{
	char buf[16];
	__utoa(v, buf, base);
	if (w > 1)
	{
		// '0' padding
		for (size_t l = strlen(buf); l < w; ++l)
			(fn)('0');
	}
	// Flush digits
	const char *s = buf;
	while (*s)
		(fn)(*s++);
}


void FormatByte(PutC_Fn fn, uint32_t v)
{
	const char *scale = NULL;
	char buf[16];
	if (v >= (1024 * 1024 * 1024))
	{
		v /= (1024 * 1024);	// bit rotate
		v = 10 * v / 1024;
		scale = " GB";
	}
	else if (v >= (1024 * 1024))
	{
		v /= 1024;		// bit rotate
		scale = " MB";
	}
	else if (v > 2000)
	{
		scale = " KB";
	}
	if(scale)
		v = 10 * v / 1024;
	// scale == NULL for bytes
	__utoa(v, buf, 10);
	size_t l = strlen(buf);
	// Flush digits
	if (l == 1)
	{
		// Single digit case
		if (scale && buf[0] != '0')
		{
			// Prefix 
			(fn)('0');
			(fn)('.');
		}
		else
			scale = " B";
		(fn)(buf[0]);
	}
	else
	{
		// Print first digits and stop at the last
		const char *s = buf;
		while (*s)
		{
			if (l == 1)
				break;
			(fn)(*s++);
			--l;
		}
		// individual bytes?
		if (scale)
		{
			// Emulates decimal digit
			if (*s != '0')
			{
				(fn)('.');
				(fn)(*s);
			}
		}
		else
		{
			(fn)(*s);
			scale = " B";
		}
	}
	// Send scale
	while (*scale)
		(fn)(*scale++);
}


static char *ptr_;
static size_t cnt_;
static size_t max_;
void SetInternalFiller(char *buf, size_t size)
{
	// recursive call not supported
	assert(
		((ptr_ == NULL) && (buf != NULL))
		|| ((ptr_ != NULL) && (buf == NULL))
	);
	ptr_ = buf;
	cnt_ = 0;
	max_ = size;
}


void SetInternalPutC(char ch)
{
	if (ptr_ && cnt_ < max_)
	{
		ptr_[cnt_++] = ch;
		ptr_[cnt_] = 0;
	}
}


}	// namespace f

