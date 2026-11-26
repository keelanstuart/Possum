// source\RGBASlidersDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Possum.h"
#include "RGBASlidersDlg.h"
#include "afxdialogex.h"
#include "wtfpropertygridctrl.h"

// CRGBASlidersDlg dialog

IMPLEMENT_DYNAMIC(CRGBASlidersDlg, CDialog)

HHOOK CRGBASlidersDlg::s_hMouseHook = NULL;
CRGBASlidersDlg *CRGBASlidersDlg::s_pLastHookedDlg = nullptr;

CRGBASlidersDlg::CRGBASlidersDlg()
{
	for (int i = 0; i < COLOR_COUNT; i++)
		m_V[i] = 1.0;
}

CRGBASlidersDlg::~CRGBASlidersDlg()
{
	if (s_pLastHookedDlg && s_hMouseHook)
	{
		::UnhookWindowsHookEx(s_hMouseHook);
		s_hMouseHook = NULL;
		s_pLastHookedDlg = nullptr;
	}
}

void CRGBASlidersDlg::DoDataExchange(CDataExchange* pDX)
{
	const int slider[COLOR_COUNT] = {IDC_SLIDER_RED, IDC_SLIDER_GREEN, IDC_SLIDER_BLUE, IDC_SLIDER_ALPHA};
	const int edit[COLOR_COUNT] = {IDC_EDIT_RED, IDC_EDIT_GREEN, IDC_EDIT_BLUE, IDC_EDIT_ALPHA};

	for (int i = 0; i < COLOR_COUNT; i++)
	{
		DDX_Control(pDX, slider[i], m_Slider[i]);
		m_Slider[i].SetRange(0, 1000);

		DDX_Control(pDX, edit[i], m_Edit[i]);

		CString s;
		s.Format(_T("%0.02f"), m_V[i]);
		m_Edit[i].SetWindowText(s);
		DDX_Text(pDX, edit[i], m_V[i]);

//		DDV_MinMaxDouble(pDX, m_V[i], 0.0, 1.0);

		int c = int(m_V[i] * 1000.0);
		DDX_Slider(pDX, slider[i], c);
	}

	CDialog::DoDataExchange(pDX);
}

bool CRGBASlidersDlg::DoModeless(CWnd *parent, CPoint pt_topright, float init_r, float init_g, float init_b, float init_a, DWORD change_msg_id)
{
	if (s_pLastHookedDlg)
		s_pLastHookedDlg->OnClose();
	s_pLastHookedDlg = this;

	m_V[RED] = init_r;
	m_V[GREEN] = init_g;
	m_V[BLUE] = init_b;
	m_V[ALPHA] = init_a;

	if (!GetSafeHwnd())
	{
		if (!Create(IDD_DIALOG_RGBASLIDERS, parent))
			return false;
	}
	else
	{
		SetParent(parent);
	}

	CRect r;
	GetClientRect(r);
	r.OffsetRect(pt_topright.x - r.Width(), pt_topright.y);
	MoveWindow(r);
	ShowWindow(SW_SHOW);
	SetFocus();

	if (!s_hMouseHook)
		s_hMouseHook = ::SetWindowsHookEx(WH_MOUSE, MouseHookProc, 0, GetCurrentThreadId());

	return true;
}

LRESULT CALLBACK CRGBASlidersDlg::MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode != HC_ACTION)
		return CallNextHookEx(s_hMouseHook, nCode, wParam, lParam);

	MOUSEHOOKSTRUCT* lpMS = (MOUSEHOOKSTRUCT*) lParam;

	// Find a toolbar matched to the mouse hook:
	CWnd *p = CWnd::WindowFromPoint(lpMS->pt);
	bool inside = true;
	while (p && (p != s_pLastHookedDlg))
	{
		p = p->GetParent();
	}

	if (!p)
		inside = false;

	switch (wParam)
	{
		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:
			if (p != s_pLastHookedDlg)
				inside = inside;
			if (!inside)
				((CWTFPropertyGridCtrl *)s_pLastHookedDlg->GetParent())->CloseColorPopup();
			break;

		default:
			break;
	}

	return 0;
}

