// NCWorkDlg1.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "NCDoc.h"
#include "NCWorkDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CNCWorkDlg1, CPropertyPage)
	ON_BN_CLICKED(IDC_WORK_SHOW, &CNCWorkDlg1::OnShow)
	ON_BN_CLICKED(IDC_WORK_HIDE, &CNCWorkDlg1::OnHide)
	ON_BN_CLICKED(IDC_WORK_RECOVER, &CNCWorkDlg1::OnRecover)
	ON_BN_CLICKED(IDC_WORK_COMMENT, &CNCWorkDlg1::OnComment)
	ON_MESSAGE(WM_USERSWITCHDOCUMENT, &CNCWorkDlg1::OnUserSwitchDocument)
END_MESSAGE_MAP()

#define	GetParentSheet()	static_cast<CNCWorkDlg *>(GetParentSheet())

/////////////////////////////////////////////////////////////////////////////
// CNCWorkDlg1 ダイアログ

CNCWorkDlg1::CNCWorkDlg1() : CPropertyPage(CNCWorkDlg1::IDD)
{
//	m_psp.dwFlags &= ~PSP_HASHELP;
}

void CNCWorkDlg1::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNCWorkDlg1)
	DDX_Control(pDX, IDC_WORK_SHOW, m_ctShow);
	DDX_Control(pDX, IDC_WORK_HIDE, m_ctHide);
	DDX_Control(pDX, IDC_WORK_RECOVER, m_ctRecover);
	DDX_Control(pDX, IDC_WORK_COMMENT, m_ctComment);
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

