// Source\TrackTimeline.cpp : implementation file
//

#include "stdafx.h"
#include "Possum.h"
#include "TrackTimeline.h"
#include "TrackListCtrl.h"
#include "PossumDocView.h"


// CTrackTimeline

//IMPLEMENT_DYNCREATE(CTrackTimeline, CWnd)
IMPLEMENT_DYNAMIC(CTrackTimeline, CStatic)

CTrackTimeline::CTrackTimeline()
{
	m_FirstTrack = 0;
	m_Scale = 1.0;
}

CTrackTimeline::~CTrackTimeline()
{
}
/*
BOOL CTrackTimeline::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
{
	return CWnd::Create(GetGlobalData()->RegisterWindowClass(_T("CTrackTimeline")), _T(""), dwStyle, rect, pParentWnd, nID, NULL);
}
*/

BEGIN_MESSAGE_MAP(CTrackTimeline, CStatic)
	ON_WM_CREATE()
	ON_WM_PAINT()
END_MESSAGE_MAP()


// CTrackTimeline message handlers


int CTrackTimeline::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	return 0;
}

/*
BOOL CTrackTimeline::PreCreateWindow(CREATESTRUCT& cs)
{
	return CWnd::PreCreateWindow(cs);
}
*/

void CTrackTimeline::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	UINT h = ((CPossumDocView *)GetParent())->m_TrackHeight;

	CRect r, ri;
	GetClientRect(r);
	ri = r;
	ri.top = 0;
	ri.bottom = h;

	dc.FillSolidRect(r, RGB(40, 40, 40));

	size_t maxi = 8;
	CPoint line_start(0, h / 2), line_end(r.Width(), line_start.y);

	for (size_t i = 0; i < maxi; i++)
	{
		dc.FillSolidRect(ri, (i & 1) ? RGB(40, 40, 40) : RGB(45, 45, 45));
		ri.OffsetRect(0, h);

		CPen pen(PS_DOT, 3, (i & 1) ? RGB(45, 45, 45) : RGB(40, 40, 40));
		CPen *oldpen = dc.SelectObject(&pen);
		dc.MoveTo(line_start);
		dc.LineTo(line_end);

		line_start.Offset(0, h);
		line_end.Offset(0, h);
		dc.SelectObject(oldpen);
	}
}
