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
	ON_CBN_SELCHANGE(IDC_MKNC4_ZPROCESS, &CMKNCSetup4::OnSelchangeZProcess)
	ON_BN_CLICKED(IDC_MKNC4_CIRCLE, &CMKNCSetup4::OnCircle)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

#define	GetParentSheet()	static_cast<CMKNCSetup *>(GetParentSheet())

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
	__super::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMKNCSetup4)
	DDX_Control(pDX, IDC_MKNC4_FEED, m_dFeed);
	DDX_Control(pDX, IDC_MKNC4_CIRCLEGROUP, m_ctCircleGroup);
	DDX_Control(pDX, IDC_MKNC4_CIRCLEBREAK, m_ctCircleBreak);
	DDX_Control(pDX, IDC_MKNC4_CIRCLEPROCESS, m_ctCircleProcess);
	DDX_Control(pDX, IDC_MKNC4_DRILLQ, m_dDrillQ);
	DDX_Control(pDX, IDC_MKNC4_CIRCLE_R, m_dCircleR);
	DDX_Control(pDX, IDC_MKNC4_DWELLUNIT, m_ctDwellUnit);
	DDX_Control(pDX, IDC_MKNC4_ZCUT, m_dDrillZ);
	DDX_Control(pDX, IDC_MKNC4_R, m_dDrillR);
	DDX_Control(pDX, IDC_MKNC4_DWELL, m_nDwell);
	DDX_Control(pDX, IDC_MKNC4_SPINDLE, m_nSpindle);
	DDX_Check(pDX, IDC_MKNC4_MATCH, m_bDrillMatch);
	DDX_Check(pDX, IDC_MKNC4_CIRCLE, m_bCircle);
	DDX_Check(pDX, IDC_MKNC4_CIRCLEBREAK, m_bCircleBreak);
	DDX_CBIndex(pDX, IDC_MKNC4_PROCESS, m_nProcess);
	DDX_CBIndex(pDX, IDC_MKNC4_DWELLFORMAT, m_nDwellFormat);
	DDX_CBIndex(pDX, IDC_MKNC4_ZPROCESS, m_nDrillReturn);
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

void CMKNCSetup4::EnableControl_DrillQ(void)
{
	// G83以外入力不可
	m_dDrillQ.EnableWindow( m_nDrillReturn == 2 );
}

/////////////////////////////////////////////////////////////////////////////
// CMKNCSetup4 メッセージ ハンドラ

BOOL CMKNCSetup4::OnInitDialog() 
{
	__super::OnInitDialog();

	// ｶｽﾀﾑｺﾝﾄﾛｰﾙはｺﾝｽﾄﾗｸﾀで初期化できない
	// + GetParentSheet() ﾎﾟｲﾝﾀを取得できない
	CNCMakeMillOpt* pOpt = GetParentSheet()->GetNCMakeOption();
	m_nSpindle		= pOpt->MIL_I_DRILLSPINDLE;
	m_dFeed			= pOpt->MIL_D_DRILLFEED;
	m_bDrillMatch	= pOpt->MIL_F_DRILLMATCH;
	m_nDwell		= pOpt->MIL_I_DWELL;
	m_nDwellFormat	= pOpt->MIL_I_DWELLFORMAT;
	m_nProcess		= pOpt->MIL_I_DRILLPROCESS;
	m_nDrillReturn	= pOpt->MIL_I_DRILLRETURN;
	m_dDrillR		= pOpt->MIL_D_DRILLR;
	m_dDrillZ		= pOpt->MIL_D_DRILLZ;
	m_dDrillQ		= pOpt->MIL_D_DRILLQ;
	m_bCircle		= pOpt->MIL_F_DRILLCIRCLE;
	m_dCircleR		= pOpt->MIL_D_DRILLCIRCLE;
	m_nSort			= pOpt->MIL_I_DRILLSORT;
	m_bCircleBreak	= pOpt->MIL_F_DRILLBREAK;
	m_nCircleProcess= pOpt->MIL_I_DRILLCIRCLEPROCESS;
	EnableControl_Circle();
	EnableControl_Dwell();
	EnableControl_DrillQ();

	UpdateData(FALSE);

	return TRUE;  // コントロールにフォーカスを設定しないとき、戻り値は TRUE となります
	              // 例外: OCX プロパティ ページの戻り値は FALSE となります
}

void CMKNCSetup4::OnSelchangeDwellFormat() 
{
	UpdateData();
	EnableControl_Dwell();
}

void CMKNCSetup4::OnSelchangeZProcess()
{
	UpdateData();
	EnableControl_DrillQ();
}

void CMKNCSetup4::OnCircle() 
{
	UpdateData();
	EnableControl_Circle();
}

BOOL CMKNCSetup4::OnApply() 
{
	CNCMakeMillOpt* pOpt = GetParentSheet()->GetNCMakeOption();
	pOpt->MIL_I_DRILLSPINDLE	= m_nSpindle;
	pOpt->MIL_D_DRILLFEED		= m_dFeed;
	pOpt->MIL_F_DRILLMATCH		= m_bDrillMatch;
	pOpt->MIL_I_DWELL			= m_nDwell;
	pOpt->MIL_I_DWELLFORMAT		= m_nDwellFormat;
	pOpt->MIL_I_DRILLPROCESS	= m_nProcess;
	pOpt->MIL_I_DRILLRETURN		= m_nDrillReturn;
	pOpt->MIL_D_DRILLR			= m_dDrillR;
	pOpt->MIL_D_DRILLZ			= m_dDrillZ;
	pOpt->MIL_D_DRILLQ			= m_dDrillQ;		
	pOpt->MIL_F_DRILLCIRCLE		= m_bCircle;
	pOpt->MIL_D_DRILLCIRCLE		= m_dCircleR;
	pOpt->MIL_I_DRILLSORT		= m_nSort;
	pOpt->MIL_F_DRILLBREAK		= m_bCircleBreak;
	pOpt->MIL_I_DRILLCIRCLEPROCESS = m_nCircleProcess;

	return TRUE;
}

BOOL CMKNCSetup4::OnKillActive() 
{
	if ( !__super::OnKillActive() )
		return FALSE;

	if ( (float)m_dDrillZ > (float)m_dDrillR ) {
		AfxMessageBox(IDS_ERR_ZCUT, MB_OK|MB_ICONEXCLAMATION);
		m_dDrillZ.SetFocus();
		m_dDrillZ.SetSel(0, -1);
		return FALSE;
	}
	if ( m_dDrillQ <= 0 ) {
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_dDrillQ.SetFocus();
		m_dDrillQ.SetSel(0, -1);
		return FALSE;
	}
	if ( m_dFeed <= 0 ) {
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_dFeed.SetFocus();
		m_dFeed.SetSel(0, -1);
		return FALSE;
	}
	if ( m_bCircle && m_dCircleR<=0.0f ) {
		AfxMessageBox(IDS_ERR_SETTING, MB_OK|MB_ICONEXCLAMATION);
		m_dCircleR.SetFocus();
		m_dCircleR.SetSel(0, -1);
		return FALSE;
	}
	return TRUE;
}
