// Source\MediaListCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "MediaListCtrl.h"


// CMediaListCtrl

IMPLEMENT_DYNAMIC(CMediaListCtrl, CListCtrl)

CMediaListCtrl::CMediaListCtrl()
{

}

CMediaListCtrl::~CMediaListCtrl()
{
}


BEGIN_MESSAGE_MAP(CMediaListCtrl, CListCtrl)
	ON_WM_CREATE()
END_MESSAGE_MAP()



// CMediaListCtrl message handlers




int CMediaListCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CListCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;

	SetBkColor(RGB(0, 0, 0));

	return 0;
}


bool CMediaListCtrl::NavigateTo(const TCHAR *path)
{
//	if (!PathFileExists(path))
//		return false;

	if (!GetSafeHwnd())
		return false;

	CWnd *ppar = GetParent();
	if (!ppar)
		return false;

	TCHAR _path[MAX_PATH];
	_tcscpy_s(_path, path);
	::DlgDirList(ppar->GetSafeHwnd(), _path, this->GetDlgCtrlID(), 0, DDL_ARCHIVE | DDL_READONLY | DDL_READWRITE | DDL_DIRECTORY);

	return true;
}
