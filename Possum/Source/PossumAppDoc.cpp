
// PossumAppDoc.cpp : implementation of the CPossumAppDoc class
//

#include "stdafx.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "Possum.h"
#endif

#include "PossumAppDoc.h"

#include <propkey.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CPossumAppDoc

IMPLEMENT_DYNCREATE(CPossumAppDoc, CDocument)

BEGIN_MESSAGE_MAP(CPossumAppDoc, CDocument)
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CPossumAppDoc, CDocument)
END_DISPATCH_MAP()

// Note: we add support for IID_IPossum to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .IDL file.

// {AA86A884-BCE5-4ADF-8D48-97C3209E39D3}
static const IID IID_IPossum =
{ 0xAA86A884, 0xBCE5, 0x4ADF, { 0x8D, 0x48, 0x97, 0xC3, 0x20, 0x9E, 0x39, 0xD3 } };

BEGIN_INTERFACE_MAP(CPossumAppDoc, CDocument)
	INTERFACE_PART(CPossumAppDoc, IID_IPossum, Dispatch)
END_INTERFACE_MAP()


// CPossumAppDoc construction/destruction

CPossumAppDoc::CPossumAppDoc()
{
	// TODO: add one-time construction code here
	m_sRunFromDir = _T("");
	m_sWndClass = _T("");
	m_sWndName = _T("");

	EnableAutomation();

	AfxOleLockApp();
}

CPossumAppDoc::~CPossumAppDoc()
{
	AfxOleUnlockApp();
}

BOOL CPossumAppDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}


void CPossumAppDoc::SetRunFromDir(const TCHAR *runfromdir)
{
	m_sRunFromDir = runfromdir;
}

const TCHAR *CPossumAppDoc::GetRunFromDir()
{
	return m_sRunFromDir.c_str();
}

void CPossumAppDoc::SetWndClass(const TCHAR *wndclass)
{
	m_sWndClass = wndclass;
}

const TCHAR *CPossumAppDoc::GetWndClass()
{
	return m_sWndClass.c_str();
}

void CPossumAppDoc::SetWndName(const TCHAR *wndname)
{
	m_sWndName = wndname;
}

const TCHAR *CPossumAppDoc::GetWndName()
{
	return m_sWndName.c_str();
}


// CPossumAppDoc serialization

void CPossumAppDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

#ifdef SHARED_HANDLERS

// Support for thumbnails
void CPossumAppDoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
{
	// Modify this code to draw the document's data
	dc.FillSolidRect(lprcBounds, RGB(255, 255, 255));

	CString strText = _T("TODO: implement thumbnail drawing here");
	LOGFONT lf;

	CFont* pDefaultGUIFont = CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT));
	pDefaultGUIFont->GetLogFont(&lf);
	lf.lfHeight = 36;

	CFont fontDraw;
	fontDraw.CreateFontIndirect(&lf);

	CFont* pOldFont = dc.SelectObject(&fontDraw);
	dc.DrawText(strText, lprcBounds, DT_CENTER | DT_WORDBREAK);
	dc.SelectObject(pOldFont);
}

// Support for Search Handlers
void CPossumAppDoc::InitializeSearchContent()
{
	CString strSearchContent;
	// Set search contents from document's data. 
	// The content parts should be separated by ";"

	// For example:  strSearchContent = _T("point;rectangle;circle;ole object;");
	SetSearchContent(strSearchContent);
}

void CPossumAppDoc::SetSearchContent(const CString& value)
{
	if (value.IsEmpty())
	{
		RemoveChunk(PKEY_Search_Contents.fmtid, PKEY_Search_Contents.pid);
	}
	else
	{
		CMFCFilterChunkValueImpl *pChunk = NULL;
		ATLTRY(pChunk = new CMFCFilterChunkValueImpl);
		if (pChunk != NULL)
		{
			pChunk->SetTextValue(PKEY_Search_Contents, value, CHUNK_TEXT);
			SetChunkValue(pChunk);
		}
	}
}

#endif // SHARED_HANDLERS

// CPossumAppDoc diagnostics

#ifdef _DEBUG
void CPossumAppDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CPossumAppDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CPossumAppDoc commands
