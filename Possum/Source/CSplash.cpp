#include "stdafx.h"
#include "CSplash.h"

#include <algorithm>

IMPLEMENT_DYNAMIC(CSplash, CWnd)

BEGIN_MESSAGE_MAP(CSplash, CWnd)
    ON_WM_TIMER()
    ON_WM_DESTROY()
    ON_WM_ERASEBKGND()
    ON_WM_PAINT()
    ON_WM_MOUSEACTIVATE()
END_MESSAGE_MAP()

CSplash::CSplash() = default;

CSplash::~CSplash()
{
    CloseNow();
    ReleaseSurface();
}

BOOL CSplash::CreateSplash(Gdiplus::Image *logo, DWORD minimumDisplayMs)
{
    if (GetSafeHwnd() || !logo || logo->GetLastStatus() != Gdiplus::Ok ||
        logo->GetWidth() == 0 || logo->GetHeight() == 0)
        return FALSE;

    m_Logo.reset(logo->Clone());
    if (!m_Logo || m_Logo->GetLastStatus() != Gdiplus::Ok)
    {
        m_Logo.reset();
        return FALSE;
    }

    m_LogoWidth = static_cast<int>(std::min<UINT>(m_Logo->GetWidth(), 480));
    m_LogoHeight = static_cast<int>(std::min<UINT>(m_Logo->GetHeight(), 480));
    m_Width = std::max<int>(360, m_LogoWidth + 48);
    m_Height = m_LogoHeight + 32 + 62;

    m_MinimumDisplayMs = minimumDisplayMs;
    m_FadeOutStartTick = 0;
    m_FinishRequested = false;
    m_Status = _T("Starting Possum...");
    m_Opacity = 0;

    POINT cursor{};
    ::GetCursorPos(&cursor);
    MONITORINFO monitor{};
    monitor.cbSize = sizeof(monitor);
    const HMONITOR hm = ::MonitorFromPoint(cursor, MONITOR_DEFAULTTOPRIMARY);
    if (!::GetMonitorInfo(hm, &monitor))
    {
        monitor.rcMonitor = {0, 0, ::GetSystemMetrics(SM_CXSCREEN), ::GetSystemMetrics(SM_CYSCREEN)};
    }

    const int x = monitor.rcMonitor.left + (monitor.rcMonitor.right - monitor.rcMonitor.left - m_Width) / 2;
    const int y = monitor.rcMonitor.top + (monitor.rcMonitor.bottom - monitor.rcMonitor.top - m_Height) / 2;
    const CRect location(x, y, x + m_Width, y + m_Height);

    LPCTSTR cls = AfxRegisterWndClass(0, ::LoadCursor(nullptr, IDC_ARROW), nullptr, nullptr);
    if (!CreateEx(WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
                  cls, _T("Playing Possum"), WS_POPUP, location, nullptr, 0))
    {
        m_Logo.reset();
        return FALSE;
    }

    if (!CreateSurface())
    {
        CloseNow();
        return FALSE;
    }

    DrawSurface();
    Present(0);
    ShowWindow(SW_SHOWNOACTIVATE);

    // A short synchronous fade-in makes the splash genuinely visible even
    // though InitInstance() has not entered MFC's ordinary message loop yet.
    m_StartTick = ::GetTickCount64();
    while (GetSafeHwnd() && ::GetTickCount64() - m_StartTick < kFadeInMs)
    {
        AdvanceAnimation();
        ::Sleep(15);
    }
    AdvanceAnimation();

    SetTimer(kTimerId, kTimerPeriodMs, nullptr);
    return TRUE;
}

bool CSplash::CreateSurface()
{
    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = m_Width;
    bmi.bmiHeader.biHeight = -m_Height; // Top-down: GDI+ and the DIB agree on orientation.
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    HDC screenDC = ::GetDC(nullptr);
    m_Dib = ::CreateDIBSection(screenDC, &bmi, DIB_RGB_COLORS, &m_Pixels, nullptr, 0);
    ::ReleaseDC(nullptr, screenDC);
    if (!m_Dib || !m_Pixels)
        return false;

    m_MemoryDC = ::CreateCompatibleDC(nullptr);
    if (!m_MemoryDC)
        return false;

    m_PreviousBitmap = ::SelectObject(m_MemoryDC, m_Dib);
    if (!m_PreviousBitmap || m_PreviousBitmap == HGDI_ERROR)
        return false;

    // GDI+ draws directly into the DIB in PREMULTIPLIED alpha format,
    // which UpdateLayeredWindow(ULW_ALPHA) specifically requires.
    m_Surface = std::make_unique<Gdiplus::Bitmap>(
        m_Width, m_Height, m_Width * 4,
        PixelFormat32bppPARGB, static_cast<BYTE*>(m_Pixels));

    return m_Surface && m_Surface->GetLastStatus() == Gdiplus::Ok;
}

void CSplash::ReleaseSurface()
{
    m_Surface.reset(); // Release GDI+'s view before deleting the DIB storage.
    if (m_MemoryDC && m_PreviousBitmap && m_PreviousBitmap != HGDI_ERROR)
        ::SelectObject(m_MemoryDC, m_PreviousBitmap);

	m_PreviousBitmap = nullptr;
    if (m_Dib)
        ::DeleteObject(m_Dib);

	m_Dib = nullptr;
    m_Pixels = nullptr;

	if (m_MemoryDC)
        ::DeleteDC(m_MemoryDC);

	m_MemoryDC = nullptr;
}

