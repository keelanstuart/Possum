// **************************************************************
// Nucleus Source File
//
// Copyright © 2019-2026, Keelan Sturt

#pragma once

#include <NuRetCode.h>


namespace nucleus
{

	class IDevice;

	/// Implement this interface to be notified of device status changes in your application
	class IDeviceStatusListener
	{

	public:
		virtual void DeviceStatusChanged(const IDevice *pdevice) = NULL;

	};

	class NUCLEUS_API IDeviceManager
	{

	public:
		/// Registers a device with the device manager. Once registered, a device is discoverable by any application that has the owning INucleus interface
		/// Implicitly calls IDevice::Initialize()
		virtual RetCode RegisterDevice(IDevice *ppdev) = NULL;

		/// Removes the device, effectively disconnecting it from Nucleus
		/// Implicitly calls IDevice::Shutdown()
		virtual RetCode UnregisterDevice(IDevice *pdev) = NULL;

		/// Reports the number of registered devices
		virtual size_t GetDeviceCount() = NULL;

		/// Returns true if the given device is registered, false otherwise
		virtual bool IsDeviceRegistered(const IDevice *pdev) = NULL;

		/// Returns a device given its index. Obviously the index may change, so it should never be serialized to reference a particular device
		virtual IDevice *GetDevice(size_t idx) = NULL;

		/// Returns a device given its name
		virtual IDevice *GetDeviceByName(const TCHAR *name) = NULL;

		/// Updates all devices
		virtual void Update(double time = 0.0f) = NULL;
	};

};