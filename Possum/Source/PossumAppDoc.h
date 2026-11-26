
// PossumAppDoc.h : interface of the CPossumAppDoc class
//


#pragma once


class CPossumAppDoc : public CDocument
{
protected: // create from serialization only
	CPossumAppDoc();
	DECLARE_DYNCREATE(CPossumAppDoc)

// Attributes
public:

// Operations
public:

// Overrides
public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
#ifdef SHARED_HANDLERS
	virtual void InitializeSearchContent();
	virtual void OnDrawThumbnail(CDC& dc, LPRECT lprcBounds);
#endif // SHARED_HANDLERS

// Implementation
public:
	virtual ~CPossumAppDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	void SetRunFromDir(const TCHAR *runfromdir);
	const TCHAR *GetRunFromDir();

	void SetWndClass(const TCHAR *wndclass);
	const TCHAR *GetWndClass();

	void SetWndName(const TCHAR *wndname);
	const TCHAR *GetWndName();

protected:
	tstring m_sRunFromDir;
	tstring m_sWndClass;
	tstring m_sWndName;


// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()

	// Generated OLE dispatch map functions

	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()

#ifdef SHARED_HANDLERS
	// Helper function that sets search content for a Search Handler
	void SetSearchContent(const CString& value);
#endif // SHARED_HANDLERS
};
