// **************************************************************
// Nucleus Source File
//
// Copyright © 2019-2026, Keelan Sturt


#include "stdafx.h"

#include "TrackPrivate.h"
#include "NuTrackSet.h"
#include "NuCommand.h"
#include "NuCommandInfo.h"
#include "NuDeviceManager.h"
#include "NuDevice.h"
#include "CommandPrivate.h"
#include "DeviceManagerPrivate.h"
#include "Range.h"
#include "PropertyHelper.h"


CTrack::CTrack()
{
	m_pProps = props::IPropertySet::CreatePropertySet();
	if (m_pProps)
	{
		props::IProperty *p;

		p = m_pProps->CreateProperty(_T("Name"), 'NAME');
		p->SetString(_T("Track"));
		p->Flags().SetAll(props::IProperty::PROPFLAG(props::IProperty::EPropFlag::REQUIRED));

		p = m_pProps->CreateProperty(_T("Description"), 'DESC');
		p->SetString(_T("Anonymous Track"));
		p->Flags().SetAll(props::IProperty::PROPFLAG(props::IProperty::EPropFlag::REQUIRED));

		p = m_pProps->CreateProperty(_T("Enabled"), 'ENAB');
		p->SetBool(true);
		p->Flags().SetAll(props::IProperty::PROPFLAG(props::IProperty::EPropFlag::REQUIRED));

		p = m_pProps->CreateProperty(_T("AllowCommandOverlap"), 'ACOL');
		p->SetBool(true);
		p->Flags().SetAll(props::IProperty::PROPFLAG(props::IProperty::EPropFlag::REQUIRED));
	}
}


CTrack::~CTrack()
{
	if (m_pProps)
		m_pProps->Release();

	for (TCommandMap::const_iterator it = m_CmdMap.begin(); it != m_CmdMap.end(); it++)
	{
		it->second->Release();
	}
}


void CTrack::Release()
{
	delete this;
}


nucleus::ITrackSet *CTrack::GetOwner() const
{
	return m_pOwner;
}

void CTrack::SetOwner(nucleus::ITrackSet *powner)
{
	m_pOwner = powner;
}


props::IPropertySet *CTrack::GetProperties() const
{
	return m_pProps;
}


const TCHAR *CTrack::GetName() const
{
	if (m_pProps)
	{
		const props::IProperty *pp = m_pProps->GetPropertyById('NAME');
		if (pp)
			return pp->AsString();
	}

	return nullptr;
}


const TCHAR *CTrack::GetDescription() const
{
	if (m_pProps)
	{
		const props::IProperty *pp = m_pProps->GetPropertyById('DESC');
		if (pp)
			return pp->AsString();
	}

	return nullptr;
}


size_t CTrack::GetNumCommands() const
{
	return m_Cmd.size();
}


nucleus::ICommand *CTrack::GetCommand(size_t idx, double *time) const
{
	if (idx < m_Cmd.size())
	{
		if (time)
		{
			TTimeMap::const_iterator it = m_TimeMap.find(m_Cmd[idx]);
			if (it != m_TimeMap.end())
				*time = it->second;
		}

		return m_Cmd[idx];
	}

	return nullptr;
}

nucleus::ICommand *CTrack::GetFirstCommandByTime(double time) const
{
	// Include ongoing commands, even when several earlier commands overlap.
	for (auto it = m_CmdMap.cbegin(); it != m_CmdMap.cend(); ++it)
	{
		if (it->first >= time || it->first + it->second->GetDuration() > time)
			return it->second;
	}
	return nullptr;
}


nucleus::ICommand *CTrack::GetNextCommand(const nucleus::ICommand* pcmd, std::optional<const ICommandInfo *> type) const
{
	TTimeMap::const_iterator tit = m_TimeMap.find(pcmd);
	if (tit == m_TimeMap.cend())
		return nullptr;

	auto range = m_CmdMap.equal_range(tit->second);
	TCommandMap::const_iterator cit = range.first;
	while (cit != range.second && cit->second != pcmd)
		++cit;
	if (cit == range.second)
		return nullptr;

	do
	{
		cit++;
		if (cit == m_CmdMap.cend())
			return nullptr;
	}
	while (type.has_value() && (cit->second->GetCommandInfo() != *type));

	return cit->second;
}


