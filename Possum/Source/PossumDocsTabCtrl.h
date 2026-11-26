#pragma once


// CPossumDocsTabCtrl

class CPossumDocsTabCtrl : public CMFCTabCtrl
{
	DECLARE_DYNAMIC(CPossumDocsTabCtrl)

public:
	CPossumDocsTabCtrl();
	virtual ~CPossumDocsTabCtrl();

	virtual void OnChangeTabs();

protected:
	DECLARE_MESSAGE_MAP()

public:
	afx_msg BOOL OnEraseBkgnd(CDC *pDC);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	afx_msg void OnClose();
};


