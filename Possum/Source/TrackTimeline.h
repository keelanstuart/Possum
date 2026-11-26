#pragma once



// CTrackTimeline view

class CTrackTimeline : public CWnd
{
	//DECLARE_DYNCREATE(CTrackTimeline)
	DECLARE_DYNAMIC(CTrackTimeline)

protected:

public:
	CTrackTimeline();           // protected constructor used by dynamic creation
	virtual ~CTrackTimeline();

protected:

	UINT m_uTrackHeight;
	size_t m_FirstTrack;

	double m_Scale;

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
};


