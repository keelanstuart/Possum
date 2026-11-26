#pragma once

void GetTimeComponents(double time_in_seconds, UINT &h, UINT &m, UINT &s, UINT &f);

void MyAppendMenuItem(HMENU menu, UINT32 type, const TCHAR *text = NULL, BOOL enabled = true, HMENU submenu = NULL, UINT id = -1, DWORD_PTR data = NULL);

using TClefGlyphMap = std::map<tstring, tstring>;

extern const TClefGlyphMap g_PresetClefs;
extern const tstring g_DefaultPreset;