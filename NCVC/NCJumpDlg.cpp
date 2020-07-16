// NCJumpDlg.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "NCChild.h"
#include "NCdata.h"
#include "NCDoc.h"
#include "NCJumpDlg.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CNCJumpDlg, CDialog)
	//{{AFX_MSG_MAP(CNCJumpDlg)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USERSWITCHDOCUMENT, OnUserSwitchDocument)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNCJumpDlg ダイアログ

CNCJumpDlg::CNCJumpDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNCJumpDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNCJumpDlg)
	//}}AFX_DATA_INIT
}

void CNCJumpDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNCJumpDlg)
	DDX_Control(pDX, IDOK, m_ctOK);
	DDX_Control(pDX, IDC_JUMP, m_nJump);
	//}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////
// CNCJumpDlg メッセージ ハンドラ

BOOL CNCJumpDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	// ｴﾗｰがあればその行をﾃﾞﾌｫﾙﾄ(最初だけ)
	// ｶｽﾀﾑｺﾝﾄﾛｰﾙのため，ｺﾝｽﾄﾗｸﾀでは不可能
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

	// 移動ﾎﾞﾀﾝの初期化
	OnUserSwitchDocument(NULL, NULL);

	// ｳｨﾝﾄﾞｳ位置読み込み
	CPoint	pt;
	if ( AfxGetNCVCApp()->GetDlgWindow(IDS_REGKEY_WINDOW_JUMPDLG, &pt) )
		SetWindowPos(NULL, pt.x, pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	return TRUE;  // コントロールにフォーカスを設定しないとき、戻り値は TRUE となります
	              // 例外: OCX プロパティ ページの戻り値は FALSE となります
}

void CNCJumpDlg::OnOK() 
{
	// ﾄﾞｷｭﾒﾝﾄﾋﾞｭｰへの変更通知
	// このｲﾍﾞﾝﾄが発生するときは，OnUserSwitchDocument() より
	// 必ず AfxGetNCVCMainWnd()->MDIGetActive() が CNCChild を指している
	CNCChild* pFrame = (CNCChild *)(AfxGetNCVCMainWnd()->MDIGetActive());
	if ( pFrame && pFrame->IsKindOf(RUNTIME_CLASS(CNCChild)) ) {	// 念のため
		UpdateData();
		pFrame->SetJumpList(m_nJump);
	}
}

void CNCJumpDlg::OnCancel() 
{
	// ｳｨﾝﾄﾞｳ位置保存
	AfxGetNCVCApp()->SaveDlgWindow(IDS_REGKEY_WINDOW_JUMPDLG, this);

	DestroyWindow();	// ﾓｰﾄﾞﾚｽﾀﾞｲｱﾛｸﾞ
//	CDialog::OnCancel();
}

void CNCJumpDlg::PostNcDestroy() 
{
	AfxGetNCVCMainWnd()->SetModelessDlg(MLD_NCJUMP, NULL);
	delete	this;
//	CDialog::PostNcDestroy();
}

LRESULT CNCJumpDlg::OnUserSwitchDocument(WPARAM, LPARAM)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CNCJumpDlg::OnUserSwitchDocument()\nCalling");
#endif
	CMDIChildWnd* pFrame = AfxGetNCVCMainWnd()->MDIGetActive();
	EnableButton( pFrame && pFrame->IsKindOf(RUNTIME_CLASS(CNCChild)) ? TRUE : FALSE );
	return 0;
}
