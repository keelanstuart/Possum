#pragma once
#include "PossumDocument.h"
#include <deque>

// Index of open Nucleus TrackSet documents; MFC owns their lifetimes.
class CPossumDocManager
{
public:
    CPossumDocManager();
    virtual ~CPossumDocManager();
    CPossumDocument *CreateDocument(const TCHAR *filename = nullptr, bool create_default_tracks = true);
    CPossumDocument *FindDocument(const TCHAR *filename);
    void RegisterDocument(CPossumDocument *pdoc);
    void UnregisterDocument(CPossumDocument *pdoc);
    void MakeActiveDocument(CPossumDocument *pdoc);
    CPossumDocument *GetActiveDocument();
    void CloseDocument(const TCHAR *filename);
    void CloseAllDocuments();
protected:
    typedef std::deque<CPossumDocument *> TDocDeque;
    TDocDeque m_Docs;
    CPossumDocument *m_pActiveDoc = nullptr;
};
