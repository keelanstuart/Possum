// Source\PossumButton.cpp : implementation file
//

#include "stdafx.h"
#include "Possum.h"
#include "PossumButton.h"


// CPossumButton

IMPLEMENT_DYNAMIC(CPossumButton, CMFCButton)

CPossumButton::CPossumButton()
{

}

CPossumButton::~CPossumButton()
{
}


BEGIN_MESSAGE_MAP(CPossumButton, CMFCButton)
	//ON_CONTROL_REFLECT(BN_CLICKED, &CPossumButton::OnBnClicked)
	//ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, &CPossumButton::OnNMCustomdraw)
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()



// CPossumButton message handlers




BOOL CPossumButton::Create(LPCTSTR lpszCaption, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
{
	BOOL ret = CMFCButton::Create(lpszCaption, dwStyle, rect, pParentWnd, nID);

	if (ret)
	{
		if (m_Font.CreatePointFont(72, _T("Webdings"), GetDC()))
			SetFont(&m_Font);
	}

	return ret;
}


void CPossumButton::OnBnClicked()
{
}


void CPossumButton::OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);

	CDC dc;
	dc.FromHandle(pNMCD->hdc);

	switch (pNMCD->dwDrawStage)
	{
	    case CDDS_PREPAINT :
		    *pResult = CDRF_NOTIFYITEMDRAW;
			return;
			break;

	    case CDDS_ITEMPREPAINT:
		{
			CFont f;
			f.CreatePointFont(8, _T("Webdings"), &dc);
	        SelectObject(pNMCD->hdc, f);

			*pResult = CDRF_NEWFONT;
			return;
			break;
		}

		case CDDS_ITEMPOSTPAINT:
			break;
	}

	*pResult = 0;
}


BOOL CPossumButton::OnEraseBkgnd(CDC* pDC)
{
	return CMFCButton::OnEraseBkgnd(pDC);
}
