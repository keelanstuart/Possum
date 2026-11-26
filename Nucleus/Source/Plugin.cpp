#include "stdafx.h"
#include "NuPlugin.h"
#include "NuPluginManager.h"
#include "NuRetCode.h"

#include "PluginPrivate.h"

#include <Shlwapi.h>
#include <io.h>


CPlugin::CPlugin(const TCHAR *filename, nucleus::INucleus *psys)
{
	m_pSys = psys;

	m_Filename = filename;

	m_hMod = NULL;
	m_bActive = false;

	m_fnGetName = nullptr;
	m_fnGetShortName = nullptr;
	m_fnGetPluginVersion = nullptr;
	m_fnGetAuthor = nullptr;
	m_fnGetWebsite = nullptr;
	m_fnGetDescription = nullptr;
	m_fnGetShortDescription = nullptr;
	m_fnGetCopyright = nullptr;
	m_fnActivate = nullptr;
	m_fnDeactivate = nullptr;

	if (PathFileExists(m_Filename.c_str()))
		m_hMod = LoadLibraryEx(m_Filename.c_str(), NULL, 0);

	if (m_hMod)
	{
		bool invalid = false;

		PLUGFUNC_GETNUCLEUSVERSION fnNucleusVersion = (PLUGFUNC_GETNUCLEUSVERSION)GetProcAddress(m_hMod, "NucleusPlugin_GetNucleusVersion");
		if (fnNucleusVersion)
		{
			uint64_t nucleus_version_that_plugin_expects = fnNucleusVersion();

			if (NUCLEUS_VERSION != nucleus_version_that_plugin_expects)
				invalid = true;
		}

		PLUGFUNC_GETPLATFORM fnGetPlatform = (PLUGFUNC_GETPLATFORM)GetProcAddress(m_hMod, "NucleusPlugin_GetPlatform");
		if (fnGetPlatform)
		{
			uint32_t platform_that_plugin_expects = fnGetPlatform();

#if defined(_M_X64)
			if (64 != platform_that_plugin_expects)
				invalid = true;
#else
			if (32 != platform_that_plugin_expects)
				invalid = true;
#endif
		}

		PLUGFUNC_ISUNICODE fnIsUnicode = (PLUGFUNC_ISUNICODE)GetProcAddress(m_hMod, "NucleusPlugin_IsUnicode");
		if (fnIsUnicode)
		{
			bool plugin_expects_unicode = fnIsUnicode();

#if defined(UNICODE)
			if (!plugin_expects_unicode)
				invalid = true;
#else
			if (plugin_expects_unicode)
				invalid = true;
#endif
		}

		if (invalid)
		{
			FreeLibrary(m_hMod);

			m_hMod = NULL;

			return;
		}

		m_fnGetName = (PLUGFUNC_GETNAME)GetProcAddress(m_hMod, "NucleusPlugin_GetName");
		m_fnGetShortName = (PLUGFUNC_GETSHORTNAME)GetProcAddress(m_hMod, "NucleusPlugin_GetShortName");
		m_fnGetPluginVersion = (PLUGFUNC_GETVERSION)GetProcAddress(m_hMod, "NucleusPlugin_GetPluginVersion");
		m_fnGetAuthor = (PLUGFUNC_GETAUTHOR)GetProcAddress(m_hMod, "NucleusPlugin_GetAuthor");
		m_fnGetWebsite = (PLUGFUNC_GETWEBSITE)GetProcAddress(m_hMod, "NucleusPlugin_GetWebsite");
		m_fnGetDescription = (PLUGFUNC_GETDESC)GetProcAddress(m_hMod, "NucleusPlugin_GetDescription");
		m_fnGetShortDescription = (PLUGFUNC_GETSHORTDESC)GetProcAddress(m_hMod, "NucleusPlugin_GetShortDescription");
		m_fnGetCopyright = (PLUGFUNC_GETCOPYRIGHT)GetProcAddress(m_hMod, "NucleusPlugin_GetCopyright");
		m_fnActivate = (PLUGFUNC_ACTIVATE)GetProcAddress(m_hMod, "NucleusPlugin_Activate");
		m_fnDeactivate = (PLUGFUNC_DEACTIVATE)GetProcAddress(m_hMod, "NucleusPlugin_Deactivate");
	}
}

CPlugin::~CPlugin()
{
	if (m_hMod)
		FreeLibrary(m_hMod);

	m_hMod = NULL;
	m_bActive = false;

	m_fnGetName = nullptr;
	m_fnGetShortName = nullptr;
	m_fnGetAuthor = nullptr;
	m_fnGetWebsite = nullptr;
	m_fnGetDescription = nullptr;
	m_fnGetShortDescription = nullptr;
	m_fnGetCopyright = nullptr;
	m_fnGetPluginVersion = nullptr;
	m_fnActivate = nullptr;
	m_fnDeactivate = nullptr;
}

const TCHAR *g_EmptyStr = _T("");

const TCHAR *CPlugin::GetFilename()
{
	return m_Filename.c_str();
}

const TCHAR *CPlugin::GetName()
{
	if (m_fnGetName)
		return m_fnGetName();

	return g_EmptyStr;
}

const TCHAR *CPlugin::GetShortName()
{
	if (m_fnGetShortName)
		return m_fnGetShortName();

	return g_EmptyStr;
}

