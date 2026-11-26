
// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#if defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit

// turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions


#include <afxdisp.h>        // MFC Automation classes



#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC support for Internet Explorer 4 Common Controls
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>             // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxcontrolbars.h>     // MFC support for ribbons and control bars


#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif

#include <inttypes.h>

#include <tchar.h>

#include <string>
#include <map>
#include <vector>
#include <deque>
#include <algorithm>
#include <set>

#include <assert.h>

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


// includes from this project
#include <NuConfiguration.h>

#include "Version_Major.h"
#include "Version_Minor.h"
#include "Version_Build.h"

// third-party include
#include <PowerProps/Include/PowerProps.h>
#include <GenIO/Include/GenIO.h>

#include <gdiplus.h>

#define GetAValue(rgba)      (LOBYTE((rgba)>>24))


#define CONVERT_WCS2MBCS(wcs, mbcs) if (wcs) {			\
  size_t origsize = _tcslen(wcs) + 1;					\
  size_t newsize = (origsize * 2) * sizeof(char);		\
  mbcs = (char *)_alloca(newsize);						\
  size_t retval = 0;									\
  wcstombs_s(&retval, mbcs, newsize, wcs, newsize); }	\
  else mbcs = nullptr;

#define CONVERT_MBCS2WCS(mbcs, wcs) if (mbcs) {			\
  size_t origsize = strlen(mbcs) + 1;					\
  size_t newsize = origsize * sizeof(wchar_t);			\
  wcs = (wchar_t *)_alloca(newsize);					\
  size_t retval = 0;									\
  mbstowcs_s(&retval, wcs, origsize, mbcs, newsize); }	\
  else wcs = nullptr;

#if defined(_UNICODE) || defined(UNICODE)

#define CONVERT_TCS2MBCS(tcs, mbcs) CONVERT_WCS2MBCS(tcs, mbcs)

#define CONVERT_TCS2WCS(tcs, wcs) wcs = (wchar_t *)tcs;

#define CONVERT_MBCS2TCS(mbcs, tcs) CONVERT_MBCS2WCS(mbcs, tcs)

#define CONVERT_WCS2TCS(wcs, tcs) tcs = (TCHAR *)wcs;

#else

#define CONVERT_TCS2MBCS(tcs, mbcs) mbcs = (char *)tcs;

#define CONVERT_TCS2WCS(tcs, wcs) CONVERT_MBCS2WCS(tcs, wcs)

#define CONVERT_MBCS2TCS(mbcs, tcs) tcs = (TCHAR *)mbcs;

#define CONVERT_WCS2TCS(tcs, wcs) CONVERT_WCS2MBCS(wcs, tcs)

#endif
