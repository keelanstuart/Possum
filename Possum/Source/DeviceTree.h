#pragma once

#include "ViewTree.h"
#include <NuDevice.h>
#include <NuCommand.h>

// CDeviceTree

class CDeviceTree : public CViewTree
{
	DECLARE_DYNAMIC(CDeviceTree)

public:
	CDeviceTree();
	virtual ~CDeviceTree();

	void RefreshItems();

protected:
	CBrush m_brBackground;
	COleDataSource m_DropSource;
	DROPEFFECT m_de;

	HTREEITEM AddDeviceItem(const nucleus::IDevice *pdev, HTREEITEM parent);
	HTREEITEM AddCommandItem(const nucleus::ICommandInfo *pcmd, HTREEITEM parent);

	typedef std::map<HTREEITEM, const nucleus::IDevice *> TDeviceMap;
	typedef std::pair<HTREEITEM, const nucleus::IDevice *> TDevicePair;
	TDeviceMap m_DevMap;

	typedef std::map<HTREEITEM, const nucleus::ICommandInfo *> TCommandMap;
	typedef std::pair<HTREEITEM, const nucleus::ICommandInfo *> TCommandPair;
	TCommandMap m_CmdMap;

	DECLARE_MESSAGE_MAP()

	int GetTreeItemDepth(HTREEITEM hti);

	void DrawTreeItem(LPNMCUSTOMDRAW pcd);

	void ShowSelectedProperties();

public:
	afx_msg void OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMClick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnBegindrag(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnSelchanged(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
	afx_msg void OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);

	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
};


