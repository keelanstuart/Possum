#pragma once

#include "wtfpropertygridctrl.h"

// CGenPropEditDlg dialog

class CGenPropEditDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CGenPropEditDlg)

public:
	CGenPropEditDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CGenPropEditDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_GENPROPEDITDLG };
#endif

public:
	props::IPropertySet *GetProperties();

protected:
	props::IPropertySet *m_pProps;

	CWTFPropertyGridCtrl m_PropGrid;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
};
