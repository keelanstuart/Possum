// source\PossumPropertyGrid.cpp : implementation file
//

#include "stdafx.h"
#include "Possum.h"
#include "PossumPropertyGrid.h"
#include "PossumDocument.h"
#include <NuTrack.h>
#include <assert.h>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <NuCommand.h>

#if 0
class CWTFPropertyGridDateTimeProperty : public CWTFPropertyGridProperty
{
	DECLARE_DYNAMIC(CWTFPropertyGridDateTimeProperty)

// Construction
public:

	CWTFPropertyGridDateTimeProperty(const CString& strName, struct tm &datetime) :
		CWTFPropertyGridProperty(strName, _T("1900/01/01"), NULL, NULL, NULL, NULL, _T("0123456789/-")),
		m_DateTime(datetime)
	{
		TCHAR buf[64];
		errno_t err;

		err = _wctime_s(buf, 64, &m_DateTime);
		SetValue(buf);
		AllowEdit(false);
	}

	virtual ~CWTFPropertyGridDateTimeProperty()
	{
	}

// Overrides
public:
	virtual void OnClickButton(CPoint point)
	{
	}

	virtual BOOL HasButton() { return TRUE; }

// Attributes
protected:
	struct tm m_DateTime;
};

IMPLEMENT_DYNAMIC(CWTFPropertyGridDateTimeProperty, CWTFPropertyGridProperty)
#endif

class CWTFPropertyGridDateProperty : public CWTFPropertyGridProperty
{
	DECLARE_DYNAMIC(CWTFPropertyGridDateProperty)

// Construction
public:

	CWTFPropertyGridDateProperty(const CString& strName, struct tm &datetime) :
		CWTFPropertyGridProperty(strName, _T("1900/01/01"), NULL, NULL, NULL, NULL, _T("0123456789/-")),
		m_Date(datetime)
	{
		TCHAR buf[64];
		_stprintf_s(buf, _T("%04d-%02d-%02d"), m_Date.tm_year + 1900, m_Date.tm_mon + 1, m_Date.tm_mday);
		SetValue(buf);
		AllowEdit(false);
	}

	virtual ~CWTFPropertyGridDateProperty()
	{
	}

// Overrides
public:
	virtual void OnClickButton(CPoint point)
	{
	}

	virtual BOOL HasButton() { return TRUE; }

// Attributes
protected:
	struct tm m_Date;
};

IMPLEMENT_DYNAMIC(CWTFPropertyGridDateProperty, CWTFPropertyGridProperty)


class CWTFPropertyGridTimeProperty : public CWTFPropertyGridProperty
{
	DECLARE_DYNAMIC(CWTFPropertyGridTimeProperty)

// Construction
public:

	CWTFPropertyGridTimeProperty(const CString& strName, struct tm &time) :
		CWTFPropertyGridProperty(strName, _T("00:00:00"), NULL, NULL, NULL, NULL, _T("0123456789:")),
		m_Time(time)
	{
		TCHAR buf[64];
		_stprintf_s(buf, _T("%02d:%02d:%02d"), m_Time.tm_hour, m_Time.tm_min, m_Time.tm_sec);
		SetValue(buf);
		AllowEdit(false);
	}

	virtual ~CWTFPropertyGridTimeProperty()
	{
	}

// Overrides
public:
	virtual void OnClickButton(CPoint point)
	{
	}

	virtual BOOL HasButton() { return TRUE; }

// Attributes
protected:
	struct tm m_Time;
};

IMPLEMENT_DYNAMIC(CWTFPropertyGridTimeProperty, CWTFPropertyGridProperty)


// CPossumPropertyGrid

IMPLEMENT_DYNAMIC(CPossumPropertyGrid, CWTFPropertyGridCtrl)


CPossumPropertyGrid::CPossumPropertyGrid()
{
	m_pPopup = nullptr;
}

CPossumPropertyGrid::~CPossumPropertyGrid()
{
}

CWTFPropertyGridProperty *CPossumPropertyGrid::FindItemByName(const TCHAR *name, CWTFPropertyGridProperty *top)
{
	CWTFPropertyGridProperty *ret = nullptr;

	if (!top)
	{
		for (int i = 0, maxi = GetPropertyCount(); i < maxi; i++)
		{
			CWTFPropertyGridProperty *p = GetProperty(i);
			if (!_tcsicmp(p->GetName(), name))
				return p;

			if (p->GetSubItemsCount() && (nullptr != (ret = FindItemByName(name, p))))
				break;
		}
	}
	else
	{
		for (int i = 0, maxi = top->GetSubItemsCount(); i < maxi; i++)
		{
			CWTFPropertyGridProperty *p = top->GetSubItem(i);
			if (!_tcsicmp(p->GetName(), name))
				return p;

			if (p->GetSubItemsCount() && (nullptr != (ret = FindItemByName(name, p))))
				break;
		}
	}

	return ret;
}

