// MKNCSetup5.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "NCMakeMillOpt.h"
#include "MKNCSetup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CMKNCSetup5, CPropertyPage)
	//{{AFX_MSG_MAP(CMKNCSetup5)
	ON_CBN_SELCHANGE(IDC_MKNC5_DRILL, &CMKNCSetup5::OnSelchangeDrill)
	ON_BN_CLICKED(IDC_MKNC5_SCRIPT_LOOKUP, &CMKNCSetup5::OnScriptLookup)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

#define	GetParentSheet()	static_cast<CMKNCSetup *>(GetParentSheet())

/////////////////////////////////////////////////////////////////////////////
// CMKNCSetup5 プロパティ ページ

CMKNCSetup5::CMKNCSetup5() : CPropertyPage(CMKNCSetup5::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;
	//{{AFX_DATA_INIT(CMKNCSetup5)
	m_nOptimaizeDrill	= 0;
	m_nTolerance		= -1;
	//}}AFX_DATA_INIT
}

void CMKNCSetup5::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMKNCSetup5)
	DDX_Control(pDX, IDC_MKNC5_TOLERANCE, m_dTolerance);
	DDX_Control(pDX, IDC_MKNC5_DRILLMARGIN, m_dDrillMargin);
	DDX_Control(pDX, IDC_MKNC5_ZAPPROACH, m_dZApproach);
	DDX_Control(pDX, IDC_MKNC5_SCRIPT, m_ctScript);
	DDX_CBIndex(pDX, IDC_MKNC5_DRILL, m_nOptimaizeDrill);
	DDX_CBIndex(pDX, IDC_MKNC5_TOLERANCE_P, m_nTolerance);
	DDX_Text(pDX, IDC_MKNC5_SCRIPT, m_strScript);
	//}}AFX_DATA_MAP
}

void CMKNCSetup5::EnableControl_Drill(void)
{
	m_dDrillMargin.EnableWindow(m_nOptimaizeDrill==0 ? FALSE : TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// CMKNCSetup5 メッセージ ハンドラ

BOOL CMKNCSetup5::OnInitDialog() 
{
	__super::OnInitDialog();
	
	// ｶｽﾀﾑｺﾝﾄﾛｰﾙはｺﾝｽﾄﾗｸﾀで初期化できない
	// + GetParentSheet() ﾎﾟｲﾝﾀを取得できない
	CNCMakeMillOpt* pOpt = GetParentSheet()->GetNCMakeOption();
	m_dTolerance		= pOpt->MIL_D_TOLERANCE;
	m_nTolerance		= pOpt->MIL_I_TOLERANCE;
	m_nOptimaizeDrill	= pOpt->MIL_I_OPTIMAIZEDRILL;
	m_dDrillMargin		= pOpt->MIL_D_DRILLMARGIN;
	m_dZApproach		= pOpt->MIL_D_ZAPPROACH;
	::Path_Name_From_FullPath(pOpt->MIL_S_PERLSCRIPT, m_strScriptPath, m_strScript);
	// ﾊﾟｽ表示の最適化(shlwapi.h)
	::PathSetDlgItemPath(m_hWnd, IDC_MKNC5_SCRIPTPATH, m_strScriptPath);
	//
	EnableControl_Drill();

	UpdateData(FALSE);

	return TRUE;  // コントロールにフォーカスを設定しないとき、戻り値は TRUE となります
	              // 例外: OCX プロパティ ページの戻り値は FALSE となります
}

void CMKNCSetup5::OnSelchangeDrill() 
{
	UpdateData();
	EnableControl_Drill();
}

void CMKNCSetup5::OnScriptLookup() 
{
	UpdateData();
	if ( ::NCVC_FileDlgCommon(IDS_CUSTOMFILE, IDS_PERL_FILTER, TRUE, m_strScript, m_strScriptPath) == IDOK ) {
		// ﾃﾞｰﾀの反映
		::Path_Name_From_FullPath(m_strScript, m_strScriptPath, m_strScript);
		::PathSetDlgItemPath(m_hWnd, IDC_MKNC5_SCRIPTPATH, m_strScriptPath);
		UpdateData(FALSE);
		// 文字選択状態
		m_ctScript.SetFocus();
		m_ctScript.SetSel(0, -1);
	}
}

BOOL CMKNCSetup5::OnApply() 
{
	CNCMakeMillOpt* pOpt = GetParentSheet()->GetNCMakeOption();
	pOpt->MIL_D_TOLERANCE		= fabs((float)m_dTolerance);
	if ( pOpt->MIL_D_TOLERANCE < NCMIN )
		pOpt->MIL_D_TOLERANCE = NCMIN;
	pOpt->MIL_I_TOLERANCE		= m_nTolerance;
	pOpt->MIL_I_OPTIMAIZEDRILL	= m_nOptimaizeDrill;
	pOpt->MIL_D_DRILLMARGIN		= fabs((float)m_dDrillMargin);
	pOpt->MIL_S_PERLSCRIPT		= m_strScriptPath+m_strScript;
	pOpt->MIL_D_ZAPPROACH		= m_dZApproach;

	return TRUE;
}

BOOL CMKNCSetup5::OnKillActive() 
{
	if ( !__super::OnKillActive() )
		return FALSE;

	if ( !m_strScript.IsEmpty() && !::IsFileExist(m_strScriptPath+m_strScript) ) {
		m_ctScript.SetFocus();
		return FALSE;
	}
	if ( m_dZApproach < 0 ) {
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_dZApproach.SetFocus();
		m_dZApproach.SetSel(0, -1);
		return FALSE;
	}

	return TRUE;
}
