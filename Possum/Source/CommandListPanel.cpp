
#include "stdafx.h"
#include "Possum.h"
#include "CommandListPanel.h"
#include "Resource.h"

#include <NuDevice.h>
#include <NuCommand.h>


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCommandListPanel::CCommandListPanel()
{
}

CCommandListPanel::~CCommandListPanel()
{
}

BEGIN_MESSAGE_MAP(CCommandListPanel, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_CONTEXTMENU()
	ON_WM_PAINT()
	ON_WM_SETFOCUS()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCommandListPanel message handlers

int CCommandListPanel::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect r;
	r.SetRectEmpty();

	// Create views:
	const DWORD vs = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

	if (!m_wndCommandListView.Create(vs | TVS_HASBUTTONS, r, this, ID_VIEW_COMMANDS) ||
		!m_wndCommandListFilter.Create(vs, r, this, ID_VIEW_COMMANDS + 1))
	{
		TRACE0("Failed to create all controls\n");
		return -1;      // fail to create
	}

	return 0;
}

void CCommandListPanel::AdjustLayout()
{
	if (GetSafeHwnd() == NULL)
	{
		return;
	}

	CRect r;
	GetClientRect(r);

	r.top = 24;
	r.DeflateRect(1, 1, 1, 1);
	m_wndCommandListView.SetWindowPos(NULL, r.left, r.top, r.Width(), r.Height(), SWP_NOACTIVATE | SWP_NOZORDER);

	r.bottom = r.top - 1;
	r.top = 1;
	m_wndCommandListFilter.SetWindowPos(NULL, r.left, r.top, r.Width(), r.Height(), SWP_NOACTIVATE | SWP_NOZORDER);
}

void CCommandListPanel::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);

	AdjustLayout();
}


void CCommandListPanel::OnContextMenu(CWnd* pWnd, CPoint point)
{
}

BOOL CCommandListPanel::PreTranslateMessage(MSG* pMsg)
{
	return CDockablePane::PreTranslateMessage(pMsg);
}


void CCommandListPanel::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	CRect rectTree;
	m_wndCommandListView.GetWindowRect(rectTree);
	ScreenToClient(rectTree);

	rectTree.InflateRect(1, 1);
	dc.Draw3dRect(rectTree, ::GetSysColor(COLOR_3DSHADOW), ::GetSysColor(COLOR_3DSHADOW));
}

void CCommandListPanel::OnSetFocus(CWnd* pOldWnd)
{
	CDockablePane::OnSetFocus(pOldWnd);

	m_wndCommandListView.SetFocus();
}

void CCommandListPanel::Update(bool deep)
{
	m_wndCommandListView.RefreshItems(deep);
}
