// Source\PossumDocument.cpp : implementation file
//

#include "stdafx.h"
#include "Possum.h"
#include "PossumDocument.h"
#include "Util.h"
#include "PossumEffects.h"

#include <NuCommand.h>
#include <NuTrack.h>
#include <NuCommandInfo.h>
#include <NuDevice.h>

// CPossumDocument

IMPLEMENT_DYNCREATE(CPossumDocument, CDocument)

CPossumDocument::CPossumDocument() : CDocument()
{
	static int id = 0;
	id++;
	m_Id = id;

	m_pView = nullptr;

	m_TrackSet = nucleus::ITrackSet::Create(theApp.m_pSys);
	m_Sel = nucleus::ICommandSet::Create();
	m_Undo.SetSelectionCallbacks(
		[this]() { return CaptureUndoSelection(); },
		[this](const CUndoManager::Selection &selection) { RestoreUndoSelection(selection); });
}

BOOL CPossumDocument::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
        return FALSE;

	nucleus::ITrack *track = nullptr;
    m_TrackSet->CreateTrack(&track, _T("Audio 1"), nullptr);
    m_TrackSet->CreateTrack(&track, _T("Audio 2"), nullptr);

	PossumEffects::EnsureMaster(m_TrackSet);
    theApp.m_EffectDefaults.Apply(m_TrackSet);
    theApp.m_DocMan.RegisterDocument(this);

	return TRUE;
}

BOOL CPossumDocument::OnOpenDocument(LPCTSTR filename)
{
    if (m_TrackSet->Load(filename) != nucleus::RetCode::OK)
        return FALSE;
    PossumEffects::EnsureMaster(m_TrackSet);
    SetModifiedFlag(FALSE);
    theApp.m_DocMan.RegisterDocument(this);
    return TRUE;
}

BOOL CPossumDocument::OnSaveDocument(LPCTSTR filename)
{
    if (m_TrackSet->Save(filename) != nucleus::RetCode::OK)
        return FALSE;
    SetModifiedFlag(FALSE);
    return TRUE;
}

void CPossumDocument::OnCloseDocument()
{
    // Audio keeps non-owning track pointers; stop before MFC destroys this document.
    if (theApp.m_Player && theApp.m_Player->IsTrackSetInPlayStack(m_TrackSet))
        theApp.m_Player->SetPlayState(nucleus::IPlayer::State::Stopped);
    theApp.m_DocMan.UnregisterDocument(this);
    CDocument::OnCloseDocument();
}

CPossumDocument::~CPossumDocument()
{
	m_Undo.Clear(); // History must release detached objects before the TrackSet.

	if (m_Sel)
		m_Sel->Release();

	if (m_TrackSet)
		m_TrackSet->Release();

}


BEGIN_MESSAGE_MAP(CPossumDocument, CDocument)
END_MESSAGE_MAP()


// CPossumDocument diagnostics

#ifdef _DEBUG
void CPossumDocument::AssertValid() const
{
	CDocument::AssertValid();
}

#ifndef _WIN32_WCE
void CPossumDocument::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif
#endif //_DEBUG

#ifndef _WIN32_WCE
// CPossumDocument serialization

void CPossumDocument::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
	}
	else
	{

	}
}
#endif



CUndoManager::Selection CPossumDocument::CaptureUndoSelection() const
{
	CUndoManager::Selection selection;
	for (size_t i = 0; i < m_Sel->Size(); ++i)
		selection.push_back(m_Sel->Get(i));
	return selection;
}

void CPossumDocument::RestoreUndoSelection(const CUndoManager::Selection &selection)
{
	// A saved selection is non-owning: validate by pointer identity without
	// dereferencing commands that may have been removed by subsequent edits.
	m_Sel->Clear();
	for (auto *wanted : selection)
	{
		bool found = false;
		for (size_t row = 0; !found && row < m_TrackSet->GetNumTracks(); ++row)
		{
			auto *track = m_TrackSet->GetTrack(row);
			for (size_t i = 0; i < track->GetNumCommands(); ++i)
				if (track->GetCommand(i) == wanted) { found = true; break; }
		}
		if (found) m_Sel->Add(wanted);
	}
	UpdatePropertiesForSelection();
}

void CPossumDocument::ClearSelection()
{
	if (m_Sel->Size() == 0)
		return;

	m_Sel->Clear();

	theApp.RedrawActiveTimeline();
	theApp.RefreshCommandList(true);
	UpdatePropertiesForSelection();
}

