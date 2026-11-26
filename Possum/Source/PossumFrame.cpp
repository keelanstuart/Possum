
// PossumFrame.cpp : implementation of the CPossumMainFrame class
//

#include "stdafx.h"
#include "Possum.h"

#include "PossumFrame.h"


#include "PlaybackToolBarButtons.h"
#include "PossumLayout.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CPossumMainFrame

IMPLEMENT_DYNCREATE(CPossumMDIChildFrame, CMDIChildWndEx)

BEGIN_MESSAGE_MAP(CPossumMDIChildFrame, CMDIChildWndEx)
    ON_WM_MDIACTIVATE()
END_MESSAGE_MAP()

void CPossumMDIChildFrame::OnMDIActivate(BOOL active, CWnd *activated, CWnd *deactivated)
{
    CMDIChildWndEx::OnMDIActivate(active, activated, deactivated);
    if (active && GetMDIFrame())
        GetMDIFrame()->PostMessage(WM_POSSUM_MDI_ACTIVE);
}

IMPLEMENT_DYNAMIC(CPossumMainFrame, CMDIFrameWndEx)

const int  iMaxUserToolbars = 10;
const UINT uiFirstUserToolBarId = AFX_IDW_CONTROLBAR_FIRST + 40;
const UINT uiLastUserToolBarId = uiFirstUserToolBarId + iMaxUserToolbars - 1;

BEGIN_MESSAGE_MAP(CPossumMainFrame, CMDIFrameWndEx)
	ON_WM_CREATE()
	ON_COMMAND_RANGE(ID_LAYOUT_TIME_LEFT, ID_LAYOUT_PITCH_HIGH, &CPossumMainFrame::OnLayoutCommand)
	ON_UPDATE_COMMAND_UI_RANGE(ID_LAYOUT_TIME_LEFT, ID_LAYOUT_PITCH_HIGH, &CPossumMainFrame::OnUpdateLayoutCommand)
	ON_COMMAND(ID_LAYOUT_AUDITION, &CPossumMainFrame::OnLayoutAudition)
	ON_UPDATE_COMMAND_UI(ID_LAYOUT_AUDITION, &CPossumMainFrame::OnUpdateLayoutAudition)
	ON_COMMAND(ID_LAYOUT_SNAP, &CPossumMainFrame::OnLayoutSnap)
	ON_UPDATE_COMMAND_UI(ID_LAYOUT_SNAP, &CPossumMainFrame::OnUpdateLayoutSnap)
	ON_COMMAND(ID_WINDOW_MANAGER, &CPossumMainFrame::OnWindowManager)
	ON_COMMAND(ID_VIEW_CUSTOMIZE, &CPossumMainFrame::OnViewCustomize)
	ON_REGISTERED_MESSAGE(AFX_WM_CREATETOOLBAR, &CPossumMainFrame::OnToolbarCreateNew)
	ON_REGISTERED_MESSAGE(AFX_WM_RESETTOOLBAR, &CPossumMainFrame::OnToolbarReset)
	ON_COMMAND_RANGE(ID_VIEW_APPLOOK_WIN_2000, ID_VIEW_APPLOOK_WINDOWS_7, &CPossumMainFrame::OnApplicationLook)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_APPLOOK_WIN_2000, ID_VIEW_APPLOOK_WINDOWS_7, &CPossumMainFrame::OnUpdateApplicationLook)
	ON_COMMAND(ID_VIEW_CAPTION_BAR, &CPossumMainFrame::OnViewCaptionBar)
	ON_UPDATE_COMMAND_UI(ID_VIEW_CAPTION_BAR, &CPossumMainFrame::OnUpdateViewCaptionBar)
	ON_COMMAND(ID_TOOLS_OPTIONS, &CPossumMainFrame::OnOptions)
	ON_COMMAND(ID_VIEW_PROPERTIESWND, &CPossumMainFrame::OnViewPropertiesWindow)
	ON_UPDATE_COMMAND_UI(ID_VIEW_PROPERTIESWND, &CPossumMainFrame::OnUpdateViewPropertiesWindow)
	ON_COMMAND(ID_VIEW_MEDIAWND, &CPossumMainFrame::OnViewMediaWindow)
	ON_UPDATE_COMMAND_UI(ID_VIEW_MEDIAWND, &CPossumMainFrame::OnUpdateViewMediaWindow)
	ON_COMMAND(ID_VIEW_PRODUCTSWND, &CPossumMainFrame::OnViewProductsWindow)
	ON_UPDATE_COMMAND_UI(ID_VIEW_PRODUCTSWND, &CPossumMainFrame::OnUpdateViewProductsWindow)
	ON_COMMAND(ID_VIEW_DEVICESWND, &CPossumMainFrame::OnViewDevicesWindow)
	ON_UPDATE_COMMAND_UI(ID_VIEW_DEVICESWND, &CPossumMainFrame::OnUpdateViewDevicesWindow)
	ON_COMMAND(ID_VIEW_COMMANDLISTWND, &CPossumMainFrame::OnViewCommandListWindow)
	ON_UPDATE_COMMAND_UI(ID_VIEW_COMMANDLISTWND, &CPossumMainFrame::OnUpdateViewCommandListWindow)
	ON_WM_SETTINGCHANGE()
	ON_COMMAND(ID_TRACKHEIGHT_TINY,&CPossumMainFrame::OnTrackheightTiny)
	ON_COMMAND(ID_TRACKHEIGHT_SMALL,&CPossumMainFrame::OnTrackheightSmall)
	ON_COMMAND(ID_TRACKHEIGHT_MEDIUM,&CPossumMainFrame::OnTrackheightMedium)
	ON_COMMAND(ID_TRACKHEIGHT_LARGE,&CPossumMainFrame::OnTrackheightLarge)
	ON_COMMAND(ID_TRACKHEIGHT_HUGE,&CPossumMainFrame::OnTrackheightHuge)
	ON_WM_CLOSE()
	ON_WM_TIMER()
	ON_MESSAGE(WM_POSSUM_MDI_ACTIVE, &CPossumMainFrame::OnMDIActiveChanged)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

