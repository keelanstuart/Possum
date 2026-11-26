#pragma once

#include <PowerProps\Include\PowerProps.h>
#include <GenIO\Include\GenIO.h>

bool LoadPropertySet(props::IPropertySet *propset, tinyxml2::XMLNode *propset_root);
bool SavePropertySet(const props::IPropertySet *propset, genio::ITextOutput *pto);
