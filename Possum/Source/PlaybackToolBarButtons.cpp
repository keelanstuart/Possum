// PlayPauseToolBarButton.cpp : implementation file
//

#include "stdafx.h"
#include "Possum.h"
#include "PlaybackToolBarButtons.h"
#include "PossumDocView.h"
#include "Util.h"
#include <NuDevice.h>


// CPlayPauseToolBarButton

IMPLEMENT_SERIAL(CPlayPauseToolBarButton, CMFCToolBarButton, VERSIONABLE_SCHEMA | 1)


CPlayPauseToolBarButton::CPlayPauseToolBarButton() : CMFCToolBarButton()
{
}

CPlayPauseToolBarButton::CPlayPauseToolBarButton(UINT uiID, int iImage, LPCTSTR lpszText, BOOL bUserButton, BOOL bLocked) : CMFCToolBarButton(uiID, iImage, lpszText, bUserButton, bLocked)
{
}

CPlayPauseToolBarButton::~CPlayPauseToolBarButton()
{
}

void CPlayPauseToolBarButton::OnDraw(CDC *pDC, const CRect &rect, CMFCToolBarImages *pImages, BOOL bHorz, BOOL bCustomizeMode, BOOL bHighlight, BOOL bDrawBorder, BOOL bGrayDisabledButtons)
{
	CPaintDC *pdc = (CPaintDC *)CDC::FromHandle(pDC->GetSafeHdc());

	Gdiplus::Graphics gr(*pdc);

	CPossumDocument *pdoc = theApp.m_DocMan.GetActiveDocument();
	CPossumDocView *pview = pdoc ? pdoc->GetView() : nullptr;

	UINT picres = IDR_PAUSE_BUTTON;
	if (theApp.m_Player->GetPlayState() != nucleus::IPlayer::State::Playing)
	{
		picres = (pview && pview->GetSelectedTime() > 0.0) ? IDR_CUEPLAY_BUTTON : IDR_PLAY_BUTTON;
	}

	Gdiplus::Image *img = (Gdiplus::Image *)theApp.m_PicMan.GetResPic(picres);
	if (img)
		gr.DrawImage(img, rect.left, rect.top, rect.Width(), rect.Height());
}



IMPLEMENT_SERIAL(CRewindToolBarButton, CMFCToolBarButton, VERSIONABLE_SCHEMA | 1)

CRewindToolBarButton::CRewindToolBarButton() : CMFCToolBarButton()
{
}

CRewindToolBarButton::CRewindToolBarButton(UINT uiID, int iImage, LPCTSTR lpszText, BOOL bUserButton, BOOL bLocked) : CMFCToolBarButton(uiID, iImage, lpszText, bUserButton, bLocked)
{
}

CRewindToolBarButton::~CRewindToolBarButton()
{
}

void CRewindToolBarButton::OnDraw(CDC *pDC, const CRect &rect, CMFCToolBarImages *pImages, BOOL bHorz, BOOL bCustomizeMode, BOOL bHighlight, BOOL bDrawBorder, BOOL bGrayDisabledButtons)
{
	CPaintDC *pdc = (CPaintDC *)CDC::FromHandle(pDC->GetSafeHdc());

	Gdiplus::Graphics gr(*pdc);

	CPossumDocument *pdoc = theApp.m_DocMan.GetActiveDocument();
	CPossumDocView *pview = pdoc ? pdoc->GetView() : nullptr;

	Gdiplus::Image *img = (Gdiplus::Image *)theApp.m_PicMan.GetResPic(IDR_REWIND_BUTTON);
	if (img)
		gr.DrawImage(img, rect.left, rect.top, rect.Width(), rect.Height());
}



IMPLEMENT_SERIAL(CStopToolBarButton, CMFCToolBarButton, VERSIONABLE_SCHEMA | 1)

CStopToolBarButton::CStopToolBarButton() : CMFCToolBarButton()
{
}

CStopToolBarButton::CStopToolBarButton(UINT uiID, int iImage, LPCTSTR lpszText, BOOL bUserButton, BOOL bLocked) : CMFCToolBarButton(uiID, iImage, lpszText, bUserButton, bLocked)
{
}

CStopToolBarButton::~CStopToolBarButton()
{
}

void CStopToolBarButton::OnDraw(CDC *pDC, const CRect &rect, CMFCToolBarImages *pImages, BOOL bHorz, BOOL bCustomizeMode, BOOL bHighlight, BOOL bDrawBorder, BOOL bGrayDisabledButtons)
{
	CPaintDC *pdc = (CPaintDC *)CDC::FromHandle(pDC->GetSafeHdc());

	Gdiplus::Graphics gr(*pdc);

	CPossumDocument *pdoc = theApp.m_DocMan.GetActiveDocument();
	CPossumDocView *pview = pdoc ? pdoc->GetView() : nullptr;

	Gdiplus::Image *img = (Gdiplus::Image *)theApp.m_PicMan.GetResPic(IDR_STOP_BUTTON);
	if (img)
		gr.DrawImage(img, rect.left, rect.top, rect.Width(), rect.Height());
}


