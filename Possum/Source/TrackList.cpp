// Source\TrackTimeline.cpp : implementation file
//

#include "stdafx.h"
#include "Possum.h"
#include "TrackList.h"
#include "TrackTimeline.h"
#include "PossumDocView.h"

// CTrackList

IMPLEMENT_DYNAMIC(CTrackList, CListCtrl)

CTrackList::CTrackList()
{
}

CTrackList::~CTrackList()
{
}

BEGIN_MESSAGE_MAP(CTrackList, CListCtrl)
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_CLEAR, OnEditClear)
	ON_COMMAND(ID_VIEW_OUTPUTWND, OnViewOutput)
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnNMCustomDraw)
	ON_WM_MEASUREITEM_REFLECT()
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_DRAWITEM()
END_MESSAGE_MAP()


void CTrackList::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	CMenu menu;
	menu.LoadMenu(IDR_OUTPUT_POPUP);

	CMenu* pSumMenu = menu.GetSubMenu(0);

	if (AfxGetMainWnd()->IsKindOf(RUNTIME_CLASS(CMDIFrameWndEx)))
	{
		CMFCPopupMenu* pPopupMenu = new CMFCPopupMenu;

		if (!pPopupMenu->Create(this, point.x, point.y, (HMENU)pSumMenu->m_hMenu, FALSE, TRUE))
			return;

		((CMDIFrameWndEx*)AfxGetMainWnd())->OnShowPopupMenu(pPopupMenu);
		UpdateDialogControls(this, FALSE);
	}

	SetFocus();
}

void CTrackList::OnEditCopy()
{
	MessageBox(_T("Copy output"));
}

void CTrackList::OnEditClear()
{
	MessageBox(_T("Clear output"));
}

void CTrackList::OnViewOutput()
{
	CDockablePane* pParentBar = DYNAMIC_DOWNCAST(CDockablePane, GetOwner());
	CMDIFrameWndEx* pMainFrame = DYNAMIC_DOWNCAST(CMDIFrameWndEx, GetTopLevelFrame());

	if (pMainFrame != NULL && pParentBar != NULL)
	{
		pMainFrame->SetFocus();
		pMainFrame->ShowPane(pParentBar, FALSE, FALSE, FALSE);
		pMainFrame->RecalcLayout();
	}
}


BOOL CTrackList::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	return CListCtrl::OnNotify(wParam, lParam, pResult);
}


void CTrackList::MeasureItem(LPMEASUREITEMSTRUCT pmi)
{
	CRect r;
	GetClientRect(r);
	pmi->itemHeight = ((CPossumDocView *)GetParent())->m_TrackHeight;
	pmi->itemWidth = r.Width();
}

void CTrackList::OnNMCustomDraw(NMHDR *pNMHDR, LRESULT *pResult)
{
#if 1
	LPNMLVCUSTOMDRAW pLVCD = reinterpret_cast<LPNMLVCUSTOMDRAW>(pNMHDR);

	if (pLVCD->nmcd.dwDrawStage & CDDS_PREPAINT)
	{
		*pResult = CDRF_NOTIFYITEMDRAW;
		return;
	}

	*pResult = CDRF_SKIPDEFAULT;
#else
	LPNMLVCUSTOMDRAW pLVCD = reinterpret_cast<LPNMLVCUSTOMDRAW>(pNMHDR);

	if (pLVCD->nmcd.dwDrawStage & CDDS_PREPAINT)
	{
		*pResult = CDRF_NOTIFYITEMDRAW;
		return;
	}

	*pResult = CDRF_DODEFAULT;

	int iidx = int(pLVCD->nmcd.dwItemSpec);
	CDC *pdc = CDC::FromHandle(pLVCD->nmcd.hdc);

	CRect cr;
	GetClientRect(cr);

	CRect r;
	GetItemRect(iidx, &r, LVIR_BOUNDS);

	COLORREF clr[2] = 
	{
		 (iidx & 1) ? RGB(40, 40, 40) : RGB(45, 45, 45),
		!(iidx & 1) ? RGB(40, 40, 40) : RGB(45, 45, 45)
	};

	switch (pLVCD->nmcd.dwDrawStage)
	{
		case CDDS_PREERASE:
		{
			*pResult = CDRF_SKIPDEFAULT;

			break;
		}

		case (CDDS_ITEM | CDDS_PREPAINT):
		{
			*pResult = CDRF_SKIPDEFAULT;// CDRF_NOTIFYSUBITEMDRAW;

#if 0
			r.right = cr.right;

			CBrush br(clr[0]);
			pdc->FillRect(&r, &br);

			CString t = GetItemText(iidx, 0);

			CFont dummyfont;
			LOGFONT dummylogfont;
			pdc->SetTextColor(RGB(255, 255, 255));
			pdc->GetCurrentFont()->GetLogFont(&dummylogfont);
			dummyfont.CreateFontIndirect(&dummylogfont);
			CFont *oldfont = pdc->SelectObject(&dummyfont);

			r.left += 10;
			pdc->DrawTextEx(t, r, DT_SINGLELINE | DT_END_ELLIPSIS | DT_LEFT | DT_VCENTER, NULL);

			pdc->SelectObject(&oldfont);

			CPen pen(PS_DOT, 3, clr[1]);
			CPen *oldpen = pdc->SelectObject(&pen);

			CPoint p1(GetColumnWidth(0), r.CenterPoint().y);
			CPoint p2(r.right, p1.y);

			pdc->MoveTo(p1);
			pdc->LineTo(p2);

			pdc->SelectObject(oldpen);

			*pResult = CDRF_SKIPDEFAULT;
#endif
			break;
		}

		case CDDS_ITEMPOSTPAINT:
		{
			CRect r;
			GetItemRect(iidx, &r, LVIR_BOUNDS);

			//pdc->DrawFocusRect(r);

			*pResult = CDRF_SKIPDEFAULT;

			break;
		}
	}
#endif
}