void CSplash::DrawSurface()
{
    if (!m_Surface || !m_Logo)
        return;

    Gdiplus::Graphics gr(m_Surface.get());

	gr.Clear(Gdiplus::Color(0, 0, 0, 0));
    gr.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    gr.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    gr.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);

    const Gdiplus::Rect imageRect((m_Width - m_LogoWidth) / 2, 16, m_LogoWidth, m_LogoHeight);
    gr.DrawImage(m_Logo.get(), imageRect);

    const Gdiplus::Rect statusBox(18, m_Height - 53, m_Width - 36, 40);
    constexpr int radius = 12;
	Gdiplus::GraphicsPath rounded;
    rounded.AddArc(statusBox.X, statusBox.Y, radius * 2, radius * 2, 180, 90);
    rounded.AddArc(statusBox.GetRight() - radius * 2, statusBox.Y, radius * 2, radius * 2, 270, 90);
    rounded.AddArc(statusBox.GetRight() - radius * 2, statusBox.GetBottom() - radius * 2, radius * 2, radius * 2, 0, 90);
    rounded.AddArc(statusBox.X, statusBox.GetBottom() - radius * 2, radius * 2, radius * 2, 90, 90);
    rounded.CloseFigure();

    Gdiplus::SolidBrush background(Gdiplus::Color(202, 25, 28, 34));
    Gdiplus::Pen outline(Gdiplus::Color(100, 185, 190, 198), 1.0f);
    gr.FillPath(&background, &rounded);
    gr.DrawPath(&outline, &rounded);

    Gdiplus::Font font(L"Segoe UI", 16.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    Gdiplus::SolidBrush foreground(Gdiplus::Color(255, 236, 239, 243));
    Gdiplus::StringFormat format;
    format.SetAlignment(Gdiplus::StringAlignmentCenter);
    format.SetLineAlignment(Gdiplus::StringAlignmentCenter);
    format.SetTrimming(Gdiplus::StringTrimmingEllipsisCharacter);
    format.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);
    const Gdiplus::RectF statusRect(
        float(statusBox.X + 12), float(statusBox.Y + 3),
        float(statusBox.Width - 24), float(statusBox.Height - 6));
    const CStringW wideStatus(m_Status);

	gr.DrawString(wideStatus, -1, &font, statusRect, &format, &foreground);
}

void CSplash::Present(BYTE opacity)
{
    if (!GetSafeHwnd() || !m_MemoryDC)
        return;

    CRect window;
    GetWindowRect(&window);
    POINT destination{window.left, window.top};
    POINT source{0, 0};
    SIZE size{m_Width, m_Height};
    BLENDFUNCTION blend{AC_SRC_OVER, 0, opacity, AC_SRC_ALPHA};

    HDC screenDC = ::GetDC(nullptr);
    if (screenDC)
    {
        ::UpdateLayeredWindow(m_hWnd, screenDC, &destination, &size,
                              m_MemoryDC, &source, 0, &blend, ULW_ALPHA);
        ::ReleaseDC(nullptr, screenDC);
    }

	m_Opacity = opacity;
}

void CSplash::SetStatus(LPCTSTR status)
{
    if (!GetSafeHwnd() || m_FinishRequested)
        return;

    CString next(status ? status : _T(""));
    if (m_Status == next)
        return;

    m_Status = next;
    DrawSurface();
    Present(m_Opacity);
}

void CSplash::Finish()
{
    if (!GetSafeHwnd() || m_FinishRequested)
        return;

    m_FinishRequested = true;
    AdvanceAnimation();
}

void CSplash::AdvanceAnimation()
{
    if (!GetSafeHwnd())
        return;

    const ULONGLONG now = ::GetTickCount64();
    BYTE opacity = 255;

    if (m_FinishRequested && now - m_StartTick >= m_MinimumDisplayMs)
    {
        if (m_FadeOutStartTick == 0)
            m_FadeOutStartTick = now;

        const ULONGLONG fadeElapsed = now - m_FadeOutStartTick;
        if (fadeElapsed >= kFadeOutMs)
        {
            CloseNow();
            return;
        }
        opacity = static_cast<BYTE>(255 * (kFadeOutMs - fadeElapsed) / kFadeOutMs);
    }
    else if (now - m_StartTick < kFadeInMs)
    {
        opacity = static_cast<BYTE>(255 * (now - m_StartTick) / kFadeInMs);
    }

    if (opacity != m_Opacity)
        Present(opacity);
}

void CSplash::CloseNow()
{
    if (GetSafeHwnd())
        DestroyWindow();
    else
    {
        ReleaseSurface();
        m_Logo.reset();
    }
}

void CSplash::OnTimer(UINT_PTR id)
{
    if (id == kTimerId)
    {
        AdvanceAnimation();
        return;
    }
    CWnd::OnTimer(id);
}

void CSplash::OnDestroy()
{
    KillTimer(kTimerId);
    ReleaseSurface();
    m_Logo.reset();
    CWnd::OnDestroy();
}

BOOL CSplash::OnEraseBkgnd(CDC*)
{
    return TRUE; // UpdateLayeredWindow supplies the pixels, not WM_PAINT.
}

void CSplash::OnPaint()
{
    CPaintDC dc(this); // Validate the region; all drawing is layered.
}

int CSplash::OnMouseActivate(CWnd*, UINT, UINT)
{
    return MA_NOACTIVATE;
}
