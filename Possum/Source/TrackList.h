#pragma once

#include "EnvelopeCommandWnd.h"

class CTrackList : public CListCtrl
{
	DECLARE_DYNAMIC(CTrackList)

public:
	CTrackList();
	virtual ~CTrackList();

protected:
//	CTrackTimeline m_Timeline;
	double m_Time;

	CEnvelopeCommandWnd m_Test;

	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnEditCopy();
	afx_msg void OnEditClear();
	afx_msg void OnViewOutput();

	DECLARE_MESSAGE_MAP()
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	afx_msg void OnNMCustomDraw(NMHDR *pNMHDR, LRESULT *pResult);

public:
	virtual void MeasureItem(LPMEASUREITEMSTRUCT pmi);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual void DrawItem(LPDRAWITEMSTRUCT pdi);
};


