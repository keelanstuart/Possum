// **************************************************************
// Nucleus Source File
//
// Copyright © 2019-2026, Keelan Sturt

#pragma once

#include <NuRetCode.h>
#include <NuPlugin.h>


namespace nucleus
{

	class NUCLEUS_API IPluginManager
	{

	public:

		/// Discovers plug-ins matching the given filespec and can auto-activate them if desired
		virtual RetCode DiscoverPlugins(
#if defined(DEBUG)
			const TCHAR *filespec = _T("*.dnudll")
#else
			const TCHAR *filespec = _T("*.nudll")
#endif
			, bool auto_activate = true, size_t *numfound = nullptr) = NULL;

		/// Unloads a specific plug-in
		virtual RetCode UnloadPlugin(IPlugin *pplug) = NULL;

		/// Unloads all plug-ins
		virtual void UnloadAllPlugins() = NULL;

		/// Returns the number of plug-ins that were successfully loaded
		virtual size_t GetNumPlugins() = NULL;

		/// Returns an interface to a plug-in, given its index
		virtual IPlugin *GetPlugin(size_t idx) = NULL;

	};

};