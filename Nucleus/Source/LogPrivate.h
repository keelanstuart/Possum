#pragma once

class CLog : public nucleus::ILog
{

public:

	CLog();
	virtual ~CLog();

	virtual nucleus::RetCode SetLogFile(const TCHAR *filename = NULL);
	virtual nucleus::RetCode SetRedirectFunction(LogRedirectFunction lrf);

	virtual void Reset();
	virtual void Log(const TCHAR *format, ...);

protected:

	FILE *m_File;
	tstring m_Filename;

	LogRedirectFunction m_pcbRedirect;
	void *m_pUserData;

};
