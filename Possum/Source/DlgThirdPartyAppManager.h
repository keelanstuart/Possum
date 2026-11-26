#pragma once


// CThirdPartyAppManagerDlg dialog

class CThirdPartyAppManagerDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CThirdPartyAppManagerDlg)

public:
	CThirdPartyAppManagerDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CThirdPartyAppManagerDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_THIRDPARTYAPPMANAGER };
#endif

protected:
	CListCtrl m_RegAppList;
	CButton m_DelAppBtn;
	CButton m_AddAppBtn;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	void UpdateAppList();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedButtonNewThirdPartyApp();
	afx_msg void OnBnClickedButtonDeleteThirdPartyApp();
	afx_msg void OnLvnItemchangedListRegisteredApps(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkListRegisteredApps(NMHDR *pNMHDR, LRESULT *pResult);
	virtual BOOL OnInitDialog();
};
