// **************************************************************
// Nucleus Source File
//
// Copyright © 2019-2026, Keelan Sturt


#pragma once

#include <NuDeviceManager.h>


class CDeviceManager : public nucleus::IDeviceManager
{

public:
	CDeviceManager(nucleus::INucleus *psys);
	virtual ~CDeviceManager();

	virtual nucleus::RetCode RegisterDevice(nucleus::IDevice *ppdev);
	virtual nucleus::RetCode UnregisterDevice(nucleus::IDevice *pdev);
	virtual bool IsDeviceRegistered(const nucleus::IDevice *pdev);

	virtual size_t GetDeviceCount();
	virtual nucleus::IDevice *GetDevice(size_t idx);
	virtual nucleus::IDevice *GetDeviceByName(const TCHAR *name);

	virtual void Update(double time);

protected:
	nucleus::INucleus *m_pSys;

	typedef std::deque<nucleus::IDevice *> TDeviceArray;
	TDeviceArray m_Devs;

	typedef std::map<tstring, nucleus::IDevice *> TDeviceMap;
	typedef std::pair<tstring, nucleus::IDevice *> TDeviceMapPair;
	TDeviceMap m_MappedDevs;
};