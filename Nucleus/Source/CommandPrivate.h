// **************************************************************
// Nucleus Source File
//
// Copyright © 2019-2026, Keelan Sturt


#pragma once

#include <NuCommand.h>
#include <NuCommandInfo.h>
#include <NuTrack.h>

#define MIN_DURATION		0.001

using namespace nucleus;

class CCommand : public nucleus::ICommand, props::IPropertyChangeListener
{

public:
	CCommand(const nucleus::ICommandInfo *pcmdinfo);
	virtual ~CCommand();

	virtual void Release();

	virtual const ICommandInfo *GetCommandInfo() const;
	virtual props::IPropertySet *GetParams() const;
	virtual const ITrack *GetOwner(double *time = nullptr) const;
	virtual double GetDuration() const;
	virtual void SetPropertyChangeHandler(PropertyChangeHandler handler);

	virtual void PropertyChanged(const props::IProperty *pprop);

	virtual void SetOwner(ITrack *ptrack);

	virtual bool Load(tinyxml2::XMLNode *command_root);
	virtual bool Save(genio::ITextOutput *pto);

protected:
	props::IPropertySet *m_pParams;
	props::IProperty *m_pDuration;
	const ICommandInfo *m_pCmdInfo;
	ITrack *m_pOwner;
	PropertyChangeHandler m_PropChangeCB;

};