#pragma once

#include <NuTrackSet.h>
#include <NuCommand.h>
#include <NuCommandSet.h>
#include "UndoManager.h"

class CPossumDocView;

// CPossumDocument document

class CPossumDocument : public CDocument
{
	DECLARE_DYNCREATE(CPossumDocument)

public:
	CPossumDocument();
	virtual ~CPossumDocument();

	nucleus::ITrackSet *GetTrackSet() { return m_TrackSet; }
	CUndoManager &UndoHistory() { return m_Undo; }
	bool Undo();
	bool Redo();

	int GetId() { return m_Id; }

#ifndef _WIN32_WCE
	virtual void Serialize(CArchive& ar);   // overridden for document i/o
#endif

#ifdef _DEBUG
	virtual void AssertValid() const;
#ifndef _WIN32_WCE
	virtual void Dump(CDumpContext& dc) const;
#endif
#endif

	enum EBlockSelectMode
	{
		BSM_NORMAL = 0,
		BSM_ADD,
		BSM_REMOVE
	};

	void ClearSelection();
	void BlockSelection(size_t start_track, size_t end_track, double start_time, double end_time, EBlockSelectMode mode);
	void AddToSelection(nucleus::ICommand *pcmd, bool update_props = true);
	void RemoveFromSelection(nucleus::ICommand *pcmd, bool update_props = true);
	bool IsSelected(nucleus::ICommand *pcmd);
	size_t GetNumSelected();
	nucleus::ICommand *GetSelection(size_t idx);
	void UpdatePropertiesForSelection();
	CUndoManager::Selection CaptureUndoSelection() const;
	void RestoreUndoSelection(const CUndoManager::Selection &selection);

	CPossumDocView *GetView();
	void SetView(CPossumDocView *pv);

	const nucleus::ICommandSet* SelectionSet() { return m_Sel; }

protected:
	nucleus::ICommandSet *m_Sel;
	CUndoManager m_Undo;

	CPossumDocView *m_pView;

	nucleus::ITrackSet *m_TrackSet;
	int m_Id;

	CString m_Author, m_Copyright, m_Description, m_Title;

public:
	virtual BOOL OnNewDocument();
	virtual BOOL OnOpenDocument(LPCTSTR filename);
	virtual BOOL OnSaveDocument(LPCTSTR filename);
	virtual void OnCloseDocument();
    BOOL Save() { return DoFileSave(); }
    BOOL SaveAs() { return DoSave(nullptr, TRUE); }

	DECLARE_MESSAGE_MAP()
};
