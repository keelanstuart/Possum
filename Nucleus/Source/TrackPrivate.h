#pragma once

#include <NuTrack.h>
#include <NuCommandInfo.h>

class CTrack : public nucleus::ITrack
{
public:
	props::IPropertySet *m_pProps;
	nucleus::ITrackSet *m_pOwner;

	typedef std::multimap<double, nucleus::ICommand *> TCommandMap;
	typedef std::pair<double, nucleus::ICommand *> TCommandPair;
	TCommandMap m_CmdMap;

	typedef std::map<const nucleus::ICommand *, double> TTimeMap;
	typedef std::pair<const nucleus::ICommand *, double> TTimePair;
	TTimeMap m_TimeMap;

	typedef std::deque<nucleus::ICommand *> TCommandArray;
	TCommandArray m_Cmd;

public:
	CTrack();
	virtual ~CTrack();

	virtual void Release();

	virtual nucleus::ITrackSet *GetOwner() const;
	virtual void SetOwner(nucleus::ITrackSet *powner);

	virtual props::IPropertySet *GetProperties() const;
	virtual const TCHAR *GetName() const;
	virtual const TCHAR *GetDescription() const;

	virtual size_t GetNumCommands() const;
	virtual nucleus::ICommand *GetCommand(size_t idx, double *time = nullptr) const;
	virtual bool FindCommand(double time, nucleus::ICommand **ppcmd_ret = nullptr) const;
	virtual bool FindCommand(const nucleus::ICommand *pcmd, double *ptime_ret = nullptr) const;
	virtual bool FindCommand(const nucleus::ICommand *pcmd, size_t *pindex_ret = nullptr) const;
	virtual nucleus::RetCode AddCommand(const nucleus::ICommandInfo *pcmdinfo, double time, nucleus::ICommand **ret);
	virtual nucleus::RetCode RemoveCommand(nucleus::ICommand *pcmd, bool release_cmd = true);
	virtual nucleus::RetCode MoveCommand(nucleus::ICommand *pcmd, double time, const double *new_duration = nullptr, const nucleus::ICommandSet *ignore_blockers = nullptr);
	virtual nucleus::ICommand *GetFirstCommandByTime(double time) const;
	virtual nucleus::ICommand *GetNextCommand(const nucleus::ICommand *pcmd, std::optional<const nucleus::ICommandInfo *> type = std::nullopt) const;
	virtual nucleus::ICommand *GetPrevCommand(const nucleus::ICommand *pcmd, std::optional<const nucleus::ICommandInfo *> type = std::nullopt) const;
	virtual bool CanFit(double time, double duration, const nucleus::ICommand *pcmd, const nucleus::ICommandSet *ignore_blockers = nullptr);
	virtual double Length() const;

	virtual bool Load(tinyxml2::XMLNode *track_root);
	virtual bool Save(genio::ITextOutput *pto);

protected:
	bool LoadCommand(tinyxml2::XMLElement *cmd_root);

	virtual double GetCommandEndTime(const nucleus::ICommand *pcmd);
	virtual double GetCommandDistance(const nucleus::ICommand *pcmd1, const nucleus::ICommand *pcmd2);
	virtual bool TimeFallsWithinCommand(double time, const nucleus::ICommand *pcmd);
	virtual bool GetTimeSpanOverlapsCommand(double time, double duration, const nucleus::ICommand *pcmd);
	virtual nucleus::RetCode RemoveCommandPrivate(nucleus::ICommand *pcmd, bool release);
	virtual nucleus::RetCode AddCommandPrivate(nucleus::ICommand *pcmd, double time, TCommandMap::const_iterator &ins);

};
