#pragma once


// Interactively parses elements
class Parser
{
public:
	struct SkipSpacesOnInit {};

	// A r/w buffer containing the string to be parsed
	Parser(char *buf);
	Parser(char *buf, SkipSpacesOnInit &);
	~Parser() { RestoreMem(); }
	// Restarts scanning from the begin
	void RestartScanner();
	// Checks if there are more tokens to be read
	bool HasMore() const { return (pos_ != NULL) && (GetCurChar() != 0);  }
	void SkipSpaces();
	uint32_t GetUint32(int base = 0);
	uint32_t GetHexLsb(uint8_t bytes = 4);
	// Returns the char that will be next parsed
	char GetCurChar() const { return mem_ ? mem_ : *pos_; }
	char *GetRawBuffer() const { return buf_; }
	char *GetRawData() const { return pos_; }
	char GetNextChar();
	char *GetNextArg(const char * delims);
	// Skips one char, typically after a switch case on the first
	Parser &SkipChars(int cnt);
	ALWAYS_INLINE Parser &SkipChar() { return SkipChars(1); }
	// Interpret arguments as a command line
	char *GetArg();
	// Fill buffer with hex stream
	uint32_t UnhexifyBufferAndReset();
	char *GetNextListArg() { return GetNextArg(";"); }

protected:
	ALWAYS_INLINE void RestoreMem()
	{
		if (mem_)
		{
			*pos_ = mem_;
			mem_ = 0;
		}
	}
protected:
	char * const buf_;
	char *pos_;
	char mem_;
};