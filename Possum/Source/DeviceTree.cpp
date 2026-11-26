// DeviceTree.cpp : implementation file
//

#include "stdafx.h"
#include "Possum.h"
#include "DeviceTree.h"
#include "PossumFrame.h"

#include <NuDevice.h>
#include <NuCommandInfo.h>
#include <NuCommand.h>
#include <NuTrack.h>

#include <gdiplus.h>

// CDeviceTree

IMPLEMENT_DYNAMIC(CDeviceTree, CTreeCtrl)

CDeviceTree::CDeviceTree()
{
	m_brBackground.CreateSolidBrush(RGB(40, 40, 40));
	m_de = NULL;
}

CDeviceTree::~CDeviceTree()
{
}

void CDeviceTree::RefreshItems()
{
	m_DevMap.clear();
	m_CmdMap.clear();
	DeleteAllItems();

	if (!theApp.m_pSys)
		return;

	SetItemHeight(32);

	nucleus::IDeviceManager *pdm = theApp.m_pSys->DeviceManager();
	if (!pdm)
		return;

	for (size_t i = 0, maxi = pdm->GetDeviceCount(); i < maxi; i++)
	{
		nucleus::IDevice *pdev = pdm->GetDevice(i);

		AddDeviceItem(pdev, TVI_ROOT);
	}
}

HTREEITEM CDeviceTree::AddDeviceItem(const nucleus::IDevice *pdev, HTREEITEM parent)
{
	assert(pdev);

	HTREEITEM ret = InsertItem(pdev->GetName(), 0, 0, parent);
	SetItemData(ret, (DWORD_PTR)pdev);
	m_DevMap.insert(TDevicePair(ret, pdev));

	for (size_t i = 0, maxi = pdev->GetNumCommands(); i < maxi; i++)
	{
		const nucleus::ICommandInfo *pcmd = pdev->GetCommand(i);
		assert(pcmd);
		props::IProperty *picon_prop = pcmd->GetDefaultParams()->GetPropertyById('ICON');
		if (!picon_prop)
			continue;

		AddCommandItem(pcmd, ret);
	}

	Expand(ret, TVE_EXPAND);

	return ret;
}

HTREEITEM CDeviceTree::AddCommandItem(const nucleus::ICommandInfo *pcmd, HTREEITEM parent)
{
	assert(pcmd);

	HTREEITEM ret = InsertItem(pcmd->GetName(), 1, 1, parent);
	SetItemData(ret, (DWORD_PTR)pcmd);
	m_CmdMap.insert(TCommandPair(ret, pcmd));

	return ret;
}


BEGIN_MESSAGE_MAP(CDeviceTree, CTreeCtrl)
	ON_NOTIFY_REFLECT(NM_DBLCLK, &CDeviceTree::OnNMDblclk)
	ON_NOTIFY_REFLECT(NM_CLICK, &CDeviceTree::OnNMClick)
	ON_NOTIFY_REFLECT(TVN_BEGINDRAG, &CDeviceTree::OnTvnBegindrag)
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, &CDeviceTree::OnTvnSelchanged)
	ON_WM_CTLCOLOR()
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnCustomDraw)            // hand entered 
	ON_WM_CTLCOLOR_REFLECT()
END_MESSAGE_MAP()



// CDeviceTree message handlers



