#pragma once

#include <NuDevice.h>
#include <NuCommand.h>
#include <NuTrack.h>
#include <vector>
#include <unordered_set>

class CPossumDocument;

// Chronologically ordered, filterable virtual list of commands.
// A row index maps to m_Items; the Windows list view never owns ICommand objects.
class CCommandList : public CListCtrl
{
    DECLARE_DYNAMIC(CCommandList)

public:
    CCommandList();
    virtual ~CCommandList();

    // deep: document/track/command/filter changes.  Shallow refresh re-reads
    // times (notes may have moved) and selection without recreating Win32 rows.
    void RefreshItems(bool deep);
    bool IsActiveDocument();

protected:
    struct CommandEntry
    {
        nucleus::ICommand* command = nullptr;
        nucleus::ITrack* track = nullptr;
        double time = 0.0;
        size_t trackIndex = 0;
        size_t sequence = 0;
    };

    CPossumDocument* m_pDoc = nullptr;
    std::vector<CommandEntry> m_Items;
    std::unordered_set<const nucleus::ICommand*> m_Selected;
    nucleus::ICommand* m_SelectionAnchor = nullptr; // Non-owning; validated on rebuild.
    CBrush m_brBackground;

    nucleus::ICommand* CommandAt(int index) const;
    void RefreshSelectionCache();
    void SortByTime();
    CString CellText(const CommandEntry& entry, int column) const;

    DECLARE_MESSAGE_MAP()

public:
    afx_msg void DrawItem(LPDRAWITEMSTRUCT pdi);
    afx_msg void MeasureItem(LPMEASUREITEMSTRUCT pmi);
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnPaint();
    afx_msg void OnGetDispInfo(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnLvnItemActivate(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnNMClick(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
};