namespace
{
	bool EqualPropertyValue(props::IProperty *a, props::IProperty *b)
	{
		if (!a || !b || a->GetType() != b->GetType() || a->GetAspect() != b->GetAspect())
			return false;

		using Type = props::IProperty::PROPERTY_TYPE;
		switch (a->GetType())
		{
			case Type::PT_STRING:
			case Type::PT_GUID:
			case Type::PT_ENUM:
				return !_tcscmp(a->AsString(), b->AsString());
			case Type::PT_BOOLEAN: return a->AsBool() == b->AsBool();
			case Type::PT_INT: return a->AsInt() == b->AsInt();
			case Type::PT_FLOAT: return a->AsFloat() == b->AsFloat();
			case Type::PT_INT_V2:
			case Type::PT_INT_V3:
			case Type::PT_INT_V4:
			{
				const int count = a->GetType() == Type::PT_INT_V2 ? 2 :
					a->GetType() == Type::PT_INT_V3 ? 3 : 4;
				for (int i = 0; i < count; ++i)
				{
					const int64_t av = count == 2 ? a->AsVec2I()->v[i] :
						count == 3 ? a->AsVec3I()->v[i] : a->AsVec4I()->v[i];
					const int64_t bv = count == 2 ? b->AsVec2I()->v[i] :
						count == 3 ? b->AsVec3I()->v[i] : b->AsVec4I()->v[i];
					if (av != bv) return false;
				}
				return true;
			}
			case Type::PT_FLOAT_V2:
			case Type::PT_FLOAT_V3:
			case Type::PT_FLOAT_V4:
			{
				const int count = a->GetType() == Type::PT_FLOAT_V2 ? 2 :
					a->GetType() == Type::PT_FLOAT_V3 ? 3 : 4;
				for (int i = 0; i < count; ++i)
				{
					const float av = count == 2 ? a->AsVec2F()->v[i] :
						count == 3 ? a->AsVec3F()->v[i] : a->AsVec4F()->v[i];
					const float bv = count == 2 ? b->AsVec2F()->v[i] :
						count == 3 ? b->AsVec3F()->v[i] : b->AsVec4F()->v[i];
					if (av != bv) return false;
				}
				return true;
			}
			default: return false;
		}
	}

	bool CompatibleProperty(props::IProperty *a, props::IProperty *b)
	{
		if (!a || !b || a->GetType() != b->GetType() || a->GetAspect() != b->GetAspect() ||
			_tcscmp(a->GetName(), b->GetName()))
			return false;

		if (a->Flags().IsSet(props::IProperty::PROPFLAG(props::IProperty::EPropFlag::HIDDEN)) ||
			b->Flags().IsSet(props::IProperty::PROPFLAG(props::IProperty::EPropFlag::HIDDEN)))
			return false;

		if (a->GetType() == props::IProperty::PROPERTY_TYPE::PT_ENUM)
		{
			if (a->GetMaxEnumVal() != b->GetMaxEnumVal()) return false;
			for (size_t i = 0; i < a->GetMaxEnumVal(); ++i)
				if (_tcscmp(a->GetEnumString(i), b->GetEnumString(i))) return false;
		}
		return true;
	}

	bool VariantAsFloat(const COleVariant &value, float &result)
	{
		VARIANT converted; VariantInit(&converted);
		const HRESULT hr = VariantChangeType(&converted, const_cast<VARIANT *>(static_cast<const VARIANT *>(&value)), 0, VT_R4);
		if (FAILED(hr)) return false;
		result = converted.fltVal;
		VariantClear(&converted);
		return std::isfinite(result);
	}

	bool VariantAsInt(const COleVariant &value, int64_t &result)
	{
		VARIANT converted; VariantInit(&converted);
		const HRESULT hr = VariantChangeType(&converted, const_cast<VARIANT *>(static_cast<const VARIANT *>(&value)), 0, VT_I8);
		if (FAILED(hr)) return false;
		result = converted.llVal;
		VariantClear(&converted);
		return true;
	}

