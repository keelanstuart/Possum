// source\DlgGenPropEdit.cpp : implementation file
//

#include "stdafx.h"
#include "Possum.h"
#include "DlgGenPropEdit.h"
#include "afxdialogex.h"
#include "wtfpropertygridctrl.h"


// CGenPropEditDlg dialog

IMPLEMENT_DYNAMIC(CGenPropEditDlg, CDialogEx)

CGenPropEditDlg::CGenPropEditDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_GENPROPEDITDLG, pParent)
{
	m_pProps = props::IPropertySet::CreatePropertySet();
}

CGenPropEditDlg::~CGenPropEditDlg()
{
	if (m_pProps)
		m_pProps->Release();
}

void CGenPropEditDlg::DoDataExchange(CDataExchange* pDX)
{
	DDX_Control(pDX, IDC_PROPGRID_GENERIC, m_PropGrid);

	CDialogEx::DoDataExchange(pDX);
}

props::IPropertySet *CGenPropEditDlg::GetProperties()
{
	return m_pProps;
}

BEGIN_MESSAGE_MAP(CGenPropEditDlg, CDialogEx)
END_MESSAGE_MAP()


// CGenPropEditDlg message handlers


BOOL CGenPropEditDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	if (m_pProps)
	{
		for (size_t i = 0, maxi = m_pProps->GetPropertyCount(); i < maxi; i++)
		{
			props::IProperty *pp = m_pProps->GetProperty(i);
			CWTFPropertyGridProperty *pgp = nullptr;

			switch (pp->GetType())
			{
				case props::IProperty::PT_FLOAT:
					pgp = new CWTFPropertyGridProperty(CString(pp->GetName()), pp->AsFloat(), NULL, NULL, NULL, NULL, _T("0123456789.-"));
					break;

				case props::IProperty::PT_STRING:
				{
					switch (pp->GetAspect())
					{
						case props::IProperty::PA_FILENAME:
							pgp = new CWTFPropertyGridFileProperty(CString(pp->GetName()), TRUE, CString(pp->AsString()), _T(""), 0, ((pp->GetID() == 'EXE') ? _T("*.exe") : _T("*.*")));
							break;

						case props::IProperty::PA_DIRECTORY:
							pgp = new CWTFPropertyGridFileProperty(CString(pp->GetName()), CString(pp->AsString()));
							break;

						default:
							pgp = new CWTFPropertyGridProperty(CString(pp->GetName()), pp->AsString());
							break;
					}

					break;
				}
				case props::IProperty::PT_BOOLEAN:
				{
					switch (pp->GetAspect())
					{
						case props::IProperty::PA_BOOL_ONOFF:
						case props::IProperty::PA_BOOL_YESNO:
						default:
							pgp = new CWTFPropertyGridProperty(CString(pp->GetName()), (_variant_t)(pp->AsBool()), CString(_T("")));
							break;
					}
				}
				case props::IProperty::PT_INT:
				{
					switch (pp->GetAspect())
					{
						case props::IProperty::PA_COLOR_RGB:
						{
							UINT32 r = GetRValue(pp->AsInt());
							UINT32 g = GetGValue(pp->AsInt());
							UINT32 b = GetBValue(pp->AsInt());

							pgp = new CWTFPropertyGridColorProperty(CString(pp->GetName()), RGB(r, g, b));
							((CWTFPropertyGridColorProperty *)pgp)->EnableOtherButton(_T("Other..."));
							((CWTFPropertyGridColorProperty *)pgp)->EnableAutomaticButton(_T("Default"), ::GetSysColor(COLOR_3DFACE));

							break;
						}

						case props::IProperty::PA_COLOR_RGBA:
						{
							UINT32 r = GetRValue(pp->AsInt());
							UINT32 g = GetGValue(pp->AsInt());
							UINT32 b = GetBValue(pp->AsInt());
							//UINT32 a = GetBValue(pp->AsInt());

							pgp = new CWTFPropertyGridColorProperty(CString(pp->GetName()), RGB(r, g, b));
							((CWTFPropertyGridColorProperty *)pgp)->EnableOtherButton(_T("Other..."));
							((CWTFPropertyGridColorProperty *)pgp)->EnableAutomaticButton(_T("Default"), ::GetSysColor(COLOR_3DFACE));

							break;
						}

						default:
							pgp = new CWTFPropertyGridProperty(CString(pp->GetName()), (_variant_t)pp->AsInt(), NULL, NULL, NULL, NULL, _T("0123456789-"));
							break;
					}

					break;
				}
			}

			if (pgp)
			{
				m_PropGrid.AddProperty(pgp);
				pgp->SetData((DWORD_PTR)pp);
			}
		}
	}

	m_PropGrid.SetBoolLabels(_T("Enabled"), _T("Disabled"));
	m_PropGrid.SetGroupNameFullWidth(TRUE, FALSE);

	CRect r;
	m_PropGrid.GetClientRect(r);
	HDITEM hdItem;
	hdItem.mask = HDI_WIDTH;
	hdItem.cxy = r.Width() / 3;
	m_PropGrid.GetHeaderCtrl().SetItem(0, &hdItem);

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}


void CGenPropEditDlg::OnOK()
{
	// if the user accepts their changes, then copy the properties back if they're different...

	for (int i = 0, maxi = m_PropGrid.GetPropertyCount(); i < maxi; i++)
	{
		CWTFPropertyGridProperty *pgp = m_PropGrid.GetProperty(i);
		if (pgp && pgp->IsModified())
		{
			props::IProperty *pp = (props::IProperty *)pgp->GetData();
			if (pp)
			{
				props::IProperty::PROPERTY_TYPE pt = pp->GetType();
				props::IProperty::PROPERTY_ASPECT pa = pp->GetAspect();

				pp->SetString(pgp->GetValue().bstrVal);
				pp->ConvertTo(pt);
				pp->SetAspect(pa);
			}
		}
	}

	CDialogEx::OnOK();
}
