// NCWorkDlg.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "NCChild.h"
#include "NCDoc.h"
#include "NCWorkDlg.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CNCWorkDlg, CDialog)
	//{{AFX_MSG_MAP(CNCWorkDlg)
	ON_BN_CLICKED(IDC_WORK_HIDE, OnHide)
	ON_BN_CLICKED(IDC_WORK_RECOVER, OnRecover)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USERSWITCHDOCUMENT, OnUserSwitchDocument)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNCWorkDlg �_�C�A���O

CNCWorkDlg::CNCWorkDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNCWorkDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNCWorkDlg)
	//}}AFX_DATA_INIT
}

void CNCWorkDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNCWorkDlg)
	DDX_Control(pDX, IDOK, m_ctOK);
	DDX_Control(pDX, IDC_WORK_HIDE, m_ctHide);
	DDX_Control(pDX, IDC_WORK_RECOVER, m_ctRecover);
	//}}AFX_DATA_MAP
	int		i, j, n;
	for ( i=0; i<SIZEOF(m_ctWork); i++ ) {
		for ( j=0; j<NCXYZ; j++ ) {
			n = i * NCXYZ + j;
			DDX_Control(pDX, n+IDC_WORK_X,  m_ctWork[i][j]);
			DDX_Control(pDX, n+IDC_WORK_XS, m_ctLabel[i][j]);
		}
	}
}

void CNCWorkDlg::EnableButton(BOOL bEnable, BOOL bLathe)
{
	static	LPCTSTR	szLabel[][NCXYZ] = {
		{"��(&W)", "���s(&D)", "����(&H)"},
		{"�a(&D)", "Zmin(&L)", "Zmax(&R)"}
	};
	int		i, n = (bEnable & bLathe) ? 1 : 0;

	m_ctOK.EnableWindow(bEnable);
	m_ctHide.EnableWindow(bEnable);
	m_ctRecover.EnableWindow(bEnable);

	for ( i=0; i<NCXYZ; i++ ) {
		// �傫���w���� bEnable
		m_ctWork[0][i].EnableWindow(bEnable);
		// �̾�Ďw���� bEnable & !bLathe
		m_ctWork[1][i].EnableWindow(bEnable & !bLathe);
		// �̾�����ق� !bLathe
		m_ctLabel[1][i].EnableWindow(!bLathe);
		// ���ٕύX
		m_ctLabel[0][i].SetWindowText(szLabel[n][i]);
	}
}