const TCHAR *CPlugin::GetAuthor()
{
	if (m_fnGetAuthor)
		return m_fnGetAuthor();

	return g_EmptyStr;
}

const TCHAR *CPlugin::GetCopyright()
{
	if (m_fnGetCopyright)
		return m_fnGetCopyright();

	return g_EmptyStr;
}

const TCHAR *CPlugin::GetWebsite()
{
	if (m_fnGetWebsite)
		return m_fnGetWebsite();

	return g_EmptyStr;
}

const TCHAR *CPlugin::GetDescription()
{
	if (m_fnGetDescription)
		return m_fnGetDescription();

	return g_EmptyStr;
}

const TCHAR *CPlugin::GetShortDescription()
{
	if (m_fnGetShortDescription)
		return m_fnGetShortDescription();

	return g_EmptyStr;
}

UINT32 CPlugin::GetPluginVersion()
{
	if (m_fnGetPluginVersion)
		return m_fnGetPluginVersion();

	return 0;
}

nucleus::RetCode CPlugin::Activate()
{
	m_bActive = (m_fnActivate != nullptr) ? m_fnActivate(m_pSys) : true;

	return m_bActive ? nucleus::RetCode::OK : nucleus::RetCode::PLUG_NOACTIVATE;
}

void __cdecl ActivatePlugin(LPVOID param0, LPVOID param1, size_t task_number)
{
	CPlugin *pp = (CPlugin *)param0;

	pp->Activate();
}

nucleus::RetCode CPlugin::Deactivate()
{
	m_bActive = (m_fnDeactivate != nullptr) ? !m_fnDeactivate(m_pSys) : false;

	return !m_bActive ? nucleus::RetCode::OK : nucleus::RetCode::PLUG_NODEACTIVATE;
}

bool CPlugin::Active()
{
	return m_bActive;
}

bool CPlugin::Loaded()
{
	return (m_hMod != NULL) ? true : false;
}




CPluginManager::CPluginManager(nucleus::INucleus *psys)
{
	m_pSys = psys;
}

CPluginManager::~CPluginManager()
{
	UnloadAllPlugins();
}

nucleus::RetCode CPluginManager::DiscoverPlugins(const TCHAR *filespec, bool auto_activate, size_t *numfound)
{
	if (!filespec)
		return nucleus::RetCode::NULLFILESPEC;

	size_t ret = 0;

	TCHAR _filespec[MAX_PATH];
	_tcsncpy_s(_filespec, MAX_PATH, filespec, MAX_PATH - 1);

	TCHAR *c = _filespec;
	while (*c)
	{
		if (*c == '/')
			*c = '\\';
		c++;
	}

	if (!PathIsRoot(_filespec))
	{
		TCHAR cwd[MAX_PATH];
		_tgetcwd(cwd, MAX_PATH - 1);

		PathCombine(_filespec, cwd, _filespec);
	}

	TCHAR dir[MAX_PATH];
	_tcsncpy_s(dir, MAX_PATH, _filespec, MAX_PATH - 1);
	PathRemoveFileSpec(dir);
	PathAddBackslash(dir);

	WIN32_FIND_DATA fd;
	HANDLE hfind = FindFirstFile(_filespec, &fd);

	if (hfind != INVALID_HANDLE_VALUE)
	{
		TCHAR plugname[MAX_PATH];

		do
		{
			_tcsncpy_s(plugname, MAX_PATH, dir, MAX_PATH - 1);
			_tcsncat_s(plugname, MAX_PATH, fd.cFileName, MAX_PATH - 1);

			CPlugin *tmp = new CPlugin(plugname, m_pSys);

			if (tmp->Loaded())
			{
				ret++;

				m_Plugins.push_back(tmp);

				if (auto_activate)
				{
#if 0
					m_pSys->ThreadPool()->RunTask(ActivatePlugin, (LPVOID)tmp);
#else
					tmp->Activate();
#endif
				}
			}
			else
			{
				delete tmp;
			}

		} while (FindNextFile(hfind, &fd));

		FindClose(hfind);
	}

	if (numfound)
		*numfound = ret;

	return nucleus::RetCode::OK;
}

nucleus::RetCode CPluginManager::UnloadPlugin(nucleus::IPlugin *pplug)
{
	for (TPluginArray::iterator it = m_Plugins.begin(), last_it = m_Plugins.end(); it != last_it; it++)
	{
		// first, make sure the plugin is being managed
		if (pplug == *it)
		{
			pplug->Deactivate();

			delete pplug;
			m_Plugins.erase(it);

			return nucleus::RetCode::OK;
		}
	}

	return nucleus::RetCode::NOTFOUND;
}

void CPluginManager::UnloadAllPlugins()
{
	for (TPluginArray::iterator it = m_Plugins.begin(), last_it = m_Plugins.end(); it != last_it; it++)
	{
		CPlugin *pp = (CPlugin *)(*it);

		if (pp)
		{
			pp->Deactivate();

			delete pp;
		}
	}

	m_Plugins.clear();
}

size_t CPluginManager::GetNumPlugins()
{
	return m_Plugins.size();
}

nucleus::IPlugin *CPluginManager::GetPlugin(size_t idx)
{
	if (idx < m_Plugins.size())
		return m_Plugins[idx];

	return nullptr;
}