// CPossumMainFrame construction/destruction

CPossumMainFrame::CPossumMainFrame()
{
	EnableActiveAccessibility();
	// TODO: add member initialization code here
	theApp.m_nAppLook = theApp.GetInt(_T("ApplicationLook"), ID_VIEW_APPLOOK_VS_2008);
}

CPossumMainFrame::~CPossumMainFrame()
{
}

int CPossumMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMDIFrameWndEx::OnCreate(lpCreateStruct) == -1)
		return -1;

	BOOL bNameValid;

	CMDITabInfo mdiTabParams;
	mdiTabParams.m_style = CMFCTabCtrl::STYLE_3D_ONENOTE; // other styles available...
	mdiTabParams.m_bActiveTabCloseButton = FALSE;      // set to FALSE to place close button at right of tab area
	mdiTabParams.m_bTabCloseButton = FALSE;
	mdiTabParams.m_bTabIcons = FALSE;    // set to TRUE to enable document icons on MDI taba
	mdiTabParams.m_bAutoColor = TRUE;    // set to FALSE to disable auto-coloring of MDI tabs
	mdiTabParams.m_bDocumentMenu = TRUE; // enable the document menu at the right edge of the tab area
	EnableMDITabbedGroups(TRUE, mdiTabParams);

	if (!m_wndMenuBar.Create(this))
	{
		TRACE0("Failed to create menubar\n");
		return -1;      // fail to create
	}

	m_wndMenuBar.SetPaneStyle(m_wndMenuBar.GetPaneStyle() | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY);

	// prevent the menu bar from taking the focus on activation
	CMFCPopupMenu::SetForceMenuFocus(FALSE);

	CMFCToolBarInfo tbi, tbip;
	const CRect r1(1, 1, 1, 1);
	DWORD dwCtrlStyle = TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | CBRS_SIZE_DYNAMIC;
	DWORD dwStyle = AFX_DEFAULT_TOOLBAR_STYLE | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC;

	if (!m_wndToolBar.CreateEx(this, dwCtrlStyle, dwStyle, r1, IDR_MAINFRAME_256) ||
		!m_wndToolBar.LoadToolBarEx(IDR_MAINFRAME_256, tbi, TRUE))
	{
		TRACE0("Failed to create standard toolbar\n");
		return -1;      // fail to create
	}

	CString strToolBarName;
	bNameValid = strToolBarName.LoadString(IDS_TOOLBAR_STANDARD);
	ASSERT(bNameValid);
	m_wndToolBar.SetWindowText(strToolBarName);

	if (!m_wndPlaybackToolBar.CreateEx(this, dwCtrlStyle, dwStyle, r1, IDR_PLAYBACK_256) ||
		!m_wndPlaybackToolBar.LoadToolBarEx(IDR_PLAYBACK_256, tbip, TRUE))
	{
		TRACE0("Failed to create playback toolbar\n");
		return -1;      // fail to create
	}

	CString strPlaybackToolBarName;
	bNameValid = strPlaybackToolBarName.LoadString(IDS_TOOLBAR_PLAYBACK);
	ASSERT(bNameValid);
	m_wndPlaybackToolBar.SetWindowText(strPlaybackToolBarName);

	CMFCToolBarInfo tbil;
    if (!m_wndLayoutToolBar.CreateEx(this, dwCtrlStyle, dwStyle, r1, IDR_LAYOUT_256) ||
        !m_wndLayoutToolBar.LoadToolBarEx(IDR_LAYOUT_256, tbil, TRUE))
    {
        TRACE0("Failed to create layout toolbar\n");
        return -1;
    }
    m_wndLayoutToolBar.SetWindowText(_T("Layout"));

    CString strCustomize;
	bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	ASSERT(bNameValid);
