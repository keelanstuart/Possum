// **************************************************************
// Nucleus Source File
//
// Copyright © 2019-2026, Keelan Sturt


#pragma once


namespace nucleus
{

	class ITrackSet;

	class NUCLEUS_API IPlayer
	{

	public:

		static IPlayer *Create(INucleus *pnucleus);

		virtual void Release() = NULL;

		using State = enum
		{
			Stopped = 0,
			Paused,
			Playing,
			PlaybackEnded		// FYI: for notification only! Trying to set this state won't work.
		};

		/// Implement a function following this prototype to be notified of the active trackset's state and playtime.
		using NotifyFunction = std::function<void(IPlayer *, State, double)>;

		/// Sets the callback for playback notifications
		virtual void SetNotifyFunction(NotifyFunction func) = NULL;

		/// Implement a function following this prototype when you want to return ITrackSet interfaces based on a filename.
		/// Why would you want to do this? Sub-shows. When a sub-show is encountered, this allows your application to load
		/// and give it back to the IPlayer.
		using TrackSetLoader = std::function<ITrackSet *(const TCHAR *)>;

		/// Sets the callback for a trackset loading function, used when a trackset reference command is encountered
		virtual void SetTrackSetLoaderFunction(TrackSetLoader func) = NULL;

		/// Clears and resets the top of the ITrackSet stack, effectively loading a new one
		virtual RetCode SetActiveTrackSet(const ITrackSet *ptracks) = NULL;

		/// Returns the top of the ITrackSet stack
		virtual ITrackSet *GetActiveTrackSet() = NULL;

		/// Returns true if the given ITrackSet is in the stack
		virtual bool IsTrackSetInPlayStack(const ITrackSet *ptracks) = NULL;

		/// Returns the playback time in the current top-of-stack ITrackSet
		virtual double GetPlayPosition() = NULL;

		/// Sets the playback time for the top-of-stack ITrackSet.
		/// When called while stopped, the next Play starts here, skipping
		/// content already underway but reconstructing effect envelopes.
		virtual void SetPlayPosition(double time) = NULL;

		/// Returns the state of playback: STOPPED, PAUSED, or PLAYING
		virtual State GetPlayState() const = NULL;

		/// Sets the playing state
		virtual RetCode SetPlayState(State state) = NULL;

	};

};
