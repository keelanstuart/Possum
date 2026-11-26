#pragma once

#include "CPicture.h"

class CPicManager
{
public:
	CPicManager();
	virtual ~CPicManager();

	Gdiplus::Image *GetPic(const TCHAR *filename);
	Gdiplus::Image *GetResPic(UINT resource_id);

	void Cleanup();

protected:
	typedef std::map<tstring, Gdiplus::Image *> TPicMap;
	TPicMap m_Pics;
};