#if 0
	m_wndToolBar.EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);

	// Allow user-defined toolbars operations:
	InitUserToolbars(NULL, uiFirstUserToolBarId, uiLastUserToolBarId);
#endif

	if (!m_wndStatusBar.Create(this))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}
	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT));

	EnableDocking(CBRS_ALIGN_ANY);

	m_wndMenuBar.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_wndMenuBar);

	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_wndToolBar);

	m_wndPlaybackToolBar.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_wndPlaybackToolBar);
	m_wndLayoutToolBar.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_wndLayoutToolBar);

	// enable Visual Studio 2005 style docking window behavior
	CDockingManager::SetDockingMode(DT_SMART);
	// enable Visual Studio 2005 style docking window auto-hide behavior
	EnableAutoHidePanes(CBRS_ALIGN_ANY);

	// Create a caption bar:
	if (!CreateCaptionBar())
	{
		TRACE0("Failed to create caption bar\n");
		return -1;      // fail to create
	}

	// Load menu item image (not placed on any standard toolbars):
	CMFCToolBar::AddToolBarForImageCollection(IDR_MENU_IMAGES, theApp.m_bHiColorIcons ? IDB_MENU_IMAGES_24 : 0);
	CMFCToolBar::AddToolBarForImageCollection(IDR_PLAYBACK, theApp.m_bHiColorIcons ? IDR_PLAYBACK_256 : 0);

	// create docking windows
	if (!CreateDockingWindows())
	{
		TRACE0("Failed to create docking windows\n");
		return -1;
	}

	CDockablePane* pTabbedBar = NULL;


	m_wndProperties.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_wndProperties);

	m_wndProductBrowser.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_wndProductBrowser);

	m_wndDevices.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_wndDevices);

	m_wndCommandList.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_wndCommandList);

	m_wndMedia.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_wndMedia);

	// set the visual manager and style based on persisted value
	OnApplicationLook(theApp.m_nAppLook);

	// Enable enhanced windows management dialog
	EnableWindowsDialog(ID_WINDOW_MANAGER, ID_WINDOW_MANAGER, TRUE);

	// Enable toolbar and docking window menu replacement
	EnablePaneMenu(TRUE, ID_VIEW_CUSTOMIZE, strCustomize, ID_VIEW_TOOLBAR);

#if 0
	// enable quick (Alt+drag) toolbar customization
	CMFCToolBar::EnableQuickCustomization();

	if (CMFCToolBar::GetUserImages() == NULL)
	{
		// load user-defined toolbar images
		if (m_UserImages.Load(_T(".\\UserImages.bmp")))
		{
			CMFCToolBar::SetUserImages(&m_UserImages);
		}
	}
#endif

