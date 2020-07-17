// MKNCSetup1.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "ExecOption.h"
#include "NCMakeMillOpt.h"
#include "MKNCSetup.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CMKNCSetup1, CPropertyPage)
	//{{AFX_MSG_MAP(CMKNCSetup1)
	ON_BN_CLICKED(IDC_MKNC1_HEADER_LOOPUP, &CMKNCSetup1::OnHeaderLoopup)
	ON_BN_CLICKED(IDC_MKNC1_FOOTER_LOOPUP, &CMKNCSetup1::OnFooterLoopup)
	ON_BN_CLICKED(IDC_MKNC1_HEADER_EDIT, &CMKNCSetup1::OnHeaderEdit)
	ON_BN_CLICKED(IDC_MKNC1_FOOTER_EDIT, &CMKNCSetup1::OnFooterEdit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

#define	GetParentSheet()	static_cast<CMKNCSetup *>(GetParentSheet())

/////////////////////////////////////////////////////////////////////////////
// CMKNCSetup1 プロパティ ページ

CMKNCSetup1::CMKNCSetup1() : CPropertyPage(CMKNCSetup1::IDD),
	m_dFeed(TRUE), m_dZFeed(TRUE)	// CFloatEditﾌﾗｸﾞ設定
{
	m_psp.dwFlags &= ~PSP_HASHELP;
	//{{AFX_DATA_INIT(CMKNCSetup1)
	m_bXrev = FALSE;
	m_bYrev = FALSE;
	//}}AFX_DATA_INIT
}

void CMKNCSetup1::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMKNCSetup1)
	DDX_Control(pDX, IDC_MKNC1_ZFEED, m_dZFeed);
	DDX_Control(pDX, IDC_MKNC1_FEED, m_dFeed);
	DDX_Control(pDX, IDC_MKNC1_HEADER_EDIT, m_ctHeaderBt);
	DDX_Control(pDX, IDC_MKNC1_FOOTER_EDIT, m_ctFooterBt);
	DDX_Control(pDX, IDC_MKNC1_HEADER, m_ctHeader);
	DDX_Control(pDX, IDC_MKNC1_FOOTER, m_ctFooter);
	DDX_Control(pDX, IDC_MKNC1_ZCUT, m_dZCut);
	DDX_Control(pDX, IDC_MKNC1_R, m_dZG0Stop);
	DDX_Control(pDX, IDC_MKNC1_SPINDLE, m_nSpindle);
	DDX_Text(pDX, IDC_MKNC1_FOOTER, m_strFooter);
	DDX_Text(pDX, IDC_MKNC1_HEADER, m_strHeader);
	DDX_Check(pDX, IDC_MKNC1_XREV, m_bXrev);
	DDX_Check(pDX, IDC_MKNC1_YREV, m_bYrev);
	//}}AFX_DATA_MAP
	for ( int i=0; i<NCXYZ; i++ )
		DDX_Control(pDX, i+IDC_MKNC1_G92X, m_dG92[i]);
}

/////////////////////////////////////////////////////////////////////////////
// CMKNCSetup1 メッセージ ハンドラ

BOOL CMKNCSetup1::OnInitDialog() 
{
	__super::OnInitDialog();

	// ｶｽﾀﾑｺﾝﾄﾛｰﾙはｺﾝｽﾄﾗｸﾀで初期化できない
	// + GetParentSheet() ﾎﾟｲﾝﾀを取得できない
	CNCMakeMillOpt* pOpt = GetParentSheet()->GetNCMakeOption();
	m_nSpindle	= pOpt->MIL_I_SPINDLE;
	m_dFeed		= pOpt->MIL_D_FEED;
	m_dZFeed	= pOpt->MIL_D_ZFEED;
	m_dZCut		= pOpt->MIL_D_ZCUT;
	m_dZG0Stop	= pOpt->MIL_D_ZG0STOP;
	for ( int i=0; i<NCXYZ; i++ )
		m_dG92[i] = pOpt->m_pDblOpt[MKNC_DBL_G92X+i];
	m_strHeader = pOpt->m_strOption[MKNC_STR_HEADER];
	m_strFooter = pOpt->m_strOption[MKNC_STR_FOOTER];
	m_bXrev		= pOpt->MIL_F_XREV;
	m_bYrev		= pOpt->MIL_F_YREV;
	// 編集ﾎﾞﾀﾝの有効無効
	if ( AfxGetNCVCApp()->GetExecList()->GetCount() < 1 ) {
		m_ctHeaderBt.EnableWindow(FALSE);
		m_ctFooterBt.EnableWindow(FALSE);
	}

	UpdateData(FALSE);

	return TRUE;  // コントロールにフォーカスを設定しないとき、戻り値は TRUE となります
	              // 例外: OCX プロパティ ページの戻り値は FALSE となります
}

void CMKNCSetup1::OnHeaderLoopup() 
{
	UpdateData();
	CString		strPath, strFile;
	::Path_Name_From_FullPath(m_strHeader, strPath, strFile);
	if ( !strFile.IsEmpty() )
		strFile = m_strHeader;
	if ( ::NCVC_FileDlgCommon(IDS_CUSTOMFILE, IDS_TXT_FILTER, TRUE, strFile, strPath) == IDOK ) {
		// ﾃﾞｰﾀの反映
		m_strHeader = strFile;
		UpdateData(FALSE);
		// 文字選択状態
		m_ctHeader.SetFocus();
		m_ctHeader.SetSel(0, -1);
	}
}

