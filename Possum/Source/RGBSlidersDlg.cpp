// source\RGBSlidersDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Possum.h"
#include "RGBSlidersDlg.h"
#include "afxdialogex.h"


// CRGBSlidersDlg dialog

IMPLEMENT_DYNAMIC(CRGBSlidersDlg, CDialog)

CRGBSlidersDlg::CRGBSlidersDlg()
{
	for (int i = 0; i < COLOR_COUNT; i++)
		m_V[i] = 1.0;
}

CRGBSlidersDlg::~CRGBSlidersDlg()
{
}

void CRGBSlidersDlg::DoDataExchange(CDataExchange* pDX)
{
	DDX_Control(pDX, IDC_SLIDER_RED, m_Slider[RED]);
	DDX_Control(pDX, IDC_SLIDER_GREEN, m_Slider[GREEN]);
	DDX_Control(pDX, IDC_SLIDER_BLUE, m_Slider[BLUE]);
	for (int i = 0; i < COLOR_COUNT; i++)
		m_Slider[i].SetRange(0, 1000);

	DDX_Control(pDX, IDC_EDIT_RED, m_Edit[RED]);
	DDX_Control(pDX, IDC_EDIT_GREEN, m_Edit[GREEN]);
	DDX_Control(pDX, IDC_EDIT_BLUE, m_Edit[BLUE]);

	DDX_Text(pDX, IDC_EDIT_RED, m_V[RED]);
	DDX_Text(pDX, IDC_EDIT_GREEN, m_V[GREEN]);
	DDX_Text(pDX, IDC_EDIT_BLUE, m_V[BLUE]);

	for (int i = 0; i < COLOR_COUNT; i++)
		DDV_MinMaxDouble(pDX, m_V[i], 0.0, 1.0);

	int r = m_V[RED] * 1000.0;
	DDX_Slider(pDX, IDC_SLIDER_RED, r);

	int g = m_V[GREEN] * 1000.0;
	DDX_Slider(pDX, IDC_SLIDER_RED, g);

	int b = m_V[BLUE] * 1000.0;
	DDX_Slider(pDX, IDC_SLIDER_RED, b);

	CDialog::DoDataExchange(pDX);
}

bool CRGBSlidersDlg::DoModeless(CWnd *parent, CPoint pt_topright, double init_r, double init_g, double init_b, DWORD change_msg_id)
{
	m_V[RED] = init_r;
	m_V[GREEN] = init_g;
	m_V[BLUE] = init_b;

	if (!Create(IDD_DIALOG_RGBSLIDERS, parent))
		return false;

	CRect r;
	GetClientRect(r);
	pt_topright.x -= r.Width();
	MoveWindow(r);
	ShowWindow(SW_SHOW);

	return true;
}


BEGIN_MESSAGE_MAP(CRGBSlidersDlg, CDialog)
	ON_WM_NCACTIVATE()
END_MESSAGE_MAP()


// CRGBSlidersDlg message handlers


BOOL CRGBSlidersDlg::OnNcActivate(BOOL bActive)
{
	BOOL ret = CDialog::OnNcActivate(bActive);

	DestroyWindow();

	return ret;
}


void CRGBSlidersDlg::OnOK()
{
}


void CRGBSlidersDlg::OnCancel()
{
	CDialog::OnCancel();
}