// We don't want to show only "relevant" commands in the menus-- we want all of them
#if 0
	// enable menu personalization (most-recently used commands)
	// TODO: define your own basic commands, ensuring that each pulldown menu has at least one basic command.
	CList<UINT, UINT> lstBasicCommands;

	lstBasicCommands.AddTail(ID_FILE_NEW);
	lstBasicCommands.AddTail(ID_FILE_OPEN);
	lstBasicCommands.AddTail(ID_FILE_SAVE);
	lstBasicCommands.AddTail(ID_FILE_PRINT);
	lstBasicCommands.AddTail(ID_APP_EXIT);
	lstBasicCommands.AddTail(ID_EDIT_CUT);
	lstBasicCommands.AddTail(ID_EDIT_PASTE);
	lstBasicCommands.AddTail(ID_EDIT_UNDO);
	lstBasicCommands.AddTail(ID_APP_ABOUT);
	lstBasicCommands.AddTail(ID_VIEW_STATUS_BAR);
	lstBasicCommands.AddTail(ID_VIEW_TOOLBAR);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2003);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_VS_2005);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2007_BLUE);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2007_SILVER);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2007_BLACK);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2007_AQUA);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_WINDOWS_7);
	lstBasicCommands.AddTail(ID_SORTING_SORTALPHABETIC);
	lstBasicCommands.AddTail(ID_SORTING_SORTBYTYPE);
	lstBasicCommands.AddTail(ID_SORTING_SORTBYACCESS);
	lstBasicCommands.AddTail(ID_SORTING_GROUPBYTYPE);

	CMFCToolBar::SetBasicCommands(lstBasicCommands);
#endif

	// Switch the order of document name and application name on the window title bar. This
	// improves the usability of the taskbar because the document name is visible with the thumbnail.
	ModifyStyle(0, FWS_PREFIXTITLE);

	SetTimer(POSSUM_PLAYHEAD_TIMER, 33, nullptr);
	return 0;
}

BOOL CPossumMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CMDIFrameWndEx::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return TRUE;
}

BOOL CPossumMainFrame::CreateDockingWindows()
{
	BOOL bNameValid;

	// Create properties window
	CString strPropertiesWnd;
	bNameValid = strPropertiesWnd.LoadString(IDS_PROPERTIES_WND);
	ASSERT(bNameValid);
	if (!m_wndProperties.Create(strPropertiesWnd, this, CRect(0, 0, 200, 200), TRUE, ID_VIEW_PROPERTIESWND, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_RIGHT | CBRS_FLOAT_MULTI))
	{
		TRACE0("Failed to create Properties window\n");
		return FALSE; // failed to create
	}

	// Create properties window
	CString strProductWnd;
	bNameValid = strProductWnd.LoadString(IDS_PRODUCTS_WND);
	ASSERT(bNameValid);
	if (!m_wndProductBrowser.Create(strProductWnd, this, CRect(0, 0, 200, 200), TRUE, ID_VIEW_PRODUCTSWND, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_RIGHT | CBRS_FLOAT_MULTI))
	{
		TRACE0("Failed to create Product Browser window\n");
		return FALSE; // failed to create
	}

	// Create devices window
	CString strDeviceWnd;
	bNameValid = strDeviceWnd.LoadString(IDS_DEVICES_WND);
	ASSERT(bNameValid);
	if (!m_wndDevices.Create(strDeviceWnd, this, CRect(0, 0, 200, 200), TRUE, ID_VIEW_DEVICESWND, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_RIGHT | CBRS_FLOAT_MULTI))
	{
		TRACE0("Failed to create Device Browser window\n");
		return FALSE; // failed to create
	}

	// Create devices window
	CString strCommandListWnd;
	bNameValid = strCommandListWnd.LoadString(IDS_COMMANDLIST_WND);
	ASSERT(bNameValid);
	if (!m_wndCommandList.Create(strCommandListWnd, this, CRect(0, 0, 200, 200), TRUE, ID_VIEW_COMMANDLISTWND, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_RIGHT | CBRS_FLOAT_MULTI))
	{
		TRACE0("Failed to create command list window\n");
		return FALSE; // failed to create
	}

	// Create shows window
	CString strMediaWnd;
	bNameValid = strMediaWnd.LoadString(IDS_MEDIA_WND);
	ASSERT(bNameValid);
	if (!m_wndMedia.Create(strMediaWnd, this, CRect(0, 0, 100, 100), TRUE, ID_VIEW_MEDIAWND, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_BOTTOM | CBRS_FLOAT_MULTI))
	{
		TRACE0("Failed to create Media window\n");
		return FALSE; // failed to create
	}

	SetDockingWindowIcons(theApp.m_bHiColorIcons);
	return TRUE;
}