IMPLEMENT_SERIAL(CLoopToolBarButton, CMFCToolBarButton, VERSIONABLE_SCHEMA | 1)

CLoopToolBarButton::CLoopToolBarButton() : CMFCToolBarButton()
{
}

CLoopToolBarButton::CLoopToolBarButton(UINT uiID, int iImage, LPCTSTR lpszText, BOOL bUserButton, BOOL bLocked) : CMFCToolBarButton(uiID, iImage, lpszText, bUserButton, bLocked)
{
}

CLoopToolBarButton::~CLoopToolBarButton()
{
}

void CLoopToolBarButton::OnDraw(CDC *pDC, const CRect &rect, CMFCToolBarImages *pImages, BOOL bHorz, BOOL bCustomizeMode, BOOL bHighlight, BOOL bDrawBorder, BOOL bGrayDisabledButtons)
{
	CPaintDC *pdc = (CPaintDC *)CDC::FromHandle(pDC->GetSafeHdc());

	Gdiplus::Graphics gr(*pdc);

	CPossumDocument *pdoc = theApp.m_DocMan.GetActiveDocument();
	CPossumDocView *pview = pdoc ? pdoc->GetView() : nullptr;

	UINT picres = theApp.m_LoopPlayback ? IDR_LOOP_BUTTON : IDR_LOOPOFF_BUTTON;

	Gdiplus::Image *img = (Gdiplus::Image *)theApp.m_PicMan.GetResPic(picres);
	if (img)
		gr.DrawImage(img, rect.left, rect.top, rect.Width(), rect.Height());
}



IMPLEMENT_SERIAL(CTimeToolBarButton, CMFCToolBarButton, VERSIONABLE_SCHEMA | 1)

CTimeToolBarButton::CTimeToolBarButton() : CMFCToolBarButton()
{
}

CTimeToolBarButton::CTimeToolBarButton(UINT uiID, int iImage, LPCTSTR lpszText, BOOL bUserButton, BOOL bLocked) : CMFCToolBarButton(uiID, iImage, lpszText, bUserButton, bLocked)
{
}

CTimeToolBarButton::~CTimeToolBarButton()
{
}

