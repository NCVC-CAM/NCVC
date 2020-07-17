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

BEGIN_MESSAGE_MAP(CChildBase, CMDIChildWnd)
	ON_WM_CLOSE()
	// ̧�ٕύX�ʒm from DocBase.cpp
	ON_MESSAGE(WM_USERFILECHANGENOTIFY, &CChildBase::OnUserFileChangeNotify)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChildBase

void CChildBase::ActivateFrame(int nCmdShow)
{
#ifdef _DEBUG
	g_dbg.printf("CDXFChild::ActivateFrame() Call");
#endif
	// �P�Ԗڂ�MDI�q�ڰѳ���޳���C���ݱ�è�ނȎq�ڰт��ő剻�̂Ƃ�
	BOOL	fMax = FALSE;
	if ( !AfxGetNCVCMainWnd()->MDIGetActive(&fMax) || fMax )
		nCmdShow = SW_SHOWMAXIMIZED;

	__super::ActivateFrame(nCmdShow);
}

void CChildBase::OnClose() 
{
	AfxGetNCVCMainWnd()->AllModelessDlg_PostSwitchMessage();
	__super::OnClose();
}

LRESULT CChildBase::OnUserFileChangeNotify(WPARAM, LPARAM)
{
	if ( m_bNotify )	// �����ʒm�̖h�~
		return 0;

	// ����޳���ق̓_��
	AfxGetMainWnd()->FlashWindowEx(FLASHW_ALL, 5, 0);

	m_bNotify = TRUE;
	MDIActivate();	// �����ŎqMDI�ŏ�ʂ���GetActiveDocument()���߽���擾
	CString	strMsg;
	strMsg.Format(IDS_ANA_FILECHANGE, GetActiveDocument()->GetPathName());
	int	nResult = AfxMessageBox(strMsg, MB_YESNO|MB_ICONQUESTION);
	AfxGetMainWnd()->FlashWindowEx(FLASHW_STOP, 0, 0);	// ��ɳ���޳�ׯ�����~�߂Ă���
	if ( nResult == IDYES )
//		AfxGetNCVCMainWnd()->SendMessage(WM_USERFILECHANGENOTIFY);
		AfxGetNCVCMainWnd()->PostMessage(WM_USERFILECHANGENOTIFY);

	m_bNotify = FALSE;

	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CChildBase �N���X�̐f�f

#ifdef _DEBUG
void CChildBase::AssertValid() const
{
	__super::AssertValid();
}

void CChildBase::Dump(CDumpContext& dc) const
{
	__super::Dump(dc);
}
#endif //_DEBUG
