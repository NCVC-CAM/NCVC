// MakeNCSetup1.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "ExecOption.h"
#include "NCMakeMillOpt.h"
#include "MakeNCSetup.h"
#include "MakeNurbsSetup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CMakeNCSetup1, CPropertyPage)
	//{{AFX_MSG_MAP(CMakeNCSetup1)
	ON_BN_CLICKED(IDC_MKNC1_HEADER_LOOPUP, &CMakeNCSetup1::OnHeaderLoopup)
	ON_BN_CLICKED(IDC_MKNC1_FOOTER_LOOPUP, &CMakeNCSetup1::OnFooterLoopup)
	ON_BN_CLICKED(IDC_MKNC1_HEADER_EDIT, &CMakeNCSetup1::OnHeaderEdit)
	ON_BN_CLICKED(IDC_MKNC1_FOOTER_EDIT, &CMakeNCSetup1::OnFooterEdit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMakeNCSetup1 �v���p�e�B �y�[�W

CMakeNCSetup1::CMakeNCSetup1() : CPropertyPage(CMakeNCSetup1::IDD),
	m_dFeed(TRUE), m_dZFeed(TRUE)	// CFloatEdit�׸ސݒ�
{
	m_psp.dwFlags &= ~PSP_HASHELP;
	//{{AFX_DATA_INIT(CMakeNCSetup1)
	m_bXrev = FALSE;
	m_bYrev = FALSE;
	//}}AFX_DATA_INIT
}

void CMakeNCSetup1::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMakeNCSetup1)
	DDX_Control(pDX, IDC_MKNC1_ZFEED, m_dZFeed);
	DDX_Control(pDX, IDC_MKNC1_FEED, m_dFeed);
	DDX_Control(pDX, IDC_MKNC1_HEADER_EDIT, m_ctHeaderBt);
	DDX_Control(pDX, IDC_MKNC1_FOOTER_EDIT, m_ctFooterBt);
	DDX_Control(pDX, IDC_MKNC1_HEADER, m_ctHeader);
	DDX_Control(pDX, IDC_MKNC1_FOOTER, m_ctFooter);
	DDX_Control(pDX, IDC_MKNC1_XREV, m_ctXrevBt);
	DDX_Control(pDX, IDC_MKNC1_YREV, m_ctYrevBt);
	DDX_Control(pDX, IDC_MKNC1_ZCUT, m_dZCut);
	DDX_Control(pDX, IDC_MKNC1_R, m_dZG0Stop);
	DDX_Control(pDX, IDC_MKNC1_SPINDLE, m_nSpindle);
	DDX_Text(pDX, IDC_MKNC1_FOOTER, m_strFooter);
	DDX_Text(pDX, IDC_MKNC1_HEADER, m_strHeader);
	DDX_Check(pDX, IDC_MKNC1_XREV, m_bXrev);
	DDX_Check(pDX, IDC_MKNC1_YREV, m_bYrev);
	//}}AFX_DATA_MAP
	for ( int i=0; i<NCXYZ; i++ )
		DDX_Control(pDX, i+IDC_MKNC1_G92X, m_dG92[i]);
}

/////////////////////////////////////////////////////////////////////////////
// CMakeNCSetup1 ���b�Z�[�W �n���h��

