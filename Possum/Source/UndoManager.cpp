#include "stdafx.h"
#include "UndoManager.h"
#include <algorithm>
#include <utility>

CUndoManager::PropertySnapshot CUndoManager::Snapshot(props::IPropertySet *source)
{
    if (!source)
		return {};

	PropertySnapshot result(props::IPropertySet::CreatePropertySet(), [](props::IPropertySet *p)
	{
		if (p)
			p->Release();
	});

	if (result)
		result->AppendPropertySet(source);

	return result;
}

void CUndoManager::SetSelectionCallbacks(CaptureSelection capture, RestoreSelection restore)
{
    m_CaptureSelection = std::move(capture);
    m_RestoreSelection = std::move(restore);
}

void CUndoManager::Begin(const TCHAR *description)
{
    if (m_Applying || m_Recording)
		return;

	m_Current = {};
	m_Current.description = description ? description : _T("Edit");

	if (m_CaptureSelection)
		m_Current.beforeSelection = m_CaptureSelection();

	m_Recording = true;
}

void CUndoManager::Commit()
{
    if (!m_Recording)
		return;

	m_Recording = false;
    
	if (m_CaptureSelection)
		m_Current.afterSelection = m_CaptureSelection();

	if (!m_Current.changes.empty())
    {
        m_Redo.clear();
        m_Undo.push_back(std::move(m_Current));
    }

	m_Current = {};
}

void CUndoManager::Cancel()
{
    if (!m_Recording)
		return;

	m_Recording = false;
    m_Current = {};
}

void CUndoManager::Clear()
{
    m_Current = {};

	m_Undo.clear();
    m_Redo.clear();
    
	m_Recording = m_Applying = false;

	// A detached track owns all commands still attached to it.
    // Release remaining loose commands first; their owners are null.
    for (auto *cmd : m_Commands)
        if (!cmd->GetOwner())
			cmd->Release();

	for (auto *track : m_Tracks)
        if (!track->GetOwner())
			track->Release();

	m_Commands.clear();
    m_Tracks.clear();
}

void CUndoManager::RecordProperties(props::IPropertySet *target, PropertySnapshot before, PropertySnapshot after)
{
    if (!m_Recording || m_Applying || !target || !before || !after)
		return;

	Change c;
    c.kind = Change::Properties;
    c.properties = target;
    c.beforeProperties = std::move(before);
    c.afterProperties = std::move(after);

	m_Current.changes.push_back(std::move(c));
}

void CUndoManager::RecordCommand(nucleus::ICommand *command, CommandLocation before, CommandLocation after)
{
    if (!m_Recording || m_Applying || !command)
		return;

	Change c;
    c.kind = Change::Command;
    c.command = command;
    c.beforeCommand = before;
    c.afterCommand = after;
    m_Current.changes.push_back(std::move(c));

	m_Commands.insert(command);
}

void CUndoManager::RecordCommandGroup(std::vector<CommandChange> changes)
{
    if (!m_Recording || m_Applying || changes.empty())
        return;

    Change entry;
    entry.kind = Change::CommandGroup;
    for (const auto &change : changes)
        if (change.command)
            m_Commands.insert(change.command);
    entry.commandGroup = std::move(changes);
    m_Current.changes.push_back(std::move(entry));
}

// This function is also used for the initial edit, ensuring that forward,
// Undo and Redo all enforce the same atomic placement rules.
bool CUndoManager::ApplyCommandGroup(const std::vector<CommandChange> &changes, bool forward)
{
    struct Original
    {
        nucleus::ICommand *command;
        nucleus::ITrack *track;
        double time, duration;
    };
    std::vector<Original> originals;
    originals.reserve(changes.size());

    // Validate the entire source set before detaching anything.
    for (const auto &change : changes)
    {
        if (!change.command || !change.command->GetOwner())
            return false;
        auto *track = const_cast<nucleus::ITrack *>(change.command->GetOwner());
        double time = 0.0;
        if (!track->FindCommand(change.command, &time))
            return false;
        originals.push_back({change.command, track, time, change.command->GetDuration()});
        const auto &dest = forward ? change.after : change.before;
        if (!dest.track || dest.time < 0.0 || dest.duration <= 0.0)
            return false;
    }

    size_t detached = 0;
    for (; detached < originals.size(); ++detached)
        if (originals[detached].track->RemoveCommand(originals[detached].command, false) != nucleus::RetCode::OK)
            break;

    if (detached != originals.size())
    {
        for (size_t i = 0; i < detached; ++i)
            originals[i].track->MoveCommand(originals[i].command, originals[i].time, &originals[i].duration);
        return false;
    }

    size_t placed = 0;
    for (; placed < changes.size(); ++placed)
    {
        const auto &dest = forward ? changes[placed].after : changes[placed].before;
        if (dest.track->MoveCommand(changes[placed].command, dest.time, &dest.duration) != nucleus::RetCode::OK)
            break;
    }

    if (placed == changes.size())
        return true;

    // Undo any placements made so far; then restore every original object.
    for (size_t i = 0; i < placed; ++i)
    {
        auto *owner = const_cast<nucleus::ITrack *>(changes[i].command->GetOwner());
        if (owner)
            owner->RemoveCommand(changes[i].command, false);
    }
    bool restored = true;
    for (const auto &original : originals)
        if (original.track->MoveCommand(original.command, original.time, &original.duration) != nucleus::RetCode::OK)
            restored = false;
    ASSERT(restored);
    return false;
}

