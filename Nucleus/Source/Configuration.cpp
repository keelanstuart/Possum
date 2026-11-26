#include "stdafx.h"

#include "NuConfiguration.h"
#include <assert.h>


class CConfiguration : public nucleus::IConfiguration
{
public:
	CConfiguration(const TCHAR *filename);
	virtual ~CConfiguration();

	virtual void Release();

	virtual bool GetBool(const TCHAR *path, bool def);
	virtual void SetBool(const TCHAR *path, bool val);

	virtual COLORREF GetColor(const TCHAR *path, COLORREF def);
	virtual void SetColor(const TCHAR *path, COLORREF val);

	virtual double GetDouble(const TCHAR *path, double def);
	virtual void SetDouble(const TCHAR *path, double val);

	virtual float GetFloat(const TCHAR *path, float def);
	virtual void SetFloat(const TCHAR *path, float val);

	virtual int64_t GetInt(const TCHAR *path, int64_t def);
	virtual void SetInt(const TCHAR *path, int64_t val);

	virtual bool GetPoint(const TCHAR *path, POINT &val, const POINT *def = nullptr);
	virtual void SetPoint(const TCHAR *path, const POINT &val);

	virtual bool GetRect(const TCHAR *path, RECT &val, const RECT *def = nullptr);
	virtual void SetRect(const TCHAR *path, const RECT &def);

	virtual const TCHAR *GetString(const TCHAR *path, const TCHAR *def);
	virtual void SetString(const TCHAR *path, const TCHAR *val);

	virtual size_t GetNumSubKeys(const TCHAR *path);
	virtual bool GetSubKeyName(const TCHAR *path, size_t subkeyindex, TCHAR *retbuf, size_t retbuflen);

	virtual bool RemoveKey(const TCHAR *path);
	virtual bool RemoveSubKey(const TCHAR *path, size_t subkeyindex);

protected:
	tstring m_sFilename;

	tinyxml2::XMLDocument m_ConfigDoc;

	tinyxml2::XMLElement *EvaluatePath(const TCHAR *path, bool create, bool &didcreate);

	tinyxml2::XMLAttribute *GetAttribute(const tinyxml2::XMLElement *pel, const TCHAR *name);

	bool IsType(const tinyxml2::XMLElement *pel, const TCHAR *type);

	typedef std::map<tinyxml2::XMLElement *, tstring> TXmlEl2StrMap;
	typedef std::pair<tinyxml2::XMLElement *, tstring> TXmlEl2StrPair;
	TXmlEl2StrMap m_StringMap;
};


nucleus::IConfiguration *nucleus::IConfiguration::Create(const TCHAR *filename)
{
	nucleus::IConfiguration *ret = nullptr;

	if (filename)
	{
		ret = new CConfiguration(filename);
	}

	return ret;
}

CConfiguration::CConfiguration(const TCHAR *filename) :
	m_StringMap()
{
	m_sFilename = filename;

	FILE *f = nullptr;
	if ((_tfopen_s(&f, m_sFilename.c_str(), _T("rb, ccs=UTF-8")) != EINVAL) && f)
	{
		m_ConfigDoc.LoadFile(f);
		fclose(f);
	}
}

CConfiguration::~CConfiguration(void)
{
	FILE *f = nullptr;
	errno_t err = _tfopen_s(&f, m_sFilename.c_str(), _T("wb, ccs=UTF-8"));
	if ((err != EINVAL) && f)
	{
		m_ConfigDoc.SaveFile(f, false);
		fclose(f);
	}
}

void CConfiguration::Release()
{
	delete this;
}

int64_t CConfiguration::GetInt(const TCHAR *path, int64_t def)
{
	bool created;
	tinyxml2::XMLElement *pel = EvaluatePath(path, true, created);

	if (pel)
	{
		if (created)
		{
			pel->SetAttribute("value", def);
			pel->SetAttribute("type", "int");
			return def;
		}

		if (IsType(pel, _T("int")))
		{
			tinyxml2::XMLAttribute *pa = GetAttribute(pel, _T("value"));
			if (pa)
			{
				return pa->Int64Value();
			}
		}
	}

	return def;
}


void CConfiguration::SetInt(const TCHAR *path, int64_t val)
{
	bool created;
	tinyxml2::XMLElement *pel = EvaluatePath(path, true, created);

	if (pel)
	{
		if (created)
		{
			pel->SetAttribute("type", "int");
			pel->SetAttribute("value", val);
			return;
		}

		if (IsType(pel, _T("int")))
		{
			tinyxml2::XMLAttribute *pa = GetAttribute(pel, _T("value"));
			if (pa)
			{
				pa->SetAttribute(val);
			}
		}
	}
}