BOOL CMakeNCSetup1::OnInitDialog() 
{
	__super::OnInitDialog();

	// ���Ѻ��۰قͺݽ�׸��ŏ������ł��Ȃ�
	// + GetParentSheet() �߲�����擾�ł��Ȃ�
	int		i;
	CNCMakeMillOpt*	pOpt;
	CWnd*	pWnd = GetParentSheet();
	if ( pWnd->IsKindOf(RUNTIME_CLASS(CMakeNCSetup)) ) {
		// �ʏ�t���C�X���[�h
		pOpt = static_cast<CMakeNCSetup *>(pWnd)->GetNCMakeOption();
	}
	else {
		// NURBS���[�h
		pOpt = static_cast<CMakeNurbsSetup *>(pWnd)->GetNCMakeOption();
		m_ctXrevBt.EnableWindow(FALSE);
		m_ctYrevBt.EnableWindow(FALSE);
		for ( i=0; i<NCXYZ; i++ )
			m_dG92[i].EnableWindow(FALSE);
	}

	m_nSpindle	= pOpt->MIL_I_SPINDLE;
	m_dFeed		= pOpt->MIL_D_FEED;
	m_dZFeed	= pOpt->MIL_D_ZFEED;
	m_dZCut		= pOpt->MIL_D_ZCUT;
	m_dZG0Stop	= pOpt->MIL_D_ZG0STOP;
	m_bXrev		= pOpt->MIL_F_XREV;
	m_bYrev		= pOpt->MIL_F_YREV;
	for ( i=0; i<NCXYZ; i++ )
		m_dG92[i] = pOpt->m_pDblOpt[MKNC_DBL_G92X+i];
	::Path_Name_From_FullPath(pOpt->MIL_S_HEADER, m_strHeaderPath, m_strHeader);
	::Path_Name_From_FullPath(pOpt->MIL_S_FOOTER, m_strFooterPath, m_strFooter);
	// �߽�\���̍œK��(shlwapi.h)
	::PathSetDlgItemPath(m_hWnd, IDC_MKNC1_HEADERPATH, m_strHeaderPath);
	::PathSetDlgItemPath(m_hWnd, IDC_MKNC1_FOOTERPATH, m_strFooterPath);
	// �ҏW���݂̗L������
	if ( AfxGetNCVCApp()->GetExecList()->GetCount() < 1 ) {
		m_ctHeaderBt.EnableWindow(FALSE);
		m_ctFooterBt.EnableWindow(FALSE);
	}

	UpdateData(FALSE);

	return TRUE;  // �R���g���[���Ƀt�H�[�J�X��ݒ肵�Ȃ��Ƃ��A�߂�l�� TRUE �ƂȂ�܂�
	              // ��O: OCX �v���p�e�B �y�[�W�̖߂�l�� FALSE �ƂȂ�܂�
}

void CMakeNCSetup1::OnHeaderLoopup() 
{
	UpdateData();
	if ( ::NCVC_FileDlgCommon(IDS_CUSTOMFILE, IDS_TXT_FILTER, TRUE, m_strHeader, m_strHeaderPath) == IDOK ) {
		// �ް��̔��f
		::Path_Name_From_FullPath(m_strHeader, m_strHeaderPath, m_strHeader);
		::PathSetDlgItemPath(m_hWnd, IDC_MKNC1_HEADERPATH, m_strHeaderPath);
		UpdateData(FALSE);
		// �����I�����
		m_ctHeader.SetFocus();
		m_ctHeader.SetSel(0, -1);
	}
}

void CMakeNCSetup1::OnFooterLoopup() 
{
	UpdateData();
	if ( ::NCVC_FileDlgCommon(IDS_CUSTOMFILE, IDS_TXT_FILTER, TRUE, m_strFooter, m_strFooterPath) == IDOK ) {
		// �ް��̔��f
		::Path_Name_From_FullPath(m_strFooter, m_strFooterPath, m_strFooter);
		::PathSetDlgItemPath(m_hWnd, IDC_MKNC1_FOOTERPATH, m_strFooterPath);
		UpdateData(FALSE);
		// �����I�����
		m_ctFooter.SetFocus();
		m_ctFooter.SetSel(0, -1);
	}
}

void CMakeNCSetup1::OnHeaderEdit() 
{
	ASSERT( AfxGetNCVCApp()->GetExecList()->GetCount() > 0 );
	UpdateData();
	AfxGetNCVCMainWnd()->CreateOutsideProcess(
		AfxGetNCVCApp()->GetExecList()->GetHead()->GetFileName(),
		"\""+m_strHeaderPath+m_strHeader+"\"");
	m_ctHeader.SetFocus();
}

void CMakeNCSetup1::OnFooterEdit() 
{
	ASSERT( AfxGetNCVCApp()->GetExecList()->GetCount() > 0 );
	UpdateData();
	AfxGetNCVCMainWnd()->CreateOutsideProcess(
		AfxGetNCVCApp()->GetExecList()->GetHead()->GetFileName(),
		"\""+m_strFooterPath+m_strFooter+"\"");
	m_ctFooter.SetFocus();
}