void CDeviceTree::OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult)
{
	CPossumMainFrame *pmf = (CPossumMainFrame *)theApp.GetMainWnd();
	if (!pmf)
		return;

	HTREEITEM hti = GetSelectedItem();
	if (hti)
	{
		HTREEITEM hpar = GetParentItem(hti);
		if (hpar)
		{
			nucleus::ICommandInfo *pci = (nucleus::ICommandInfo *)GetItemData(hti);
			if (pci)
			{
				CPossumDocument *pd = theApp.m_DocMan.GetActiveDocument();
				CPossumDocView *pdv = pd->GetView();
				if (!pdv)
					return;

				double t = pdv->GetSelectedTime();

				nucleus::ICommand *pc = nullptr;
				nucleus::RetCode rc;

				nucleus::ITrackSet *pts = pd->GetTrackSet();
				size_t i = 0, maxi = pts->GetNumTracks();
				for (; i < maxi; i++)
				{
					nucleus::ITrack *pt = pts->GetTrack(i);
					if (!pt)
						continue;

					rc = pt->AddCommand(pci, t, &pc);

					if (rc == nucleus::RetCode::OK)
						break;
				}

				if (pc)
				{
					props::IPropertySet *ps = pc->GetParams();
					if (ps)
					{
						props::IProperty* pp = ps->CreateProperty(_T("Tag"), 'TAG');
						if (pp)
						{
							pp->Flags().Set(props::IProperty::PROPFLAG(props::IProperty::EPropFlag::TOOLTIPITEM));
							pp->SetString(_T(""));
						}
					}

					theApp.m_DocMan.GetActiveDocument()->ClearSelection();
					theApp.m_DocMan.GetActiveDocument()->AddToSelection(pc);
				
					theApp.RefreshCommandList(true);
					theApp.RedrawActiveTimeline();
					theApp.m_DocMan.GetActiveDocument()->SetModifiedFlag();
				}
			}
		}
		else
		{
			nucleus::IDevice *pdev = (nucleus::IDevice *)GetItemData(hti);
			if (pdev)
			{
			}
		}
	}

	*pResult = 0;
}

void CDeviceTree::OnNMClick(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;

	CPoint pt;
	GetCursorPos(&pt);
	ScreenToClient(&pt);

	HTREEITEM hti = HitTest(pt);

	if (hti && hti == GetSelectedItem())
		ShowSelectedProperties();
}

void CDeviceTree::OnTvnBegindrag(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pnmtv = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

	HTREEITEM hti = HitTest(pnmtv->ptDrag);
	if (!hti)
		return;

	TCommandMap::const_iterator cit = m_CmdMap.find(hti);
	if (cit == m_CmdMap.end())
		return;

	const nucleus::ICommandInfo* pci = cit->second;

	tstring s;
	s += _T("[");
	s += pci->GetDevice()->GetName();
	s += _T(":");
	s += pci->GetName();
	s += _T("]");

	COleDataSource datasrc;
	FORMATETC etc = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

	// Add 1 extra for the final null char, and the size of the DROPFILES struct.
	size_t bufsz = sizeof(DROPFILES) + (sizeof(TCHAR) * (s.length() + 2));

	// Allocate memory from the heap for the DROPFILES struct.
	HGLOBAL hdrop = GlobalAlloc(GHND | GMEM_SHARE | GMEM_ZEROINIT, bufsz);
	if (NULL == hdrop)
		return;

	DROPFILES *pdrop = (DROPFILES *)GlobalLock(hdrop);
	if (NULL == pdrop)
	{
		GlobalFree(hdrop);
		return;
	}

	pdrop->pFiles = sizeof(DROPFILES);

#ifdef _UNICODE
	pdrop->fWide = TRUE;
#endif;

	TCHAR *buf = (TCHAR *)(LPBYTE(pdrop) + sizeof(DROPFILES));
	//_tcscpy_s(buf, bufsz - sizeof(DROPFILES), s.c_str());

	GlobalUnlock (hdrop);
	datasrc.CacheGlobalData(CF_HDROP, hdrop, &etc);

	HGLOBAL hb;
	hb = GlobalAlloc(GHND | GMEM_SHARE, sizeof(bool));
	if (NULL == hb)
	{
		GlobalFree(hdrop);
		return;
	}

	// Put the data in the data source.
	etc.cfFormat = theApp.m_DragDeviceFormat;
	datasrc.CacheGlobalData(theApp.m_DragDeviceFormat, hb, &etc);

	theApp.m_DragData.m_eMode = CPossum::SDragData::CMDINFO;
	theApp.m_DragData.m_pCmdInfo = pci;

	SelectItem(hti);
	RedrawWindow();

	// Start the drag 'n' drop!
	DROPEFFECT de = datasrc.DoDragDrop(DROPEFFECT_COPY | DROPEFFECT_MOVE);

	switch (de)
	{
		case DROPEFFECT_COPY:
			break;

		case DROPEFFECT_MOVE:
			break;

		case DROPEFFECT_NONE:
			break;
	}

	*pResult = 0;
}