void CPossumMainFrame::SetDockingWindowIcons(BOOL bHiColorIcons)
{

	HICON hPropertiesBarIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_PROPERTIES_WND_HC), IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), 0);
	m_wndProperties.SetIcon(hPropertiesBarIcon, FALSE);

	HICON hProductsBarIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_PRODUCTS_WND_HC), IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), 0);
	m_wndProductBrowser.SetIcon(hProductsBarIcon, FALSE);

	HICON hDevicesBarIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_PRODUCTS_WND_HC), IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), 0);
	m_wndDevices.SetIcon(hDevicesBarIcon, FALSE);

	HICON hCommandListBarIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_PRODUCTS_WND_HC), IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), 0);
	m_wndCommandList.SetIcon(hCommandListBarIcon, FALSE);

	HICON hMediaBarIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_MEDIA_WND_HC), IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), 0);
	m_wndMedia.SetIcon(hMediaBarIcon, FALSE);

	UpdateMDITabbedBarsIcons();
}

BOOL CPossumMainFrame::CreateCaptionBar()
{
	if (!m_wndCaptionBar.Create(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, this, ID_VIEW_CAPTION_BAR, -1, TRUE))
	{
		TRACE0("Failed to create caption bar\n");
		return FALSE;
	}

	BOOL bNameValid;

	CString strTemp, strTemp2;
	bNameValid = strTemp.LoadString(IDS_CAPTION_BUTTON);
	ASSERT(bNameValid);
	m_wndCaptionBar.SetButton(strTemp, ID_TOOLS_OPTIONS, CMFCCaptionBar::ALIGN_LEFT, FALSE);
	bNameValid = strTemp.LoadString(IDS_CAPTION_BUTTON_TIP);
	ASSERT(bNameValid);
	m_wndCaptionBar.SetButtonToolTip(strTemp);

	bNameValid = strTemp.LoadString(IDS_CAPTION_TEXT);
	ASSERT(bNameValid);
	m_wndCaptionBar.SetText(strTemp, CMFCCaptionBar::ALIGN_LEFT);

	m_wndCaptionBar.SetBitmap(IDB_INFO, RGB(255, 255, 255), FALSE, CMFCCaptionBar::ALIGN_LEFT);
	bNameValid = strTemp.LoadString(IDS_CAPTION_IMAGE_TIP);
	ASSERT(bNameValid);
	bNameValid = strTemp2.LoadString(IDS_CAPTION_IMAGE_TEXT);
	ASSERT(bNameValid);
	m_wndCaptionBar.SetImageToolTip(strTemp, strTemp2);

	return TRUE;
}

// CPossumMainFrame diagnostics

#ifdef _DEBUG
void CPossumMainFrame::AssertValid() const
{
	CMDIFrameWndEx::AssertValid();
}

void CPossumMainFrame::Dump(CDumpContext& dc) const
{
	CMDIFrameWndEx::Dump(dc);
}
#endif //_DEBUG


// CPossumMainFrame message handlers

void CPossumMainFrame::OnWindowManager()
{
	ShowWindowsDialog();
}

void CPossumMainFrame::OnViewCustomize()
{
	CMFCToolBarsCustomizeDialog* pDlgCust = new CMFCToolBarsCustomizeDialog(this, TRUE /* scan menus */);
	pDlgCust->EnableUserDefinedToolbars();
	pDlgCust->Create();
}

LRESULT CPossumMainFrame::OnToolbarCreateNew(WPARAM wp, LPARAM lp)
{
	LRESULT lres = CMDIFrameWndEx::OnToolbarCreateNew(wp,lp);
	if (lres == 0)
	{
		return 0;
	}

	CMFCToolBar* pUserToolbar = (CMFCToolBar*)lres;
	ASSERT_VALID(pUserToolbar);

	BOOL bNameValid;
	CString strCustomize;
	bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	ASSERT(bNameValid);

	pUserToolbar->EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);
	return lres;
}

