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

BEGIN_MESSAGE_MAP(CChildBase, CMDIChildWnd)
	ON_WM_CLOSE()
	// ﾌｧｲﾙ変更通知 from DocBase.cpp
	ON_MESSAGE(WM_USERFILECHANGENOTIFY, &CChildBase::OnUserFileChangeNotify)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChildBase

void CChildBase::ActivateFrame(int nCmdShow)
{
#ifdef _DEBUG
	g_dbg.printf("CDXFChild::ActivateFrame() Call");
#endif
	// １番目のMDI子ﾌﾚｰﾑｳｨﾝﾄﾞｳか，現在ｱｸﾃｨﾌﾞな子ﾌﾚｰﾑが最大化のとき
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
	if ( m_bNotify )	// 複数通知の防止
		return 0;

	// ｳｨﾝﾄﾞｳﾀｲﾄﾙの点滅
	AfxGetMainWnd()->FlashWindowEx(FLASHW_ALL, 5, 0);

	m_bNotify = TRUE;
	MDIActivate();	// ここで子MDI最上位かつGetActiveDocument()でﾊﾟｽ名取得
	CString	strMsg;
	strMsg.Format(IDS_ANA_FILECHANGE, GetActiveDocument()->GetPathName());
	int	nResult = AfxMessageBox(strMsg, MB_YESNO|MB_ICONQUESTION);
	AfxGetMainWnd()->FlashWindowEx(FLASHW_STOP, 0, 0);	// 先にｳｨﾝﾄﾞｳﾌﾗｯｼｭを止めておく
	if ( nResult == IDYES )
//		AfxGetNCVCMainWnd()->SendMessage(WM_USERFILECHANGENOTIFY);
		AfxGetNCVCMainWnd()->PostMessage(WM_USERFILECHANGENOTIFY);

	m_bNotify = FALSE;

	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CChildBase クラスの診断

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
