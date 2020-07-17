// NCWorkDlg.cpp : インプリメンテーション ファイル
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

BEGIN_MESSAGE_MAP(CNCWorkDlg, CDialog)
	//{{AFX_MSG_MAP(CNCWorkDlg)
	ON_BN_CLICKED(IDC_WORK_HIDE, OnHide)
	ON_BN_CLICKED(IDC_WORK_RECOVER, OnRecover)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USERSWITCHDOCUMENT, OnUserSwitchDocument)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNCWorkDlg ダイアログ

CNCWorkDlg::CNCWorkDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNCWorkDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNCWorkDlg)
	//}}AFX_DATA_INIT
}

void CNCWorkDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNCWorkDlg)
	DDX_Control(pDX, IDOK, m_ctOK);
	DDX_Control(pDX, IDC_WORK_HIDE, m_ctHide);
	DDX_Control(pDX, IDC_WORK_RECOVER, m_ctRecover);
	//}}AFX_DATA_MAP
	int		i, j, n;
	for ( i=0; i<SIZEOF(m_ctWork); i++ ) {
		for ( j=0; j<NCXYZ; j++ ) {
			n = i * NCXYZ + j;
			DDX_Control(pDX, n+IDC_WORK_X,  m_ctWork[i][j]);
			DDX_Control(pDX, n+IDC_WORK_XS, m_ctLabel[i][j]);
		}
	}
}

void CNCWorkDlg::EnableButton(BOOL bEnable, BOOL bLathe)
{
	static	LPCTSTR	szLabel[][NCXYZ] = {
		{"幅(&W)", "奥行(&D)", "高さ(&H)"},
		{"径(&D)", "Zmin(&L)", "Zmax(&R)"}
	};
	int		i, n = (bEnable & bLathe) ? 1 : 0;

	m_ctOK.EnableWindow(bEnable);
	m_ctHide.EnableWindow(bEnable);
	m_ctRecover.EnableWindow(bEnable);

	for ( i=0; i<NCXYZ; i++ ) {
		// 大きさ指示は bEnable
		m_ctWork[0][i].EnableWindow(bEnable);
		// ｵﾌｾｯﾄ指示は bEnable & !bLathe
		m_ctWork[1][i].EnableWindow(bEnable & !bLathe);
		// ｵﾌｾｯﾄﾗﾍﾞﾙは !bLathe
		m_ctLabel[1][i].EnableWindow(!bLathe);
		// ﾗﾍﾞﾙ変更
		m_ctLabel[0][i].SetWindowText(szLabel[n][i]);
	}
}

