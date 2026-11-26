// MFCToolBarEx.cpp : implementation file
//

#include "stdafx.h"
#include "PossumFrame.h"
#include "MFCToolBarEx.h"


// CMFCToolBarEx

IMPLEMENT_DYNAMIC(CMFCToolBarEx, CMFCToolBar)

CMFCToolBarEx::CMFCToolBarEx()
{
}

CMFCToolBarEx::~CMFCToolBarEx()
{
}


BEGIN_MESSAGE_MAP(CMFCToolBarEx, CMFCToolBar)
END_MESSAGE_MAP()



// CMFCToolBarEx message handlers


// Overrides

void CMFCToolBarEx::OnAfterChangeParent(CWnd* pWndOldParent)
{
	CPossumMainFrame * pFrame = (CPossumMainFrame *)AfxGetMainWnd();

	if (pFrame)
	{
		CRect rect;
		pFrame->GetClientRect(rect);
		pFrame->InvalidateRect(rect);
	}

	CMFCToolBar::OnAfterChangeParent(pWndOldParent);
}
