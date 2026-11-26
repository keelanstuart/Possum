
// Possum.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "afxwinappex.h"
#include "afxdialogex.h"
#include "Possum.h"
#include "PossumFrame.h"


#include "PossumDocument.h"
#include "PossumDocView.h"
#include <NuImporter.h>
#include <NuTrack.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CPossum

BEGIN_MESSAGE_MAP(CPossum, CWinAppEx)
	ON_COMMAND(ID_APP_ABOUT, &CPossum::OnAppAbout)
	// Standard file based document commands
	ON_COMMAND(ID_NUCLEUS_FILE_NEW, &CPossum::OnFileNew)
	ON_COMMAND(ID_NUCLEUS_FILE_OPEN, &CPossum::OnFileOpen)
	ON_COMMAND(ID_NUCLEUS_FILE_SAVE, &CPossum::OnFileSave)
	ON_COMMAND(ID_NUCLEUS_FILE_SAVE_AS, &CPossum::OnFileSaveAs)
	ON_COMMAND(ID_NUCLEUS_FILE_CLOSE, &CPossum::OnFileClose)
	ON_COMMAND(ID_NUCLEUS_FILE_EXPORT, &CPossum::OnFileExport)
	ON_COMMAND(ID_EDIT_UNDO, &CPossum::OnEditUndo)
	ON_COMMAND(ID_EDIT_REDO, &CPossum::OnEditRedo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, &CPossum::OnUpdateEditUndo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, &CPossum::OnUpdateEditRedo)
	ON_COMMAND_RANGE(ID_FILE_MRU_FILE1, ID_FILE_MRU_FILE16, &CPossum::OnOpenRecentFile)
	ON_COMMAND(ID_PLAYPAUSE, &CPossum::OnPlayPause)
	ON_COMMAND(ID_REWIND, &CPossum::OnRewind)
	ON_COMMAND(ID_STOP, &CPossum::OnStop)
	ON_COMMAND(ID_LOOPING, &CPossum::OnLooping)
	ON_UPDATE_COMMAND_UI(ID_PLAYPAUSE, &CPossum::OnUpdatePlayPause)
	ON_UPDATE_COMMAND_UI(ID_REWIND, &CPossum::OnUpdateRewind)
	ON_UPDATE_COMMAND_UI(ID_STOP, &CPossum::OnUpdateStop)
	ON_UPDATE_COMMAND_UI(ID_LOOPING, &CPossum::OnUpdateLooping)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, &CWinAppEx::OnFilePrintSetup)
END_MESSAGE_MAP()


pool::IThreadPool::TaskReturn _UpdateDevices(CPossum *pp)
{
	pp->UpdateDevices();

	return pool::IThreadPool::TaskReturn::OK;
}


// CPossum construction

CPossum::CPossum()
{
	m_pCfg = nullptr;
	m_pSys = nullptr;
	m_pPlugMan = nullptr;
	m_pDevMan = nullptr;
	m_Player = nullptr;

	m_Exiting = false;

	m_bHiColorIcons = TRUE;

	m_TrackHeight = 40;

	// support Restart Manager
	//m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_ALL_ASPECTS;

	SetAppID(_T("PossumAudio"));
}

CPossum::~CPossum()
{
}

// The one and only CPossum object

CPossum theApp;
// This identifier was generated to be statistically unique for your app
// You may change it if you prefer to choose a specific identifier


HANDLE g_AppMutex = NULL;
#define APPMUTEXNAME	_T("PossumApplication")

// CPossum initialization

