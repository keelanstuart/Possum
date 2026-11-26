
#pragma once

#include "WebBrowserWnd.h"

class CWebBrowserPanel : public CDockablePane
{
public:
	CWebBrowserPanel();
	virtual ~CWebBrowserPanel();

	void AdjustLayout();
	void OnChangeVisualStyle();

	const CWebBrowser &GetBrowser() { return m_wndBrowser; }

protected:
	CWebBrowser m_wndBrowser;

// Overrides
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnPaint();
	afx_msg void OnSetFocus(CWnd* pOldWnd);

	DECLARE_MESSAGE_MAP()
};

