#include <synchapi.h>
#include <tchar.h>

#include <Nucleus.h>

#include <NuTrackSet.h>
#include <NuPlayer.h>

#include <map>
#include <string>
#include <algorithm>


typedef std::map<tstring, nucleus::ITrackSet *> TNamedTrackMap;
TNamedTrackMap g_LoadedTracks;


void mylog(const TCHAR *msg)
{
	// just print to the console... but we could write to a gui textbox in a different kind of app
	_tprintf(msg);
}


int _tmain(int argc, const TCHAR *argv[])
{
	if (argc < 2)
		return -1;

	// create a nucleus
	nucleus::INucleus *pnuc = nucleus::INucleus::Create();
	if (!pnuc)
		return -2;

	// get the log
	nucleus::ILog *plog = pnuc->Log();
	if (!plog)
		return -3;

	int ret = 0;

	// set the logging output function
	pnuc->Log()->SetRedirectFunction(mylog);

	plog->Log(_T("Loading plugins...\n"));

	// just look in the cwd for *.nudll (or *.dnudll in _DEBUG)
#if !defined(_DEBUG)
	pnuc->PluginManager()->DiscoverPlugins(_T("*.nudll"), true);
#else
	pnuc->PluginManager()->DiscoverPlugins(_T("*.dnudll"), true);
#endif

	// iterate over the loaded plug-ins and report their names
	for (size_t i = 0, maxi = pnuc->PluginManager()->GetNumPlugins(); i < maxi; i++)
	{
		nucleus::IPlugin *pplug = pnuc->PluginManager()->GetPlugin(i);
		if (!pplug)
			continue;

		plog->Log(_T("\t%s\n"), pplug->GetName());
	}

	plog->Log(_T("\n"));

	// create a trackset
	nucleus::ITrackSet *ptracks = nucleus::ITrackSet::Create(pnuc);
	if (ptracks)
	{
		plog->Log(_T("Playing: \"%s\"\n"), argv[1]);

		// load the trackset from a file
		if (ptracks->Load(argv[1]) == nucleus::RetCode::OK)
		{
			// if we loaded it, the create a player
			// in a multiple-document view scenario, you would create a player for each document
			nucleus::IPlayer *pplayer = nucleus::IPlayer::Create(pnuc);
			if (pplayer)
			{
				// create a loader function for the player with a cache lookup...
				// this allows sub-shows to be loaded and played on the play stack, then cleaned up later
				nucleus::IPlayer::TrackSetLoader myloader = [pnuc](const TCHAR *filename)
				{
					tstring fn = filename;
					std::transform(fn.begin(), fn.end(), fn.begin(), _tolower);

					// look for the name of the file in the map of laoded tracksets and return it if found
					TNamedTrackMap::iterator it = g_LoadedTracks.find(fn);
					if (it != g_LoadedTracks.end())
						return it->second;

					// otherwise, create a new one, load it, and insert it into the map
					nucleus::ITrackSet *ret = nucleus::ITrackSet::Create(pnuc);
					if (ret)
					{
						ret->Load(fn.c_str());
						g_LoadedTracks.insert(TNamedTrackMap::value_type(fn, ret));
					}

					return ret;
				};

				// set the loader function, for sub-shows
				pplayer->SetTrackSetLoaderFunction(myloader);

				// set the trackset on the player
				pplayer->SetActiveTrackSet(ptracks);

				// start playback
				pplayer->SetPlayState(nucleus::IPlayer::State::Playing);

				// wait until playback is complete
				while (pplayer->GetPlayState() == nucleus::IPlayer::State::Playing)
				{
					if (pplayer->GetPlayPosition() > ptracks->Length())
						pplayer->SetPlayState(nucleus::IPlayer::State::Stopped);

					Sleep(1000);
					plog->Log(_T("."));
				}

				plog->Log(_T("\nFinished\n\n"));

				// release the player
				pplayer->Release();
			}
			else
				ret = -5;

			// release the trackset
			ptracks->Release();

			// release any tracks the player loaded
			std::for_each(g_LoadedTracks.begin(), g_LoadedTracks.end(), [](TNamedTrackMap::value_type &n) { if (n.second) n.second->Release(); });
		}
		else
			ret = -4;
	}

	// release the nucleus
	pnuc->Release();

	return ret;
}

