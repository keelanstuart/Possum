#include "stdafx.h"
#include "ProductBrowserPanel.h"
#include "Resource.h"
#include "Possum.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CProductBrowserPanel::CProductBrowserPanel()
{
}

CProductBrowserPanel::~CProductBrowserPanel()
{
}

BEGIN_MESSAGE_MAP(CProductBrowserPanel, CWebBrowserPanel)
	ON_WM_CREATE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CClassView message handlers

int CProductBrowserPanel::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	int ret = __super::OnCreate(lpCreateStruct);

	if (ret == -1)
		return -1;

	m_wndBrowser.Navigate(_T(""), NULL, NULL, NULL, NULL);

	return 0;
}

