
// Possum.h : main header file for the Possum application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols

#include "PicManager.h"
#include "PossumDocManager.h"
#include "PossumEffects.h"
#include <PowerProps\Include\PowerProps.h>
#include <Pool\Include\Pool.h>
#include <NuPlayer.h>
#include "CSplash.h"

// CPossum:
// See Possum.cpp for the implementation of this class
//

class CPossum : public CWinAppEx
{
public:
	CPossum();
	virtual ~CPossum();

// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// Implementation
	nucleus::IConfiguration *m_pCfg;
	nucleus::INucleus *m_pSys;
	nucleus::IPluginManager *m_pPlugMan;
	nucleus::IDeviceManager *m_pDevMan;
	CPicManager m_PicMan;
	CPossumDocManager m_DocMan;
	PossumEffects::Defaults m_EffectDefaults;
	CString m_EffectDefaultsFile;
	UINT m_DragDeviceFormat;
	CSplash m_Splash;

	UINT m_TrackHeight;
	bool m_LayoutSnapEnabled = true;
	bool m_LoopPlayback = false;
	bool m_AuditionEnabled = false;

	struct SDragData
	{
		enum {NONE = 0, CMDINFO, CMDSET} m_eMode;
		union
		{
			const nucleus::ICommandInfo *m_pCmdInfo;
			nucleus::ICommandSet *m_pCmdSet;
		};
	};

	typedef std::set<LPVOID> TPtrSet;
	typedef std::map<tstring, TPtrSet> TFilterMap;
	TFilterMap m_FilterMap;
	TPtrSet m_ActiveCommandFilter;
	void BuildFilterMap();
	void SaveFilterMap();
	void ClearFilterMap();
	void AddFilterToMap(const TCHAR *filtername, TPtrSet *pps);

	SDragData m_DragData;

	ULONG_PTR m_gdiplusToken;


	nucleus::IPlayer *m_Player;

	// Server object for document creation
	UINT m_nAppLook;
	BOOL m_bHiColorIcons;
	bool m_Exiting;
	HANDLE m_hUpdateMutex;

	nucleus::IPlayer::State m_NotifiedState;

	void UpdateDevices();

	virtual void PreLoadState();
	virtual void LoadCustomState();
	virtual void SaveCustomState();


	void SetActiveProperties(props::IPropertySet *props, bool readonly = false, const TCHAR *title = nullptr, nucleus::ICommandInfo* pci = nullptr, nucleus::IDevice* pd = nullptr);
	void RedrawActiveTimeline();
	void SetActiveTimelineExtents(double start_time, double end_time);
	void RefreshCommandList(bool deep);
	void SetActiveDocument(CPossumDocument *doc);

	void SetTrackHeight(UINT h);
	UINT GetTrackHeight();

	static void __cdecl PlayerNotificationHandler(const nucleus::ITrackSet *ptracks, nucleus::IPlayer::State state, double playtime, void *userdata);
	static nucleus::ITrackSet * __cdecl TracksetLoaderHandler(const TCHAR *path, void *userdata);


	afx_msg void OnAppAbout();
	afx_msg void OnFileNew();
	afx_msg void OnFileOpen();
	afx_msg void OnFileSave();
	afx_msg void OnFileSaveAs();
	afx_msg void OnFileClose();
	afx_msg void OnFileExport();
	afx_msg void OnEditUndo();
	afx_msg void OnEditRedo();
	afx_msg void OnUpdateEditUndo(CCmdUI *ui);
	afx_msg void OnUpdateEditRedo(CCmdUI *ui);
	afx_msg void OnOpenRecentFile(UINT id);

	afx_msg void OnPlayPause();
	afx_msg void OnRewind();
	afx_msg void OnStop();
	afx_msg void OnLooping();
	afx_msg void OnUpdatePlayPause(CCmdUI *pCmdUI);
	afx_msg void OnUpdateRewind(CCmdUI *pCmdUI);
	afx_msg void OnUpdateStop(CCmdUI *pCmdUI);
	afx_msg void OnUpdateLooping(CCmdUI *pCmdUI);

	DECLARE_MESSAGE_MAP()
};

extern CPossum theApp;
