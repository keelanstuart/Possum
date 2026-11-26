#pragma once


// CRGBSlidersDlg dialog

class CRGBSlidersDlg : public CDialog
{
	DECLARE_DYNAMIC(CRGBSlidersDlg)

public:
	CRGBSlidersDlg();   // standard constructor
	virtual ~CRGBSlidersDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_RGBSLIDERS };
#endif

	bool DoModeless(CWnd *parent, CPoint pt_topright, double init_r, double init_g, double init_b, DWORD change_msg_id);

	void GetColors(double &r, double &g, double &b)
	{
		r = m_V[RED]; g = m_V[GREEN]; b = m_V[BLUE];
	}

protected:
	enum { RED = 0, GREEN, BLUE, COLOR_COUNT } COLORS;

	CSliderCtrl m_Slider[COLOR_COUNT];
	CEdit m_Edit[COLOR_COUNT];
	double m_V[COLOR_COUNT];

	// this message is sent to the parent when the values have changed... 
	DWORD m_ChangeMessage;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg BOOL OnNcActivate(BOOL bActive);
	virtual void OnOK();
	virtual void OnCancel();
};
