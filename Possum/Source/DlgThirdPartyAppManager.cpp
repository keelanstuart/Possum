// DlgThirdPartyAppManager.cpp : implementation file
//

#include "stdafx.h"
#include "Possum.h"
#include "DlgThirdPartyAppManager.h"
#include "afxdialogex.h"

#include "DlgGenPropEdit.h"


// CThirdPartyAppManagerDlg dialog

IMPLEMENT_DYNAMIC(CThirdPartyAppManagerDlg, CDialogEx)

CThirdPartyAppManagerDlg::CThirdPartyAppManagerDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_THIRDPARTYAPPMANAGER, pParent)
{

}

CThirdPartyAppManagerDlg::~CThirdPartyAppManagerDlg()
{
}

void CThirdPartyAppManagerDlg::DoDataExchange(CDataExchange* pDX)
{
	DDX_Control(pDX, IDC_LIST_REGISTEREDAPPS, m_RegAppList);
	DDX_Control(pDX, IDC_BUTTON_NEW_THIRDPARTYAPP, m_AddAppBtn);
	DDX_Control(pDX, IDC_BUTTON_DELETE_THIRDPARTYAPP, m_DelAppBtn);

	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CThirdPartyAppManagerDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CThirdPartyAppManagerDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON_NEW_THIRDPARTYAPP, &CThirdPartyAppManagerDlg::OnBnClickedButtonNewThirdPartyApp)
	ON_BN_CLICKED(IDC_BUTTON_DELETE_THIRDPARTYAPP, &CThirdPartyAppManagerDlg::OnBnClickedButtonDeleteThirdPartyApp)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_REGISTEREDAPPS, &CThirdPartyAppManagerDlg::OnLvnItemchangedListRegisteredApps)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_REGISTEREDAPPS, &CThirdPartyAppManagerDlg::OnNMDblclkListRegisteredApps)
END_MESSAGE_MAP()


// CThirdPartyAppManagerDlg message handlers


void CThirdPartyAppManagerDlg::OnBnClickedOk()
{
	CDialogEx::OnOK();
}


void CThirdPartyAppManagerDlg::OnBnClickedButtonNewThirdPartyApp()
{
	CGenPropEditDlg dpe;
	props::IPropertySet *pps = dpe.GetProperties();

	props::IProperty *pname = pps->CreateProperty(_T("Applcation Name"), 'NAME');
	pname->SetString(_T("New Application"));

	props::IProperty *pexe = pps->CreateProperty(_T("Applcation Executable"), 'EXE');
	pexe->SetString(_T("*.exe"));
	pexe->SetAspect(props::IProperty::PA_FILENAME);

	props::IProperty *prunfrom = pps->CreateProperty(_T("Run From..."), 'RUND');
	prunfrom->SetString(_T("C:\\"));
	prunfrom->SetAspect(props::IProperty::PA_DIRECTORY);

	props::IProperty *pparams = pps->CreateProperty(_T("Parameters"), 'PARM');
	pparams->SetString(_T(""));

	props::IProperty *pwndclass = pps->CreateProperty(_T("WND Class"), 'WNDC');
	pwndclass->SetString(_T(""));

	props::IProperty *pwndname = pps->CreateProperty(_T("WND Name"), 'WNDN');
	pwndname->SetString(_T(""));

	if (dpe.DoModal() == IDOK)
	{
		if (!theApp.m_pTPAM->IsAppRegistered(pname->AsString()))
		{
			// register a new app
			if (theApp.m_pTPAM->RegisterApp(pname->AsString(), pexe->AsString(), prunfrom->AsString(), pparams->AsString(), pwndclass->AsString(), pwndname->AsString()))
			{
				theApp.OpenAppDoc(pname->AsString());
			}
		}
	}

	UpdateAppList();
}


void CThirdPartyAppManagerDlg::OnBnClickedButtonDeleteThirdPartyApp()
{
	int sel = m_RegAppList.GetSelectionMark();
	if (sel != -1)
	{
		tstring s[CThirdPartyAppManager::NUMPARAMS];

		CString name = m_RegAppList.GetItemText(sel, 0);
		if (theApp.m_pTPAM->GetRegisteredApp(size_t(sel), &s[CThirdPartyAppManager::NAME]) && (name == s[CThirdPartyAppManager::NAME].c_str()))
		{
			tstring confstr = _T("Are you sure you want to remove \"");
			confstr += s[CThirdPartyAppManager::NAME];
			confstr += _T("\" from your list of registered applications?");

			if (::MessageBox(GetSafeHwnd(), confstr.c_str(), _T("Remove Application?"), MB_YESNO) == IDYES)
			{
				theApp.m_pTPAM->UnregisterApp(s[CThirdPartyAppManager::NAME].c_str());
				UpdateAppList();
			}
		}
	}
}


void CThirdPartyAppManagerDlg::OnLvnItemchangedListRegisteredApps(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	*pResult = 0;
}