LRESULT CPossumMainFrame::OnToolbarReset(WPARAM wp, LPARAM lp)
{
	UINT uiToolBarId = (UINT)wp;

	switch (uiToolBarId)
	{
		case IDR_PLAYBACK_256:
			CRewindToolBarButton rewind_button(ID_REWIND, 0, nullptr, TRUE);
			m_wndPlaybackToolBar.ReplaceButton(ID_REWIND, rewind_button);

			CPlayPauseToolBarButton playpause_button(ID_PLAYPAUSE, 1, nullptr, TRUE);
			m_wndPlaybackToolBar.ReplaceButton(ID_PLAYPAUSE, playpause_button);

			CStopToolBarButton stop_button(ID_STOP, 2, nullptr, TRUE);
			m_wndPlaybackToolBar.ReplaceButton(ID_STOP, stop_button);

			CLoopToolBarButton loop_button(ID_LOOPING, 3, nullptr, TRUE);
			m_wndPlaybackToolBar.ReplaceButton(ID_LOOPING, loop_button);

			CTimeToolBarButton time_display(ID_TIME, 4, nullptr, TRUE);
			m_wndPlaybackToolBar.ReplaceButton(ID_TIME, time_display);
			break;
	}

	return 0;
}

void CPossumMainFrame::OnApplicationLook(UINT id)
{
	CWaitCursor wait;

	theApp.m_nAppLook = id;

	switch (theApp.m_nAppLook)
	{
		case ID_VIEW_APPLOOK_WIN_2000:
			CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManager));
			break;

		case ID_VIEW_APPLOOK_OFF_XP:
			CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOfficeXP));
			break;

		case ID_VIEW_APPLOOK_WIN_XP:
			CMFCVisualManagerWindows::m_b3DTabsXPTheme = TRUE;
			CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
			break;

		case ID_VIEW_APPLOOK_OFF_2003:
			CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2003));
			CDockingManager::SetDockingMode(DT_SMART);
			break;

		case ID_VIEW_APPLOOK_VS_2005:
			CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerVS2005));
			CDockingManager::SetDockingMode(DT_SMART);
			break;

		case ID_VIEW_APPLOOK_VS_2008:
			CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerVS2008));
			CDockingManager::SetDockingMode(DT_SMART);
			break;

		case ID_VIEW_APPLOOK_WINDOWS_7:
			CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows7));
			CDockingManager::SetDockingMode(DT_SMART);
			break;

		default:
			switch (theApp.m_nAppLook)
			{
				case ID_VIEW_APPLOOK_OFF_2007_BLUE:
					CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_LunaBlue);
					break;

				case ID_VIEW_APPLOOK_OFF_2007_BLACK:
					CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_ObsidianBlack);
					break;

				case ID_VIEW_APPLOOK_OFF_2007_SILVER:
					CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Silver);
					break;

				case ID_VIEW_APPLOOK_OFF_2007_AQUA:
					CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Aqua);
					break;
			}

			CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2007));
			CDockingManager::SetDockingMode(DT_SMART);
	}

	RedrawWindow(NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME | RDW_ERASE);

	theApp.WriteInt(_T("ApplicationLook"), theApp.m_nAppLook);
}

void CPossumMainFrame::OnUpdateApplicationLook(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(theApp.m_nAppLook == pCmdUI->m_nID);
}

void CPossumMainFrame::OnViewCaptionBar()
{
	m_wndCaptionBar.ShowWindow(m_wndCaptionBar.IsVisible() ? SW_HIDE : SW_SHOW);
	RecalcLayout(FALSE);
}

void CPossumMainFrame::OnUpdateViewCaptionBar(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_wndCaptionBar.IsVisible());
}

void CPossumMainFrame::OnOptions()
{
}

void CPossumMainFrame::OnViewPropertiesWindow()
{
	// Show or activate the pane, depending on current state.  The
	// pane can only be closed via the [x] button on the pane frame.
	m_wndProperties.ShowPane(TRUE, FALSE, TRUE);
	m_wndProperties.SetFocus();
}