	// Apply a single edited value, leaving all other fields (particularly each
	// note's individual frequency) unchanged. Vector sub-edits change one axis.
	bool ApplyEditedValue(props::IProperty *target, CWTFPropertyGridProperty *grid,
		CWTFPropertyGridProperty *changed, int changedAxis)
	{
		if (!target || !grid || !changed) return false;
		const COleVariant value = changed->GetValue();
		using Type = props::IProperty::PROPERTY_TYPE;
		switch (target->GetType())
		{
			case Type::PT_STRING:
			case Type::PT_GUID:
			{
				CString s = value;
				target->SetString(s);
				if (target->GetType() == Type::PT_GUID) target->ConvertTo(Type::PT_GUID);
				return true;
			}
			case Type::PT_ENUM:
			{
				CString s = value;
				target->SetEnumValByString(s);
				return true;
			}
			case Type::PT_BOOLEAN:
			{
				CString s = value;
				const TCHAR *yes = target->GetAspect() == props::IProperty::PA_BOOL_YESNO ? _T("Yes") :
					target->GetAspect() == props::IProperty::PA_BOOL_ONOFF ? _T("On") : _T("True");
				target->SetBool(!_tcsicmp(s, yes));
				return true;
			}
			case Type::PT_INT:
			{
				if (target->GetAspect() == props::IProperty::PA_DATE || target->GetAspect() == props::IProperty::PA_TIME)
					return false; // Date/time multi-edit needs separate parsing/validation.
				int64_t n = 0;
				if (!VariantAsInt(value, n)) return false;
				target->SetInt(n);
				return true;
			}
			case Type::PT_FLOAT:
			{
				float f = 0.0f;
				if (!VariantAsFloat(value, f)) return false;
				target->SetFloat(f);
				return true;
			}
			case Type::PT_INT_V2:
			case Type::PT_INT_V3:
			case Type::PT_INT_V4:
			{
				const int count = target->GetType() == Type::PT_INT_V2 ? 2 :
					target->GetType() == Type::PT_INT_V3 ? 3 : 4;
				if (changedAxis < 0 || changedAxis >= count) return false;
				int64_t n = 0;
				if (!VariantAsInt(value, n)) return false;
				if (count == 2) { auto v = *target->AsVec2I(); v.v[changedAxis] = n; target->SetVec2I(v); }
				if (count == 3) { auto v = *target->AsVec3I(); v.v[changedAxis] = n; target->SetVec3I(v); }
				if (count == 4) { auto v = *target->AsVec4I(); v.v[changedAxis] = n; target->SetVec4I(v); }
				return true;
			}
			case Type::PT_FLOAT_V2:
			case Type::PT_FLOAT_V3:
			case Type::PT_FLOAT_V4:
			{
				const int count = target->GetType() == Type::PT_FLOAT_V2 ? 2 :
					target->GetType() == Type::PT_FLOAT_V3 ? 3 : 4;
				if (changedAxis < 0 || changedAxis >= count) return false;
				float f = 0.0f;
				if (!VariantAsFloat(value, f)) return false;
				if (count == 2) { auto v = *target->AsVec2F(); v.v[changedAxis] = f; target->SetVec2F(v); }
				if (count == 3) { auto v = *target->AsVec3F(); v.v[changedAxis] = f; target->SetVec3F(v); }
				if (count == 4) { auto v = *target->AsVec4F(); v.v[changedAxis] = f; target->SetVec4F(v); }
				return true;
			}
			default: return false;
		}
	}
}

void CPossumPropertyGrid::SetActiveProperties(props::IPropertySet *props, bool readonly, const TCHAR *title)
{
	m_MultiDoc = nullptr;
	m_MultiCommands.clear();
	BuildProperties(props, readonly, title);
}

void CPossumPropertyGrid::SetMultipleProperties(CPossumDocument *doc, bool readonly, const TCHAR *title)
{
	m_MultiCommands.clear();
	m_MultiDoc = nullptr;

	if (!doc || doc->GetNumSelected() < 2)
	{
		BuildProperties(nullptr, readonly, title);
		return;
	}

	m_MultiCommands.reserve(doc->GetNumSelected());
	for (size_t i = 0; i < doc->GetNumSelected(); ++i)
		m_MultiCommands.push_back(doc->GetSelection(i));

	m_MultiDoc = doc;
	BuildProperties(m_MultiCommands.front()->GetParams(), readonly, title);
}

bool CPossumPropertyGrid::IsSharedProperty(props::IProperty *first, bool &mixed) const
{
	mixed = false;
	for (size_t i = 1; i < m_MultiCommands.size(); ++i)
	{
		auto *p = m_MultiCommands[i]->GetParams()->GetPropertyById(first->GetID());
		if (!CompatibleProperty(first, p)) return false;
		mixed |= !EqualPropertyValue(first, p);
	}
	return true;
}

bool CPossumPropertyGrid::IsMixedComponent(props::IProperty *first, int axis) const
{
	using Type = props::IProperty::PROPERTY_TYPE;
	const auto readComponent = [axis](props::IProperty *p) -> double
	{
		switch (p->GetType())
		{
			case Type::PT_INT_V2: return double(p->AsVec2I()->v[axis]);
			case Type::PT_INT_V3: return double(p->AsVec3I()->v[axis]);
			case Type::PT_INT_V4: return double(p->AsVec4I()->v[axis]);
			case Type::PT_FLOAT_V2: return double(p->AsVec2F()->v[axis]);
			case Type::PT_FLOAT_V3: return double(p->AsVec3F()->v[axis]);
			case Type::PT_FLOAT_V4: return double(p->AsVec4F()->v[axis]);
			default: return 0.0;
		}
	};
	const double baseline = readComponent(first);
	for (size_t i = 1; i < m_MultiCommands.size(); ++i)
	{
		auto *other = m_MultiCommands[i]->GetParams()->GetPropertyById(first->GetID());
		if (other && readComponent(other) != baseline) return true;
	}
	return false;
}

