// MKNCSetup2.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "resource.h"
#include "NCMakeMillOpt.h"
#include "NCMakeLatheOpt.h"
#include "NCMakeWireOpt.h"
#include "MKNCSetup.h"
#include "MKLASetup.h"
#include "MKWISetup.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CMKNCSetup2, CPropertyPage)
	//{{AFX_MSG_MAP(CMKNCSetup2)
	ON_BN_CLICKED(IDC_MKNC2_PROG, &CMKNCSetup2::OnProgNo)
	ON_BN_CLICKED(IDC_MKNC2_PROGAUTO, &CMKNCSetup2::OnProgNo)
	ON_BN_CLICKED(IDC_MKNC2_LINE, &CMKNCSetup2::OnLineAdd)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMKNCSetup2 プロパティ ページ

CMKNCSetup2::CMKNCSetup2() : CPropertyPage(CMKNCSetup2::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;
	//{{AFX_DATA_INIT(CMKNCSetup2)
	m_bProg				= FALSE;
	m_bProgAuto			= FALSE;
	m_bLineAdd			= FALSE;
	m_nLineAdd			= -1;
	m_bGclip			= FALSE;
	m_bDisableSpindle	= FALSE;
	m_nG90				= -1;
	m_nZReturn			= -1;
	//}}AFX_DATA_INIT
}

void CMKNCSetup2::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMKNCSetup2)
	DDX_Control(pDX, IDC_MKNC2_PROGAUTO, m_ctProgAuto);
	DDX_Control(pDX, IDC_MKNC2_PROGNO, m_nProg);
	DDX_Control(pDX, IDC_MKNC2_ZRETURN_S, m_ctZReturnS);
	DDX_Control(pDX, IDC_MKNC2_ZRETURN, m_ctZReturn);
	DDX_Control(pDX, IDC_MKNC2_LINEADD, m_ctLineAdd);
	DDX_Control(pDX, IDC_MKNC2_LINEFORMAT, m_ctLineForm);
	DDX_Control(pDX, IDC_MKNC2_SPINDLE, m_ctDisableSpindle);
	DDX_CBIndex(pDX, IDC_MKNC2_LINEADD, m_nLineAdd);
	DDX_CBIndex(pDX, IDC_MKNC2_G90, m_nG90);
	DDX_CBIndex(pDX, IDC_MKNC2_ZRETURN, m_nZReturn);
	DDX_Check(pDX, IDC_MKNC2_PROG, m_bProg);
	DDX_Check(pDX, IDC_MKNC2_PROGAUTO, m_bProgAuto);
	DDX_Check(pDX, IDC_MKNC2_LINE, m_bLineAdd);
	DDX_Check(pDX, IDC_MKNC2_GCODE, m_bGclip);
	DDX_Check(pDX, IDC_MKNC2_SPINDLE, m_bDisableSpindle);
	DDX_Text(pDX, IDC_MKNC2_LINEFORMAT, m_strLineForm);
	DDX_Text(pDX, IDC_MKNC2_EOB, m_strEOB);
	//}}AFX_DATA_MAP
}

void CMKNCSetup2::EnableControl_ProgNo(void)
{
	m_nProg.EnableWindow(m_bProg && !m_bProgAuto);
	m_ctProgAuto.EnableWindow(m_bProg);
}

void CMKNCSetup2::EnableControl_LineAdd(void)
{
	m_ctLineForm.EnableWindow(m_bLineAdd);
	m_ctLineAdd.EnableWindow(m_bLineAdd);
}

/////////////////////////////////////////////////////////////////////////////
// CMKNCSetup2 メッセージ ハンドラ

