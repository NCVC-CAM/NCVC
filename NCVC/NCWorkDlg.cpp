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

extern	LPCTSTR	g_szNdelimiter;	// "XYZRIJKPLDH" from NCDoc.cpp

BEGIN_MESSAGE_MAP(CNCWorkDlg, CDialog)
	//{{AFX_MSG_MAP(CNCWorkDlg)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_WORK_HIDE, OnHide)
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
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNCWorkDlg)
	DDX_Control(pDX, IDC_WORK_HIDE, m_ctHide);
	DDX_Control(pDX, IDOK, m_ctOK);
	//}}AFX_DATA_MAP
	int		i, j, n;
	for ( i=0; i<SIZEOF(m_ctWork); i++ ) {
		for ( j=0; j<NCXYZ; j++ ) {
			n = i * NCXYZ + j;
			DDX_Control(pDX, n+IDC_WORK_X, m_ctWork[i][j]);
		}
	}
}

void CNCWorkDlg::SaveValue(void)
{
//	UpdateData();
	int		i, j;
	CString	strRegKey(GetSubTreeRegKey(IDS_REGKEY_NC, IDS_REGKEY_WINDOW_WORKDLG)),
			strEntry, strTmp;
	for ( i=0; i<SIZEOF(m_ctWork); i++ ) {
		for ( j=0; j<NCXYZ; j++ ) {
			strEntry.Format(IDS_MAKENCD_FORMAT, (double)m_ctWork[i][j]);
			AfxGetApp()->WriteProfileString(strRegKey, g_szNdelimiter[j]+strTmp, strEntry);
		}
		strTmp = 'O';
	}

	// 移行完了
	VERIFY(strEntry.LoadString(IDS_REG_CONVERT));
	AfxGetApp()->WriteProfileInt(strRegKey, strEntry, 1);
}

/////////////////////////////////////////////////////////////////////////////
// CNCWorkDlg メッセージ ハンドラ

BOOL CNCWorkDlg::OnInitDialog() 
{

	CDialog::OnInitDialog();
	int		i, j;

	// ｺﾝｽﾄﾗｸﾀでは初期化できないｺﾝﾄﾛｰﾙの初期化
	CString	strRegKey(GetSubTreeRegKey(IDS_REGKEY_NC, IDS_REGKEY_WINDOW_WORKDLG)),	// StdAfx.h
			strEntry, strTmp;
	VERIFY(strEntry.LoadString(IDS_REG_CONVERT));
	// X,Y,Z, XO,YO,ZO のﾊﾟﾗﾒｰﾀ読み込み
	if ( AfxGetApp()->GetProfileInt(strRegKey, strEntry, 0) == 0 ) {
		// 旧版:Int型読み込み
		for ( i=0; i<SIZEOF(m_ctWork); i++ ) {
			for ( j=0; j<NCXYZ; j++ )
				m_ctWork[i][j] = (int)AfxGetApp()->GetProfileInt(strRegKey, g_szNdelimiter[j]+strTmp, 0);
			strTmp = 'O';
		}
	}
	else {
		// 新版:double(STR)型読み込み
		for ( i=0; i<SIZEOF(m_ctWork); i++ ) {
			for ( j=0; j<NCXYZ; j++ ) {
				strEntry = AfxGetApp()->GetProfileString(strRegKey, g_szNdelimiter[j]+strTmp);
				m_ctWork[i][j] = strEntry.IsEmpty() ? 0 : atof(strEntry);
			}
			strTmp = 'O';
		}
	}
//	UpdateData(FALSE);

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
	// ﾄﾞｷｭﾒﾝﾄﾋﾞｭｰへの変更通知
	// このｲﾍﾞﾝﾄが発生するときは，OnUserSwitchDocument() より
	// 必ず AfxGetNCVCMainWnd()->MDIGetActive() が CNCChild を指している
	CNCChild* pFrame = static_cast<CNCChild *>(AfxGetNCVCMainWnd()->MDIGetActive());
	if ( pFrame && pFrame->IsKindOf(RUNTIME_CLASS(CNCChild)) ) {	// 念のため
//		UpdateData();
		CRect3D		rc(0, 0,
			m_ctWork[0][NCA_X], m_ctWork[0][NCA_Y],	// 幅(r), 奥行(b),
			m_ctWork[0][NCA_Z], 0);					// 高さ(w)
		CPoint3D	pt(m_ctWork[1][NCA_X], m_ctWork[1][NCA_Y], m_ctWork[1][NCA_Z]);	// X, Y, Z ｵﾌｾｯﾄ
		rc.OffsetRect(pt);
		// 情報更新
		pFrame->SetWorkRect(TRUE, rc, pt);
	}
}

void CNCWorkDlg::OnHide() 
{
	// 情報更新
	CNCChild* pFrame = static_cast<CNCChild *>(AfxGetNCVCMainWnd()->MDIGetActive());
	if ( pFrame && pFrame->IsKindOf(RUNTIME_CLASS(CNCChild)) )
		pFrame->SetWorkRect(FALSE, CRect3D(), CPoint3D());
}

void CNCWorkDlg::OnCancel() 
{
	// ｳｨﾝﾄﾞｳ位置保存
	AfxGetNCVCApp()->SaveDlgWindow(IDS_REGKEY_WINDOW_WORKDLG, this);
	// 設定値保存
	SaveValue();

	DestroyWindow();	// ﾓｰﾄﾞﾚｽﾀﾞｲｱﾛｸﾞ
//	CDialog::OnCancel();
}

void CNCWorkDlg::OnDestroy() 
{
	// 設定値保存
	SaveValue();
	CDialog::OnDestroy();
}

void CNCWorkDlg::PostNcDestroy() 
{
	AfxGetNCVCMainWnd()->SetModelessDlg(MLD_NCWORK, NULL);
	delete	this;
//	CDialog::PostNcDestroy();
}

LRESULT CNCWorkDlg::OnUserSwitchDocument(WPARAM, LPARAM)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CNCWorkDlg::OnUserSwitchDocument()\nCalling");
#endif
	CMDIChildWnd* pFrame = AfxGetNCVCMainWnd()->MDIGetActive();
	if ( pFrame && pFrame->IsKindOf(RUNTIME_CLASS(CNCChild)) ) {
		EnableButton(TRUE);
		CNCDoc*	pDoc = static_cast<CNCDoc *>(pFrame->GetActiveDocument());
		if ( pDoc && pDoc->IsKindOf(RUNTIME_CLASS(CNCDoc)) && pDoc->IsWorkRect() ) {
			CRect3D		rc(pDoc->GetWorkRect());
			CPoint3D	pt(pDoc->GetWorkRectOffset());
			m_ctWork[0][NCA_X] = rc.Width();
			m_ctWork[0][NCA_Y] = rc.Height();
			m_ctWork[0][NCA_Z] = rc.Depth();
			m_ctWork[1][NCA_X] = pt.x;
			m_ctWork[1][NCA_Y] = pt.y;
			m_ctWork[1][NCA_Z] = pt.z;
//			UpdateData(FALSE);
		}
	}
	else
		EnableButton(FALSE);
	return 0;
}
