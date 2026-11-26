// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit
#define _AFX_NO_MFC_CONTROLS_IN_DIALOGS         // remove support for MFC controls in dialogs

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
#endif

#include <afx.h>
#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC support for Internet Explorer 4 Common Controls
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>                     // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <iostream>
// Windows Header Files:
#include <windows.h>

#include <tchar.h>

#include <string>
#include <map>
#include <vector>
#include <deque>
#include <algorithm>

#include "miniaudio.h"

typedef std::basic_string<TCHAR> tstring;


// macros to convert to and from wide- and multi-byte- character
// strings from the other type... all on the stack.
#define LOCAL_WCS2MBCS(wcs, mbcs) {               \
  size_t origsize = _tcslen(wcs) + 1;             \
  size_t newsize = (origsize * 2) * sizeof(char); \
  mbcs = (char *)_alloca(newsize);                \
  size_t retval = 0;                              \
  wcstombs_s(&retval, mbcs, newsize, wcs, newsize); }

#define LOCAL_MBCS2WCS(mbcs, wcs) {               \
  size_t origsize = strlen(mbcs) + 1;             \
  size_t newsize = origsize * sizeof(wchar_t);    \
  wcs = (wchar_t *)_alloca(newsize);              \
  size_t retval = 0;                              \
  mbstowcs_s(&retval, wcs, origsize, mbcs, newsize); }

#if defined(_UNICODE) || defined(UNICODE)

#define LOCAL_TCS2MBCS(tcs, mbcs) LOCAL_WCS2MBCS(tcs, mbcs)

#define LOCAL_TCS2WCS(tcs, wcs) wcs = (wchar_t *)tcs;

#define LOCAL_MBCS2TCS(mbcs, tcs) LOCAL_MBCS2WCS(mbcs, tcs)

#define LOCAL_WCS2TCS(wcs, tcs) tcs = (TCHAR *)wcs;

#else

#define LOCAL_TCS2MBCS(tcs, mbcs) mbcs = (char *)tcs;

#define LOCAL_TCS2WCS(tcs, wcs) LOCAL_MBCS2WCS(tcs, wcs)

#define LOCAL_MBCS2TCS(mbcs, tcs) tcs = (TCHAR *)mbcs;

#define LOCAL_WCS2TCS(tcs, wcs) LOCAL_WCS2MBCS(wcs, tcs)

#endif
