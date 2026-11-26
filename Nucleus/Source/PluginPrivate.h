#pragma once

#include <NuPlugin.h>
#include <NuPluginManager.h>

class CPlugin : public nucleus::IPlugin
{

	typedef const TCHAR* const (__cdecl *PLUGFUNC_GETNAME)();
	typedef const TCHAR* const (__cdecl *PLUGFUNC_GETSHORTNAME)();
	typedef const TCHAR* const (__cdecl *PLUGFUNC_GETAUTHOR)();
	typedef const TCHAR* const (__cdecl *PLUGFUNC_GETWEBSITE)();
	typedef const TCHAR* const (__cdecl *PLUGFUNC_GETDESC)();
	typedef const TCHAR* const (__cdecl *PLUGFUNC_GETSHORTDESC)();
	typedef const TCHAR* const (__cdecl *PLUGFUNC_GETCOPYRIGHT)();
	typedef uint32_t(__cdecl *PLUGFUNC_GETVERSION)();
	typedef uint64_t(__cdecl *PLUGFUNC_GETNUCLEUSVERSION)();
	typedef uint32_t(__cdecl* PLUGFUNC_GETPLATFORM)();
	typedef bool(__cdecl* PLUGFUNC_ISUNICODE)();
	typedef bool(__cdecl *PLUGFUNC_ACTIVATE)(nucleus::INucleus *psys);
	typedef bool(__cdecl *PLUGFUNC_DEACTIVATE)(nucleus::INucleus *psys);

public:
	CPlugin(const TCHAR *filename, nucleus::INucleus *psys);
	virtual ~CPlugin();

	virtual const TCHAR *GetFilename();
	virtual const TCHAR *GetName();
	virtual const TCHAR *GetShortName();
	virtual const TCHAR *GetAuthor();
	virtual const TCHAR *GetCopyright();
	virtual const TCHAR *GetWebsite();
	virtual const TCHAR *GetDescription();
	virtual const TCHAR *GetShortDescription();
	virtual uint32_t GetPluginVersion();

	virtual nucleus::RetCode Activate();
	virtual nucleus::RetCode Deactivate();

	virtual bool Active();

	virtual bool Loaded();

protected:

	nucleus::INucleus *m_pSys;

	HMODULE m_hMod;
	bool m_bActive;

	tstring m_Filename;

	PLUGFUNC_GETNAME m_fnGetName;
	PLUGFUNC_GETSHORTNAME m_fnGetShortName;
	PLUGFUNC_GETVERSION m_fnGetPluginVersion;
	PLUGFUNC_GETAUTHOR m_fnGetAuthor;
	PLUGFUNC_GETWEBSITE m_fnGetWebsite;
	PLUGFUNC_GETDESC m_fnGetDescription;
	PLUGFUNC_GETSHORTDESC m_fnGetShortDescription;
	PLUGFUNC_GETCOPYRIGHT m_fnGetCopyright;
	PLUGFUNC_ACTIVATE m_fnActivate;
	PLUGFUNC_DEACTIVATE m_fnDeactivate;

};

class CPluginManager : public nucleus::IPluginManager
{

public:
	CPluginManager(nucleus::INucleus *psys);
	virtual ~CPluginManager();

	// Performs a discovery operation on the given filespec, looking for valid plugins.
	virtual nucleus::RetCode DiscoverPlugins(const TCHAR *filespec, bool auto_activate = true, size_t *numfound = nullptr);
	virtual nucleus::RetCode UnloadPlugin(nucleus::IPlugin *pplug);
	virtual void UnloadAllPlugins();

	virtual size_t GetNumPlugins();
	virtual nucleus::IPlugin *GetPlugin(size_t idx);

protected:
	nucleus::INucleus *m_pSys;

	typedef std::deque<nucleus::IPlugin *> TPluginArray;
	TPluginArray m_Plugins;

};
