#include "stdafx.h"
#include "Possum.h"
#include "PossumDocument.h"
#include "PossumLayout.h"
#include "resource.h"
#include <NuTrack.h>
#include <NuCommand.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace PossumLayout
{
    struct Item
    {
        nucleus::ICommand *command;
        nucleus::ITrack *track;
		double start, duration;
		std::optional<double> val_freq, val_cpnt;
    };

    bool CanApply(CPossumDocument *document)
    {
        return document && document->GetNumSelected() > 1 &&
            (!theApp.m_Player || theApp.m_Player->GetPlayState() == nucleus::IPlayer::State::Stopped);
    }

    bool Apply(CPossumDocument *document, UINT commandId)
    {
        if (!CanApply(document))
            return false;

        std::vector<Item> items;
        items.reserve(document->GetNumSelected());
        double first = DBL_MAX;
        double last = 0.0;

		double lo_freq = DBL_MAX;
        double hi_freq = 0.0;

		double lo_cpnt = DBL_MAX;
		double hi_cpnt = -DBL_MAX;

        for (size_t i = 0; i < document->GetNumSelected(); ++i)
        {
            auto *command = document->GetSelection(i);
            auto *track = command ? const_cast<nucleus::ITrack *>(command->GetOwner()) : nullptr;
            if (!track)
                continue;

			double time = 0.0;
            if (!track->FindCommand(command, &time))
                continue;

			auto *params = command->GetParams();

			double duration = command->GetDuration();

			items.push_back({command, track, time, duration});

			if (auto *p = params->GetPropertyById('FREQ'))
				items.back().val_freq = (double)p->AsFloat();
			else if (auto *p = params->GetPropertyById('PMUL'))
				items.back().val_freq = (double)p->AsFloat() * 261.63;
			else if (auto *p = params->GetPropertyById('CPNT'))
				items.back().val_cpnt = (double)p->AsFloat();

			first = std::min<double>(first, time);
            last = std::max<double>(last, time + duration);

			if (items.back().val_freq.has_value())
			{
				lo_freq = std::min<double>(lo_freq, *items.back().val_freq);
				hi_freq = std::max<double>(hi_freq, *items.back().val_freq);
			}

			if (items.back().val_cpnt.has_value())
			{
				lo_cpnt = std::min<double>(lo_cpnt, *items.back().val_cpnt);
				hi_cpnt = std::max<double>(hi_cpnt, *items.back().val_cpnt);
			}
		}

		if (items.size() < 2)
            return false;

        bool vertical = commandId == ID_LAYOUT_PITCH_LOW || commandId == ID_LAYOUT_PITCH_HIGH;
        auto &undo = document->UndoHistory();

		if (vertical)
        {
            if ((lo_freq == DBL_MAX) && (lo_cpnt == DBL_MAX))
                return false;

			double targ_freq = (commandId == ID_LAYOUT_PITCH_LOW) ? lo_freq : hi_freq;
			double targ_cpnt = (commandId == ID_LAYOUT_PITCH_LOW) ? lo_cpnt : hi_cpnt;

			undo.Begin(_T("Align Pitch"));
            bool changed = false;

			for (auto &item : items)
            {
				auto *params = item.command->GetParams();
                auto before = CUndoManager::Snapshot(params);

				if (item.val_freq.has_value())
				{
					if (auto *p = params->GetPropertyById('FREQ'))
						p->SetFloat(float(targ_freq));
					else if (auto *p = params->GetPropertyById('PMUL'))
						p->SetFloat(float(targ_freq / 261.63));
				}

				if (item.val_cpnt.has_value())
				{
					if (auto *p = params->GetPropertyById('CPNT'))
						p->SetFloat(float(targ_cpnt));
				}

				undo.RecordProperties(params, before, CUndoManager::Snapshot(params));
                changed = true;
            }

			if (changed)
                undo.Commit();
            else
                undo.Cancel();

			if (!changed)
                return false;
        }
        else
        {
            std::vector<CUndoManager::CommandChange> changes;
            changes.reserve(items.size());

			for (const auto &item : items)
            {
                double time = item.start;

				switch (commandId)
                {
					case ID_LAYOUT_TIME_LEFT: time = first; break;
					case ID_LAYOUT_TIME_CENTER: time = (first + last - item.duration) * 0.5; break;
					case ID_LAYOUT_TIME_RIGHT: time = last - item.duration; break;
					default: return false;
                }

				if (std::abs(time - item.start) > 1e-9)
                    changes.push_back({item.command,
                        {item.track, item.start, item.duration},
                        {item.track, time, item.duration}});
            }

			if (changes.empty() || !CUndoManager::ApplyCommandGroup(changes, true))
                return false;

			undo.Begin(_T("Align Commands"));
            undo.RecordCommandGroup(std::move(changes));
            undo.Commit();
        }

		document->SetModifiedFlag();
        document->UpdatePropertiesForSelection();

		theApp.RedrawActiveTimeline();
        theApp.RefreshCommandList(false);

		return true;
    }
}
