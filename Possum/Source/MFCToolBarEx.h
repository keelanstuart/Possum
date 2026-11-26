#pragma once


// CMFCToolBarEx

class CMFCToolBarEx : public CMFCToolBar
{
	DECLARE_DYNAMIC(CMFCToolBarEx)

public:
	CMFCToolBarEx();
	virtual ~CMFCToolBarEx();

protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual void OnAfterChangeParent(CWnd* pWndOldParent);
};