nucleus::ICommand *CTrack::GetPrevCommand(const nucleus::ICommand *pcmd, std::optional<const ICommandInfo *> type) const
{
	TTimeMap::const_iterator tit = m_TimeMap.find(pcmd);
	if (tit == m_TimeMap.cend())
		return nullptr;

	auto range = m_CmdMap.equal_range(tit->second);
	TCommandMap::const_iterator cit = range.first;
	while (cit != range.second && cit->second != pcmd)
		++cit;
	if (cit == range.second)
		return nullptr;

	do
	{
		if (cit == m_CmdMap.cbegin())
			return nullptr;
		--cit;
	}
	while (type.has_value() && (cit->second->GetCommandInfo() != *type));

	return cit->second;
}


bool CTrack::FindCommand(double time, nucleus::ICommand **ppcmd_ret) const
{
	// An overlapping command may have begun before the lower bound.
	for (auto it = m_CmdMap.cbegin(); it != m_CmdMap.cend() && it->first <= time; ++it)
	{
		if (time < it->first + it->second->GetDuration())
		{
			if (ppcmd_ret)
				*ppcmd_ret = it->second;
			return true;
		}
	}
	return false;
}


bool CTrack::FindCommand(const nucleus::ICommand *pcmd, double *ptime_ret) const
{
	TTimeMap::const_iterator it = m_TimeMap.find(pcmd);
	if (it != m_TimeMap.end())
	{
		if (ptime_ret)
			*ptime_ret = it->second;

		return true;
	}

	return false;
}


bool CTrack::FindCommand(const nucleus::ICommand *pcmd, size_t *pindex_ret) const
{
	TCommandArray::const_iterator it = std::find(m_Cmd.cbegin(), m_Cmd.cend(), pcmd);
	if (it != m_Cmd.end())
	{
		if (pindex_ret)
			*pindex_ret = (it - m_Cmd.cbegin());

		return true;
	}

	return false;
}


nucleus::RetCode CTrack::AddCommandPrivate(nucleus::ICommand *pcmd, double time, TCommandMap::const_iterator &ins)
{
	if (pcmd)
	{
		if (ins != m_CmdMap.cend())
		{
			TCommandArray::iterator iit = std::find(m_Cmd.begin(), m_Cmd.end(), ins->second);
			m_Cmd.insert(iit, pcmd);
		}
		else
		{
			m_Cmd.push_back(pcmd);
		}

		m_CmdMap.insert(TCommandPair(time, pcmd));
		m_TimeMap.insert(TTimePair(pcmd, time));

		((CCommand *)pcmd)->SetOwner(this);

		return nucleus::RetCode::OK;
	}

	return nucleus::RetCode::UNKNOWN_ERROR;
}


nucleus::RetCode CTrack::AddCommand(const nucleus::ICommandInfo *pcmdinfo, double time, nucleus::ICommand **ret)
{
	if (!ret)
		return nucleus::RetCode::TRACK_NORETURNABLE;

	if (!pcmdinfo)
		return nucleus::RetCode::TRACK_UNKNOWNCOMMAND;

	double duration = MIN_DURATION;
	const props::IPropertySet *ps = pcmdinfo->GetDefaultParams();
	if (ps)
	{
		const props::IProperty *p = ps->GetPropertyById('DURA');
		if (p)
			duration = p->AsFloat();
	}
	if (!CanFit(time, duration, nullptr))
		return nucleus::RetCode::TRACK_TIMEOVERLAP;

	TCommandMap::const_iterator uit = m_CmdMap.upper_bound(time);
	*ret = nucleus::ICommand::CreateCommand(pcmdinfo);
	if (*ret)
		return AddCommandPrivate(*ret, time, uit);

	return nucleus::RetCode::UNKNOWN_ERROR;
}

