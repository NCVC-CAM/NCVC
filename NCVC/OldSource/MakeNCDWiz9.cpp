// MakeNCDWiz9.cpp : �����t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "MakeNCDWiz.h"
#include "MKNCSetup.h"
#include "MakeNCDlg.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
extern	CMagaDbg	g_dbg;
#endif

// CMakeNCDWiz9 �_�C�A���O

BEGIN_MESSAGE_MAP(CMakeNCDWiz9, CPropertyPage)
	ON_BN_CLICKED(IDC_MKNC_NCFILEUP, OnMKNCFileUp)
	ON_BN_CLICKED(IDC_MKNC_INITUP, OnMKNCInitUp)
	ON_BN_CLICKED(IDC_MKNC_INITED, OnMKNCInitEdit)
	ON_CBN_SELCHANGE(IDC_MKNC_INIT, OnSelChangeInit)
	ON_EN_KILLFOCUS(IDC_MKNC_NCFILE, OnKillFocusNCFile)
	ON_CBN_KILLFOCUS(IDC_MKNC_INIT, OnKillFocusInit)
END_MESSAGE_MAP()

CMakeNCDWiz9::CMakeNCDWiz9() : CPropertyPage(CMakeNCDWiz9::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;

	m_bNCView = TRUE;
}

CMakeNCDWiz9::~CMakeNCDWiz9()
{
}

void CMakeNCDWiz9::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MKNC_NCFILE, m_ctNCFileName);
	DDX_Text(pDX, IDC_MKNC_NCFILE, m_strNCFileName);
	DDX_Control(pDX, IDC_MKNC_INIT, m_ctInitFileName);
	DDX_CBString(pDX, IDC_MKNC_INIT, m_strInitFileName);
	DDX_Check(pDX, IDC_MKNC_VIEW, m_bNCView);
	DDX_Control(pDX, IDC_MKNC_VIEW, m_ctNCView);
}

// CMakeNCDWiz9 ���b�Z�[�W �n���h��

BOOL CMakeNCDWiz9::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	// �ݽ�׸��ł͂ł��Ȃ����۰ق̏�����
	
	// �޷���Ė�����NÇ�ٖ����쐬
	CreateNCFile( ((CMakeNCDWiz *)GetParent())->m_pDoc, m_strNCPath, m_strNCFileName);	// MakeNCD.cpp
	// �؍�����������珉���\��̧�ق��擾
	const	CDXFOption*	pOpt = AfxGetNCVCApp()->GetDXFOption();
	if ( pOpt->GetInitList()->GetCount() > 0 )
		::Path_Name_From_FullPath(pOpt->GetInitList()->GetHead(),
				m_strInitPath, m_strInitFileName);
	m_bNCView = pOpt->GetDxfFlag(DXFOPT_VIEW);
	UpdateData(FALSE);
	// �؍���������ޯ���ɱ��ђǉ�
	InitialMakeNCDlgComboBox(AfxGetNCVCApp()->GetDXFOption()->GetInitList(), m_ctInitFileName);
	// �߽�\���̍œK��(shlwapi.h)
	::PathSetDlgItemPath(m_hWnd, IDC_MKNC_NCPATH,   m_strNCPath);
	::PathSetDlgItemPath(m_hWnd, IDC_MKNC_INITPATH, m_strInitPath);

	return TRUE;  // return TRUE unless you set the focus to a control
	// ��O : OCX �v���p�e�B �y�[�W�͕K�� FALSE ��Ԃ��܂��B
}

BOOL CMakeNCDWiz9::OnSetActive()
{
	((CPropertySheet *)GetParent())->SetWizardButtons(PSWIZB_BACK | PSWIZB_FINISH);
	return CPropertyPage::OnSetActive();
}

LRESULT CMakeNCDWiz9::OnWizardBack()
{
	LRESULT	lResult = -1;
	switch ( ((CMakeNCDWiz *)GetParent())->m_pParam->nType ) {
	case 0:
		lResult = IDD_MAKENCD_WIZ2;
		break;
	case 1:
		lResult = IDD_MAKENCD_WIZ3;
		break;
	}
	return lResult;
}

