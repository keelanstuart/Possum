// Source\TimeBarCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "Possum.h"
#include "TimeBarCtrl.h"
#include "PossumDocView.h"
#include <gdiplus.h>
#include "Util.h"


// CTimeBarCtrl

IMPLEMENT_DYNAMIC(CTimeBarCtrl, CWnd)

CTimeBarCtrl::CTimeBarCtrl()
{
}

CTimeBarCtrl::~CTimeBarCtrl()
{
}


BEGIN_MESSAGE_MAP(CTimeBarCtrl, CWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()



// CTimeBarCtrl message handlers



void CTimeBarCtrl::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	CMemDC mdc(dc, this);
	Gdiplus::Graphics gr(mdc.GetDC());

	Gdiplus::SolidBrush br_bg(Gdiplus::Color(255, 0, 0, 0));
	Gdiplus::SolidBrush br_text(Gdiplus::Color(255, 255, 255, 255));
	Gdiplus::SolidBrush br_seltime(Gdiplus::Color(128, 255, 0, 255));
	Gdiplus::SolidBrush br_seltimeint(Gdiplus::Color(255, 64, 0, 64));
	Gdiplus::SolidBrush br_playtime(Gdiplus::Color(128, 0, 255, 0));
	Gdiplus::SolidBrush br_playtimeint(Gdiplus::Color(255, 0, 64, 0));
	Gdiplus::Pen pn_seltime(&br_seltime, 3);
	Gdiplus::Pen pn_seltimebord(&br_seltime, 1);
	Gdiplus::Pen pn_playtime(&br_playtime, 3);
	Gdiplus::Pen pn_playtimebord(&br_playtime, 1);

	Gdiplus::Font f_time(_T("Arial"), 7);

	CRect rcli;
	GetClientRect(rcli);

	gr.FillRectangle(&br_bg, rcli.left, rcli.top, rcli.Width(), rcli.Height());

	double start_time = ((CPossumDocView *)GetParent())->GetDisplayStartTime();
	double end_time = ((CPossumDocView *)GetParent())->GetDisplayEndTime();
	double d = end_time - start_time;
	double pixels_per_second = double(rcli.Width()) / d;

	CString tt;
	UINT h, m, s, f;
	Gdiplus::RectF tr;

	INT y = rcli.Height() / 2;

	double pixels_per_period[] = {1.0, 10.0, 30.0, 60.0, 300.0, 600.0, 900.0, 1800.0, 3600.0};
	const int num_periods = sizeof(pixels_per_period) / sizeof(double);

	int start_period = 0, end_period;

	for (end_period = 1; end_period < num_periods; end_period++)
	{
		if ((pixels_per_period[end_period] >= d) || (end_period >= (num_periods - 1)))
			break;
	}

	start_period = max(0, (end_period - 3));

	for (int q = start_period; q <= end_period; q++)
	{
		int brightness = 50 + (q * 20);

		pixels_per_period[q] *= pixels_per_second;

		double stdiff = start_time / pixels_per_period[q];
		double stw = ceil(stdiff);
		double stf = floor(stdiff) - stdiff;

		INT x = rcli.left + INT(stf * pixels_per_period[q]);
		INT xi = INT(pixels_per_period[q]);

		Gdiplus::SolidBrush br_tick(Gdiplus::Color(255, brightness, brightness, brightness));
		Gdiplus::Pen pn_tick(&br_tick, Gdiplus::REAL(std::max<int>(1, q / 2)));

		while (x < rcli.right)
		{
			gr.DrawLine(&pn_tick, x, y, x, rcli.bottom);

			x += xi;
		}

		//y -= 2;
	}

	GetTimeComponents(start_time, h, m, s, f);
	tt.Format(_T("%02d:%02d:%02d.%01d"), h, m, s, f);
	gr.MeasureString((LPCTSTR)tt, tt.GetLength(), &f_time, Gdiplus::PointF(0.0, 0.0), &tr);
	gr.DrawString(tt, tt.GetLength(), &f_time, Gdiplus::PointF(1.0, 2.0), &br_text);

	GetTimeComponents(end_time, h, m, s, f);
	tt.Format(_T("%02d:%02d:%02d.%01d"), h, m, s, f);
	gr.MeasureString((LPCTSTR)tt, tt.GetLength(), &f_time, Gdiplus::PointF(0.0, 0.0), &tr);
	gr.DrawString(tt, tt.GetLength(), &f_time, Gdiplus::PointF(Gdiplus::REAL(rcli.right) - tr.Width, 2.0), &br_text);

	double seltime = ((CPossumDocView *)GetParent())->GetSelectedTime();
	{
		INT sx = (int)((seltime - start_time) * pixels_per_second);
		gr.DrawLine(&pn_seltime, Gdiplus::Point(sx, 0), Gdiplus::Point(sx, rcli.bottom));

		if (((CPossumDocView *)GetParent())->IsShowingSelectedTime())
		{
			GetTimeComponents(seltime, h, m, s, f);
			tt.Format(_T("%02d:%02d:%02d.%01d"), h, m, s, f);
			gr.MeasureString((LPCTSTR)tt, tt.GetLength(), &f_time, Gdiplus::PointF(0.0, 0.0), &tr);

			Gdiplus::RectF selrect(Gdiplus::REAL(sx + 2), Gdiplus::REAL(rcli.top + 2), tr.Width + 4, Gdiplus::REAL(rcli.Height() - 4));

			if (sx > (rcli.Width() / 2))
				selrect.X -= selrect.Width;

			gr.FillRectangle(&br_seltimeint, selrect);
			gr.DrawRectangle(&pn_seltimebord, selrect);

			gr.DrawString(tt, tt.GetLength(), &f_time, Gdiplus::PointF(selrect.X + 1, selrect.Y), &br_text);
		}
	}

	double playtime = ((CPossumDocView *)GetParent())->GetPlayingTime();
	if ((playtime >= start_time) && (playtime <= end_time))
	{
		INT sx = (int)((playtime - start_time) * pixels_per_second);
		gr.DrawLine(&pn_playtime, Gdiplus::Point(sx, 0), Gdiplus::Point(sx, rcli.bottom));
	}

	dc.BitBlt(0, 0, rcli.Width(), rcli.Height(), &(mdc.GetDC()), 0, 0, SRCCOPY);
}


