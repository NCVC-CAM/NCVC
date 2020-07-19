// OBSdlg.cpp : �����t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "NCChild.h"
#include "OBSdlg.h"
#include "MCOption.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(COBSdlg, CDialog)
	ON_BN_CLICKED(IDC_MCST5_ALLON, &COBSdlg::OnAllON)
	ON_BN_CLICKED(IDC_MCST5_ALLOFF, &COBSdlg::OnAllOFF)
	ON_BN_CLICKED(IDC_OBS_WRITE, &COBSdlg::OnObsWrite)
	ON_MESSAGE (WM_USERSWITCHDOCUMENT, &COBSdlg::OnUserSwitchDocument)
	ON_MESSAGE (WM_USERFILECHANGENOTIFY, &COBSdlg::OnUserSwitchMachine)
END_MESSAGE_MAP()

// COBSdlg �_�C�A���O

COBSdlg::COBSdlg(CWnd* pParent /*=NULL*/)
	: CDialog(IDD_OBS, pParent)
{
	GetOBSdata();
}

void COBSdlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDOK, m_ctOK);
	DDX_Text(pDX, IDC_OBS_MCFILE, m_strMCFile);
	for ( int i=0; i<SIZEOF(m_bOBS); i++ )
		DDX_Check(pDX, IDC_MCST5_OBS0+i, m_bOBS[i]);
}

BOOL COBSdlg::PreTranslateMessage(MSG* pMsg) 
{
	// Ӱ��ڽ�޲�۸ނ�Ҳݳ���޳�̷��ް�ޱ���ڰ���L���ɂ���
	CFrameWnd*	pFrame = GetParentFrame();	// CMainFrame
	if ( pFrame && ::TranslateAccelerator(pFrame->GetSafeHwnd(), pFrame->GetDefaultAccelerator(), pMsg))
		return TRUE;
	return __super::PreTranslateMessage(pMsg);
}

void COBSdlg::GetOBSdata(void)
{
	CMCOption*	pMCopt = AfxGetNCVCApp()->GetMCOption();
	for ( int i=0; i<SIZEOF(m_bOBS); i++ )
		m_bOBS[i] = pMCopt->m_bOBS[i];
	CString		strPath;
	::Path_Name_From_FullPath(pMCopt->GetMCHeadFileName(), strPath, m_strMCFile);
}

void COBSdlg::SetOBSdata(void)
{
	CMCOption*	pMCopt = AfxGetNCVCApp()->GetMCOption();
	for ( int i=0; i<SIZEOF(m_bOBS); i++ )
		pMCopt->m_bOBS[i] = m_bOBS[i];
}

// COBSdlg ���b�Z�[�W �n���h���[

BOOL COBSdlg::OnInitDialog() 
{
	__super::OnInitDialog();

	// �ړ����݂̏�����
	OnUserSwitchDocument(NULL, NULL);

	// ����޳�ʒu�ǂݍ���
	CPoint	pt;
	if ( AfxGetNCVCApp()->GetDlgWindow(IDS_REGKEY_WINDOW_OBSDLG, &pt) )
		SetWindowPos(NULL, pt.x, pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	return TRUE;  // �R���g���[���Ƀt�H�[�J�X��ݒ肵�Ȃ��Ƃ��A�߂�l�� TRUE �ƂȂ�܂�
	              // ��O: OCX �v���p�e�B �y�[�W�̖߂�l�� FALSE �ƂȂ�܂�
}

void COBSdlg::OnObsWrite()
{
	UpdateData();
	SetOBSdata();
	CMCOption*	pMCopt = AfxGetNCVCApp()->GetMCOption();
	CString		strName(pMCopt->GetMCHeadFileName()), strMsg;
	if ( pMCopt->SaveMCoption(strName) ) {
		strMsg.Format(IDS_ANA_FILEOUTPUT, strName);
		AfxMessageBox(strMsg, MB_OK|MB_ICONINFORMATION);
	}
	else {
		strMsg.Format(IDS_ERR_WRITESETTING, strName);
		AfxMessageBox(strMsg, MB_OK|MB_ICONEXCLAMATION);
	}
}

void COBSdlg::OnOK() 
{
	UpdateData();
	SetOBSdata();
	// �ēǂݍ��ݒʒm
	AfxGetNCVCMainWnd()->PostMessage(WM_USERFILECHANGENOTIFY);
}

void COBSdlg::OnCancel() 
{
	// ����޳�ʒu�ۑ�
	AfxGetNCVCApp()->SaveDlgWindow(IDS_REGKEY_WINDOW_OBSDLG, this);

	DestroyWindow();	// Ӱ��ڽ�޲�۸�
//	__super::OnCancel();
}

void COBSdlg::PostNcDestroy() 
{
	AfxGetNCVCMainWnd()->SetModelessDlg(MLD_NCOBS, NULL);
	delete	this;
//	__super::PostNcDestroy();
}

void COBSdlg::OnAllON()
{
	for ( int i=0; i<SIZEOF(m_bOBS); i++ )
		m_bOBS[i] = TRUE;
	UpdateData(FALSE);
}

void COBSdlg::OnAllOFF()
{
	for ( int i=0; i<SIZEOF(m_bOBS); i++ )
		m_bOBS[i] = FALSE;
	UpdateData(FALSE);
}

LRESULT COBSdlg::OnUserSwitchDocument(WPARAM, LPARAM)
{
#ifdef _DEBUG
	printf("COBSdlg::OnUserSwitchDocument() Calling\n");
#endif
	CMDIChildWnd* pFrame = AfxGetNCVCMainWnd()->MDIGetActive();

	BOOL	bEnable = pFrame && pFrame->IsKindOf(RUNTIME_CLASS(CNCChild)) ? TRUE : FALSE;
	m_ctOK.EnableWindow(bEnable);

	return 0;
}

LRESULT COBSdlg::OnUserSwitchMachine(WPARAM, LPARAM)
{
#ifdef _DEBUG
	printf("COBSdlg::OnUserSwitchMachine() Calling\n");
#endif
	GetOBSdata();
	UpdateData(FALSE);

	return 0;
}
