// **************************************************************
// Nucleus Source File
//
// Copyright © 2019-2026, Keelan Sturt


#pragma once

#include <PowerProps\Include\PowerProps.h>


namespace nucleus
{

	class ICommandInfo;
	class ITrack;

	class NUCLEUS_API ICommand
	{

	public:

		using PropertyChangeHandler = std::function<void(ICommand *, const props::IProperty *)>;

		/// Gets the ICommandInfo that 
		virtual const ICommandInfo *GetCommandInfo() const = NULL;

		/// Provides the parameters for the command
		virtual props::IPropertySet *GetParams() const = NULL;

		/// Gets the track that owns this command and the time, if desired
		virtual const ITrack *GetOwner(double *time = nullptr) const = NULL;

		/// Returns the amount of time this ICommand should last in seconds
		virtual double GetDuration() const = NULL;

		/// Releases the resources allocated by this ICommand
		virtual void Release() = NULL;

		/// Sets the function that is called back when a property owned by this ICommand changes
		virtual void SetPropertyChangeHandler(PropertyChangeHandler handler) = NULL;

		/// Creates an ICommand instance from the given ICommandInfo
		static ICommand *CreateCommand(const ICommandInfo *pcmdinfo);
	};

};
