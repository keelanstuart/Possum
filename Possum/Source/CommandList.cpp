// CommandList.cpp : implementation file
//

#include "stdafx.h"
#include "Possum.h"
#include "CommandList.h"
#include "PossumFrame.h"
#include "PossumDocument.h"
#include "Util.h"

#include <NuDevice.h>
#include <NuCommandInfo.h>
#include <NuCommand.h>
#include <NuTrackSet.h>
#include <NuTrack.h>
#include <algorithm>
#include <limits>
#include <cfloat>
#include <vector>

// CCommandList

IMPLEMENT_DYNAMIC(CCommandList, CListCtrl)

CCommandList::CCommandList()
{
	m_pDoc = nullptr;
	m_brBackground.CreateSolidBrush(RGB(40, 40, 40));
}

CCommandList::~CCommandList()
{
}

bool CCommandList::IsActiveDocument()
{
	return (theApp.m_DocMan.GetActiveDocument() == m_pDoc) ? true : false;
}

nucleus::ICommand* CCommandList::CommandAt(int index) const
{
	return index >= 0 && size_t(index) < m_Items.size() ? m_Items[index].command : nullptr;
}

void CCommandList::SortByTime()
{
	// Do not truncate sub-100ms differences (the old comparator did).
	std::sort(m_Items.begin(), m_Items.end(), [](const CommandEntry& a, const CommandEntry& b)
	{
		if (a.time != b.time)
			return a.time < b.time;
		if (a.trackIndex != b.trackIndex)
			return a.trackIndex < b.trackIndex;
		return a.sequence < b.sequence;
	});
}

void CCommandList::RefreshSelectionCache()
{
	m_Selected.clear();
	if (!m_pDoc)
		return;

	const size_t count = m_pDoc->GetNumSelected();
	m_Selected.reserve(count);
	for (size_t i = 0; i < count; i++)
		if (auto* command = m_pDoc->GetSelection(i))
			m_Selected.insert(command);
}

CString CCommandList::CellText(const CommandEntry& entry, int column) const
{
	CString result;
	const auto* info = entry.command->GetCommandInfo();
	const auto* device = info ? info->GetDevice() : nullptr;

	switch (column)
	{
		case 1:
		{
			UINT h = 0, m = 0, s = 0, f = 0;
			GetTimeComponents(entry.time, h, m, s, f);
			result.Format(_T("%02u:%02u:%02u.%01u"), h, m, s, f);
		}
		break;
		case 2:
			if (entry.track && entry.track->GetName())
				result = entry.track->GetName();
			break;
		case 3:
			if (device && device->GetName())
				result = device->GetName();
			break;
		case 4:
			if (info && info->GetName())
				result = info->GetName();
			break;
		case 5:
			if (auto* tag = entry.command->GetParams()->GetPropertyById('TAG'))
				result = tag->AsString();
			break;
	}
	return result;
}

