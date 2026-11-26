// **************************************************************
// Nucleus Source File
//
// Copyright © 2019-2026, Keelan Sturt


#pragma once


namespace nucleus
{

	class IDevice;

	class ICommandInfo
	{

	public:

		using OnCreatedHandler = std::function<void(ICommand *)>;

		/// Returns the device that owns this command
		virtual nucleus::IDevice *GetDevice() const = NULL;

		/// Returns the command's name
		virtual const TCHAR *GetName() const = NULL;

		/// Returns the command's description
		virtual const TCHAR *GetDescription() const = NULL;

		/// Provides the default parameters for the command. When issuing
		/// a command to a device, the property set should be duplicated from this and
		/// and the values overridden
		virtual const props::IPropertySet *GetDefaultParams() const = NULL;

		virtual void SetOnCreatedHandler(OnCreatedHandler handler) = NULL;

		virtual void RunOnCreatedHandler(ICommand *pcmd) const = NULL;

		virtual void Release() = NULL;

	};

	/// HELPER MACROS FOR CREATING COMMANDS EASILY
#define COMMAND_REGISTRY() typedef std::vector<nucleus::ICommandInfo *> TCommandArray; TCommandArray m_Cmds;					\
	typedef std::map<tstring, nucleus::ICommandInfo *> TCommandMap; TCommandMap m_MappedCmds;									\
	typedef std::pair<tstring, nucleus::ICommandInfo *> TCommandPair;															\
	public:																														\
	virtual size_t GetNumCommands() const																						\
	{																															\
		return m_Cmds.size();																									\
	}																															\
	virtual const nucleus::ICommandInfo *GetCommand(size_t idx) const															\
	{																															\
		if (idx < m_Cmds.size())																								\
			return m_Cmds[idx];																									\
		return nullptr;																											\
	}																															\
	virtual const nucleus::ICommandInfo *GetCommandByName(const TCHAR *name) const												\
	{																															\
		TCommandMap::const_iterator it = m_MappedCmds.find(name);																\
		if (it == m_MappedCmds.end())																							\
			return nullptr;																										\
		return it->second;																										\
	}

#define COMMAND_CLASS_NAME(_name) CCmd##_name
#define COMMAND_INSTPTR_NAME(_name) _name##InstPtr

#define REGISTER_COMMAND(_name)																									\
	{																															\
		COMMAND_INSTPTR_NAME(_name) = new COMMAND_CLASS_NAME(_name)(this);														\
		m_Cmds.push_back(COMMAND_INSTPTR_NAME(_name));																			\
		m_MappedCmds.insert(TCommandPair(_T(#_name), COMMAND_INSTPTR_NAME(_name)));												\
	}

#define RELEASE_COMMANDS()																										\
	{																															\
		for (TCommandArray::iterator it = m_Cmds.begin(), last_it = m_Cmds.end(); it != last_it; it++)							\
			(*it)->Release();																									\
	}

#define BEGIN_COMMAND_DEFINITION(_name, _desc) class COMMAND_CLASS_NAME(_name) : public nucleus::ICommandInfo					\
	{																															\
	friend IDevice;																												\
	protected:																													\
		props::IPropertySet *m_pProps;																							\
		IDevice *m_pDev;																										\
		OnCreatedHandler m_OnCreatedHandler{};																					\
	public:																														\
		virtual ~COMMAND_CLASS_NAME(_name)() { if (m_pProps) { m_pProps->Release(); m_pProps = nullptr; } }						\
		virtual void Release() { delete this; }																					\
		virtual const TCHAR *GetName() const { return _T(#_name); }																\
		virtual const TCHAR *GetDescription() const { return _T(_desc); }														\
		virtual const props::IPropertySet *GetDefaultParams() const { return m_pProps; }										\
		virtual IDevice *GetDevice() const { return m_pDev; }																	\
		/* integer properties */																								\
		virtual void AddParam(const TCHAR *name, uint32_t FCC, int64_t default_ival,											\
			props::IProperty::PROPERTY_ASPECT aspect = props::IProperty::PROPERTY_ASPECT::PA_GENERIC, uint32_t flags = 0)		\
		{ props::IProperty *pp = m_pProps->CreateProperty(name, FCC); pp->SetInt(default_ival); pp->SetAspect(aspect);			\
			pp->Flags().SetAll(flags); pp->SetAspect(aspect); }																	\
		/* boolean properties */																								\
		virtual void AddParam(const TCHAR *name, uint32_t FCC, bool default_bval,												\
			props::IProperty::PROPERTY_ASPECT aspect = props::IProperty::PROPERTY_ASPECT::PA_GENERIC, uint32_t flags = 0)		\
		{ props::IProperty *pp = m_pProps->CreateProperty(name, FCC); pp->SetBool(default_bval); pp->SetAspect(aspect); 		\
			pp->Flags().SetAll(flags); pp->SetAspect(aspect); }																	\
		/* string properties */																									\
		virtual void AddParam(const TCHAR *name, uint32_t FCC, const TCHAR *default_sval,										\
			props::IProperty::PROPERTY_ASPECT aspect = props::IProperty::PROPERTY_ASPECT::PA_GENERIC, uint32_t flags = 0)		\
		{ props::IProperty *pp = m_pProps->CreateProperty(name, FCC); pp->SetString(default_sval); pp->SetAspect(aspect); 		\
			pp->Flags().SetAll(flags); pp->SetAspect(aspect); }																	\
		/* enum properties */																									\
		virtual void AddParam(const TCHAR *name, uint32_t FCC, const TCHAR *default_esval, size_t default_eval,					\
			props::IProperty::PROPERTY_ASPECT aspect = props::IProperty::PROPERTY_ASPECT::PA_GENERIC, uint32_t flags = 0)		\
		{ props::IProperty *pp = m_pProps->CreateProperty(name, FCC); pp->SetEnumStrings(default_esval);						\
			pp->SetEnumVal(default_eval); pp->Flags().SetAll(flags); pp->SetAspect(aspect); }									\
		/* real number (float) properties */																					\
		virtual void AddParam(const TCHAR *name, uint32_t FCC, float default_dval,												\
			props::IProperty::PROPERTY_ASPECT aspect = props::IProperty::PROPERTY_ASPECT::PA_GENERIC, uint32_t flags = 0)		\
		{ props::IProperty *pp = m_pProps->CreateProperty(name, FCC); pp->SetFloat(default_dval); pp->SetAspect(aspect); 		\
			pp->Flags().SetAll(flags); pp->SetAspect(aspect); }																	\
		/* 2D vector (float) properties */																						\
		virtual void AddParam(const TCHAR *name, uint32_t FCC, props::TVec2F default_v2fval,									\
			props::IProperty::PROPERTY_ASPECT aspect = props::IProperty::PROPERTY_ASPECT::PA_GENERIC, uint32_t flags = 0)		\
		{ props::IProperty *pp = m_pProps->CreateProperty(name, FCC); pp->SetVec2F(default_v2fval); pp->SetAspect(aspect); 		\
			pp->Flags().SetAll(flags); pp->SetAspect(aspect); }																	\
		/* 3D vector (float) properties */																						\
		virtual void AddParam(const TCHAR *name, uint32_t FCC, props::TVec3F default_v3fval,									\
			props::IProperty::PROPERTY_ASPECT aspect = props::IProperty::PROPERTY_ASPECT::PA_GENERIC, uint32_t flags = 0)		\
		{ props::IProperty *pp = m_pProps->CreateProperty(name, FCC); pp->SetVec3F(default_v3fval); pp->SetAspect(aspect); 		\
			pp->Flags().SetAll(flags); pp->SetAspect(aspect); }																	\
		/* 4D vector (float) properties */																						\
		virtual void AddParam(const TCHAR *name, uint32_t FCC, props::TVec4F default_v4fval,									\
			props::IProperty::PROPERTY_ASPECT aspect = props::IProperty::PROPERTY_ASPECT::PA_GENERIC, uint32_t flags = 0)		\
		{ props::IProperty *pp = m_pProps->CreateProperty(name, FCC); pp->SetVec4F(default_v4fval); pp->SetAspect(aspect); 		\
			pp->Flags().SetAll(flags); pp->SetAspect(aspect); }																	\
		/* GUID properties */																									\
		virtual void AddParam(const TCHAR *name, uint32_t FCC, GUID default_gval,												\
			props::IProperty::PROPERTY_ASPECT aspect = props::IProperty::PROPERTY_ASPECT::PA_GENERIC, uint32_t flags = 0)		\
		{ props::IProperty *pp = m_pProps->CreateProperty(name, FCC); pp->SetGUID(default_gval); pp->SetAspect(aspect);			\
			pp->Flags().SetAll(flags); pp->SetAspect(aspect); }																	\
		virtual void SetOnCreatedHandler(OnCreatedHandler handler) { m_OnCreatedHandler = handler; }							\
		virtual void RunOnCreatedHandler(nucleus::ICommand *pcmd) const { if (m_OnCreatedHandler) m_OnCreatedHandler(pcmd); }	\
		COMMAND_CLASS_NAME(_name)(IDevice *pdev)																				\
		{																														\
			m_pProps = props::IPropertySet::CreatePropertySet();																\
			m_pDev = pdev;

#define END_COMMAND_DEFINITION(_name) } };																						\
		nucleus::ICommandInfo * COMMAND_INSTPTR_NAME(_name)


	/*
		The helpers should show up like this in your code when defining a command...

		BEGIN_COMMAND_DEFINITION(DoSomething, "This does something.", GUID({ 0x48e8115d, 0x86a1, 0x4766, { 0xa0, 0x83, 0x91, 0x82, 0x47, 0x4a, 0x51, 0xfd } })
			ADD_PARAMETER(SomeColor, 'COLR', PT_INT, PA_COLOR, 0xFF00FF00)
			ADD_PARAMETER(SomeInt, 'INT0', PT_INT, PA_GENERIC, 500)
			ADD_PARAMETER(MyFile, 'MYST', PT_STRING, PA_FILENAME, "../fizz/ban.txt")
		END_COMMAND_DEFINITION()
	*/

};