COLORREF CConfiguration::GetColor(const TCHAR *path, COLORREF def)
{
	bool created;
	tinyxml2::XMLElement *pel = EvaluatePath(path, true, created);

	if (pel)
	{
		if (created)
		{
			char s[32];
			def = (def & 0xff00ff00) | ((def & 0x00ff0000) >> 16) | ((def & 0x000000ff) << 16);
			sprintf_s(s, sizeof(s), "#%08x", def);

			pel->SetAttribute("value", s);

			pel->SetAttribute("type", "color");
		}

		if (IsType(pel, _T("color")))
		{
			tinyxml2::XMLAttribute *pa = GetAttribute(pel, _T("value"));
			if (pa)
			{
				COLORREF ret;
				sscanf_s(pa->Value(), "#%08x", &ret);

				ret = (ret & 0xff00ff00) | ((ret & 0x00ff0000) >> 16) | ((ret & 0x000000ff) << 16);
				return ret;
			}
		}
	}

	return def;
}


void CConfiguration::SetColor(const TCHAR *path, COLORREF val)
{
	bool created;
	tinyxml2::XMLElement *pel = EvaluatePath(path, true, created);

	if (pel)
	{
		if (created)
		{
			pel->SetAttribute("type", "color");
			pel->SetAttribute("value", "#00000000");
		}

		if (IsType(pel, _T("color")))
		{
			tinyxml2::XMLAttribute *pa = GetAttribute(pel, _T("value"));
			if (pa)
			{
				char s[32];
				val = (val & 0xff00ff00) | ((val & 0x00ff0000) >> 16) | ((val & 0x000000ff) << 16);
				sprintf_s(s, sizeof(s), "#%08x", val);

				pa->SetAttribute(s);
			}
		}
	}
}


double CConfiguration::GetDouble(const TCHAR *path, double def)
{
	bool created;
	tinyxml2::XMLElement *pel = EvaluatePath(path, true, created);

	if (pel)
	{
		if (created)
		{
			pel->SetAttribute("type", "double");
			pel->SetAttribute("value", def);
			return def;
		}

		if (IsType(pel, _T("double")))
		{
			tinyxml2::XMLAttribute *pa = GetAttribute(pel, _T("value"));
			if (pa)
			{
				return pa->DoubleValue();
			}
		}
	}

	return def;
}


void CConfiguration::SetDouble(const TCHAR *path, double val)
{
	bool created;
	tinyxml2::XMLElement *pel = EvaluatePath(path, true, created);

	if (pel)
	{
		if (created)
		{
			pel->SetAttribute("value", "0.0");
			pel->SetAttribute("type", "double");
			return;
		}

		if (IsType(pel, _T("double")))
		{
			tinyxml2::XMLAttribute *pa = GetAttribute(pel, _T("value"));
			if (pa)
			{
				pa->SetAttribute(val);
			}
		}
	}
}


float CConfiguration::GetFloat(const TCHAR *path, float def)
{
	bool created;
	tinyxml2::XMLElement *pel = EvaluatePath(path, true, created);

	if (pel)
	{
		if (created)
		{
			pel->SetAttribute("type", "float");
			pel->SetAttribute("value", def);
			return def;
		}

		if (IsType(pel, _T("float")))
		{
			tinyxml2::XMLAttribute *pa = GetAttribute(pel, _T("value"));
			if (pa)
			{
				return pa->FloatValue();
			}
		}
	}

	return def;
}


void CConfiguration::SetFloat(const TCHAR *path, float val)
{
	bool created;
	tinyxml2::XMLElement *pel = EvaluatePath(path, true, created);

	if (pel)
	{
		if (created)
		{
			pel->SetAttribute("value", "0.0");
			pel->SetAttribute("type", "float");
			return;
		}

		if (IsType(pel, _T("float")))
		{
			tinyxml2::XMLAttribute *pa = GetAttribute(pel, _T("value"));
			if (pa)
			{
				pa->SetAttribute(val);
			}
		}
	}
}


