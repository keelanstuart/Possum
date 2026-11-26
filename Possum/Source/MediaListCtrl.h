#pragma once


// CMediaListCtrl

class CMediaListCtrl : public CListCtrl
{
	DECLARE_DYNAMIC(CMediaListCtrl)

public:
	CMediaListCtrl();
	virtual ~CMediaListCtrl();

	bool NavigateTo(const TCHAR *path);

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
};


