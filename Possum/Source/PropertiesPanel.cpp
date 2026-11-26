
#include "stdafx.h"

#include "PropertiesPanel.h"
#include "Resource.h"
#include "PossumFrame.h"
#include "Possum.h"
#include "PossumDocument.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// CResourceViewBar

CPropertiesPanel::CPropertiesPanel()
{
	m_pProps = nullptr;
	m_pActiveCmdInfo = nullptr;
	m_pDevice = nullptr;
}

CPropertiesPanel::~CPropertiesPanel()
{
}

BEGIN_MESSAGE_MAP(CPropertiesPanel, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_COMMAND(ID_EXPAND_ALL, OnExpandAllProperties)
	ON_UPDATE_COMMAND_UI(ID_EXPAND_ALL, OnUpdateExpandAllProperties)
	ON_COMMAND(ID_SORTPROPERTIES, OnSortProperties)
	ON_UPDATE_COMMAND_UI(ID_SORTPROPERTIES, OnUpdateSortProperties)
	ON_COMMAND(ID_RESET, OnReset)
	ON_UPDATE_COMMAND_UI(ID_RESET, OnUpdateReset)
	ON_COMMAND(ID_EXECUTECOMMAND, OnExecuteCommand)
	ON_UPDATE_COMMAND_UI(ID_EXECUTECOMMAND, OnUpdateExecuteCommand)
	ON_WM_SETFOCUS()
	ON_WM_SETTINGCHANGE()
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CResourceViewBar message handlers

void CPropertiesPanel::SetActiveProperties(props::IPropertySet* props, bool readonly, const TCHAR* title, nucleus::ICommandInfo* pci, nucleus::IDevice* pd)
{
	m_pActiveCmdInfo = pci;
	m_pDevice = pd;
	m_pProps = props;

	if (m_wndTitle.GetSafeHwnd())
	{
		m_wndTitle.SetWindowTextW(title ? title : _T(""));
	}

	if (m_wndPropList.GetSafeHwnd())
	{
		// CPossumDocument sends this title and a null property set when
		// multiple commands are selected. Preserve the existing public API.
		auto *doc = theApp.m_DocMan.GetActiveDocument();
		if (!props && doc && doc->GetNumSelected() > 1 && title &&
			!_tcscmp(title, _T("[multiple selected]")))
			m_wndPropList.SetMultipleProperties(doc, readonly, title);
		else
			m_wndPropList.SetActiveProperties(props, readonly, title);
	}

	//m_wndToolBar.OnUpdateCmdUI(.UpdateButton(2);
}

void CPropertiesPanel::AdjustLayout()
{
	if (GetSafeHwnd () == NULL || (AfxGetMainWnd() != NULL && AfxGetMainWnd()->IsIconic()))
	{
		return;
	}

	CRect rectClient;
	GetClientRect(rectClient);

	int cyTlb = m_wndToolBar.CalcFixedLayout(FALSE, TRUE).cy;

	m_wndTitle.SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	m_wndToolBar.SetWindowPos(NULL, rectClient.left, rectClient.top + cyTlb, rectClient.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	m_wndPropList.SetWindowPos(NULL, rectClient.left, rectClient.top + cyTlb + cyTlb, rectClient.Width(), rectClient.Height() - cyTlb - cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
}

#define ID_TITLEBG		1
#define ID_TITLE		2
#define ID_PROPLIST		3


int CPropertiesPanel::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rectDummy;
	rectDummy.SetRectEmpty();

	if (!m_wndTitle.Create(_T(""), WS_VISIBLE | WS_CHILD | SS_WORDELLIPSIS | SS_LEFT | SS_CENTERIMAGE | SS_REALSIZECONTROL, rectDummy, this, ID_TITLE))
	{
		return -1;
	}

	if (!m_wndPropList.Create(WS_VISIBLE | WS_CHILD, rectDummy, this, ID_PROPLIST))
	{
		TRACE0("Failed to create Properties Grid \n");
		return -1;      // fail to create
	}

	InitPropList();

	m_wndToolBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE, IDR_PROPERTIES);
	m_wndToolBar.LoadToolBar(IDR_PROPERTIES, 0, 0, TRUE /* Is locked */);
	m_wndToolBar.CleanUpLockedImages();
	m_wndToolBar.LoadBitmap(theApp.m_bHiColorIcons ? IDR_PROPERTIES_256 : IDR_PROPERTIES, 0, 0, TRUE /* Locked */);

	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));
	m_wndToolBar.SetOwner(this);

	m_wndToolBar.EnableToolTips(true);

	// All commands will be routed via this control , not via the parent frame:
	m_wndToolBar.SetRouteCommandsViaFrame(FALSE);

	AdjustLayout();
	return 0;
}

