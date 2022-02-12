// NCJumpDlg.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "NCChild.h"
#include "NCdata.h"
#include "NCDoc.h"
#include "NCJumpDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CNCJumpDlg, CDialog)
	//{{AFX_MSG_MAP(CNCJumpDlg)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USERSWITCHDOCUMENT, &CNCJumpDlg::OnUserSwitchDocument)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNCJumpDlg �_�C�A���O

CNCJumpDlg::CNCJumpDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNCJumpDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNCJumpDlg)
	//}}AFX_DATA_INIT
}

void CNCJumpDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNCJumpDlg)
	DDX_Control(pDX, IDOK, m_ctOK);
	DDX_Control(pDX, IDC_JUMP, m_nJump);
	//}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////
// CNCJumpDlg ���b�Z�[�W �n���h��

BOOL CNCJumpDlg::OnInitDialog() 
{
	__super::OnInitDialog();

	// �װ������΂��̍s����̫��(�ŏ�����)
	// ���Ѻ��۰ق̂��߁C�ݽ�׸��ł͕s�\
	m_nJump = 1;
	CMDIChildWnd*	pChild = AfxGetNCVCMainWnd()->MDIGetActive();
	CNCDoc*			pDoc   = pChild ? (CNCDoc *)(pChild->GetActiveDocument()) : NULL;
	if ( pDoc ) {
		for ( int i=0; i<pDoc->GetNCBlockSize(); i++ ) {
			if ( pDoc->GetNCblock(i)->GetNCBlkErrorCode() > 0 ) {
				m_nJump = i + 1;
				break;
			}
		}
	}

	// �ړ����݂̏�����
	OnUserSwitchDocument(NULL, NULL);

	// ����޳�ʒu�ǂݍ���
	CPoint	pt;
	if ( AfxGetNCVCApp()->GetDlgWindow(IDS_REGKEY_WINDOW_JUMPDLG, &pt) )
		SetWindowPos(NULL, pt.x, pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	return TRUE;  // �R���g���[���Ƀt�H�[�J�X��ݒ肵�Ȃ��Ƃ��A�߂�l�� TRUE �ƂȂ�܂�
	              // ��O: OCX �v���p�e�B �y�[�W�̖߂�l�� FALSE �ƂȂ�܂�
}

void CNCJumpDlg::OnOK() 
{
	// �޷�����ޭ��ւ̕ύX�ʒm
	// ���̲���Ă���������Ƃ��́COnUserSwitchDocument() ���
	// �K�� AfxGetNCVCMainWnd()->MDIGetActive() �� CNCChild ���w���Ă���
	CNCChild* pFrame = static_cast<CNCChild *>(AfxGetNCVCMainWnd()->MDIGetActive());
	if ( pFrame && pFrame->IsKindOf(RUNTIME_CLASS(CNCChild)) ) {	// �O�̂���
		UpdateData();
		pFrame->SetJumpList((int)m_nJump - 1);	// Zero Origin
	}
}

void CNCJumpDlg::OnCancel() 
{
	// ����޳�ʒu�ۑ�
	AfxGetNCVCApp()->SaveDlgWindow(IDS_REGKEY_WINDOW_JUMPDLG, this);

	DestroyWindow();	// Ӱ��ڽ�޲�۸�
//	__super::OnCancel();
}

void CNCJumpDlg::PostNcDestroy() 
{
	AfxGetNCVCMainWnd()->SetModelessDlg(MLD_NCJUMP, NULL);
	delete	this;
//	__super::PostNcDestroy();
}

LRESULT CNCJumpDlg::OnUserSwitchDocument(WPARAM, LPARAM)
{
#ifdef _DEBUG
	printf("CNCJumpDlg::OnUserSwitchDocument() Calling\n");
#endif
	CMDIChildWnd* pFrame = AfxGetNCVCMainWnd()->MDIGetActive();

	BOOL	bEnable = pFrame && pFrame->IsKindOf(RUNTIME_CLASS(CNCChild)) ? TRUE : FALSE;
	m_ctOK.EnableWindow(bEnable);
	m_nJump.EnableWindow(bEnable);

	return 0;
}