BOOL CTrackList::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style |= LVS_OWNERDRAWFIXED | LVS_NOCOLUMNHEADER;
	cs.dwExStyle |= LVS_EX_FULLROWSELECT;

	return CListCtrl::PreCreateWindow(cs);
}


int CTrackList::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CListCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect r;
	r.SetRectEmpty();

	const DWORD dwStyle = WS_CHILD | WS_VISIBLE;

//	if (!m_Timeline.Create(dwStyle, r, this, 10))
//		return -1;

	r.right = 150; r.bottom = 30; r.OffsetRect(200, 100);
//	if (!m_Test.Create(-1, this))
//		return -1;

	return 0;
}


void CTrackList::OnSize(UINT nType, int cx, int cy)
{
	CListCtrl::OnSize(nType, cx, cy);

	int vsw = GetSystemMetrics(SM_CXVSCROLL);

	SetColumnWidth(1, cx - GetColumnWidth(0) - vsw);

	Invalidate(FALSE);
}


void CTrackList::DrawItem(LPDRAWITEMSTRUCT pdi)
{
	CDC *pdc = CDC::FromHandle(pdi->hDC);

	COLORREF clr[2] =
	{
		(pdi->itemID & 1) ? RGB(40, 40, 40) : RGB(45, 45, 45),
		!(pdi->itemID & 1) ? RGB(40, 40, 40) : RGB(45, 45, 45)
	};

	CRect r = pdi->rcItem;

	CBrush br(clr[0]);
	pdc->FillRect(&r, &br);

	{
		GetSubItemRect(pdi->itemID, 0, LVIR_BOUNDS, r);

		CString t = GetItemText(pdi->itemID, 0);

		CFont dummyfont;
		LOGFONT dummylogfont;
		pdc->SetTextColor(RGB(255, 255, 255));
		pdc->GetCurrentFont()->GetLogFont(&dummylogfont);
		dummyfont.CreateFontIndirect(&dummylogfont);
		CFont *oldfont = pdc->SelectObject(&dummyfont);

		r.left += 10;
		pdc->DrawTextEx(t, r, DT_SINGLELINE | DT_END_ELLIPSIS | DT_LEFT | DT_VCENTER, NULL);

		pdc->SelectObject(&oldfont);
	}

	{
		GetSubItemRect(pdi->itemID, 1, LVIR_BOUNDS, r);

		CPen pen(PS_DOT, 3, clr[1]);
		CPen *oldpen = pdc->SelectObject(&pen);

		CPoint p1(GetColumnWidth(0), r.CenterPoint().y);
		CPoint p2(r.right, p1.y);

		pdc->MoveTo(p1);
		pdc->LineTo(p2);

		pdc->SelectObject(oldpen);
	}
}
