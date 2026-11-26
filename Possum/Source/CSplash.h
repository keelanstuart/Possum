#pragma once

#include <gdiplus.h>
#include <memory>

// A transparent, per-pixel-alpha startup splash for PossumAudio.
// The caller owns the original image; CSplash clones it.
// GDI+ must already be initialized before CreateSplash() is called.
class CSplash : public CWnd
{
    DECLARE_DYNAMIC(CSplash)

public:
    CSplash();
    ~CSplash() override;

    // Creates, centers, shows, and fades in the splash (about 220 ms).
    // Keep this CSplash object alive until the window has closed.
    BOOL CreateSplash(Gdiplus::Image* logo, DWORD minimumDisplayMs = 1400);

    // Safe to call synchronously between stages of CWinApp::InitInstance().
    // Renders the new status immediately; no message pump is required.
    void SetStatus(LPCTSTR status);

    // Nonblocking: starts the fade-out after the minimum display time.
    // The fade-out runs on WM_TIMER once MFC's message loop starts.
    void Finish();

    // For startup failure or app shutdown, before GdiplusShutdown().
    void CloseNow();

    bool IsVisible() const { return GetSafeHwnd() != nullptr; }

private:
    static constexpr UINT_PTR kTimerId = 1;
    static constexpr UINT kTimerPeriodMs = 20;
    static constexpr ULONGLONG kFadeInMs = 220;
    static constexpr ULONGLONG kFadeOutMs = 260;

    std::unique_ptr<Gdiplus::Image> m_Logo;
    std::unique_ptr<Gdiplus::Bitmap> m_Surface;
    CString m_Status;

    HDC m_MemoryDC = nullptr;
    HBITMAP m_Dib = nullptr;
    HGDIOBJ m_PreviousBitmap = nullptr;
    void* m_Pixels = nullptr;
    int m_Width = 0;
    int m_Height = 0;
    int m_LogoWidth = 0;
    int m_LogoHeight = 0;

    DWORD m_MinimumDisplayMs = 1400;
    ULONGLONG m_StartTick = 0;
    ULONGLONG m_FadeOutStartTick = 0;
    bool m_FinishRequested = false;
    BYTE m_Opacity = 0;

    bool CreateSurface();
    void ReleaseSurface();
    void DrawSurface();
    void Present(BYTE opacity);
    void AdvanceAnimation();

    afx_msg void OnTimer(UINT_PTR id);
    afx_msg void OnDestroy();
    afx_msg BOOL OnEraseBkgnd(CDC* dc);
    afx_msg void OnPaint();
    afx_msg int OnMouseActivate(CWnd* desktopWnd, UINT hitTest, UINT message);
    DECLARE_MESSAGE_MAP()
};