void CCommandList::RefreshItems(bool deep)
{
	auto* current = theApp.m_DocMan.GetActiveDocument();
	if (current != m_pDoc)
	{
		m_pDoc = current;
		m_SelectionAnchor = nullptr;
		deep = true;
	}

	bool needsSort = deep;
	if (deep)
	{
		std::vector<CommandEntry> items;
		if (m_pDoc && m_pDoc->GetTrackSet())
		{
			auto* ts = m_pDoc->GetTrackSet();
			size_t sequence = 0;
			for (size_t row = 0; row < ts->GetNumTracks(); row++)
			{
				auto* track = ts->GetTrack(row);
				if (!track)
					continue;

				for (size_t j = 0; j < track->GetNumCommands(); j++)
				{
					double time = 0.0;
					auto* cmd = track->GetCommand(j, &time);
					if (!cmd)
						continue;

					auto* info = cmd->GetCommandInfo();
					if (!info)
						continue;
					if (!theApp.m_ActiveCommandFilter.empty())
					{
						const auto* device = info->GetDevice();

						if (theApp.m_ActiveCommandFilter.find((LPVOID)info) == theApp.m_ActiveCommandFilter.end() &&
							theApp.m_ActiveCommandFilter.find((LPVOID)device) == theApp.m_ActiveCommandFilter.end())
							continue;
					}
					items.push_back({cmd, track, time, row, sequence++});
				}
			}
		}
		m_Items.swap(items);
	}
	else
	{
		// The existing callers use a shallow update for both selection changes
		// and note moves.  Keep chronological order correct without recreating
		// the command index or any list-view items.
		for (auto& entry : m_Items)
		{
			double time = entry.time;
			if (!entry.track->FindCommand(entry.command, &time))
			{
				// A caller used a shallow refresh after removing an item.
				RefreshItems(true);
				return;
			}
			if (time != entry.time)
			{
				entry.time = time;
				needsSort = true;
			}
		}
	}

	if (needsSort)
		SortByTime();

	// Anchor by command identity, not row index: sorting and filtering can
	// change a command's displayed row without changing the intended anchor.
	if (deep && m_SelectionAnchor && std::none_of(m_Items.cbegin(), m_Items.cend(),
		[this](const CommandEntry& entry) {
			return entry.command == m_SelectionAnchor;
		}))
	{
		m_SelectionAnchor = nullptr;
	}

	RefreshSelectionCache();

	if (!GetSafeHwnd())
		return;

	if (deep || GetItemCount() != int(m_Items.size()))
		SetItemCountEx(int(m_Items.size()), LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL);

	Invalidate(FALSE);
}

BEGIN_MESSAGE_MAP(CCommandList, CListCtrl)
	ON_WM_DRAWITEM()
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_MEASUREITEM_REFLECT()
	ON_NOTIFY_REFLECT(LVN_GETDISPINFO, &CCommandList::OnGetDispInfo)
	ON_NOTIFY_REFLECT(LVN_ITEMACTIVATE, &CCommandList::OnLvnItemActivate)
	ON_NOTIFY_REFLECT(NM_CLICK, &CCommandList::OnNMClick)
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()



// CCommandList message handlers


void CCommandList::OnGetDispInfo(NMHDR* pNMHDR, LRESULT* pResult)
{
	auto* request = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	const int row = request->item.iItem;
	if ((request->item.mask & LVIF_TEXT) && request->item.pszText && request->item.cchTextMax > 0)
	{
		CString value;
		if (row >= 0 && size_t(row) < m_Items.size())
			value = CellText(m_Items[row], request->item.iSubItem);
		_tcsncpy_s(request->item.pszText, size_t(request->item.cchTextMax),
				   value.GetString(), _TRUNCATE);
	}
	*pResult = 0;
}

void CCommandList::MeasureItem(LPMEASUREITEMSTRUCT pmi)
{
	CRect r;
	GetClientRect(r);
	pmi->itemHeight = 32;//((CPossumDocView *)GetParent())->GetTrackHeight();
	pmi->itemWidth = r.Width();
}

BOOL CCommandList::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style &= ~(LVS_SORTASCENDING | LVS_SORTDESCENDING);
	cs.style |= LVS_OWNERDATA | LVS_OWNERDRAWFIXED | LVS_REPORT | LVS_SHOWSELALWAYS;

	return CListCtrl::PreCreateWindow(cs);
}

