// NCFindDlg.cpp : �����t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "NCChild.h"
#include "NCFindDlg.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CNCFindDlg, CDialog)
	ON_MESSAGE (WM_USERSWITCHDOCUMENT, OnUserSwitchDocument)
END_MESSAGE_MAP()

// CNCFindDlg �_�C�A���O

CNCFindDlg::CNCFindDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNCFindDlg::IDD, pParent)
{
	m_nUpDown = 1;		// ����
}

CNCFindDlg::~CNCFindDlg()
{
}

void CNCFindDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDOK, m_ctFind);
	DDX_Control(pDX, IDC_NCVIEW_FIND_STR, m_ctStrFind);
	DDX_Control(pDX, IDC_NCVIEW_FIND_UP, m_ctFindUp);
	DDX_Control(pDX, IDC_NCVIEW_FIND_DOWN, m_ctFindDown);
	DDX_Text(pDX, IDC_NCVIEW_FIND_STR, m_strFind);
	DDX_Radio(pDX, IDC_NCVIEW_FIND_UP, m_nUpDown);
}

BOOL CNCFindDlg::PreTranslateMessage(MSG* pMsg) 
{
	// Ӱ��ڽ�޲�۸ނ�Ҳݳ���޳�̷��ް�ޱ���ڰ���L���ɂ���
	CFrameWnd*	pFrame = GetParentFrame();	// CMainFrame
	if ( pFrame && ::TranslateAccelerator(pFrame->GetSafeHwnd(), pFrame->GetDefaultAccelerator(), pMsg))
		return TRUE;
	return __super::PreTranslateMessage(pMsg);
}

// CNCFindDlg ���b�Z�[�W �n���h��

BOOL CNCFindDlg::OnInitDialog() 
{
	__super::OnInitDialog();

	// �ړ����݂̏�����
	OnUserSwitchDocument(NULL, NULL);

	// ����޳�ʒu�ǂݍ���
	CPoint	pt;
	if ( AfxGetNCVCApp()->GetDlgWindow(IDS_REGKEY_WINDOW_FINDDLG, &pt) )
		SetWindowPos(NULL, pt.x, pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	return TRUE;  // �R���g���[���Ƀt�H�[�J�X��ݒ肵�Ȃ��Ƃ��A�߂�l�� TRUE �ƂȂ�܂�
	              // ��O: OCX �v���p�e�B �y�[�W�̖߂�l�� FALSE �ƂȂ�܂�
}

void CNCFindDlg::OnOK() 
{
	UpdateData();
	if ( !m_strFind.IsEmpty() ) {
		// �޷�����ޭ��ւ̕ύX�ʒm
		// ���̲���Ă���������Ƃ��́COnUserSwitchDocument() ���
		// �K�� AfxGetNCVCMainWnd()->MDIGetActive() �� CNCChild ���w���Ă���
		CNCChild* pFrame = static_cast<CNCChild *>(AfxGetNCVCMainWnd()->MDIGetActive());
		if ( pFrame && pFrame->IsKindOf(RUNTIME_CLASS(CNCChild)) )	// �O�̂���
			pFrame->SetFindList(m_nUpDown, m_strFind);
	}
}

void CNCFindDlg::OnCancel() 
{
	// ����޳�ʒu�ۑ�
	AfxGetNCVCApp()->SaveDlgWindow(IDS_REGKEY_WINDOW_FINDDLG, this);

	DestroyWindow();	// Ӱ��ڽ�޲�۸�
//	__super::OnCancel();
}

void CNCFindDlg::PostNcDestroy() 
{
	AfxGetNCVCMainWnd()->SetModelessDlg(MLD_NCFIND, NULL);
	delete	this;
//	__super::PostNcDestroy();
}

LRESULT CNCFindDlg::OnUserSwitchDocument(WPARAM, LPARAM)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CNCFindDlg::OnUserSwitchDocument()\nCalling");
#endif
	CMDIChildWnd* pFrame = AfxGetNCVCMainWnd()->MDIGetActive();

	BOOL	bEnable = pFrame && pFrame->IsKindOf(RUNTIME_CLASS(CNCChild)) ? TRUE : FALSE;
	m_ctFind.EnableWindow(bEnable);
	m_ctStrFind.EnableWindow(bEnable);
	m_ctFindUp.EnableWindow(bEnable);
	m_ctFindDown.EnableWindow(bEnable);

	return 0;
}
