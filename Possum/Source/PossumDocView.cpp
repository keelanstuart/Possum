// Source\PossumDocView.cpp : implementation file
//

#include "stdafx.h"
#include "Possum.h"
#include "PossumDocView.h"
#include <NuTrack.h>
#include <NuCommandInfo.h>
#include <NuCommand.h>
#include <NuDevice.h>
#include "PossumFrame.h"


// CPossumDocView

IMPLEMENT_DYNCREATE(CPossumDocView, CView)

CPossumDocView::CPossumDocView()
{
	m_SelectedTime = 0.0;
	m_PlayingTime = 0.0;
	m_SelectedTimeVisible = false;
	m_Playing = false;

}

CPossumDocView::~CPossumDocView()
{
}

BEGIN_MESSAGE_MAP(CPossumDocView, CView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_HSCROLL()
	ON_WM_PAINT()
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_WM_ERASEBKGND()
	ON_COMMAND(ID_EDIT_COPY, &CPossumDocView::OnEditCopy)
	ON_COMMAND(ID_EDIT_CUT, &CPossumDocView::OnEditCut)
	ON_COMMAND(ID_EDIT_PASTE, &CPossumDocView::OnEditPaste)
	ON_COMMAND(ID_EDIT_CLEAR, &CPossumDocView::OnEditClear)
	ON_COMMAND(ID_EDIT_DUPLICATE, &CPossumDocView::OnEditDuplicate)
END_MESSAGE_MAP()

// The frame routes Edit menu commands to its active CView; forward them
// to the timeline child, which owns the selection and clipboard operations.
void CPossumDocView::OnEditCopy()      { m_Tracks.CopySelection(); }
void CPossumDocView::OnEditCut()       { m_Tracks.CutSelection(); }
void CPossumDocView::OnEditPaste()     { m_Tracks.PasteSelection(); }
void CPossumDocView::OnEditDuplicate() { m_Tracks.DuplicateSelection(); }
void CPossumDocView::OnEditClear()     { m_Tracks.SendMessage(WM_COMMAND, ID_EDIT_CLEAR); }

// CPossumDocView message handlers

int CPossumDocView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect r;
	r.SetRectEmpty();

	const DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN;

	if (!m_Tracks.Create(dwStyle | WS_VSCROLL | WS_TABSTOP, r, this, 10)
        || !m_TimeBar.Create(dwStyle, r, this, 30))
        return -1;

	m_Tracks.DragAcceptFiles();

	UpdateScrollBar(600.0);

	SetDisplayStartTime(0.0);
	SetDisplayEndTime(60.0);

	return 0;
}


void CPossumDocView::OnInitialUpdate()
{
    CView::OnInitialUpdate();
    GetDocument()->SetView(this);
	m_Tracks.RefreshTracks();

}

void CPossumDocView::OnActivateView(BOOL active, CView* activated, CView* deactivated)
{
    CView::OnActivateView(active, activated, deactivated);
    if (active && GetParentFrame())
        AfxGetMainWnd()->PostMessage(WM_POSSUM_MDI_ACTIVE);
}

void CPossumDocView::OnDestroy()
{
    if (GetDocument() && GetDocument()->GetView() == this)
        GetDocument()->SetView(nullptr);
    CView::OnDestroy();
}

BOOL CPossumDocView::PreCreateWindow(CREATESTRUCT& cs)
{
	return CView::PreCreateWindow(cs);
}

void CPossumDocView::OnDraw(CDC *pDC)
{
}


void CPossumDocView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);

	int hsh = GetSystemMetrics(SM_CYHSCROLL);
	int vsw = GetSystemMetrics(SM_CXVSCROLL);

	CRect rl, rt, rb;

	rl.SetRectEmpty();
	rt.SetRectEmpty();

	rl.top = hsh;
	rl.bottom = cy;
	rl.right = cx;
	m_Tracks.MoveWindow(rl, FALSE);

	rt.left = m_Tracks.GetHeaderWidth();
	rt.top = 0;
	rt.bottom = hsh;
	rt.right = rt.left + (cx - m_Tracks.GetHeaderWidth());
	m_TimeBar.MoveWindow(rt, FALSE);

	RedrawActiveTimeline();
}


