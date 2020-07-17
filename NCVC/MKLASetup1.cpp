// MKLASetup1.cpp : 実装ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "ExecOption.h"
#include "NCMakeLatheOpt.h"
#include "MKLASetup.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CMKLASetup1, CPropertyPage)
	ON_BN_CLICKED(IDC_MKNC1_HEADER_LOOPUP, &CMKLASetup1::OnHeaderLoopup)
	ON_BN_CLICKED(IDC_MKNC1_FOOTER_LOOPUP, &CMKLASetup1::OnFooterLoopup)
	ON_BN_CLICKED(IDC_MKNC1_HEADER_EDIT, &CMKLASetup1::OnHeaderEdit)
	ON_BN_CLICKED(IDC_MKNC1_FOOTER_EDIT, &CMKLASetup1::OnFooterEdit)
END_MESSAGE_MAP()

#define	GetParentSheet()	static_cast<CMKLASetup *>(GetParentSheet())

/////////////////////////////////////////////////////////////////////////////
// CMKLASetup1 プロパティ ページ

CMKLASetup1::CMKLASetup1() : CPropertyPage(CMKLASetup1::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;
}

void CMKLASetup1::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MKNC1_FOOTER_EDIT, m_ctButton2);
	DDX_Control(pDX, IDC_MKNC1_HEADER_EDIT, m_ctButton1);
	DDX_Control(pDX, IDC_MKNC1_HEADER, m_ctHeader);
	DDX_Control(pDX, IDC_MKNC1_FOOTER, m_ctFooter);
	DDX_Control(pDX, IDC_MKNC1_SPINDLE, m_nSpindle);
	DDX_Control(pDX, IDC_MKNC1_FEED, m_dFeed);
	DDX_Control(pDX, IDC_MKLA1_XFEED, m_dXFeed);
	DDX_Control(pDX, IDC_MKLA1_CUT, m_dCut);
	DDX_Control(pDX, IDC_MKLA1_PULL_Z, m_dPullZ);
	DDX_Control(pDX, IDC_MKLA1_PULL_X, m_dPullX);
	DDX_Control(pDX, IDC_MKLA1_MARGIN, m_dMargin);
	DDX_Control(pDX, IDC_MKLA1_MARGINNUM, m_nMargin);
	DDX_Text(pDX, IDC_MKNC1_FOOTER, m_strFooter);
	DDX_Text(pDX, IDC_MKNC1_HEADER, m_strHeader);
}

/////////////////////////////////////////////////////////////////////////////
// CMKLASetup1 メッセージ ハンドラ

BOOL CMKLASetup1::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	// ｶｽﾀﾑｺﾝﾄﾛｰﾙはｺﾝｽﾄﾗｸﾀで初期化できない
	// + GetParentSheet() ﾎﾟｲﾝﾀを取得できない
	CNCMakeLatheOpt* pOpt = GetParentSheet()->GetNCMakeOption();
	m_nSpindle	= pOpt->m_nSpindle;
	m_dFeed		= pOpt->m_dFeed;
	m_dXFeed	= pOpt->m_dXFeed;
	m_dCut		= pOpt->m_dCut * 2.0;	// 直径値へ変換
	m_dPullZ	= pOpt->m_dPullZ;
	m_dPullX	= pOpt->m_dPullX * 2.0;
	m_dMargin	= pOpt->m_dMargin * 2.0;
	m_nMargin	= pOpt->m_nMargin;
	m_strHeader = pOpt->m_strOption[MKLA_STR_HEADER];
	m_strFooter = pOpt->m_strOption[MKLA_STR_FOOTER];
	// 編集ﾎﾞﾀﾝの有効無効
	if ( AfxGetNCVCApp()->GetExecList()->GetCount() < 1 ) {
		m_ctButton1.EnableWindow(FALSE);
		m_ctButton2.EnableWindow(FALSE);
	}

	UpdateData(FALSE);

	return TRUE;
}

void CMKLASetup1::OnHeaderLoopup() 
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

void CMKLASetup1::OnFooterLoopup() 
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

void CMKLASetup1::OnHeaderEdit() 
{
	ASSERT( AfxGetNCVCApp()->GetExecList()->GetCount() > 0 );
	UpdateData();
	AfxGetNCVCMainWnd()->CreateOutsideProcess(
		AfxGetNCVCApp()->GetExecList()->GetHead()->GetFileName(), "\""+m_strHeader+"\"");
	m_ctHeader.SetFocus();
}

void CMKLASetup1::OnFooterEdit() 
{
	ASSERT( AfxGetNCVCApp()->GetExecList()->GetCount() > 0 );
	UpdateData();
	AfxGetNCVCMainWnd()->CreateOutsideProcess(
		AfxGetNCVCApp()->GetExecList()->GetHead()->GetFileName(), "\""+m_strFooter+"\"");
	m_ctFooter.SetFocus();
}

BOOL CMKLASetup1::OnApply() 
{
	CNCMakeLatheOpt* pOpt = GetParentSheet()->GetNCMakeOption();
	pOpt->m_nSpindle	= m_nSpindle;
	pOpt->m_dFeed		= m_dFeed;
	pOpt->m_dXFeed		= m_dXFeed;
	pOpt->m_dCut		= m_dCut / 2.0;	// 半径値へ変換
	pOpt->m_dPullZ		= m_dPullZ;
	pOpt->m_dPullX		= m_dPullX / 2.0;
	pOpt->m_dMargin		= m_dMargin / 2.0;
	pOpt->m_nMargin		= m_nMargin;
	pOpt->m_strOption[MKLA_STR_HEADER] = m_strHeader;
	pOpt->m_strOption[MKLA_STR_FOOTER] = m_strFooter;

	return TRUE;
}

BOOL CMKLASetup1::OnKillActive() 
{
	if ( !CPropertyPage::OnKillActive() )
		return FALSE;

	if ( m_dFeed <= 0 ) {
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_dFeed.SetFocus();
		m_dFeed.SetSel(0, -1);
		return FALSE;
	}
	if ( m_dXFeed <= 0 ) {
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_dXFeed.SetFocus();
		m_dXFeed.SetSel(0, -1);
		return FALSE;
	}
	if ( m_dCut <= 0 ) {
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_dCut.SetFocus();
		m_dCut.SetSel(0, -1);
		return FALSE;
	}
	if ( m_dPullZ <= 0 ) {
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_dPullZ.SetFocus();
		m_dPullZ.SetSel(0, -1);
		return FALSE;
	}
	if ( m_dPullX <= 0 ) {
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_dPullX.SetFocus();
		m_dPullX.SetSel(0, -1);
		return FALSE;
	}
	if ( m_dMargin <= 0 ) {
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_dMargin.SetFocus();
		m_dMargin.SetSel(0, -1);
		return FALSE;
	}
	if ( m_nMargin < 0 ) {		// 回数はｾﾞﾛOK
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_nMargin.SetFocus();
		m_nMargin.SetSel(0, -1);
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

	return TRUE;
}
