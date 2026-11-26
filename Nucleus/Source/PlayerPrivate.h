#pragma once

#include "NuPlayer.h"
#include "NuCommand.h"
#include "NuCommandInfo.h"
#include "NuDevice.h"
#include "NuTrackSet.h"
#include "NuTrack.h"


class CPlayer : public nucleus::IPlayer
{
protected:
	nucleus::INucleus *m_pSys;

	struct STrackSetData
	{
		nucleus::ITrackSet *ptrackset;
		double time;
	};
	typedef std::deque<STrackSetData> TPlayStack;
	TPlayStack m_PlayStack;

	struct STimedCommandData
	{
		nucleus::ICommand *pcmd;
		double time;
	};
	typedef std::deque<STimedCommandData> TTimedCommandArray;
	TTimedCommandArray m_PlayedCommands;
	TTimedCommandArray m_UndoCommands;

	nucleus::IPlayer::State m_State;

	nucleus::IPlayer::NotifyFunction m_NotifyFunc;

	nucleus::IPlayer::TrackSetLoader m_LoaderFunc;

	CRITICAL_SECTION m_cs;

	HANDLE m_hQuit;
	HANDLE m_thPlay;
	DWORD m_PlayThreadId;

	LARGE_INTEGER m_Frequency;
	LARGE_INTEGER m_SystemStartPlayTime, m_LastSystemTime;

	bool m_CommandsAvailable;


public:

	CPlayer(nucleus::INucleus *psystem);

	virtual ~CPlayer();

	virtual void Release();

	/// Sets the callback for playback notifications
	virtual void SetNotifyFunction(nucleus::IPlayer::NotifyFunction func);

	/// Sets the callback for a trackset loading function, used when a trackset reference command is encountered
	virtual void SetTrackSetLoaderFunction(nucleus::IPlayer::TrackSetLoader func);

	virtual nucleus::RetCode SetActiveTrackSet(const nucleus::ITrackSet *ptracks);

	virtual nucleus::ITrackSet *GetActiveTrackSet();

	virtual bool IsTrackSetInPlayStack(const nucleus::ITrackSet *ptracks);

	virtual double GetPlayPosition();

	virtual void SetPlayPosition(double time);

	virtual nucleus::IPlayer::State GetPlayState() const;

	virtual nucleus::RetCode SetPlayState(nucleus::IPlayer::State state);

	// Synchronous resource and stateful-envelope preparation on first Play.
	nucleus::RetCode PreparePlaybackStart(nucleus::ITrackSet *tracks, double start);

	void NotifyCommandMoved(const nucleus::ICommand *pcmd, double newtime);

	static pool::IThreadPool::TaskReturn __cdecl ExecuteCommand(CPlayer *_this, nucleus::ICommand *pc, double time);

	static pool::IThreadPool::TaskReturn __cdecl CallNotifyFunc(CPlayer *_this, STrackSetData *pd,
		std::optional<nucleus::IPlayer::State> state_override = std::nullopt);

	DWORD PlayThreadProc();

	static DWORD WINAPI _PlayThreadProc(LPVOID param);
};
