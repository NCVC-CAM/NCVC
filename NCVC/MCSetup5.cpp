// MCSetup5.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "MCOption.h"
#include "MCSetup.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CMCSetup5, CPropertyPage)
	ON_CBN_SELCHANGE(IDC_MCST5_VIEWMODE, &CMCSetup5::OnSelchangeViewMode)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMCSetup5 プロパティ ページ

CMCSetup5::CMCSetup5() : CPropertyPage(CMCSetup5::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;
	const CMCOption*	pMCopt = AfxGetNCVCApp()->GetMCOption();
	m_nForceViewMode	= pMCopt->m_nForceViewMode;
	m_bL0Cycle			= pMCopt->m_bL0Cycle;
	m_strAutoBreak		= pMCopt->m_strAutoBreak;
}

void CMCSetup5::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_CBIndex(pDX, IDC_MCST5_VIEWMODE, m_nForceViewMode);
	DDX_Control(pDX, IDC_MCST5_WIREDEPTH, m_dDefWireDepth);
	DDX_Check(pDX, IDC_MCST5_L0CYCLE, m_bL0Cycle);
	DDX_Text(pDX, IDC_MCST5_AUTOBREAK, m_strAutoBreak);
	DDX_Control(pDX, IDC_MCST5_LABEL1, m_ctDepthLabel1);
	DDX_Control(pDX, IDC_MCST5_LABEL2, m_ctDepthLabel2);
}

void CMCSetup5::EnableControl_ViewMode(void)
{
	if ( m_nForceViewMode == MC_VIEWMODE_WIRE ) {
		m_ctDepthLabel1.EnableWindow(TRUE);
		m_ctDepthLabel2.EnableWindow(TRUE);
		m_dDefWireDepth.EnableWindow(TRUE);
	}
	else {
		m_ctDepthLabel1.EnableWindow(FALSE);
		m_ctDepthLabel2.EnableWindow(FALSE);
		m_dDefWireDepth.EnableWindow(FALSE);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CMCSetup5 メッセージ ハンドラ

BOOL CMCSetup5::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	const CMCOption*	pMCopt = AfxGetNCVCApp()->GetMCOption();

	// ｺﾝｽﾄﾗｸﾀではできないｺﾝﾄﾛｰﾙの初期化
	m_dDefWireDepth = pMCopt->m_dDefWireDepth;

	EnableControl_ViewMode();

	return TRUE;
}

BOOL CMCSetup5::OnApply() 
{
	CMCSetup*	pParent = static_cast<CMCSetup *>(GetParent());
	CMCOption*	pMCopt = AfxGetNCVCApp()->GetMCOption();

	// 再読込ﾁｪｯｸ
	if ( pMCopt->m_nForceViewMode != m_nForceViewMode ) {
		pMCopt->m_nForceViewMode = m_nForceViewMode;
		pParent->m_bReload = TRUE;		// 再読込が必要
	}
	if ( pMCopt->m_dDefWireDepth != m_dDefWireDepth ) {
		pMCopt->m_dDefWireDepth = m_dDefWireDepth;
		pParent->m_bReload = TRUE;
	}
	if ( pMCopt->m_bL0Cycle != m_bL0Cycle ) {
		pMCopt->m_bL0Cycle = m_bL0Cycle;
		pParent->m_bReload = TRUE;
	}
	if ( pMCopt->m_strAutoBreak != m_strAutoBreak ) {
		pMCopt->m_strAutoBreak = m_strAutoBreak;
		pParent->m_bReload = TRUE;
	}

	return TRUE;
}

void CMCSetup5::OnSelchangeViewMode() 
{
	UpdateData();
	EnableControl_ViewMode();
}
