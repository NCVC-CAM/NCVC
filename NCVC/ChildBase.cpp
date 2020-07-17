// ChildBase.cpp: CChildBase クラスのインプリメンテーション
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
	// １番目のMDI子ﾌﾚｰﾑｳｨﾝﾄﾞｳか，現在ｱｸﾃｨﾌﾞな子ﾌﾚｰﾑが最大化のとき
	BOOL	fMax = FALSE;
	if ( !AfxGetNCVCMainWnd()->MDIGetActive(&fMax) || fMax )
		nCmdShow = SW_SHOWMAXIMIZED;
	return nCmdShow;
}

void CChildBase::OnMDIActivate(CMDIChildWnd* pChild, BOOL bActivate)
{
	// ﾓｰﾄﾞﾚｽﾀﾞｲｱﾛｸﾞへのﾄﾞｷｭﾒﾝﾄ切替通知
	if ( bActivate )
		AfxGetNCVCMainWnd()->AllModelessDlg_PostSwitchMessage();
}

void CChildBase::OnUserFileChangeNotify(CMDIChildWnd* pChildFrame)
{
	if ( m_bNotify )	// 複数通知の防止
		return;

	// ｳｨﾝﾄﾞｳﾀｲﾄﾙの点滅
	AfxGetMainWnd()->FlashWindowEx(FLASHW_ALL, 5, 0);

	m_bNotify = TRUE;
	pChildFrame->MDIActivate();	// ここで子MDI最上位かつGetActiveDocument()でﾊﾟｽ名取得
	CString	strMsg;
	strMsg.Format(IDS_ANA_FILECHANGE, pChildFrame->GetActiveDocument()->GetPathName());
	int	nResult = AfxMessageBox(strMsg, MB_YESNO|MB_ICONQUESTION);
	AfxGetMainWnd()->FlashWindowEx(FLASHW_STOP, 0, 0);	// 先にｳｨﾝﾄﾞｳﾌﾗｯｼｭを止めておく
	if ( nResult == IDYES )
//		AfxGetNCVCMainWnd()->SendMessage(WM_USERFILECHANGENOTIFY);
		AfxGetNCVCMainWnd()->PostMessage(WM_USERFILECHANGENOTIFY);

	m_bNotify = FALSE;
}
