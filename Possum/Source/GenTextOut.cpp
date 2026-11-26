#include "stdafx.h"
#include <stdio.h>
#include "GenTextOut.h"


// ************************************************************************
// Text Output Stream Methods

CGenTextOutput::CGenTextOutput()
{
	m_f = nullptr;
	indentchar = _T('\t');
	indentlvl = 0;

	modestr = _T("%d");
}


CGenTextOutput::CGenTextOutput(FILE *f)
{
	m_f = f;
	indentchar = _T('\t');
	indentlvl = 0;

	modestr = _T("%d");
}


CGenTextOutput::~CGenTextOutput()
{
}


void CGenTextOutput::NextLine(size_t skip, bool indent)
{
	// end the current line...
	PrintF(_T("\n"));

	// then skip over the specified number of lines...
	while (skip)
	{
		PrintF(_T("\n"));
		skip--;
	}

	// then indent that line...
	if (indent)
		for (size_t i = 0; i < indentlvl; i++)
			PrintF(_T("%c"), indentchar);
}


void CGenTextOutput::SetIndentChar(TCHAR ch)
{
	indentchar = ch;
}


TCHAR CGenTextOutput::GetIndentChar()
{
	return indentchar;
}


void CGenTextOutput::IncIndent(size_t inc)
{
	indentlvl += inc;
	if (indentlvl > TEXTSTREAM_MAXINDENT)
		indentlvl = TEXTSTREAM_MAXINDENT;
}


void CGenTextOutput::DecIndent(size_t dec)
{
	if (dec <= indentlvl)
		indentlvl -= dec;
}


void CGenTextOutput::SetIntegerMode(TEXTSTREAM_MODE mode)
{
	switch(mode)
	{
		case TEXTSTREAM_HEXMODE:
			modestr = _T("%x");
			break;

		case TEXTSTREAM_DECMODE:
		default:
			modestr = _T("%I64d");
			break;
	}
}


void CGenTextOutput::Print(const TCHAR *d, size_t number)
{
	if (!m_f)
		return;

	while (number)
	{
		PrintF(_T("%s"), d);
		number--;
	}
}


void CGenTextOutput::PrintF(const TCHAR *format, ...)
{
	if (!m_f)
		return;

	va_list marker;
	va_start(marker, format);
	_vftprintf(m_f, format, marker);
}

CGenTextOutput& CGenTextOutput::operator << (int64_t d)
{
	PrintF(modestr.c_str(), d);

	return *this;
}


CGenTextOutput& CGenTextOutput::operator << (uint64_t d)
{
	PrintF(modestr.c_str(), d);

	return *this;
}


CGenTextOutput& CGenTextOutput::operator << (int32_t d)
{
	PrintF(modestr.c_str(), int64_t(d));

	return *this;
}


CGenTextOutput& CGenTextOutput::operator << (uint32_t d)
{
	PrintF(modestr.c_str(), uint64_t(d));

	return *this;
}


CGenTextOutput& CGenTextOutput::operator << (DWORD d)
{
	PrintF(modestr.c_str(), uint64_t(d));

	return *this;
}


CGenTextOutput& CGenTextOutput::operator << (int16_t d)
{
	PrintF(modestr.c_str(), int64_t(d));

	return *this;
}


CGenTextOutput& CGenTextOutput::operator << (uint16_t d)
{
	PrintF(modestr.c_str(), uint64_t(d));

	return *this;
}


CGenTextOutput& CGenTextOutput::operator << (int8_t d)
{
	PrintF(modestr.c_str(), int64_t(d));

	return *this;
}


CGenTextOutput& CGenTextOutput::operator << (uint8_t d)
{
	PrintF(modestr.c_str(), uint64_t(d));

	return *this;
}


CGenTextOutput& CGenTextOutput::operator << (double d)
{
	PrintF(_T("%f"), d);

	return *this;
}


CGenTextOutput& CGenTextOutput::operator << (float d)
{
	PrintF(_T("%f"), d);

	return *this;
}


CGenTextOutput& CGenTextOutput::operator << (const TCHAR *d)
{
	PrintF(_T("\"%s\""), d);

	return *this;
}
