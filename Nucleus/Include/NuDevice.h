// **************************************************************
// Nucleus Source File
//
// Copyright © 2019-2026, Keelan Sturt


#pragma once

#include <PowerProps\Include\PowerProps.h>
#include <NuRetCode.h>


namespace nucleus
{

	class ICommand;
	class ICommandInfo;

	/// IDevice is an interface that is implemnted for anything controllable.
	/// This concept is truly an abstract one, for a "device" may be a physical device
	/// attached to a USB or serial port, or could be a software component.
	class IDevice
	{

	public:

		using Status = enum
		{
			Unknown = 0,

			Ready,				/// The device is ready to receive commands
			Absent,				/// The device, though registered, is not connected to the system. This may be a physical condition -- or a program may not be running
			Busy,				/// The device is busy doing something and cannot be interrupted

			NumStates
		};

		/// Initializes the device, giving it the INucleus interface that it is registered with
		virtual RetCode Initialize(const INucleus *psys) = NULL;

		/// Shuts the device down
		virtual RetCode Shutdown() = NULL;

		/// Shuts the device down
		virtual RetCode Reset() = NULL;

		/// Sets the device pause state
		virtual RetCode SetPaused(bool pause) = NULL;

		/// Performs any updates that the device may require, giving it a chance to poll, etc.
		virtual void Update(double time) = NULL;

		/// Reports the status of the device, given then 
		virtual Status GetStatus() const = NULL;

		/// Returns the name of the device
		virtual const TCHAR *GetName() const = NULL;

		/// Returns a description of the device
		virtual const TCHAR *GetDescription() const = NULL;

		/// Returns the number of commands that the device can handle
		virtual size_t GetNumCommands() const = NULL;

		/// Returns an ICommand given its index (< GetNumCommands)
		virtual const ICommandInfo *GetCommand(size_t idx) const = NULL;

		/// Returns an ICommand given its name
		virtual const ICommandInfo *GetCommandByName(const TCHAR *name) const = NULL;

		/// Causes the device to take action; executes the given command
		/// If desired, the device may return a new command that undoes what the one issues dones
		virtual nucleus::RetCode ExecuteCommand(const nucleus::ICommand *pcmd, double time, nucleus::ICommand **pcmd_undo = nullptr) = NULL;

		/// Properties applicable to the entire device
		virtual props::IPropertySet *GetProperties() = NULL;

	};

};