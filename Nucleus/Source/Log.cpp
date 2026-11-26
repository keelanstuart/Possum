// **************************************************************
// Nucleus Source File
//
// Copyright © 2019-2026, Keelan Sturt


#include "stdafx.h"

#include "LogPrivate.h"


CLog::CLog()
{
	m_File = NULL;
	m_pcbRedirect = NULL;
	m_pUserData = NULL;
}

CLog::~CLog()
{
	SetLogFile(NULL);
}

nucleus::RetCode CLog::SetLogFile(const TCHAR *filename)
{
	if (m_File)
	{
		fflush(m_File);
		fclose(m_File);
		m_File = NULL;
	}

	if (filename)
	{
		m_Filename = filename;
		_tfopen_s(&m_File, filename, _T("at"));
	}

	if (m_File)
		return nucleus::RetCode::OK;

	return nucleus::RetCode::UNKNOWN_ERROR;
}

nucleus::RetCode CLog::SetRedirectFunction(LogRedirectFunction lrf)
{
	m_pcbRedirect = lrf;

	return nucleus::RetCode::OK;
}

void CLog::Reset()
{
	if (m_File)
	{
		fflush(m_File);
		fclose(m_File);

		m_File = NULL;

		DeleteFile(m_Filename.c_str());

		_tfopen_s(&m_File, m_Filename.c_str(), _T("wt"));
	}
}

void CLog::Log(const TCHAR *format, ...)
{
	TCHAR buf[1024];	// Temporary buffer for output

	va_list marker;
	va_start(marker, format);
	_vsntprintf_s(buf, 1024, (sizeof(buf) / sizeof(TCHAR)) - 1, format, marker);

	// this writes the message to the log file...
	if (m_File)
	{
		_ftprintf_s(m_File, buf);
		fflush(m_File);
	}

	if (m_pcbRedirect)
		m_pcbRedirect(buf);

	_RPT0(_CRT_WARN, buf);
}
