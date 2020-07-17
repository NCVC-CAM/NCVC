// MKNCSetup1.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "ExecOption.h"
#include "NCMakeOption.h"
#include "MKNCSetup.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CMKNCSetup1, CPropertyPage)
	//{{AFX_MSG_MAP(CMKNCSetup1)
	ON_BN_CLICKED(IDC_MKNC1_HEADER_LOOPUP, OnHeaderLoopup)
	ON_BN_CLICKED(IDC_MKNC1_FOOTER_LOOPUP, OnFooterLoopup)
	ON_BN_CLICKED(IDC_MKNC1_HEADER_EDIT, OnHeaderEdit)
	ON_BN_CLICKED(IDC_MKNC1_FOOTER_EDIT, OnFooterEdit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMKNCSetup1 プロパティ ページ

CMKNCSetup1::CMKNCSetup1() : CPropertyPage(CMKNCSetup1::IDD),
	m_dFeed(TRUE), m_dZFeed(TRUE)	// CFloatEditﾌﾗｸﾞ設定
{
	m_psp.dwFlags &= ~PSP_HASHELP;
	//{{AFX_DATA_INIT(CMKNCSetup1)
	//}}AFX_DATA_INIT
}

CMKNCSetup1::~CMKNCSetup1()
{
}

void CMKNCSetup1::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMKNCSetup1)
	DDX_Control(pDX, IDC_MKNC1_ZFEED, m_dZFeed);
	DDX_Control(pDX, IDC_MKNC1_FEED, m_dFeed);
	DDX_Control(pDX, IDC_MKNC1_FOOTER_EDIT, m_ctButton2);
	DDX_Control(pDX, IDC_MKNC1_HEADER_EDIT, m_ctButton1);
	DDX_Control(pDX, IDC_MKNC1_HEADER, m_ctHeader);
	DDX_Control(pDX, IDC_MKNC1_FOOTER, m_ctFooter);
	DDX_Control(pDX, IDC_MKNC1_ZCUT, m_dZCut);
	DDX_Control(pDX, IDC_MKNC1_R, m_dZG0Stop);
	DDX_Control(pDX, IDC_MKNC1_SPINDLE, m_nSpindle);
	DDX_Text(pDX, IDC_MKNC1_FOOTER, m_strFooter);
	DDX_Text(pDX, IDC_MKNC1_HEADER, m_strHeader);
	//}}AFX_DATA_MAP
	for ( int i=0; i<NCXYZ; i++ )
		DDX_Control(pDX, i+IDC_MKNC1_G92X, m_dG92[i]);
}

/////////////////////////////////////////////////////////////////////////////
// CMKNCSetup1 メッセージ ハンドラ

BOOL CMKNCSetup1::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	// ｶｽﾀﾑｺﾝﾄﾛｰﾙはｺﾝｽﾄﾗｸﾀで初期化できない
	// + GetParent() ﾎﾟｲﾝﾀを取得できない
	CNCMakeOption* pOpt = static_cast<CMKNCSetup *>(GetParent())->GetNCMakeOption();
	m_nSpindle	= pOpt->m_nSpindle;
	m_dFeed		= pOpt->m_dFeed;
	m_dZFeed	= pOpt->m_dZFeed;
	m_dZCut		= pOpt->m_dZCut;
	m_dZG0Stop	= pOpt->m_dZG0Stop;
	for ( int i=0; i<NCXYZ; i++ )
		m_dG92[i] = pOpt->m_dG92[i];
	m_strHeader = pOpt->m_strOption[MKNC_STR_HEADER];
	m_strFooter = pOpt->m_strOption[MKNC_STR_FOOTER];
	// 編集ﾎﾞﾀﾝの有効無効
	if ( AfxGetNCVCApp()->GetExecList()->GetCount() < 1 ) {
		m_ctButton1.EnableWindow(FALSE);
		m_ctButton2.EnableWindow(FALSE);
	}

	UpdateData(FALSE);

	return TRUE;  // コントロールにフォーカスを設定しないとき、戻り値は TRUE となります
	              // 例外: OCX プロパティ ページの戻り値は FALSE となります
}

void CMKNCSetup1::OnHeaderLoopup() 
{
	UpdateData();
	CString		strPath, strFile, strInitialDir;
	::Path_Name_From_FullPath(m_strHeader, strPath, strFile);
	if ( !strFile.IsEmpty() )
		strFile = m_strHeader;
	else
		strInitialDir = strPath;
	if ( ::NCVC_FileDlgCommon(IDS_CUSTOMFILE, IDS_TXT_FILTER, strFile, strPath) == IDOK ) {
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
	CString		strPath, strFile, strInitialDir;
	::Path_Name_From_FullPath(m_strHeader, strPath, strFile);
	if ( !strFile.IsEmpty() )
		strFile = m_strHeader;
	else
		strInitialDir = strPath;
	if ( ::NCVC_FileDlgCommon(IDS_CUSTOMFILE, IDS_TXT_FILTER, strFile, strPath) == IDOK ) {
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
	CMKNCSetup*	pParent = static_cast<CMKNCSetup *>(GetParent());
	CNCMakeOption* pOpt = pParent->GetNCMakeOption();

	if ( ::IsWindow(pParent->m_dlg3.m_hWnd) ) {
		nMakeEnd	= pParent->m_dlg3.m_nMakeEnd;
		dMakeValue	= pParent->m_dlg3.m_dMakeValue;
		bDeep		= pParent->m_dlg3.m_bDeep;
		dDeep		= pParent->m_dlg3.m_dDeep;
	}
	else {
		nMakeEnd	= pOpt->m_nMakeEnd;
		dMakeValue	= pOpt->m_dMakeValue;
		bDeep		= pOpt->m_bDeep;
		dDeep		= pOpt->m_dDeep;
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

	pOpt->m_nSpindle	= m_nSpindle;
	pOpt->m_dFeed		= m_dFeed;
	pOpt->m_dZFeed		= m_dZFeed;
	pOpt->m_dZCut		= m_dZCut;
	pOpt->m_dZG0Stop	= m_dZG0Stop;
	for ( int i=0; i<NCXYZ; i++ )
		pOpt->m_dG92[i] = m_dG92[i];
	pOpt->m_strOption[MKNC_STR_HEADER] = m_strHeader;
	pOpt->m_strOption[MKNC_STR_FOOTER] = m_strFooter;

	return TRUE;
}

BOOL CMKNCSetup1::OnKillActive() 
{
	if ( !CPropertyPage::OnKillActive() )
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