void CUndoManager::RecordTrack(nucleus::ITrack *track, TrackLocation before, TrackLocation after)
{
    if (!m_Recording || m_Applying || !track)
		return;

	Change c;
    c.kind = Change::Track;
    c.track = track;
    c.beforeTrack = before;
    c.afterTrack = after;
    m_Current.changes.push_back(std::move(c));

	m_Tracks.insert(track);
}

bool CUndoManager::Apply(const Change &change, bool forward)
{
    switch (change.kind)
    {
		case Change::Properties:
		{
			auto source = forward ? change.afterProperties : change.beforeProperties;

			if (!change.properties || !source)
				return false;

			for (size_t i = 0; i < source->GetPropertyCount(); i++)
			{
				auto *src = source->GetProperty(i);
				if (!src)
					continue;

				if (auto *dest = change.properties->GetPropertyById(src->GetID()))
					dest->SetFromProperty(src);
			}

			return true;
		}

		case Change::Command:
		{
			auto destination = forward ? change.afterCommand : change.beforeCommand;
			if (!destination.track)
			{
				auto *owner = const_cast<nucleus::ITrack *>(change.command->GetOwner());
				return !owner || owner->RemoveCommand(change.command, false) == nucleus::RetCode::OK;
			}

            const double *duration = destination.duration >= 0.0 ? &destination.duration : nullptr;
			return destination.track->MoveCommand(change.command, destination.time, duration) == nucleus::RetCode::OK;
		}

        case Change::CommandGroup:
            return ApplyCommandGroup(change.commandGroup, forward);

		case Change::Track:
		{
			auto destination = forward ? change.afterTrack : change.beforeTrack;
			if (!destination.set)
			{
				auto *owner = change.track->GetOwner();
				return !owner || owner->RemoveTrack(change.track, false) == nucleus::RetCode::OK;
			}

			auto *owner = change.track->GetOwner();

			if (owner && owner != destination.set && owner->RemoveTrack(change.track, false) != nucleus::RetCode::OK)
				return false;

			return destination.set->MoveTrack(change.track, destination.position) == nucleus::RetCode::OK;
		}
    }

	return false;
}

bool CUndoManager::Replay(Transaction &transaction, bool forward)
{
    m_Applying = true;
    size_t completed = 0;
    size_t count = transaction.changes.size();

	for (; completed < count; completed++)
    {
        const auto &change = transaction.changes[forward ? completed : count - 1 - completed];

		if (!Apply(change, forward))
			break;
    }

	bool success = (completed == count);
    if (!success)
    {
        // Return to the original state if any operation (e.g. CanFit) failed.
        while (completed)
        {
            completed--;

			Apply(transaction.changes[forward ? completed : count - 1 - completed], !forward);
        }
    }

	m_Applying = false;

	return success;
}

bool CUndoManager::Undo()
{
    if (!CanUndo())
		return false;

	if (!Replay(m_Undo.back(), false))
		return false;

	if (m_RestoreSelection)
		m_RestoreSelection(m_Undo.back().beforeSelection);

	m_Redo.push_back(std::move(m_Undo.back()));
    m_Undo.pop_back();

	return true;
}

bool CUndoManager::Redo()
{
    if (!CanRedo())
		return false;

	if (!Replay(m_Redo.back(), true))
		return false;

	if (m_RestoreSelection)
		m_RestoreSelection(m_Redo.back().afterSelection);

	m_Undo.push_back(std::move(m_Redo.back()));
    m_Redo.pop_back();

	return true;
}

CString CUndoManager::UndoDescription() const
{
	return CanUndo() ? m_Undo.back().description : CString();
}

CString CUndoManager::RedoDescription() const
{
	return CanRedo() ? m_Redo.back().description : CString();
}
