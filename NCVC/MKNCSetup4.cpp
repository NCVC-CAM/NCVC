// MKNCSetup4.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "resource.h"
#include "NCMakeMillOpt.h"
#include "MKNCSetup.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CMKNCSetup4, CPropertyPage)
	//{{AFX_MSG_MAP(CMKNCSetup4)
	ON_CBN_SELCHANGE(IDC_MKNC4_DWELLFORMAT, &CMKNCSetup4::OnSelchangeDwellFormat)
	ON_BN_CLICKED(IDC_MKNC4_CIRCLE, &CMKNCSetup4::OnCircle)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMKNCSetup4 プロパティ ページ

CMKNCSetup4::CMKNCSetup4() : CPropertyPage(CMKNCSetup4::IDD),
	m_dFeed(TRUE)	// CFloatEditﾌﾗｸﾞ設定
{
	m_psp.dwFlags &= ~PSP_HASHELP;
	//{{AFX_DATA_INIT(CMKNCSetup4)
	m_nProcess			= 0;
	m_bDrillMatch		= FALSE;
	m_nDwellFormat		= -1;
	m_nDrillReturn		= -1;
	m_bCircle			= FALSE;
	m_bCircleBreak		= FALSE;
	m_nSort				= -1;
	m_nCircleProcess	= -1;
	//}}AFX_DATA_INIT
}

void CMKNCSetup4::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMKNCSetup4)
	DDX_Control(pDX, IDC_MKNC4_FEED, m_dFeed);
	DDX_Control(pDX, IDC_MKNC4_CIRCLEGROUP, m_ctCircleGroup);
	DDX_Control(pDX, IDC_MKNC4_CIRCLEBREAK, m_ctCircleBreak);
	DDX_Control(pDX, IDC_MKNC4_CIRCLEPROCESS, m_ctCircleProcess);
	DDX_Control(pDX, IDC_MKNC4_CIRCLE_R, m_dCircleR);
	DDX_Control(pDX, IDC_MKNC4_DWELLUNIT, m_ctDwellUnit);
	DDX_Control(pDX, IDC_MKNC4_ZCUT, m_dDrillZ);
	DDX_Control(pDX, IDC_MKNC4_R, m_dDrillR);
	DDX_Control(pDX, IDC_MKNC4_DWELL, m_nDwell);
	DDX_Control(pDX, IDC_MKNC4_SPINDLE, m_nSpindle);
	DDX_CBIndex(pDX, IDC_MKNC4_PROCESS, m_nProcess);
	DDX_Check(pDX, IDC_MKNC4_MATCH, m_bDrillMatch);
	DDX_CBIndex(pDX, IDC_MKNC4_DWELLFORMAT, m_nDwellFormat);
	DDX_CBIndex(pDX, IDC_MKNC4_ZPROCESS, m_nDrillReturn);
	DDX_Check(pDX, IDC_MKNC4_CIRCLE, m_bCircle);
	DDX_Check(pDX, IDC_MKNC4_CIRCLEBREAK, m_bCircleBreak);
	DDX_CBIndex(pDX, IDC_MKNC4_CIRCLEGROUP, m_nSort);
	DDX_CBIndex(pDX, IDC_MKNC4_CIRCLEPROCESS, m_nCircleProcess);
	//}}AFX_DATA_MAP
}

void CMKNCSetup4::EnableControl_Dwell(void)
{
	static	LPCTSTR	g_szDwellUnit[] = {
		"sec", "sec/msec"
	};

	if ( m_nDwellFormat<0 || m_nDwellFormat>SIZEOF(g_szDwellUnit) )
		m_nDwellFormat = 0;
	m_ctDwellUnit.SetWindowText(g_szDwellUnit[m_nDwellFormat]);
}

void CMKNCSetup4::EnableControl_Circle(void)
{
	m_ctCircleProcess.EnableWindow(m_bCircle);
	m_dCircleR.EnableWindow(m_bCircle);
	m_ctCircleBreak.EnableWindow(m_bCircle);
	m_ctCircleGroup.EnableWindow(m_bCircle);
}

