// CommandListFilterControl.cpp : implementation file
//

#include "stdafx.h"
#include "CommandListFilterControl.h"
#include "Possum.h"
#include "FilterEditorDlg.h"


// CCommandListFilterControl

IMPLEMENT_DYNAMIC(CCommandListFilterControl, CWnd)

CCommandListFilterControl::CCommandListFilterControl()
{
}

CCommandListFilterControl::~CCommandListFilterControl()
{
}

#define IDC_FILTERS_DROPDOWN		10
#define IDC_FILTERS_CFGBUTTON		20

BEGIN_MESSAGE_MAP(CCommandListFilterControl, CWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_FILTERS_CFGBUTTON, OnCfgClicked)
	ON_CBN_SELCHANGE(IDC_FILTERS_DROPDOWN, OnFilterChanged)
END_MESSAGE_MAP()



// CCommandListFilterControl message handlers

void CCommandListFilterControl::FillFilterDropdown()
{
	if (!m_ddFilters.GetSafeHwnd())
		return;

	m_ddFilters.ResetContent();

	m_ddFilters.AddString(_T("Everything"));
	m_ddFilters.SetCurSel(0);

	for (CPossum::TFilterMap::const_iterator it = theApp.m_FilterMap.cbegin(), last_it = theApp.m_FilterMap.cend(); it != last_it; it++)
	{
		m_ddFilters.AddString(it->first.c_str());
	}
}


int CCommandListFilterControl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if(CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	NONCLIENTMETRICS metrics{};
	metrics.cbSize = sizeof(metrics);
	afxGlobalData.GetNonClientMetrics(metrics);

	m_Font.CreateFontIndirect(&metrics.lfMenuFont);

	CRect r;
	r.SetRectEmpty();
	const DWORD vs = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

	if (!m_ddFilters.Create(vs | CBS_DROPDOWNLIST, r, this, IDC_FILTERS_DROPDOWN) ||
		!m_butCfgFilters.Create(_T("..."), vs | BS_PUSHBUTTON, r, this, IDC_FILTERS_CFGBUTTON))
	{
		return -1;
	}

	if (m_Font.GetSafeHandle() != NULL)
	{
		m_ddFilters.SetFont(&m_Font);
		m_butCfgFilters.SetFont(&m_Font);
	}

	FillFilterDropdown();

	return 0;
}


void CCommandListFilterControl::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	CRect rc(0, 0, (cx * 9) / 10, cy);
	CRect rb(0, 0, (cx / 10) - 1, cy);
	rb.OffsetRect(rc.Width() + 1, 0);

	m_ddFilters.SetWindowPos(NULL, rc.left, rc.top, rc.Width(), rc.Height(), SWP_NOACTIVATE | SWP_NOZORDER);
	m_butCfgFilters.SetWindowPos(NULL, rb.left, rb.top, rb.Width(), rb.Height(), SWP_NOACTIVATE | SWP_NOZORDER);
}


BOOL CCommandListFilterControl::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext)
{
	return CWnd::Create(GetGlobalData()->RegisterWindowClass(_T("CCommandListFilterCtrl")), _T(""), dwStyle, rect, pParentWnd, nID, pContext);
}

void CCommandListFilterControl::OnCfgClicked()
{
	CFilterEditorDlg fed;

	if (fed.DoModal() == IDOK)
	{
		FillFilterDropdown();
	}
}

void CCommandListFilterControl::OnFilterChanged()
{
	theApp.m_ActiveCommandFilter.clear();

	if (m_ddFilters.GetCurSel() > 0)
	{
		CString s;
		m_ddFilters.GetWindowText(s);
		CPossum::TFilterMap::const_iterator it = theApp.m_FilterMap.find((LPCTSTR)s);
		if (it != theApp.m_FilterMap.cend())
		{
			theApp.m_ActiveCommandFilter = it->second;
		}
	}

	theApp.RefreshCommandList(true);
}