BOOL CPossum::InitInstance()
{
	// only allow one instance of Possum to run at a time
	g_AppMutex = OpenMutex(SYNCHRONIZE, true, APPMUTEXNAME);
	if (g_AppMutex)
		TerminateProcess(m_hInstance, -1);

	g_AppMutex = CreateMutex(nullptr, true, APPMUTEXNAME);

	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	m_hUpdateMutex = CreateMutex(nullptr, FALSE, nullptr);

	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinAppEx::InitInstance();

	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}

	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::Status st = Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);
	if (st != Gdiplus::Status::Ok)
		return FALSE;

	if (auto logo = m_PicMan.GetResPic(IDR_POSSUM_LOGO))
		m_Splash.CreateSplash(logo, 2500);

	AfxEnableControlContainer();

	EnableTaskbarInteraction();

	// AfxInitRichEdit2() is required to use RichEdit control	
	// AfxInitRichEdit2();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("PossumAudio"));

	// Load standard INI file options (including MRU)
	LoadStdProfileSettings(10);
    m_LayoutSnapEnabled = GetProfileInt(_T("Layout"), _T("Snap"), 1) != 0;
	m_LoopPlayback = GetProfileInt(_T("Playback"), _T("Loop"), 0) != 0;
	m_AuditionEnabled = theApp.GetProfileInt(_T("Layout"), _T("Audition"), 0) != 0;

	InitContextMenuManager();

	InitKeyboardManager();

	InitTooltipManager();
	CMFCToolTipInfo ttParams;
	ttParams.m_bVislManagerTheme = TRUE;
	theApp.GetTooltipManager()->SetTooltipParams(AFX_TOOLTIP_TYPE_ALL,
		RUNTIME_CLASS(CMFCToolTipCtrl), &ttParams);

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views
	CMultiDocTemplate* pDocTemplate;
	pDocTemplate = new CMultiDocTemplate(IDR_PossumTYPE,
		RUNTIME_CLASS(CPossumDocument),
		RUNTIME_CLASS(CPossumMDIChildFrame),
		RUNTIME_CLASS(CPossumDocView));

	if (!pDocTemplate)
		return FALSE;

	AddDocTemplate(pDocTemplate);

	m_DragDeviceFormat = RegisterClipboardFormat(_T("REACTOR_DEVICEDRAG_EB31366A-CACD-4741-94D5-017356BA9A6B"));

	m_Splash.SetStatus(_T("Initializing Nucleus..."));
	m_pSys = nucleus::INucleus::Create();
	if (!m_pSys)
		return FALSE;

	m_Player = nucleus::IPlayer::Create(m_pSys);
	if (m_Player)
	{
		// Playback position is polled by the main frame's UI timer.
        // Do not touch MFC windows from Nucleus's pool notification threads.
        nucleus::IPlayer::NotifyFunction PlayerNotificationHandler = [](nucleus::IPlayer *pp, nucleus::IPlayer::State s, double t)
		{
			if (s == nucleus::IPlayer::State::PlaybackEnded)
			{
				if (theApp.m_LoopPlayback)
				{
					pp->SetPlayPosition(0.0);
					pp->SetPlayState(nucleus::IPlayer::Playing);
				}
			}

			theApp.m_NotifiedState = s;
		};

		m_Player->SetNotifyFunction(PlayerNotificationHandler);

		nucleus::IPlayer::TrackSetLoader TracksetLoaderHandler = [this](const TCHAR *path) -> nucleus::ITrackSet *
		{
			CPossumDocument *pd = this->m_DocMan.CreateDocument(path);
			if (pd)
				return pd->GetTrackSet();

			return nullptr;
		};

		m_Player->SetTrackSetLoaderFunction(TracksetLoaderHandler);
	}

	m_Splash.SetStatus(_T("Discovering plugins..."));
	m_pPlugMan = m_pSys->PluginManager();
	if (!m_pPlugMan)
		return FALSE;

	m_pPlugMan->DiscoverPlugins();

	m_pDevMan = m_pSys->DeviceManager();
	if (!m_pDevMan)
		return FALSE;

	m_pSys->ThreadPool()->RunTask([this](size_t task_nummber) -> pool::IThreadPool::TaskReturn
	{
		this->UpdateDevices();

		return pool::IThreadPool::TaskReturn::OK;
	});

	TCHAR cfgpath[MAX_PATH] = { 0 };
	TCHAR *rootpath;
	HRESULT hr = SHGetKnownFolderPath(FOLDERID_ProgramData, 0, NULL, &rootpath);
	if (SUCCEEDED(hr))
	{
		_tcscat_s(cfgpath, MAX_PATH, rootpath);
		PathAddBackslash(cfgpath);
		CoTaskMemFree(rootpath);

		_tcscat_s(cfgpath, MAX_PATH, _T("PossumAudio"));
		if (!PathIsDirectory(cfgpath) && !PathIsDirectory(cfgpath))
			if (!CreateDirectory(cfgpath, nullptr))
				return false;

		PathAddBackslash(cfgpath);
		_tcscat_s(cfgpath, MAX_PATH, _T("Possum"));
		if (!PathIsDirectory(cfgpath) && !PathIsDirectory(cfgpath))
			if (!CreateDirectory(cfgpath, nullptr))
				return false;

		if (!PathIsDirectory(cfgpath) && !PathIsDirectory(cfgpath))
			return false;

		PathAddBackslash(cfgpath);
	}

	m_Splash.SetStatus(_T("Loading Configuration..."));
	_tcscat_s(cfgpath, MAX_PATH, _T("Possum.cfg"));

	// Independent, plain-XML defaults for new documents' envelope presentation.
    TCHAR appdata[MAX_PATH] = {};
    if (SHGetFolderPath(nullptr, CSIDL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, appdata) == S_OK)
    {
        CString directory(appdata);
        directory += _T("\\PossumAudio");
        CreateDirectory(directory, nullptr);
        m_EffectDefaultsFile = directory + _T("\\effects.xml");
        m_EffectDefaults.Load(m_EffectDefaultsFile);
    }

	m_pCfg = nucleus::IConfiguration::Create(cfgpath);
	if (!m_pCfg)
		return FALSE;

	BuildFilterMap();

	m_TrackHeight = (UINT)m_pCfg->GetInt(_T("view.tracks.height"), 40);


	m_Splash.SetStatus(_T("Building the interface..."));

	// create main MDI Frame window
	CPossumMainFrame* pMainFrame = new CPossumMainFrame;
	if (!pMainFrame || !pMainFrame->LoadFrame(IDR_MAINFRAME))
	{
		delete pMainFrame;
		return FALSE;
	}
	m_pMainWnd = pMainFrame;

	// call DragAcceptFiles only if there's a suffix
	//  In an MDI app, this should occur immediately after setting m_pMainWnd
	// Enable drag/drop open
	m_pMainWnd->DragAcceptFiles();

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

    EnableShellOpen();
    RegisterShellFileTypes(TRUE);

	if (cmdInfo.m_nShellCommand == CCommandLineInfo::FileNew)
		cmdInfo.m_nShellCommand = CCommandLineInfo::FileNothing;

	// Dispatch commands specified on the command line.  Will return FALSE if
	// app was launched with /RegServer, /Register, /Unregserver or /Unregister.
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_ObsidianBlack);

	// The main window has been initialized, so show and update it
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();

	m_Splash.SetStatus(_T("Reticulating Splines..."));
	m_Splash.Finish();

	return TRUE;
}

