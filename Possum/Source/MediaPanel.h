#pragma once
#include "MediaExplorerCtrl.h"

class CMediaPanel : public CDockablePane
{
    DECLARE_DYNAMIC(CMediaPanel)
public:
    CMediaPanel();
    virtual ~CMediaPanel();
    bool NavigateTo(LPCTSTR path);
    CMediaExplorerCtrl &MediaView() { return m_wndMediaView; }
protected:
    CMediaExplorerCtrl m_wndMediaView;
    DECLARE_MESSAGE_MAP()
    afx_msg int OnCreate(LPCREATESTRUCT create);
    afx_msg void OnSize(UINT type, int cx, int cy);
    afx_msg BOOL OnEraseBkgnd(CDC *dc);
};
