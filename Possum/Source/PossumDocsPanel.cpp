
#include "stdafx.h"

#include "Possum.h"
#include "PossumDocsPanel.h"
#include "Resource.h"
#include "PossumFrame.h"
#include "PossumDocView.h"
#include <NuImporter.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define ADD_DOC_TABID		99999

/////////////////////////////////////////////////////////////////////////////
// COutputBar

CTrackTimelinePanel::CTrackTimelinePanel()
{
}

CTrackTimelinePanel::~CTrackTimelinePanel()
{
}

BEGIN_MESSAGE_MAP(CTrackTimelinePanel, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	//ON_REGISTERED_MESSAGE(AFX_WM_CHANGE_ACTIVE_TAB, OnTabSetActive)
	ON_REGISTERED_MESSAGE(AFX_WM_CHANGING_ACTIVE_TAB, OnTabSetActive)
	//ON_REGISTERED_MESSAGE(AFX_WM_ON_GET_TAB_TOOLTIP, OnTabSetActive)
	ON_WM_ERASEBKGND()
	ON_WM_SIZING()
	ON_WM_MOVE()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

class CDummyWnd : public CWnd
{
	// Construction
public:
	CDummyWnd() { }
	virtual ~CDummyWnd() { }

	virtual BOOL Create(CWnd* pParentWnd, UINT nID)
	{
		CRect rect;
		rect.SetRectEmpty();
		DWORD dwStyle = WS_CHILD;// | WS_VISIBLE;

		return CWnd::Create(GetGlobalData()->RegisterWindowClass(_T("CDummyRDPWnd")), _T(""), dwStyle, rect, pParentWnd, nID, nullptr);
	}

	afx_msg void OnPaint()
	{
	}

	afx_msg BOOL OnEraseBackground()
	{
		return FALSE;
	}

	DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(CDummyWnd, CWnd)
	ON_WM_PAINT()
END_MESSAGE_MAP()

static CDummyWnd s_PlusTabWnd;

int CTrackTimelinePanel::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rectDummy;
	rectDummy.SetRectEmpty();

	// Create tabs window:
	if (!m_wndTabs.Create(CMFCTabCtrl::STYLE_FLAT_SHARED_HORZ_SCROLL, rectDummy, this, 1))
	{
		TRACE0("Failed to create output tab window\n");
		return -1;      // fail to create
	}

	m_wndTabs.EnableActivateLastActive(FALSE);
	m_wndTabs.SetFlatFrame(FALSE, FALSE);
	m_wndTabs.EnableTabDocumentsMenu(FALSE);

	UpdateFonts();

#if 1
	s_PlusTabWnd.Create(this, ADD_DOC_TABID);
	m_wndTabs.AddTab(&s_PlusTabWnd, _T("+"), ADD_DOC_TABID, FALSE);
#endif

	//AddDocument(theApp.m_DocMan.CreateDocument());

	return 0;
}

void CTrackTimelinePanel::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);

	// Tab control should cover the whole client area:
	m_wndTabs.SetWindowPos(NULL, -1, -1, cx, cy, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
}

void CTrackTimelinePanel::AdjustHorzScroll(CListBox& wndListBox)
{
	CClientDC dc(this);
	CFont* pOldFont = dc.SelectObject(&afxGlobalData.fontRegular);

	int cxExtentMax = 0;

	for (int i = 0; i < wndListBox.GetCount(); i++)
	{
		CString strItem;
		wndListBox.GetText(i, strItem);

		cxExtentMax = max(cxExtentMax, (int)dc.GetTextExtent(strItem).cx);
	}

	wndListBox.SetHorizontalExtent(cxExtentMax);
	dc.SelectObject(pOldFont);
}

void CTrackTimelinePanel::UpdateFonts()
{
	//m_List.SetFont(&afxGlobalData.fontRegular);s
}

void CTrackTimelinePanel::RedrawActiveTimeline()
{
	CPossumDocView *pdv = dynamic_cast<CPossumDocView *>(m_wndTabs.GetActiveWnd());
	if (pdv)
		pdv->RedrawActiveTimeline();
}

