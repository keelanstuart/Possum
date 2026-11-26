#pragma once


// CCommandListFilterControl

class CCommandListFilterControl : public CWnd
{
	DECLARE_DYNAMIC(CCommandListFilterControl)

public:
	CCommandListFilterControl();
	virtual ~CCommandListFilterControl();

	void FillFilterDropdown();

	BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = nullptr);

protected:
	CComboBox m_ddFilters;
	CButton m_butCfgFilters;
	CFont m_Font;

	DECLARE_MESSAGE_MAP()
public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType,int cx,int cy);
	afx_msg void OnCfgClicked();
	afx_msg void OnFilterChanged();
};