void CCommandList::DrawItem(LPDRAWITEMSTRUCT pdi)
{
	if (!m_pDoc || size_t(pdi->itemID) >= m_Items.size())
		return;

	const auto& entry = m_Items[pdi->itemID];
	auto* pc = entry.command;
	const auto* pt = entry.track;
	const double t = entry.time;

	Gdiplus::Graphics gr(pdi->hDC);

	Gdiplus::SolidBrush br_sbg(Gdiplus::Color(255, 128, 128, 100));
	Gdiplus::SolidBrush br_text(Gdiplus::Color(255, 255, 255, 255));
	Gdiplus::SolidBrush br_acc(Gdiplus::Color(255, 16, 16, 16));
	Gdiplus::SolidBrush br_sacc(Gdiplus::Color(255, 255, 255, 128));
	Gdiplus::Pen pn_acc(&br_acc, 1);
	Gdiplus::Pen pn_sacc(&br_sacc, 1);

	Gdiplus::Font f_list(pdi->hDC);// , Gdiplus::FontFamily(_T("Arial")), 12);
#if 0
	LOGFONT lf_cmd;
	f_list.GetLogFont(&gr, &lf_cmd);
	lf_cmd.lfWeight = FW_EXTRALIGHT;
	lf_cmd.lfHeight--;
	lf_cmd.lfItalic = TRUE;
	Gdiplus::Font f_cmd(pdi->hDC, &lf_cmd);
#endif
	CRect ritem = pdi->rcItem, rcol[6], rcli;

	GetClientRect(rcli);
	GetSubItemRect(pdi->itemID, 1, LVIR_BOUNDS, rcol[1]);
	GetSubItemRect(pdi->itemID, 2, LVIR_BOUNDS, rcol[2]);
	GetSubItemRect(pdi->itemID, 3, LVIR_BOUNDS, rcol[3]);
	GetSubItemRect(pdi->itemID, 4, LVIR_BOUNDS, rcol[4]);

	const nucleus::ICommandInfo *pci = pc->GetCommandInfo();
	const nucleus::IDevice *pd = pci->GetDevice();

	bool sel = m_Selected.find(pc) != m_Selected.end();

	int cr, cg, cb;
	props::IProperty *pcolor = pc->GetParams()->GetPropertyById('VCLR');
	if (pcolor)
	{
		cr = GetRValue(pcolor->AsInt()), cg = GetGValue(pcolor->AsInt()), cb = GetBValue(pcolor->AsInt());
	}
	else
	{
		cr = cg = cb = 45;
	}

	if (!(pdi->itemID & 1))
	{
		cr /= 4; cg /= 4; cb /= 4;
	}

	Gdiplus::SolidBrush br_bg(Gdiplus::Color(255, cr, cg, cb));

	gr.FillRectangle(sel ? &br_sbg : &br_bg, ritem.left, ritem.top, ritem.Width(), ritem.Height());

	CString str;

	GetSubItemRect(pdi->itemID, 1, LVIR_BOUNDS, rcol[1]);

	props::IProperty *iconprop = pc->GetCommandInfo()->GetDefaultParams()->GetPropertyById('ICON');
	if (iconprop)
	{
		rcol[0].left = rcli.left;
		rcol[0].right = rcol[0].left + GetColumnWidth(0);
		rcol[0].top = rcol[1].top;
		rcol[0].bottom = rcol[1].bottom;

		int h = min(rcol[0].Width(), rcol[0].Height()) / 2;
		CPoint p = rcol[0].CenterPoint();
		CRect ri(p, p);
		ri.InflateRect(h, h, h, h);
		ri.OffsetRect(-GetScrollPos(SB_HORZ), 0);
		Gdiplus::Image *img = (Gdiplus::Image *)theApp.m_PicMan.GetPic(iconprop->AsString());
		if (img)
			gr.DrawImage(img, ri.left, ri.top, ri.Width(), ri.Height());
	}

	Gdiplus::RectF rt;

	UINT h, m, s, f;
	GetTimeComponents(t, h, m, s, f);
	str.Format(_T("%02d:%02d:%02d.%01d"), h, m, s, f);
	gr.SetClip(Gdiplus::Rect(rcol[1].left, rcol[1].top, rcol[1].Width(), rcol[1].Height()));
	gr.MeasureString((LPCTSTR)str, str.GetLength(), &f_list, Gdiplus::PointF(0.0, 0.0), &rt);
	rcol[1].OffsetRect(1, (rcol[1].Height() - (int)rt.Height) / 2);
	gr.DrawString(str, str.GetLength(), &f_list, Gdiplus::PointF((Gdiplus::REAL)rcol[1].left, (Gdiplus::REAL)rcol[1].top), &br_text);

	if (pt)
	{
		str = pt->GetProperties()->GetPropertyById('NAME')->AsString();
		GetSubItemRect(pdi->itemID, 2, LVIR_BOUNDS, rcol[2]);
		gr.SetClip(Gdiplus::Rect(rcol[2].left, rcol[2].top, rcol[2].Width(), rcol[2].Height()));
		gr.MeasureString((LPCTSTR)str, str.GetLength(), &f_list, Gdiplus::PointF(0.0, 0.0), &rt);
		rcol[2].OffsetRect(1, (rcol[2].Height() - (int)rt.Height) / 2);
		gr.DrawString(str, str.GetLength(), &f_list, Gdiplus::PointF((Gdiplus::REAL)rcol[2].left, (Gdiplus::REAL)rcol[2].top), &br_text);
	}

	str = pd->GetName();
	GetSubItemRect(pdi->itemID, 3, LVIR_BOUNDS, rcol[3]);
	gr.SetClip(Gdiplus::Rect(rcol[3].left, rcol[3].top, rcol[3].Width(), rcol[3].Height()));
	gr.MeasureString((LPCTSTR)str, str.GetLength(), &f_list, Gdiplus::PointF(0.0, 0.0), &rt);
	rcol[3].OffsetRect(1, (rcol[3].Height() - (int)rt.Height) / 2);
	gr.DrawString(str, str.GetLength(), &f_list, Gdiplus::PointF((Gdiplus::REAL)rcol[3].left, (Gdiplus::REAL)rcol[3].top), &br_text);

	str = pci->GetName();
	GetSubItemRect(pdi->itemID, 4, LVIR_BOUNDS, rcol[4]);
	gr.SetClip(Gdiplus::Rect(rcol[4].left, rcol[4].top, rcol[4].Width(), rcol[4].Height()));
	gr.MeasureString((LPCTSTR)str, str.GetLength(), &f_list, Gdiplus::PointF(0.0, 0.0), &rt);
	rcol[4].OffsetRect(1, (rcol[4].Height() - (int)rt.Height) / 2);
	gr.DrawString(str, str.GetLength(), &f_list, Gdiplus::PointF((Gdiplus::REAL)rcol[4].left, (Gdiplus::REAL)rcol[4].top), &br_text);

	props::IProperty *ptag = pc->GetParams()->GetPropertyById('TAG');
	if (ptag)
	{
		str = ptag->AsString();
		if (!str.IsEmpty())
		{
			GetSubItemRect(pdi->itemID, 5, LVIR_BOUNDS, rcol[5]);
			gr.SetClip(Gdiplus::Rect(rcol[5].left, rcol[5].top, rcol[5].Width(), rcol[5].Height()));
			gr.MeasureString((LPCTSTR)str, str.GetLength(), &f_list, Gdiplus::PointF(0.0, 0.0), &rt);
			rcol[5].OffsetRect(1, (rcol[5].Height() - (int)rt.Height) / 2);
			gr.DrawString(str, str.GetLength(), &f_list, Gdiplus::PointF((Gdiplus::REAL)rcol[5].left, (Gdiplus::REAL)rcol[5].top), &br_text);
		}
	}
}