void CNCWorkDlg1::EnableButton(BOOL bEnable, BOOL bLathe)
{
	static	LPCTSTR	szLabel[][NCXYZ] = {
		{"幅(&W)", "奥行(&D)", "高さ(&H)"},
		{"径(&D)", "Zmin(&L)", "Zmax(&R)"}
	};
	int		i, n = (bEnable & bLathe) ? 1 : 0;

	m_ctShow.EnableWindow(bEnable);
	m_ctHide.EnableWindow(bEnable);
	m_ctRecover.EnableWindow(bEnable);
	m_ctComment.EnableWindow(bEnable);

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

void CNCWorkDlg1::SetValue(const CNCDoc* pDoc, const CRect3F& rc)
{
	if ( pDoc->IsDocFlag(NCDOC_LATHE) ) {
		m_ctWork[0][NCA_X] = rc.high * 2.0f;	// 直径表示
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
// CNCWorkDlg1 メッセージ ハンドラ

BOOL CNCWorkDlg1::OnInitDialog() 
{
	extern	LPCTSTR	g_szNdelimiter;	// "XYZUVWIJKRPLDH" from NCDoc.cpp

	__super::OnInitDialog();
	int		i, j;

	// ｺﾝｽﾄﾗｸﾀでは初期化できないｺﾝﾄﾛｰﾙの初期化
	CString	strRegKey(GetSubTreeRegKey(IDS_REGKEY_NC, IDS_REGKEY_WINDOW_WORKDLG)),	// stdafx.h
			strTmp;

	// ﾚｼﾞｽﾄﾘに保存されたﾜｰｸ矩形情報は削除
	for ( i=0; i<SIZEOF(m_ctWork); i++ ) {
		for ( j=0; j<NCXYZ; j++ )
			AfxGetApp()->WriteProfileString(strRegKey, g_szNdelimiter[j]+strTmp, NULL);
		strTmp = 'O';
	}

	// 表示/非表示ﾎﾞﾀﾝの初期化
	OnUserSwitchDocument(NULL, NULL);

	return TRUE;  // コントロールにフォーカスを設定しないとき、戻り値は TRUE となります
	              // 例外: OCX プロパティ ページの戻り値は FALSE となります
}

BOOL CNCWorkDlg1::PreTranslateMessage(MSG* pMsg)
{
	// CPropertySheetﾓｰﾄﾞﾚｽでESCが効かないので
	// 強制的にﾒｯｾｰｼﾞを捕まえて閉じる
	if ( pMsg->message == WM_KEYDOWN ) {
		switch ( pMsg->wParam ) {
		case VK_ESCAPE:
			GetParentSheet()->PostMessage(WM_CLOSE);
			return TRUE;
		case VK_RETURN:
			OnShow();
			return TRUE;
		}
	}
	return CPropertyPage::PreTranslateMessage(pMsg);
}

void CNCWorkDlg1::OnShow() 
{
	CWaitCursor		wait;

	// ﾄﾞｷｭﾒﾝﾄﾋﾞｭｰへの変更通知
	// このｲﾍﾞﾝﾄが発生するときは，OnUserSwitchDocument() より
	// 必ず AfxGetNCVCMainWnd()->MDIGetActive() が CNCChild を指している
	CNCDoc*	pDoc = GetParentSheet()->GetNCDocument();
	if ( pDoc ) {
		CRect3F	rc;
		if ( pDoc->IsDocFlag(NCDOC_LATHE) ) {
			rc.high   = m_ctWork[0][NCA_X] / 2.0f;	// 径
			rc.left   = m_ctWork[0][NCA_Y];			// Zmin
			rc.right  = m_ctWork[0][NCA_Z];			// Zmax
		}
		else {
			rc.right  = m_ctWork[0][NCA_X];			// 幅
			rc.bottom = m_ctWork[0][NCA_Y];			// 奥行
			rc.high   = m_ctWork[0][NCA_Z];			// 高さ
			rc.OffsetRect(m_ctWork[1][NCA_X], m_ctWork[1][NCA_Y], m_ctWork[1][NCA_Z]);	// X, Y, Z ｵﾌｾｯﾄ
		}
		// 情報更新
		pDoc->SetWorkRect(TRUE, rc);
	}
}

void CNCWorkDlg1::OnHide() 
{
	CNCDoc*	pDoc = GetParentSheet()->GetNCDocument();
	if ( pDoc )
		pDoc->SetWorkRect(FALSE, CRect3F());
}

void CNCWorkDlg1::OnComment()
{
	extern	LPCTSTR	g_szNCcomment[];	// "WorkRect", "LatheView" from NCDoc.cpp
	CNCDoc*	pDoc = GetParentSheet()->GetNCDocument();
	if ( pDoc ) {
		CString	strComment;
		if ( pDoc->IsDocFlag(NCDOC_LATHE) ) {
			strComment.Format("(%s=%.3f,%.3f,%.3f)",
				LATHEVIEW_S,
				float(m_ctWork[0][NCA_X]), float(m_ctWork[0][NCA_Y]), float(m_ctWork[0][NCA_Z]));
		}
		else {
			strComment.Format("(%s=%.3f,%.3f,%.3f,%.3f,%.3f,%.3f)",
				WORKRECT_S,
				float(m_ctWork[0][NCA_X]), float(m_ctWork[0][NCA_Y]), float(m_ctWork[0][NCA_Z]),
				float(m_ctWork[1][NCA_X]), float(m_ctWork[1][NCA_Y]), float(m_ctWork[1][NCA_Z]));
		}
		pDoc->SetCommentStr(strComment);
		OnShow();	// 表示の更新
	}
}

void CNCWorkDlg1::OnRecover()
{
	CNCDoc*	pDoc = GetParentSheet()->GetNCDocument();
	if ( pDoc )
		SetValue(pDoc, pDoc->GetWorkRectOrg());		// Org
}

LRESULT CNCWorkDlg1::OnUserSwitchDocument(WPARAM, LPARAM)
{
#ifdef _DEBUG
	printf("CNCWorkDlg1::OnUserSwitchDocument() Calling\n");
#endif
	CNCDoc*	pDoc = GetParentSheet()->GetNCDocument();
	if ( pDoc ) {
		// ﾜｲﾔ放電加工機ﾓｰﾄﾞは不可
		EnableButton(!pDoc->IsDocFlag(NCDOC_WIRE), pDoc->IsDocFlag(NCDOC_LATHE));
		SetValue(pDoc, pDoc->GetWorkRect());
	}
	else
		EnableButton(FALSE, FALSE);

	return 0;
}
