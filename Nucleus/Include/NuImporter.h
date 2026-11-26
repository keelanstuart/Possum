// **************************************************************
// Nucleus Source File
//
// Copyright © 2019-2026, Keelan Sturt


#pragma once


#include <tchar.h>


namespace nucleus
{

	class ITrackSet;

	/// Implement an IImporter, then Register to have use it to read the contents
	/// of a file when ITrackSet::Load is called -- YOU SHOULD NEVER NEED TO CALL IMPORT YOURSELF!!!
	class NUCLEUS_API IImporter
	{

	public:

		/// Call when the code for your Importer becomes available. For example, when your plug-in is Activated.
		static bool Register(const IImporter *pimp);

		/// Call when the code for your Importer goes away. For example, when your plug-in is Deactivated.
		static bool Unregister(const IImporter *pimp);

		/// Returns the number of registered Importer interfaces
		static size_t GetNumImporters();

		/// Returns the registered Importer at the given index
		/// In general, you should looking up importers if you're also loading / unloading them;
		/// They're stored in an array type and you may not get what you think
		static const IImporter *GetImporter(size_t index);

		/// *** YOU IMPLEMENT FROM HERE DOWN ***

		/// Returns the number of extensions that are read by this Importer
		virtual size_t GetNumExtensions() const = NULL;

		/// Returns the file extension at the given index
		/// This should be ONLY the characters in the extension ("abc", "xyz", "trackset", etc.) without a '.' or other characters
		virtual const TCHAR *GetExtension(size_t index) const = NULL;

		/// Returns a description of data source the Importer consumes (ATM-4, Possum, Digistar[?], etc.)
		virtual const TCHAR *GetDescription() const = NULL;

		/// Read the given file and fill out the given TrackSet
		virtual bool Import(const TCHAR *filename, ITrackSet *ptrackset) const = NULL;

	};
};