// **************************************************************
// Nucleus Source File
//
// Copyright © 2019-2026, Keelan Sturt


#include "stdafx.h"
#include "PlayerPrivate.h"
#include <algorithm>
#include <cmath>
#include <map>


CPlayer::CPlayer(nucleus::INucleus *psystem)
{
	m_pSys = psystem;

	m_State = nucleus::IPlayer::State::Stopped;

	m_NotifyFunc = nullptr;

	m_LoaderFunc = nullptr;

	m_CommandsAvailable = true;

	InitializeCriticalSection(&m_cs);

	QueryPerformanceFrequency(&m_Frequency);

	QueryPerformanceCounter(&m_SystemStartPlayTime);
	m_LastSystemTime = m_SystemStartPlayTime;

	m_hQuit = CreateSemaphore(nullptr, 0, 1, nullptr);
	m_thPlay = CreateThread(NULL, 1 << 16, _PlayThreadProc, this, 0, &m_PlayThreadId);
}

CPlayer::~CPlayer()
{
	ReleaseSemaphore(m_hQuit, 1, nullptr);
	WaitForSingleObject(m_thPlay, INFINITE);

	DeleteCriticalSection(&m_cs);
}

void CPlayer::Release()
{
	delete (CPlayer *)this;
}

/// Sets the callback for playback notifications
void CPlayer::SetNotifyFunction(nucleus::IPlayer::NotifyFunction func)
{
	m_NotifyFunc = func;
}

/// Sets the callback for a trackset loading function, used when a trackset reference command is encountered
void CPlayer::SetTrackSetLoaderFunction(nucleus::IPlayer::TrackSetLoader func)
{
	m_LoaderFunc = func;
}

nucleus::RetCode CPlayer::SetActiveTrackSet(const nucleus::ITrackSet *ptracks)
{
	nucleus::RetCode ret = nucleus::RetCode::UNKNOWN_ERROR;

	EnterCriticalSection(&m_cs);

	m_State = nucleus::IPlayer::State::Stopped;

	m_PlayStack.clear();

	if (ptracks)
	{
		STrackSetData d;
		d.ptrackset = (nucleus::ITrackSet *)ptracks;
		d.time = 0.0;
		m_PlayStack.push_back(d);
		STrackSetData *pd = &(m_PlayStack.back());

		ret = nucleus::RetCode::OK;

		m_pSys->ThreadPool()->RunTask([this, pd](size_t task_number) -> pool::IThreadPool::TaskReturn
		{
			return CallNotifyFunc(this, pd);
		});
	}

	LeaveCriticalSection(&m_cs);

	return ret;
}

nucleus::ITrackSet *CPlayer::GetActiveTrackSet()
{
	nucleus::ITrackSet *ret = nullptr;

	if(!m_PlayStack.empty())
	{
		EnterCriticalSection(&m_cs);

		ret = (nucleus::ITrackSet *)m_PlayStack.back().ptrackset;

		LeaveCriticalSection(&m_cs);
	}

	return ret;
}

bool CPlayer::IsTrackSetInPlayStack(const nucleus::ITrackSet *ptracks)
{
	for (TPlayStack::const_iterator it = m_PlayStack.cbegin(), last_it = m_PlayStack.cend(); it != last_it; it++)
	{
		if ((const nucleus::ITrackSet *)(it->ptrackset) == ptracks)
			return true;
	}

	return false;
}

double CPlayer::GetPlayPosition()
{
	double ret = 0.0;

	if(!m_PlayStack.empty())
	{
		EnterCriticalSection(&m_cs);

		ret = m_PlayStack.back().time;

		LeaveCriticalSection(&m_cs);
	}

	return ret;
}

void CPlayer::SetPlayPosition(double time)
{
	if (!std::isfinite(time))
		return;

	m_CommandsAvailable = true;

	time = std::max<double>(0.0, time);
	if(!m_PlayStack.empty())
	{
		EnterCriticalSection(&m_cs);

		m_PlayStack.back().time = time;
		// Starting from a chosen time is a fresh playback pass. Keep the
		// existing live-seek bookkeeping for callers that seek while running.
		if (m_State == nucleus::IPlayer::State::Stopped)
			m_PlayedCommands.clear();
		else
			m_PlayedCommands.erase(std::remove_if(m_PlayedCommands.begin(), m_PlayedCommands.end(),
				[time](const STimedCommandData &d) { return d.time >= time; }), m_PlayedCommands.end());

		STrackSetData *pd = &(m_PlayStack.back());
		m_pSys->ThreadPool()->RunTask([this, pd](size_t task_number) -> pool::IThreadPool::TaskReturn
		{
			return CallNotifyFunc(this, pd);
		}, 1, false);

		while (!m_UndoCommands.empty() && (m_UndoCommands.back().time > time))
		{
			nucleus::ICommand *pc = m_UndoCommands.back().pcmd;
			m_pSys->ThreadPool()->RunTask([this, pc, time](size_t task_number) -> pool::IThreadPool::TaskReturn
			{
				return ExecuteCommand(this, pc, time);
			}, 1, false);
				
				
			m_UndoCommands.pop_back();
		}

		LeaveCriticalSection(&m_cs);
	}
}

