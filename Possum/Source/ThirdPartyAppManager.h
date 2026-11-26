#pragma once

class CThirdPartyAppManager
{
public:
	typedef std::map<tstring, HANDLE> TNamedProcessMap;
	typedef std::map<HANDLE, HWND> TProcessWindowMap;
	typedef std::map<HANDLE, DWORD> TProcessIdMap;

	enum EAppRegParamOrd
	{
		NAME = 0,
		EXEPATH,
		RUNDIR,
		PARAMS,
		WNDCLASS,
		WNDNAME,

		NUMPARAMS
	};

	CThirdPartyAppManager();
	virtual ~CThirdPartyAppManager();

	size_t GetAppCount();
	bool RegisterApp(const TCHAR *name, const TCHAR *exepath, const TCHAR *rundir = nullptr, const TCHAR *params = nullptr, const TCHAR *wndclass = nullptr, const TCHAR *wndname = nullptr);
	bool UnregisterApp(const TCHAR *name);

	bool GetRegisteredApp(size_t idx, tstring *name = nullptr, tstring *exepath = nullptr, tstring *rundir = nullptr, tstring *params = nullptr, tstring *wndclass = nullptr, tstring *wndname = nullptr);

	bool IsAppRegistered(const TCHAR *name);

	bool FindAppByExePath(const TCHAR *exepath, size_t &idx);
	bool FindAppByName(const TCHAR *name, size_t &idx);

	bool IsAppRunning(const TCHAR *name, HANDLE *hproc = nullptr, HWND *hwnd = nullptr);
	bool HasMainWindow(const TCHAR *name, HWND *hwnd = nullptr);
	bool RunApp(const TCHAR *name);
	bool KillApp(const TCHAR *name);
	void KillAllOpenApps();

	HWND GetMainHwnd(HANDLE hproc);

protected:
	TNamedProcessMap m_ProcMap;
	TProcessWindowMap m_WndMap;
	TProcessIdMap m_IdMap;

	HMODULE m_hNtdll;

	void MakeAppTag(tstring &tag, size_t idx);
};

