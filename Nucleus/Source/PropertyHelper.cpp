#include "stdafx.h"
#include "PropertyHelper.h"

bool LoadProperty(props::IPropertySet *propset, tinyxml2::XMLElement *prop_root)
{
	if (!prop_root)
		return false;

	const tinyxml2::XMLAttribute *pname = prop_root->FindAttribute("name");
	const tinyxml2::XMLAttribute *ptype = prop_root->FindAttribute("type");
	const tinyxml2::XMLAttribute *paspect = prop_root->FindAttribute("aspect");
	const tinyxml2::XMLAttribute *pid = prop_root->FindAttribute("fcc");

	// we must have a name, type, and id --- aspect can be generic (in which case it may be absent)
	if (!pname || !ptype || !pid)
		return false;

	TCHAR *_pname, *_ptype, *_paspect, *_pid;
	LOCAL_MBCS2TCS(pname->Value(), _pname);
	LOCAL_MBCS2TCS(ptype->Value(), _ptype);
	LOCAL_MBCS2TCS(pid->Value(), _pid);
	if (paspect)
	{
		LOCAL_MBCS2TCS(paspect->Value(), _paspect);
	}
	else
	{
		_paspect = _T("generic");
	}

	union { props::FOURCHARCODE fcc; char _fcc[4]; } id;
	ZeroMemory(&id, sizeof(props::FOURCHARCODE));
	size_t i = 3;
	while (*_pid)
	{
		id._fcc[i] = (char)(*_pid);
		if (!i)
			break;
		i--;
		_pid++;
	}
	
	props::IProperty *p = propset->GetPropertyByName(_pname);

	if (!p)
		p = propset->CreateProperty(_pname, id.fcc);

	if (!p)
		return false;

	props::IProperty::PROPERTY_TYPE pt;
	if (!_tcsicmp(_ptype, _T("bool")))
		pt = props::IProperty::PT_BOOLEAN;
	else if (!_tcsicmp(_ptype, _T("int")))
		pt = props::IProperty::PT_INT;
	else if (!_tcsicmp(_ptype, _T("int2")))
		pt = props::IProperty::PT_INT_V2;
	else if (!_tcsicmp(_ptype, _T("int3")))
		pt = props::IProperty::PT_INT_V3;
	else if (!_tcsicmp(_ptype, _T("int4")))
		pt = props::IProperty::PT_INT_V4;
	else if (!_tcsicmp(_ptype, _T("float")))
		pt = props::IProperty::PT_FLOAT;
	else if (!_tcsicmp(_ptype, _T("float2")))
		pt = props::IProperty::PT_FLOAT_V2;
	else if (!_tcsicmp(_ptype, _T("float3")))
		pt = props::IProperty::PT_FLOAT_V3;
	else if (!_tcsicmp(_ptype, _T("float4")))
		pt = props::IProperty::PT_FLOAT_V4;
	else if (!_tcsicmp(_ptype, _T("string")))
		pt = props::IProperty::PT_STRING;
	else if (!_tcsicmp(_ptype, _T("guid")))
		pt = props::IProperty::PT_GUID;
	else if (!_tcsicmp(_ptype, _T("enum")))
		pt = props::IProperty::PT_ENUM;
	else
		return false;

	props::IProperty::PROPERTY_ASPECT pa;
	if (!_tcsicmp(_paspect, _T("generic")))
		pa = props::IProperty::PA_GENERIC;
	else if (!_tcsicmp(_paspect, _T("onoff")))
		pa = props::IProperty::PA_BOOL_ONOFF;
	else if (!_tcsicmp(_paspect, _T("yesno")))
		pa = props::IProperty::PA_BOOL_YESNO;
	else if (!_tcsicmp(_paspect, _T("filename")))
		pa = props::IProperty::PA_FILENAME;
	else if (!_tcsicmp(_paspect, _T("directory")))
		pa = props::IProperty::PA_DIRECTORY;
	else if (!_tcsicmp(_paspect, _T("rgb")))
		pa = props::IProperty::PA_COLOR_RGB;
	else if (!_tcsicmp(_paspect, _T("rgba")))
		pa = props::IProperty::PA_COLOR_RGBA;
	else if (!_tcsicmp(_paspect, _T("latlon")))
		pa = props::IProperty::PA_LATLON;
	else if (!_tcsicmp(_paspect, _T("alevazim")))
		pa = props::IProperty::PA_ELEVAZIM;
	else if (!_tcsicmp(_paspect, _T("rascdec")))
		pa = props::IProperty::PA_RASCDEC;
	else if (!_tcsicmp(_paspect, _T("quaternion")))
		pa = props::IProperty::PA_QUATERNION;
	else if (!_tcsicmp(_paspect, _T("fontdesc")))
		pa = props::IProperty::PA_FONT_DESC;
	else if (!_tcsicmp(_paspect, _T("date")))
		pa = props::IProperty::PA_DATE;
	else if (!_tcsicmp(_paspect, _T("time")))
		pa = props::IProperty::PA_TIME;
	else if (!_tcsicmp(_paspect, _T("ipaddress")))
		pa = props::IProperty::PA_IPADDRESS;
	else
		return false;

	TCHAR *s;
	const char *rt = prop_root->GetText();
	if (!rt)
		rt = "";

	LOCAL_MBCS2TCS(rt, s);

	if (pt != props::IProperty::PT_ENUM)
	{
		p->SetString(s);

		if (pt != props::IProperty::PT_STRING)
			p->ConvertTo(pt);
	}
	else
	{
		p->SetEnumStrings(s);
		p->SetEnumVal(prop_root->IntAttribute("enumval"));
	}

	p->SetAspect(pa);

	return true;
}

