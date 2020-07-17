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

extern	LPCTSTR	g_szNdelimiter;	// "XYZRIJKPLDH" from NCDoc.cpp

BEGIN_MESSAGE_MAP(CNCWorkDlg, CDialog)
	//{{AFX_MSG_MAP(CNCWorkDlg)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_WORK_HIDE, OnHide)
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
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNCWorkDlg)
	DDX_Control(pDX, IDC_WORK_HIDE, m_ctHide);
	DDX_Control(pDX, IDOK, m_ctOK);
	//}}AFX_DATA_MAP
	int		i, j, n;
	for ( i=0; i<SIZEOF(m_ctWork); i++ ) {
		for ( j=0; j<NCXYZ; j++ ) {
			n = i * NCXYZ + j;
			DDX_Control(pDX, n+IDC_WORK_X, m_ctWork[i][j]);
		}
	}
}

void CNCWorkDlg::SaveValue(void)
{
//	UpdateData();
	int		i, j;
	CString	strRegKey(GetSubTreeRegKey(IDS_REGKEY_NC, IDS_REGKEY_WINDOW_WORKDLG)),
			strEntry, strTmp;
	for ( i=0; i<SIZEOF(m_ctWork); i++ ) {
		for ( j=0; j<NCXYZ; j++ ) {
			strEntry.Format(IDS_MAKENCD_FORMAT, (double)m_ctWork[i][j]);
			AfxGetApp()->WriteProfileString(strRegKey, g_szNdelimiter[j]+strTmp, strEntry);
		}
		strTmp = 'O';
	}

	// �ڍs����
	VERIFY(strEntry.LoadString(IDS_REG_CONVERT));
	AfxGetApp()->WriteProfileInt(strRegKey, strEntry, 1);
}

/////////////////////////////////////////////////////////////////////////////
// CNCWorkDlg ���b�Z�[�W �n���h��

BOOL CNCWorkDlg::OnInitDialog() 
{

	CDialog::OnInitDialog();
	int		i, j;

	// �ݽ�׸��ł͏������ł��Ȃ����۰ق̏�����
	CString	strRegKey(GetSubTreeRegKey(IDS_REGKEY_NC, IDS_REGKEY_WINDOW_WORKDLG)),	// StdAfx.h
			strEntry, strTmp;
	VERIFY(strEntry.LoadString(IDS_REG_CONVERT));
	// X,Y,Z, XO,YO,ZO �����Ұ��ǂݍ���
	if ( AfxGetApp()->GetProfileInt(strRegKey, strEntry, 0) == 0 ) {
		// ����:Int�^�ǂݍ���
		for ( i=0; i<SIZEOF(m_ctWork); i++ ) {
			for ( j=0; j<NCXYZ; j++ )
				m_ctWork[i][j] = (int)AfxGetApp()->GetProfileInt(strRegKey, g_szNdelimiter[j]+strTmp, 0);
			strTmp = 'O';
		}
	}
	else {
		// �V��:double(STR)�^�ǂݍ���
		for ( i=0; i<SIZEOF(m_ctWork); i++ ) {
			for ( j=0; j<NCXYZ; j++ ) {
				strEntry = AfxGetApp()->GetProfileString(strRegKey, g_szNdelimiter[j]+strTmp);
				m_ctWork[i][j] = strEntry.IsEmpty() ? 0 : atof(strEntry);
			}
			strTmp = 'O';
		}
	}
//	UpdateData(FALSE);

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
	// �޷�����ޭ��ւ̕ύX�ʒm
	// ���̲���Ă���������Ƃ��́COnUserSwitchDocument() ���
	// �K�� AfxGetNCVCMainWnd()->MDIGetActive() �� CNCChild ���w���Ă���
	CNCChild* pFrame = static_cast<CNCChild *>(AfxGetNCVCMainWnd()->MDIGetActive());
	if ( pFrame && pFrame->IsKindOf(RUNTIME_CLASS(CNCChild)) ) {	// �O�̂���
//		UpdateData();
		CRect3D		rc(0, 0,
			m_ctWork[0][NCA_X], m_ctWork[0][NCA_Y],	// ��(r), ���s(b),
			m_ctWork[0][NCA_Z], 0);					// ����(w)
		CPoint3D	pt(m_ctWork[1][NCA_X], m_ctWork[1][NCA_Y], m_ctWork[1][NCA_Z]);	// X, Y, Z �̾��
		rc.OffsetRect(pt);
		// ���X�V
		pFrame->SetWorkRect(TRUE, rc, pt);
	}
}

void CNCWorkDlg::OnHide() 
{
	// ���X�V
	CNCChild* pFrame = static_cast<CNCChild *>(AfxGetNCVCMainWnd()->MDIGetActive());
	if ( pFrame && pFrame->IsKindOf(RUNTIME_CLASS(CNCChild)) )
		pFrame->SetWorkRect(FALSE, CRect3D(), CPoint3D());
}

void CNCWorkDlg::OnCancel() 
{
	// ����޳�ʒu�ۑ�
	AfxGetNCVCApp()->SaveDlgWindow(IDS_REGKEY_WINDOW_WORKDLG, this);
	// �ݒ�l�ۑ�
	SaveValue();

	DestroyWindow();	// Ӱ��ڽ�޲�۸�
//	CDialog::OnCancel();
}

void CNCWorkDlg::OnDestroy() 
{
	// �ݒ�l�ۑ�
	SaveValue();
	CDialog::OnDestroy();
}

void CNCWorkDlg::PostNcDestroy() 
{
	AfxGetNCVCMainWnd()->SetModelessDlg(MLD_NCWORK, NULL);
	delete	this;
//	CDialog::PostNcDestroy();
}

LRESULT CNCWorkDlg::OnUserSwitchDocument(WPARAM, LPARAM)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CNCWorkDlg::OnUserSwitchDocument()\nCalling");
#endif
	CMDIChildWnd* pFrame = AfxGetNCVCMainWnd()->MDIGetActive();
	if ( pFrame && pFrame->IsKindOf(RUNTIME_CLASS(CNCChild)) ) {
		EnableButton(TRUE);
		CNCDoc*	pDoc = static_cast<CNCDoc *>(pFrame->GetActiveDocument());
		if ( pDoc && pDoc->IsKindOf(RUNTIME_CLASS(CNCDoc)) && pDoc->IsWorkRect() ) {
			CRect3D		rc(pDoc->GetWorkRect());
			CPoint3D	pt(pDoc->GetWorkRectOffset());
			m_ctWork[0][NCA_X] = rc.Width();
			m_ctWork[0][NCA_Y] = rc.Height();
			m_ctWork[0][NCA_Z] = rc.Depth();
			m_ctWork[1][NCA_X] = pt.x;
			m_ctWork[1][NCA_Y] = pt.y;
			m_ctWork[1][NCA_Z] = pt.z;
//			UpdateData(FALSE);
		}
	}
	else
		EnableButton(FALSE);
	return 0;
}
