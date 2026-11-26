// **************************************************************
// Nucleus Source File
//
// Copyright © 2019-2026, Keelan Sturt


#include "stdafx.h"

#include <NuImporter.h>


using namespace nucleus;


typedef std::deque<const IImporter *> TImporterArray;
TImporterArray g_Importers;


bool IImporter::Register(const IImporter *pimp)
{
	if (!pimp)
		return false;

	TImporterArray::const_iterator it = std::find(g_Importers.cbegin(), g_Importers.cend(), pimp);
	if (it != g_Importers.cend())
		return true;

	g_Importers.push_back(pimp);

	return true;
}

bool IImporter::Unregister(const IImporter *pimp)
{
	if (!pimp)
		return false;

	TImporterArray::const_iterator it = std::find(g_Importers.cbegin(), g_Importers.cend(), pimp);
	if (it != g_Importers.cend())
		return false;

	g_Importers.erase(it);

	return true;
}

size_t IImporter::GetNumImporters()
{
	return g_Importers.size();
}

const IImporter *IImporter::GetImporter(size_t index)
{
	return g_Importers[index];
}
