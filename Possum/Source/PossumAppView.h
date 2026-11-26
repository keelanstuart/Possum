
// PossumAppView.h : interface of the CPossumAppView class
//

#pragma once


class CPossumAppView : public CView
{
protected: // create from serialization only
	CPossumAppView();
	DECLARE_DYNCREATE(CPossumAppView)

// Attributes
public:
	CPossumAppDoc* GetDocument() const;

protected:
	HANDLE m_hClientProcess;
	DWORD m_ClientProcessId;
	HWND m_hClientWnd;
	bool m_Escape;
	bool m_EscapeAck;

	tstring m_AppName;

	TCHAR m_ProgPath[MAX_PATH];
	tstring m_Params;

	static pool::IThreadPool::TaskReturn _spawnapp(void *param0, void *param1, size_t task_number);
	void SpawnApp();

// Operations
public:

// Overrides
public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

// Implementation
public:
	virtual ~CPossumAppView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	static CPossumAppView *s_ActiveAppView;
	static LRESULT CALLBACK CbtHookFunc(int nCode, WPARAM wParam, LPARAM lParam);
	static HWND s_ActiveAppHWND;

// Generated message map functions
protected:
	afx_msg void OnFilePrintPreview();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	DECLARE_MESSAGE_MAP()
public:
	virtual void OnInitialUpdate();
	afx_msg void OnSize(UINT nType, int cx, int cy);
};

#ifndef _DEBUG  // debug version in PossumAppView.cpp
inline CPossumAppDoc* CPossumAppView::GetDocument() const
   { return reinterpret_cast<CPossumAppDoc*>(m_pDocument); }
#endif

