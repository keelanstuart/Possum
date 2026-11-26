// **************************************************************
// Nucleus Source File
//
// Copyright © 2019-2026, Keelan Sturt


#include "stdafx.h"

#include "NuTrack.h"
#include "NuTrackSet.h"
#include "NuCommand.h"
#include "NuCommandInfo.h"
#include "TrackPrivate.h"
#include "PropertyHelper.h"
#include "NuImporter.h"


class CTrackSet : public nucleus::ITrackSet
{
protected:
	typedef std::deque<nucleus::ITrack *> TTrackArray;
	TTrackArray m_Tracks;
	nucleus::INucleus *m_pSys;

	props::IPropertySet *m_Props;

public:
	CTrackSet(nucleus::INucleus *pnucleus)
	{
		m_pSys = pnucleus;
		m_Props = props::IPropertySet::CreatePropertySet();
	}


	virtual ~CTrackSet()
	{
		for (TTrackArray::iterator it = m_Tracks.begin(), last_it = m_Tracks.end(); it != last_it; it++)
		{
			(*it)->Release();
		}

		if (m_Props)
			m_Props->Release();
	}


	virtual void Release()
	{
		delete this;
	}


	virtual nucleus::RetCode Load(const TCHAR *filename)
	{
		if (!filename)
			return nucleus::RetCode::NULLFILESPEC;

		// Find the extension...
		TCHAR *ext = PathFindExtension(filename);
		if (ext)
		{
			// advance past a dot
			if (*ext == _T('.'))
				ext++;

			// go through each importer and check the supported extensions
			for (size_t ii = 0, maxii = nucleus::IImporter::GetNumImporters(); ii < maxii; ii++)
			{
				const nucleus::IImporter *pimp = nucleus::IImporter::GetImporter(ii);
				if (!pimp)
					continue;

				for (size_t ei = 0, maxei = pimp->GetNumExtensions(); ei < maxei; ei++)
				{
					// get the extension and check it against what we're trying to load
					const TCHAR *pext = pimp->GetExtension(ei);
					if (!pext || _tcsicmp(pext, ext))
						continue;

					// import!!
					bool expres = pimp->Import(filename, this);
					if (expres)
						return nucleus::RetCode::OK;
					
					return nucleus::RetCode::UNKNOWN_ERROR;
				}
			}
		}

		// files that don't have a registered extension or lacked an extension get the normal load path

		char *_filename;
		LOCAL_TCS2MBCS(filename, _filename);

		tinyxml2::XMLDocument doc;
		if (doc.LoadFile(_filename) != tinyxml2::XMLError::XML_SUCCESS)
			return nucleus::RetCode::UNKNOWN_ERROR;

		tinyxml2::XMLElement *pxts = doc.FirstChildElement("nucleus:trackset");
		if (pxts)
		{
			tinyxml2::XMLElement *pxp = pxts->FirstChildElement("properties");
			if (pxp)
				LoadPropertySet(m_Props, pxp);

			tinyxml2::XMLElement *pxt = pxts->FirstChildElement("nucleus:track");
			while (pxt)
			{
				CTrack *pt  = new CTrack();
				if (pt)
				{
					pt->SetOwner(this);
					m_Tracks.push_back(pt);

					pt->Load(pxt);
				}

				pxt = pxt->NextSiblingElement("nucleus:track");
			}
		}

		return nucleus::RetCode::OK;
	}


	virtual nucleus::RetCode Save(const TCHAR *filename) const
	{
		//HANDLE h = CreateFile(filename, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, 0, NULL);
		//if (h != INVALID_HANDLE_VALUE)
		{
			genio::ITextOutput *pto = genio::ITextOutput::Create(filename);
			if (pto)
			{
				pto->Print(_T("<!-- Created by a Nucleus application (probably Possum) -->"));
				pto->NextLine(1);

				pto->Print(_T("<nucleus:trackset>"));
				pto->IncIndent();
				pto->NextLine(1);

				SavePropertySet(m_Props, pto);
				pto->NextLine(1);

				for (size_t i = 0, maxi = GetNumTracks(); i < maxi; i++)
				{
					((CTrack *)GetTrack(i))->Save(pto);
					pto->NextLine((i < (maxi - 1)) ? 1 : 0);
				}

				pto->DecIndent();
				pto->NextLine();
				pto->Print(_T("</nucleus:trackset>"));

				pto->Flush();

				pto->Release();
			}

			//CloseHandle(h);
		}

		return nucleus::RetCode::OK;
	}


