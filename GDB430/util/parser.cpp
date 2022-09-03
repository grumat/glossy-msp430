#include "stdproj.h"
#include "parser.h"
#include <ctype.h>
#include "util.h"


ALWAYS_INLINE char *SkipSpaces_(char *p)
{
	// Skip spaces
	while (*p && isspace(*p))
		++p;
	return p;
}

Parser::Parser(char *buf)
	: buf_(buf)
	, pos_(buf)
	, mem_(0)
{
	// Needs a string buffer to be parsed
	assert(buf != NULL);
}


Parser::Parser(char *buf, SkipSpacesOnInit&)
	: buf_(SkipSpaces_(buf))
	, pos_(buf_)
	, mem_(0)
{
	// Needs a string buffer to be parsed
	assert(buf != NULL);
}


void Parser::RestartScanner()
{
	RestoreMem();
	pos_ = buf_;
}


void Parser::SkipSpaces()
{
	// Skip spaces
	pos_ = SkipSpaces_(pos_);
}


/*!
This method reuses the parser buffer by decoding data in hex format and writing the binary
equivalent at the start of the buffer. Collisions will never happen because hex requires 
the double of space, so the scanner position runs twice as fast as the target index.
This cannot be said about "past bytes" which will partly overwritten, so caller needs to 
ensure that previous elements are decoded before calling this method.

\remark This method always skips the current parser position or preceding spaces.

\remark No further byte can be parsed after calling this method.
*/
uint32_t Parser::UnhexifyBufferAndReset()
{
	SkipSpaces();

	// Index to target position
	uint32_t len = 0;
	// Fill buffer, reserving one position for NUL terminator
	while (ishex(pos_[0]) && ishex(pos_[1]))
	{
		buf_[len++] = (hexval(pos_[0]) << 4) | hexval(pos_[1]);
		pos_ += 2;
	}
	buf_[len] = 0;
	pos_ = buf_;
	return len;
}


/*!
This method reuses the parser buffer by decoding data in escaped binary format and writing 
the unescaped binary equivalent at the start of the buffer. Collisions will never happen 
because commands have headers and arguments before the binary payload starts.
This cannot be said about "past bytes" which will partly overwritten, so caller needs to 
ensure that previous elements are decoded before calling this method.

\remark This method always skips the current parser position.

\remark No further byte can be parsed after calling this method.
*/
uint32_t Parser::UnescapeBinBufferAndReset()
{
	SkipChar();
	// Index to target position
	uint32_t len = 0;
	while (*pos_ != '#')
	{
		uint8_t ch = *pos_++;
		if (ch != '}')
			ch = *pos_++ ^ 0x20;
		buf_[len++] = ch;
	}
	buf_[len] = 0;
	pos_ = buf_;
	return len;
}


Parser &Parser::SkipChars(int cnt)
{
	RestoreMem();
	while(*pos_ && cnt > 0)
	{
		++pos_;
		--cnt;
	}
	return *this;
}


uint32_t Parser::GetUint32(int base)
{
	RestoreMem();
	// Skip spaces
	SkipSpaces();
	// parse value
	uint32_t r = strtoul(pos_, &pos_, base);
	// Skip spaces
	SkipSpaces();
	return r;
}


uint32_t Parser::GetHexLsb(uint8_t bytes)
{
	RestoreMem();
	// Skip spaces
	SkipSpaces();

	bytes *= 8;	// bytes to bits
	uint32_t val = 0;
	uint8_t shift = 0;
	// Parser register value
	while (*pos_ && shift < bytes)
	{
		// Hex digits allowed
		if (ishex(*pos_))
		{
			// compute byte value
			uint32_t v = (hexval(*pos_++) << 4) + hexval(*pos_++);
			// Value expressed in target order (LSB)
			val += (v << shift);
			shift += 8;
		}
		else
			break;
	}
	// Skip spaces
	SkipSpaces();
	return val;
}


char Parser::GetNextChar()
{
	RestoreMem();
	// Skip spaces
	SkipSpaces();
	char ch = *pos_;
	if (ch)
	{
		++pos_;
		// Skip spaces
		SkipSpaces();
	}
	return ch;
}


char *Parser::GetArg()
{
	RestoreMem();

	SkipSpaces();

	if (!*pos_)
		return NULL;

	/* We've found the start of the argument. Parse it. */
	char *start = pos_;
	char *rewrite = pos_;
	int qstate = 0;
	int qval = 0;
	while (*pos_)
	{
		switch (qstate)
		{
		case 0: /* Bare */
			if (isspace(*pos_))
				goto out;
			else if (*pos_ == '"')
				qstate = 1;
			else if (*pos_ == '\'')
				qstate = 2;
			else
				*(rewrite++) = *pos_;
			break;

		case 1: /* In quotes */
			if (*pos_ == '"')
				qstate = 0;
			else if (*pos_ == '\'')
				qstate = 3;
			else if (*pos_ == '\\')
				qstate = 4;
			else
				*(rewrite++) = *pos_;
			break;

		case 2: /* In quote (verbatim) */
		case 3:
			if (*pos_ == '\'')
				qstate -= 2;
			else
				*(rewrite++) = *pos_;
			break;

		case 4: /* Backslash */
			if (*pos_ == '\\')
				*(rewrite++) = '\\';
			else if (*pos_ == '\'')
				*(rewrite++) = '\'';
			else if (*pos_ == 'n')
				*(rewrite++) = '\n';
			else if (*pos_ == 'r')
				*(rewrite++) = '\r';
			else if (*pos_ == 't')
				*(rewrite++) = '\t';
			else if (*pos_ >= '0' && *pos_ <= '3')
			{
				qstate = 50;
				qval = *pos_ - '0';
			}
			else if (*pos_ == 'x')
			{
				qstate = 60;
				qval = 0;
			}
			else
				*(rewrite++) = *pos_;

			if (qstate == 4)
				qstate = 1;
			break;

		case 50: /* Octal */
		case 51:
			if (*pos_ >= '0' && *pos_ <= '7')
				qval = (qval << 3) | (*pos_ - '0');

			if (qstate == 51)
			{
				*(rewrite++) = qval;
				qstate = 1;
			}
			else
			{
				++qstate;
			}
			break;

		case 60: /* Hex */
		case 61:
			if (isdigit(*pos_))
				qval = (qval << 4) | (*pos_ - '0');
			else if (isupper(*pos_))
				qval = (qval << 4) | (*pos_ - 'A' + 10);
			else if (islower(*pos_))
				qval = (qval << 4) | (*pos_ - 'a' + 10);

			if (qstate == 61)
			{
				*(rewrite++) = qval;
				qstate = 1;
			}
			else
			{
				++qstate;
			}
			break;
		}
		++pos_;
	}
out:
	/* Leave the text pointer at the end of the next argument */
	SkipSpaces();

	mem_ = *pos_;	// the next statement potentially erases the char below pos_ pointer
	*rewrite = 0;
	return start;
}


char *Parser::GetNextArg(const char *delims)
{
	RestoreMem();

	if (!*pos_)
		return NULL;

	char *start = pos_;
	while (*pos_ && strchr(delims, *pos_) == 0)
		++pos_;
	mem_ = *pos_;
	*pos_ = 0;
	return start;
}

