#include "stdafx.h"
#include "PicManager.h"
#include <memory>
#include <Shlwapi.h>
#include <gdiplus.h>

#pragma comment(lib, "Shlwapi.lib")


CPicManager::CPicManager()
{

}

CPicManager::~CPicManager()
{
	Cleanup();
}

void CPicManager::Cleanup()
{
	for (TPicMap::iterator it = m_Pics.begin(), lastit = m_Pics.end(); it != lastit; it++)
	{
		if (it->second)
			delete it->second;
	}

	m_Pics.clear();
}

Gdiplus::Image *CPicManager::GetPic(const TCHAR *filename)
{
	TPicMap::iterator it = m_Pics.find(filename);
	if (it != m_Pics.end())
	{
		return it->second;
	}

	Gdiplus::Image *pimg = Gdiplus::Image::FromFile(filename, true);
	std::pair<TPicMap::iterator, bool> insret = m_Pics.insert(TPicMap::value_type(filename, pimg));

	if (!insret.second)
	{
		delete pimg;
		return nullptr;
	}

	return pimg;
}

Gdiplus::Image *CPicManager::GetResPic(UINT resource_id)
{
	TCHAR filename[64];
	_stprintf_s(filename, _T("#%06d"), resource_id);

	TPicMap::iterator it = m_Pics.find(filename);
	if (it != m_Pics.end())
	{
		return it->second;
	}

	Gdiplus::Image *pimg = nullptr;

	HINSTANCE module = AfxGetResourceHandle();
	HRSRC resource = ::FindResource(module, MAKEINTRESOURCE(resource_id), RT_RCDATA);
	if (resource)
	{
		DWORD size = ::SizeofResource(module, resource);
		HGLOBAL handle = ::LoadResource(module, resource);
		if (handle && size)
		{
			const BYTE* data = static_cast<const BYTE *>(::LockResource(handle));
			if (data)
			{
				IStream* stream = ::SHCreateMemStream(data, size);
				if (stream)
				{
					Gdiplus::Bitmap *decoded = Gdiplus::Bitmap::FromStream(stream);

					if (decoded && decoded->GetLastStatus() == Gdiplus::Ok)
					{
						// Make an independent 32-bit bitmap.
						pimg = decoded->Clone(0, 0, decoded->GetWidth(), decoded->GetHeight(), PixelFormat32bppARGB);

						std::pair<TPicMap::iterator, bool> insret = m_Pics.insert(TPicMap::value_type(filename, pimg));

						if (!insret.second)
						{
							delete pimg;
							return nullptr;
						}
					}

					delete decoded;
					stream->Release();
				}
			}
		}
	}

	return pimg;
}