	virtual nucleus::RetCode CreateTrack(nucleus::ITrack **pptrackret, const TCHAR *name, const TCHAR *description)
	{
		if (!pptrackret)
			return nucleus::RetCode::TRACK_NORETURNABLE;

		if (!name)
			return nucleus::RetCode::TRACK_NONAME;

		CTrack *pt  = new CTrack();
		if (pt)
		{
			pt->SetOwner(this);
			m_Tracks.push_back(pt);

			props::IPropertySet *ps = pt->GetProperties();
			if (ps)
			{
				props::IProperty *p;

				p = ps->GetPropertyById('NAME');
				if (p)
					p->SetString(name);

				p = ps->GetPropertyById('DESC');
				if (p)
					p->SetString(description ? description : _T(""));
			}

			*pptrackret = pt;

			return nucleus::RetCode::OK;
		}

		return nucleus::RetCode::UNKNOWN_ERROR;
	}


	virtual nucleus::RetCode MoveTrack(nucleus::ITrack *ptrack, size_t position)
	{
		if (!ptrack)
			return nucleus::RetCode::UNKNOWN_ERROR;

		CTrack *pt = dynamic_cast<CTrack *>(ptrack);
		if (!pt)
			return nucleus::RetCode::UNKNOWN_ERROR;

		auto it = std::find(m_Tracks.begin(), m_Tracks.end(), pt);

		if (it != m_Tracks.end())
			m_Tracks.erase(it);
		else
			pt->SetOwner(this);

		position = std::min<size_t>(position, m_Tracks.size());
		m_Tracks.insert(m_Tracks.begin() + position, pt);

		return nucleus::RetCode::OK;
	}


	virtual nucleus::RetCode RemoveTrack(nucleus::ITrack *ptrack, bool release_track)
	{
		TTrackArray::iterator it = std::find(m_Tracks.begin(), m_Tracks.end(), ptrack);
		if (it != m_Tracks.end())
		{
			m_Tracks.erase(it);

			if (release_track)
				ptrack->Release();
			else
			{
				CTrack *pt  = dynamic_cast<CTrack *>(ptrack);
				pt->SetOwner(nullptr);
			}

			return nucleus::RetCode::OK;
		}

		return nucleus::RetCode::UNKNOWN_ERROR;
	}


	virtual size_t GetNumTracks() const
	{
		return m_Tracks.size();
	}


	virtual nucleus::ITrack *GetTrack(size_t index) const
	{
		if (index < m_Tracks.size())
			return m_Tracks[index];

		return nullptr;
	}


	virtual nucleus::ITrack *GetTrackByName(const _TCHAR *name) const
	{
		for (TTrackArray::const_iterator it = m_Tracks.cbegin(), last_it = m_Tracks.cend(); it != last_it; it++)
		{
			props::IPropertySet *ps = (*it)->GetProperties();
			if (ps)
			{
				props::IProperty *p = ps->GetPropertyById('NAME');
				if (p && !_tcsicmp(p->AsString(), name))
					return *it;
			}
		}

		return nullptr;
	}


	virtual size_t GetTrackIndex(const nucleus::ITrack *ptrack) const
	{
		for (size_t i = 0, maxi = m_Tracks.size(); i < maxi; i++)
		{
			if (ptrack == m_Tracks[i])
				return i;
		}

		return INVALID_TRACK;
	}


	virtual props::IPropertySet *GetProperties()
	{
		return m_Props;
	}


	virtual nucleus::INucleus *GetNucleus() const
	{
		return m_pSys;
	}

	virtual double Length() const
	{
		double ret = 0.0;

		for(size_t i = 0, maxi = m_Tracks.size(); i < maxi; i++)
		{
			ret = max(ret, m_Tracks[i]->Length());
		}

		return ret;
	}

};

nucleus::ITrackSet *nucleus::ITrackSet::Create(nucleus::INucleus *pnucleus)
{
	if (!pnucleus)
		return nullptr;

	return new CTrackSet(pnucleus);
}

