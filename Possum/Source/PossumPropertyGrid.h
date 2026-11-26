#pragma once

#include "wtfpropertygridctrl.h"
#include <vector>

namespace nucleus { class ICommand; }
class CPossumDocument;

// CPossumPropertyGrid

class CPossumPropertyGrid : public CWTFPropertyGridCtrl
{
	DECLARE_DYNAMIC(CPossumPropertyGrid)

public:
	CPossumPropertyGrid();
	virtual ~CPossumPropertyGrid();

	void SetActiveProperties(props::IPropertySet *props, bool readonly = false, const TCHAR *title = nullptr);
	void SetMultipleProperties(CPossumDocument *doc, bool readonly = false, const TCHAR *title = nullptr);

	CWTFPropertyGridProperty *FindItemByName(const TCHAR *name, CWTFPropertyGridProperty *top = nullptr);
	void SetPopupDlg(CDialog *popup) { m_pPopup = popup; }

protected:
	props::IPropertySet *m_Props = nullptr;
	CDialog *m_pPopup;

	// Possum-only multi-edit. These command pointers are non-owning and are
	// replaced whenever the active selection is changed.
	CPossumDocument *m_MultiDoc = nullptr;
	std::vector<nucleus::ICommand *> m_MultiCommands;
	bool m_ReadOnly = false;
	void BuildProperties(props::IPropertySet *props, bool readonly, const TCHAR *title);
	bool IsSharedProperty(props::IProperty *first, bool &mixed) const;
	bool IsMixedComponent(props::IProperty *first, int axis) const;
	void OnMultiplePropertyChanged(CWTFPropertyGridProperty *property);
	afx_msg LRESULT OnRebuildMultiProperties(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()

public:
	virtual void OnPropertyChanged(CWTFPropertyGridProperty* pProp);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
};


