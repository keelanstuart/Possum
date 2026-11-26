#pragma once

#include "TrackListCtrl.h"
#include "TimeBarCtrl.h"
#include <NuTrackSet.h>
#include <NuCommand.h>
#include "PossumDocument.h"

// CPossumDocView

class CPossumDocView : public CView
{
	DECLARE_DYNCREATE(CPossumDocView)

public:
	CPossumDocView();
	virtual ~CPossumDocView();

	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

	void UpdateScrollBar(double max_time);
	void RedrawActiveTimeline();
	void SetActiveTimelineExtents(double start_time, double end_time);

	void SetSelectedTime(double t);
	double GetSelectedTime();
	bool IsShowingSelectedTime();
	void ShowSelectedTime(bool s);

	void SetPlayingTime(double t);
	double GetPlayingTime();
	double GetEndTime();

	double GetDisplayStartTime();
	void SetDisplayStartTime(double t);

	double GetDisplayEndTime();
	void SetDisplayEndTime(double t);

	CPossumDocument *GetDocument() { return static_cast<CPossumDocument *>(m_pDocument); }

protected:
	CTrackListCtrl m_Tracks;
	CTimeBarCtrl m_TimeBar;

	double m_SelectedTime;
	bool m_SelectedTimeVisible;
	double m_PlayingTime;
	bool m_Playing;
	double m_DisplayStartTime;
	double m_DisplayEndTime;


protected:
	virtual void OnInitialUpdate();
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnEditCopy();
	afx_msg void OnEditCut();
	afx_msg void OnEditPaste();
	afx_msg void OnEditClear();
	afx_msg void OnEditDuplicate();
public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT *pResult);
	afx_msg void OnDestroy();
};


