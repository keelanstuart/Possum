// **************************************************************
// Nucleus Source File
//
// Copyright © 2019-2026, Keelan Sturt

#pragma once

#include <NuRetCode.h>
#include <functional>


namespace nucleus
{

	class ILog
	{

	public:
		/// Implement a function following this prototype to facilitate custom message logging (to screen, for example)
		using LogRedirectFunction = std::function<void(const TCHAR *)>;

		/// Sets the file that messages are written to
		virtual RetCode SetLogFile(const TCHAR *filename = nullptr) = NULL;

		/// Sets a function that will be called back with a complete message when Log is called
		virtual RetCode SetRedirectFunction(LogRedirectFunction func) = NULL;

		/// Deletes the existing log file
		virtual void Reset() = NULL;

		/// Logs a message, uses the same mechanism as printf
		virtual void Log(const TCHAR *format, ...) = NULL;

	};

};