// source\FilterEditorDlg.cpp : implementation file
//

#include "stdafx.h"
#include "FilterEditorDlg.h"
#include "afxdialogex.h"
#include "FilterEditorDlg.h"
#include "resource.h"
#include "Possum.h"

#include <NuDevice.h>
#include <NuCommandInfo.h>

IMPLEMENT_DYNAMIC(CVSListBoxFED, CVSListBox)

CVSListBoxFED::CVSListBoxFED()
{
}

CVSListBoxFED::~CVSListBoxFED()
{
}

BOOL CVSListBoxFED::OnBeforeRemoveItem(int iItem)
{
	CPossum::TPtrSet *pps = (CPossum::TPtrSet *)GetItemData(iItem);
	if (pps)
	{
		delete pps;
		SetItemData(iItem, 0);
	}

	return TRUE;
}

void CVSListBoxFED::OnAfterAddItem(int iItem)
{
	CPossum::TPtrSet *pps = new CPossum::TPtrSet;
	SetItemData(iItem, (DWORD_PTR)pps);

	LRESULT res;
	((CFilterEditorDlg *)GetParent())->OnListItemChanged(nullptr, &res);
}

void CVSListBoxFED::OnSelectionChanged()
{
	LRESULT res;
	((CFilterEditorDlg *)GetParent())->OnListItemChanged(nullptr, &res);
}


// CFilterEditorDlg dialog

IMPLEMENT_DYNAMIC(CFilterEditorDlg, CDialog)

CFilterEditorDlg::CFilterEditorDlg(CWnd *pParent) : CDialog(IDD_FILTEREDITOR, pParent)
{
	m_Init = false;
}

CFilterEditorDlg::~CFilterEditorDlg()
{
}

void CFilterEditorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_FILTERLIST, m_List);
	DDX_Control(pDX, IDC_FILTERTREE, m_Tree);
}


BEGIN_MESSAGE_MAP(CFilterEditorDlg, CDialog)
	ON_NOTIFY(LBN_SELCHANGE, IDC_FILTERLIST, OnListItemChanged)
	ON_NOTIFY(TVN_ITEMCHANGED, IDC_FILTERTREE, OnTreeItemChanged)
END_MESSAGE_MAP()


// CFilterEditorDlg message handlers


BOOL CFilterEditorDlg::OnInitDialog()
{
	m_Init = true;

	CDialog::OnInitDialog();

	for (CPossum::TFilterMap::const_iterator it = theApp.m_FilterMap.cbegin(), last_it = theApp.m_FilterMap.cend(); it != last_it; it++)
	{
		int idx = m_List.AddItem(CString(it->first.c_str()));
		m_List.OnAfterAddItem(idx);
		CPossum::TPtrSet *pps = (CPossum::TPtrSet *)m_List.GetItemData(idx);
		if (pps)
		{
			*pps = it->second;
		}
	}

	for (size_t i = 0, maxi = theApp.m_pSys->DeviceManager()->GetDeviceCount(); i < maxi; i++)
	{
		nucleus::IDevice *pdev = theApp.m_pSys->DeviceManager()->GetDevice(i);
		HTREEITEM htid = m_Tree.InsertItem(pdev->GetName());
		m_Tree.SetItemData(htid, (DWORD_PTR)pdev);

		for (size_t j = 0, maxj = pdev->GetNumCommands(); j < maxj; j++)
		{
			const nucleus::ICommandInfo *pcmd = pdev->GetCommand(j);
			HTREEITEM htic = m_Tree.InsertItem(pcmd->GetName(), htid);
			m_Tree.SetItemData(htic, (DWORD_PTR)pcmd);
		}

		m_Tree.Expand(htid, TVE_EXPAND);
	}

	m_Tree.SetScrollPos(SB_VERT, 0);

#if 0
	m_List.SelectItem(0);
	LRESULT tmp;
	OnListItemChanged(nullptr, &tmp);
#endif

	m_Init = false;

	return TRUE;
}


void CFilterEditorDlg::OnOK()
{
	theApp.ClearFilterMap();
	for (int i = 0, maxi = m_List.GetCount(); i < maxi; i++)
	{
		theApp.AddFilterToMap(m_List.GetItemText(i), (CPossum::TPtrSet *)m_List.GetItemData(i));
		m_List.OnBeforeRemoveItem(i);
	}
	theApp.SaveFilterMap();

	CDialog::OnOK();
}

void CFilterEditorDlg::OnListItemChanged(NMHDR *pNMHDR, LRESULT *pResult)
{
	if (!m_Tree.GetCount())
		return;

	int sel = m_List.GetSelItem();
	if (sel < 0)
		return;

	CPossum::TPtrSet *pps = (CPossum::TPtrSet *)m_List.GetItemData(sel);
	if (!pps)
		return;

	HTREEITEM htir = m_Tree.GetRootItem();
	while (htir)
	{
		HTREEITEM htic = m_Tree.GetChildItem(htir);
		while (htic)
		{
			m_Tree.SetCheck(htic, (pps->find((LPVOID)m_Tree.GetItemData(htic)) == pps->end()) ? FALSE : TRUE);
			htic = m_Tree.GetNextSiblingItem(htic);
		}

		m_Tree.SetCheck(htir, (pps->find((LPVOID)m_Tree.GetItemData(htir)) == pps->end()) ? FALSE : TRUE);
		htir = m_Tree.GetNextSiblingItem(htir);
	}

	*pResult = 0;
}

void CFilterEditorDlg::OnTreeItemChanged(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;
	if (m_Init)
		return;

	NM_TREEVIEW *pnmt = (NM_TREEVIEW *)pNMHDR;

	NMTVITEMCHANGE *ptic = (NMTVITEMCHANGE *)pNMHDR;
	BOOL checked = ((BOOL)(ptic->uStateNew >> 12) - 1);

	int selidx = m_List.GetSelItem();
	if (selidx >= 0)
	{
		CPossum::TPtrSet *pps = (CPossum::TPtrSet *)m_List.GetItemData(selidx);
		if (pps)
		{
			LPVOID pv = (LPVOID)m_Tree.GetItemData(ptic->hItem);
			if (!pv)
				return;

			if (checked)
				pps->insert(pv);
			else
				pps->erase(pv);
		}
	}
}