void CTimeToolBarButton::OnDraw(CDC *pDC, const CRect &rect, CMFCToolBarImages *pImages, BOOL bHorz, BOOL bCustomizeMode, BOOL bHighlight, BOOL bDrawBorder, BOOL bGrayDisabledButtons)
{
	CPaintDC *pdc = (CPaintDC *)CDC::FromHandle(pDC->GetSafeHdc());

	Gdiplus::Graphics gr(*pdc);

	CPossumDocument *pdoc = theApp.m_DocMan.GetActiveDocument();
	CPossumDocView *pview = pdoc ? pdoc->GetView() : nullptr;

    if (!pdoc || !pview)
        return;

	Gdiplus::LinearGradientBrush br_bg(Gdiplus::Point(0, 0), Gdiplus::Point(rect.Width() + 1, 10), Gdiplus::Color(230, 22, 22, 22), Gdiplus::Color(200, 8, 8, 8));
	Gdiplus::SolidBrush br_text(Gdiplus::Color(255, 255, 255, 255));
	Gdiplus::SolidBrush br_seltime(Gdiplus::Color(255, 255, 128, 255));
	Gdiplus::SolidBrush br_ibg(Gdiplus::Color(255, 0, 0, 0));
	Gdiplus::SolidBrush br_played(Gdiplus::Color(255, 192, 255, 128));
	Gdiplus::SolidBrush br_fadepeak(Gdiplus::Color(255, 0, 192, 0));
	Gdiplus::SolidBrush br_peak(Gdiplus::Color(255, 0, 255, 0));
	Gdiplus::SolidBrush br_clipped(Gdiplus::Color(255, 255, 0, 0));
	Gdiplus::Font f_time(_T("Arial"), 7);

	gr.FillRectangle(&br_bg, rect.left, rect.top, rect.Width(), rect.Height());

	CString tt;
	UINT h[2], m[2], s[2], f[2];
	Gdiplus::RectF tr((float)rect.left, (float)rect.top, (float)rect.Width(), (float)rect.Height());

	Gdiplus::REAL ledge = rect.left + 1.0f;
	Gdiplus::REAL ledgebg = ledge + 1.0f;

	Gdiplus::REAL tedge = Gdiplus::REAL(rect.top);// +1.0;
	Gdiplus::REAL tedgebg = tedge + 1.0f;

	tt = theApp.m_DocMan.GetActiveDocument()->GetTitle();
	gr.MeasureString((LPCTSTR)tt, tt.GetLength(), &f_time, Gdiplus::PointF(0.0, 0.0), &tr);
	gr.DrawString(tt, tt.GetLength(), &f_time, Gdiplus::PointF(ledgebg, tedgebg), &br_ibg);
	gr.DrawString(tt, tt.GetLength(), &f_time, Gdiplus::PointF(ledge, tedge), &br_text);

	tedge -= 1.0;
	tedgebg -= 1.0;

	Gdiplus::REAL vspc = tr.Height;// +1.0;

	GetTimeComponents(pview->GetPlayingTime(), h[0], m[0], s[0], f[0]);
	GetTimeComponents(theApp.m_DocMan.GetActiveDocument()->GetTrackSet()->Length(), h[1], m[1], s[1], f[1]);
	tt.Format(_T("%02d:%02d:%02d.%01d / %02d:%02d:%02d.%01d"), h[0], m[0], s[0], f[0], h[1], m[1], s[1], f[1]);
	gr.MeasureString((LPCTSTR)tt, tt.GetLength(), &f_time, Gdiplus::PointF(0.0, 0.0), &tr);
	gr.DrawString(tt, tt.GetLength(), &f_time, Gdiplus::PointF(ledgebg, tedgebg + vspc), &br_ibg);
	gr.DrawString(tt, tt.GetLength(), &f_time, Gdiplus::PointF(ledge, tedge + vspc), &br_text);

	GetTimeComponents(pview->GetSelectedTime(), h[0], m[0], s[0], f[0]);
	tt.Format(_T("%02d:%02d:%02d.%01d"), h[0], m[0], s[0], f[0]);
	gr.MeasureString((LPCTSTR)tt, tt.GetLength(), &f_time, Gdiplus::PointF(0.0, 0.0), &tr);
	gr.DrawString(tt, tt.GetLength(), &f_time, Gdiplus::PointF(ledgebg, tedgebg + (vspc * 2.0f)), &br_ibg);
	gr.DrawString(tt, tt.GetLength(), &f_time, Gdiplus::PointF(ledge, tedge + (vspc * 2.0f)), &br_seltime);

	Gdiplus::REAL pct = std::min<Gdiplus::REAL>(1.0, ((Gdiplus::REAL)pview->GetPlayingTime() / (Gdiplus::REAL)theApp.m_DocMan.GetActiveDocument()->GetTrackSet()->Length()));
	gr.FillRectangle(&br_ibg, ledgebg, (Gdiplus::REAL)rect.bottom - 7.0f, (Gdiplus::REAL)rect.Width() - Gdiplus::REAL(4.0), Gdiplus::REAL(5.0));
	gr.FillRectangle(&br_played, ledgebg, (Gdiplus::REAL)rect.bottom - 7.0f, ((Gdiplus::REAL)rect.Width() - Gdiplus::REAL(4.0)) * pct, Gdiplus::REAL(5.0));

	auto doc = theApp.m_DocMan.GetActiveDocument();

	nucleus::IDevice *paudio = theApp.m_pDevMan->GetDeviceByName(_T("Audio"));
	if (paudio)
	{
		props::IProperty *pml = paudio->GetProperties()->GetPropertyById('MTRL');
		props::IProperty *pmr = paudio->GetProperties()->GetPropertyById('MTRR');
		float ml = pml ? pml->AsFloat() : 0.0f;
		float mr = pmr ? pmr->AsFloat() : 0.0f;
		m_MeterLeft = std::max<float>(ml, m_MeterLeft * 0.85f);
		m_MeterRight = std::max<float>(mr, m_MeterRight * 0.85f);

		gr.FillRectangle(ml > 1.0f ? &br_clipped : &br_peak, rect.right - 16.0f, rect.top + 2.0f, 5.0f, ((Gdiplus::REAL)rect.Height() - 11.0f));
		gr.FillRectangle(&br_fadepeak, rect.right - 16.0f, rect.top + 2.0f, 5.0f, (1.0f - ml) * ((Gdiplus::REAL)rect.Height() - 12.0f));
		gr.FillRectangle(&br_ibg, rect.right - 16.0f, rect.top + 2.0f, 5.0f, (1.0f - m_MeterLeft) * ((Gdiplus::REAL)rect.Height() - 12.0f));

		gr.FillRectangle(mr > 1.0f ? &br_clipped : &br_peak, rect.right - 7.0f, rect.top + 2.0f, 5.0f, ((Gdiplus::REAL)rect.Height() - 11.0f));
		gr.FillRectangle(&br_fadepeak, rect.right - 7.0f, rect.top + 2.0f, 5.0f, (1.0f - mr) * ((Gdiplus::REAL)rect.Height() - 12.0f));
		gr.FillRectangle(&br_ibg, rect.right - 7.0f, rect.top + 2.0f, 5.0f, (1.0f - m_MeterRight) * ((Gdiplus::REAL)rect.Height() - 12.0f));
	}
}

SIZE CTimeToolBarButton::OnCalculateSize(CDC *pDC, const CSize &sizeDefault, BOOL bHorz)
{
	ASSERT_VALID(pDC);

	if (!IsVisible())
		return CSize(0, 0);

	CSize size = sizeDefault;

	CSize sizeExtra = m_bExtraSize ? CMFCVisualManager::GetInstance()->GetButtonExtraBorder() : CSize(0, 0);

	size.cx *= 4;

	if (!bHorz)
	{
		std::swap(size.cx, size.cy);
	}

	return size;
}
