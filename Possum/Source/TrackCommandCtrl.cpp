// TrackCommandCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "Possum.h"
#include "CPicture.h"
#include "TrackCommandCtrl.h"


// CTrackCommandCtrl

IMPLEMENT_DYNAMIC(CTrackCommandCtrl, CWnd)

CTrackCommandCtrl::CTrackCommandCtrl()
{
	m_Icon = nullptr;
}

CTrackCommandCtrl::~CTrackCommandCtrl()
{
}

void CTrackCommandCtrl::SetIcon(const TCHAR *filename)
{
	m_Icon = theApp.m_PicMan.GetPic(filename);
}


BEGIN_MESSAGE_MAP(CTrackCommandCtrl, CWnd)
	ON_WM_PAINT()
END_MESSAGE_MAP()



// CTrackCommandCtrl message handlers




void CTrackCommandCtrl::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	CRect r;
	GetClientRect(r);

	CRect rr(r);
	rr.DeflateRect(0, 4, 0, 4);
	CPoint pr(4, 4);

	CBrush barbrush(RGB(30, 30, 30));
	CGdiObject *o = dc.SelectObject(&barbrush);

	dc.RoundRect(rr, pr);

	if (m_Icon)
	{
		m_Icon->Draw(&dc, 0, (r.Height() - m_Icon->GetHeight()) / 2, 0, false);
	}

	dc.SelectObject(o);
}
