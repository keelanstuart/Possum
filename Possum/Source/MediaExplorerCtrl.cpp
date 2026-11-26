#include "stdafx.h"
#include "MediaExplorerCtrl.h"
#include "Util.h"
#include <filesystem>
#include <set>
#include <algorithm>
#include <cwctype>
#include <shlwapi.h>
#include <shellapi.h>
#include <shlobj.h>

IMPLEMENT_DYNAMIC(CMediaExplorerCtrl, CListCtrl)
BEGIN_MESSAGE_MAP(CMediaExplorerCtrl, CListCtrl)
    ON_NOTIFY_REFLECT(LVN_BEGINDRAG, &CMediaExplorerCtrl::OnBeginDrag)
    ON_NOTIFY_REFLECT(LVN_GETINFOTIP, &CMediaExplorerCtrl::OnGetInfoTip)
    ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

BOOL CMediaExplorerCtrl::Create(DWORD style, const RECT &rect, CWnd *parent, UINT id)
{
    if (!CListCtrl::Create(style | LVS_ICON | LVS_AUTOARRANGE | LVS_SHOWSELALWAYS | LVS_SINGLESEL,
        rect, parent, id))
        return FALSE;
    m_Images.Create(32, 32, ILC_COLOR32 | ILC_MASK, 16, 16);
    SetImageList(&m_Images, LVSIL_NORMAL);
    SetBkColor(RGB(39, 41, 46));
    SetTextBkColor(RGB(39, 41, 46));
    SetTextColor(RGB(228, 230, 233));
    SetExtendedStyle(GetExtendedStyle() | LVS_EX_INFOTIP);
    return TRUE;
}

void CMediaExplorerCtrl::AddDirectory(const TCHAR *directory)
{
    if (!directory || !*directory)
        return;
    CString path(directory);
    for (const auto &existing : m_Directories)
        if (!existing.CompareNoCase(path)) return;
    m_Directories.push_back(path);
}

void CMediaExplorerCtrl::AddExtension(const TCHAR *extension)
{
    if (!extension || !*extension) return;
    CString ext(extension);
    if (ext[0] != _T('.')) ext = _T(".") + ext;
    for (const auto &existing : m_Extensions)
        if (!existing.CompareNoCase(ext)) return;
    m_Extensions.push_back(ext);
}

void CMediaExplorerCtrl::ClearDirectories() { m_Directories.clear(); }
void CMediaExplorerCtrl::ClearExtensions() { m_Extensions.clear(); }

bool CMediaExplorerCtrl::NavigateTo(const TCHAR *directory)
{
    ClearDirectories();
    AddDirectory(directory);
    Refresh();
    return true;
}

void CMediaExplorerCtrl::Refresh()
{
    if (!GetSafeHwnd()) return;
    DeleteAllItems();
    SetImageList(nullptr, LVSIL_NORMAL);
    m_Images.DeleteImageList();
    m_Images.Create(32, 32, ILC_COLOR32 | ILC_MASK, 16, 16);
    SetImageList(&m_Images, LVSIL_NORMAL);
    m_Files.clear();
    std::set<std::wstring> seen;
    std::error_code error;

    for (const auto &root : m_Directories)
    {
        error.clear();
        std::filesystem::path path((LPCTSTR)root);
        if (!std::filesystem::is_directory(path, error)) continue;
        std::filesystem::recursive_directory_iterator it(path,
            std::filesystem::directory_options::skip_permission_denied, error), end;
        while (it != end)
        {
            if (!error && it->is_regular_file(error))
            {
                const std::filesystem::path file = it->path();
                CString ext(file.extension().c_str());
                bool accepted = m_Extensions.empty();
                for (const auto &match : m_Extensions)
                    if (ext.CompareNoCase(match) == 0) { accepted = true; break; }
                if (accepted)
                {
                    CString full(file.c_str());
                    std::wstring key((LPCTSTR)full);
                    std::transform(key.begin(), key.end(), key.begin(), [](wchar_t ch) { return wchar_t(towlower(ch)); });
                    if (seen.insert(key).second)
                        m_Files.push_back(full);
                }
            }
            error.clear();
            it.increment(error);
            if (error) error.clear();
        }
    }

    std::sort(m_Files.begin(), m_Files.end(), [](const CString &a, const CString &b)
    {
        return a.CompareNoCase(b) < 0;
    });
    for (size_t i = 0; i < m_Files.size(); ++i)
    {
        SHFILEINFO info{};
        const CString &file = m_Files[i];
        int icon = -1;
        if (::SHGetFileInfo(file, 0, &info, sizeof(info), SHGFI_ICON | SHGFI_LARGEICON))
        {
            icon = m_Images.Add(info.hIcon);
            ::DestroyIcon(info.hIcon);
        }
        CString name = ::PathFindFileName((LPCTSTR)file);
        int index = InsertItem(int(i), name, icon);
        if (index >= 0) SetItemData(index, DWORD_PTR(i));
    }
}

void CMediaExplorerCtrl::OnContextMenu(CWnd *, CPoint screen)
{
	HMENU menu = ::CreatePopupMenu();
	if (!menu)
		return;

	constexpr UINT ID_REFRESH = 1;
	MyAppendMenuItem(menu, MFT_STRING, _T("Refresh media"), true, 0, ID_REFRESH);

	if (screen.x == -1 && screen.y == -1)
		::GetCursorPos(&screen);

	UINT chosen = ::TrackPopupMenu(menu, TPM_NONOTIFY | TPM_RETURNCMD | TPM_LEFTALIGN | TPM_RIGHTBUTTON,
		screen.x, screen.y, 0, GetSafeHwnd(), NULL);
	::DestroyMenu(menu);

	if (chosen == ID_REFRESH)
		Refresh();
}

void CMediaExplorerCtrl::OnBeginDrag(NMHDR *header, LRESULT *result)
{
    *result = 0;
    auto *info = reinterpret_cast<NMLISTVIEW *>(header);
    if (info->iItem < 0) return;
    size_t index = size_t(GetItemData(info->iItem));
    if (index >= m_Files.size()) return;

    // Standard CF_HDROP data, compatible with Explorer and OLE drop targets.
    const CString &path = m_Files[index];
    const SIZE_T bytes = sizeof(DROPFILES) + (path.GetLength() + 2) * sizeof(TCHAR);
    HGLOBAL memory = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, bytes);
    if (!memory) return;
    auto *data = static_cast<DROPFILES *>(::GlobalLock(memory));
    if (!data) { ::GlobalFree(memory); return; }
    data->pFiles = sizeof(DROPFILES);
#ifdef UNICODE
    data->fWide = TRUE;
#endif
    memcpy(reinterpret_cast<BYTE *>(data) + sizeof(DROPFILES),
        (LPCTSTR)path, (path.GetLength() + 1) * sizeof(TCHAR));
    ::GlobalUnlock(memory);
    COleDataSource source;
    source.CacheGlobalData(CF_HDROP, memory);
    source.DoDragDrop(DROPEFFECT_COPY);
}

void CMediaExplorerCtrl::OnGetInfoTip(NMHDR *header, LRESULT *result)
{
    *result = 0;
    auto *tip = reinterpret_cast<NMLVGETINFOTIP *>(header);
    if (tip->iItem < 0) return;
    const size_t index = size_t(GetItemData(tip->iItem));
    if (index < m_Files.size())
        _tcsncpy_s(tip->pszText, tip->cchTextMax, m_Files[index], _TRUNCATE);
}
