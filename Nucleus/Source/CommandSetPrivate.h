#pragma once

#include <NuCommandSet.h>
#include <set>

class CCommandSet : public nucleus::ICommandSet
{

public:

	CCommandSet();
	virtual ~CCommandSet();

	virtual void Add(nucleus::ICommand* pcmd);
	virtual void Remove(nucleus::ICommand* pcmd);
	virtual nucleus::ICommand* Get(size_t index) const;
	virtual void Clear();
	virtual bool Contains(nucleus::ICommand* pcmd) const;
	virtual size_t Size() const;
	virtual bool Empty() const;
	virtual void ForEach(nucleus::ICommandSet::CommandFunc func) const;
	virtual void Release();

protected:
	typedef std::set<nucleus::ICommand*> TCommandSet;
	TCommandSet m_Set;

};