void CPossumPropertyGrid::BuildProperties(props::IPropertySet *props, bool readonly, const TCHAR *title)
{
	m_Props = props;
	m_ReadOnly = readonly;

	RemoveAll();

	TCHAR tempname[256];
	SetGroupNameFullWidth(TRUE, FALSE);

	for (size_t i = 0, maxi = m_Props ? props->GetPropertyCount() : 0; i < maxi; i++)
	{
		props::IProperty *p = props->GetProperty(i);
		if (p->Flags().IsSet(props::IProperty::PROPFLAG(props::IProperty::EPropFlag::HIDDEN)))
			continue;

		bool mixed = false;
		if (m_MultiCommands.size() > 1 && !IsSharedProperty(p, mixed))
			continue;

		CWTFPropertyGridProperty *pwp = nullptr, *parent_prop = nullptr;

		_tcsncpy_s(tempname, 255, p->GetName(), 255);
		TCHAR *name_start = tempname, *name_end = name_start;
		CString propname;

		while (nullptr != (name_end  = _tcschr(name_start, _T('\\'))))
		{
			*name_end = _T('\0');

			propname = name_start;

			// create a collapsible parent property
			CWTFPropertyGridProperty *tmpprop = FindItemByName(propname);
			if (!tmpprop)
			{
				tmpprop = new CWTFPropertyGridProperty(propname, 0Ui64, FALSE);

				if (parent_prop)
					parent_prop->AddSubItem(tmpprop);
				else
					AddProperty(tmpprop);
			}

			parent_prop = tmpprop;

			// move past it
			name_end++;
			name_start = name_end;
		}

		propname = name_start;
		if (mixed)
			propname += _T(" (mixed)"); // Preserve a typed editor and identify nonuniform values.

		switch (p->GetType())
		{
			case props::IProperty::PROPERTY_TYPE::PT_STRING:
			case props::IProperty::PROPERTY_TYPE::PT_GUID:
			{
				switch (p->GetAspect())
				{
					case props::IProperty::PROPERTY_ASPECT::PA_FILENAME:
					{
						pwp = new CWTFPropertyGridFileProperty(propname, TRUE, CString(p->AsString()), _T(""), 0, _T("*.*"), _T("A File"));
						break;
					}

					case props::IProperty::PROPERTY_ASPECT::PA_DIRECTORY:
					{
						pwp = new CWTFPropertyGridFileProperty(propname, CString(p->AsString()), 0, _T("A Directory"));
						break;
					}

					case props::IProperty::PROPERTY_ASPECT::PA_DATE:
					{
						time_t tt = p->AsInt();
						struct tm _tt;
						gmtime_s(&_tt, &tt);
						//pwp = new CWTFPropertyGridDateProperty(propname, p->AsString(), _T("The date in yyyy/mm/dd format"), 0Ui64, _T("01234567689-/"));//, _T("%04d/%02d/%02d"));
						pwp = new CWTFPropertyGridDateProperty(propname, _tt);
						break;
					}

					case props::IProperty::PROPERTY_ASPECT::PA_TIME:
					{
						time_t tt = p->AsInt();
						struct tm _tt;
						gmtime_s(&_tt, &tt);
						//pwp = new CWTFPropertyGridProperty(propname, p->AsString(), _T("The time in hh:mm:ss format"), 0Ui64, _T("01234567689:"));//, _T("%02d:%02d:%02d"));
						pwp = new CWTFPropertyGridTimeProperty(propname, _tt);
						break;
					}

					case props::IProperty::PROPERTY_ASPECT::PA_FONT_DESC:
					{
						//pwp = new CWTFPropertyGridFontProperty(propname, CString(p->AsString()), 0, _T("A Font"));
						//break;
					}

					default:
					{
						pwp = new CWTFPropertyGridProperty(propname, mixed ? _T("<multiple>") : p->AsString());
						break;
					}
				}
				break;
			}

			case props::IProperty::PROPERTY_TYPE::PT_BOOLEAN:
			{
				const TCHAR *truestr, *falsestr;
				switch (p->GetAspect())
				{
					case props::IProperty::PROPERTY_ASPECT::PA_BOOL_YESNO:
						truestr = _T("Yes"); falsestr = _T("No");
						break;

					case props::IProperty::PROPERTY_ASPECT::PA_BOOL_ONOFF:
						truestr = _T("On"); falsestr = _T("Off");
						break;

					default:
						 truestr = _T("True"); falsestr = _T("False");
						 break;
				}

				pwp = new CWTFPropertyGridProperty(propname, mixed ? _T("<multiple>") : (p->AsBool() ? truestr : falsestr), _T(""));
				if (pwp)
				{
					pwp->AddOption(falsestr);
					pwp->AddOption(truestr);
					pwp->AllowEdit(false);
				}
				break;
			}

			case props::IProperty::PROPERTY_TYPE::PT_ENUM:
			{
				pwp = new CWTFPropertyGridProperty(propname, mixed ? _T("<multiple>") : p->AsString());
				if (pwp)
				{
					for (size_t i = 0, maxi = p->GetMaxEnumVal(); i < maxi; i++)
						pwp->AddOption(p->GetEnumString(i));

					pwp->AllowEdit(false);
				}
				break;
			}

			case props::IProperty::PROPERTY_TYPE::PT_INT:
			{
				switch (p->GetAspect())
				{
					case props::IProperty::PROPERTY_ASPECT::PA_COLOR_RGB:
					case props::IProperty::PROPERTY_ASPECT::PA_COLOR_RGBA:
					{
						UINT r = GetRValue(p->AsInt());
						UINT g = GetGValue(p->AsInt());
						UINT b = GetBValue(p->AsInt());
						pwp = new CWTFPropertyGridColorProperty(propname, RGB(r, g, b));
						((CWTFPropertyGridColorProperty *)pwp)->EnableOtherButton(_T("Other..."));
						((CWTFPropertyGridColorProperty *)pwp)->EnableAutomaticButton(_T("Default"), ::GetSysColor(COLOR_3DFACE));
						break;
					}

					case props::IProperty::PROPERTY_ASPECT::PA_DATE:
					{
						time_t tt = p->AsInt();
						struct tm _tt;
						gmtime_s(&_tt, &tt);

						pwp = new CWTFPropertyGridDateProperty(propname, _tt);
						break;
					}

					case props::IProperty::PROPERTY_ASPECT::PA_TIME:
					{
						time_t tt = p->AsInt();
						struct tm _tt;
						gmtime_s(&_tt, &tt);

						pwp = new CWTFPropertyGridTimeProperty(propname, _tt);
						break;
					}

					default:
					{
						if (mixed)
							pwp = new CWTFPropertyGridProperty(propname, _T("<multiple>"), NULL, NULL, NULL, NULL, _T("0123456789-"));
						else
							pwp = new CWTFPropertyGridProperty(propname, (LONG)(p->AsInt()), NULL, NULL, NULL, NULL, _T("0123456789-"));
						break;
					}
				}
				break;
			}

			case props::IProperty::PROPERTY_TYPE::PT_INT_V2:
			case props::IProperty::PROPERTY_TYPE::PT_INT_V3:
			case props::IProperty::PROPERTY_TYPE::PT_INT_V4:
			{
				pwp = new CWTFPropertyGridProperty(propname, 0, TRUE);

				const TCHAR *xname = _T("x"), *yname = _T("y"), *zname = _T("z"), *wname = _T("w");
				switch (p->GetAspect())
				{
					case props::IProperty::PROPERTY_ASPECT::PA_LATLON:
						xname = _T("Longitude");
						yname = _T("Lattitude");
						zname = _T("Altitude");
						break;
				}

				const int components = p->GetType() == props::IProperty::PROPERTY_TYPE::PT_INT_V2 ? 2 :
					p->GetType() == props::IProperty::PROPERTY_TYPE::PT_INT_V3 ? 3 : 4;
				const TCHAR *names[] = {xname, yname, zname, wname};
				for (int axis = 0; axis < components; ++axis)
				{
					const int64_t value = components == 2 ? p->AsVec2I()->v[axis] :
						components == 3 ? p->AsVec3I()->v[axis] : p->AsVec4I()->v[axis];
					CWTFPropertyGridProperty *part = nullptr;
					if (m_MultiCommands.size() > 1 && IsMixedComponent(p, axis))
						part = new CWTFPropertyGridProperty(names[axis], _T("<multiple>"), NULL, NULL, NULL, NULL, _T("0123456789-"));
					else
						part = new CWTFPropertyGridProperty(names[axis], (LONG)value, NULL, NULL, NULL, NULL, _T("0123456789-"));
					pwp->AddSubItem(part);
				}
				break;
			}

			case props::IProperty::PROPERTY_TYPE::PT_FLOAT:
			{
				if (mixed)
					pwp = new CWTFPropertyGridProperty(propname, _T("<multiple>"), NULL, NULL, NULL, NULL, _T("0123456789.-"));
				else
					pwp = new CWTFPropertyGridProperty(propname, p->AsFloat(), NULL, NULL, NULL, NULL, _T("0123456789.-"));
				break;
			}

			case props::IProperty::PROPERTY_TYPE::PT_FLOAT_V2:
			case props::IProperty::PROPERTY_TYPE::PT_FLOAT_V3:
			case props::IProperty::PROPERTY_TYPE::PT_FLOAT_V4:
			{
				if ((props::IProperty::PROPERTY_TYPE::PT_FLOAT_V3 == p->GetType()) && (props::IProperty::PROPERTY_ASPECT::PA_COLOR_RGB == p->GetAspect()))
				{
					pwp = new CWTFPropertyGridColorSlidersProperty(propname, RGB(255, 255, 255));
				}
				else if ((props::IProperty::PROPERTY_TYPE::PT_FLOAT_V4 == p->GetType()) && (props::IProperty::PROPERTY_ASPECT::PA_COLOR_RGBA == p->GetAspect()))
				{
					pwp = new CWTFPropertyGridColorSlidersProperty(propname, RGB(255, 255, 255));
				}
				else
				{
					pwp = new CWTFPropertyGridProperty(propname, 0, TRUE);

					const TCHAR *xname = _T("x"), *yname = _T("y"), *zname = _T("z"), *wname = _T("w");
					if (p->GetAspect() == props::IProperty::PROPERTY_ASPECT::PA_LATLON)
					{
						xname = _T("Longitude"); yname = _T("Lattitude"); zname = _T("Altitude");
					}
					else if (p->GetAspect() == props::IProperty::PROPERTY_ASPECT::PA_ELEVAZIM)
					{
						xname = _T("Azimuth"); yname = _T("Elevation");
					}
					else if (p->GetAspect() == props::IProperty::PROPERTY_ASPECT::PA_RASCDEC)
					{
						xname = _T("Right Asecnsion"); yname = _T("Declination");
					}

					const int components = p->GetType() == props::IProperty::PROPERTY_TYPE::PT_FLOAT_V2 ? 2 :
						p->GetType() == props::IProperty::PROPERTY_TYPE::PT_FLOAT_V3 ? 3 : 4;
					const TCHAR *names[] = {xname, yname, zname, wname};
					for (int axis = 0; axis < components; ++axis)
					{
						const float value = components == 2 ? p->AsVec2F()->v[axis] :
							components == 3 ? p->AsVec3F()->v[axis] : p->AsVec4F()->v[axis];
						CWTFPropertyGridProperty *part = nullptr;
						if (m_MultiCommands.size() > 1 && IsMixedComponent(p, axis))
							part = new CWTFPropertyGridProperty(names[axis], _T("<multiple>"), NULL, NULL, NULL, NULL, _T("0123456789.-"));
						else
							part = new CWTFPropertyGridProperty(names[axis], value, NULL, NULL, NULL, NULL, _T("0123456789.-"));
						pwp->AddSubItem(part);
					}
				}

				break;
			}
		}

		if (pwp)
		{
			const bool supported = !m_MultiCommands.size() ||
				((p->GetType() != props::IProperty::PROPERTY_TYPE::PT_INT ||
					(p->GetAspect() != props::IProperty::PA_DATE && p->GetAspect() != props::IProperty::PA_TIME)) &&
				 (p->GetType() != props::IProperty::PROPERTY_TYPE::PT_STRING ||
					(p->GetAspect() != props::IProperty::PA_DATE && p->GetAspect() != props::IProperty::PA_TIME)));
			pwp->AllowEdit(!readonly && supported);
			pwp->SetData(p->GetID());
			if (!parent_prop)
				AddProperty(pwp, FALSE, FALSE);
			else
				parent_prop->AddSubItem(pwp);
		}
	}

	ExpandAll();

	AdjustLayout();
//	RedrawWindow();
}