void CThirdPartyAppManagerDlg::OnNMDblclkListRegisteredApps(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	int sel = m_RegAppList.GetSelectionMark();
	if (sel != -1)
	{
		tstring s[CThirdPartyAppManager::NUMPARAMS];

		CString name = m_RegAppList.GetItemText(sel, 0);
		if (theApp.m_pTPAM->GetRegisteredApp(size_t(sel), &s[CThirdPartyAppManager::NAME], &s[CThirdPartyAppManager::EXEPATH],
			&s[CThirdPartyAppManager::RUNDIR], &s[CThirdPartyAppManager::PARAMS], 
			&s[CThirdPartyAppManager::WNDCLASS], &s[CThirdPartyAppManager::WNDNAME]))
		{
			CGenPropEditDlg dpe;
			props::IPropertySet *pps = dpe.GetProperties();

			props::IProperty *pname = pps->CreateProperty(_T("Applcation Name"), 'NAME');
			pname->SetString(s[CThirdPartyAppManager::NAME].c_str());

			props::IProperty *pexe = pps->CreateProperty(_T("Applcation Executable"), 'EXE');
			pexe->SetString(s[CThirdPartyAppManager::EXEPATH].c_str());
			pexe->SetAspect(props::IProperty::PA_FILENAME);

			props::IProperty *prunfrom = pps->CreateProperty(_T("Run From..."), 'RUND');
			prunfrom->SetString(s[CThirdPartyAppManager::RUNDIR].c_str());
			prunfrom->SetAspect(props::IProperty::PA_DIRECTORY);

			props::IProperty *pparams = pps->CreateProperty(_T("Parameters"), 'PARM');
			pparams->SetString(s[CThirdPartyAppManager::PARAMS].c_str());

			props::IProperty *pwndclass = pps->CreateProperty(_T("WND Class"), 'WNDC');
			pwndclass->SetString(s[CThirdPartyAppManager::WNDCLASS].c_str());

			props::IProperty *pwndname = pps->CreateProperty(_T("WND Name"), 'WNDN');
			pwndname->SetString(s[CThirdPartyAppManager::WNDNAME].c_str());

			if (dpe.DoModal() == IDOK)
			{
				if (!theApp.m_pTPAM->IsAppRegistered(pname->AsString()))
				{
					// register a new app
					if (theApp.m_pTPAM->RegisterApp(pname->AsString(), pexe->AsString(), prunfrom->AsString(), pparams->AsString(), pwndclass->AsString(), pwndname->AsString()))
					{
						theApp.OpenAppDoc(pname->AsString());
					}
				}
			}

			UpdateAppList();
		}
	}

	*pResult = 0;
}


BOOL CThirdPartyAppManagerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	static tstring col_name[CThirdPartyAppManager::NUMPARAMS] = { _T("Application"), _T("Target"), _T("Start In"), _T("Parameters"), _T("WndClass"), _T("WndName") };

	m_RegAppList.SetExtendedStyle(m_RegAppList.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_SINGLEROW);

	LVCOLUMN colinfo;

	colinfo.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	colinfo.fmt = LVCFMT_LEFT;

	// Column 0 is for the property's name
	for (size_t i = 0; i < CThirdPartyAppManager::NUMPARAMS; i++)
	{
		colinfo.cx = 120;
		colinfo.pszText = (LPTSTR)col_name[i].c_str();
		colinfo.cchTextMax = int(col_name[i].length() + 1);
		colinfo.iSubItem = int(i);
		m_RegAppList.InsertColumn(int(i), &colinfo);
	}

	m_RegAppList.UpdateWindow();

	UpdateAppList();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

void CThirdPartyAppManagerDlg::UpdateAppList()
{
	m_RegAppList.DeleteAllItems();

	LVITEM iteminfo;
	memset(&iteminfo, 0, sizeof(LVITEM));

	size_t maxi = theApp.m_pTPAM->GetAppCount();

	for (size_t i = 0; i < maxi; i++)
	{
		iteminfo.mask = LVIF_TEXT;
		iteminfo.iItem = int(i);

		tstring s[CThirdPartyAppManager::NUMPARAMS];
		theApp.m_pTPAM->GetRegisteredApp(i, &s[CThirdPartyAppManager::NAME], &s[CThirdPartyAppManager::EXEPATH],
			&s[CThirdPartyAppManager::RUNDIR], &s[CThirdPartyAppManager::PARAMS],
			&s[CThirdPartyAppManager::WNDCLASS], &s[CThirdPartyAppManager::WNDNAME]);

		for (size_t k = 0; k < CThirdPartyAppManager::NUMPARAMS; k++)
		{
			iteminfo.pszText = (LPTSTR)s[k].c_str();
			iteminfo.cchTextMax = int(s[k].length() + 1);
			iteminfo.iSubItem = int(k);
			if (k)
			{
				m_RegAppList.SetItem(&iteminfo);
			}
			else
			{
				iteminfo.iItem = m_RegAppList.InsertItem(&iteminfo);
			}
		}
	}

	m_DelAppBtn.EnableWindow((maxi > 0) ? true : false);
}
