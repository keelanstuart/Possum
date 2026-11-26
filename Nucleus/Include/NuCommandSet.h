// **************************************************************
// Nucleus Source File
//
// Copyright © 2019-2026, Keelan Sturt


#pragma once


namespace nucleus
{

	class ICommand;

	class NUCLEUS_API ICommandSet
	{
	public:
		/// Adds the given ICommand to the set
		virtual void Add(ICommand *pcmd) = 0;

		/// Removes the given ICommand from the set
		virtual void Remove(ICommand *pcmd) = 0;

		/// Gets the ICommand at the given index or returns nullptr if the index is out of bounds
		virtual ICommand* Get(size_t index) const = 0;

		/// Removes all ICommands from the set
		virtual void Clear() = 0;

		/// Returns true if the set contains the given ICommand, false if it does not.
		virtual bool Contains(ICommand *pcmd) const = 0;

		/// Returns the number of elements in the set
		virtual size_t Size() const = 0;

		/// Returns true if the set if empty, false if there is anything in it
		virtual bool Empty() const = 0;

		/// A parameter to the ForEach function; will be called for each GUID in the set
		using CommandFunc = std::function<void(ICommand *)>;

		/// Calls the given user-defined function back for each ICommand in the set. Passes it userdata1 and userdata2
		virtual void ForEach(CommandFunc func) const = 0;

		/// Frees any resources the set was using
		virtual void Release() = 0;

		static ICommandSet* Create();
	};

};
