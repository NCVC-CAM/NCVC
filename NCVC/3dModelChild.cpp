// 3dModelChild.cpp : 実装ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "3dModelChild.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// C3dModelChild

IMPLEMENT_DYNCREATE(C3dModelChild, CMDIChildWnd)

BEGIN_MESSAGE_MAP(C3dModelChild, CChildBase)
	ON_WM_MDIACTIVATE()
END_MESSAGE_MAP()

C3dModelChild::C3dModelChild()
{
}

C3dModelChild::~C3dModelChild()
{
}

/////////////////////////////////////////////////////////////////////////////
// C3dModelChild メッセージ ハンドラー

void C3dModelChild::OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd)
{
	__super::OnMDIActivate(bActivate, pActivateWnd, pDeactivateWnd);
#ifdef _DEBUG
	printf("C3dModelChild::bActivate=%d\n", bActivate);
#endif
	if ( bActivate ) {
		// ﾓｰﾄﾞﾚｽﾀﾞｲｱﾛｸﾞへのﾄﾞｷｭﾒﾝﾄ切替通知
		AfxGetNCVCMainWnd()->AllModelessDlg_PostSwitchMessage();
		// アクセラレータテーブルの強制置換
		// DestroyAcceleratorTable() しなくてもいい？
		m_hAccelTable = NULL;	// これがないとMFC側でASSERT
		LoadAccelTable( MAKEINTRESOURCE(IDR_3DMTYPE) );
	}
}