void CPossumMainFrame::OnUpdateViewPropertiesWindow(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(TRUE);
}

void CPossumMainFrame::OnViewMediaWindow()
{
	// Show or activate the pane, depending on current state.  The
	// pane can only be closed via the [x] button on the pane frame.
	m_wndMedia.ShowPane(TRUE, FALSE, TRUE);
	m_wndMedia.SetFocus();
}

void CPossumMainFrame::OnUpdateViewMediaWindow(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(TRUE);
}

void CPossumMainFrame::OnViewProductsWindow()
{
	// Show or activate the pane, depending on current state.  The
	// pane can only be closed via the [x] button on the pane frame.
	m_wndProductBrowser.ShowPane(TRUE, FALSE, TRUE);
	m_wndProductBrowser.SetFocus();
}

void CPossumMainFrame::OnUpdateViewProductsWindow(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(TRUE);
}


void CPossumMainFrame::OnViewDevicesWindow()
{
	// Show or activate the pane, depending on current state.  The
	// pane can only be closed via the [x] button on the pane frame.
	m_wndDevices.ShowPane(TRUE, FALSE, TRUE);
	m_wndDevices.SetFocus();
}

void CPossumMainFrame::OnUpdateViewDevicesWindow(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(TRUE);
}


void CPossumMainFrame::OnViewCommandListWindow()
{
	// Show or activate the pane, depending on current state.  The
	// pane can only be closed via the [x] button on the pane frame.
	m_wndCommandList.ShowPane(TRUE, FALSE, TRUE);
	m_wndCommandList.SetFocus();
}

void CPossumMainFrame::OnUpdateViewCommandListWindow(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(TRUE);
}


BOOL CPossumMainFrame::LoadFrame(UINT nIDResource, DWORD dwDefaultStyle, CWnd* pParentWnd, CCreateContext* pContext)
{
	// base class does the real work

	if (!CMDIFrameWndEx::LoadFrame(nIDResource, dwDefaultStyle, pParentWnd, pContext))
	{
		return FALSE;
	}

	// enable customization button for all user toolbars
	BOOL bNameValid;
	CString strCustomize;
	bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	ASSERT(bNameValid);

	for (int i = 0; i < iMaxUserToolbars; i ++)
	{
		CMFCToolBar *pUserToolbar = GetUserToolBarByIndex(i);
		if (pUserToolbar != NULL)
		{
			pUserToolbar->EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);
		}
	}

	return TRUE;
}


void CPossumMainFrame::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	CMDIFrameWndEx::OnSettingChange(uFlags, lpszSection);
}


void CPossumMainFrame::OnFileNew() { theApp.OnFileNew(); }
void CPossumMainFrame::OnFileOpen() { theApp.OnFileOpen(); }
void CPossumMainFrame::OnFileSave() { theApp.OnFileSave(); }
void CPossumMainFrame::OnFileSaveAs() { theApp.OnFileSaveAs(); }
void CPossumMainFrame::OnFileClose() { theApp.OnFileClose(); }

void CPossumMainFrame::SetActiveProperties(props::IPropertySet *props, bool readonly, const TCHAR *title, nucleus::ICommandInfo *pci, nucleus::IDevice *pd)
{
	if (m_wndProperties.GetSafeHwnd())
		m_wndProperties.SetActiveProperties(props, readonly, title, pci, pd);
}

void CPossumMainFrame::RedrawActiveTimeline()
{
    if (auto *view = GetActiveDocView()) view->RedrawActiveTimeline();
    if (m_wndPlaybackToolBar.GetSafeHwnd())
        m_wndPlaybackToolBar.RedrawWindow(NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE);
}

void CPossumMainFrame::SetActiveTimelineExtents(double start_time, double end_time)
{
    if (auto *view = GetActiveDocView())
        view->SetActiveTimelineExtents(start_time, end_time);
}

void CPossumMainFrame::RefreshCommandList(bool deep)
{
	if (m_wndCommandList.GetSafeHwnd())
	{
		m_wndCommandList.Update(deep);
	}
}