void CPossumDocument::UpdatePropertiesForSelection()
{
	if (theApp.m_DocMan.GetActiveDocument()->GetNumSelected() == 0)
	{
		theApp.SetActiveProperties(nullptr, false, _T(""));
	}
	else if (theApp.m_DocMan.GetActiveDocument()->GetNumSelected() == 1)
	{
		double t;
		nucleus::ICommand *pc = m_Sel->Get(0);
		const nucleus::ITrack* pt = pc->GetOwner(&t);
		const nucleus::ICommandInfo* pci = pc->GetCommandInfo();
		const nucleus::IDevice* pd = pci->GetDevice();
		pc->GetOwner()->FindCommand(pc, &t);

		UINT h, m, s, f;
		GetTimeComponents(t, h, m, s, f);

		CString title;
		title.Format(_T("%s : %s : %s @ %02d:%02d:%02d.%01d"), pt->GetProperties()->GetPropertyById('NAME')->AsString(), pd->GetName(), pci->GetName(), h, m, s, f);

		theApp.SetActiveProperties(pc->GetParams(), false, title, (nucleus::ICommandInfo *)pci, nullptr);
	}
	else
	{
		theApp.SetActiveProperties(nullptr, false, (theApp.m_DocMan.GetActiveDocument()->GetNumSelected() > 1) ? _T("[multiple selected]") : _T(""));
	}
}

void CPossumDocument::BlockSelection(size_t start_track, size_t end_track, double start_time, double end_time, EBlockSelectMode mode)
{
	if (start_track > m_TrackSet->GetNumTracks())
		return;

	if (mode == EBlockSelectMode::BSM_NORMAL)
		ClearSelection();

	nucleus::ICommand* pc = nullptr;

	for (size_t t = start_track; t <= end_track; t++)
	{
		double ct;

		nucleus::ITrack *pt = m_TrackSet->GetTrack(t);
		pc = pt->GetFirstCommandByTime(start_time);
		pt->FindCommand(pc, &ct);
		while (pc && (ct < end_time))
		{
			if (mode == EBlockSelectMode::BSM_REMOVE)
			{
				if (IsSelected(pc))
					RemoveFromSelection(pc, false);
			}
			else
			{
				AddToSelection(pc, true);
			}

			pc = pt->GetNextCommand(pc);
			pt->FindCommand(pc, &ct);
		}
	}

	theApp.RedrawActiveTimeline();
	theApp.RefreshCommandList(false);
	UpdatePropertiesForSelection();
}

void CPossumDocument::AddToSelection(nucleus::ICommand* pcmd, bool update_props)
{
	if (!pcmd)
		return;

	m_Sel->Add(pcmd);

	if (update_props)
	{
		theApp.RedrawActiveTimeline();
		theApp.RefreshCommandList(false);
		UpdatePropertiesForSelection();
	}
}

void CPossumDocument::RemoveFromSelection(nucleus::ICommand* pcmd, bool update_props)
{
	if (!pcmd)
		return;

	m_Sel->Remove(pcmd);

	if (update_props)
	{
		theApp.RedrawActiveTimeline();
		theApp.RefreshCommandList(false);
		UpdatePropertiesForSelection();
	}
}

bool CPossumDocument::IsSelected(nucleus::ICommand *pcmd)
{
	return m_Sel->Contains(pcmd);
}

size_t CPossumDocument::GetNumSelected()
{
	return m_Sel->Size();
}

nucleus::ICommand *CPossumDocument::GetSelection(size_t idx)
{
	return m_Sel->Get(idx);
}

CPossumDocView *CPossumDocument::GetView()
{
	return m_pView;
}

void CPossumDocument::SetView(CPossumDocView *pv)
{
	m_pView = pv;
}

// CPossumDocument commands


bool CPossumDocument::Undo()
{
    if (theApp.m_Player && theApp.m_Player->GetPlayState() != nucleus::IPlayer::State::Stopped) return false;
    if (!m_Undo.Undo()) return false;
    SetModifiedFlag();
    theApp.RedrawActiveTimeline();
    theApp.RefreshCommandList(true);
    return true;
}

bool CPossumDocument::Redo()
{
    if (theApp.m_Player && theApp.m_Player->GetPlayState() != nucleus::IPlayer::State::Stopped) return false;
    if (!m_Undo.Redo()) return false;
    SetModifiedFlag();
    theApp.RedrawActiveTimeline();
    theApp.RefreshCommandList(true);
    return true;
}