int CPossum::ExitInstance()
{
	m_Exiting = true;

	if (m_Player)
	{
		m_Player->SetPlayState(nucleus::IPlayer::Stopped);
		m_Player->Release();
		m_Player = nullptr;
	}

	if (m_pPlugMan)
	{
		m_pPlugMan->UnloadAllPlugins();
		m_pPlugMan = nullptr;
	}

	if (m_pSys)
	{
		m_pSys->ThreadPool()->PurgeAllPendingTasks();
		m_pSys->ThreadPool()->WaitForAllTasks(INFINITE);

		m_DocMan.CloseAllDocuments();
		m_pSys->Release();
		m_pSys = nullptr;
	}

	if (!m_EffectDefaultsFile.IsEmpty())
		m_EffectDefaults.Save(m_EffectDefaultsFile);

	if (m_hUpdateMutex)
	{
		WaitForSingleObject(m_hUpdateMutex, INFINITE);
		CloseHandle(m_hUpdateMutex);
	}

	AfxOleTerm(FALSE);

	m_PicMan.Cleanup();
	m_Splash.CloseNow();

	Gdiplus::GdiplusShutdown(m_gdiplusToken);

	if (m_pCfg)
	{
		m_pCfg->Release();
		m_pCfg = nullptr;
	}

	CloseHandle(g_AppMutex);

	int ret = CWinAppEx::ExitInstance();

	return ret;
}

