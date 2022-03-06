// MakeLatheSetup0.cpp : 実装ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "ExecOption.h"
#include "MainFrm.h"
#include "NCMakeLatheOpt.h"
#include "MakeLatheSetup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CMakeLatheSetup0, CPropertyPage)
	ON_BN_CLICKED(IDC_MKNC1_HEADER_LOOPUP, &CMakeLatheSetup0::OnHeaderLoopup)
	ON_BN_CLICKED(IDC_MKNC1_FOOTER_LOOPUP, &CMakeLatheSetup0::OnFooterLoopup)
	ON_BN_CLICKED(IDC_MKNC1_HEADER_EDIT, &CMakeLatheSetup0::OnHeaderEdit)
	ON_BN_CLICKED(IDC_MKNC1_FOOTER_EDIT, &CMakeLatheSetup0::OnFooterEdit)
END_MESSAGE_MAP()

#define	GetParentSheet()	static_cast<CMakeLatheSetup *>(GetParentSheet())

/////////////////////////////////////////////////////////////////////////////
// CMakeLatheSetup0 プロパティ ページ

CMakeLatheSetup0::CMakeLatheSetup0() : CPropertyPage(CMakeLatheSetup0::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;
}

void CMakeLatheSetup0::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MKNC1_FOOTER_EDIT, m_ctButton2);
	DDX_Control(pDX, IDC_MKNC1_HEADER_EDIT, m_ctButton1);
	DDX_Control(pDX, IDC_MKNC1_HEADER, m_ctHeader);
	DDX_Control(pDX, IDC_MKNC1_FOOTER, m_ctFooter);
	DDX_Text(pDX, IDC_MKNC1_FOOTER, m_strFooter);
	DDX_Text(pDX, IDC_MKNC1_HEADER, m_strHeader);
}

/////////////////////////////////////////////////////////////////////////////
// CMakeLatheSetup0 メッセージ ハンドラ

BOOL CMakeLatheSetup0::OnInitDialog() 
{
	__super::OnInitDialog();

	CNCMakeLatheOpt* pOpt = GetParentSheet()->GetNCMakeOption();
	::Path_Name_From_FullPath(pOpt->LTH_S_HEADER, m_strHeaderPath, m_strHeader);
	::Path_Name_From_FullPath(pOpt->LTH_S_FOOTER, m_strFooterPath, m_strFooter);
	// ﾊﾟｽ表示の最適化(shlwapi.h)
	::PathSetDlgItemPath(m_hWnd, IDC_MKNC1_HEADERPATH, m_strHeaderPath);
	::PathSetDlgItemPath(m_hWnd, IDC_MKNC1_FOOTERPATH, m_strFooterPath);
	// 編集ﾎﾞﾀﾝの有効無効
	if ( AfxGetNCVCApp()->GetExecList()->GetCount() < 1 ) {
		m_ctButton1.EnableWindow(FALSE);
		m_ctButton2.EnableWindow(FALSE);
	}

	UpdateData(FALSE);

	return TRUE;
}

BOOL CMakeLatheSetup0::OnApply() 
{
	CNCMakeLatheOpt* pOpt = GetParentSheet()->GetNCMakeOption();
	pOpt->LTH_S_HEADER	= m_strHeaderPath+m_strHeader;
	pOpt->LTH_S_FOOTER	= m_strFooterPath+m_strFooter;

	return TRUE;
}

BOOL CMakeLatheSetup0::OnKillActive() 
{
	if ( !__super::OnKillActive() )
		return FALSE;

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
	if ( !::IsFileExist(m_strHeaderPath+m_strHeader) ) {
		m_ctHeader.SetFocus();
		return FALSE;
	}
	if ( !::IsFileExist(m_strFooterPath+m_strFooter) ) {
		m_ctFooter.SetFocus();
		return FALSE;
	}

	return TRUE;
}

void CMakeLatheSetup0::OnHeaderLoopup() 
{
	UpdateData();
	if ( ::NCVC_FileDlgCommon(IDS_CUSTOMFILE, IDS_TXT_FILTER, TRUE, m_strHeader, m_strHeaderPath) == IDOK ) {
		// ﾃﾞｰﾀの反映
		::Path_Name_From_FullPath(m_strHeader, m_strHeaderPath, m_strHeader);
		::PathSetDlgItemPath(m_hWnd, IDC_MKNC1_HEADERPATH, m_strHeaderPath);
		UpdateData(FALSE);
		// 文字選択状態
		m_ctHeader.SetFocus();
		m_ctHeader.SetSel(0, -1);
	}
}

void CMakeLatheSetup0::OnFooterLoopup() 
{
	UpdateData();
	if ( ::NCVC_FileDlgCommon(IDS_CUSTOMFILE, IDS_TXT_FILTER, TRUE, m_strFooter, m_strFooterPath) == IDOK ) {
		// ﾃﾞｰﾀの反映
		::Path_Name_From_FullPath(m_strFooter, m_strFooterPath, m_strFooter);
		::PathSetDlgItemPath(m_hWnd, IDC_MKNC1_FOOTERPATH, m_strFooterPath);
		UpdateData(FALSE);
		// 文字選択状態
		m_ctFooter.SetFocus();
		m_ctFooter.SetSel(0, -1);
	}
}

void CMakeLatheSetup0::OnHeaderEdit() 
{
	ASSERT( AfxGetNCVCApp()->GetExecList()->GetCount() > 0 );
	UpdateData();
	AfxGetNCVCMainWnd()->CreateOutsideProcess(
		AfxGetNCVCApp()->GetExecList()->GetHead()->GetFileName(),
		"\""+m_strHeaderPath+m_strHeader+"\"");
	m_ctHeader.SetFocus();
}

void CMakeLatheSetup0::OnFooterEdit() 
{
	ASSERT( AfxGetNCVCApp()->GetExecList()->GetCount() > 0 );
	UpdateData();
	AfxGetNCVCMainWnd()->CreateOutsideProcess(
		AfxGetNCVCApp()->GetExecList()->GetHead()->GetFileName(),
		"\""+m_strFooterPath+m_strFooter+"\"");
	m_ctFooter.SetFocus();
}
