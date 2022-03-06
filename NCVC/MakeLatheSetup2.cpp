// MakeLatheSetup2.cpp : 実装ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "NCMakeLatheOpt.h"
#include "MakeLatheSetup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CMakeLatheSetup2, CPropertyPage)
	ON_BN_CLICKED(IDC_MKLA2_CYCLE, &CMakeLatheSetup2::OnCycle)
END_MESSAGE_MAP()

#define	GetParentSheet()	static_cast<CMakeLatheSetup *>(GetParentSheet())

/////////////////////////////////////////////////////////////////////////////
// CMakeLatheSetup2 プロパティ ページ

CMakeLatheSetup2::CMakeLatheSetup2() : CPropertyPage(CMakeLatheSetup2::IDD)
{
	m_bCycle	= FALSE;
	m_psp.dwFlags &= ~PSP_HASHELP;
}

void CMakeLatheSetup2::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_MKLA2_DRILL, m_strDrill);
	DDX_Text(pDX, IDC_MKLA2_SPINDLE, m_strSpindle);
	DDX_Text(pDX, IDC_MKLA2_FEED, m_strFeed);
	DDX_Text(pDX, IDC_MKLA2_CUSTOMCODE, m_strCustom);
	DDX_Check(pDX, IDC_MKLA2_CYCLE, m_bCycle);
	DDX_Control(pDX, IDC_MKNC4_ZCUT, m_dDrillZ);
	DDX_Control(pDX, IDC_MKNC4_R, m_dDrillR);
	DDX_Control(pDX, IDC_MKNC4_DRILLQ, m_dDrillQ);
	DDX_Control(pDX, IDC_MKLA2_D, m_dDrillD);
	DDX_Control(pDX, IDC_MKNC4_DWELL, m_nDwell);
	DDX_Control(pDX, IDC_MKLA2_HOLE, m_dHole);
	DDX_Control(pDX, IDC_MKLA2_DTEXT, m_ctDrillDtext);
}

void CMakeLatheSetup2::EnableControl_Cycle(void)
{
	m_dDrillD.EnableWindow(!m_bCycle);
	m_ctDrillDtext.ShowWindow(m_bCycle);
}

/////////////////////////////////////////////////////////////////////////////
// CMakeLatheSetup2 メッセージ ハンドラー

BOOL CMakeLatheSetup2::OnInitDialog()
{
	__super::OnInitDialog();

	CNCMakeLatheOpt* pOpt = GetParentSheet()->GetNCMakeOption();
	m_strDrill		= pOpt->LTH_S_DRILL;
	m_strSpindle	= pOpt->LTH_S_DRILLSPINDLE;
	m_strFeed		= pOpt->LTH_S_DRILLFEED;
	m_strCustom		= pOpt->LTH_S_D_CUSTOM;
	m_bCycle		= pOpt->LTH_F_CYCLE;
	m_dDrillZ		= pOpt->LTH_D_DRILLZ;
	m_dDrillR		= pOpt->LTH_D_DRILLR;
	m_dDrillQ		= pOpt->LTH_D_DRILLQ;
	m_dDrillD		= pOpt->LTH_D_DRILLD;
	m_dHole			= pOpt->LTH_D_HOLE;
	m_nDwell		= (int)(pOpt->LTH_D_DWELL);
	EnableControl_Cycle();

	UpdateData(FALSE);

	return TRUE;
}

BOOL CMakeLatheSetup2::OnApply()
{
	CNCMakeLatheOpt* pOpt = GetParentSheet()->GetNCMakeOption();
	pOpt->LTH_S_DRILL		= m_strDrill;
	pOpt->LTH_S_DRILLSPINDLE= m_strSpindle;
	pOpt->LTH_S_DRILLFEED	= m_strFeed;
	pOpt->LTH_S_D_CUSTOM	= m_strCustom;
	pOpt->LTH_F_CYCLE		= m_bCycle;
	pOpt->LTH_D_DRILLZ		= m_dDrillZ;
	pOpt->LTH_D_DRILLR		= m_dDrillR;
	pOpt->LTH_D_DRILLQ		= m_dDrillQ;
	pOpt->LTH_D_DRILLD		= m_dDrillD;
	pOpt->LTH_D_HOLE		= m_dHole;
	pOpt->LTH_D_DWELL		= (float)m_nDwell;

	return TRUE;
}

BOOL CMakeLatheSetup2::OnKillActive()
{
	if ( !__super::OnKillActive() )
		return FALSE;

	if ( !m_strDrill.IsEmpty() ) {
		// 下穴ﾄﾞﾘﾙ設定があるときだけ
		if ( m_dDrillZ > 0 ) {
			AfxMessageBox(IDS_ERR_POSITIVE, MB_OK|MB_ICONEXCLAMATION);
			m_dDrillZ.SetFocus();
			m_dDrillZ.SetSel(0, -1);
			return FALSE;
		}
		if ( m_dDrillR < 0 ) {
			AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
			m_dDrillR.SetFocus();
			m_dDrillR.SetSel(0, -1);
			return FALSE;
		}
		if ( m_dDrillQ < 0 ) {
			AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
			m_dDrillQ.SetFocus();
			m_dDrillQ.SetSel(0, -1);
			return FALSE;
		}
		if ( m_dDrillD < 0 ) {
			AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
			m_dDrillD.SetFocus();
			m_dDrillD.SetSel(0, -1);
			return FALSE;
		}
	}
	if ( m_dHole < 0 ) {
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_dHole.SetFocus();
		m_dHole.SetSel(0, -1);
		return FALSE;
	}

	return TRUE;
}

void CMakeLatheSetup2::OnCycle()
{
	UpdateData();
	EnableControl_Cycle();
}