nucleus::RetCode CTrack::RemoveCommandPrivate(nucleus::ICommand *pcmd, bool release)
{
	TTimeMap::iterator it = m_TimeMap.find(pcmd);
	if (it != m_TimeMap.end())
	{
		auto range = m_CmdMap.equal_range(it->second);
		for (auto cit = range.first; cit != range.second; ++cit)
		{
			if (cit->second == pcmd)
			{
				m_CmdMap.erase(cit);
				break;
			}
		}
		m_TimeMap.erase(it);
	}

	TCommandArray::iterator lit = std::find(m_Cmd.begin(), m_Cmd.end(), pcmd);
	if (lit == m_Cmd.end())
		return nucleus::RetCode::UNKNOWN_ERROR;
	
	m_Cmd.erase(lit);

	if (release)
	{
		pcmd->Release();
	}
	else
	{
		CCommand *pc = dynamic_cast<CCommand *>(pcmd);
		if (pc)
			pc->SetOwner(nullptr);
	}

	return nucleus::RetCode::OK;
}

nucleus::RetCode CTrack::RemoveCommand(nucleus::ICommand *pcmd, bool release_cmd)
{
	return RemoveCommandPrivate(pcmd, release_cmd);
}

nucleus::RetCode CTrack::MoveCommand(nucleus::ICommand *pcmd, double time, const double *new_duration, const nucleus::ICommandSet* ignore_blockers)
{
	time = max(time, 0.0f);

	if (CanFit(time, (new_duration == nullptr) ? pcmd->GetDuration() : *new_duration, pcmd, ignore_blockers))
	{
		if (new_duration)
		{
			props::IProperty *pd = pcmd->GetParams()->GetPropertyById('DURA');
			if (pd)
			{
				pd->SetFloat((float)(std::max<double>(MIN_DURATION, *new_duration)));
			}
		}

		CTrack* old_owner = (CTrack *)(pcmd->GetOwner());
		if (old_owner)
			old_owner->RemoveCommandPrivate(pcmd, false);

		TCommandMap::const_iterator uit = m_CmdMap.upper_bound(time);

		return AddCommandPrivate(pcmd, time, uit);
	}

	return nucleus::RetCode::TRACK_INSUFFICIENTTIME;
}

double CTrack::GetCommandEndTime(const nucleus::ICommand *pcmd)
{
	double start = 0.0;

	// gets the command's start time
	FindCommand(pcmd, &start);

	// Add the duration
	return (start + pcmd->GetDuration());
}

double CTrack::GetCommandDistance(const nucleus::ICommand *pcmd1, const nucleus::ICommand *pcmd2)
{
	if (!pcmd1 || !pcmd2 || (pcmd1 == pcmd2))
		return 0.0f;

	double t1 = GetCommandEndTime(pcmd1), t2;
	FindCommand(pcmd2, &t2);

	return (t2 - t1);
}

bool CTrack::TimeFallsWithinCommand(double time, const nucleus::ICommand *pcmd)
{
	double cs = DBL_MAX;
	double ce = 0.0f;

	TTimeMap::const_iterator it = m_TimeMap.find(pcmd);
	if (it != m_TimeMap.end())
	{
		cs = it->second;
		ce = pcmd->GetDuration() + cs;
	}

	return ((time > cs) && (time < ce)) ? true : false;
}

bool CTrack::GetTimeSpanOverlapsCommand(double time, double duration, const nucleus::ICommand *pcmd)
{
	TTimeMap::const_iterator it = m_TimeMap.find(pcmd);
	if (it != m_TimeMap.end())
	{
		return ((time < it->second) && (it->second <= (time + duration))) ? true : false;
	}

	return false;
}

