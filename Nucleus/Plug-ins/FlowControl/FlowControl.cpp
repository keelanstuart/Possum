// FlowControl.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "FlowControl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include <Nucleus.h>
#include <NuPlugin.h>
#include <NuDevice.h>
#include <NuDeviceManager.h>
#include <NuCommandInfo.h>
#include <NuCommand.h>


using namespace std;


NUPLUGIN_NAME("Flow Control");
NUPLUGIN_SHORTNAME("Control");
NUPLUGIN_AUTHOR("Keelan Stuart");
NUPLUGIN_WEBSITE("");
NUPLUGIN_DESC("Provides flow control mechanisms");
NUPLUGIN_COPYRIGHT("2019-2026, Keelan Stuart");
NUPLUGIN_VERSION(1000);
NUPLUGIN_NUCLEUSVERSION();		// required to determine if this plug-in is compatible with the version of Nucleus that's loading it
NUPLUGIN_GETPLATFORM();
NUPLUGIN_ISUNICODE();


class CFlowControlDevice : nucleus::IDevice
{
protected:

	BEGIN_COMMAND_DEFINITION(Pause, "Pauses the show")
		AddParam(_T("Icon"),				'ICON', _T("./icons/icon_pause_color_normal.png"), props::IProperty::PA_FILENAME, props::IProperty::PROPFLAG(props::IProperty::EPropFlag::HIDDEN));
	END_COMMAND_DEFINITION(Pause);

	BEGIN_COMMAND_DEFINITION(Mark, "A marker to delineate portions of your show")
		AddParam(_T("Duration (seconds)"),	'DURA', 8.0f);
		AddParam(_T("Icon"),				'ICON', _T("./icons/icon_pin_color_normal.png"), props::IProperty::PA_FILENAME, props::IProperty::PROPFLAG(props::IProperty::EPropFlag::HIDDEN));
	END_COMMAND_DEFINITION(Mark);

	BEGIN_COMMAND_DEFINITION(Jump, "Jumps to a command matching the tag you specify")
		AddParam(_T("Jump Tag"),			'JUMP', _T(""));
		AddParam(_T("Icon"),				'ICON', _T("./icons/icon_jump_color_normal.png"), props::IProperty::PA_FILENAME, props::IProperty::PROPFLAG(props::IProperty::EPropFlag::HIDDEN));
	END_COMMAND_DEFINITION(Jump);

	BEGIN_COMMAND_DEFINITION(Subshow, "Recurses into another show, allowing you to re-use content")
		AddParam(_T("Show Path"),			'PATH', _T(""), props::IProperty::PA_FILENAME);
		AddParam(_T("Duration (seconds)"),	'DURA', 0.0f, props::IProperty::PA_GENERIC, props::IProperty::PROPFLAG(props::IProperty::EPropFlag::READONLY) | props::IProperty::PROPFLAG(props::IProperty::EPropFlag::HIDDEN));
		AddParam(_T("Icon"),				'ICON', _T("./icons/icon_subshow_color_normal.png"), props::IProperty::PA_FILENAME, props::IProperty::PROPFLAG(props::IProperty::EPropFlag::HIDDEN));
	END_COMMAND_DEFINITION(Subshow);

	COMMAND_REGISTRY();

public:
	CFlowControlDevice()
	{
		m_pSys = nullptr;

		REGISTER_COMMAND(Pause);
		REGISTER_COMMAND(Mark);
		REGISTER_COMMAND(Jump);
		REGISTER_COMMAND(Subshow);
	}

	virtual ~CFlowControlDevice()
	{
		RELEASE_COMMANDS();
	}

	virtual props::IPropertySet *GetProperties() { return nullptr; }

	/// Initializes the device, giving it the INucleus interface that it is registered with
	virtual nucleus::RetCode Initialize(const nucleus::INucleus *psys)
	{
		m_pSys = (nucleus::INucleus *)psys;

		return nucleus::RetCode::OK;
	}

	virtual nucleus::RetCode Shutdown()
	{
		return nucleus::RetCode::OK;
	}

	virtual nucleus::RetCode Reset()
	{
		return nucleus::RetCode::OK;
	}

	virtual nucleus::RetCode SetPaused(bool pause)
	{
		return nucleus::RetCode::OK;
	}

	/// Performs any updates that the device may require, giving it a chance to poll, etc.
	virtual void Update(double time)
	{

	}

	/// Reports the status of the device, given then 
	virtual nucleus::IDevice::Status GetStatus() const
	{
		return Ready;
	}

	/// Returns the name of the device
	virtual const TCHAR *GetName() const
	{
		return _T("Flow Control");
	}

	/// Returns a description of the device
	virtual const TCHAR *GetDescription() const
	{
		return _T("Provides flow control mechanisms for shows.");
	}

	/// Causes the device to take action; executes the given command with a set of arguments
	virtual nucleus::RetCode ExecuteCommand(const nucleus::ICommand *pcmd, double time, nucleus::ICommand **pcmd_undo = nullptr)
	{
		const nucleus::ICommandInfo *pcmdinfo = pcmd->GetCommandInfo();
		const nucleus::ITrack *pt = pcmd->GetOwner();

		if (pcmdinfo->GetDevice() != this)
			return nucleus::RetCode::COMMAND_GIVEN_TO_WRONG_DEVICE;

		if (pcmdinfo == COMMAND_INSTPTR_NAME(Subshow))
		{
			return nucleus::RetCode::COMMAND_LOADS_SUBSHOW;
		}
		if (pcmdinfo == COMMAND_INSTPTR_NAME(Pause))
		{
			return nucleus::RetCode::COMMAND_PAUSES;
		}

		return nucleus::RetCode::OK;
	}

public:
	nucleus::INucleus *m_pSys;

} FlowControlDevice;

NUPLUGIN_ONACTIVATE_BEGIN()
	psys->DeviceManager()->RegisterDevice((nucleus::IDevice *)&FlowControlDevice);

	return true;
NUPLUGIN_ONACTIVATE_END()

NUPLUGIN_ONDEACTIVATE_BEGIN()
	psys->DeviceManager()->UnregisterDevice((nucleus::IDevice *)&FlowControlDevice);

	return true;
NUPLUGIN_ONDEACTIVATE_END()