void CNCWorkDlg::SetValue(const CNCDoc* pDoc, const CRect3D& rc)
{
	if ( pDoc->IsNCDocFlag(NCDOC_LATHE) ) {
		m_ctWork[0][NCA_X] = rc.high * 2.0;		// ���a�\��
		m_ctWork[0][NCA_Y] = rc.left;
		m_ctWork[0][NCA_Z] = rc.right;
		m_ctWork[1][NCA_X] = 0;
		m_ctWork[1][NCA_Y] = 0;
		m_ctWork[1][NCA_Z] = 0;
	}
	else {
		m_ctWork[0][NCA_X] = rc.Width();
		m_ctWork[0][NCA_Y] = rc.Height();
		m_ctWork[0][NCA_Z] = rc.Depth();
		m_ctWork[1][NCA_X] = rc.left;
		m_ctWork[1][NCA_Y] = rc.top;
		m_ctWork[1][NCA_Z] = rc.low;
	}
	UpdateData(FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// CNCWorkDlg ���b�Z�[�W �n���h��

BOOL CNCWorkDlg::OnInitDialog() 
{
	extern	LPCTSTR	g_szNdelimiter;	// "XYZUVWIJKRPLDH" from NCDoc.cpp

	__super::OnInitDialog();
	int		i, j;

	// �ݽ�׸��ł͏������ł��Ȃ����۰ق̏�����
	CString	strRegKey(GetSubTreeRegKey(IDS_REGKEY_NC, IDS_REGKEY_WINDOW_WORKDLG)),	// StdAfx.h
			strTmp;

	// ڼ޽�؂ɕۑ����ꂽܰ���`���͍폜
	for ( i=0; i<SIZEOF(m_ctWork); i++ ) {
		for ( j=0; j<NCXYZ; j++ )
			AfxGetApp()->WriteProfileString(strRegKey, g_szNdelimiter[j]+strTmp, NULL);
		strTmp = 'O';
	}

	// �\��/��\�����݂̏�����
	OnUserSwitchDocument(NULL, NULL);

	// ����޳�ʒu�ǂݍ���
	CPoint	pt;
	if ( AfxGetNCVCApp()->GetDlgWindow(IDS_REGKEY_WINDOW_WORKDLG, &pt) )
		SetWindowPos(NULL, pt.x, pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	return TRUE;  // �R���g���[���Ƀt�H�[�J�X��ݒ肵�Ȃ��Ƃ��A�߂�l�� TRUE �ƂȂ�܂�
	              // ��O: OCX �v���p�e�B �y�[�W�̖߂�l�� FALSE �ƂȂ�܂�
}

void CNCWorkDlg::OnOK() 
{
	CWaitCursor		wait;

	// �޷�����ޭ��ւ̕ύX�ʒm
	// ���̲���Ă���������Ƃ��́COnUserSwitchDocument() ���
	// �K�� AfxGetNCVCMainWnd()->MDIGetActive() �� CNCChild ���w���Ă���
	CMDIChildWnd* pFrame = AfxGetNCVCMainWnd()->MDIGetActive();
	if ( pFrame && pFrame->IsKindOf(RUNTIME_CLASS(CNCChild)) ) {
		CNCDoc*	pDoc = static_cast<CNCDoc *>(pFrame->GetActiveDocument());
		if ( pDoc && pDoc->IsKindOf(RUNTIME_CLASS(CNCDoc)) ) {
			CRect3D	rc;
			if ( pDoc->IsNCDocFlag(NCDOC_LATHE) ) {
				rc.high   = m_ctWork[0][NCA_X] / 2.0;	// �a
				rc.left   = m_ctWork[0][NCA_Y];			// Zmin
				rc.right  = m_ctWork[0][NCA_Z];			// Zmax
			}
			else {
				rc.left   = m_ctWork[0][NCA_X];			// ��
				rc.bottom = m_ctWork[0][NCA_Y];			// ���s
				rc.high   = m_ctWork[0][NCA_Z];			// ����
				rc.OffsetRect(m_ctWork[1][NCA_X], m_ctWork[1][NCA_Y], m_ctWork[1][NCA_Z]);	// X, Y, Z �̾��
			}
			// ���X�V
			pDoc->SetWorkRect(TRUE, rc);
		}
	}
}

void CNCWorkDlg::OnHide() 
{
	CMDIChildWnd* pFrame = AfxGetNCVCMainWnd()->MDIGetActive();
	if ( pFrame && pFrame->IsKindOf(RUNTIME_CLASS(CNCChild)) ) {
		CNCDoc*	pDoc = static_cast<CNCDoc *>(pFrame->GetActiveDocument());
		if ( pDoc && pDoc->IsKindOf(RUNTIME_CLASS(CNCDoc)) )
			pDoc->SetWorkRect(FALSE, CRect3D());
	}
}

void CNCWorkDlg::OnRecover()
{
	CMDIChildWnd* pFrame = AfxGetNCVCMainWnd()->MDIGetActive();
	if ( pFrame && pFrame->IsKindOf(RUNTIME_CLASS(CNCChild)) ) {
		CNCDoc*	pDoc = static_cast<CNCDoc *>(pFrame->GetActiveDocument());
		if ( pDoc && pDoc->IsKindOf(RUNTIME_CLASS(CNCDoc)) )
			SetValue(pDoc, pDoc->GetWorkRectOrg());		// Org
	}
}

void CNCWorkDlg::OnCancel() 
{
	// ����޳�ʒu�ۑ�
	AfxGetNCVCApp()->SaveDlgWindow(IDS_REGKEY_WINDOW_WORKDLG, this);

	DestroyWindow();	// Ӱ��ڽ�޲�۸�
//	__super::OnCancel();
}

void CNCWorkDlg::PostNcDestroy() 
{
	AfxGetNCVCMainWnd()->SetModelessDlg(MLD_NCWORK, NULL);
	delete	this;
//	__super::PostNcDestroy();
}

LRESULT CNCWorkDlg::OnUserSwitchDocument(WPARAM, LPARAM)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CNCWorkDlg::OnUserSwitchDocument()\nCalling");
#endif
	CMDIChildWnd* pFrame = AfxGetNCVCMainWnd()->MDIGetActive();
	if ( pFrame && pFrame->IsKindOf(RUNTIME_CLASS(CNCChild)) ) {
		CNCDoc*	pDoc = static_cast<CNCDoc *>(pFrame->GetActiveDocument());
		if ( pDoc && pDoc->IsKindOf(RUNTIME_CLASS(CNCDoc)) ) {
			EnableButton(TRUE, pDoc->IsNCDocFlag(NCDOC_LATHE));
			SetValue(pDoc, pDoc->GetWorkRect());
			return 0;
		}
	}

	EnableButton(FALSE, FALSE);
	return 0;
}
