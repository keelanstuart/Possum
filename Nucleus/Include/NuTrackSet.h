// **************************************************************
// Nucleus Source File
//
// Copyright © 2019-2026, Keelan Sturt


#pragma once

#include <PowerProps/Include/PowerProps.h>

namespace nucleus
{

	class ITrack;

	/// A top-level interface (as in, you can create one yourself, as opposed to things like ITrack, which
	/// can only be instantiated by an ITrackSet) that contains all show data in a set of tracks and can
	/// de/serialize that data
	class NUCLEUS_API ITrackSet
	{

#define INVALID_TRACK	-1

	public:

		/// Instantiates an object that implements the ITrackSet interface
		static ITrackSet *Create(INucleus *pnucleus);

		/// Frees all resources allocated by this TrackSet
		virtual void Release() = NULL;

		/// Loads TrackSet data from a file
		virtual RetCode Load(const TCHAR *filename) = NULL;

		/// Saves the data from this TrackSet to a file
		virtual RetCode Save(const TCHAR *filename) const = NULL;

		/// Creates an additional track in this TrackSet
		virtual RetCode CreateTrack(ITrack **pptrackret, const TCHAR *name, const TCHAR *description) = NULL;

		/// Deletes a track from this TrackSet
		virtual RetCode RemoveTrack(ITrack *ptrack, bool release_track) = NULL;

		/// Moves an track inside this TrackSet by the given offset
		virtual RetCode MoveTrack(ITrack *ptrack, size_t position) = NULL;

		/// Returns the number of tracks currently in this set
		virtual size_t GetNumTracks() const = NULL;

		/// Returns a track by it's index in this set
		virtual ITrack *GetTrack(size_t index) const = NULL;

		/// Given a name, returns a track if it is found, otherwise nullptr
		virtual ITrack *GetTrackByName(const _TCHAR *name) const = NULL;

		/// Given a track, returns it's index in this set, or INVALID_TRACK if it isn't part of this set
		virtual size_t GetTrackIndex(const ITrack *ptrack) const = NULL;

		/// Returns a set properties that apply to this TrackSet
		virtual props::IPropertySet *GetProperties() = NULL;

		/// Returns the INucleus interface that was used to create this TrackSet
		virtual INucleus *GetNucleus() const = NULL;

		/// Returns the aggregate length of this TrackSet
		virtual double Length() const = NULL;

	};

};
