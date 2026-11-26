#pragma once
#include <NuTrackSet.h>
#include <NuTrack.h>
#include <NuCommand.h>
#include <PowerProps/Include/PowerProps.h>
#include <memory>
#include <vector>
#include <set>
#include <functional>

// Owned by CPossumDocument. Destroy before the document's ITrackSet.
class CUndoManager
{

public:
    using PropertySnapshot = std::shared_ptr<props::IPropertySet>;

	struct CommandLocation
	{
		nucleus::ITrack *track = nullptr;
		double time = 0.0;
        // Negative means leave duration unchanged; otherwise MoveCommand
        // updates time and duration atomically (important for overlap checks).
        double duration = -1.0;
	};

    struct CommandChange
    {
        nucleus::ICommand *command = nullptr;
        CommandLocation before, after;
    };

    // Atomically applies a group of placements, rolling back on failure.
    static bool ApplyCommandGroup(const std::vector<CommandChange> &changes, bool forward);

	struct TrackLocation
	{
		nucleus::ITrackSet *set = nullptr;
		size_t position = 0;
	};

    CUndoManager() = default;
    ~CUndoManager() { Clear(); }
    CUndoManager(const CUndoManager &) = delete;

	CUndoManager &operator=(const CUndoManager &) = delete;

    static PropertySnapshot Snapshot(props::IPropertySet *source);

	using Selection = std::vector<nucleus::ICommand *>;
	using CaptureSelection = std::function<Selection()>;
    using RestoreSelection = std::function<void(const Selection &)>;

	void SetSelectionCallbacks(CaptureSelection capture, RestoreSelection restore);
    void Begin(const TCHAR *description);
    void Commit();
    void Cancel(); // discard a transaction before it is applied; caller owns rollback
    void Clear();
    void RecordProperties(props::IPropertySet *target, PropertySnapshot before, PropertySnapshot after);
    void RecordCommand(nucleus::ICommand *command, CommandLocation before, CommandLocation after);
    void RecordCommandGroup(std::vector<CommandChange> changes);
    void RecordTrack(nucleus::ITrack *track, TrackLocation before, TrackLocation after);
    bool Undo();
    bool Redo();
    bool CanUndo() const { return !m_Undo.empty() && !m_Recording && !m_Applying; }
    bool CanRedo() const { return !m_Redo.empty() && !m_Recording && !m_Applying; }
    bool IsApplying() const { return m_Applying; }
    bool IsRecording() const { return m_Recording; }
   
	CString UndoDescription() const;
    CString RedoDescription() const;

private:

	struct Change
    {
        enum Kind { Properties, Command, Track, CommandGroup } kind = Properties;
        props::IPropertySet *properties = nullptr;
        PropertySnapshot beforeProperties, afterProperties;
        nucleus::ICommand *command = nullptr;
        CommandLocation beforeCommand, afterCommand;
        std::vector<CommandChange> commandGroup;
        nucleus::ITrack *track = nullptr;
        TrackLocation beforeTrack, afterTrack;
    };

	struct Transaction
	{
		CString description;
		std::vector<Change> changes;
		Selection beforeSelection, afterSelection;
	};

	std::vector<Transaction> m_Undo, m_Redo;
	Transaction m_Current;

	bool m_Recording = false;
	bool m_Applying = false;

	CaptureSelection m_CaptureSelection;
    RestoreSelection m_RestoreSelection;

	// Maintain registry of objects ever detached into history; only the manager
    // releases objects that are *still* detached when all history is cleared.
    std::set<nucleus::ICommand *> m_Commands;
    std::set<nucleus::ITrack *> m_Tracks;

	static bool Apply(const Change &change, bool forward);
    bool Replay(Transaction &transaction, bool forward);
};