int CCommandList::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CListCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;

	SetView(LV_VIEW_DETAILS);
	SetExtendedStyle(GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_TRACKSELECT);
	SetBkColor(RGB(40, 40, 40));

	InsertColumn(0, _T(""), LVCFMT_LEFT, 32);
	InsertColumn(1, _T("Time"), LVCFMT_LEFT, 100);
	InsertColumn(2, _T("Track"), LVCFMT_LEFT, 100);
	InsertColumn(3, _T("Device"), LVCFMT_LEFT, 100);
	InsertColumn(4, _T("Command"), LVCFMT_LEFT, 100);
	InsertColumn(5, _T("Tag"), LVCFMT_LEFT, 100);

	return 0;
}


BOOL CCommandList::OnEraseBkgnd(CDC* pDC)
{
	return FALSE;
}


void CCommandList::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	CMemDC mdc(dc, this);

	Gdiplus::Graphics gr(mdc.GetDC());

	Gdiplus::SolidBrush br_bg(Gdiplus::Color(255, 40, 40, 40));

	CRect r;
	GetClientRect(r);
	gr.FillRectangle(&br_bg, r.left, r.top, r.Width(), r.Height());

	DefWindowProc(WM_PAINT, (WPARAM)(mdc.GetDC().GetSafeHdc()), (LPARAM)0);

	dc.BitBlt(0, 0, r.right, r.bottom, &(mdc.GetDC()), 0, 0, SRCCOPY);
}