void CMKNCSetup1::OnFooterLoopup() 
{
	UpdateData();
	CString		strPath, strFile;
	::Path_Name_From_FullPath(m_strFooter, strPath, strFile);
	if ( !strFile.IsEmpty() )
		strFile = m_strFooter;
	if ( ::NCVC_FileDlgCommon(IDS_CUSTOMFILE, IDS_TXT_FILTER, TRUE, strFile, strPath) == IDOK ) {
		// ﾃﾞｰﾀの反映
		m_strFooter = strFile;
		UpdateData(FALSE);
		// 文字選択状態
		m_ctFooter.SetFocus();
		m_ctFooter.SetSel(0, -1);
	}
}

void CMKNCSetup1::OnHeaderEdit() 
{
	ASSERT( AfxGetNCVCApp()->GetExecList()->GetCount() > 0 );
	UpdateData();
	AfxGetNCVCMainWnd()->CreateOutsideProcess(
		AfxGetNCVCApp()->GetExecList()->GetHead()->GetFileName(), "\""+m_strHeader+"\"");
	m_ctHeader.SetFocus();
}

void CMKNCSetup1::OnFooterEdit() 
{
	ASSERT( AfxGetNCVCApp()->GetExecList()->GetCount() > 0 );
	UpdateData();
	AfxGetNCVCMainWnd()->CreateOutsideProcess(
		AfxGetNCVCApp()->GetExecList()->GetHead()->GetFileName(), "\""+m_strFooter+"\"");
	m_ctFooter.SetFocus();
}

BOOL CMKNCSetup1::OnApply() 
{
	// ﾍﾟｰｼﾞ間の依存関係
	// OnKillActive() ではﾍﾟｰｼﾞを切り替えられないのでうっとおしい
	int		nMakeEnd;
	BOOL	bDeep;
	double	dDeep, dMakeValue;
	CMKNCSetup*	pParent = GetParentSheet();
	CNCMakeMillOpt* pOpt = pParent->GetNCMakeOption();

	if ( ::IsWindow(pParent->m_dlg3.m_hWnd) ) {
		nMakeEnd	= pParent->m_dlg3.m_nMakeEnd;
		dMakeValue	= pParent->m_dlg3.m_dMakeValue;
		bDeep		= pParent->m_dlg3.m_bDeep;
		dDeep		= pParent->m_dlg3.m_dDeep;
	}
	else {
		nMakeEnd	= pOpt->MIL_I_MAKEEND;
		dMakeValue	= pOpt->MIL_D_MAKEEND;
		bDeep		= pOpt->MIL_F_DEEP;
		dDeep		= pOpt->MIL_D_DEEP;
	}

	if ( nMakeEnd == 2 ) {
		if ( dMakeValue > m_dZG0Stop ) {
			AfxMessageBox(IDS_ERR_DEEPFIXR, MB_OK|MB_ICONEXCLAMATION);
			m_dZG0Stop.SetFocus();
			m_dZG0Stop.SetSel(0, -1);
			return FALSE;
		}
		if ( dMakeValue <= m_dZCut ) {
			AfxMessageBox(IDS_ERR_DEEPFIXZ, MB_OK|MB_ICONEXCLAMATION);
			m_dZCut.SetFocus();
			m_dZCut.SetSel(0, -1);
			return FALSE;
		}
	}
	if ( bDeep && m_dZCut<dDeep ) {
		AfxMessageBox(IDS_ERR_DEEPFINAL, MB_OK|MB_ICONEXCLAMATION);
		m_dZCut.SetFocus();
		m_dZCut.SetSel(0, -1);
		return FALSE;
	}

	pOpt->MIL_I_SPINDLE	= m_nSpindle;
	pOpt->MIL_D_FEED	= m_dFeed;
	pOpt->MIL_D_ZFEED	= m_dZFeed;
	pOpt->MIL_D_ZCUT	= m_dZCut;
	pOpt->MIL_D_ZG0STOP	= m_dZG0Stop;
	for ( int i=0; i<NCXYZ; i++ )
		pOpt->m_pDblOpt[MKNC_DBL_G92X+i] = m_dG92[i];
	pOpt->m_strOption[MKNC_STR_HEADER] = m_strHeader;
	pOpt->m_strOption[MKNC_STR_FOOTER] = m_strFooter;
	pOpt->MIL_F_XREV	= m_bXrev;
	pOpt->MIL_F_YREV	= m_bYrev;

	return TRUE;
}

BOOL CMKNCSetup1::OnKillActive() 
{
	if ( !__super::OnKillActive() )
		return FALSE;

	if ( m_dFeed <= 0 ) {
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_dFeed.SetFocus();
		m_dFeed.SetSel(0, -1);
		return FALSE;
	}
	if ( m_dZFeed <= 0 ) {
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_dZFeed.SetFocus();
		m_dZFeed.SetSel(0, -1);
		return FALSE;
	}
	if ( m_strHeader.IsEmpty() ) {
		AfxMessageBox(IDS_ERR_CUSTOMFILE, MB_OK|MB_ICONEXCLAMATION);
		m_ctHeader.SetFocus();
		return FALSE;
	}
	if ( m_strFooter.IsEmpty() ) {
		AfxMessageBox(IDS_ERR_CUSTOMFILE, MB_OK|MB_ICONEXCLAMATION);
		m_ctFooter.SetFocus();
		return FALSE;
	}
	if ( (double)m_dZCut > (double)m_dZG0Stop ) {
		AfxMessageBox(IDS_ERR_ZCUT, MB_OK|MB_ICONEXCLAMATION);
		m_dZCut.SetFocus();
		m_dZCut.SetSel(0, -1);
		return FALSE;
	}

	return TRUE;
}