CPossumDocView *CPossumMainFrame::GetActiveDocView()
{
    CMDIChildWnd *child = MDIGetActive();
    return child ? DYNAMIC_DOWNCAST(CPossumDocView, child->GetActiveView()) : nullptr;
}

CPossumDocView *CPossumMainFrame::FindDocViewByTrackSet(const nucleus::ITrackSet *tracks)
{
    for (POSITION pos = theApp.GetFirstDocTemplatePosition(); pos; )
    {
        CDocTemplate *templ = theApp.GetNextDocTemplate(pos);
        for (POSITION docpos = templ->GetFirstDocPosition(); docpos; )
        {
            auto *doc = DYNAMIC_DOWNCAST(CPossumDocument, templ->GetNextDoc(docpos));
            if (doc && doc->GetTrackSet() == tracks)
                return doc->GetView();
        }
    }
    return nullptr;
}

void CPossumMainFrame::OnFinalRelease()
{
	CMDIFrameWndEx::OnFinalRelease();
}


void CPossumMainFrame::OnTrackheightTiny()
{
	theApp.SetTrackHeight(28);
}


void CPossumMainFrame::OnTrackheightSmall()
{
	theApp.SetTrackHeight(32);
}


void CPossumMainFrame::OnTrackheightMedium()
{
	theApp.SetTrackHeight(40);
}


void CPossumMainFrame::OnTrackheightLarge()
{
	theApp.SetTrackHeight(52);
}


void CPossumMainFrame::OnTrackheightHuge()
{
	theApp.SetTrackHeight(64);
}


LRESULT CPossumMainFrame::OnMDIActiveChanged(WPARAM, LPARAM)
{
    auto *view = GetActiveDocView();
    theApp.SetActiveDocument(view ? view->GetDocument() : nullptr);
    RedrawActiveTimeline();
    return 0;
}

void CPossumMainFrame::OnTimer(UINT_PTR id)
{
    if (id == POSSUM_PLAYHEAD_TIMER)
    {
        if (theApp.m_Player)
        {
            auto *tracks = theApp.m_Player->GetActiveTrackSet();
            auto *view = tracks ? FindDocViewByTrackSet(tracks) : nullptr;
            if (view)
                view->SetPlayingTime(theApp.m_Player->GetPlayPosition());
        }
        if (m_wndPlaybackToolBar.GetSafeHwnd())
            m_wndPlaybackToolBar.Invalidate(FALSE);
        return;
    }
    CMDIFrameWndEx::OnTimer(id);
}

void CPossumMainFrame::OnClose()
{
	KillTimer(POSSUM_PLAYHEAD_TIMER);
	theApp.m_Exiting = true;

	CMDIFrameWndEx::OnClose();
}

void CPossumMainFrame::OnLayoutCommand(UINT id)
{
    auto *doc = theApp.m_DocMan.GetActiveDocument();
    if (doc)
        PossumLayout::Apply(doc, id);
}

void CPossumMainFrame::OnUpdateLayoutCommand(CCmdUI *ui)
{
    auto *doc = theApp.m_DocMan.GetActiveDocument();
    ui->Enable(doc && PossumLayout::CanApply(doc));
}

void CPossumMainFrame::OnLayoutSnap()
{
    theApp.m_LayoutSnapEnabled = !theApp.m_LayoutSnapEnabled;
    theApp.WriteProfileInt(_T("Layout"), _T("Snap"), theApp.m_LayoutSnapEnabled ? 1 : 0);
}

void CPossumMainFrame::OnUpdateLayoutSnap(CCmdUI *ui)
{
    ui->Enable(TRUE);
    ui->SetCheck(theApp.m_LayoutSnapEnabled ? 1 : 0);
}

void CPossumMainFrame::OnLayoutAudition()
{
    theApp.m_AuditionEnabled ^= true;
    theApp.WriteProfileInt(_T("Layout"), _T("Audition"), theApp.m_AuditionEnabled ? 1 : 0);
}

void CPossumMainFrame::OnUpdateLayoutAudition(CCmdUI *ui)
{
    ui->Enable();
    ui->SetCheck(theApp.m_AuditionEnabled ? 1 : 0);
}

bool CPossumMainFrame::IsAuditionEnabled() const
{
	return theApp.m_AuditionEnabled;
}