// Synchronously prepare a fresh pass beginning at 'start'. This is a
// transport operation: only PREP commands and the last control point of
// each envelope are run. Notes/samples beginning before 'start' are not
// reconstructed; those beginning exactly at 'start' remain schedulable.
nucleus::RetCode CPlayer::PreparePlaybackStart(nucleus::ITrackSet *tracks, double start)
{
	if (!tracks)
		return nucleus::RetCode::UNKNOWN_ERROR;

	// First resolve resource dependencies across the entire track set.
	// PREP is the opt-in marker used by PreloadSample and other devices.
	for (size_t i = 0; i < tracks->GetNumTracks(); ++i)
	{
		auto *track = tracks->GetTrack(i);
		const auto *enabled = track->GetProperties()->GetPropertyById('ENAB');
		if (enabled && !enabled->AsBool())
			continue;

		for (size_t j = 0; j < track->GetNumCommands(); ++j)
		{
			double commandTime = 0.0;
			auto *cmd = track->GetCommand(j, &commandTime);

			if (commandTime > start)
				break; // Track commands are in time order.

			if (!cmd)
				continue;

			const auto *prep = cmd->GetParams()->GetPropertyById('PREP');
			if (!prep || !prep->AsBool())
				continue;

			auto result = cmd->GetCommandInfo()->GetDevice()->ExecuteCommand(cmd, commandTime);
			if (result != nucleus::RetCode::OK)
				return result;

			// A command scheduled at the start must not run a second time
			// when the normal scheduler processes its first interval.
			if (commandTime == start)
				m_PlayedCommands.push_back({cmd, commandTime});
		}
	}

	// Reconstruct the most recent segment for each envelope on each track.
	// The Audio plugin identifies envelope commands by CPNT + CVEC; both
	// properties are required, avoiding ordinary content and PREP commands.
	for (size_t i = 0; i < tracks->GetNumTracks(); ++i)
	{
		auto *track = tracks->GetTrack(i);
		const auto *enabled = track->GetProperties()->GetPropertyById('ENAB');

		if (enabled && !enabled->AsBool())
			continue;

		struct LastKnot
		{
			nucleus::ICommand *command = nullptr;
			double time = 0.0;
		};
		std::map<const nucleus::ICommandInfo *, LastKnot> lastByType;

		for (size_t j = 0; j < track->GetNumCommands(); ++j)
		{
			double commandTime = 0.0;
			auto *cmd = track->GetCommand(j, &commandTime);

			if (commandTime > start)
				break;

			if (!cmd)
				continue;

			const auto *params = cmd->GetParams();
			if (params->GetPropertyById('CPNT') && params->GetPropertyById('CVEC'))
				lastByType[cmd->GetCommandInfo()] = {cmd, commandTime};
		}

		for (const auto &entry : lastByType)
		{
			auto *cmd = entry.second.command;

			// The command owns the original knot timestamps, but executing it
			// at the start time anchors the device's render clock correctly.
			auto result = cmd->GetCommandInfo()->GetDevice()->ExecuteCommand(cmd, start);

			if (result != nucleus::RetCode::OK)
				return result;

			if (entry.second.time == start)
				m_PlayedCommands.push_back({cmd, entry.second.time});
		}
	}

	return nucleus::RetCode::OK;
}


nucleus::IPlayer::State CPlayer::GetPlayState() const
{
	return m_State;
}

