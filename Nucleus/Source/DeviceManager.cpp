// **************************************************************
// Nucleus Source File
//
// Copyright © 2019-2026, Keelan Sturt


#include "stdafx.h"

#include <NuCommand.h>
#include <NuDevice.h>
#include <NuDeviceManager.h>
#include <NuCommand.h>
#include <NuCommandInfo.h>
#include "DeviceManagerPrivate.h"
#include "PropertyHelper.h"


CDeviceManager::CDeviceManager(nucleus::INucleus *psys)
{
	m_pSys = psys;
}


CDeviceManager::~CDeviceManager()
{
	m_Devs.clear();
	m_MappedDevs.clear();
}

bool GetCfgPath(TCHAR *path, size_t pathsz, nucleus::IDevice *pdev)
{
	if (!path || !pathsz)
		return false;

	*path = _T('\0');
	TCHAR *rootpath;
	HRESULT hr = SHGetKnownFolderPath(FOLDERID_ProgramData, 0, NULL, &rootpath);
	if (SUCCEEDED(hr))
	{
		_tcscat_s(path, MAX_PATH, rootpath);
		PathAddBackslash(path);
		CoTaskMemFree(rootpath);

		_tcscat_s(path, MAX_PATH, _T("Possum"));
		if (!PathIsDirectory(path) && !PathIsDirectory(path))
			if (!CreateDirectory(path, nullptr))
				return false;

		PathAddBackslash(path);
		_tcscat_s(path, MAX_PATH, _T("Nucleus"));
		if (!PathIsDirectory(path) && !PathIsDirectory(path))
			if (!CreateDirectory(path, nullptr))
				return false;

		if (!PathIsDirectory(path) && !PathIsDirectory(path))
			return false;

		PathAddBackslash(path);
	}

	_tcscat_s(path, MAX_PATH, pdev->GetName());
	_tcscat_s(path, MAX_PATH, _T(".cfg"));

	return true;
}

nucleus::RetCode CDeviceManager::RegisterDevice(nucleus::IDevice *pdev)
{
	TDeviceArray::iterator ait = std::find(m_Devs.begin(), m_Devs.end(), pdev);
	if (ait == m_Devs.end())
	{
		m_MappedDevs.insert(TDeviceMapPair(tstring(((nucleus::IDevice *)pdev)->GetName()), (nucleus::IDevice *)pdev));
		m_Devs.push_back((nucleus::IDevice *)pdev);

		TCHAR _filename[MAX_PATH];
		if (GetCfgPath(_filename, MAX_PATH, pdev))
		{
			char *filename;
			LOCAL_TCS2MBCS(_filename, filename);

			tinyxml2::XMLDocument doc;
			if (doc.LoadFile(filename) == tinyxml2::XMLError::XML_SUCCESS)
			{
				{
					TCHAR key[256];
					_stprintf_s(key, 256, _T("%s:properties"), pdev->GetName());
					char* _key;
					LOCAL_TCS2MBCS(key, _key);

					tinyxml2::XMLElement* pxts = doc.FirstChildElement(_key);
					if (pxts)
					{
						tinyxml2::XMLElement* pxp = pxts->FirstChildElement();

						if (pxp)
							LoadPropertySet(pdev->GetProperties(), pxp);
					}
				}

				{
					TCHAR key[256];
					_stprintf_s(key, 256, _T("%s:commands"), pdev->GetName());
					char* _key;
					LOCAL_TCS2MBCS(key, _key);

					tinyxml2::XMLElement* pxts = doc.FirstChildElement(_key);

					tinyxml2::XMLElement* pxt = pxts ? pxts->FirstChildElement("command") : nullptr;
					while (pxt)
					{
						const char* cmdname = pxt->Attribute("name");
						if (!cmdname)
							continue;

						TCHAR* _cmdname;
						LOCAL_MBCS2TCS(cmdname, _cmdname);

						nucleus::ICommandInfo* pci = (nucleus::ICommandInfo*)pdev->GetCommandByName(_cmdname);
						if (pci)
						{
							tinyxml2::XMLElement *pxp = pxt->FirstChildElement("properties");
							if (pxp)
								LoadPropertySet((props::IPropertySet *)(pci->GetDefaultParams()), pxp);
						}

						pxt = pxt->NextSiblingElement("command");
					}
				}
			}
		}

		nucleus::RetCode rc = pdev->Initialize(m_pSys);
		if (rc != nucleus::RetCode::OK)
			return rc;

		return nucleus::RetCode::OK;
	}

	return nucleus::RetCode::DEVICE_EXISTS;
}