bool CTrack::CanFit(double time, double duration, const nucleus::ICommand *pcmd, const nucleus::ICommandSet* ignore_blockers)
{
	if (time < 0)
		return false;

	props::IProperty *p = m_pProps->GetPropertyById('ACOL');
	if (p && p->AsBool())
		return true;

	double end_time = time + duration;
	TRange<double> cmd_range(time, end_time);

	// not cool, but the brute force way of determining time bounds works and the 
	for (TCommandMap::const_iterator it = m_CmdMap.begin(), lit = m_CmdMap.end(); it != lit; it++)
	{
		if ((pcmd == it->second) || (ignore_blockers && ignore_blockers->Contains(it->second)))
			continue;

		if (it->first > end_time)
			break;

		TRange<double> it_range(it->first, it->first + it->second->GetDuration());
		if (it_range.Intersects(cmd_range))
			return false;
	}

	return true;
}

bool CTrack::LoadCommand(tinyxml2::XMLElement *cmd_root)
{
	nucleus::IDeviceManager *pdm = m_pOwner->GetNucleus()->DeviceManager();

	TCHAR *sd;
	LOCAL_MBCS2TCS(cmd_root->Attribute("device"), sd);
	nucleus::IDevice *pd = pdm->GetDeviceByName(sd);
	if (pd)
	{
		TCHAR *sci;
		LOCAL_MBCS2TCS(cmd_root->Attribute("name"), sci);
		const nucleus::ICommandInfo *pci = pd->GetCommandByName(sci);

		double t = cmd_root->DoubleAttribute("time");

		nucleus::ICommand *pc;
		if (nucleus::RetCode::OK == AddCommand(pci, t, &pc))
		{
			LoadPropertySet(pc->GetParams(), cmd_root->FirstChildElement("properties"));
		}
	}

	return true;
}

bool CTrack::Load(tinyxml2::XMLNode *track_root)
{
	if (!track_root)
		return false;

	tinyxml2::XMLElement *pxp = track_root->FirstChildElement("properties");
	LoadPropertySet(m_pProps, pxp);

	tinyxml2::XMLElement *pxcs = track_root->FirstChildElement("commands");
	if (pxcs)
	{
		tinyxml2::XMLElement *pxc = pxcs->FirstChildElement("nucleus:command");
		while (pxc)
		{
			if (!LoadCommand(pxc))
				return false;

			pxc = pxc->NextSiblingElement("nucleus:command");
		}
	}

	return true;
}

bool CTrack::Save(genio::ITextOutput *pto)
{
	if (!pto)
		return false;

	pto->Print(_T("<nucleus:track>"));
	pto->IncIndent();
	pto->NextLine();

	SavePropertySet(GetProperties(), pto);

	if (GetNumCommands())
	{
		pto->NextLine(1);

		pto->Print(_T("<commands>"));
		pto->IncIndent();

		for (size_t i = 0, maxi = GetNumCommands(); i < maxi; i++)
		{
			pto->NextLine((i > 0) ? 1 : 0);

			double t;
			nucleus::ICommand *pc = GetCommand(i, &t);
			if (!pc)
				continue;

			const nucleus::ICommandInfo *pci = pc->GetCommandInfo();
			if (!pci)
				continue;

			pto->PrintF(_T("<nucleus:command device=\"%s\" name=\"%s\" time=\"%.17g\">"), pci->GetDevice()->GetName(), pci->GetName(), t);
			pto->IncIndent();
			pto->NextLine();

			SavePropertySet(pc->GetParams(), pto);

			pto->DecIndent();
			pto->NextLine();
			pto->PrintF(_T("</nucleus:command>"));
		}

		pto->DecIndent();
		pto->NextLine();
		pto->Print(_T("</commands>"));
	}

	pto->DecIndent();
	pto->NextLine();
	pto->Print(_T("</nucleus:track>"));

	return true;
}

double CTrack::Length() const
{
	if (m_CmdMap.empty())
		return 0.0;

	double end_time = 0.0;
	for (const auto &entry : m_CmdMap)
		end_time = max(end_time, entry.first + entry.second->GetDuration());
	return end_time;
}
