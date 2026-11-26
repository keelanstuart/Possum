#pragma once

#include <afxvslistbox.h>

class CVSListBoxFED : public CVSListBox
{
	DECLARE_DYNAMIC(CVSListBoxFED)

public:
	CVSListBoxFED();
	virtual ~CVSListBoxFED();

	virtual BOOL OnBeforeRemoveItem(int iItem);
	virtual void OnAfterAddItem(int iItem);
	virtual void OnSelectionChanged();

};

// CFilterEditorDlg dialog

class CFilterEditorDlg : public CDialog
{
	DECLARE_DYNAMIC(CFilterEditorDlg)

public:
	CFilterEditorDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CFilterEditorDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_FILTEREDITOR };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	CVSListBoxFED m_List;
	CTreeCtrl m_Tree;
	bool m_Init;

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	afx_msg void OnListItemChanged(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTreeItemChanged(NMHDR *pNMHDR, LRESULT *pResult);
};
