
// PossumAppFrame.cpp : implementation of the CPossumAppFrame class
//

#include "stdafx.h"
#include "Possum.h"

#include "PossumAppFrame.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CPossumAppFrame

IMPLEMENT_DYNCREATE(CPossumAppFrame, CMDIChildWndEx)

BEGIN_MESSAGE_MAP(CPossumAppFrame, CMDIChildWndEx)
END_MESSAGE_MAP()

// CPossumAppFrame construction/destruction

CPossumAppFrame::CPossumAppFrame()
{
	EnableActiveAccessibility();
	// TODO: add member initialization code here
}

CPossumAppFrame::~CPossumAppFrame()
{
}


BOOL CPossumAppFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying the CREATESTRUCT cs
	if( !CMDIChildWndEx::PreCreateWindow(cs) )
		return FALSE;

	return TRUE;
}

// CPossumAppFrame diagnostics

#ifdef _DEBUG
void CPossumAppFrame::AssertValid() const
{
	CMDIChildWndEx::AssertValid();
}

void CPossumAppFrame::Dump(CDumpContext& dc) const
{
	CMDIChildWndEx::Dump(dc);
}
#endif //_DEBUG

// CPossumAppFrame message handlers
