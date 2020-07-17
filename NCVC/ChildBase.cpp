// ChildBase.cpp: CChildBase �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "ChildBase.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

/////////////////////////////////////////////////////////////////////////////
// CChildBase

int CChildBase::ActivateFrameSP(int nCmdShow)
{
	// �P�Ԗڂ�MDI�q�ڰѳ���޳���C���ݱ�è�ނȎq�ڰт��ő剻�̂Ƃ�
	BOOL	fMax = FALSE;
	if ( !AfxGetNCVCMainWnd()->MDIGetActive(&fMax) || fMax )
		nCmdShow = SW_SHOWMAXIMIZED;
	return nCmdShow;
}

void CChildBase::OnMDIActivate(CMDIChildWnd* pChild, BOOL bActivate)
{
	// Ӱ��ڽ�޲�۸ނւ��޷���Đؑ֒ʒm
	if ( bActivate )
		AfxGetNCVCMainWnd()->AllModelessDlg_PostSwitchMessage();
}

void CChildBase::OnUserFileChangeNotify(CMDIChildWnd* pChildFrame)
{
	if ( m_bNotify )	// �����ʒm�̖h�~
		return;

	// ����޳���ق̓_��
	AfxGetMainWnd()->FlashWindowEx(FLASHW_ALL, 5, 0);

	m_bNotify = TRUE;
	pChildFrame->MDIActivate();	// �����ŎqMDI�ŏ�ʂ���GetActiveDocument()���߽���擾
	CString	strMsg;
	strMsg.Format(IDS_ANA_FILECHANGE, pChildFrame->GetActiveDocument()->GetPathName());
	int	nResult = AfxMessageBox(strMsg, MB_YESNO|MB_ICONQUESTION);
	AfxGetMainWnd()->FlashWindowEx(FLASHW_STOP, 0, 0);	// ��ɳ���޳�ׯ�����~�߂Ă���
	if ( nResult == IDYES )
//		AfxGetNCVCMainWnd()->SendMessage(WM_USERFILECHANGENOTIFY);
		AfxGetNCVCMainWnd()->PostMessage(WM_USERFILECHANGENOTIFY);

	m_bNotify = FALSE;
}
