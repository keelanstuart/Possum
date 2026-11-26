// **************************************************************
// Nucleus Source File
//
// Copyright © 2019-2026, Keelan Sturt


#include "stdafx.h"

#include "CommandPrivate.h"
#include "PropertyHelper.h"


CCommand::CCommand(const nucleus::ICommandInfo *cmdinfo)
{
	assert(cmdinfo);
	m_pCmdInfo = cmdinfo;

	m_pParams = props::IPropertySet::CreatePropertySet();
	if (m_pParams)
		m_pParams->AppendPropertySet(m_pCmdInfo->GetDefaultParams(), true);
	m_pDuration = m_pParams->GetPropertyById('DURA');

	m_pOwner = nullptr;
	m_pParams->SetChangeListener(this);
}


CCommand::~CCommand()
{
	if (m_pParams)
		m_pParams->Release();
}


void CCommand::Release()
{
	delete this;
}


const ICommandInfo *CCommand::GetCommandInfo() const
{
	return m_pCmdInfo;
}


props::IPropertySet *CCommand::GetParams() const
{
	return m_pParams;
}


const ITrack *CCommand::GetOwner(double *time) const
{
	// if we were given a time, get it from the track that owns this command instance
	if (m_pOwner && time)
		m_pOwner->FindCommand(this, time);

	return m_pOwner;
}


void CCommand::SetOwner(ITrack *ptrack)
{
	m_pOwner = ptrack;
}


double CCommand::GetDuration() const
{
	return (m_pDuration ? m_pDuration->AsFloat() : MIN_DURATION);
}

void CCommand::SetPropertyChangeHandler(PropertyChangeHandler handler)
{
	m_PropChangeCB = handler;
}

void CCommand::PropertyChanged(const props::IProperty *pprop)
{
	if (!m_pDuration && (pprop->GetID() == 'DURA'))
		m_pDuration = (props::IProperty *)pprop;

	if (m_PropChangeCB)
		m_PropChangeCB(this, pprop);
}

bool CCommand::Load(tinyxml2::XMLNode *command_root)
{
	return true;
}

bool CCommand::Save(genio::ITextOutput *pto)
{
	return true;
}

nucleus::ICommand *nucleus::ICommand::CreateCommand(const ICommandInfo *pcmdinfo)
{
	if (!pcmdinfo)
		return nullptr;

	nucleus::ICommand *ret = new CCommand(pcmdinfo);

	pcmdinfo->RunOnCreatedHandler(ret);

	return ret;
}
