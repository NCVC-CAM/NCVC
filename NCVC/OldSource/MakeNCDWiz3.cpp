// MakeNCDWiz3.cpp : �����t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "MakeNCDWiz.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
extern	CMagaDbg	g_dbg;
#endif

// CMakeNCDWiz3 �_�C�A���O

BEGIN_MESSAGE_MAP(CMakeNCDWiz3, CPropertyPage)
END_MESSAGE_MAP()

CMakeNCDWiz3::CMakeNCDWiz3() : CPropertyPage(CMakeNCDWiz3::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;
}

CMakeNCDWiz3::~CMakeNCDWiz3()
{
}

void CMakeNCDWiz3::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MAKENCD_TOOL, m_dTool);
	DDX_Control(pDX, IDC_MAKENCD_REMAIN, m_dRemain);
}

// CMakeNCDWiz3 ���b�Z�[�W �n���h��

BOOL CMakeNCDWiz3::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	// TODO :  �����ɏ�������ǉ����Ă�������

	return TRUE;  // return TRUE unless you set the focus to a control
	// ��O : OCX �v���p�e�B �y�[�W�͕K�� FALSE ��Ԃ��܂��B
}

BOOL CMakeNCDWiz3::OnSetActive()
{
	((CPropertySheet *)GetParent())->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);
	return CPropertyPage::OnSetActive();
}

LRESULT CMakeNCDWiz3::OnWizardNext()
{
	UpdateData();
	// �ް�����
	if ( m_dTool <= 0 ) {
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_dTool.SetFocus();
		m_dTool.SetSel(0, -1);
		return -1;
	}
	if ( m_dRemain < 0 ) {
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_dRemain.SetFocus();
		m_dRemain.SetSel(0, -1);
		return -1;
	}

	LPMAKENCDWIZARDPARAM pParam = ((CMakeNCDWiz *)GetParent())->m_pParam;
	pParam->dTool	= m_dTool;
	pParam->dRemain	= m_dRemain;

	return IDD_MAKENCD_WIZ9;	// ���ɕ\�������޲�۸�ID
}

LRESULT CMakeNCDWiz3::OnWizardBack()
{
	return IDD_MAKENCD_WIZ1;
}