bool CConfiguration::GetBool(const TCHAR *path, bool def)
{
	bool created;
	tinyxml2::XMLElement *pel = EvaluatePath(path, true, created);

	if (pel)
	{
		if (created)
		{
			pel->SetAttribute("value", def ? "true" : "false");
			pel->SetAttribute("type", "bool");
			return def;
		}

		if (IsType(pel, _T("bool")))
		{
			tinyxml2::XMLAttribute *pa = GetAttribute(pel, _T("value"));
			if (pa)
			{
				return (_stricmp(pa->Value(), "true") ? false : true);
			}
		}
	}

	return def;
}


void CConfiguration::SetBool(const TCHAR *path, bool val)
{
	bool created;
	tinyxml2::XMLElement *pel = EvaluatePath(path, true, created);

	if (pel)
	{
		if (created)
		{
			pel->SetAttribute("value", "false");
			pel->SetAttribute("type", "bool");
		}

		if (IsType(pel, _T("bool")))
		{
			tinyxml2::XMLAttribute *pa = GetAttribute(pel, _T("value"));
			if (pa)
			{
				pa->SetAttribute(val ? "true" : "false");
			}
		}
	}
}


bool CConfiguration::GetRect(const TCHAR *path, RECT &val, const RECT *def)
{
	bool created;
	tinyxml2::XMLElement *pel = EvaluatePath(path, true, created);

	if (pel)
	{
		if (created)
		{
			char s[32];

			sprintf_s(s, sizeof(s), "%d", def ? def->left : 0);
			pel->SetAttribute("left", s);

			sprintf_s(s, sizeof(s), "%d", def ? def->top : 0);
			pel->SetAttribute("top", s);

			sprintf_s(s, sizeof(s), "%d", def ? def->right : 0);
			pel->SetAttribute("right", s);

			sprintf_s(s, sizeof(s), "%d", def ? def->bottom : 0);
			pel->SetAttribute("bottom", s);

			pel->SetAttribute("type", "rect");
		}

		if (IsType(pel, _T("rect")))
		{
			tinyxml2::XMLAttribute *pa = GetAttribute(pel, _T("left"));
			if (pa)
			{
				val.left = pa->IntValue();
			}
			pa = GetAttribute(pel, _T("top"));
			if (pa)
			{
				val.top = pa->IntValue();
			}
			pa = GetAttribute(pel, _T("right"));
			if (pa)
			{
				val.right = pa->IntValue();
			}
			pa = GetAttribute(pel, _T("bottom"));
			if (pa)
			{
				val.bottom = pa->IntValue();
			}

			return true;
		}
	}

	return false;
}


void CConfiguration::SetRect(const TCHAR *path, const RECT &val)
{
	bool created;
	tinyxml2::XMLElement *pel = EvaluatePath(path, true, created);

	if (pel)
	{
		if (created)
		{
			pel->SetAttribute("left", "0");
			pel->SetAttribute("top", "0");
			pel->SetAttribute("right", "0");
			pel->SetAttribute("bottom", "0");
			pel->SetAttribute("type", "rect");
		}

		if (IsType(pel, _T("rect")))
		{
			tinyxml2::XMLAttribute *pa = GetAttribute(pel, _T("left"));
			if (pa)
			{
				pa->SetAttribute((int)val.left);
			}
			pa = GetAttribute(pel, _T("top"));
			if (pa)
			{
				pa->SetAttribute((int)val.top);
			}
			pa = GetAttribute(pel, _T("right"));
			if (pa)
			{
				pa->SetAttribute((int)val.right);
			}
			pa = GetAttribute(pel, _T("bottom"));
			if (pa)
			{
				pa->SetAttribute((int)val.bottom);
			}
		}
	}
}


bool CConfiguration::GetPoint(const TCHAR *path, POINT &val, const POINT *def)
{
	bool created;
	tinyxml2::XMLElement *pel = EvaluatePath(path, true, created);

	if (pel)
	{
		if (created)
		{
			char s[32];

			sprintf_s(s, sizeof(s), "%d", def ? def->x : 0);
			pel->SetAttribute("x", s);

			sprintf_s(s, sizeof(s), "%d", def ? def->y : 0);
			pel->SetAttribute("y", s);

			pel->SetAttribute("type", "point");
		}

		if (IsType(pel, _T("point")))
		{
			tinyxml2::XMLAttribute *pa = GetAttribute(pel, _T("x"));
			if (pa)
			{
				val.x = pa->IntValue();
			}
			pa = GetAttribute(pel, _T("y"));
			if (pa)
			{
				val.y = pa->IntValue();
			}

			return true;
		}
	}

	return false;
}


