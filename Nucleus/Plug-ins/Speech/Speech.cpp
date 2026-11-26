// Speech.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "Speech.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include <Nucleus.h>
#include <NuPlugin.h>
#include <NuDevice.h>
#include <NuDeviceManager.h>
#include <NuCommandInfo.h>
#include <NuCommand.h>

#include <sapi.h>
#include "sphelper.h"


using namespace std;


NUPLUGIN_NAME("Speech Synthesis");
NUPLUGIN_SHORTNAME("Speech");
NUPLUGIN_AUTHOR("Keelan Stuart");
NUPLUGIN_WEBSITE("");
NUPLUGIN_DESC("Provides speech synthesis capabilities");
NUPLUGIN_COPYRIGHT("2019-2026, Keelan Stuart");
NUPLUGIN_VERSION(1000);
NUPLUGIN_NUCLEUSVERSION();		// required to determine if this plug-in is compatible with the version of Nucleus that's loading it
NUPLUGIN_GETPLATFORM();
NUPLUGIN_ISUNICODE();


class CSpeechDevice : nucleus::IDevice
{
protected:

	BEGIN_COMMAND_DEFINITION(Say, "Says the given [Text]")
		AddParam(_T("Text"),			'TEXT', _T("This is the text to be spoken."));
		AddParam(_T("Gender"),			'GNDR', _T("Male,Female"), (size_t)0);
		AddParam(_T("Duration"),		'DURA', 10.0f, props::IProperty::PA_GENERIC, props::IProperty::PROPFLAG(props::IProperty::EPropFlag::HIDDEN));
		AddParam(_T("Icon"),			'ICON', _T("./icons/icon_say_color_normal.png"), props::IProperty::PA_FILENAME, props::IProperty::PROPFLAG(props::IProperty::EPropFlag::HIDDEN));
	END_COMMAND_DEFINITION(Say);

	COMMAND_REGISTRY();

public:
	CSpeechDevice()
	{
		m_pSys = nullptr;

		REGISTER_COMMAND(Say);
	}

	virtual ~CSpeechDevice()
	{
		RELEASE_COMMANDS();

		//::CoUninitialize();
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

	/// Performs any updates that the device may require, giving it a chance to poll, etc.
	virtual void Update(double time)
	{

	}

	virtual nucleus::RetCode Reset()
	{
		return nucleus::RetCode::OK;
	}

	virtual nucleus::RetCode SetPaused(bool pause)
	{
		return nucleus::RetCode::OK;
	}

	/// Reports the status of the device, given then 
	virtual nucleus::IDevice::Status GetStatus() const
	{
		return Ready;
	}

	/// Returns the name of the device
	virtual const TCHAR *GetName() const
	{
		return _T("Speech");
	}

	/// Returns a description of the device
	virtual const TCHAR *GetDescription() const
	{
		return _T("Provides speech synthesis capabilities.");
	}

	/// Causes the device to take action; executes the given command with a set of arguments
	virtual nucleus::RetCode ExecuteCommand(const nucleus::ICommand *pcmd, double time, nucleus::ICommand **pcmd_undo = nullptr)
	{
		const nucleus::ICommandInfo *pcmdinfo = pcmd->GetCommandInfo();
		const nucleus::ITrack *pt = pcmd->GetOwner();
		const props::IPropertySet *params = pcmd->GetParams();

		if (pcmdinfo->GetDevice() != this)
			return nucleus::RetCode::COMMAND_GIVEN_TO_WRONG_DEVICE;

		if (pcmdinfo == COMMAND_INSTPTR_NAME(Say))
		{
			props::IProperty *pt = params->GetPropertyById('TEXT');
			props::IProperty *pg = params->GetPropertyById('GNDR');

			HRESULT hr = S_OK;
			CComPtr <ISpVoice>	cpVoice;
			CComPtr<IEnumSpObjectTokens> voice_tokens;

			// do this here because of thread pooling
			::CoInitialize(NULL);

			if (S_OK == cpVoice.CoCreateInstance(CLSID_SpVoice))
			{
				HRESULT hr_spet = SpEnumTokens(SPCAT_VOICES, NULL, NULL, &voice_tokens);
				if (S_OK == hr_spet)
				{
					ULONG voice_count, i = 0;
					hr = voice_tokens->GetCount(&voice_count);

					for (ULONG i = 0; i < voice_count; i++)
					{
						CComPtr<ISpObjectToken> voice_token;

						if (S_OK != voice_tokens->Next(1, &voice_token, NULL))
							continue;

						WCHAR *description;
						if (S_OK != SpGetDescription(voice_token, &description))
							continue;

						const TCHAR kAttributesKey[] = L"Attributes";
						const TCHAR kGenderValue[] = L"Gender";
						const TCHAR kLanguageValue[] = L"Language";

						CComPtr<ISpDataKey> attributes;
						if (S_OK != voice_token->OpenKey(kAttributesKey, &attributes))
							continue;

						WCHAR *language;
						if (S_OK != attributes->GetStringValue(kLanguageValue, &language))
							continue;

						WCHAR *uri;
						voice_token->GetId(&uri);

						TCHAR *gender_s;
						if (S_OK == attributes->GetStringValue(kGenderValue, &gender_s))
						{
							if (!_tcsicmp(gender_s, pg->AsString()))
							{
								hr = cpVoice->Speak(pt->AsString(), SPF_DEFAULT, NULL);
							}
						}

						voice_token.Release();
						attributes.Release();
						// NOTE:  IEnumSpObjectTokens::Next will *overwrite* the pointer; must manually release
					}
				}
			}
		}

		return nucleus::RetCode::OK;
	}

public:
	nucleus::INucleus *m_pSys;

} SpeechDevice;

NUPLUGIN_ONACTIVATE_BEGIN()
	psys->DeviceManager()->RegisterDevice((nucleus::IDevice *)&SpeechDevice);

	return true;
NUPLUGIN_ONACTIVATE_END()

NUPLUGIN_ONDEACTIVATE_BEGIN()
	psys->DeviceManager()->UnregisterDevice((nucleus::IDevice *)&SpeechDevice);

	return true;
NUPLUGIN_ONDEACTIVATE_END()