// CPossum message handlers


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
public:

};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
	EnableActiveAccessibility();
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
	ON_WM_SIZE()
END_MESSAGE_MAP()

// App command to run the dialog
void CPossum::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

// CPossum customization load/save methods

void CPossum::PreLoadState()
{
	BOOL bNameValid;
	CString strName;
	bNameValid = strName.LoadString(IDS_EDIT_MENU);
	ASSERT(bNameValid);
	GetContextMenuManager()->AddMenu(strName, IDR_POPUP_EDIT);
	bNameValid = strName.LoadString(IDS_EXPLORER);
	ASSERT(bNameValid);
	GetContextMenuManager()->AddMenu(strName, IDR_POPUP_EXPLORER);
}

void CPossum::LoadCustomState()
{
}

void CPossum::SaveCustomState()
{
}

void CPossum::SetActiveProperties(props::IPropertySet *props, bool readonly, const TCHAR *title, nucleus::ICommandInfo* pci, nucleus::IDevice* pd)
{
	if (m_pMainWnd && m_pMainWnd->GetSafeHwnd())
		((CPossumMainFrame *)m_pMainWnd)->SetActiveProperties(props, readonly, title, pci, pd);
}

void CPossum::RedrawActiveTimeline()
{
	if (m_pMainWnd && m_pMainWnd->GetSafeHwnd())
		((CPossumMainFrame *)m_pMainWnd)->RedrawActiveTimeline();
}

void CPossum::SetActiveTimelineExtents(double start_time, double end_time)
{
	if (m_pMainWnd && m_pMainWnd->GetSafeHwnd())
		((CPossumMainFrame *)m_pMainWnd)->SetActiveTimelineExtents(start_time, end_time);
}

void CPossum::RefreshCommandList(bool deep)
{
	if (m_pMainWnd && m_pMainWnd->GetSafeHwnd())
		((CPossumMainFrame *)m_pMainWnd)->RefreshCommandList(deep);
}

void CPossum::SetActiveDocument(CPossumDocument *pdoc)
{
	m_DocMan.MakeActiveDocument(pdoc);

	RefreshCommandList(true);
}

// CPossum message handlers

void CPossum::OnFileNew()
{
    m_DocMan.CreateDocument();
}

void CPossum::OnFileOpen()
{
    tstring filter = _T("All Supported File Types|*.possum");
    for (size_t i = 0; i < nucleus::IImporter::GetNumImporters(); i++)
    {
        const nucleus::IImporter *importer = nucleus::IImporter::GetImporter(i);
        for (size_t e = 0; e < importer->GetNumExtensions(); e++)
        {
            filter += _T(";*.");
            filter += importer->GetExtension(e);
        }
    }
    filter += _T("|Possum Show Files (*.possum)|*.possum");
    for (size_t i = 0; i < nucleus::IImporter::GetNumImporters(); i++)
    {
        const nucleus::IImporter *importer = nucleus::IImporter::GetImporter(i);
        if (!importer->GetNumExtensions()) continue;
        filter += _T("|");
        filter += importer->GetDescription();
        filter += _T("|");
        for (size_t e = 0; e < importer->GetNumExtensions(); e++)
        {
            if (e) filter += _T(";");
            filter += _T("*.");
            filter += importer->GetExtension(e);
        }
    }
    filter += _T("|All Files (*.*)|*.*||");
    CFileDialog dialog(TRUE, nullptr, nullptr, OFN_FILEMUSTEXIST, filter.c_str());
    if (dialog.DoModal() == IDOK)
        m_DocMan.CreateDocument(dialog.GetPathName());
}


