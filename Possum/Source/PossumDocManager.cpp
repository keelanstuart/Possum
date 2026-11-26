#include "stdafx.h"
#include "Possum.h"
#include "PossumDocManager.h"
#include "PossumDocView.h"
#include <algorithm>

CPossumDocManager::CPossumDocManager() = default;
CPossumDocManager::~CPossumDocManager() = default;

CPossumDocument *CPossumDocManager::CreateDocument(const TCHAR *filename, bool /*create_default_tracks*/)
{
	CPossumDocument *ret = nullptr;

	if (filename)
    {
		ret = FindDocument(filename);

		if (ret)
		{
			ret->GetView();
			ret->GetView()->GetParentFrame()->ActivateFrame();
        }
		else
		{
			ret = DYNAMIC_DOWNCAST(CPossumDocument, theApp.OpenDocumentFile(filename));
		}
    }
	else
	{
		POSITION pos = AfxGetApp()->GetFirstDocTemplatePosition();
		CDocTemplate* pTemplate = (CDocTemplate*)theApp.GetNextDocTemplate(pos);

		ASSERT(pTemplate != NULL);
		ASSERT_KINDOF(CDocTemplate, pTemplate);

		ret = DYNAMIC_DOWNCAST(CPossumDocument, pTemplate->OpenDocumentFile(NULL));
	}

	return ret;
}

CPossumDocument *CPossumDocManager::FindDocument(const TCHAR *filename)
{
    if (!filename)
		return nullptr;

	for (auto *doc : m_Docs)
    {
        if (!_tcsicmp(filename, doc->GetPathName()) || !_tcsicmp(filename, doc->GetTitle()))
            return doc;
    }

	return nullptr;
}

void CPossumDocManager::RegisterDocument(CPossumDocument *doc)
{
    if (doc && std::find(m_Docs.begin(), m_Docs.end(), doc) == m_Docs.end())
        m_Docs.push_back(doc);
}

void CPossumDocManager::UnregisterDocument(CPossumDocument *doc)
{
    if (m_pActiveDoc == doc)
		m_pActiveDoc = nullptr;

	m_Docs.erase(std::remove(m_Docs.begin(), m_Docs.end(), doc), m_Docs.end());
}

void CPossumDocManager::MakeActiveDocument(CPossumDocument *doc)
{
    m_pActiveDoc = doc;

	if (doc && theApp.m_Player &&
        theApp.m_Player->GetPlayState() != nucleus::IPlayer::State::Playing &&
        !theApp.m_Player->IsTrackSetInPlayStack(doc->GetTrackSet()))
    {
        theApp.m_Player->SetActiveTrackSet(doc->GetTrackSet());

		if (doc->GetView())
            theApp.m_Player->SetPlayPosition(doc->GetView()->GetPlayingTime());
    }
}

CPossumDocument *CPossumDocManager::GetActiveDocument() { return m_pActiveDoc; }

void CPossumDocManager::CloseDocument(const TCHAR *filename)
{
    if (auto *doc = FindDocument(filename))
    {
        if (auto *view = doc->GetView())
            view->GetParentFrame()->SendMessage(WM_CLOSE); // SaveModified + MFC teardown.
        else
            doc->OnCloseDocument();
    }
}

void CPossumDocManager::CloseAllDocuments()
{
    while (!m_Docs.empty())
        m_Docs.front()->OnCloseDocument();
}
