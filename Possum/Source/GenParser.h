#pragma once

class CGenParser
{
public:
	CGenParser();

	~CGenParser();

	enum TOKEN_TYPE
	{
		TT_NONE = 0,

		TT_IDENT,			// [a-z,A-Z,_]+[0-9,a-z,A-Z,_]*
		TT_STRING,			// ["'][...]*["']
		TT_NUMBER,			// [-][0-9]+[.][0-9]*
		TT_SYMBOL,			// [!@#$%^&*()`~-=+[]{}<>/?;':",.|\]
		TT_HEXNUMBER,		// [0][xX][0-9,a-f,A-F]+
		TT_SHORTCOMMENT,	// "//"[...]*[\n]
		TT_LONGCOMMENT,		// "/*"[...]*"*/"

		TT_NUMTOKENTYPES
	};

	void SetSourceData(const TCHAR *data, UINT datalen);

	bool NextToken();
	bool NextLine();
	bool ToEndOfLine(); // captures the data in the stream until the next detected EOL but does not move the current stream position

	TOKEN_TYPE GetCurrentTokenType();
	TCHAR *GetCurrentTokenString();

	bool IsToken(TCHAR *s, bool case_sensitive = false);

protected:
	TCHAR *m_data;
	UINT m_datalen;
	UINT m_pos;

	tstring m_curStr;
	TOKEN_TYPE m_curType;
};