BOOL CTimeBarCtrl::OnEraseBkgnd(CDC* pDC)
{
	return FALSE; //CWnd::OnEraseBkgnd(pDC);
}


int CTimeBarCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	return 0;
}


BOOL CTimeBarCtrl::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext)
{
	return CWnd::Create(GetGlobalData()->RegisterWindowClass(_T("CTimeBarCtrl")), _T(""), dwStyle, rect, pParentWnd, nID, pContext);
}


BOOL CTimeBarCtrl::PreCreateWindow(CREATESTRUCT& cs)
{
	return CWnd::PreCreateWindow(cs);
}


void CTimeBarCtrl::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_NOERASE | RDW_UPDATENOW);
}


void CTimeBarCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	double start_time = ((CPossumDocView *)GetParent())->GetDisplayStartTime();
	double end_time = ((CPossumDocView *)GetParent())->GetDisplayEndTime();

	CRect r;
	GetClientRect(r);
	double seltime = double(point.x) / double(r.Width()) * (end_time - start_time) + start_time;
	((CPossumDocView *)GetParent())->SetSelectedTime(seltime);

	CWnd::OnLButtonDown(nFlags, point);
}


void CTimeBarCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	if (nFlags & MK_LBUTTON)
	{
		double start_time = ((CPossumDocView *)GetParent())->GetDisplayStartTime();
		double end_time = ((CPossumDocView *)GetParent())->GetDisplayEndTime();

		CRect r;
		GetClientRect(r);
		double seltime = double(point.x) / double(r.Width()) * (end_time - start_time) + start_time;
		((CPossumDocView *)GetParent())->SetSelectedTime(seltime);
		((CPossumDocView *)GetParent())->ShowSelectedTime(true);
	}
	else
	{
		((CPossumDocView *)GetParent())->ShowSelectedTime(false);
	}

	CWnd::OnMouseMove(nFlags, point);
}
