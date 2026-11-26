
#pragma once

#include "PossumDocView.h"
#include "PossumDocsTabCtrl.h"
#include "PossumDocument.h"


class CTrackTimelinePanel : public CDockablePane
{
	// Construction
public:
	CTrackTimelinePanel();

	void UpdateFonts();

	// Attributes
protected:
	CPossumDocsTabCtrl	m_wndTabs;

	typedef std::deque<CPossumDocView *> TTrackDocViewArray;
	TTrackDocViewArray m_TrackTimelines;

	void AdjustHorzScroll(CListBox& wndListBox);

	// Implementation
public:
	bool AddDocument(CPossumDocument *pdoc, bool activate = true);

	void RedrawActiveTimeline();
	void SetActiveTimelineExtents(double start_time, double end_time);
	CPossumDocView *GetActiveDocView();
	CPossumDocView *FindDocViewByTrackSet(const nucleus::ITrackSet *pts);

	virtual ~CTrackTimelinePanel();

protected:
	afx_msg LRESULT OnTabSetActive(WPARAM wParam, LPARAM lParam);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnFileNew();
	afx_msg void OnFileOpen();
	afx_msg void OnFileSave();
	afx_msg void OnFileSaveAs();
	afx_msg void OnFileClose();
	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
	afx_msg void OnMove(int x, int y);
	afx_msg void OnDestroy();
	afx_msg BOOL OnEraseBkgnd(CDC *pDC);
};