nucleus::RetCode CDeviceManager::UnregisterDevice(nucleus::IDevice *pdev)
{
	TDeviceArray::iterator ait = std::find(m_Devs.begin(), m_Devs.end(), pdev);
	if (ait != m_Devs.end())
	{
		m_Devs.erase(ait);

		TDeviceMap::iterator mit = m_MappedDevs.find(pdev->GetName());
		if (mit != m_MappedDevs.end())
			m_MappedDevs.erase(mit);

		TCHAR filename[MAX_PATH];
		if (GetCfgPath(filename, MAX_PATH, pdev))
		{
			genio::ITextOutput* pto = genio::ITextOutput::Create(filename);
			if (pto)
			{
				props::IPropertySet* ps = pdev->GetProperties();
				if (ps)
				{
					pto->PrintF(_T("<!-- %s device properties -->"), pdev->GetName());
					pto->NextLine(1);

					pto->PrintF(_T("<%s:properties>"), pdev->GetName());
					pto->IncIndent();
					pto->NextLine();

					SavePropertySet(ps, pto);

					pto->DecIndent();
					pto->NextLine();
					pto->PrintF(_T("</%s:properties>"), pdev->GetName());

					pto->NextLine(1);
				}

				pto->PrintF(_T("<!-- %s overrides for command parameters (delete as desired to restore defaults) -->"), pdev->GetName());
				pto->NextLine(1);

				pto->PrintF(_T("<%s:commands>"), pdev->GetName());
				pto->IncIndent();

				for (size_t i = 0, maxi = pdev->GetNumCommands(); i < maxi; i++)
				{
					pto->NextLine(1);
					const nucleus::ICommandInfo* pci = pdev->GetCommand(i);

					pto->PrintF(_T("<command name=\"%s\">"), pci->GetName());
					pto->IncIndent();
					pto->NextLine();

					SavePropertySet(pci->GetDefaultParams(), pto);

					pto->DecIndent();
					pto->NextLine();
					pto->Print(_T("</command>"));
				}

				pto->DecIndent();
				pto->NextLine(1);
				pto->PrintF(_T("</%s:commands>"), pdev->GetName());

				pto->Flush();

				pto->Release();
			}
		}

		pdev->Shutdown();

		return nucleus::RetCode::OK;
	}

	return nucleus::RetCode::DEVICE_EXISTS;
}


bool CDeviceManager::IsDeviceRegistered(const nucleus::IDevice *pdev)
{
	return (std::find(m_Devs.cbegin(), m_Devs.cend(), pdev) == m_Devs.cend()) ? false : true;
}


size_t CDeviceManager::GetDeviceCount()
{
	return m_Devs.size();
}


nucleus::IDevice *CDeviceManager::GetDevice(size_t idx)
{
	if (idx < m_Devs.size())
		return m_Devs[idx];

	return nullptr;
}


nucleus::IDevice *CDeviceManager::GetDeviceByName(const TCHAR *name)
{
	TDeviceMap::iterator mit = m_MappedDevs.find(tstring(name));
	if (mit != m_MappedDevs.end())
		return mit->second;

	return nullptr;
}

void CDeviceManager::Update(double time)
{
	for (TDeviceArray::const_iterator it = m_Devs.cbegin(), last_it = m_Devs.cend(); it != last_it; it++)
	{
		(*it)->Update(time);
	}
}