void CNCWorkDlg::SetValue(const CNCDoc* pDoc, const CRect3D& rc)
{
	if ( pDoc->IsNCDocFlag(NCDOC_LATHE) ) {
		m_ctWork[0][NCA_X] = rc.high * 2.0;		// 直径表示
		m_ctWork[0][NCA_Y] = rc.left;
		m_ctWork[0][NCA_Z] = rc.right;
		m_ctWork[1][NCA_X] = 0;
		m_ctWork[1][NCA_Y] = 0;
		m_ctWork[1][NCA_Z] = 0;
	}
	else {
		m_ctWork[0][NCA_X] = rc.Width();
		m_ctWork[0][NCA_Y] = rc.Height();
		m_ctWork[0][NCA_Z] = rc.Depth();
		m_ctWork[1][NCA_X] = rc.left;
		m_ctWork[1][NCA_Y] = rc.top;
		m_ctWork[1][NCA_Z] = rc.low;
	}
	UpdateData(FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// CNCWorkDlg メッセージ ハンドラ

BOOL CNCWorkDlg::OnInitDialog() 
{
	extern	LPCTSTR	g_szNdelimiter;	// "XYZUVWIJKRPLDH" from NCDoc.cpp

	__super::OnInitDialog();
	int		i, j;

	// ｺﾝｽﾄﾗｸﾀでは初期化できないｺﾝﾄﾛｰﾙの初期化
	CString	strRegKey(GetSubTreeRegKey(IDS_REGKEY_NC, IDS_REGKEY_WINDOW_WORKDLG)),	// StdAfx.h
			strTmp;

	// ﾚｼﾞｽﾄﾘに保存されたﾜｰｸ矩形情報は削除
	for ( i=0; i<SIZEOF(m_ctWork); i++ ) {
		for ( j=0; j<NCXYZ; j++ )
			AfxGetApp()->WriteProfileString(strRegKey, g_szNdelimiter[j]+strTmp, NULL);
		strTmp = 'O';
	}

	// 表示/非表示ﾎﾞﾀﾝの初期化
	OnUserSwitchDocument(NULL, NULL);

	// ｳｨﾝﾄﾞｳ位置読み込み
	CPoint	pt;
	if ( AfxGetNCVCApp()->GetDlgWindow(IDS_REGKEY_WINDOW_WORKDLG, &pt) )
		SetWindowPos(NULL, pt.x, pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	return TRUE;  // コントロールにフォーカスを設定しないとき、戻り値は TRUE となります
	              // 例外: OCX プロパティ ページの戻り値は FALSE となります
}

void CNCWorkDlg::OnOK() 
{
	CWaitCursor		wait;

	// ﾄﾞｷｭﾒﾝﾄﾋﾞｭｰへの変更通知
	// このｲﾍﾞﾝﾄが発生するときは，OnUserSwitchDocument() より
	// 必ず AfxGetNCVCMainWnd()->MDIGetActive() が CNCChild を指している
	CMDIChildWnd* pFrame = AfxGetNCVCMainWnd()->MDIGetActive();
	if ( pFrame && pFrame->IsKindOf(RUNTIME_CLASS(CNCChild)) ) {
		CNCDoc*	pDoc = static_cast<CNCDoc *>(pFrame->GetActiveDocument());
		if ( pDoc && pDoc->IsKindOf(RUNTIME_CLASS(CNCDoc)) ) {
			CRect3D	rc;
			if ( pDoc->IsNCDocFlag(NCDOC_LATHE) ) {
				rc.high   = m_ctWork[0][NCA_X] / 2.0;	// 径
				rc.left   = m_ctWork[0][NCA_Y];			// Zmin
				rc.right  = m_ctWork[0][NCA_Z];			// Zmax
			}
			else {
				rc.left   = m_ctWork[0][NCA_X];			// 幅
				rc.bottom = m_ctWork[0][NCA_Y];			// 奥行
				rc.high   = m_ctWork[0][NCA_Z];			// 高さ
				rc.OffsetRect(m_ctWork[1][NCA_X], m_ctWork[1][NCA_Y], m_ctWork[1][NCA_Z]);	// X, Y, Z ｵﾌｾｯﾄ
			}
			// 情報更新
			pDoc->SetWorkRect(TRUE, rc);
		}
	}
}

void CNCWorkDlg::OnHide() 
{
	CMDIChildWnd* pFrame = AfxGetNCVCMainWnd()->MDIGetActive();
	if ( pFrame && pFrame->IsKindOf(RUNTIME_CLASS(CNCChild)) ) {
		CNCDoc*	pDoc = static_cast<CNCDoc *>(pFrame->GetActiveDocument());
		if ( pDoc && pDoc->IsKindOf(RUNTIME_CLASS(CNCDoc)) )
			pDoc->SetWorkRect(FALSE, CRect3D());
	}
}

void CNCWorkDlg::OnRecover()
{
	CMDIChildWnd* pFrame = AfxGetNCVCMainWnd()->MDIGetActive();
	if ( pFrame && pFrame->IsKindOf(RUNTIME_CLASS(CNCChild)) ) {
		CNCDoc*	pDoc = static_cast<CNCDoc *>(pFrame->GetActiveDocument());
		if ( pDoc && pDoc->IsKindOf(RUNTIME_CLASS(CNCDoc)) )
			SetValue(pDoc, pDoc->GetWorkRectOrg());		// Org
	}
}

void CNCWorkDlg::OnCancel() 
{
	// ｳｨﾝﾄﾞｳ位置保存
	AfxGetNCVCApp()->SaveDlgWindow(IDS_REGKEY_WINDOW_WORKDLG, this);

	DestroyWindow();	// ﾓｰﾄﾞﾚｽﾀﾞｲｱﾛｸﾞ
//	__super::OnCancel();
}

void CNCWorkDlg::PostNcDestroy() 
{
	AfxGetNCVCMainWnd()->SetModelessDlg(MLD_NCWORK, NULL);
	delete	this;
//	__super::PostNcDestroy();
}

LRESULT CNCWorkDlg::OnUserSwitchDocument(WPARAM, LPARAM)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CNCWorkDlg::OnUserSwitchDocument()\nCalling");
#endif
	CMDIChildWnd* pFrame = AfxGetNCVCMainWnd()->MDIGetActive();
	if ( pFrame && pFrame->IsKindOf(RUNTIME_CLASS(CNCChild)) ) {
		CNCDoc*	pDoc = static_cast<CNCDoc *>(pFrame->GetActiveDocument());
		if ( pDoc && pDoc->IsKindOf(RUNTIME_CLASS(CNCDoc)) ) {
			EnableButton(TRUE, pDoc->IsNCDocFlag(NCDOC_LATHE));
			SetValue(pDoc, pDoc->GetWorkRect());
			return 0;
		}
	}

	EnableButton(FALSE, FALSE);
	return 0;
}
