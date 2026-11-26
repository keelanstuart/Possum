// **************************************************************
// Nucleus Source File
//
// Copyright © 2019-2026, Keelan Sturt

#pragma once

#include <NuRetCode.h>


// Macros to use in plugin libraries to create and export relevant functions

#define NUPLUGIN_NAME(s)			extern "C" { const __declspec(dllexport) TCHAR *NucleusPlugin_GetName() { return _T(s); } }
#define NUPLUGIN_SHORTNAME(s)		extern "C" { const __declspec(dllexport) TCHAR *NucleusPlugin_GetShortName() { return _T(s); } }
#define NUPLUGIN_AUTHOR(s)			extern "C" { const __declspec(dllexport) TCHAR *NucleusPlugin_GetAuthor() { return _T(s); } }
#define NUPLUGIN_WEBSITE(s)			extern "C" { const __declspec(dllexport) TCHAR *NucleusPlugin_GetWebsite() { return _T(s); } }
#define NUPLUGIN_DESC(s)			extern "C" { const __declspec(dllexport) TCHAR *NucleusPlugin_GetDescription() { return _T(s); } }
#define NUPLUGIN_COPYRIGHT(s)		extern "C" { const __declspec(dllexport) TCHAR *NucleusPlugin_GetCopyright() { return _T(s); } }
#define NUPLUGIN_VERSION(n)			extern "C" { uint32_t __declspec(dllexport) NucleusPlugin_GetPluginVersion() { return n; } }
#define NUPLUGIN_NUCLEUSVERSION()	extern "C" { uint64_t __declspec(dllexport) NucleusPlugin_GetNucleusVersion() { return NUCLEUS_VERSION; } }
#if defined(_M_X64)
#define NUPLUGIN_GETPLATFORM()		extern "C" { uint32_t __declspec(dllexport) NucleusPlugin_GetPlatform() { return 64; } }
#else
#define NUPLUGIN_GETPLATFORM()		extern "C" { uint32_t __declspec(dllexport) NucleusPlugin_GetPlatform() { return 32; } }
#endif
#if defined(UNICODE)
#define NUPLUGIN_ISUNICODE()		extern "C" { bool __declspec(dllexport) NucleusPlugin_IsUnicode() { return true; } }
#else
#define NUPLUGIN_ISUNICODE()		extern "C" { bool __declspec(dllexport) NucleusPlugin_IsUnicode() { return false; } }
#endif

// The Activate and Deactivate functions are meant to provide your plug-in with a way of allocating
// or freeing system resources. Client applications may choose to demand load / unload your
// code, so this it is essential for "good behavior" that you use these appropriately.
#define NUPLUGIN_ONACTIVATE_BEGIN()		extern "C" { bool __declspec(dllexport) NucleusPlugin_Activate(nucleus::INucleus *psys) {
#define NUPLUGIN_ONACTIVATE_END()		} }

#define NUPLUGIN_ONDEACTIVATE_BEGIN()	extern "C" { bool __declspec(dllexport) NucleusPlugin_Deactivate(nucleus::INucleus *psys) {
#define NUPLUGIN_ONDEACTIVATE_END()		} }

// Use these functions like this in your original plug-in code:

/*
NUPLUGIN_NAME("MyPlugin");
NUPLUGIN_AUTHOR("Me");
NUPLUGIN_DESC("This does all the things I want");
NUPLUGIN_COPYRIGHT("2019 Me, All Rights Reserved");

NUPLUGIN_ONACTIVATE_BEGIN()
	// example activation code...
	::MessageBox("blah", "blah", MB_OK);
NUPLUGIN_ONACTIVATE_END()

int dllmain()
{
....
return 0;
}
*/



namespace nucleus
{

	// The plugin information class
	class IPlugin
	{

	public:
		virtual const TCHAR *GetFilename() = NULL;
		virtual const TCHAR *GetName() = NULL;
		virtual const TCHAR *GetShortName() = NULL;
		virtual const TCHAR *GetAuthor() = NULL;
		virtual const TCHAR *GetCopyright() = NULL;
		virtual const TCHAR *GetWebsite() = NULL;
		virtual const TCHAR *GetDescription() = NULL;
		virtual UINT32 GetPluginVersion() = NULL;

		virtual RetCode Activate() = NULL;
		virtual RetCode Deactivate() = NULL;

		virtual bool Active() = NULL;

		virtual bool Loaded() = NULL;

	};

}