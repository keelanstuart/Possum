#include "stdafx.h"
#include "ThirdPartyAppManager.h"
#include "Possum.h"

#include <tlhelp32.h>


CThirdPartyAppManager::CThirdPartyAppManager()
{
}

CThirdPartyAppManager::~CThirdPartyAppManager()
{
}


size_t CThirdPartyAppManager::GetAppCount()
{
	return theApp.m_pCfg->GetNumSubKeys(_T("third-party-apps"));
}



bool CThirdPartyAppManager::RegisterApp(const TCHAR *name, const TCHAR *exepath, const TCHAR *rundir, const TCHAR *params, const TCHAR *wndclass, const TCHAR *wndname)
{
	if (!IsAppRegistered(name))
	{
		tstring tmp, appkey;
		MakeAppTag(appkey, GetAppCount());
		
		tmp = appkey + _T(".name");
		theApp.m_pCfg->SetString(tmp.c_str(), name);

		tmp = appkey + _T(".exepath");
		theApp.m_pCfg->SetString(tmp.c_str(), exepath);

		tmp = appkey + _T(".rundir");
		theApp.m_pCfg->SetString(tmp.c_str(), rundir);

		tmp = appkey + _T(".params");
		theApp.m_pCfg->SetString(tmp.c_str(), params);

		tmp = appkey + _T(".wndclass");
		theApp.m_pCfg->SetString(tmp.c_str(), wndclass);

		tmp = appkey + _T(".wndname");
		theApp.m_pCfg->SetString(tmp.c_str(), wndname);

		return true;
	}

	return false;
}

bool CThirdPartyAppManager::UnregisterApp(const TCHAR *name)
{
	tstring tag;

	for (size_t i = 0, maxi = GetAppCount(); i < maxi; i++)
	{
		MakeAppTag(tag, i);

		tstring tmptag = tag;
		tmptag += _T(".name");

		if (!_tcsicmp(theApp.m_pCfg->GetString(tmptag.c_str(), _T("")), name))
		{
			theApp.m_pCfg->RemoveKey(tag.c_str());

			return true;
		}
	}

	return false;
}

void CThirdPartyAppManager::MakeAppTag(tstring &tag, size_t idx)
{
	TCHAR tagnum[16];
	_itot_s(int(idx), tagnum, 16, 10);

	tag = _T("third-party-apps.app#");
	tag += tagnum;
}

bool CThirdPartyAppManager::IsAppRegistered(const TCHAR *name)
{
	tstring tag;

	for (size_t i = 0, maxi = GetAppCount(); i < maxi; i++)
	{
		MakeAppTag(tag, i);
		tag += _T(".name");

		if (!_tcsicmp(theApp.m_pCfg->GetString(tag.c_str(), _T("")), name))
		{
			return true;
		}
	}

	return false;
}

bool CThirdPartyAppManager::GetRegisteredApp(size_t idx, tstring *name, tstring *exepath, tstring *rundir, tstring *params, tstring *wndclass, tstring *wndname)
{
	if (idx > GetAppCount())
		return false;

	tstring approotkey;
	MakeAppTag(approotkey, idx);

	if (name)
	{
		tstring temp = approotkey;
		temp += _T(".name");
		*name = theApp.m_pCfg->GetString(temp.c_str(), _T(""));
	}

	if (exepath)
	{
		tstring temp = approotkey;
		temp += _T(".exepath");
		*exepath = theApp.m_pCfg->GetString(temp.c_str(), _T(""));
	}

	if (rundir)
	{
		tstring temp = approotkey;
		temp += _T(".rundir");
		*rundir = theApp.m_pCfg->GetString(temp.c_str(), _T(""));
	}

	if (params)
	{
		tstring temp = approotkey;
		temp += _T(".params");
		*params = theApp.m_pCfg->GetString(temp.c_str(), _T(""));
	}

	if (wndclass)
	{
		tstring temp = approotkey;
		temp += _T(".wndclass");
		*wndclass = theApp.m_pCfg->GetString(temp.c_str(), _T(""));
	}

	if (wndname)
	{
		tstring temp = approotkey;
		temp += _T(".wndname");
		*wndname = theApp.m_pCfg->GetString(temp.c_str(), _T(""));
	}

	return true;
}

