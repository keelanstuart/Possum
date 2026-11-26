#pragma once

#include <NuCommandInfo.h>
#include "PossumPropertyGrid.h"


class CPropertiesToolBar : public CMFCToolBar
{
public:
	virtual void OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL bDisableIfNoHndler)
	{
		CMFCToolBar::OnUpdateCmdUI((CFrameWnd*) GetOwner(), bDisableIfNoHndler);
	}

	virtual BOOL AllowShowOnList() const { return FALSE; }
};

class CPropertiesPanel : public CDockablePane
{
// Construction
public:
	CPropertiesPanel();

	void AdjustLayout();
	void SetActiveProperties(props::IPropertySet *props, bool readonly = false, const TCHAR *title = nullptr, nucleus::ICommandInfo *pci = nullptr, nucleus::IDevice *pd = nullptr);

// Attributes
public:
	void SetVSDotNetLook(BOOL bSet)
	{
		m_wndPropList.SetVSDotNetLook(bSet);
		m_wndPropList.SetGroupNameFullWidth(bSet);
	}

protected:
	CFont m_fntPropList;
	CComboBox m_wndObjectCombo;
	CPropertiesToolBar m_wndToolBar;
	CPossumPropertyGrid m_wndPropList;
	CStatic m_wndTitle;

	props::IPropertySet *m_pProps;
	nucleus::ICommandInfo *m_pActiveCmdInfo;
	nucleus::IDevice *m_pDevice;

// Implementation
public:
	virtual ~CPropertiesPanel();

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnExpandAllProperties();
	afx_msg void OnUpdateExpandAllProperties(CCmdUI* pCmdUI);
	afx_msg void OnSortProperties();
	afx_msg void OnUpdateSortProperties(CCmdUI* pCmdUI);
	afx_msg void OnReset();
	afx_msg void OnUpdateReset(CCmdUI* pCmdUI);
	afx_msg void OnExecuteCommand();
	afx_msg void OnUpdateExecuteCommand(CCmdUI* pCmdUI);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);

	DECLARE_MESSAGE_MAP()

	void InitPropList();
	void SetPropListFont();
public:
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
};

