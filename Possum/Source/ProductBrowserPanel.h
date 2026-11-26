
#pragma once

#include "WebBrowserPanel.h"

class CProductBrowserPanel : public CWebBrowserPanel
{
public:
	CProductBrowserPanel();
	virtual ~CProductBrowserPanel();

protected:

// Overrides
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);

	DECLARE_MESSAGE_MAP()
};

