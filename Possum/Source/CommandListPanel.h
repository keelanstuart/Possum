
#pragma once

#include "CommandList.h"
#include "CommandListFilterControl.h"

class CCommandListPanel : public CDockablePane
{
public:
	CCommandListPanel();
	virtual ~CCommandListPanel();

	void AdjustLayout();

	void Update(bool deep);

protected:
	CCommandList m_wndCommandListView;
	CCommandListFilterControl m_wndCommandListFilter;

// Overrides
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnPaint();
	afx_msg void OnSetFocus(CWnd* pOldWnd);

	DECLARE_MESSAGE_MAP()
};