void CCommandList::OnLvnItemActivate(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMIA = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	*pResult = 0;
}


void CCommandList::OnNMClick(NMHDR* pNMHDR, LRESULT* pResult)
{
	auto* click = reinterpret_cast<NMITEMACTIVATE*>(pNMHDR);
	*pResult = 0;
	if (!m_pDoc)
		return;

	int row = HitTest(click->ptAction);
	auto* command = CommandAt(row);
	bool ctrl = (click->uKeyFlags & LVKF_CONTROL) != 0;
	bool shift = (click->uKeyFlags & LVKF_SHIFT) != 0;

	if (shift && command)
	{
		// Find the last explicitly clicked item in the *current* chronological
		// list.  Keep the anchor unchanged during subsequent Shift-clicks.
		int anchor = row;
		if (m_SelectionAnchor)
		{
			for (size_t i = 0; i < m_Items.size(); ++i)
			{
				if (m_Items[i].command == m_SelectionAnchor)
				{
					anchor = int(i);
					break;
				}
			}
		}

		// Shift selects the range; Ctrl+Shift adds it to the existing selection.
		// Batch notifications: adding one command must not redraw the timeline
		// or rebuild the property inspector thousands of times.
		if (!ctrl)
			m_pDoc->ClearSelection();

		int first = std::min<int>(anchor, row);
		int last = std::max<int>(anchor, row);

		for (int i = first; i <= last; ++i)
		{
			auto* cmd = CommandAt(i);
			if (cmd && (!ctrl || m_Selected.find(cmd) == m_Selected.end()))
				m_pDoc->AddToSelection(cmd, false);
		}

		theApp.RefreshCommandList(false);
		theApp.RedrawActiveTimeline();
		m_pDoc->UpdatePropertiesForSelection();
		return;
	}

	if (!ctrl || !command)
		m_pDoc->ClearSelection();

	if (command)
	{
		if (ctrl && m_pDoc->IsSelected(command))
			m_pDoc->RemoveFromSelection(command);
		else
			m_pDoc->AddToSelection(command);
	}

	// Ctrl-click also becomes a valid starting point for the next Shift-click.
	m_SelectionAnchor = command;
}


enum EPopupCommand
{
	PC_DELETE = 1,
	PC_SETCOLOR,
	PC_ZOOMTO,
	PC_FILTER_ALL = 900,
	PC_FILTER_FIRST = 1000
};