nucleus::RetCode CPlayer::SetPlayState(nucleus::IPlayer::State state)
{
	if (state == nucleus::IPlayer::State::PlaybackEnded)
		return nucleus::RetCode::INVALID_PARAM;

	nucleus::RetCode ret = nucleus::RetCode::UNKNOWN_ERROR;

	EnterCriticalSection(&m_cs);

	if(!m_PlayStack.empty() && (state != m_State))
	{
		// A stopped -> playing transition always begins with a synchronous
		// preparation phase. Even for a nonzero position, preload resources
		// and restore the last control point for each envelope before the
		// real-time scheduler begins dispatching content commands.
		if (state == nucleus::IPlayer::State::Playing &&
			m_State == nucleus::IPlayer::State::Stopped)
		{
			// Prevent devices with their own render clocks from advancing
			// during decoding and envelope reconstruction.
			for (size_t c = 0; c < m_pSys->DeviceManager()->GetDeviceCount(); ++c)
				m_pSys->DeviceManager()->GetDevice(c)->SetPaused(true);

			ret = PreparePlaybackStart(m_PlayStack.back().ptrackset, m_PlayStack.back().time);
			if (ret != nucleus::RetCode::OK)
			{
				// Do not leave a partially initialized mixer on a failed start.
				for (size_t c = 0; c < m_pSys->DeviceManager()->GetDeviceCount(); ++c)
					m_pSys->DeviceManager()->GetDevice(c)->Reset();
				m_PlayedCommands.clear();
				LeaveCriticalSection(&m_cs);
				return ret;
			}
		}

		if (state == nucleus::IPlayer::State::Playing)
		{
			// No stalled/preparation interval should be charged to playback.
			QueryPerformanceCounter(&m_LastSystemTime);

			m_CommandsAvailable = false;
		}

		switch (state)
		{
			case nucleus::IPlayer::State::Stopped:
			{
				while(m_PlayStack.size() > 1)
				{
					m_PlayStack.pop_back();
				}

				m_PlayedCommands.clear();
				m_PlayStack.back().time = 0.0;

				for (size_t c = 0, maxc = m_pSys->DeviceManager()->GetDeviceCount(); c < maxc; c++)
				{
					nucleus::IDevice *pdev = m_pSys->DeviceManager()->GetDevice(c);
					assert(pdev);

					pdev->Reset();
				}

				break;
			}

			default:
			{
				for (size_t c = 0, maxc = m_pSys->DeviceManager()->GetDeviceCount(); c < maxc; c++)
				{
					nucleus::IDevice *pdev = m_pSys->DeviceManager()->GetDevice(c);
					assert(pdev);
					pdev->SetPaused(state == nucleus::IPlayer::State::Paused);
				}

				break;
			}
		}

		m_State = state;
		ret = nucleus::RetCode::OK;

		STrackSetData *pd = &(m_PlayStack.back());
		m_pSys->ThreadPool()->RunTask([this, pd](size_t task_number) -> pool::IThreadPool::TaskReturn
		{
			return CallNotifyFunc(this, pd);
		});
	}

	LeaveCriticalSection(&m_cs);

	return ret;
}

pool::IThreadPool::TaskReturn __cdecl CPlayer::ExecuteCommand(CPlayer *_this, nucleus::ICommand *pc, double time)
{
	props::IPropertySet *params = pc->GetParams();
	const nucleus::ICommandInfo *pci = pc->GetCommandInfo();
	nucleus::IDevice *pd = pci->GetDevice();

	nucleus::RetCode execret = pd->ExecuteCommand(pc, time);
	switch(execret)
	{
		case nucleus::RetCode::COMMAND_PAUSES:
			_this->SetPlayState(nucleus::IPlayer::State::Paused);
			break;

		case nucleus::RetCode::COMMAND_LOADS_SUBSHOW:
		{
			if (_this->m_LoaderFunc)
			{
				props::IProperty *ppathprop = params->GetPropertyById('PATH');
				if (ppathprop)
				{
					nucleus::ITrackSet *pts = _this->m_LoaderFunc(ppathprop->AsString());
					if (pts)
					{
						EnterCriticalSection(&_this->m_cs);

						STrackSetData d;
						d.ptrackset = pts;
						d.time = 0.0;
						_this->m_PlayStack.push_back(d);

						LeaveCriticalSection(&_this->m_cs);
					}
				}
			}
			break;
		}

		case nucleus::RetCode::COMMAND_JUMPS:
		{
			props::IProperty *pjumpprop = params->GetPropertyById('JUMP');
			if(pjumpprop)
			{
				EnterCriticalSection(&_this->m_cs);

				STrackSetData &d = _this->m_PlayStack.back();

				LeaveCriticalSection(&_this->m_cs);
			}
			break;
		}

		case nucleus::RetCode::OK:
		default:
			break;
	}

	return pool::IThreadPool::TaskReturn::OK;
}

pool::IThreadPool::TaskReturn __cdecl CPlayer::CallNotifyFunc(CPlayer *_this, STrackSetData *pd, std::optional<nucleus::IPlayer::State> state_override)
{
	if (_this->m_NotifyFunc)
		_this->m_NotifyFunc(_this, state_override.has_value() ? *state_override : _this->m_State, pd ? pd->time : 0.0f);

	return pool::IThreadPool::TaskReturn::OK;
}


#define COMMAND_PROCESSING_LATENCY_MS		10

