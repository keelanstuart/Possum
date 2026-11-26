#pragma once
#include <vector>
#include <string>

// Aggregated icon catalog, not a single-folder ExplorerBrowser.
// Directory and extension configuration is programmatic for now.
class CMediaExplorerCtrl : public CListCtrl
{
    DECLARE_DYNAMIC(CMediaExplorerCtrl)
public:
    CMediaExplorerCtrl() = default;
    virtual ~CMediaExplorerCtrl() = default;
    BOOL Create(DWORD style, const RECT &rect, CWnd *parent, UINT id);
    void AddDirectory(const TCHAR *directory);
    void AddExtension(const TCHAR *extension);
    void ClearDirectories();
    void ClearExtensions();
    void Refresh();
    bool NavigateTo(const TCHAR *directory); // compatibility with old panel callers

protected:
    std::vector<CString> m_Directories;
    std::vector<CString> m_Extensions;
    std::vector<CString> m_Files;
    CImageList m_Images;
    afx_msg void OnBeginDrag(NMHDR *header, LRESULT *result);
    afx_msg void OnGetInfoTip(NMHDR *header, LRESULT *result);
    afx_msg void OnContextMenu(CWnd *window, CPoint screen);
    DECLARE_MESSAGE_MAP()
};
