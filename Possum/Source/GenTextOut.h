#pragma once


#define TEXTSTREAM_MAXINDENT		255


class CGenTextOutput
{
	enum TEXTSTREAM_MODE
	{
		TEXTSTREAM_DECMODE = 0,
		TEXTSTREAM_HEXMODE,

		TEXTSTREAM_NUMMODES
	};

public:
	CGenTextOutput();
	CGenTextOutput(FILE *f);
	virtual ~CGenTextOutput();

	// Write a string to the stream without formatting; allows the string to be optionally repeated
	// a number of times
	void Print(const TCHAR *d, size_t number = 1);

	// Writes a string of formatted text to the stream (see the standard c printf documentation for usage)
	void PrintF(const TCHAR *format, ...);

	// Overloaded operators for writing certain types in the C++ fashion
	CGenTextOutput& operator << (int64_t  d);
	CGenTextOutput& operator << (uint64_t d);
	CGenTextOutput& operator << (int32_t  d);
	CGenTextOutput& operator << (uint32_t d);
	CGenTextOutput& operator << (DWORD  d);
	CGenTextOutput& operator << (int16_t  d);
	CGenTextOutput& operator << (uint16_t d);
	CGenTextOutput& operator << (int8_t   d);
	CGenTextOutput& operator << (uint8_t  d);
	CGenTextOutput& operator << (double d);
	CGenTextOutput& operator << (float d);
	CGenTextOutput& operator << (const TCHAR *d);

	// Begins the next line
	// Allows you to skip over a number of lines
	// Also allows you to optionally disable indenting
	void NextLine(size_t skip = 0, bool indent = true);

	// Indentation character functions
	// The indentation character is the character that will be inserted n times (where n is the
	// current indentation level) before each new line that is written.
	void SetIndentChar(TCHAR ch = _T('\t'));
	TCHAR GetIndentChar();

	// Indentation level increase
	void IncIndent(size_t inc = 1);

	// Indentation level decrease
	void DecIndent(size_t dec = 1);

	// Set the way in which integer-based types will be written to the stream
	// options are:
	//   TEXTSTREAM_DECMODE for decimal output (ie., 254 -> "254")
	//   TEXTSTREAM_HEXMODE for hexadecimal output (ie., 254 -> "0xFE")
	void SetIntegerMode(TEXTSTREAM_MODE mode = TEXTSTREAM_DECMODE);


protected:
	size_t indentlvl;
	TCHAR indentchar;

	UINT modeflags;
	tstring modestr;

	FILE *m_f;

};
