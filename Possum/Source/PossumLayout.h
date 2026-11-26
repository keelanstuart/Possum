#pragma once

class CPossumDocument;

namespace PossumLayout
{
    bool CanApply(CPossumDocument *document);
    bool Apply(CPossumDocument *document, UINT commandId);
}
