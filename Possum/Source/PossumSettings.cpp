#include "stdafx.h"

#include "PossumSettings.h"


props::IProperty *Settings::SetParam(props::IPropertySet *propset, const TCHAR *name, uint32_t FCC, int64_t ival, bool force,
						props::IProperty::PROPERTY_ASPECT aspect, uint32_t flags)
{
	props::IProperty *pp = propset->GetPropertyById(FCC);
	if ((pp == nullptr) || force || (nullptr != (pp = propset->CreateProperty(name, FCC))))
	{
		pp->SetInt(ival);
		pp->SetAspect(aspect);
		pp->Flags().SetAll(flags);
		pp->SetAspect(aspect);
	}
	return pp;
}

props::IProperty *SetParam(props::IPropertySet *propset, const TCHAR *name, uint32_t FCC, bool bval, bool force,
			  props::IProperty::PROPERTY_ASPECT aspect, uint32_t flags)
{
	props::IProperty *pp = propset->GetPropertyById(FCC);
	if ((pp == nullptr) || force || (nullptr != (pp = propset->CreateProperty(name, FCC))))
	{
		pp->SetBool(bval);
		pp->SetAspect(aspect);
		pp->Flags().SetAll(flags);
		pp->SetAspect(aspect);
	}
	return pp;
}

props::IProperty *SetParam(props::IPropertySet *propset, const TCHAR *name, uint32_t FCC, const TCHAR *sval, bool force,
			  props::IProperty::PROPERTY_ASPECT aspect, uint32_t flags)
{
	props::IProperty *pp = propset->GetPropertyById(FCC);
	if ((pp == nullptr) || force || (nullptr != (pp = propset->CreateProperty(name, FCC))))
	{
		pp->SetString(sval);
		pp->SetAspect(aspect);
		pp->Flags().SetAll(flags);
		pp->SetAspect(aspect);
	}
	return pp;
}

props::IProperty *SetParam(props::IPropertySet *propset, const TCHAR *name, uint32_t FCC, const TCHAR *esval, size_t eval, bool force,
			  props::IProperty::PROPERTY_ASPECT aspect, uint32_t flags)
{
	props::IProperty *pp = propset->GetPropertyById(FCC);
	if ((pp == nullptr) || force || (nullptr != (pp = propset->CreateProperty(name, FCC))))
	{
		pp->SetEnumStrings(esval);
		pp->SetEnumVal(eval);
		pp->SetAspect(aspect);
		pp->Flags().SetAll(flags);
		pp->SetAspect(aspect);
	}
	return pp;
}

props::IProperty *SetParam(props::IPropertySet *propset, const TCHAR *name, uint32_t FCC, float fval, bool force,
			  props::IProperty::PROPERTY_ASPECT aspect, uint32_t flags)
{
	props::IProperty *pp = propset->GetPropertyById(FCC);
	if ((pp == nullptr) || force || (nullptr != (pp = propset->CreateProperty(name, FCC))))
	{
		pp->SetFloat(fval);
		pp->SetAspect(aspect);
		pp->Flags().SetAll(flags);
		pp->SetAspect(aspect);
	}
	return pp;
}

props::IProperty *SetParam(props::IPropertySet *propset, const TCHAR *name, uint32_t FCC, props::TVec2F v2fval, bool force,
			  props::IProperty::PROPERTY_ASPECT aspect, uint32_t flags)
{
	props::IProperty *pp = propset->GetPropertyById(FCC);
	if ((pp == nullptr) || force || (nullptr != (pp = propset->CreateProperty(name, FCC))))
	{
		pp->SetVec2F(v2fval);
		pp->SetAspect(aspect);
		pp->Flags().SetAll(flags);
		pp->SetAspect(aspect);
	}
	return pp;
}
