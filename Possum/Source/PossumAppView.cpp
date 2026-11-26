
// PossumAppView.cpp : implementation of the CPossumAppView class
//

#include "stdafx.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "Possum.h"
#endif

#include "PossumAppDoc.h"
#include "PossumAppView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CPossumAppView

IMPLEMENT_DYNCREATE(CPossumAppView, CView)

BEGIN_MESSAGE_MAP(CPossumAppView, CView)
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CPossumAppView::OnFilePrintPreview)
	ON_WM_CONTEXTMENU()
	ON_WM_RBUTTONUP()
	ON_WM_SIZE()
END_MESSAGE_MAP()

// CPossumAppView construction/destruction

CPossumAppView::CPossumAppView()
{
	EnableActiveAccessibility();

	m_hClientWnd = NULL;
	m_ClientProcessId = NULL;
	m_hClientProcess = NULL;
	m_Escape = false;
	m_EscapeAck = false;
}

CPossumAppView::~CPossumAppView()
{
	m_Escape = true;
	while (!m_EscapeAck)
		Sleep(10);

	if (m_hClientProcess)
		TerminateProcess(m_hClientProcess, 0);
}

BOOL CPossumAppView::PreCreateWindow(CREATESTRUCT& cs)
{
	return CView::PreCreateWindow(cs);
}

// CPossumAppView drawing

void CPossumAppView::OnDraw(CDC* /*pDC*/)
{
	CPossumAppDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// TODO: add draw code for native data here
}


// CPossumAppView printing


void CPossumAppView::OnFilePrintPreview()
{
#ifndef SHARED_HANDLERS
	AFXPrintPreview(this);
#endif
}

BOOL CPossumAppView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CPossumAppView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CPossumAppView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

void CPossumAppView::OnRButtonUp(UINT /* nFlags */, CPoint point)
{
	ClientToScreen(&point);
	OnContextMenu(this, point);
}

void CPossumAppView::OnContextMenu(CWnd* /* pWnd */, CPoint point)
{
#ifndef SHARED_HANDLERS
	theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_EDIT, point.x, point.y, this, TRUE);
#endif
}


// CPossumAppView diagnostics

#ifdef _DEBUG
void CPossumAppView::AssertValid() const
{
	//CView::AssertValid();
}

void CPossumAppView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CPossumAppDoc* CPossumAppView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CPossumAppDoc)));
	return (CPossumAppDoc*)m_pDocument;
}
#endif //_DEBUG


// CPossumAppView message handlers

extern BOOL is_main_window(HWND handle);
extern BOOL CALLBACK enum_windows_callback(HWND handle, LPARAM lParam);
extern HWND find_main_window(unsigned long process_id, const TCHAR *wndclass, const TCHAR *wndname);


CPossumAppView *CPossumAppView::s_ActiveAppView = nullptr;


void CPossumAppView::SpawnApp()
{
	CPossumAppDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	if (theApp.m_pTPAM->RunApp(m_AppName.c_str()))
	{
		HANDLE hp = NULL; // hProc

		while (!m_Escape && !theApp.m_pTPAM->IsAppRunning(m_AppName.c_str(), &hp, &m_hClientWnd))
		{
			Sleep(50);
		}

		if (hp)
		{
			m_ClientProcessId = GetProcessId(hp);
		}
	}

	if (!m_Escape)
	{
		s_ActiveAppView = nullptr;

		if (m_hClientWnd)
		{
			if (this->GetSafeHwnd())
			{
				::SetParent(m_hClientWnd, this->GetSafeHwnd());

				LONG_PTR style = ::GetWindowLongPtr(m_hClientWnd, GWL_STYLE);
				style &= ~(WS_BORDER | WS_CAPTION | WS_SIZEBOX);
				::SetWindowLongPtr(m_hClientWnd, GWL_STYLE, style);

				CRect r;
				::GetClientRect(this->GetSafeHwnd(), &r);

				::SetWindowPos(m_hClientWnd, NULL, 0, 0, r.Width(), r.Height(), SWP_ASYNCWINDOWPOS | SWP_NOREPOSITION | SWP_NOSENDCHANGING);

				::ShowWindow(m_hClientWnd, SW_RESTORE);

				TCHAR n[MAX_PATH], *f;
				_tcscpy_s(n, pDoc->GetPathName());
				f = PathFindFileName(n);
				PathRemoveExtension(f);
				pDoc->SetTitle(f);
			}
			else
			{
				::PostMessage(m_hClientWnd, WM_QUIT, 0, 0);
			}
		}
	}

	m_EscapeAck = true;

	ReleaseMutex(theApp.m_hSpawnMutex);
}

void CPossumAppView::OnInitialUpdate()
{
	CView::OnInitialUpdate();

	CPossumAppDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

#if 0
	size_t appidx;
	tstring s[CThirdPartyAppManager::NUMPARAMS];
	if (theApp.m_pTPAM->FindAppByExePath(pDoc->GetPathName(), appidx))
	{
		theApp.m_pTPAM->GetRegisteredApp(appidx, &s[CThirdPartyAppManager::NAME], &s[CThirdPartyAppManager::EXEPATH],
			&s[CThirdPartyAppManager::RUNDIR], &s[CThirdPartyAppManager::PARAMS],
			&s[CThirdPartyAppManager::WNDCLASS], &s[CThirdPartyAppManager::WNDNAME]);

		m_AppName = s[CThirdPartyAppManager::NAME];
		theApp.m_pSys->ThreadPool()->RunTask([this](size_t task_number) -> pool::IThreadPool::TaskReturn
		{
			this->SpawnApp();

			return pool::IThreadPool::TaskReturn::OK;
		});

		pDoc->SetRunFromDir(s[CThirdPartyAppManager::RUNDIR].c_str());
		pDoc->SetWndClass(s[CThirdPartyAppManager::WNDCLASS].c_str());
		pDoc->SetWndName(s[CThirdPartyAppManager::WNDNAME].c_str());
	}

	s_ActiveAppView = this;

	// actually launch the new process...
	const TCHAR *runfromdir = pDoc->GetRunFromDir();
	if (!runfromdir || !PathIsDirectory(runfromdir))
	{
		_tcscpy_s(m_ProgPath, pDoc->GetPathName());
		PathRemoveFileSpec(m_ProgPath);
		PathRemoveBackslash(m_ProgPath);
	}
	else
	{
		_tcscpy_s(m_ProgPath, MAX_PATH, runfromdir);
	}

	m_Params = s[CThirdPartyAppManager::PARAMS];
#endif
}



void CPossumAppView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);

	// tell the client app window that it needs to resize, too...
	CRect r;
	GetClientRect(r);
	::SetWindowPos(m_hClientWnd, NULL, 0, 0, r.Width(), r.Height(), SWP_ASYNCWINDOWPOS | SWP_NOREPOSITION | SWP_NOSENDCHANGING);
}
