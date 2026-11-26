// **************************************************************
// Nucleus Source File
//
// Copyright © 2019-2026, Keelan Sturt


#pragma once

#include <PowerProps\Include\PowerProps.h>
#include <NuCommandSet.h>
#include <optional>


namespace nucleus
{

	class ICommand;
	class ICommandInfo;
	class ITrackSet;

	class NUCLEUS_API ITrack
	{

	public:
		/// Releases any resources that the track has acquired
		virtual void Release() = NULL;

		/// Returns the trackset that owns this track
		virtual ITrackSet *GetOwner() const = NULL;

		/// Provides the properties for the track
		/// By default, a track will have a 'NAME' and 'DESC' string property
		virtual props::IPropertySet *GetProperties() const = NULL;

		/// A helper to get the 'NAME' property's contents
		virtual const TCHAR *GetName() const = NULL;

		/// A helper to get the 'DESC' property's contents
		virtual const TCHAR *GetDescription() const = NULL;

		/// Returns the number of commands that exist in this track
		virtual size_t GetNumCommands() const = NULL;

		/// Returns the ICommands in the order they appear in the track
		virtual ICommand *GetCommand(size_t idx, double *time = nullptr) const = NULL;

		/// Given a time value, returns the command during which the given time falls
		virtual bool FindCommand(double time, ICommand **ppcmd_ret = nullptr) const = NULL;

		/// Given a command, returns the time that it occurs
		virtual bool FindCommand(const ICommand *pcmd, double *ptime_ret = nullptr) const = NULL;

		/// Given a command, returns its index
		virtual bool FindCommand(const ICommand *pcmd, size_t *pindex_ret = nullptr) const = NULL;

		/// Returns the first command that occurs after time or which runs through time (start + duration)
		virtual ICommand *GetFirstCommandByTime(double time) const = NULL;

		/// Returns the command that succeeds the given command
		virtual ICommand* GetNextCommand(const ICommand *pcmd, std::optional<const ICommandInfo *> type = std::nullopt) const = NULL;

		/// Returns the command that preceeds the given command
		virtual ICommand *GetPrevCommand(const ICommand *pcmd, std::optional<const ICommandInfo *> type = std::nullopt) const = NULL;

		/// Adds a new command to the track, if possible, based on insertion time and command duration
		virtual nucleus::RetCode AddCommand(const ICommandInfo *pcmdinfo, double time, ICommand **ret) = NULL;

		/// Removes the command from this track and optionally disposes of it
		virtual nucleus::RetCode RemoveCommand(nucleus::ICommand *pcmd, bool release_cmd = true) = NULL;

		/// Moves a command from one time (or from another track) to a time given for this track
		virtual nucleus::RetCode MoveCommand(ICommand *pcmd, double time, const double *new_duration = nullptr, const nucleus::ICommandSet *ignore_blockers = nullptr) = NULL;

		/// Determines if a command can fit at the given time and duration
		virtual bool CanFit(double time, double duration, const nucleus::ICommand* pcmd, const nucleus::ICommandSet* ignore_blockers = nullptr) = NULL;

		/// Returns the length of the track in seconds
		virtual double Length() const = NULL;

	};

};