BOOL CMakeNCDWiz9::OnWizardFinish()
{
	m_ctNCView.SetFocus();
	UpdateData();
	CString	strInitPath(m_strInitPath+m_strInitFileName),
			strNCPath(m_strNCPath+m_strNCFileName);

	// �؍����̧�ق�����(̧�ٕK�{)
	if ( !::IsFileExist(strInitPath) ) {	// NCVC.cpp
		m_ctInitFileName.SetEditSel(0, -1);
		m_ctInitFileName.SetFocus();
		return FALSE;
	}
	// NC����̧�ق�����(����Ώ㏑���m�F)
	if ( !CheckMakeDlgFileExt(TYPE_NCD, strNCPath) || !::IsFileExist(strNCPath, FALSE) ) {
		m_ctNCFileName.SetFocus();
		m_ctNCFileName.SetSel(0, -1);
		return FALSE;
	}

	CMakeNCDWiz*	pParent = (CMakeNCDWiz *)GetParent();
	pParent->m_strNCFileName = strNCPath;

	// ���������X�V
	CDXFOption*	pOpt = AfxGetNCVCApp()->GetDXFOption();
	pOpt->SetViewFlag(m_bNCView);
	pOpt->AddInitHistory(strInitPath);
	pOpt->SaveInitHistory();
	// �@�B���̍X�V
	AfxGetNCVCApp()->ChangeMachine( pParent->m_dlg2.m_strMachine );

	return CPropertyPage::OnWizardFinish();
}

void CMakeNCDWiz9::OnMKNCFileUp()
{
	UpdateData();
	MakeDlgFileRefer(IDS_DXF2NCD_FILE, AfxGetNCVCApp()->GetFilterString(TYPE_NCD),
			this, IDC_MKNC_NCPATH, m_strNCPath, m_strNCFileName, FALSE);
	// �����I�����
	m_ctNCFileName.SetFocus();
	m_ctNCFileName.SetSel(0, -1);
}

void CMakeNCDWiz9::OnMKNCInitUp()
{
	UpdateData();
	CString	strFilter;
	VERIFY(strFilter.LoadString(IDS_NCI_FILTER));
	MakeDlgFileRefer(IDS_OPTION_INIT, strFilter, this, IDC_MKNC_INITPATH,
			m_strInitPath, m_strInitFileName, TRUE);
	// �����I�����
	m_ctInitFileName.SetEditSel(0, -1);
	m_ctInitFileName.SetFocus();
}

void CMakeNCDWiz9::OnMKNCInitEdit()
{
	UpdateData();
	MakeNCDlgInitFileEdit(m_strInitPath, m_strInitFileName,
		this, IDC_MKNC_INITPATH, m_ctInitFileName);
}

void CMakeNCDWiz9::OnSelChangeInit()
{
	int nIndex = MakeNCDlgSelChange(m_ctInitFileName, m_hWnd, IDC_MKNC_INITPATH,
			m_strInitPath, m_strInitFileName);
	// ����̧�ق̑�������
	CString	strFullPath(m_strInitPath+m_strInitFileName);
	if ( !::IsFileExist(strFullPath) ) {
		AfxGetNCVCApp()->GetDXFOption()->DelInitHistory(strFullPath);	// �����폜
		m_ctInitFileName.DeleteString(nIndex);
	}
}

void CMakeNCDWiz9::OnKillFocusNCFile()
{
	UpdateData();
	MakeDlgKillFocus(m_strNCPath, m_strNCFileName, this, IDC_MKNC_NCPATH);
}

void CMakeNCDWiz9::OnKillFocusInit()
{
	UpdateData();
	MakeDlgKillFocus(m_strInitPath, m_strInitFileName, this, IDC_MKNC_INITPATH);
}
