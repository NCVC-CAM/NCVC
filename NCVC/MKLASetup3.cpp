// MKLASetup3.cpp : 実装ファイル
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

BEGIN_MESSAGE_MAP(CMKLASetup3, CPropertyPage)
	ON_BN_CLICKED(IDC_MKLA3_COPY, &CMKLASetup3::OnCopyFromOut)
END_MESSAGE_MAP()

#define	GetParentSheet()	static_cast<CMKLASetup *>(GetParentSheet())

/////////////////////////////////////////////////////////////////////////////
// CMKLASetup3 プロパティ ページ

CMKLASetup3::CMKLASetup3() : CPropertyPage(CMKLASetup3::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;
}

void CMKLASetup3::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_MKLA2_CUSTOMCODE, m_strCustom);
	DDX_Control(pDX, IDC_MKNC1_SPINDLE, m_nSpindle);
	DDX_Control(pDX, IDC_MKNC1_FEED, m_dFeed);
	DDX_Control(pDX, IDC_MKLA1_XFEED, m_dXFeed);
	DDX_Control(pDX, IDC_MKLA1_CUT, m_dCut);
	DDX_Control(pDX, IDC_MKLA1_PULL_Z, m_dPullZ);
	DDX_Control(pDX, IDC_MKLA1_PULL_X, m_dPullX);
	DDX_Control(pDX, IDC_MKLA1_MARGIN, m_dMargin);
	DDX_Control(pDX, IDC_MKLA1_MARGINNUM, m_nMargin);
}

/////////////////////////////////////////////////////////////////////////////
// CMKLASetup3 メッセージ ハンドラー

BOOL CMKLASetup3::OnInitDialog()
{
	__super::OnInitDialog();

	CNCMakeLatheOpt* pOpt = GetParentSheet()->GetNCMakeOption();
	m_strCustom	= pOpt->LTH_S_I_CUSTOM;
	m_nSpindle	= pOpt->LTH_I_I_SPINDLE;
	m_dFeed		= pOpt->LTH_D_I_FEED;
	m_dXFeed	= pOpt->LTH_D_I_FEEDX;
	m_dCut		= pOpt->LTH_D_I_CUT * 2.0f;		// 直径値に変換
	m_dPullZ	= pOpt->LTH_D_I_PULLZ;
	m_dPullX	= pOpt->LTH_D_I_PULLX * 2.0f;
	m_dMargin	= pOpt->LTH_D_I_MARGIN * 2.0f;
	m_nMargin	= pOpt->LTH_I_I_MARGIN;

	UpdateData(FALSE);

	return TRUE;
}

BOOL CMKLASetup3::OnApply()
{
	CNCMakeLatheOpt* pOpt = GetParentSheet()->GetNCMakeOption();
	pOpt->LTH_S_I_CUSTOM	= m_strCustom;
	pOpt->LTH_I_I_SPINDLE	= m_nSpindle;
	pOpt->LTH_D_I_FEED		= m_dFeed;
	pOpt->LTH_D_I_FEEDX		= m_dXFeed;
	pOpt->LTH_D_I_CUT		= m_dCut / 2.0f;	// 半径値に変換
	pOpt->LTH_D_I_PULLZ		= m_dPullZ;
	pOpt->LTH_D_I_PULLX		= m_dPullX / 2.0f;
	pOpt->LTH_D_I_MARGIN	= m_dMargin / 2.0f;
	pOpt->LTH_I_I_MARGIN	= m_nMargin;

	return TRUE;
}

BOOL CMKLASetup3::OnKillActive()
{
	if ( !__super::OnKillActive() )
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

	return TRUE;
}

void CMKLASetup3::OnCopyFromOut()
{
	CMKLASetup*	pParent = GetParentSheet();
	if ( ::IsWindow(pParent->GetOutsideDialog()->m_hWnd) ) {
		m_strCustom	= pParent->GetOutsideDialog()->m_strCustom;
		m_nSpindle	= (int)(pParent->GetOutsideDialog()->m_nSpindle);
		m_dFeed		= (float)(pParent->GetOutsideDialog()->m_dFeed);
		m_dXFeed	= (float)(pParent->GetOutsideDialog()->m_dXFeed);
		m_dCut		= (float)(pParent->GetOutsideDialog()->m_dCut);
		m_dPullZ	= (float)(pParent->GetOutsideDialog()->m_dPullZ);
		m_dPullX	= (float)(pParent->GetOutsideDialog()->m_dPullX);
		m_dMargin	= (float)(pParent->GetOutsideDialog()->m_dMargin);
		m_nMargin	= (int)(pParent->GetOutsideDialog()->m_nMargin);
	}
	else {
		CNCMakeLatheOpt* pOpt = GetParentSheet()->GetNCMakeOption();
		m_strCustom	= pOpt->LTH_S_O_CUSTOM;
		m_nSpindle	= pOpt->LTH_I_O_SPINDLE;
		m_dFeed		= pOpt->LTH_D_O_FEED;
		m_dXFeed	= pOpt->LTH_D_O_FEEDX;
		m_dCut		= pOpt->LTH_D_O_CUT;
		m_dPullZ	= pOpt->LTH_D_O_PULLZ;
		m_dPullX	= pOpt->LTH_D_O_PULLX;
		m_dMargin	= pOpt->LTH_D_O_MARGIN;
		m_nMargin	= pOpt->LTH_I_O_MARGIN;
	}

	UpdateData(FALSE);
}