BOOL CMakeNCSetup1::OnApply() 
{
	CNCMakeMillOpt*	pOpt;
	CWnd*	pWnd = GetParentSheet();
	if ( pWnd->IsKindOf(RUNTIME_CLASS(CMakeNCSetup)) ) {
		// �߰�ފԂ̈ˑ��֌W
		// OnKillActive() �ł��߰�ނ�؂�ւ����Ȃ��̂ł����Ƃ�����
		int		nMakeEnd;
		BOOL	bDeep;
		float	dDeep, dMakeValue;
		CMakeNCSetup* pParent = static_cast<CMakeNCSetup *>(pWnd);
		// �ʏ�t���C�X���[�h
		pOpt = pParent->GetNCMakeOption();
		if ( ::IsWindow(pParent->m_dlg3.m_hWnd) ) {
			nMakeEnd	= pParent->m_dlg3.m_nMakeEnd;
			dMakeValue	= pParent->m_dlg3.m_dMakeValue;
			bDeep		= pParent->m_dlg3.m_bDeep;
			dDeep		= pParent->m_dlg3.m_dDeep;
		}
		else {
			nMakeEnd	= pOpt->MIL_I_MAKEEND;
			dMakeValue	= pOpt->MIL_D_MAKEEND;
			bDeep		= pOpt->MIL_F_DEEP;
			dDeep		= pOpt->MIL_D_DEEP;
		}
		if ( nMakeEnd == 2 ) {
			if ( dMakeValue > m_dZG0Stop ) {
				AfxMessageBox(IDS_ERR_DEEPFIXR, MB_OK|MB_ICONEXCLAMATION);
				m_dZG0Stop.SetFocus();
				m_dZG0Stop.SetSel(0, -1);
				return FALSE;
			}
			if ( dMakeValue <= m_dZCut ) {
				AfxMessageBox(IDS_ERR_DEEPFIXZ, MB_OK|MB_ICONEXCLAMATION);
				m_dZCut.SetFocus();
				m_dZCut.SetSel(0, -1);
				return FALSE;
			}
		}
		if ( bDeep && m_dZCut<dDeep ) {
			AfxMessageBox(IDS_ERR_DEEPFINAL, MB_OK|MB_ICONEXCLAMATION);
			m_dZCut.SetFocus();
			m_dZCut.SetSel(0, -1);
			return FALSE;
		}
	}
	else {
		// NURBS���[�h
		pOpt = static_cast<CMakeNurbsSetup *>(pWnd)->GetNCMakeOption();
	}

	pOpt->MIL_I_SPINDLE	= m_nSpindle;
	pOpt->MIL_D_FEED	= m_dFeed;
	pOpt->MIL_D_ZFEED	= m_dZFeed;
	pOpt->MIL_D_ZCUT	= m_dZCut;
	pOpt->MIL_D_ZG0STOP	= m_dZG0Stop;
	pOpt->MIL_F_XREV	= m_bXrev;
	pOpt->MIL_F_YREV	= m_bYrev;
	pOpt->MIL_S_HEADER	= m_strHeaderPath+m_strHeader;
	pOpt->MIL_S_FOOTER	= m_strFooterPath+m_strFooter;
	for ( int i=0; i<NCXYZ; i++ )
		pOpt->m_pDblOpt[MKNC_DBL_G92X+i] = m_dG92[i];

	return TRUE;
}

BOOL CMakeNCSetup1::OnKillActive() 
{
	if ( !__super::OnKillActive() )
		return FALSE;

	if ( m_dFeed <= 0 ) {
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_dFeed.SetFocus();
		m_dFeed.SetSel(0, -1);
		return FALSE;
	}
	if ( m_dZFeed <= 0 ) {
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_dZFeed.SetFocus();
		m_dZFeed.SetSel(0, -1);
		return FALSE;
	}
	if ( m_strHeader.IsEmpty() ) {
		AfxMessageBox(IDS_ERR_CUSTOMFILE, MB_OK|MB_ICONEXCLAMATION);
		m_ctHeader.SetFocus();
		return FALSE;
	}
	if ( m_strFooter.IsEmpty() ) {
		AfxMessageBox(IDS_ERR_CUSTOMFILE, MB_OK|MB_ICONEXCLAMATION);
		m_ctFooter.SetFocus();
		return FALSE;
	}
	if ( !::IsFileExist(m_strHeaderPath+m_strHeader) ) {
		m_ctHeader.SetFocus();
		return FALSE;
	}
	if ( !::IsFileExist(m_strFooterPath+m_strFooter) ) {
		m_ctFooter.SetFocus();
		return FALSE;
	}
	if ( (float)m_dZCut > (float)m_dZG0Stop ) {
		AfxMessageBox(IDS_ERR_ZCUT, MB_OK|MB_ICONEXCLAMATION);
		m_dZCut.SetFocus();
		m_dZCut.SetSel(0, -1);
		return FALSE;
	}

	return TRUE;
}