/////////////////////////////////////////////////////////////////////////////
// CMKNCSetup4 メッセージ ハンドラ

BOOL CMKNCSetup4::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	// ｶｽﾀﾑｺﾝﾄﾛｰﾙはｺﾝｽﾄﾗｸﾀで初期化できない
	// + GetParent() ﾎﾟｲﾝﾀを取得できない
	CNCMakeMillOpt* pOpt = static_cast<CMKNCSetup *>(GetParent())->GetNCMakeOption();
	m_nSpindle		= pOpt->m_nDrillSpindle;
	m_dFeed			= pOpt->m_dDrillFeed;
	m_nDwell		= pOpt->m_nDwell;
	m_dDrillR		= pOpt->m_dDrillR;
	m_dDrillZ		= pOpt->m_dDrillZ;
	m_bDrillMatch	= pOpt->m_bDrillMatch;
	m_nDwellFormat	= pOpt->m_nDwellFormat;
	m_nProcess		= pOpt->m_nDrillProcess;
	m_nDrillReturn	= pOpt->m_nDrillReturn;
	//
	m_dCircleR		= pOpt->m_dDrillCircle;
	m_bCircle		= pOpt->m_bDrillCircle;
	m_nSort			= pOpt->m_nDrillSort;
	m_bCircleBreak	= pOpt->m_bDrillBreak;
	m_nCircleProcess= pOpt->m_nDrillCircleProcess;
	EnableControl_Circle();
	EnableControl_Dwell();

	UpdateData(FALSE);

	return TRUE;  // コントロールにフォーカスを設定しないとき、戻り値は TRUE となります
	              // 例外: OCX プロパティ ページの戻り値は FALSE となります
}

void CMKNCSetup4::OnSelchangeDwellFormat() 
{
	UpdateData();
	EnableControl_Dwell();
}

void CMKNCSetup4::OnCircle() 
{
	UpdateData();
	EnableControl_Circle();
}

BOOL CMKNCSetup4::OnApply() 
{
	CNCMakeMillOpt* pOpt = static_cast<CMKNCSetup *>(GetParent())->GetNCMakeOption();
	pOpt->m_nDrillSpindle	= m_nSpindle;
	pOpt->m_dDrillFeed		= m_dFeed;
	pOpt->m_bDrillMatch		= m_bDrillMatch;
	pOpt->m_nDwell			= m_nDwell;
	pOpt->m_nDwellFormat	= m_nDwellFormat;
	pOpt->m_nDrillProcess	= m_nProcess;
	pOpt->m_nDrillReturn	= m_nDrillReturn;
	pOpt->m_dDrillR			= m_dDrillR;
	pOpt->m_dDrillZ			= m_dDrillZ;
	//
	pOpt->m_bDrillCircle	= m_bCircle;
	pOpt->m_dDrillCircle	= m_dCircleR;
	pOpt->m_nDrillSort		= m_nSort;
	pOpt->m_bDrillBreak		= m_bCircleBreak;
	pOpt->m_nDrillCircleProcess = m_nCircleProcess;

	return TRUE;
}

BOOL CMKNCSetup4::OnKillActive() 
{
	if ( !CPropertyPage::OnKillActive() )
		return FALSE;

	if ( (double)m_dDrillZ > (double)m_dDrillR ) {
		AfxMessageBox(IDS_ERR_ZCUT, MB_OK|MB_ICONEXCLAMATION);
		m_dDrillZ.SetFocus();
		m_dDrillZ.SetSel(0, -1);
		return FALSE;
	}
	if ( m_dFeed <= 0 ) {
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_dFeed.SetFocus();
		m_dFeed.SetSel(0, -1);
		return FALSE;
	}
	if ( m_bCircle && m_dCircleR<=0.0 ) {
		AfxMessageBox(IDS_ERR_SETTING, MB_OK|MB_ICONEXCLAMATION);
		m_dCircleR.SetFocus();
		m_dCircleR.SetSel(0, -1);
		return FALSE;
	}
	return TRUE;
}
