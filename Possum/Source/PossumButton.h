#pragma once


// CPossumButton

class CPossumButton : public CMFCButton
{
	DECLARE_DYNAMIC(CPossumButton)

public:
	CPossumButton();
	virtual ~CPossumButton();

protected:
	CFont m_Font;

	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL Create(LPCTSTR lpszCaption, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
	afx_msg void OnBnClicked();
	afx_msg void OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};


