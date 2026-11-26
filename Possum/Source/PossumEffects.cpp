#include "stdafx.h"
#include "PossumEffects.h"
#include <NuCommand.h>
#include <NuCommandInfo.h>
#include <NuDevice.h>
#include <NuDeviceManager.h>
#include <Nucleus.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace PossumEffects
{
    static const Definition definitions[Count] =
	{
        {_T("TrackVolume"),    _T("GlobalVolume"),    _T("Volume"),     0, 2, RGB(88,210,145)},
        {_T("TrackGain"),      _T("GlobalGain"),      _T("Gain"),     -24,24, RGB(251,185,86)},
        {_T("TrackTremolo"),   _T("GlobalTremolo"),   _T("Tremolo"),    0, 1, RGB(231,126,160)},
        {_T("TrackPitchBend"), _T("GlobalPitchBend"), _T("Pitch"),    -12,12, RGB(92,180,246)},
        {_T("TrackVibrato"),   _T("GlobalVibrato"),   _T("Vibrato"),    0,12, RGB(187,146,247)},
        {_T("TrackPan"),       _T("GlobalPan"),       _T("Pan"),       -1, 1, RGB(240,143,97)},
        {_T("TrackDelay"),     _T("GlobalDelay"),     _T("Delay"),      0, 1, RGB(130,219,211)},
        {_T("TrackReverb"),    _T("GlobalReverb"),    _T("Reverb"),     0, 1, RGB(184,216,119)},
        {_T("TrackLowPass"),   _T("GlobalLowPass"),   _T("Low pass"),   0,20000,RGB(240,207,102)},
        {_T("TrackHighPass"),  _T("GlobalHighPass"),  _T("High pass"),  0,20000,RGB(204,151,206)}
    };

	const Definition &Get(size_t layer)
	{
		return definitions[std::min<size_t>(layer, Count-1)];
	}

	static props::IProperty *Field(props::IPropertySet *set, int id, const TCHAR *name)
    {
        auto *p = set->GetPropertyById(id);
        return p ? p : set->CreateProperty(name, id);
    }

	static int ColorId(int layer)
	{
		return 'EC00' + layer;
	}

	bool IsMaster(const nucleus::ITrack *track)
    {
        if (!track)
			return false;

		auto *p = track->GetProperties()->GetPropertyById('EROL');
        return p && p->AsInt() == 1;
    }

	int FindLayer(const nucleus::ICommand *command, bool global)
    {
        if (!command || !command->GetCommandInfo())
			return -1;

		auto *name = command->GetCommandInfo()->GetName();

		for (int i = 0; i < Count; i++)
            if (!_tcscmp(name, global ? definitions[i].globalCommand : definitions[i].trackCommand))
				return i;

		return -1;
    }

	void InitializeTrack(nucleus::ITrack *track, bool master)
    {
        if (!track)
			return;

		auto *p = track->GetProperties();
        Field(p, 'EROL', _T("Editor\\Master Effects"))->SetInt(master ? 1 : 0);
        Field(p, 'EVIS', _T("Editor\\Visible Envelopes"))->SetInt(master ? (1 << Count) - 1 : 0);
        Field(p, 'EACT', _T("Editor\\Active Envelope"))->SetInt(Volume);

		if (master)
        {
            Field(p, 'RHGT', _T("Editor\\Row Height"))->SetInt(master ? 85 : 180);
            Field(p, 'PRST', _T("Editor\\Preset"))->SetString(_T("Effects"));
        }
    }

	nucleus::ITrack *EnsureMaster(nucleus::ITrackSet *set)
    {
        if (!set)
			return nullptr;

		for (size_t i = 0; i < set->GetNumTracks(); i++)
            if (IsMaster(set->GetTrack(i)))
				return set->GetTrack(i);

		nucleus::ITrack *track = nullptr;
        if (set->CreateTrack(&track, _T("Master Effects"), _T("")) != nucleus::RetCode::OK)
			return nullptr;

		InitializeTrack(track, true);

		set->MoveTrack(track, 0);

		// Neutral control points ensure a newly created master lane actually
        // displays its envelopes, while leaving the rendered audio unchanged.
        auto *nucleus = set->GetNucleus();

		auto *device = nucleus ? nucleus->DeviceManager()->GetDeviceByName(_T("Audio")) : nullptr;
        if (device)
        {
            const float neutral[Count] = {1, 0, 0, 0, 0, 0, 0, 0, 20000, 0};

			for (int i = 0; i < Count; i++)
            {
                const auto *info = device->GetCommandByName(definitions[i].globalCommand);
				nucleus::ICommand *command = nullptr;
                if (info && track->AddCommand(info, 0.0, &command) == nucleus::RetCode::OK && command)
                {
                    if (auto *point = command->GetParams()->GetPropertyById('CPNT'))
                        point->SetFloat(neutral[i]);

					if (auto *param = command->GetParams()->GetPropertyById('CVEC'))
						param->SetVec2F(props::TVec2F(1.0f, 0.0f));
				}
            }
        }

		return track;
    }

	unsigned Visible(const nucleus::ITrack *track)
    {
        if (!track)
			return 0;

		auto *p = track->GetProperties()->GetPropertyById('EVIS');
		return p ? unsigned(p->AsInt()) & ((1u << Count) - 1) : (IsMaster(track) ? (1u << Count) - 1 : 0);
	}

	void SetVisible(nucleus::ITrack *track, unsigned mask)
    {
        if (track)
			Field(track->GetProperties(), 'EVIS', _T("Editor\\Visible Envelopes"))->SetInt(mask & ((1u << Count) - 1));
    }

	int Active(const nucleus::ITrack *track)
    {
        if (!track)
			return Volume;

		auto *p = track->GetProperties()->GetPropertyById('EACT');
        return p ? std::clamp<int>(int(p->AsInt()), 0, Count-1) : Volume;
    }

	void SetActive(nucleus::ITrack *track, int layer)
    {
        if (track)
			Field(track->GetProperties(), 'EACT', _T("Editor\\Active Envelope"))->SetInt(std::clamp<int>(layer, 0, Count-1));
    }

	COLORREF Color(const nucleus::ITrackSet *set, int layer)
    {
        layer = std::clamp<int>(layer, 0, Count-1);
        auto *p = set ? const_cast<nucleus::ITrackSet *>(set)->GetProperties()->GetPropertyById(ColorId(layer)) : nullptr;
        return p ? COLORREF(p->AsInt()) : definitions[layer].color;
    }

	void SetColor(nucleus::ITrackSet *set, int layer, COLORREF color)
    {
        if (set && layer >= 0 && layer < Count)
        {
            CString label; label.Format(_T("Editor\\Envelope Color %d"), layer);
            Field(set->GetProperties(), ColorId(layer), label)->SetInt(int64_t(color));
        }
    }
    
	Defaults::Defaults()
	{
		for (int i = 0; i < Count; i++)
			colors[i] = definitions[i].color;
	}

	void Defaults::Apply(nucleus::ITrackSet *set) const
    {
        if (!set)
			return;

		for (int i = 0; i < Count; i++)
			SetColor(set, i, colors[i]);

		for (size_t i = 0; i < set->GetNumTracks(); i++)
        {
            auto *track = set->GetTrack(i);
			bool is_master = IsMaster(track);

			if (is_master)
			{
				SetVisible(track, visible);
				SetActive(track, active);
			}
			
			Field(track->GetProperties(), 'RHGT', _T("Editor\\Row Height"))->SetInt(is_master ? masterHeight : 180);
		}
    }

	static bool Attribute(const char *line, const char *key, unsigned &value)
    {
        char needle[64];
        sprintf_s(needle, "%s=\"", key);

		const char *start = strstr(line, needle);
        if (!start)
			return false;

		start += strlen(needle);
        char *end = nullptr;

		unsigned long parsed = strtoul(start, &end, 10);
        if (end == start || *end != '"')
			return false;

		value = unsigned(parsed);

		return true;
    }

	bool Defaults::Load(const TCHAR *filename)
    {
        if (!filename)
			return false;

		FILE *file = nullptr;
        if (_tfopen_s(&file, filename, _T("rb")) != 0)
			return false;

		char line[512] = {};
        bool valid = false;

		while (fgets(line, sizeof(line), file))
        {
            unsigned value;

			if (strstr(line, "<possum-settings "))
            {
                valid = true;

				if (Attribute(line, "visible", value))
					visible = value;

				if (Attribute(line, "active", value))
					active = int(value);

				if (Attribute(line, "master-height", value))
					masterHeight = int(value);
            }
            else if (strstr(line, "<envelope "))
            {
                unsigned index, color;
                if (Attribute(line, "index", index) && Attribute(line, "color", color) && index < Count)
                    colors[index] = COLORREF(color);
            }
        }

		fclose(file);
        visible &= (1u << Count) - 1;
        active = std::clamp<int>(active, 0, Count-1);
        masterHeight = std::clamp<int>(masterHeight, 36, 2000);

		return valid;
    }

	bool Defaults::Save(const TCHAR *filename) const
    {
        if (!filename)
			return false;

		FILE *file = nullptr;

        if (_tfopen_s(&file, filename, _T("wb")) != 0)
			return false;

		_ftprintf(file, _T("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"));
        _ftprintf(file, _T("<possum-settings version=\"1\" visible=\"%u\" active=\"%d\" master-height=\"%d\">\n"), visible, active, masterHeight);

		for (int i = 0; i < Count; i++)
            _ftprintf(file, _T("\t<envelope index=\"%d\" color=\"%u\"/>\n"), i, unsigned(colors[i]));

		_ftprintf(file, _T("</possum-settings>\n"));

		bool ok = !ferror(file);

		fclose(file);

		return ok;
    }
}