void CDeviceTree::OnTvnSelchanged(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;
	ShowSelectedProperties();
}


HBRUSH CDeviceTree::CtlColor(CDC* pDC, UINT nCtlColor)
{
	if ((CTLCOLOR_STATIC == nCtlColor) || (COLOR_BACKGROUND == nCtlColor))
	{
		pDC->SetBkColor(RGB(40, 40, 40));
		return m_brBackground;
	}

	return NULL;
}

void CDeviceTree::OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMTVCUSTOMDRAW* pstCD = (NMTVCUSTOMDRAW*)pNMHDR;
	*pResult = 0;

	CDC* dc = CDC::FromHandle(pstCD->nmcd.hdc);

	switch (pstCD->nmcd.dwDrawStage)
	{
		case CDDS_PREERASE:
		case CDDS_POSTERASE:
		case CDDS_ITEMPREERASE:
		case CDDS_ITEMPOSTERASE:
			*pResult = CDRF_SKIPDEFAULT;
			return;

		case CDDS_PREPAINT:
		{
			CDC* dc = CDC::FromHandle(pstCD->nmcd.hdc);
			dc->FillSolidRect(&pstCD->nmcd.rc, RGB(40, 40, 40));
			*pResult = CDRF_NOTIFYITEMDRAW;
			break;
		}

		case CDRF_DOERASE:
		{
			*pResult = CDRF_SKIPDEFAULT;
			break;
		}

		case CDDS_ITEMPREPAINT:
		{
			pstCD->clrText = RGB(255, 255, 255);
			pstCD->clrTextBk = RGB(40, 40, 40);
			DrawTreeItem(&pstCD->nmcd);
			*pResult = CDRF_SKIPDEFAULT;
			break;
		}

		case CDDS_ITEMPOSTPAINT:
		{
//			DrawTreeItem(&pstCD->nmcd);
			*pResult = CDRF_SKIPDEFAULT;
			break;
		}
	}
}

#if defined(_UNICODE) || defined(UNICODE)
#define GetLogFont	GetLogFontW
#else
#define GetLogFont	GetLogFontA
#endif