bool CThirdPartyAppManager::FindAppByExePath(const TCHAR *exepath, size_t &idx)
{
	for (size_t i = 0, maxi = GetAppCount(); i < maxi; i++)
	{
		tstring s;
		GetRegisteredApp(i, nullptr, &s, nullptr, nullptr, nullptr);
		if (!_tcsicmp(s.c_str(), exepath))
		{
			idx = i;
			return true;
		}
	}

	return false;
}

bool CThirdPartyAppManager::FindAppByName(const TCHAR *name, size_t &idx)
{
	for (size_t i = 0, maxi = GetAppCount(); i < maxi; i++)
	{
		tstring s;
		GetRegisteredApp(i, &s, nullptr, nullptr, nullptr, nullptr);
		if (!_tcsicmp(s.c_str(), name))
		{
			idx = i;
			return true;
		}
	}

	return false;
}

BOOL CALLBACK FindAppEnum(HWND hwnd, LPARAM lParam)
{
	DWORD dwID;
	GetWindowThreadProcessId(hwnd, &dwID);
	if (dwID == (DWORD)lParam)
	{
	}

	return TRUE;
}

bool CThirdPartyAppManager::IsAppRunning(const TCHAR *name, HANDLE *hproc, HWND *hwnd)
{
	size_t appidx;
	if (!FindAppByName(name, appidx))
		return false;

	tstring n = name;
	std::transform(n.begin(), n.end(), n.begin(), tolower);

	TNamedProcessMap::const_iterator it = m_ProcMap.find(n);
	if (it == m_ProcMap.end())
		return false;

	HANDLE _hproc;
	if (!hproc)
		hproc = &_hproc;

	*hproc = it->second;
	DWORD ec;
	BOOL r = GetExitCodeProcess(*hproc, &ec);

	if (hwnd)
	{
		*hwnd = NULL;
		TProcessWindowMap::const_iterator wit = m_WndMap.find(*hproc);
		if (wit != m_WndMap.cend())
			*hwnd = wit->second;
	}

	if (r && (ec == STILL_ACTIVE))
		return true;

	return false;
}

struct handle_data
{
	unsigned long process_id;
	HWND window_handle_pid_ok, window_handle_pid_bad;
	const TCHAR *wndclass;
	const TCHAR *wndname;
};

BOOL is_main_window(HWND handle)
{
	return GetWindow(handle, GW_OWNER) == (HWND)0 && IsWindowVisible(handle);
}

BOOL CALLBACK enum_windows_callback(HWND handle, LPARAM lParam)
{
	handle_data& data = *(handle_data*)lParam;
	unsigned long process_id = 0;
	TCHAR wndclass[MAX_PATH], wndname[MAX_PATH];

	// then check the wnd class for a match
	RealGetWindowClass(handle, wndclass, MAX_PATH);
	if (data.wndclass && *data.wndclass && _tcsicmp(wndclass, data.wndclass))
		return TRUE;

	// then check the wnd name for a match
	::GetWindowText(handle, wndname, MAX_PATH);
	if (data.wndname && *data.wndname && _tcsicmp(wndname, data.wndname))
		return TRUE;

	// first, check the process id for a match
	GetWindowThreadProcessId(handle, &process_id);
	if (data.process_id != process_id || !is_main_window(handle))
	{
		data.window_handle_pid_bad = handle;
		return TRUE;
	}

	// we found it!
	data.window_handle_pid_ok = handle;
	return FALSE;
}

HWND find_main_window(unsigned long process_id, const TCHAR *wndclass, const TCHAR *wndname)
{
	handle_data data;
	data.process_id = process_id;
	data.window_handle_pid_bad = data.window_handle_pid_ok = 0;
	data.wndclass = wndclass;
	data.wndname = wndname;

	EnumWindows(enum_windows_callback, (LPARAM)&data);

	return (data.window_handle_pid_ok != 0) ? data.window_handle_pid_ok : data.window_handle_pid_bad;
}

