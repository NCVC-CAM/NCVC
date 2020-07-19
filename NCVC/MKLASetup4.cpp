// MKLASetup4.cpp : 実装ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "NCMakeLatheOpt.h"
#include "MKLASetup.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CMKLASetup4, CPropertyPage)
	ON_BN_CLICKED(IDC_MKLA4_ENDFACE, &CMKLASetup4::OnCheck)
END_MESSAGE_MAP()

#define	GetParentSheet()	static_cast<CMKLASetup *>(GetParentSheet())

/////////////////////////////////////////////////////////////////////////////
// CMKLASetup4 プロパティ ページ

CMKLASetup4::CMKLASetup4() : CPropertyPage(CMKLASetup4::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;
	m_bEndFace = FALSE;
}

void CMKLASetup4::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_MKLA4_ENDFACE, m_bEndFace);
	DDX_Text(pDX, IDC_MKLA2_CUSTOMCODE, m_strCustom);
	DDX_Control(pDX, IDC_MKLA2_CUSTOMCODE, m_ctCustom);
	DDX_Control(pDX, IDC_MKNC1_SPINDLE, m_nSpindle);
	DDX_Control(pDX, IDC_MKLA1_XFEED, m_dXFeed);
	DDX_Control(pDX, IDC_MKNC3_DEEPFINAL, m_dCut);
	DDX_Control(pDX, IDC_MKNC3_ZSTEP, m_dStep);
	DDX_Control(pDX, IDC_MKLA1_PULL_Z, m_dPullZ);
	DDX_Control(pDX, IDC_MKLA1_PULL_X, m_dPullX);
}

void CMKLASetup4::EnableControl(void)
{
	m_ctCustom.EnableWindow(m_bEndFace);
	m_nSpindle.EnableWindow(m_bEndFace);
	m_dXFeed.EnableWindow(m_bEndFace);
	m_dCut.EnableWindow(m_bEndFace);
	m_dStep.EnableWindow(m_bEndFace);
	m_dPullZ.EnableWindow(m_bEndFace);
	m_dPullX.EnableWindow(m_bEndFace);
}

/////////////////////////////////////////////////////////////////////////////
// CMKLASetup4 メッセージ ハンドラ

BOOL CMKLASetup4::OnInitDialog() 
{
	__super::OnInitDialog();

	CNCMakeLatheOpt* pOpt = GetParentSheet()->GetNCMakeOption();
	m_bEndFace	= pOpt->LTH_F_ENDFACE;
	m_strCustom	= pOpt->LTH_S_E_CUSTOM;
	m_nSpindle	= pOpt->LTH_I_E_SPINDLE;
	m_dXFeed	= pOpt->LTH_D_E_FEED;
	m_dCut		= pOpt->LTH_D_E_CUT;
	m_dStep		= pOpt->LTH_D_E_STEP;
	m_dPullZ	= pOpt->LTH_D_E_PULLZ;
	m_dPullX	= pOpt->LTH_D_E_PULLX * 2.0f;		// 直径値に変換
	EnableControl();

	UpdateData(FALSE);

	return TRUE;
}

BOOL CMKLASetup4::OnApply() 
{
	CNCMakeLatheOpt* pOpt = GetParentSheet()->GetNCMakeOption();
	pOpt->LTH_F_ENDFACE		= m_bEndFace;
	pOpt->LTH_S_E_CUSTOM	= m_strCustom;
	pOpt->LTH_I_E_SPINDLE	= m_nSpindle;
	pOpt->LTH_D_E_FEED		= m_dXFeed;
	pOpt->LTH_D_E_CUT		= m_dCut;
	pOpt->LTH_D_E_STEP		= m_dStep;
	pOpt->LTH_D_E_PULLZ		= m_dPullZ;
	pOpt->LTH_D_E_PULLX		= m_dPullX / 2.0f;	// 半径値に変換

	return TRUE;
}

BOOL CMKLASetup4::OnKillActive() 
{
	if ( !__super::OnKillActive() )
		return FALSE;

	if ( !m_bEndFace )
		return TRUE;

	if ( m_dXFeed <= 0 ) {
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_dXFeed.SetFocus();
		m_dXFeed.SetSel(0, -1);
		return FALSE;
	}
	if ( m_dCut > 0 ) {
		AfxMessageBox(IDS_ERR_POSITIVE, MB_OK|MB_ICONEXCLAMATION);
		m_dCut.SetFocus();
		m_dCut.SetSel(0, -1);
		return FALSE;
	}
	if ( m_dStep > 0 ) {
		AfxMessageBox(IDS_ERR_POSITIVE, MB_OK|MB_ICONEXCLAMATION);
		m_dStep.SetFocus();
		m_dStep.SetSel(0, -1);
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
	
	return TRUE;
}

void CMKLASetup4::OnCheck() 
{
	UpdateData();
	EnableControl();
}