BEGIN_MESSAGE_MAP(CPossumPropertyGrid, CWTFPropertyGridCtrl)
	ON_WM_CREATE()
	ON_MESSAGE(WM_APP + 227, &CPossumPropertyGrid::OnRebuildMultiProperties)
END_MESSAGE_MAP()



// CPossumPropertyGrid message handlers




void CPossumPropertyGrid::OnMultiplePropertyChanged(CWTFPropertyGridProperty *changed)
{
	if (!m_MultiDoc || m_MultiCommands.size() < 2 || !changed || m_ReadOnly ||
		theApp.m_DocMan.GetActiveDocument() != m_MultiDoc)
		return;

	// Reject structural edits during playback, just like the timeline editor.
	if (theApp.m_Player && theApp.m_Player->GetPlayState() != nucleus::IPlayer::State::Stopped)
	{
		PostMessage(WM_APP + 227);
		return;
	}

	CWTFPropertyGridCtrl::OnPropertyChanged(changed);

	CWTFPropertyGridProperty *grid = changed;
	int axis = -1;
	props::IProperty *first = m_Props ? m_Props->GetPropertyById((uint32_t)grid->GetData()) : nullptr;
	if (!first && changed->GetParent())
	{
		grid = changed->GetParent();
		first = m_Props->GetPropertyById((uint32_t)grid->GetData());
		for (int i = 0; i < grid->GetSubItemsCount(); ++i)
			if (grid->GetSubItem(i) == changed) { axis = i; break; }
	}
	if (!first) return;

	bool mixed = false;
	if (!IsSharedProperty(first, mixed)) return;

	// Validate durations before changing ANY selected command. A single edit
	// must either apply to the whole selection or to none of it.
	if (first->GetID() == 'DURA' && first->GetType() == props::IProperty::PROPERTY_TYPE::PT_FLOAT)
	{
		float proposed = 0.0f;
		if (!VariantAsFloat(changed->GetValue(), proposed) || proposed < 0.005f)
		{
			::MessageBeep(MB_ICONWARNING);
			PostMessage(WM_APP + 227);
			return;
		}
		for (auto *cmd : m_MultiCommands)
		{
			double time = 0.0;
			auto *track = const_cast<nucleus::ITrack *>(cmd->GetOwner(&time));
			if (!track || !track->CanFit(time, proposed, cmd, nullptr))
			{
				::MessageBeep(MB_ICONWARNING);
				PostMessage(WM_APP + 227);
				return;
			}
		}
	}

	auto &undo = m_MultiDoc->UndoHistory();
	undo.Begin(_T("Edit Selected Properties"));
	bool any = false;
	for (auto *cmd : m_MultiCommands)
	{
		auto *params = cmd->GetParams();
		auto *target = params->GetPropertyById(first->GetID());
		if (!CompatibleProperty(first, target)) continue;

		auto before = CUndoManager::Snapshot(params);
		if (ApplyEditedValue(target, grid, changed, axis))
		{
			undo.RecordProperties(params, before, CUndoManager::Snapshot(params));
			any = true;
		}
	}

	if (any)
	{
		undo.Commit();
		m_MultiDoc->SetModifiedFlag();
		theApp.RedrawActiveTimeline();
		theApp.RefreshCommandList(false);
	}
	else
		undo.Cancel();

	// A fresh grid shows which properties are *still* mixed after editing.
	// Defer rebuilding until after the property editor releases 'changed'.
	PostMessage(WM_APP + 227);
}