void CCommandList::OnContextMenu(CWnd*, CPoint point)
{
	// WM_CONTEXTMENU uses (-1,-1) for keyboard invocation.
	if (point.x == -1 && point.y == -1)
		GetCursorPos(&point);

	CPoint client = point;
	ScreenToClient(&client);
	int row = HitTest(client);
	auto* command = CommandAt(row);
	auto* doc = m_pDoc;

	if (command && doc && !doc->IsSelected(command))
	{
		doc->ClearSelection();
		doc->AddToSelection(command);
	}

	HMENU menu = ::CreatePopupMenu();
	if (!menu)
		return;

	if (command && doc)
	{
		MyAppendMenuItem(menu, MFT_STRING, doc->GetNumSelected() > 1 ? _T("Delete Commands") : _T("Delete Command"), true, 0, PC_DELETE);
		MyAppendMenuItem(menu, MFT_SEPARATOR);
		MyAppendMenuItem(menu, MFT_STRING, _T("Set Color"), true, 0, PC_SETCOLOR);
		MyAppendMenuItem(menu, MFT_SEPARATOR);
		MyAppendMenuItem(menu, MFT_STRING, _T("Zoom Timeline Extents"), true, 0, PC_ZOOMTO);
		MyAppendMenuItem(menu, MFT_SEPARATOR);
	}

	// Saved named filters can now be selected without modifying the project.
	// An empty active set means "show all" (the existing filter convention).
	HMENU filterMenu = ::CreatePopupMenu();
	std::vector<CPossum::TFilterMap::const_iterator> choices;

	if (filterMenu)
	{
		const bool showAll = theApp.m_ActiveCommandFilter.empty();
		::AppendMenu(filterMenu, MF_STRING | (showAll ? MF_CHECKED : 0),
					 PC_FILTER_ALL, _T("Show All"));
		if (!theApp.m_FilterMap.empty())
			::AppendMenu(filterMenu, MF_SEPARATOR, 0, nullptr);

		for (auto it = theApp.m_FilterMap.cbegin(); it != theApp.m_FilterMap.cend(); ++it)
		{
			if (choices.size() >= 0xE000 - PC_FILTER_FIRST)
				break;
			const UINT id = PC_FILTER_FIRST + UINT(choices.size());
			const bool checked = !showAll && theApp.m_ActiveCommandFilter == it->second;
			::AppendMenu(filterMenu, MF_STRING | (checked ? MF_CHECKED : 0), id, it->first.c_str());
			choices.push_back(it);
		}
		::AppendMenu(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(filterMenu), _T("Filter"));
	}

	const UINT choice = ::TrackPopupMenu(menu, TPM_NONOTIFY | TPM_RETURNCMD | TPM_LEFTALIGN, point.x, point.y, 0, m_hWnd, nullptr);

	if (choice == PC_FILTER_ALL)
	{
		theApp.m_ActiveCommandFilter.clear();
		RefreshItems(true);
	}
	else if (choice >= PC_FILTER_FIRST && choice - PC_FILTER_FIRST < choices.size())
	{
		theApp.m_ActiveCommandFilter = choices[choice - PC_FILTER_FIRST]->second;
		RefreshItems(true);
	}
	else if (doc && command)
	{
		switch (choice)
		{
			case PC_DELETE:
				if (IDYES == MessageBox(_T("Are you sure you want to delete the selected command(s)?"),
										_T("Delete Selected Commands?"), MB_YESNO))
				{
					// Snapshot before clearing the selection, and refresh exactly once.
					std::vector<nucleus::ICommand*> selected;
					selected.reserve(doc->GetNumSelected());
					for (size_t i = 0; i < doc->GetNumSelected(); i++)
						selected.push_back(doc->GetSelection(i));
					doc->ClearSelection();
					for (auto* cmd : selected)
					{
						auto* track = const_cast<nucleus::ITrack*>(cmd->GetOwner());
						if (track)
							track->RemoveCommand(cmd);
					}
					doc->SetModifiedFlag();
					theApp.RefreshCommandList(true);
					theApp.RedrawActiveTimeline();
				}
				break;

			case PC_SETCOLOR:
			{
				CMFCColorDialog dialog;
				if (dialog.DoModal() == IDOK)
				{
					COLORREF color = dialog.GetColor();
					for (size_t i = 0; i < doc->GetNumSelected(); i++)
					{
						auto* cmd = doc->GetSelection(i);
						auto* prop = cmd->GetParams()->CreateProperty(_T("ViewColor"), 'VCLR');
						if (prop)
						{
							prop->Flags().Set(props::IProperty::PROPFLAG(props::IProperty::EPropFlag::HIDDEN));
							prop->SetInt(int64_t(color));
							prop->SetAspect(props::IProperty::PROPERTY_ASPECT::PA_COLOR_RGB);
						}
					}
					doc->ClearSelection();
					doc->SetModifiedFlag();
					theApp.RedrawActiveTimeline();
					RefreshItems(false);
				}
			}
			break;

			case PC_ZOOMTO:
			{
				double first = DBL_MAX, last = 0.0;

				for (size_t i = 0; i < doc->GetNumSelected(); i++)
				{
					auto* cmd = doc->GetSelection(i);
					auto* track = const_cast<nucleus::ITrack*>(cmd->GetOwner());
					double time = 0.0;

					if (track && track->FindCommand(cmd, &time))
					{
						first = std::min<double>(first, time);
						last = std::max<double>(last, time + cmd->GetDuration());
					}
				}

				if (first != DBL_MAX)
					theApp.SetActiveTimelineExtents(std::max<double>(0.0, first - 1.0), last + 1.0);
			}
			break;
		}
	}

	::DestroyMenu(menu); // destroys the attached Filter submenu, too
}
