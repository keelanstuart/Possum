#pragma once


// CEnvelopeCommandWnd

class CEnvelopeCommandWnd : public CDialogEx
{
	DECLARE_DYNAMIC(CEnvelopeCommandWnd)

public:
	CEnvelopeCommandWnd();
	virtual ~CEnvelopeCommandWnd();

protected:
	DECLARE_MESSAGE_MAP()
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
};