void CPossum::OnFileSave()
{
    if (auto *doc = m_DocMan.GetActiveDocument())
		doc->Save();
}


void CPossum::OnFileSaveAs()
{
    if (auto *doc = m_DocMan.GetActiveDocument())
		doc->SaveAs();
}


void CPossum::OnFileClose()
{
    if (auto *doc = m_DocMan.GetActiveDocument())
    {
        if (auto *view = doc->GetView())
            view->GetParentFrame()->SendMessage(WM_CLOSE);
    }
}

void CPossum::OnFileExport()
{
	bool success = false;

	if (auto *doc = m_DocMan.GetActiveDocument())
	{
		CString n = doc->GetTitle();
		PathRemoveExtension(n.GetBuffer());
		n.ReleaseBuffer();
		n += _T(".wav");

		CFileDialog fd(FALSE, _T("wav"), n, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST, _T("Wave Audio Files (*.wav)|*.wav||"), AfxGetMainWnd());
		if (fd.DoModal() == IDOK)
		{
			CWaitCursor wait;

			n = fd.GetPathName();

			TCHAR tmppath[MAX_PATH * 2] = {};
			TCHAR ntmp[MAX_PATH * 2] = {};

			GetTempPath(MAX_PATH, tmppath);
			GetTempFileName(tmppath, _T("PSM"), 0, ntmp);

			if (doc->GetTrackSet()->Save(ntmp) == nucleus::RetCode::OK)
			{
				if (nucleus::IDevice *pdev = theApp.m_pSys->DeviceManager()->GetDeviceByName(_T("Audio")))
				{
					if (const nucleus::ICommandInfo *pci = pdev->GetCommandByName(_T("Export")))
					{
						if (nucleus::ITrackSet *pts = nucleus::ITrackSet::Create(theApp.m_pSys))
						{
							nucleus::ITrack *pt = nullptr;
							if (pts->CreateTrack(&pt, _T("ExportDummy"), _T("")) == nucleus::RetCode::OK && pt)
							{
								nucleus::ICommand *pc = nullptr;
								if (pt->AddCommand(pci, 0.0, &pc) == nucleus::RetCode::OK && pc)
								{
									if (props::IPropertySet *ps = pc->GetParams())
									{
										if (props::IProperty *pp_in = ps->GetPropertyById('IN'))
											pp_in->SetString(ntmp);

										if (props::IProperty *pp_out = ps->GetPropertyById('OUT'))
											pp_out->SetString(n);

										if ((pdev->ExecuteCommand(pc, 0.0) == nucleus::RetCode::OK) && PathFileExists(n))
											success = true;
									}
								}
							}

							pts->Release();
						}
					}
				}
			}

			DeleteFile(ntmp);

			if (!success)
				MessageBox(NULL, _T("Something went wrong with your audio export!"), _T("Export Failed!"), MB_OK);
		}
	}
}


void CPossum::OnRewind()
{
	CPossumDocView *pdv = ((CPossumMainFrame *)m_pMainWnd)->GetActiveDocView();

	m_Player->SetPlayState(nucleus::IPlayer::State::Stopped);
	m_Player->SetPlayPosition(0.0);
	pdv->SetSelectedTime(0.0);
}

