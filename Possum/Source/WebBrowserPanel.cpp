#include "stdafx.h"
#include "WebBrowserPanel.h"
#include "Resource.h"
#include "Possum.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CWebBrowserPanel::CWebBrowserPanel()
{
}

CWebBrowserPanel::~CWebBrowserPanel()
{
}

BEGIN_MESSAGE_MAP(CWebBrowserPanel, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_CONTEXTMENU()
	ON_WM_PAINT()
	ON_WM_SETFOCUS()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CClassView message handlers

int CWebBrowserPanel::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rectDummy;
	rectDummy.SetRectEmpty();

	// Create views:
	const DWORD dwViewStyle = WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;


	if (!m_wndBrowser.Create(_T("WebBrowser"), dwViewStyle, rectDummy, this, 1))
	{
		TRACE0("Failed to create Product View\n");
		return -1;      // fail to create
	}

	OnChangeVisualStyle();

	return 0;
}

void CWebBrowserPanel::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	AdjustLayout();
}


void CWebBrowserPanel::AdjustLayout()
{
	if (GetSafeHwnd() == NULL)
	{
		return;
	}

	CRect rectClient;
	GetClientRect(rectClient);

	if (m_wndBrowser.GetSafeHwnd())
		m_wndBrowser.SetWindowPos(NULL, rectClient.left + 1, rectClient.top + 1, rectClient.Width() - 2, rectClient.Height() - 2, SWP_NOACTIVATE | SWP_NOZORDER);
}

BOOL CWebBrowserPanel::PreTranslateMessage(MSG* pMsg)
{
	return CDockablePane::PreTranslateMessage(pMsg);
}

void CWebBrowserPanel::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	if (m_wndBrowser.GetSafeHwnd())
	{
		CRect rectTree;
		m_wndBrowser.GetWindowRect(rectTree);
		ScreenToClient(rectTree);

		rectTree.InflateRect(1, 1);
		dc.Draw3dRect(rectTree, ::GetSysColor(COLOR_3DSHADOW), ::GetSysColor(COLOR_3DSHADOW));
	}
}

void CWebBrowserPanel::OnSetFocus(CWnd* pOldWnd)
{
	CDockablePane::OnSetFocus(pOldWnd);

	if (m_wndBrowser.GetSafeHwnd())
	{
		m_wndBrowser.SetFocus();
	}
}

void CWebBrowserPanel::OnChangeVisualStyle()
{
}
