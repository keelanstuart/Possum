// Source\EnvelopeCommandWnd.cpp : implementation file
//

#include "stdafx.h"
#include "Possum.h"
#include "EnvelopeCommandWnd.h"


// CEnvelopeCommandWnd

IMPLEMENT_DYNAMIC(CEnvelopeCommandWnd, CDialogEx)

CEnvelopeCommandWnd::CEnvelopeCommandWnd()
{

}

CEnvelopeCommandWnd::~CEnvelopeCommandWnd()
{
}


BEGIN_MESSAGE_MAP(CEnvelopeCommandWnd, CScrollBar)
//	ON_WM_PAINT()
END_MESSAGE_MAP()



// CEnvelopeCommandWnd message handlers




BOOL CEnvelopeCommandWnd::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style |= WS_CHILD;

	return __super::PreCreateWindow(cs);
}