void CPossum::OnPlayPause()
{
	CPossumDocView *pdv = ((CPossumMainFrame *)m_pMainWnd)->GetActiveDocView();
	CPossumDocument *pd = pdv->GetDocument();

	switch (m_Player->GetPlayState())
	{
		case nucleus::IPlayer::State::Playing:
			m_Player->SetPlayState(nucleus::IPlayer::State::Paused);
			break;

		case nucleus::IPlayer::State::Paused:
		{
			CPossumDocView *playerdocview = ((CPossumMainFrame *)m_pMainWnd)->FindDocViewByTrackSet(m_Player->GetActiveTrackSet());
			if (playerdocview)
			{
				CPossumDocument *playerdoc = playerdocview->GetDocument();
				if (playerdoc)
				{
					theApp.m_DocMan.MakeActiveDocument(playerdoc);
					m_Player->SetPlayState(nucleus::IPlayer::State::Playing);
				}
			}
			break;
		}

		case nucleus::IPlayer::State::Stopped:
			m_Player->SetActiveTrackSet(pd->GetTrackSet());
			m_Player->SetPlayState(nucleus::IPlayer::State::Playing);
			if (pdv->GetSelectedTime() > 0.0)
				m_Player->SetPlayPosition(pdv->GetSelectedTime());
			break;
	}
}

void CPossum::OnStop()
{
	m_Player->SetPlayState(nucleus::IPlayer::State::Stopped);
}

void CPossum::OnLooping()
{
	theApp.m_LoopPlayback ^= true;
	theApp.WriteProfileInt(_T("Playback"), _T("Loop"), theApp.m_LoopPlayback ? 1 : 0);
}

void CPossum::OnUpdatePlayPause(CCmdUI *pCmdUI)
{
	pCmdUI->Enable();;
}

void CPossum::OnUpdateRewind(CCmdUI *pCmdUI)
{
	pCmdUI->Enable();
}

void CPossum::OnUpdateStop(CCmdUI *pCmdUI)
{
	pCmdUI->Enable();;
}

void CPossum::OnUpdateLooping(CCmdUI *pCmdUI)
{
	pCmdUI->Enable();
}

void CPossum::UpdateDevices()
{
	while (!m_Exiting)
	{
		if (WaitForSingleObject(m_hUpdateMutex, INFINITE) != WAIT_OBJECT_0)
			return;

		Sleep(1000);

		m_pSys->DeviceManager()->Update();

		ReleaseMutex(m_hUpdateMutex);
	}
}

void CPossum::SetTrackHeight(UINT h)
{
	m_TrackHeight = h;
	m_pCfg->SetInt(_T("view.tracks.height"), h);
	RedrawActiveTimeline();
}

UINT CPossum::GetTrackHeight()
{
	return m_TrackHeight;
}

void CPossum::BuildFilterMap()
{
	genio::IParserW *pp = (genio::IParserW *)genio::IParser::Create(genio::IParser::CHAR_MODE::CM_UNICODE);
	if (!pp)
		return;

	ClearFilterMap();

	for (size_t i = 0, maxi = m_pCfg->GetNumSubKeys(_T("filters")); i < maxi; i++)
	{
		TPtrSet ptrs;

		TCHAR s[64];
		_stprintf_s(s, _T("filters.filter#%zu"), i);

		tstring  ns = s;
		ns += _T(".name");

		tstring  fs = s;
		fs += _T(".items");

		tstring v, itemstr;
		
		v = m_pCfg->GetString(ns.c_str(), _T(""));
		itemstr = m_pCfg->GetString(fs.c_str(), _T(""));

		pp->SetSourceData(itemstr.c_str(), itemstr.length());
		while (pp->NextToken())
		{
			genio::IParser::TOKEN_TYPE itt = pp->GetCurrentTokenType();
			if (!((itt == genio::IParser::TOKEN_TYPE::TT_IDENT) || (itt == genio::IParser::TOKEN_TYPE::TT_STRING)))
				continue;

			tstring devstr = pp->GetCurrentTokenString();
			nucleus::IDevice *pdev = m_pSys->DeviceManager()->GetDeviceByName(devstr.c_str());
			const nucleus::ICommandInfo *pci = nullptr;

			if (pp->NextToken())
			{
				if (pp->IsToken(_T(":")))
				{
					if (pp->NextToken())
					{
						itt = pp->GetCurrentTokenType();
						if (((itt == genio::IParser::TOKEN_TYPE::TT_IDENT) || (itt == genio::IParser::TOKEN_TYPE::TT_STRING)) && pdev)
						{
							tstring cmdstr = pp->GetCurrentTokenString();
							pci = pdev->GetCommandByName(cmdstr.c_str());
						}
					}
				}
			}

			if (pdev || pci)
			{
				LPVOID pv = pci ? (LPVOID)pci : (LPVOID)pdev;
				if (ptrs.find(pv) == ptrs.end())
					ptrs.insert(pv);
			}
		}

		if (!ptrs.empty())
			AddFilterToMap(v.c_str(), &ptrs);
	}

	pp->Release();
}