DWORD CPlayer::PlayThreadProc()
{
	DWORD wr;

	LARGE_INTEGER cur;
	QueryPerformanceCounter(&cur);

	// 50 ms of latency between commands
	do
	{
		EnterCriticalSection(&m_cs);

		// Sample the clock AFTER acquiring the lock: synchronous preparation
        // can hold it while decoding, and must not count as playback time.
        QueryPerformanceCounter(&cur);

		const double elapsed = double(cur.QuadPart - m_LastSystemTime.QuadPart) / double(m_Frequency.QuadPart);
        m_LastSystemTime = cur;

		if (!m_PlayStack.empty() && (m_State == nucleus::IPlayer::State::Playing))
		{
			bool commands_available = false;
			double interval = elapsed;

			for (TPlayStack::iterator it = m_PlayStack.begin(), last_it = m_PlayStack.end(); it != last_it; it++)
			{
				double next_play_time = it->time + interval;

				for (size_t i = 0, maxi = it->ptrackset->GetNumTracks(); i < maxi; ++i)
				{
					nucleus::ITrack *pt = it->ptrackset->GetTrack(i);
					const props::IProperty *enabled = pt->GetProperties()->GetPropertyById('ENAB');
					if (enabled && !enabled->AsBool())
						continue;

					// Dispatch *all* commands in this interval, including simultaneous ones.
					for (nucleus::ICommand *pc = pt->GetFirstCommandByTime(it->time); pc; pc = pt->GetNextCommand(pc))
					{
						double pctime;
						if (!pt->FindCommand(pc, &pctime))
							continue;

						commands_available = true;

						if (pctime > next_play_time)
							break;

						if (pctime < it->time)
							continue;

						const auto played = std::find_if(m_PlayedCommands.cbegin(), m_PlayedCommands.cend(),
							[pc](const STimedCommandData &d) { return d.pcmd == pc; });

						if (played != m_PlayedCommands.cend())
							continue;

						m_PlayedCommands.push_back({ pc, pctime });
						m_pSys->ThreadPool()->RunTask([this, pc, pctime](size_t) -> pool::IThreadPool::TaskReturn
						{
							return ExecuteCommand(this, pc, pctime);
						});
					}
				}

				it->time = next_play_time;
				STrackSetData *d = &*it;

				m_pSys->ThreadPool()->RunTask([this, d](size_t task_number) -> pool::IThreadPool::TaskReturn
				{
					return CallNotifyFunc(this, d);
				});
			}

			m_CommandsAvailable = commands_available;
			if (!m_CommandsAvailable && (m_State == nucleus::IPlayer::State::Playing))
			{
				SetPlayState(nucleus::IPlayer::State::Stopped);
				SetPlayPosition(0.0);

				m_pSys->ThreadPool()->RunTask([this](size_t task_number) -> pool::IThreadPool::TaskReturn
				{
					return CallNotifyFunc(this, nullptr, std::make_optional<nucleus::IPlayer::State>(nucleus::IPlayer::State::PlaybackEnded));
				});
			}

			STrackSetData &d = m_PlayStack.back();

			// if everything in this trackset has been played, then pop the stack
			if ((d.time >= d.ptrackset->Length()) && (m_PlayStack.size() > 1))
				m_PlayStack.pop_back();
		}

		LeaveCriticalSection(&m_cs);

		wr = WaitForSingleObject(m_hQuit, COMMAND_PROCESSING_LATENCY_MS);
	}
	while (wr == WAIT_TIMEOUT);

	return 0;
}


DWORD WINAPI CPlayer::_PlayThreadProc(LPVOID param)
{
	DWORD ret = 0;

	CPlayer *_this = (CPlayer *)param;
	if(_this)
		ret = _this->PlayThreadProc();

	return ret;
}


void CPlayer::NotifyCommandMoved(const nucleus::ICommand *pcmd, double newtime)
{
	EnterCriticalSection(&m_cs);

	double t = GetPlayPosition();

	TTimedCommandArray::reverse_iterator it = m_PlayedCommands.rbegin();

	for (; it != m_PlayedCommands.rend(); it++)
	{
		if (it->pcmd == pcmd)
			break;
	}

	bool needs_resort = false;

	// was it played already?
	if (it != m_PlayedCommands.rend())
	{
		if (newtime < t)
		{
			// just move the command
			it->time = newtime;
			needs_resort = true;
		}
		else
		{
			// it was in the list, but now it shouldn't be... so remove it
			m_PlayedCommands.erase((std::next(it)).base());
		}
	}
	else if (newtime < t)
	{
		// not sure what the right thing to do here is... should it play, be added to the played command list, or be ignored? for now, ignore
	}

	LeaveCriticalSection(&m_cs);
}


nucleus::IPlayer *nucleus::IPlayer::Create(nucleus::INucleus *pnucleus)
{
	return new CPlayer(pnucleus);
}