BEGIN_MESSAGE_MAP(CRGBASlidersDlg, CDialog)
	ON_WM_HSCROLL()
	ON_EN_CHANGE(IDC_EDIT_RED, &CRGBASlidersDlg::OnChangeEditRed)
	ON_EN_CHANGE(IDC_EDIT_GREEN, &CRGBASlidersDlg::OnChangeEditGreen)
	ON_EN_CHANGE(IDC_EDIT_BLUE, &CRGBASlidersDlg::OnChangeEditBlue)
	ON_EN_CHANGE(IDC_EDIT_ALPHA, &CRGBASlidersDlg::OnChangeEditAlpha)
END_MESSAGE_MAP()


// CRGBASlidersDlg message handlers


void CRGBASlidersDlg::OnOK()
{
}


void CRGBASlidersDlg::OnCancel()
{
	if (this == s_pLastHookedDlg)
	{
		::UnhookWindowsHookEx(s_hMouseHook);
		s_hMouseHook = NULL;
		s_pLastHookedDlg = nullptr;
	}

	ShowWindow(SW_HIDE);
//	GetParent()->RedrawWindow(NULL, NULL, RDW_UPDATENOW | RDW_ERASENOW);
	DestroyWindow();
}


void CRGBASlidersDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	HWND sbhw = pScrollBar->GetSafeHwnd();

	for (int i = 0; i < COLOR_COUNT; i++)
	{
		if (sbhw == m_Slider[i])
		{
			int v = m_Slider[i].GetPos();
			m_V[i] = float(v) / 1000.0f;

			CString s;
			s.Format(_T("%0.02f"), m_V[i]);
			m_Edit[i].SetWindowText(s);

			return;
		}
	}

	//CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}


void CRGBASlidersDlg::OnChangeEditRed()
{
	CString s;
	m_Edit[RED].GetWindowText(s);
	float d = (float)_ttof(s);
	if (d < 0 || d > 1.0f)
	{
		d = min(max(0.0f, d), 1.0f);
		s.Format(_T("%0.02f"), d);
		m_Edit[RED].SetWindowText(s);
	}
	m_V[RED] = d;
	m_Slider[RED].SetPos(int(d * 1000.0f));
}

void CRGBASlidersDlg::OnChangeEditGreen()
{
	CString s;
	m_Edit[GREEN].GetWindowText(s);
	float d = (float)_ttof(s);
	if (d < 0 || d > 1.0f)
	{
		d = min(max(0.0f, d), 1.0f);
		s.Format(_T("%0.02f"), d);
		m_Edit[GREEN].SetWindowText(s);
	}
	m_V[GREEN] = d;
	m_Slider[GREEN].SetPos(int(d * 1000.0f));
}

void CRGBASlidersDlg::OnChangeEditBlue()
{
	CString s;
	m_Edit[BLUE].GetWindowText(s);
	float d = (float)_ttof(s);
	if (d < 0 || d > 1.0f)
	{
		d = min(max(0.0f, d), 1.0f);
		s.Format(_T("%0.02f"), d);
		m_Edit[BLUE].SetWindowText(s);
	}
	m_V[BLUE] = d;
	m_Slider[BLUE].SetPos(int(d * 1000.0f));
}

void CRGBASlidersDlg::OnChangeEditAlpha()
{
	CString s;
	m_Edit[ALPHA].GetWindowText(s);
	float d = (float)_ttof(s);
	if (d < 0 || d > 1.0f)
	{
		d = min(max(0.0f, d), 1.0f);
		s.Format(_T("%0.02f"), d);
		m_Edit[ALPHA].SetWindowText(s);
	}
	m_V[ALPHA] = d;
	m_Slider[ALPHA].SetPos(int(d * 1000.0f));
}