bool CThirdPartyAppManager::RunApp(const TCHAR *name)
{
	HWND hw;

	size_t appidx;
	if (!FindAppByName(name, appidx))
		return false;

	tstring _exename;
	GetRegisteredApp(appidx, nullptr, &_exename, nullptr, nullptr, nullptr);
	TCHAR *exename = PathFindFileName(_exename.c_str());

	DWORD dwOwnPID = GetProcessId(GetCurrentProcess());
	HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (hSnapShot)
	{
		PROCESSENTRY32 processInfo;
		ZeroMemory(&processInfo, sizeof(PROCESSENTRY32));
		processInfo.dwSize = sizeof(PROCESSENTRY32);

		while (Process32Next(hSnapShot, &processInfo) != FALSE)
		{
			if (!_tcsicmp(processInfo.szExeFile, exename))
			{
				if (processInfo.th32ParentProcessID != dwOwnPID)
				{
					HANDLE hptemp = OpenProcess(PROCESS_TERMINATE, TRUE, processInfo.th32ProcessID);

					if (hptemp && (hptemp != INVALID_HANDLE_VALUE))
						TerminateProcess(hptemp, -1);

					CloseHandle(hptemp);
				}

				break;
			}
		}

		CloseHandle(hSnapShot);
	}

	if (!IsAppRunning(name))
	{
		size_t idx;
		if (!FindAppByName(name, idx))
			return false;

		tstring exe, rundir, params, wcls, wname;
		if (!GetRegisteredApp(idx, nullptr, &exe, &rundir, &params, &wcls, &wname))
			return false;

		HANDLE hsem = CreateSemaphore(nullptr, 1, 1, _T("PossumAppLaunchSem"));
		WaitForSingleObject(hsem, INFINITE);

		STARTUPINFO si;
		ZeroMemory(&si, sizeof(STARTUPINFO));
		si.cb = sizeof(STARTUPINFO);
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_MINIMIZE;

		PROCESS_INFORMATION pi;
		ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

		TCHAR cmdline[4096];
		_stprintf_s(cmdline, _T("\"%s\" %s"), exe.c_str(), params.c_str());

		if (CreateProcess(nullptr, cmdline, nullptr, nullptr, FALSE, CREATE_SUSPENDED, nullptr, rundir.c_str(), &si, &pi))
		{
			ResumeThread(pi.hThread);
			WaitForInputIdle(pi.hProcess, 1000 * 60);

			do
			{
				if (theApp.m_Exiting)
				{
					TerminateProcess(pi.hProcess, -1);
					return false;
				}

				// try to find a window that matches
				hw = find_main_window(pi.dwProcessId, wcls.c_str(), wname.c_str());

				if (!hw)
				{
					if (WaitForSingleObject(pi.hProcess, 50) == WAIT_OBJECT_0)
						return false;
				}
			}
			while (!hw);

			tstring n = name;
			std::transform(n.begin(), n.end(), n.begin(), tolower);
			m_ProcMap.insert(TNamedProcessMap::value_type(n, pi.hProcess));
			m_WndMap.insert(TProcessWindowMap::value_type(pi.hProcess, hw));
			m_IdMap.insert(TProcessIdMap::value_type(pi.hProcess, pi.dwProcessId));
		}

		ReleaseSemaphore(hsem, 1, nullptr);
		CloseHandle(hsem);

		return true;
	}

	return false;
}

bool CThirdPartyAppManager::KillApp(const TCHAR *name)
{
	bool ret;

	HANDLE h;
	if (IsAppRunning(name, &h))
	{
		TerminateProcess(h, 0);

		tstring n = name;

		m_ProcMap.erase(n);
		m_WndMap.erase(h);
		m_IdMap.erase(h);
	}

	ret = !IsAppRunning(name);

	return ret;
}

void CThirdPartyAppManager::KillAllOpenApps()
{
	for (TProcessWindowMap::const_iterator wit = m_WndMap.cbegin(); wit != m_WndMap.cend(); wit++)
	{
		::CloseWindow(wit->second);
		Sleep(10);
	}

	for (TNamedProcessMap::const_iterator it = m_ProcMap.cbegin(); it != m_ProcMap.cend(); it++)
	{
		TerminateProcess(it->second, 0);
		Sleep(10);
	}

	m_ProcMap.clear();
	m_WndMap.clear();
	m_IdMap.clear();
}

HWND CThirdPartyAppManager::GetMainHwnd(HANDLE hproc)
{
	TProcessWindowMap::const_iterator it = m_WndMap.find(hproc);
	if (it == m_WndMap.end())
		return NULL;

	return it->second;
}
