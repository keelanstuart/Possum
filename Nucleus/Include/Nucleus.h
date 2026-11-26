// **************************************************************
// Nucleus Source File
//
// Copyright © 2019-2020, Spitz Inc.


#pragma once


// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the NUCLEUS_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// NUCLEUS_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef NUCLEUS_EXPORTS
#define NUCLEUS_API __declspec(dllexport)
#else
#define NUCLEUS_API __declspec(dllimport)
#endif


#include <stdint.h>
#include <tchar.h>
#include <string>

using tstring = std::basic_string<TCHAR>;


#include <NuPlugin.h>
#include <NuPluginManager.h>
#include <NuDeviceManager.h>
#include <NuLog.h>
#include <pool/Include/Pool.h>


#define NUCLEUS_VERSION_MAJOR	1
#define NUCLEUS_VERSION_MINOR	0
#define NUCLEUS_VERSION_BUILD	0

// 16-bit major and minor version numbers, 32-bit build number
#define NUCLEUS_VERSION		((uint64_t(NUCLEUS_VERSION_MAJOR & 0xFFFF) << (uint64_t)48) | (uint64_t(NUCLEUS_VERSION_MINOR & 0xFFFF) << (uint64_t)32) | uint64_t(NUCLEUS_VERSION_BUILD & 0xFFFFFF))


namespace nucleus
{
	/// INucleus is the conduit through which everything interacts
	class NUCLEUS_API INucleus
	{
	public:
		/// Creates an instance of a something that exposes an INucleus interface
		static INucleus *Create();

		/// Releases the INucleus
		virtual void Release() = NULL;

		virtual uint64_t GetVersion() = NULL;

		/// Through the PluginManager, extension modules can be loaded that may
		/// register devices and their commands -- or anything else the system
		/// is capable of
		virtual IPluginManager *PluginManager() = NULL	;

		/// The DeviceManager allows all device classes to be discovered by client
		/// applications and for extension modules to un/register their own devices
		virtual IDeviceManager *DeviceManager() = NULL;

		/// A shared IThreadPool for all Nucleus functions requiring asynchronous operations
		virtual pool::IThreadPool *ThreadPool() = NULL;

		/// Log allows all connected code to write messages to a single file or redirected
		/// through user-provided code
		virtual ILog *Log() = NULL;
	};
}