void CPossum::SaveFilterMap()
{
	m_pCfg->RemoveKey(_T("filters"));

	TCHAR keyname[128];
	int i = 0;

	for (TFilterMap::const_iterator it = m_FilterMap.cbegin(), last_it = m_FilterMap.cend(); it != last_it; it++, i++)
	{
		_stprintf_s(keyname, _T("filters.filter#%d.name"), i);
		m_pCfg->SetString(keyname, it->first.c_str());

		_stprintf_s(keyname, _T("filters.filter#%d.items"), i);
		tstring s;
		for (TPtrSet::const_iterator pit = it->second.cbegin(), last_pit = it->second.cend(); pit != last_pit; )
		{
			nucleus::IDevice *pd = (nucleus::IDevice *)*pit;

			if (!m_pDevMan->IsDeviceRegistered(pd))
			{
				pd = ((nucleus::ICommandInfo *)*pit)->GetDevice();
			}

			s += _T("\"");
			s += pd->GetName();
			s += _T("\"");
			if ((LPVOID)pd != *pit)
			{
				s += _T(":");
				s += _T("\"");
				s += ((nucleus::ICommandInfo *)*pit)->GetName();
				s += _T("\"");
			}

			pit++;
			if (pit != last_pit)
				s += _T(";");
		}

		m_pCfg->SetString(keyname, s.c_str());
	}
}


void CPossum::ClearFilterMap()
{
	m_FilterMap.clear();
}

void CPossum::AddFilterToMap(const TCHAR *filtername, TPtrSet *pps)
{
	m_FilterMap.insert(TFilterMap::value_type(filtername, *pps));
}


void CPossum::OnOpenRecentFile(UINT nID)
{
	if (!theApp.m_pRecentFileList)
		return;

	const int index = nID - ID_FILE_MRU_FILE1;

	if (index >= m_pRecentFileList->GetSize())
		return;

	CString filename = (*m_pRecentFileList)[index];

	if (filename.IsEmpty())
		return;

	// Route through your existing custom load path.
    m_DocMan.CreateDocument(filename);
}


void CPossum::OnEditUndo()
{
    if (auto *doc = m_DocMan.GetActiveDocument()) doc->Undo();
}
void CPossum::OnEditRedo()
{
    if (auto *doc = m_DocMan.GetActiveDocument()) doc->Redo();
}
void CPossum::OnUpdateEditUndo(CCmdUI *ui)
{
    auto *doc = m_DocMan.GetActiveDocument();
    ui->Enable(doc && doc->UndoHistory().CanUndo() && (!m_Player || m_Player->GetPlayState() == nucleus::IPlayer::State::Stopped));
}
void CPossum::OnUpdateEditRedo(CCmdUI *ui)
{
    auto *doc = m_DocMan.GetActiveDocument();
    ui->Enable(doc && doc->UndoHistory().CanRedo() && (!m_Player || m_Player->GetPlayState() == nucleus::IPlayer::State::Stopped));
}
