// Nucleus.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "Nucleus.h"

#include "PluginPrivate.h"
#include "DeviceManagerPrivate.h"
#include "LogPrivate.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// The one and only application object

CWinApp theApp;

using namespace std;


// ***************************************************************************************

// This class is exported from the Nucleus.dll
class CNucleus : public nucleus::INucleus
{
public:
	CNucleus();
	virtual ~CNucleus();

	virtual void Release();

	virtual nucleus::IPluginManager *PluginManager();

	virtual nucleus::IDeviceManager *DeviceManager();

	virtual pool::IThreadPool *ThreadPool();

	virtual nucleus::ILog *Log();

	virtual uint64_t GetVersion() { return NUCLEUS_VERSION; }

protected:
	CPluginManager *m_pPluginManager;

	CDeviceManager *m_pDeviceManager;

	pool::IThreadPool *m_pThreadPool;

	CLog *m_pLog;
};


// ***************************************************************************************

nucleus::INucleus *nucleus::INucleus::Create()
{
	nucleus::INucleus *ret = new CNucleus();

	return ret;
}


// ***************************************************************************************

CNucleus::CNucleus()
{
	m_pPluginManager = nullptr;
	m_pLog = nullptr;
	m_pDeviceManager = nullptr;
	m_pThreadPool = nullptr;
}


// ***************************************************************************************

CNucleus::~CNucleus()
{
	if (m_pPluginManager)
	{
		delete m_pPluginManager;
		m_pPluginManager = nullptr;
	}

	if (m_pLog)
	{
		delete m_pLog;
		m_pLog = nullptr;
	}

	if (m_pDeviceManager)
	{
		delete m_pDeviceManager;
		m_pDeviceManager = nullptr;
	}

	if (m_pThreadPool)
	{
		m_pThreadPool->Release();
		m_pThreadPool = nullptr;
	}
}


// ***************************************************************************************

void CNucleus::Release()
{
	delete this;
}


// ***************************************************************************************

nucleus::IPluginManager *CNucleus::PluginManager()
{
	if (!m_pPluginManager)
		m_pPluginManager = new CPluginManager(this);

	return m_pPluginManager;
}


// ***************************************************************************************

nucleus::IDeviceManager *CNucleus::DeviceManager()
{
	if (!m_pDeviceManager)
		m_pDeviceManager = new CDeviceManager(this);

	return m_pDeviceManager;
}


// ***************************************************************************************

nucleus::ILog *CNucleus::Log()
{
	if (!m_pLog)
		m_pLog = new CLog();

	return m_pLog;
}


// ***************************************************************************************

pool::IThreadPool *CNucleus::ThreadPool()
{
	if (!m_pThreadPool)
		m_pThreadPool = pool::IThreadPool::Create(1, -1);

	return m_pThreadPool;
}