void CTrackTimelinePanel::SetActiveTimelineExtents(double start_time, double end_time)
{
	CPossumDocView *pdv = dynamic_cast<CPossumDocView *>(m_wndTabs.GetActiveWnd());
	if (pdv)
		pdv->SetActiveTimelineExtents(start_time, end_time);
}

void __cdecl ActivateTab(LPVOID param0, LPVOID param1, size_t task_number)
{
	ASSERT(0 && "this does get called, hmmm");
//	CPossumDocsTabCtrl *pt = (CPossumDocsTabCtrl *)param0;
//	pt->SetActiveTab((int)param1);
}

bool CTrackTimelinePanel::AddDocument(CPossumDocument *pdoc, bool activate)
{
	CRect rectDummy;
	rectDummy.SetRectEmpty();

	// Create output panes:
	const DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN;

	CPossumDocView *pdv = new CPossumDocView();
	if (!pdv->Create(dwStyle, rectDummy, &m_wndTabs, pdoc->GetId(), pdoc))
	{
		delete pdv;
		TRACE0("Failed to create doc view window\n");
		return false;      // fail to create
	}
	m_TrackTimelines.push_back(pdv);

	pdoc->SetView(pdv);

	// Attach list windows to tab:
	m_wndTabs.AddTab(pdv, pdoc->GetTitle());
	if (activate)
	{
		int idx = m_wndTabs.GetTabFromHwnd(pdv->GetSafeHwnd());
		m_wndTabs.SetActiveTab(idx);
	}

	return true;
}

LRESULT CTrackTimelinePanel::OnTabSetActive(WPARAM wParam, LPARAM lParam)
{
	LRESULT ret = FALSE;

	const int idx = (int)wParam;
	CString label;
	m_wndTabs.GetTabLabel(idx, label);

	CPossumDocument *pdoc;

	// if the tab that's been activated is the '+', then make a new tab...
	if (label == _T("+"))
	{
		pdoc = theApp.m_DocMan.CreateDocument();
		AddDocument(pdoc);
		m_wndTabs.RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_NOERASE | RDW_UPDATENOW | RDW_ALLCHILDREN);
		ret = TRUE;
	}
	else
	{
		pdoc = theApp.m_DocMan.FindDocument(label);
	}

	if (pdoc)
		theApp.SetActiveDocument(pdoc);

	return ret;
}


void CTrackTimelinePanel::OnFileNew()
{
	CPossumDocument *pdoc = theApp.m_DocMan.CreateDocument();
	if (pdoc)
	{
		theApp.SetActiveDocument(pdoc);
		AddDocument(pdoc);
		m_wndTabs.RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_NOERASE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}
}


void CTrackTimelinePanel::OnFileOpen()
{
	tstring filt = _T("All Supported File Types|*.rshow");
	for (size_t ii = 0, maxii = nucleus::IImporter::GetNumImporters(); ii < maxii; ii++)
	{
		const nucleus::IImporter *pimp = nucleus::IImporter::GetImporter(ii);
		for (size_t ei = 0, maxei = pimp->GetNumExtensions(); ei < maxei; ei++)
		{
			filt += _T("; *.");
			filt += pimp->GetExtension(ei);
		}
	}
	filt += _T("|Possum Show Files (*.rshow)|*.rshow");
	for (size_t ii = 0, maxii = nucleus::IImporter::GetNumImporters(); ii < maxii; ii++)
	{
		const nucleus::IImporter *pimp = nucleus::IImporter::GetImporter(ii);
		size_t maxei = pimp->GetNumExtensions();
		if (!maxei)
			continue;

		filt += _T("|");
		filt += pimp->GetDescription();
		filt += _T("|");

		for (size_t ei = 0; ei < maxei; ei++)
		{
			if (ei)
				filt += _T("; ");
			filt += _T("*.");
			filt += pimp->GetExtension(ei);
		}
	}

	filt += _T("|All Files (*.*)|*.*||");

	CFileDialog fd(TRUE, NULL, NULL, OFN_ENABLESIZING, filt.c_str());

	if (fd.DoModal() == IDOK)
	{
		CPossumDocument *pdoc = nullptr;

		pdoc = theApp.m_DocMan.CreateDocument(fd.GetPathName());

		if (pdoc)
		{
			theApp.SetActiveDocument(pdoc);
			AddDocument(pdoc);
			m_wndTabs.RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_NOERASE | RDW_UPDATENOW | RDW_ALLCHILDREN);
		}
	}
}


