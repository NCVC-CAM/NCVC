// MKWISetup1.cpp : 実装ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "ExecOption.h"
#include "NCMakeWireOpt.h"
#include "MKWISetup.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CMKWISetup1, CPropertyPage)
	ON_BN_CLICKED(IDC_MKNC1_HEADER_LOOPUP, OnHeaderLoopup)
	ON_BN_CLICKED(IDC_MKNC1_FOOTER_LOOPUP, OnFooterLoopup)
	ON_BN_CLICKED(IDC_MKNC1_HEADER_EDIT, OnHeaderEdit)
	ON_BN_CLICKED(IDC_MKNC1_FOOTER_EDIT, OnFooterEdit)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMKWISetup1 プロパティ ページ

CMKWISetup1::CMKWISetup1() : CPropertyPage(CMKWISetup1::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;
}

CMKWISetup1::~CMKWISetup1()
{
}

void CMKWISetup1::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MKNC1_FOOTER_EDIT, m_ctButton2);
	DDX_Control(pDX, IDC_MKNC1_HEADER_EDIT, m_ctButton1);
	DDX_Control(pDX, IDC_MKNC1_HEADER, m_ctHeader);
	DDX_Control(pDX, IDC_MKNC1_FOOTER, m_ctFooter);
	DDX_Control(pDX, IDC_MKNC1_FEED, m_dFeed);
	DDX_Control(pDX, IDC_MKWI1_DEPTH, m_dDepth);
	DDX_Control(pDX, IDC_MKWI1_TAPER, m_dTaper);
	DDX_Control(pDX, IDC_MKNC1_G92X, m_dG92X);
	DDX_Control(pDX, IDC_MKNC1_G92Y, m_dG92Y);
	DDX_Text(pDX, IDC_MKWI1_TAPERMODE, m_strTaperMode);
	DDX_Text(pDX, IDC_MKNC1_FOOTER, m_strFooter);
	DDX_Text(pDX, IDC_MKNC1_HEADER, m_strHeader);
}

/////////////////////////////////////////////////////////////////////////////
// CMKWISetup1 メッセージ ハンドラ

BOOL CMKWISetup1::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	// ｶｽﾀﾑｺﾝﾄﾛｰﾙはｺﾝｽﾄﾗｸﾀで初期化できない
	// + GetParent() ﾎﾟｲﾝﾀを取得できない
	CNCMakeWireOpt* pOpt = static_cast<CMKWISetup *>(GetParent())->GetNCMakeOption();
	m_dDepth		= pOpt->m_dDepth;
	m_dTaper		= pOpt->m_dTaper;
	m_strTaperMode	= pOpt->m_strOption[MKWI_STR_TAPERMODE];
	m_dFeed			= pOpt->m_dFeed;
	m_dG92X			= pOpt->m_dG92X;
	m_dG92Y			= pOpt->m_dG92Y;
	m_strHeader		= pOpt->m_strOption[MKWI_STR_HEADER];
	m_strFooter		= pOpt->m_strOption[MKWI_STR_FOOTER];
	// 編集ﾎﾞﾀﾝの有効無効
	if ( AfxGetNCVCApp()->GetExecList()->GetCount() < 1 ) {
		m_ctButton1.EnableWindow(FALSE);
		m_ctButton2.EnableWindow(FALSE);
	}

	UpdateData(FALSE);

	return TRUE;
}

void CMKWISetup1::OnHeaderLoopup() 
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

void CMKWISetup1::OnFooterLoopup() 
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

void CMKWISetup1::OnHeaderEdit() 
{
	ASSERT( AfxGetNCVCApp()->GetExecList()->GetCount() > 0 );
	UpdateData();
	AfxGetNCVCMainWnd()->CreateOutsideProcess(
		AfxGetNCVCApp()->GetExecList()->GetHead()->GetFileName(), "\""+m_strHeader+"\"");
	m_ctHeader.SetFocus();
}

void CMKWISetup1::OnFooterEdit() 
{
	ASSERT( AfxGetNCVCApp()->GetExecList()->GetCount() > 0 );
	UpdateData();
	AfxGetNCVCMainWnd()->CreateOutsideProcess(
		AfxGetNCVCApp()->GetExecList()->GetHead()->GetFileName(), "\""+m_strFooter+"\"");
	m_ctFooter.SetFocus();
}

BOOL CMKWISetup1::OnApply() 
{
	CNCMakeWireOpt* pOpt = static_cast<CMKWISetup *>(GetParent())->GetNCMakeOption();
	pOpt->m_dDepth		= m_dDepth;
	pOpt->m_dTaper		= m_dTaper;
	pOpt->m_dFeed		= m_dFeed;
	pOpt->m_dG92X		= m_dG92X;
	pOpt->m_dG92Y		= m_dG92Y;
	pOpt->m_strOption[MKWI_STR_TAPERMODE]	= m_strTaperMode;
	pOpt->m_strOption[MKWI_STR_HEADER]		= m_strHeader;
	pOpt->m_strOption[MKWI_STR_FOOTER]		= m_strFooter;

	return TRUE;
}

BOOL CMKWISetup1::OnKillActive() 
{
	if ( !CPropertyPage::OnKillActive() )
		return FALSE;

	if ( m_dDepth < 0 ) {
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_dDepth.SetFocus();
		m_dDepth.SetSel(0, -1);
		return FALSE;
	}
	if ( fabs(m_dTaper) > 45.0 ) {
		AfxMessageBox(IDS_ERR_NCBLK_OVER, MB_OK|MB_ICONEXCLAMATION);
		m_dTaper.SetFocus();
		m_dTaper.SetSel(0, -1);
		return FALSE;
	}
	if ( m_dFeed < 0 ) {
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_dFeed.SetFocus();
		m_dFeed.SetSel(0, -1);
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
