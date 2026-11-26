// Source\PossumDocsTabCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "Possum.h"
#include "PossumDocsTabCtrl.h"
#include "PossumDocView.h"


// CPossumDocsTabCtrl

IMPLEMENT_DYNAMIC(CPossumDocsTabCtrl, CMFCTabCtrl)

CPossumDocsTabCtrl::CPossumDocsTabCtrl()
{
}

CPossumDocsTabCtrl::~CPossumDocsTabCtrl()
{
}


void CPossumDocsTabCtrl::OnChangeTabs()
{
	CMFCTabCtrl::OnChangeTabs();
}


BEGIN_MESSAGE_MAP(CPossumDocsTabCtrl, CMFCTabCtrl)
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_WM_CLOSE()
END_MESSAGE_MAP()



// CPossumDocsTabCtrl message handlers



BOOL CPossumDocsTabCtrl::OnEraseBkgnd(CDC *pDC)
{
	return FALSE; // CMFCTabCtrl::OnEraseBkgnd(pDC);
}


void CPossumDocsTabCtrl::OnSize(UINT nType, int cx, int cy)
{
	CMFCTabCtrl::OnSize(nType, cx, cy);
}


void CPossumDocsTabCtrl::OnDestroy()
{
	CMFCTabCtrl::OnDestroy();
}


void CPossumDocsTabCtrl::OnClose()
{
	//RemoveAllTabs();

	CMFCTabCtrl::OnClose();
}
