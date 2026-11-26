#include "stdafx.h"
#include "Possum.h"
#include "MediaPanel.h"
#include <shlwapi.h>

IMPLEMENT_DYNAMIC(CMediaPanel, CDockablePane)
BEGIN_MESSAGE_MAP(CMediaPanel, CDockablePane)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

CMediaPanel::CMediaPanel() = default;
CMediaPanel::~CMediaPanel() = default;

int CMediaPanel::OnCreate(LPCREATESTRUCT create)
{
    if (CDockablePane::OnCreate(create) == -1) return -1;
    CRect empty(0, 0, 0, 0);
    if (!m_wndMediaView.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | WS_CLIPSIBLINGS,
        empty, this, 1)) return -1;

    m_wndMediaView.AddExtension(_T(".wav"));
    TCHAR root[MAX_PATH] = {};
    if (::GetModuleFileName(nullptr, root, MAX_PATH))
    {
        ::PathRemoveFileSpec(root);
        CString samples(root); samples += _T("\\Samples");
        CString media(root); media += _T("\\Media");
        m_wndMediaView.AddDirectory(samples);
        m_wndMediaView.AddDirectory(media);
    }
    m_wndMediaView.Refresh();
    return 0;
}

void CMediaPanel::OnSize(UINT type, int cx, int cy)
{
    CDockablePane::OnSize(type, cx, cy);
    if (m_wndMediaView.GetSafeHwnd())
        m_wndMediaView.MoveWindow(0, 0, cx, cy);
}

bool CMediaPanel::NavigateTo(LPCTSTR path)
{
    return m_wndMediaView.NavigateTo(path);
}

BOOL CMediaPanel::OnEraseBkgnd(CDC *) { return FALSE; }
