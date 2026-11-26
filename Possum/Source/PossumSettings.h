#pragma once

namespace Settings
{
	namespace Document
	{
		void ApplyDefaults();
	};

	namespace Track
	{
		void ApplyDefaults();
	};

	namespace Command
	{
		void ApplyDefaults();
	};


	props::IProperty *SetParam(props::IPropertySet *propset, const TCHAR *name, uint32_t FCC, int64_t ival, bool force = false,
				  props::IProperty::PROPERTY_ASPECT aspect = props::IProperty::PROPERTY_ASPECT::PA_GENERIC, uint32_t flags = 0);

	props::IProperty *SetParam(props::IPropertySet *propset, const TCHAR *name, uint32_t FCC, bool bval, bool force = false,
				  props::IProperty::PROPERTY_ASPECT aspect = props::IProperty::PROPERTY_ASPECT::PA_GENERIC, uint32_t flags = 0);

	props::IProperty *SetParam(props::IPropertySet *propset, const TCHAR *name, uint32_t FCC, const TCHAR *sval, bool force = false,
				  props::IProperty::PROPERTY_ASPECT aspect = props::IProperty::PROPERTY_ASPECT::PA_GENERIC, uint32_t flags = 0);

	props::IProperty *SetParam(props::IPropertySet *propset, const TCHAR *name, uint32_t FCC, const TCHAR *esval, size_t default_eval, bool force = false,
				  props::IProperty::PROPERTY_ASPECT aspect = props::IProperty::PROPERTY_ASPECT::PA_GENERIC, uint32_t flags = 0);

	props::IProperty *SetParam(props::IPropertySet *propset, const TCHAR *name, uint32_t FCC, float fval, bool force = false,
				  props::IProperty::PROPERTY_ASPECT aspect = props::IProperty::PROPERTY_ASPECT::PA_GENERIC, uint32_t flags = 0);
	
	props::IProperty *SetParam(props::IPropertySet *propset, const TCHAR *name, uint32_t FCC, props::TVec2F v2fval, bool force = false,
				  props::IProperty::PROPERTY_ASPECT aspect = props::IProperty::PROPERTY_ASPECT::PA_GENERIC, uint32_t flags = 0);


	int64_t GetParam(props::IPropertySet *propset, const TCHAR *name, uint32_t FCC, int64_t ival, bool force = false,
							   props::IProperty::PROPERTY_ASPECT aspect = props::IProperty::PROPERTY_ASPECT::PA_GENERIC, uint32_t flags = 0);

	bool GetParam(props::IPropertySet *propset, const TCHAR *name, uint32_t FCC, bool bval, bool force = false,
							   props::IProperty::PROPERTY_ASPECT aspect = props::IProperty::PROPERTY_ASPECT::PA_GENERIC, uint32_t flags = 0);

	const TCHAR *GetParam(props::IPropertySet *propset, const TCHAR *name, uint32_t FCC, const TCHAR *sval, bool force = false,
							   props::IProperty::PROPERTY_ASPECT aspect = props::IProperty::PROPERTY_ASPECT::PA_GENERIC, uint32_t flags = 0);

	size_t GetParam(props::IPropertySet *propset, const TCHAR *name, uint32_t FCC, const TCHAR *esval, size_t default_eval, bool force = false,
							   props::IProperty::PROPERTY_ASPECT aspect = props::IProperty::PROPERTY_ASPECT::PA_GENERIC, uint32_t flags = 0);

	float GetParam(props::IPropertySet *propset, const TCHAR *name, uint32_t FCC, float fval, bool force = false,
							   props::IProperty::PROPERTY_ASPECT aspect = props::IProperty::PROPERTY_ASPECT::PA_GENERIC, uint32_t flags = 0);

	props::TVec2F GetParam(props::IPropertySet *propset, const TCHAR *name, uint32_t FCC, props::TVec2F v2fval, bool force = false,
							   props::IProperty::PROPERTY_ASPECT aspect = props::IProperty::PROPERTY_ASPECT::PA_GENERIC, uint32_t flags = 0);
};
