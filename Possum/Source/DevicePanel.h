
#pragma once

#include "DeviceTree.h"

class CDevicePanel : public CDockablePane
{
public:
	CDevicePanel();
	virtual ~CDevicePanel();

	void AdjustLayout();
	void OnChangeVisualStyle();

protected:
	CDeviceTree m_wndDeviceView;
	CImageList m_DeviceViewImages;

	void FillDeviceView();

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