BOOL CMKNCSetup2::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	// ｶｽﾀﾑｺﾝﾄﾛｰﾙはｺﾝｽﾄﾗｸﾀで初期化できない
	// + GetParent() ﾎﾟｲﾝﾀを取得できない
	CWnd*	pParent = GetParent();
	if ( pParent->IsKindOf(RUNTIME_CLASS(CMKNCSetup)) ) {
		CNCMakeMillOpt* pOpt = static_cast<CMKNCSetup *>(pParent)->GetNCMakeOption();
		m_bProg				= pOpt->m_bProg;
		m_nProg				= pOpt->m_nProg;
		m_bProgAuto			= pOpt->m_bProgAuto;
		m_bLineAdd			= pOpt->m_bLineAdd;
		m_strLineForm		= pOpt->m_strOption[MKNC_STR_LINEFORM];
		m_nLineAdd			= pOpt->m_nLineAdd;
		m_strEOB			= pOpt->m_strOption[MKNC_STR_EOB];
		m_nG90				= pOpt->m_nG90;
		m_nZReturn			= pOpt->m_nZReturn;
		m_bGclip			= pOpt->m_bGclip;
		m_bDisableSpindle	= pOpt->m_bDisableSpindle;
	}
	else if ( pParent->IsKindOf(RUNTIME_CLASS(CMKLASetup)) ) {
		// 旋盤ﾓｰﾄﾞ
		m_ctZReturnS.ShowWindow(SW_HIDE);
		m_ctZReturn.ShowWindow(SW_HIDE);
		CNCMakeLatheOpt* pOpt = static_cast<CMKLASetup *>(GetParent())->GetNCMakeOption();
		m_bProg				= pOpt->m_bProg;
		m_nProg				= pOpt->m_nProg;
		m_bProgAuto			= pOpt->m_bProgAuto;
		m_bLineAdd			= pOpt->m_bLineAdd;
		m_strLineForm		= pOpt->m_strOption[MKLA_STR_LINEFORM];
		m_nLineAdd			= pOpt->m_nLineAdd;
		m_strEOB			= pOpt->m_strOption[MKLA_STR_EOB];
		m_nG90				= pOpt->m_nG90;
		m_bGclip			= pOpt->m_bGclip;
		m_bDisableSpindle	= pOpt->m_bDisableSpindle;
	}
	else {
		// ﾜｲﾔ放電加工機ﾓｰﾄﾞ
		m_ctZReturnS.ShowWindow(SW_HIDE);
		m_ctZReturn.ShowWindow(SW_HIDE);
		m_ctDisableSpindle.ShowWindow(SW_HIDE);
		CNCMakeWireOpt* pOpt = static_cast<CMKWISetup *>(GetParent())->GetNCMakeOption();
		m_bProg				= pOpt->m_bProg;
		m_nProg				= pOpt->m_nProg;
		m_bProgAuto			= pOpt->m_bProgAuto;
		m_bLineAdd			= pOpt->m_bLineAdd;
		m_strLineForm		= pOpt->m_strOption[MKLA_STR_LINEFORM];
		m_nLineAdd			= pOpt->m_nLineAdd;
		m_strEOB			= pOpt->m_strOption[MKLA_STR_EOB];
		m_nG90				= pOpt->m_nG90;
		m_bGclip			= pOpt->m_bGclip;
	}
	EnableControl_ProgNo();
	EnableControl_LineAdd();

	UpdateData(FALSE);

	return TRUE;  // コントロールにフォーカスを設定しないとき、戻り値は TRUE となります
	              // 例外: OCX プロパティ ページの戻り値は FALSE となります
}

void CMKNCSetup2::OnProgNo() 
{
	UpdateData();
	EnableControl_ProgNo();
}

void CMKNCSetup2::OnLineAdd() 
{
	UpdateData();
	EnableControl_LineAdd();
}

BOOL CMKNCSetup2::OnApply() 
{
	CWnd*	pParent = GetParent();
	if ( pParent->IsKindOf(RUNTIME_CLASS(CMKNCSetup)) ) {
		CNCMakeMillOpt* pOpt = static_cast<CMKNCSetup *>(pParent)->GetNCMakeOption();
		pOpt->m_bProg			= m_bProg;
		pOpt->m_nProg			= m_nProg;
		pOpt->m_bProgAuto		= m_bProgAuto;
		pOpt->m_bLineAdd		= m_bLineAdd;
		pOpt->m_nLineAdd		= m_nLineAdd;
		pOpt->m_nG90			= m_nG90;
		pOpt->m_nZReturn		= m_nZReturn;
		pOpt->m_bGclip			= m_bGclip;
		pOpt->m_bDisableSpindle	= m_bDisableSpindle;
		pOpt->m_strOption[MKNC_STR_LINEFORM] = m_strLineForm;
		pOpt->m_strOption[MKNC_STR_EOB] = m_strEOB;
	}
	else if ( pParent->IsKindOf(RUNTIME_CLASS(CMKLASetup)) ) {
		CNCMakeLatheOpt* pOpt = static_cast<CMKLASetup *>(GetParent())->GetNCMakeOption();
		pOpt->m_bProg			= m_bProg;
		pOpt->m_nProg			= m_nProg;
		pOpt->m_bProgAuto		= m_bProgAuto;
		pOpt->m_bLineAdd		= m_bLineAdd;
		pOpt->m_nLineAdd		= m_nLineAdd;
		pOpt->m_nG90			= m_nG90;
		pOpt->m_bGclip			= m_bGclip;
		pOpt->m_bDisableSpindle	= m_bDisableSpindle;
		pOpt->m_strOption[MKLA_STR_LINEFORM] = m_strLineForm;
		pOpt->m_strOption[MKLA_STR_EOB] = m_strEOB;
	}
	else {
		CNCMakeWireOpt* pOpt = static_cast<CMKWISetup *>(GetParent())->GetNCMakeOption();
		pOpt->m_bProg			= m_bProg;
		pOpt->m_nProg			= m_nProg;
		pOpt->m_bProgAuto		= m_bProgAuto;
		pOpt->m_bLineAdd		= m_bLineAdd;
		pOpt->m_nLineAdd		= m_nLineAdd;
		pOpt->m_nG90			= m_nG90;
		pOpt->m_bGclip			= m_bGclip;
		pOpt->m_strOption[MKLA_STR_LINEFORM] = m_strLineForm;
		pOpt->m_strOption[MKLA_STR_EOB] = m_strEOB;
	}

	return TRUE;
}

BOOL CMKNCSetup2::OnKillActive() 
{
	if ( !CPropertyPage::OnKillActive() )
		return FALSE;

	if ( m_bProg && (int)m_nProg <= 0 ) {
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_nProg.SetFocus();
		return FALSE;
	}

	if ( m_bLineAdd && m_strLineForm.IsEmpty() ) {
		AfxMessageBox(IDS_ERR_SETTING, MB_OK|MB_ICONEXCLAMATION);
		m_ctLineForm.SetFocus();
		return FALSE;
	}

	return TRUE;
}
