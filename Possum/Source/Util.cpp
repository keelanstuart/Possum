#include "stdafx.h"
#include "Util.h"

static const TClefGlyphMap g_PresetClefs =
{
	{_T("Alto"),		_T("\xD835\xDD21")},
	{_T("Treble"),		_T("\xD834\xDD1E")},
	{_T("Bass"),		_T("\xD836\xDD22")},
};
static const tstring g_DefaultPreset = _T("Treble");

/// Computes hours, minutes, seconds, and hundredths of a second from high-resolution time in seconds
void GetTimeComponents(double time_in_seconds, UINT &h, UINT &m, UINT &s, UINT &f)
{
	UINT ws = UINT(time_in_seconds);
	s = ws % 60;
	m = ws / 60;
	h = m / 60;
	m %= 60;
	f = UINT((time_in_seconds - double(ws)) * 10);
}

void MyAppendMenuItem(HMENU menu, UINT32 type, const TCHAR *text, BOOL enabled, HMENU submenu, UINT id, DWORD_PTR data)
{
	MENUITEMINFO mii;
	memset(&mii, 0, sizeof(MENUITEMINFO));

	mii.cbSize = sizeof(MENUITEMINFO);

	mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_DATA;

	mii.wID = (id == -1) ? GetMenuItemCount(menu) : id;

	mii.fType = type;

	if ((type == MFT_STRING) || text)
	{
		mii.dwTypeData = (LPWSTR)text;
		mii.cch = (UINT)_tcslen(text);
	}

	if (!enabled)
	{
		mii.fMask |= MIIM_STATE;
		mii.fState = MFS_GRAYED;
	}

	if (submenu)
	{
		mii.hSubMenu = submenu;
		mii.fMask |= MIIM_SUBMENU;
	}

	mii.dwItemData = data;

	InsertMenuItem(menu, GetMenuItemCount(menu), true, &mii);
}