void CPossumPropertyGrid::OnPropertyChanged(CWTFPropertyGridProperty* pProp)
{
	if (m_MultiDoc && m_MultiCommands.size() > 1)
	{
		OnMultiplePropertyChanged(pProp);
		return;
	}

	if (!m_Props)
		return;

	// Only changes belonging to the active timeline participate in document undo.
	auto *doc = theApp.m_DocMan.GetActiveDocument();
	bool timelineProperty = false;
	if (doc && doc->GetTrackSet())
	{
		auto *ts = doc->GetTrackSet();
		timelineProperty = (ts->GetProperties() == m_Props);
		// The common case is editing the one selected command. Avoid a full
		// scan through thousands of notes just to recognize its property set.
		if (!timelineProperty && doc->GetNumSelected() == 1)
			timelineProperty = (doc->GetSelection(0)->GetParams() == m_Props);
		for (size_t row = 0; !timelineProperty && row < ts->GetNumTracks(); ++row)
		{
			auto *track = ts->GetTrack(row);
			timelineProperty = track->GetProperties() == m_Props;
			for (size_t i = 0; !timelineProperty && i < track->GetNumCommands(); ++i)
				timelineProperty = track->GetCommand(i)->GetParams() == m_Props;
		}
	}
	auto *targetProperties = m_Props;
	auto before = timelineProperty ? CUndoManager::Snapshot(targetProperties) : CUndoManager::PropertySnapshot{};

	CWTFPropertyGridCtrl::OnPropertyChanged(pProp);

	uint32_t id = (uint32_t)pProp->GetData();

	props::IProperty *p = m_Props->GetPropertyById(id);
	if (!p && pProp->GetParent())
	{
		pProp = pProp->GetParent();
		id = (uint32_t)pProp->GetData();
		p = m_Props->GetPropertyById(id);
	}

	if (p)
	{
		COleVariant var = pProp->GetValue();

		switch (p->GetType())
		{
			case props::IProperty::PROPERTY_TYPE::PT_STRING:
			{
				CString propstr = var;
				p->SetString(propstr);
				break;
			}

			case props::IProperty::PROPERTY_TYPE::PT_INT:
			{
				switch (p->GetAspect())
				{
					case props::IProperty::PA_DATE:
					{
						CString propstr = var;
						struct tm t;
						memset(&t, 0, sizeof(struct tm));
						_stscanf_s((LPCTSTR)propstr, _T("%04d/%02d/%02d"), &t.tm_year, &t.tm_mon, &t.tm_mday);
						t.tm_mon--;
						p->SetInt(mktime(&t));
						break;
					}

					case props::IProperty::PA_TIME:
					{
						CString propstr = var;
						struct tm t;
						memset(&t, 0, sizeof(struct tm));
						_stscanf_s((LPCTSTR)propstr, _T("%02d:%02d:%02d"), &t.tm_hour, &t.tm_min, &t.tm_sec);
						p->SetInt(mktime(&t));
						break;
					}

					default:
						p->SetInt(var.llVal);
						break;
				}
				break;
			}

			case props::IProperty::PROPERTY_TYPE::PT_FLOAT:
			{
				p->SetFloat(var.fltVal);
				break;
			}

			case props::IProperty::PROPERTY_TYPE::PT_GUID:
			{
				CString propstr = var;
				p->SetString(propstr);
				p->ConvertTo(props::IProperty::PROPERTY_TYPE::PT_GUID);
				break;
			}

			case props::IProperty::PROPERTY_TYPE::PT_BOOLEAN:
			{
				const TCHAR *truestr;
				switch (p->GetAspect())
				{
					case props::IProperty::PROPERTY_ASPECT::PA_BOOL_YESNO: truestr = _T("Yes"); break;
					case props::IProperty::PROPERTY_ASPECT::PA_BOOL_ONOFF: truestr = _T("On"); break;
					default: truestr = _T("True"); break;
				}
				CString propstr = var;
				p->SetBool(!_tcsicmp(propstr, truestr) ? true : false);
				break;
			}

			case props::IProperty::PROPERTY_TYPE::PT_INT_V2:
			{
				assert(pProp->GetSubItemsCount() == 2);
				props::TVec2I v2;
				for (int i = 0, maxi = pProp->GetSubItemsCount(); i < maxi; i++)
					v2.v[i] = pProp->GetSubItem(i)->GetValue().llVal;
				p->SetVec2I(v2);
				break;
			}

			case props::IProperty::PROPERTY_TYPE::PT_INT_V3:
			{
				assert(pProp->GetSubItemsCount() == 3);
				props::TVec3I v3;
				for (int i = 0, maxi = pProp->GetSubItemsCount(); i < maxi; i++)
					v3.v[i] = pProp->GetSubItem(i)->GetValue().llVal;
				p->SetVec3I(v3);
				break;
			}

			case props::IProperty::PROPERTY_TYPE::PT_INT_V4:
			{
				assert(pProp->GetSubItemsCount() == 4);
				props::TVec4I v4;
				for (int i = 0, maxi = pProp->GetSubItemsCount(); i < maxi; i++)
					v4.v[i] = pProp->GetSubItem(i)->GetValue().llVal;
				p->SetVec4I(v4);
				break;
			}

			case props::IProperty::PROPERTY_TYPE::PT_FLOAT_V2:
			{
				assert(pProp->GetSubItemsCount() == 2);
				props::TVec2F v2;
				for (int i = 0, maxi = pProp->GetSubItemsCount(); i < maxi; i++)
					v2.v[i] = pProp->GetSubItem(i)->GetValue().fltVal;
				p->SetVec2F(v2);
				break;
			}

			case props::IProperty::PROPERTY_TYPE::PT_FLOAT_V3:
			{
				if (p->GetAspect() != props::IProperty::PA_COLOR_RGBA)
				{
					assert(pProp->GetSubItemsCount() == 3);
					props::TVec3F v3;
					for (int i = 0, maxi = pProp->GetSubItemsCount(); i < maxi; i++)
						v3.v[i] = pProp->GetSubItem(i)->GetValue().fltVal;
					p->SetVec3F(v3);
				}
				else
				{
				}
				break;
			}

			case props::IProperty::PROPERTY_TYPE::PT_FLOAT_V4:
			{
				if (p->GetAspect() != props::IProperty::PA_COLOR_RGBA)
				{
					assert(pProp->GetSubItemsCount() == 4);
					props::TVec4F v4;
					for (int i = 0, maxi = pProp->GetSubItemsCount(); i < maxi; i++)
						v4.v[i] = pProp->GetSubItem(i)->GetValue().fltVal;
					p->SetVec4F(v4);
				}
				else
				{
				}
				break;
			}

			case props::IProperty::PROPERTY_TYPE::PT_ENUM:
			{
				CString propstr = var;
				p->SetEnumValByString(propstr);

				// if the property has a dynamic data provider, then we need to refresh...
				// some properties are dependant upon others. :/
				if (p->GetEnumProvider())
				{
					SetActiveProperties(m_Props);
				}
				break;
			}
		}
	}

	if (timelineProperty && doc && before)
	{
		auto &undo = doc->UndoHistory();
		undo.Begin(_T("Edit Property"));
		undo.RecordProperties(targetProperties, before, CUndoManager::Snapshot(targetProperties));
		undo.Commit();
	}
	theApp.RedrawActiveTimeline();
	if (doc) doc->SetModifiedFlag();
}


int CPossumPropertyGrid::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWTFPropertyGridCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_brBackground.CreateSolidBrush(RGB(40, 40, 40));

//	m_bControlBarColors = false;
	m_bDescriptionArea = false;
	m_bGroupNameFullWidth = true;
	m_nHeaderHeight = 55;
	m_nRowHeight = 55;
	m_clrBackground = RGB(40, 40, 40);
	m_clrGroupBackground = RGB(40, 40, 40);
	m_clrDescriptionBackground = RGB(45, 45, 45);
	m_clrLine = RGB(40, 40, 40);
	m_clrDescriptionText = RGB(255, 255, 255);
	m_clrText = RGB(255, 255, 255);
	m_clrGroupText = RGB(255, 255, 255);
	m_clrGray = RGB(128, 128, 128);

	return 0;
}

LRESULT CPossumPropertyGrid::OnRebuildMultiProperties(WPARAM, LPARAM)
{
	if (m_MultiDoc && theApp.m_DocMan.GetActiveDocument() == m_MultiDoc &&
		m_MultiDoc->GetNumSelected() > 1)
		SetMultipleProperties(m_MultiDoc, m_ReadOnly, _T("[multiple selected]"));
	return 0;
}
