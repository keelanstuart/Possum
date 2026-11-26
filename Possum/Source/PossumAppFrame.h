
// PossumAppFrame.h : interface of the CPossumAppFrame class
//

#pragma once

class CPossumAppFrame : public CMDIChildWndEx
{
	DECLARE_DYNCREATE(CPossumAppFrame)
public:
	CPossumAppFrame();

// Attributes
public:

// Operations
public:

// Overrides
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

// Implementation
public:
	virtual ~CPossumAppFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
};