void CPossumDocView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	SCROLLINFO si;
	GetScrollInfo(SB_HORZ, &si);  // get information about the scroll

	bool update_now = true;

	switch(nSBCode)
	{
		case SB_RIGHT:				//Scrolls to the right. 
			break;

		case SB_ENDSCROLL:			//Ends scroll. 
			break;

		case SB_LINELEFT:			//Scrolls one line left. 
			SetScrollPos(SB_HORZ, GetScrollPos(SB_HORZ) - (si.nPage / 10));
			break;

		case SB_LINERIGHT:			//Scrolls one line right. 
			SetScrollPos(SB_HORZ, GetScrollPos(SB_HORZ) + (si.nPage / 10));
			break;

		case SB_PAGELEFT:			//Scrolls one page left. 
			SetScrollPos(SB_HORZ, GetScrollPos(SB_HORZ) - si.nPage);
			break;

		case SB_PAGERIGHT:			//Scrolls one page right
			SetScrollPos(SB_HORZ, GetScrollPos(SB_HORZ) + si.nPage);
			break;

		case SB_THUMBPOSITION:		//The user has dragged the scroll box (thumb) and released the mouse button. The nPos parameter indicates the position of the scroll box at the end of the drag operation. 
			break;

		case SB_THUMBTRACK:			//The user is dragging the scroll box. This message is sent repeatedly until the user releases the mouse button. The nPos parameter indicates the position that the scroll box has been dragged to.
			SetScrollPos(SB_HORZ, nPos);
			update_now = false;
			break;

		case SB_LEFT:				//Scrolls to the left. 
			break;
	}

	double adj = double(GetScrollPos(SB_HORZ)) / 10.0 - m_DisplayStartTime;

	m_DisplayStartTime += adj;
	m_DisplayEndTime += adj;

	RedrawActiveTimeline();
}

void CPossumDocView::UpdateScrollBar(double max_time)
{
	SCROLLINFO si;
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_PAGE | SIF_RANGE;// | SIF_POS;
	si.nMin = 0;
	si.nMax = int(max_time * 10.0);
	si.nPage = UINT(max_time / 5.0);

	SetScrollInfo(SB_HORZ, &si, TRUE);

	RedrawActiveTimeline();
}


void CPossumDocView::OnPaint()
{
	CPaintDC dc(this);

	CRect r, dr;

	GetClientRect(r);

	CRect tbr;
	m_TimeBar.GetWindowRect(tbr);
	ScreenToClient(tbr);

	dr.left = 0; dr.right = tbr.left; dr.top = tbr.top; dr.bottom = tbr.bottom;
	dc.FillSolidRect(dr, RGB(0, 0, 0));

	dr.left = tbr.right; dr.right = r.right; dr.top = tbr.top; dr.bottom = tbr.bottom;
	dc.FillSolidRect(dr, RGB(0, 0, 0));
}

void CPossumDocView::RedrawActiveTimeline()
{
	RedrawWindow(nullptr, nullptr, RDW_NOERASE | RDW_INVALIDATE | RDW_ALLCHILDREN);
}

void CPossumDocView::SetActiveTimelineExtents(double start_time, double end_time)
{
	m_DisplayStartTime = start_time;
	m_DisplayEndTime = end_time;

	RedrawActiveTimeline();
}

void CPossumDocView::OnTimer(UINT_PTR nIDEvent)
{
	switch (nIDEvent)
	{
		case 'DRAW':
			RedrawActiveTimeline();
			break;
	}

	CView::OnTimer(nIDEvent);
}

void CPossumDocView::SetSelectedTime(double t)
{
	if (t < 0.0)
		t = 0.0;

	if (t != m_SelectedTime)
	{
		m_SelectedTime = t;
		theApp.RedrawActiveTimeline();
	}
}

double CPossumDocView::GetSelectedTime()
{
	return m_SelectedTime;
}

void CPossumDocView::SetPlayingTime(double t)
{
	if (t < 0.0)
		t = 0.0;

	if (t != m_PlayingTime)
	{
		m_PlayingTime = t;
		theApp.RedrawActiveTimeline();
	}
}

double CPossumDocView::GetPlayingTime()
{
	return m_PlayingTime;
}

double CPossumDocView::GetEndTime()
{
	return 0.0;
}

bool CPossumDocView::IsShowingSelectedTime()
{
	return m_SelectedTimeVisible;
}

void CPossumDocView::ShowSelectedTime(bool s)
{
	if (s != m_SelectedTimeVisible)
	{
		m_SelectedTimeVisible = s;
		RedrawActiveTimeline();
	}
}

double CPossumDocView::GetDisplayStartTime()
{
	return m_DisplayStartTime;
}

void CPossumDocView::SetDisplayStartTime(double t)
{
	if (t != m_DisplayStartTime)
	{
		m_DisplayStartTime = t;
		RedrawActiveTimeline();
	}
}

double CPossumDocView::GetDisplayEndTime()
{
	return m_DisplayEndTime;
}

void CPossumDocView::SetDisplayEndTime(double t)
{
	if (t != m_DisplayEndTime)
	{
		m_DisplayEndTime = t;
		RedrawActiveTimeline();
	}
}



BOOL CPossumDocView::OnEraseBkgnd(CDC* pDC)
{
	return FALSE; //__super::OnEraseBkgnd(pDC);
}


BOOL CPossumDocView::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT *pResult)
{
	// TODO: Add your specialized code here and/or call the base class

	return CView::OnNotify(wParam, lParam, pResult);
}
