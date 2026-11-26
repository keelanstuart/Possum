#pragma once


// CRGBASlidersDlg dialog

class CRGBASlidersDlg : public CDialog
{
	DECLARE_DYNAMIC(CRGBASlidersDlg)

public:
	CRGBASlidersDlg();   // standard constructor
	virtual ~CRGBASlidersDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_RGBASLIDERS };
#endif

	bool DoModeless(CWnd *parent, CPoint pt_topright, float init_r, float init_g, float init_b, float init_a, DWORD change_msg_id);

	void GetColors(float &r, float &g, float &b, float &a)
	{
		r = m_V[RED]; g = m_V[GREEN]; b = m_V[BLUE]; a = m_V[ALPHA];
	}

protected:
	enum { RED = 0, GREEN, BLUE, ALPHA, COLOR_COUNT } COLORS;

	CSliderCtrl m_Slider[COLOR_COUNT];
	CEdit m_Edit[COLOR_COUNT];
	float m_V[COLOR_COUNT];

	// this message is sent to the parent when the values have changed... 
	DWORD m_ChangeMessage;

	static HHOOK s_hMouseHook;
	static CRGBASlidersDlg *s_pLastHookedDlg;
	static LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam);

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnChangeEditRed();
	afx_msg void OnChangeEditGreen();
	afx_msg void OnChangeEditBlue();
	afx_msg void OnChangeEditAlpha();
};