void CConfiguration::SetPoint(const TCHAR *path, const POINT &val)
{
	bool created;
	tinyxml2::XMLElement *pel = EvaluatePath(path, true, created);

	if (pel)
	{
		if (created)
		{
			pel->SetAttribute("x", "0");
			pel->SetAttribute("y", "0");
			pel->SetAttribute("type", "point");
		}

		if (IsType(pel, _T("point")))
		{
			tinyxml2::XMLAttribute *pa = GetAttribute(pel, _T("x"));
			if (pa)
			{
				pa->SetAttribute((int)val.x);
			}
			pa = GetAttribute(pel, _T("y"));
			if (pa)
			{
				pa->SetAttribute((int)val.y);
			}
		}
	}
}


const TCHAR *CConfiguration::GetString(const TCHAR *path, const TCHAR *def)
{
	bool created;
	tinyxml2::XMLElement *pel = EvaluatePath(path, true, created);

	if (pel)
	{
		TXmlEl2StrMap::iterator it = m_StringMap.find(pel);
		if (it == m_StringMap.end())
		{
			m_StringMap.insert(TXmlEl2StrPair(pel, tstring(def)));
			it = m_StringMap.find(pel);
		}

		if (created)
		{
			pel->SetAttribute("type", "string");

			tinyxml2::XMLText *pt = (tinyxml2::XMLText *)pel->FirstChild();
			if (!pt)
			{
				char *tmpdef;
				LOCAL_TCS2MBCS(def, tmpdef);
				tinyxml2::XMLText *tmp = m_ConfigDoc.NewText(tmpdef);
				pt = (tinyxml2::XMLText *)pel->InsertEndChild(tmp);
			}
		}

		if (IsType(pel, _T("string")))
		{
			const char *tmpret = pel->GetText();
			if (!tmpret)
			{
				tinyxml2::XMLAttribute *pa = GetAttribute(pel, _T("value"));
				if (pa)
				{
					tmpret = pa->Value();
				}
			}

			if (tmpret)
			{
				TCHAR *ret;
				LOCAL_MBCS2TCS(tmpret, ret);
				assert(it != m_StringMap.end());
				it->second = ret;

				return it->second.c_str();
			}
		}
	}

	return def;
}


void CConfiguration::SetString(const TCHAR *path, const TCHAR *val)
{
	bool created;
	tinyxml2::XMLElement *pel = EvaluatePath(path, true, created);

	if (pel)
	{
		if (created)
		{
			pel->SetAttribute("type", "string");
		}

		if (IsType(pel, _T("string")))
		{
			tinyxml2::XMLText *pt = (tinyxml2::XMLText *)pel->FirstChild();
			if (!pt)
			{
				tinyxml2::XMLText *tmp = m_ConfigDoc.NewText("");
				pt = (tinyxml2::XMLText *)pel->InsertEndChild(tmp);
			}

			if (pt)
			{
				char *tmpval;
				LOCAL_TCS2MBCS(val, tmpval);
				pt->SetValue(tmpval);

				TXmlEl2StrMap::iterator it = m_StringMap.find(pel);
				if (it == m_StringMap.end())
				{
					m_StringMap.insert(TXmlEl2StrPair(pel, tstring(val)));
				}
				else
				{
					it->second = val;
				}
			}
		}
	}
}


tinyxml2::XMLElement *CConfiguration::EvaluatePath(const TCHAR *path, bool create, bool &didcreate)
{
	didcreate = false;

	tinyxml2::XMLElement *ret = m_ConfigDoc.RootElement();
	if (!ret)
	{
		tinyxml2::XMLElement *root = m_ConfigDoc.NewElement("configuration");
		ret = (tinyxml2::XMLElement *)m_ConfigDoc.InsertEndChild(root);

		if (!ret)
		{
			return NULL;
		}
	}

	const TCHAR *allowed_chars = _T("_-:{}");

	bool first_char = true;
	tstring pathcomp;
	while (path && *path && ret)
	{
		if ((!first_char && isalnum(*path)) || isalpha(*path) || _tcschr(allowed_chars, *path))
		{
			first_char = false;
			pathcomp += *path;
		}

		path++;

		size_t tagidx = 0;

		if (*path == _T('#'))
		{
			tstring idxstr;
			path++;
			while (*path && isalnum(*path) && !isalpha(*path))
			{
				idxstr += *path;
				path++;
			}

			tagidx = _ttoi(idxstr.c_str());
		}

		if ((!*path || !(isalnum(*path) || _tcschr(allowed_chars, *path))) && !pathcomp.empty())
		{
			char *elstr;
			LOCAL_TCS2MBCS(pathcomp.c_str(), elstr);

			tinyxml2::XMLElement *tmpret = ret->FirstChildElement(elstr);
			while ((tagidx > 0) && tmpret)
			{
				tinyxml2::XMLElement *next = tmpret->NextSiblingElement(elstr);
				if (!next)
				{
					tinyxml2::XMLElement *dummy = m_ConfigDoc.NewElement(elstr);
					next = (tinyxml2::XMLElement *)ret->InsertAfterChild(tmpret, dummy);
				}

				tagidx--;
				tmpret = next;
			}

			if (!tmpret)
			{
				std::transform(pathcomp.begin(), pathcomp.end(), pathcomp.begin(), tolower);

				tinyxml2::XMLElement *dummy = m_ConfigDoc.NewElement(elstr);
				if (!dummy)
					return nullptr;

				tmpret = (tinyxml2::XMLElement *)ret->InsertEndChild(dummy);
				didcreate = true;
			}

			ret = tmpret;

			pathcomp.clear();
			first_char = true;
		}
	}

	return ret;
}