bool LoadPropertySet(props::IPropertySet *propset, tinyxml2::XMLNode *propset_root)
{
	if (!propset || !propset_root)
		return false;

	tinyxml2::XMLElement *pe = propset_root->FirstChildElement("property");
	while (pe)
	{
		if (!LoadProperty(propset, pe))
			return false;

		pe = pe->NextSiblingElement("property");
	}

	return true;
}


bool SavePropertySet(const props::IPropertySet *propset, genio::ITextOutput *pto)
{
	if (!propset || !pto)
		return false;

	pto->Print(_T("<properties>"));
	pto->IncIndent();

	TCHAR sv[4096];

	for (size_t i = 0, maxi = propset->GetPropertyCount(); i < maxi; i++)
	{
		props::IProperty *p = propset->GetProperty(i);
		if (p->Flags().IsSet(props::IProperty::PROPFLAG(props::IProperty::EPropFlag::HIDDEN)))
			continue;

		pto->NextLine();

		TCHAR se[32] = _T("");
		if (p->GetType() == props::IProperty::PT_ENUM)
			_stprintf_s(se, 32, _T(" enumval=\"%I64u\""), p->AsInt());

		TCHAR *st = _T("");
		switch (p->GetType())
		{
			case props::IProperty::PT_BOOLEAN:		st = _T("bool");	break;
			case props::IProperty::PT_INT:			st = _T("int");		break;
			case props::IProperty::PT_INT_V2:		st = _T("int2");	break;
			case props::IProperty::PT_INT_V3:		st = _T("int3");	break;
			case props::IProperty::PT_INT_V4:		st = _T("int4");	break;
			case props::IProperty::PT_FLOAT:		st = _T("float");	break;
			case props::IProperty::PT_FLOAT_V2:		st = _T("float2");	break;
			case props::IProperty::PT_FLOAT_V3:		st = _T("float3");	break;
			case props::IProperty::PT_FLOAT_V4:		st = _T("float4");	break;
			case props::IProperty::PT_STRING:		st = _T("string");	break;
			case props::IProperty::PT_GUID:			st = _T("guid");	break;
			case props::IProperty::PT_ENUM:			st = _T("enum");	break;
		}

		TCHAR *sa = _T("");
		switch (p->GetAspect())
		{
			case props::IProperty::PA_GENERIC:		sa =  _T("generic");	break;
			case props::IProperty::PA_BOOL_ONOFF:	sa =  _T("onoff");		break;
			case props::IProperty::PA_BOOL_YESNO:	sa =  _T("yesno");		break;
			case props::IProperty::PA_FILENAME:		sa =  _T("filename");	break;
			case props::IProperty::PA_DIRECTORY:	sa =  _T("directory");	break;
			case props::IProperty::PA_COLOR_RGB:	sa =  _T("rgb");		break;
			case props::IProperty::PA_COLOR_RGBA:	sa =  _T("rgba");		break;
			case props::IProperty::PA_LATLON:		sa =  _T("latlon");		break;
			case props::IProperty::PA_ELEVAZIM:		sa =  _T("alevazim");	break;
			case props::IProperty::PA_RASCDEC:		sa =  _T("rascdec");	break;
			case props::IProperty::PA_QUATERNION:	sa =  _T("quaternion");	break;
			case props::IProperty::PA_FONT_DESC:	sa =  _T("fontdesc");	break;
			case props::IProperty::PA_DATE:			sa =  _T("date");		break;
			case props::IProperty::PA_TIME:			sa =  _T("time");		break;
			case props::IProperty::PA_IPADDRESS:	sa =  _T("ipaddress");	break;
		}

		TCHAR *si;
		char _si[5] = {0}, *_sip = &_si[0];
		union { props::FOURCHARCODE fcc; char _fcc[4]; } id;
		id.fcc = p->GetID();
		_si[0] = id._fcc[3];
		_si[1] = id._fcc[2];
		_si[2] = id._fcc[1];
		_si[3] = id._fcc[0];
		while (!*_sip) _sip++;
		LOCAL_MBCS2TCS(_sip, si);

		if (p->GetType() != props::IProperty::PT_ENUM)
		{
			p->AsString(sv, 4096);
		}
		else
		{
			p->GetEnumStrings(sv, 4096);
		}

		pto->PrintF(_T("<property name=\"%s\" fcc=\"%s\" type=\"%s\" aspect=\"%s\"%s>%s</property>"), p->GetName(), si, st, sa, se, sv);
	}

	pto->DecIndent();
	pto->NextLine();
	pto->Print(_T("</properties>"));

	return true;
}