void CTrackTimelinePanel::OnFileSave()
{
	CPossumDocument *pdoc = theApp.m_DocMan.GetActiveDocument();
	if (pdoc)
	{
		if (pdoc->GetPathName().IsEmpty())
			OnFileSaveAs();
		else
		{
			pdoc->GetTrackSet()->Save(pdoc->GetPathName());
			pdoc->SetModifiedFlag(FALSE);
		}
	}
}

const TCHAR *s_FNFilt = _T("Possum Show Files (*.rshow)|*.rshow|All Files (*.*)|*.*||");

void CTrackTimelinePanel::OnFileSaveAs()
{
	CPossumDocument *pdoc = theApp.m_DocMan.GetActiveDocument();

	CString name = pdoc->GetTitle();
	name += ".rshow";
	CFileDialog fd(FALSE, NULL, name, OFN_ENABLESIZING | OFN_OVERWRITEPROMPT, s_FNFilt);
	if (fd.DoModal() == IDOK)
	{
		nucleus::RetCode rc = pdoc->GetTrackSet()->Save(fd.GetPathName());

		if (rc == nucleus::RetCode::OK)
		{
			pdoc->SetPathName(fd.GetPathName());

			CString t = fd.GetFileName();
			int di = t.ReverseFind(_T('.'));
			if (di >= 0)
				t.Truncate(di);
			
			pdoc->SetTitle(t);

			pdoc->SetModifiedFlag(FALSE);
		}
	}
}


void CTrackTimelinePanel::OnFileClose()
{
	CPossumDocument *pdoc = theApp.m_DocMan.GetActiveDocument();
	if (pdoc)
	{
		if (pdoc->IsModified())
		{
			CString s;
			s.Format(_T("%s has been modified. Do you really want to close without saving?"), (LPCTSTR)pdoc->GetTitle());
			int qres = MessageBox(s, _T("Close without saving?"), MB_YESNOCANCEL | MB_DEFBUTTON2);
			if (qres != IDCANCEL)
			{
				if (qres == IDNO)
				{
					OnFileSave();
				}
			}
		}

		int ti = m_wndTabs.GetActiveTab();
		m_wndTabs.SetActiveTab(ti - 1);
		m_wndTabs.RemoveTab(ti);

		CString path = pdoc->GetPathName();
		if (path.IsEmpty())
			path = pdoc->GetTitle();

		theApp.m_DocMan.CloseDocument(path);
	}
}


void CTrackTimelinePanel::OnSizing(UINT fwSide, LPRECT pRect)
{
	CDockablePane::OnSizing(fwSide, pRect);

	RedrawActiveTimeline();
}


void CTrackTimelinePanel::OnMove(int x, int y)
{
	CDockablePane::OnMove(x, y);

	RedrawActiveTimeline();
}

CPossumDocView *CTrackTimelinePanel::GetActiveDocView()
{
	return (CPossumDocView *)(m_wndTabs.GetTabWnd(m_wndTabs.GetActiveTab()));
}

CPossumDocView *CTrackTimelinePanel::FindDocViewByTrackSet(const nucleus::ITrackSet *pts)
{
	for (TTrackDocViewArray::const_iterator it = m_TrackTimelines.cbegin(), last_it = m_TrackTimelines.cend(); it != last_it; it++)
	{
		CPossumDocument *pd = ((CPossumDocView *)*it)->GetDocument();
		if (pd->GetTrackSet() == pts)
		{
			return *it;
		}
	}

	return nullptr;
}

void CTrackTimelinePanel::OnDestroy()
{
	CDockablePane::OnDestroy();

	m_wndTabs.RemoveAllTabs();

	for (TTrackDocViewArray::const_iterator it = m_TrackTimelines.cbegin(); it != m_TrackTimelines.cend(); it++)
	{
		CPossumDocView *pv = (*it);
		if (pv)
		{
			pv->GetDocument()->SetView(nullptr);
			delete pv;
		}
	}
	m_TrackTimelines.clear();
}


BOOL CTrackTimelinePanel::OnEraseBkgnd(CDC *pDC)
{
	return FALSE; // CDockablePane::OnEraseBkgnd(pDC);
}
