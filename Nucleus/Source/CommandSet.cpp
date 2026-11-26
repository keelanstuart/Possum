#include "stdafx.h"
#include "CommandSetPrivate.h"
#include <algorithm>

CCommandSet::CCommandSet()
{

}

CCommandSet::~CCommandSet()
{

}

void CCommandSet::Add(nucleus::ICommand* pcmd)
{
	m_Set.insert(pcmd);
}

void CCommandSet::Remove(nucleus::ICommand* pcmd)
{
	m_Set.erase(pcmd);
}

nucleus::ICommand* CCommandSet::Get(size_t index) const
{
	if (index >= m_Set.size())
		return nullptr;

	TCommandSet::iterator it = m_Set.begin();

	std::advance(it, index);

	return *it;
}

void CCommandSet::Clear()
{
	m_Set.clear();
}

bool CCommandSet::Contains(nucleus::ICommand* pcmd) const
{
	return (m_Set.find(pcmd) != m_Set.cend()) ? true : false;
}

size_t CCommandSet::Size() const
{
	return m_Set.size();
}

bool CCommandSet::Empty() const
{
	return m_Set.empty();
}

void CCommandSet::ForEach(nucleus::ICommandSet::CommandFunc func) const
{
	if (!func)
		return;

	for (TCommandSet::const_iterator it = m_Set.cbegin(), last_it = m_Set.cend(); it != last_it; it++)
	{
		func(*it);
	}
}

void CCommandSet::Release()
{
	delete this;
}

nucleus::ICommandSet* nucleus::ICommandSet::Create()
{
	return new CCommandSet();
}