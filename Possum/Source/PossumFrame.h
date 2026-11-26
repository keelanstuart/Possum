
// PossumFrame.h : interface of the CPossumMainFrame class
//

#pragma once
#include "PossumDocView.h"
#include "PropertiesPanel.h"
#include "ProductBrowserPanel.h"
#include "DevicePanel.h"
#include "CommandListPanel.h"
#include "MFCToolBarEx.h"
#include "MediaPanel.h"

// MDI child activation and UI-thread playback refresh.
constexpr UINT WM_POSSUM_MDI_ACTIVE = WM_APP + 0x231;
constexpr UINT_PTR POSSUM_PLAYHEAD_TIMER = 0x231;

class CPossumMDIChildFrame : public CMDIChildWndEx
{
    DECLARE_DYNCREATE(CPossumMDIChildFrame)
protected:
    afx_msg void OnMDIActivate(BOOL active, CWnd *activated, CWnd *deactivated);
    DECLARE_MESSAGE_MAP()
};

class CPossumMainFrame : public CMDIFrameWndEx
{
	DECLARE_DYNAMIC(CPossumMainFrame)
public:
	CPossumMainFrame();

// Attributes
public:

// Operations
public:

// Overrides
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL LoadFrame(UINT nIDResource, DWORD dwDefaultStyle = WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, CWnd* pParentWnd = NULL, CCreateContext* pContext = NULL);

// Implementation
public:
	virtual ~CPossumMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	void SetActiveProperties(props::IPropertySet *props, bool readonly = false, const TCHAR *title = nullptr, nucleus::ICommandInfo* pci = nullptr, nucleus::IDevice* pd = nullptr);
	void RedrawActiveTimeline();
	void SetActiveTimelineExtents(double start_time, double end_time);
	void RefreshCommandList(bool deep);
	bool IsAuditionEnabled() const;
	CPossumDocView *GetActiveDocView();
	CPossumDocView *FindDocViewByTrackSet(const nucleus::ITrackSet *pts);

protected:  // control bar embedded members
	CMFCMenuBar				m_wndMenuBar;
	CMFCToolBar				m_wndToolBar;
	CMFCToolBar				m_wndPlaybackToolBar;
	CMFCToolBarEx			m_wndLayoutToolBar;
	CMFCStatusBar			m_wndStatusBar;
	CMFCToolBarImages		m_UserImages;
	CPropertiesPanel		m_wndProperties;
	CMFCCaptionBar			m_wndCaptionBar;
	CProductBrowserPanel	m_wndProductBrowser;
	CDevicePanel			m_wndDevices;
	CCommandListPanel		m_wndCommandList;
	CMediaPanel				m_wndMedia;

// Generated message map functions
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnWindowManager();
	afx_msg void OnViewCustomize();
	afx_msg LRESULT OnToolbarCreateNew(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnToolbarReset(WPARAM wp, LPARAM lp);
	afx_msg void OnApplicationLook(UINT id);
	afx_msg void OnUpdateApplicationLook(CCmdUI* pCmdUI);
	afx_msg void OnViewPropertiesWindow();
	afx_msg void OnUpdateViewPropertiesWindow(CCmdUI* pCmdUI);
	afx_msg void OnViewMediaWindow();
	afx_msg void OnUpdateViewMediaWindow(CCmdUI *pCmdUI);
	afx_msg void OnViewProductsWindow();
	afx_msg void OnUpdateViewProductsWindow(CCmdUI* pCmdUI);
	afx_msg void OnViewDevicesWindow();
	afx_msg void OnUpdateViewDevicesWindow(CCmdUI* pCmdUI);
	afx_msg void OnViewCommandListWindow();
	afx_msg void OnUpdateViewCommandListWindow(CCmdUI* pCmdUI);
	afx_msg void OnViewCaptionBar();
	afx_msg void OnUpdateViewCaptionBar(CCmdUI* pCmdUI);
	afx_msg void OnOptions();
	afx_msg void OnLayoutCommand(UINT id);
	afx_msg void OnUpdateLayoutCommand(CCmdUI *ui);
	afx_msg void OnLayoutAudition();
	afx_msg void OnUpdateLayoutAudition(CCmdUI *ui);
	afx_msg void OnLayoutSnap();
	afx_msg void OnUpdateLayoutSnap(CCmdUI *ui);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	DECLARE_MESSAGE_MAP()

	BOOL CreateDockingWindows();
	void SetDockingWindowIcons(BOOL bHiColorIcons);
	BOOL CreateCaptionBar();

public:
	afx_msg void OnFileNew();
	afx_msg void OnFileOpen();
	afx_msg void OnFileSave();
	afx_msg void OnFileSaveAs();
	afx_msg void OnFileClose();
	afx_msg void OnFileOpenRecent(UINT nID);

	virtual void OnFinalRelease();
	afx_msg void OnTrackheightTiny();
	afx_msg void OnTrackheightSmall();
	afx_msg void OnTrackheightMedium();
	afx_msg void OnTrackheightLarge();
	afx_msg void OnTrackheightHuge();
	afx_msg void OnClose();
	afx_msg void OnTimer(UINT_PTR id);
	afx_msg LRESULT OnMDIActiveChanged(WPARAM, LPARAM);
};


