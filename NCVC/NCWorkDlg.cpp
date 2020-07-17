// NCWorkDlg.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "NCChild.h"
#include "NCWorkDlg.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

static	const	int		RANGEMAX[] = {
	20000, 5000, 5000, 5000, 5000, 5000
};

BEGIN_MESSAGE_MAP(CNCWorkDlg, CDialog)
	//{{AFX_MSG_MAP(CNCWorkDlg)
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
	for ( i=0; i<SIZEOF(m_WorkNum); i++ ) {
		for ( j=0; j<NCXYZ; j++ ) {
			n = i * NCXYZ + j;
			DDX_Control(pDX, n+IDC_WORK_X, m_WorkNum[i][j]);
			DDV_MinMaxInt(pDX, m_WorkNum[i][j], -RANGEMAX[n], RANGEMAX[n]);
			DDX_Control(pDX, n+IDC_WORK_XSPIN, m_Spin[i][j]);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// CNCWorkDlg メッセージ ハンドラ

BOOL CNCWorkDlg::OnInitDialog() 
{
	extern	LPCTSTR	g_szNdelimiter;	// "XYZRIJKPLDH" from NCDoc.cpp

	CDialog::OnInitDialog();
	int		i, j, n;

	// ｺﾝｽﾄﾗｸﾀでは初期化できないｺﾝﾄﾛｰﾙの初期化
	CString	strRegKey(GetSubTreeRegKey(IDS_REGKEY_NC, IDS_REGKEY_WINDOW_WORKDLG)),
				strEntry, strTmp;
	// X,Y,Z, XO,YO,ZO のﾊﾟﾗﾒｰﾀ読み込み
	for ( i=0; i<SIZEOF(m_WorkNum); i++ ) {
		for ( j=0; j<NCXYZ; j++ )
			m_WorkNum[i][j] = AfxGetApp()->GetProfileInt(strRegKey, g_szNdelimiter[i]+strTmp, 0);
		strTmp = 'O';
	}
	UpdateData(FALSE);

	// 表示/非表示ﾎﾞﾀﾝの初期化
	OnUserSwitchDocument(NULL, NULL);

	// ｽﾋﾟﾝﾎﾞﾀﾝの設定
	UDACCEL		uda[5];
	uda[0].nSec = 0;		uda[0].nInc = 1;
	uda[1].nSec = 1;		uda[1].nInc = 2;
	uda[2].nSec = 2;		uda[2].nInc = 5;
	uda[3].nSec = 4;		uda[2].nInc = 10;
	uda[4].nSec = 6;		uda[2].nInc = 50;
	for ( i=0; i<SIZEOF(m_Spin); i++ ) {
		for ( j=0; j<NCXYZ; j++ ) {
			n = i * NCXYZ + j;
			m_Spin[i][j].SetAccel(SIZEOF(uda), uda);
			m_Spin[i][j].SetRange(-RANGEMAX[n], RANGEMAX[n]);
		}
	}

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
		UpdateData();
		CRect3D	rc(0, 0,
			m_WorkNum[0][NCA_X], m_WorkNum[0][NCA_Y],	// 幅(r), 奥行(b),
			0, m_WorkNum[0][NCA_Z]);					// 高さ(w)
		rc.OffsetRect(m_WorkNum[1][NCA_X], m_WorkNum[1][NCA_Y], m_WorkNum[1][NCA_Z]);	// X, Y, Z ｵﾌｾｯﾄ
		// 情報更新
		pFrame->SetWorkRect(TRUE, rc);
	}
}

void CNCWorkDlg::OnHide() 
{
	// 情報更新
	CNCChild* pFrame = static_cast<CNCChild *>(AfxGetNCVCMainWnd()->MDIGetActive());
	if ( pFrame && pFrame->IsKindOf(RUNTIME_CLASS(CNCChild)) )
		pFrame->SetWorkRect(FALSE, CRect3D());
}

void CNCWorkDlg::OnCancel() 
{
	extern	LPCTSTR	g_szNdelimiter;	// "XYZRIJKPLDH" from NCDoc.cpp

	// ｳｨﾝﾄﾞｳ位置保存
	AfxGetNCVCApp()->SaveDlgWindow(IDS_REGKEY_WINDOW_WORKDLG, this);
	UpdateData();
	// 設定値保存
	int		i, j;
	CString	strRegKey(GetSubTreeRegKey(IDS_REGKEY_NC, IDS_REGKEY_WINDOW_WORKDLG)),
				strEntry, strTmp;
	for ( i=0; i<SIZEOF(m_WorkNum); i++ ) {
		for ( j=0; j<NCXYZ; j++ )
			AfxGetApp()->WriteProfileInt(strRegKey, g_szNdelimiter[i]+strTmp, m_WorkNum[i][j]);
		strTmp = 'O';
	}

	DestroyWindow();	// ﾓｰﾄﾞﾚｽﾀﾞｲｱﾛｸﾞ
//	CDialog::OnCancel();
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
	EnableButton( pFrame && pFrame->IsKindOf(RUNTIME_CLASS(CNCChild)) ? TRUE : FALSE );
	return 0;
}