void CDeviceTree::DrawTreeItem(LPNMCUSTOMDRAW pcd)
{
	HTREEITEM hi = (HTREEITEM)pcd->dwItemSpec;

	CDC* pdc = CDC::FromHandle(pcd->hdc);

	pdc->SetBkMode(TRANSPARENT);
	pdc->SetTextColor(RGB(255, 255, 255));

	CRect r;
	GetItemRect(hi, &r, false);
	CString s = GetItemText(hi);
	//pdc->DrawText(s, -1, &r, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_WORD_ELLIPSIS);

	Gdiplus::Graphics gr(*pdc);

	Gdiplus::SolidBrush br_sbg(Gdiplus::Color(255, 128, 128, 100));
	Gdiplus::SolidBrush br_text(Gdiplus::Color(255, 255, 255, 255));
	Gdiplus::SolidBrush br_acc(Gdiplus::Color(255, 16, 16, 16));
	Gdiplus::SolidBrush br_sacc(Gdiplus::Color(255, 255, 255, 128));
	Gdiplus::SolidBrush br_devunk(Gdiplus::Color(255, 128, 16, 128));
	Gdiplus::SolidBrush br_devready(Gdiplus::Color(255, 0, 128, 0));
	Gdiplus::SolidBrush br_devabsent(Gdiplus::Color(255, 128, 32, 0));
	Gdiplus::SolidBrush br_devbusy(Gdiplus::Color(255, 128, 64, 8));
	Gdiplus::Pen pn_acc(&br_acc, 1);
	Gdiplus::Pen pn_sacc(&br_sacc, 1);

	Gdiplus::Font f_list(pcd->hdc);

	Gdiplus::SolidBrush *br_devstate[nucleus::IDevice::Status::NumStates] = {&br_devunk, &br_devready, &br_devabsent, &br_devbusy};

	const nucleus::ICommandInfo* pci = nullptr;
	const nucleus::IDevice* pd = nullptr;

	TDeviceMap::const_iterator dit = m_DevMap.find(hi);
	if (dit != m_DevMap.cend())
	{
		// it's a device entry
		pd = dit->second;
	}

	TCommandMap::const_iterator cit = m_CmdMap.find(hi);
	if (cit != m_CmdMap.cend())
	{
		// it's a command entry
		pci = cit->second;
		pd = pci->GetDevice();
	}

	bool sel = (pcd->uItemState & CDIS_FOCUS) ? true : false;

	int cr, cg, cb;
	cr = cg = cb = 40;

#if 0
	if (!(pdi->itemID & 1))
	{
		cr /= 4; cg /= 4; cb /= 4;
	}
#endif

	Gdiplus::SolidBrush br_bg(Gdiplus::Color(255, cr, cg, cb));

	gr.FillRectangle(sel ? &br_sbg : &br_bg, r.left, r.top, r.Width(), r.Height());

	Gdiplus::Font defont(pcd->hdc);
	LOGFONT lf;
	defont.GetLogFont(&gr, &lf);

	CString str;

	if (pci)
	{
		props::IProperty* iconprop = pci->GetDefaultParams()->GetPropertyById('ICON');
		if (iconprop)
		{
			CRect ri(0, 0, r.Height(), r.Height());
			ri.DeflateRect(4, 4, 4, 4);
			ri.OffsetRect(22, r.top);

			Gdiplus::Image* img = (Gdiplus::Image*)theApp.m_PicMan.GetPic(iconprop->AsString());
			if (img)
				gr.DrawImage(img, ri.left, ri.top, ri.Width(), ri.Height());
		}

		str = pci->GetName();
	}
	else if (pd)
	{
		//lf.lfWeight = FW_EXTRABOLD;
		lf.lfWeight--;
		lf.lfHeight--;
		lf.lfWidth--;

		str = pd->GetName();

		gr.FillRectangle(&br_acc, r.left, r.top, r.Width(), 2);

		gr.FillRectangle(br_devstate[pd->GetStatus()], r.left, r.top, 3, r.Height());
	}

	Gdiplus::Font f(pcd->hdc, &lf);

	Gdiplus::RectF tnr;
	gr.MeasureString((LPCTSTR)str, str.GetLength(), &f, Gdiplus::PointF(pci ? 50.0f : 10.0f, 0.0f), &tnr);
	gr.DrawString((LPCTSTR)str, str.GetLength(), &f, Gdiplus::PointF(pci ? 50.0f : 10.0f, (Gdiplus::REAL)(r.CenterPoint().y - (tnr.Height / 2.0f))), &br_text);
}

BOOL CDeviceTree::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style |= TVS_FULLROWSELECT;
	//cs.style |= TVS_SINGLEEXPAND;
	cs.style &= ~(TVS_HASLINES);
	cs.dwExStyle |= TVS_EX_DOUBLEBUFFER;

	BOOL ret = CViewTree::PreCreateWindow(cs);

	return ret;
}

void CDeviceTree::ShowSelectedProperties()
{
	HTREEITEM hti = GetSelectedItem();
	if (hti)
	{
		CString title;

		TDeviceMap::const_iterator dit = m_DevMap.find(hti);
		if (dit != m_DevMap.end())
		{
			nucleus::IDevice* d = (nucleus::IDevice*)dit->second;
			const props::IPropertySet* p = d->GetProperties();

			title.Format(_T("Device: %s Properties"), d->GetName());
			theApp.SetActiveProperties((props::IPropertySet*)p, false, title, nullptr, d);
		}
		else
		{
			TCommandMap::const_iterator cit = m_CmdMap.find(hti);
			if (cit != m_CmdMap.end())
			{
				hti = GetParentItem(hti);
				dit = m_DevMap.find(hti);
				if (dit != m_DevMap.end())
					title.Format(_T("Command: %s.%s Default Parameters"), dit->second->GetName(), cit->second->GetName());
				else
					title.Format(_T("Command: %s Default Parameters"), cit->second->GetName());

				// Show the default parameters of the command
				theApp.SetActiveProperties((props::IPropertySet *)cit->second->GetDefaultParams(), false, title, (nucleus::ICommandInfo*)cit->second, nullptr);
			}
		}
	}
	else
	{
		theApp.SetActiveProperties(nullptr);
	}
}