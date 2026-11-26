#pragma once

#include "CPicture.h"

// CTrackCommandCtrl

class CTrackCommandCtrl : public CWnd
{
	DECLARE_DYNAMIC(CTrackCommandCtrl)

public:
	CTrackCommandCtrl();
	virtual ~CTrackCommandCtrl();

	void SetIcon(const TCHAR *filename);

protected:

	Gdiplus::Image *m_Icon;

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
};