tinyxml2::XMLAttribute *CConfiguration::GetAttribute(const tinyxml2::XMLElement *pel, const TCHAR *name)
{
	char *_name;
	LOCAL_TCS2MBCS(name, _name);

	tinyxml2::XMLAttribute *pa = (tinyxml2::XMLAttribute *)pel->FirstAttribute();
	while (pa)
	{
		if (!_stricmp(pa->Name(), _name))
		{
			return pa;
		}

		pa = (tinyxml2::XMLAttribute *)pa->Next();
	}

	return NULL;
}

bool CConfiguration::IsType(const tinyxml2::XMLElement *pel, const TCHAR *type)
{
	char *_type;
	LOCAL_TCS2MBCS(type, _type);

	tinyxml2::XMLAttribute *pa = GetAttribute(pel, _T("type"));
	if (pa)
	{
		if (!_stricmp(pa->Value(), _type))
			return true;
	}

	return false;
}

size_t CConfiguration::GetNumSubKeys(const TCHAR *path)
{
	size_t ret = 0;

	bool created;
	tinyxml2::XMLElement *pel = EvaluatePath(path, true, created);

	if (pel)
	{
		tinyxml2::XMLElement *pchild = pel->FirstChildElement();
		while (pchild)
		{
			ret++;

			pchild = pchild->NextSiblingElement();
		}
	}

	return ret;
}

bool CConfiguration::GetSubKeyName(const TCHAR *path, size_t subkeyindex, TCHAR *retbuf, size_t retbuflen)
{
	bool ret = false;

	bool created;
	tinyxml2::XMLElement *pel = EvaluatePath(path, true, created);

	if (pel)
	{
		tinyxml2::XMLElement *pchild = pel->FirstChildElement();

		for (size_t i = 0; i < subkeyindex; i++)
		{
			if (pchild)
				pchild = pel->NextSiblingElement();
			else
				return false;
		}

		if (pchild)
		{
			TCHAR *val;
			LOCAL_MBCS2TCS(pchild->Value(), val);

			_tcsncpy_s(retbuf, retbuflen, val, retbuflen);
			ret = true;
		}
	}

	return ret;
}

bool CConfiguration::RemoveKey(const TCHAR *path)
{
	bool created;
	tinyxml2::XMLElement *pel = EvaluatePath(path, false, created);
	if (pel)
	{
		TXmlEl2StrMap::const_iterator it = m_StringMap.find(pel);
		if (it != m_StringMap.end())
			m_StringMap.erase(it);

		m_ConfigDoc.DeleteNode(pel);
	}

	return (pel != nullptr);
}

bool CConfiguration::RemoveSubKey(const TCHAR *path, size_t subkeyindex)
{
	bool created;
	tinyxml2::XMLElement *pel = EvaluatePath(path, false, created);

	if (pel)
	{
		tinyxml2::XMLElement *pchild = pel->FirstChildElement();

		for (size_t i = 0; i < subkeyindex; i++)
		{
			if (pchild)
				pchild = pel->NextSiblingElement();
			else
				return false;
		}

		if (pchild)
		{
			TXmlEl2StrMap::const_iterator it = m_StringMap.find(pel);
			if (it != m_StringMap.end())
				m_StringMap.erase(it);

			m_ConfigDoc.DeleteNode(pel);

			return true;
		}
	}

	return false;
}