void CPropertiesPanel::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	AdjustLayout();
}

void CPropertiesPanel::OnExpandAllProperties()
{
	m_wndPropList.ExpandAll();
}

void CPropertiesPanel::OnUpdateExpandAllProperties(CCmdUI* /* pCmdUI */)
{
}

void CPropertiesPanel::OnSortProperties()
{
	m_wndPropList.SetAlphabeticMode(!m_wndPropList.IsAlphabeticMode());
}

void CPropertiesPanel::OnUpdateSortProperties(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_wndPropList.IsAlphabeticMode());
}

void CPropertiesPanel::OnReset()
{
	// TODO: Add your command handler code here
}

void CPropertiesPanel::OnUpdateReset(CCmdUI* pCmdUI)
{
	pCmdUI->Enable((m_pActiveCmdInfo != nullptr) || (m_pDevice != nullptr));
}

void CPropertiesPanel::OnExecuteCommand()
{
	if (m_pActiveCmdInfo)
	{
		// TODO: the API changing has made this not work any more... fix? abandon?
		//m_pActiveCmdInfo->GetDevice()->ExecuteCommand(m_pActiveCmdInfo, 0.0f, m_pProps);
	}
}

void CPropertiesPanel::OnUpdateExecuteCommand(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_pActiveCmdInfo != nullptr);
}

void CPropertiesPanel::InitPropList()
{
	SetPropListFont();

	//m_wndPropList.EnableHeaderCtrl(FALSE);
	//m_wndPropList.EnableDescriptionArea();
	m_wndPropList.SetVSDotNetLook();
	m_wndPropList.MarkModifiedProperties();
}

void CPropertiesPanel::OnSetFocus(CWnd* pOldWnd)
{
	CDockablePane::OnSetFocus(pOldWnd);
	m_wndPropList.SetFocus();
}

void CPropertiesPanel::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	CDockablePane::OnSettingChange(uFlags, lpszSection);
	SetPropListFont();
}

void CPropertiesPanel::SetPropListFont()
{
	::DeleteObject(m_fntPropList.Detach());

	LOGFONT lf;
	afxGlobalData.fontRegular.GetLogFont(&lf);

	NONCLIENTMETRICS info;
	info.cbSize = sizeof(info);

	afxGlobalData.GetNonClientMetrics(info);

	lf.lfHeight = info.lfMenuFont.lfHeight;
	lf.lfWeight = info.lfMenuFont.lfWeight;
	lf.lfItalic = info.lfMenuFont.lfItalic;

	m_fntPropList.CreateFontIndirect(&lf);

	m_wndPropList.SetFont(&m_fntPropList);
}


HBRUSH CPropertiesPanel::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDockablePane::OnCtlColor(pDC, pWnd, nCtlColor);

#if 0
	if (nCtlColor == CTLCOLOR_STATIC)
	{
		switch (pWnd->GetDlgCtrlID())
		{
			case ID_TITLE:
			{
				COLORREF c = RGB(40, 40, 40);

				if (m_pProps)
				{
					props::IProperty* pc = m_pProps->GetPropertyById('VCLR');
					if (pc)
						c = (COLORREF)pc->AsInt();
				}

				pDC->SetBkMode(OPAQUE);
				pDC->SetBkColor(c);
				pDC->SetTextColor(RGB(255, 255, 255));
				break;
			}
		}
	}
#endif

	return hbr;
}
