#pragma once

// CPlayPauseToolBarButton command target

class CPlayPauseToolBarButton : public CMFCToolBarButton
{
	DECLARE_SERIAL(CPlayPauseToolBarButton)

public:
	CPlayPauseToolBarButton();
	CPlayPauseToolBarButton(UINT uiID, int iImage, LPCTSTR lpszText = NULL, BOOL bUserButton = FALSE, BOOL bLocked = FALSE);
	virtual ~CPlayPauseToolBarButton();

	virtual void OnDraw(CDC *pDC, const CRect &rect, CMFCToolBarImages *pImages, BOOL bHorz = TRUE, BOOL bCustomizeMode = FALSE,
						BOOL bHighlight = FALSE, BOOL bDrawBorder = TRUE, BOOL bGrayDisabledButtons = TRUE);

};


class CRewindToolBarButton : public CMFCToolBarButton
{
	DECLARE_SERIAL(CRewindToolBarButton)

public:
	CRewindToolBarButton();
	CRewindToolBarButton(UINT uiID, int iImage, LPCTSTR lpszText = NULL, BOOL bUserButton = FALSE, BOOL bLocked = FALSE);
	virtual ~CRewindToolBarButton();

	virtual void OnDraw(CDC *pDC, const CRect &rect, CMFCToolBarImages *pImages, BOOL bHorz = TRUE, BOOL bCustomizeMode = FALSE,
						BOOL bHighlight = FALSE, BOOL bDrawBorder = TRUE, BOOL bGrayDisabledButtons = TRUE);

};


class CStopToolBarButton : public CMFCToolBarButton
{
	DECLARE_SERIAL(CStopToolBarButton)

public:
	CStopToolBarButton();
	CStopToolBarButton(UINT uiID, int iImage, LPCTSTR lpszText = NULL, BOOL bUserButton = FALSE, BOOL bLocked = FALSE);
	virtual ~CStopToolBarButton();

	virtual void OnDraw(CDC *pDC, const CRect &rect, CMFCToolBarImages *pImages, BOOL bHorz = TRUE, BOOL bCustomizeMode = FALSE,
						BOOL bHighlight = FALSE, BOOL bDrawBorder = TRUE, BOOL bGrayDisabledButtons = TRUE);

};


class CLoopToolBarButton : public CMFCToolBarButton
{
	DECLARE_SERIAL(CLoopToolBarButton)

public:
	CLoopToolBarButton();
	CLoopToolBarButton(UINT uiID, int iImage, LPCTSTR lpszText = NULL, BOOL bUserButton = FALSE, BOOL bLocked = FALSE);
	virtual ~CLoopToolBarButton();

	virtual void OnDraw(CDC *pDC, const CRect &rect, CMFCToolBarImages *pImages, BOOL bHorz = TRUE, BOOL bCustomizeMode = FALSE,
						BOOL bHighlight = FALSE, BOOL bDrawBorder = TRUE, BOOL bGrayDisabledButtons = TRUE);

};


class CTimeToolBarButton : public CMFCToolBarButton
{
	DECLARE_SERIAL(CTimeToolBarButton)

public:
	CTimeToolBarButton();
	CTimeToolBarButton(UINT uiID, int iImage, LPCTSTR lpszText = NULL, BOOL bUserButton = FALSE, BOOL bLocked = FALSE);
	virtual ~CTimeToolBarButton();

	virtual void OnDraw(CDC *pDC, const CRect &rect, CMFCToolBarImages *pImages, BOOL bHorz = TRUE, BOOL bCustomizeMode = FALSE,
						BOOL bHighlight = FALSE, BOOL bDrawBorder = TRUE, BOOL bGrayDisabledButtons = TRUE);

	virtual SIZE OnCalculateSize(CDC *pDC, const CSize &sizeDefault, BOOL bHorz);

protected:
	float m_MeterLeft, m_MeterRight;

};
