// MakeLatheSetup1.cpp : 実装ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "NCMakeLatheOpt.h"
#include "MakeLatheSetup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CMakeLatheSetup1, CPropertyPage)
	ON_BN_CLICKED(IDC_MKLA3_COPY, &CMakeLatheSetup1::OnCopyFromIn)
END_MESSAGE_MAP()

#define	GetParentSheet()	static_cast<CMakeLatheSetup *>(GetParentSheet())

/////////////////////////////////////////////////////////////////////////////
// CMakeLatheSetup1 プロパティ ページ

CMakeLatheSetup1::CMakeLatheSetup1() : CPropertyPage(CMakeLatheSetup1::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;
}

void CMakeLatheSetup1::DoDataExchange(CDataExchange* pDX)
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
// CMakeLatheSetup1 メッセージ ハンドラ

BOOL CMakeLatheSetup1::OnInitDialog() 
{
	__super::OnInitDialog();

	// ｶｽﾀﾑｺﾝﾄﾛｰﾙはｺﾝｽﾄﾗｸﾀで初期化できない
	// + GetParentSheet() ﾎﾟｲﾝﾀを取得できない
	CNCMakeLatheOpt* pOpt = GetParentSheet()->GetNCMakeOption();
	m_strCustom	= pOpt->LTH_S_O_CUSTOM;
	m_nSpindle	= pOpt->LTH_I_O_SPINDLE;
	m_dFeed		= pOpt->LTH_D_O_FEED;
	m_dXFeed	= pOpt->LTH_D_O_FEEDX;
	m_dCut		= pOpt->LTH_D_O_CUT * 2.0f;	// 直径値に変換
	m_dPullZ	= pOpt->LTH_D_O_PULLZ;
	m_dPullX	= pOpt->LTH_D_O_PULLX  * 2.0f;
	m_dMargin	= pOpt->LTH_D_O_MARGIN * 2.0f;
	m_nMargin	= pOpt->LTH_I_O_MARGIN;

	UpdateData(FALSE);

	return TRUE;
}

BOOL CMakeLatheSetup1::OnApply() 
{
	CNCMakeLatheOpt* pOpt = GetParentSheet()->GetNCMakeOption();
	pOpt->LTH_S_O_CUSTOM	= m_strCustom;
	pOpt->LTH_I_O_SPINDLE	= m_nSpindle;
	pOpt->LTH_D_O_FEED		= m_dFeed;
	pOpt->LTH_D_O_FEEDX		= m_dXFeed;
	pOpt->LTH_D_O_CUT		= m_dCut / 2.0f;	// 半径値に変換
	pOpt->LTH_D_O_PULLZ		= m_dPullZ;
	pOpt->LTH_D_O_PULLX		= m_dPullX / 2.0f;
	pOpt->LTH_D_O_MARGIN	= m_dMargin / 2.0f;
	pOpt->LTH_I_O_MARGIN	= m_nMargin;

	return TRUE;
}

BOOL CMakeLatheSetup1::OnKillActive() 
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

void CMakeLatheSetup1::OnCopyFromIn()
{
	CMakeLatheSetup*	pParent = GetParentSheet();
	if ( ::IsWindow(pParent->GetInsideDialog()->m_hWnd) ) {
		m_strCustom	= pParent->GetInsideDialog()->m_strCustom;
		m_nSpindle	= (int)(pParent->GetInsideDialog()->m_nSpindle);
		m_dFeed		= (float)(pParent->GetInsideDialog()->m_dFeed);
		m_dXFeed	= (float)(pParent->GetInsideDialog()->m_dXFeed);
		m_dCut		= (float)(pParent->GetInsideDialog()->m_dCut);
		m_dPullZ	= (float)(pParent->GetInsideDialog()->m_dPullZ);
		m_dPullX	= (float)(pParent->GetInsideDialog()->m_dPullX);
		m_dMargin	= (float)(pParent->GetInsideDialog()->m_dMargin);
		m_nMargin	= (int)(pParent->GetInsideDialog()->m_nMargin);
	}
	else {
		CNCMakeLatheOpt* pOpt = GetParentSheet()->GetNCMakeOption();
		m_strCustom = pOpt->LTH_S_I_CUSTOM;
		m_nSpindle	= pOpt->LTH_I_I_SPINDLE;
		m_dFeed		= pOpt->LTH_D_I_FEED;
		m_dXFeed	= pOpt->LTH_D_I_FEEDX;
		m_dCut		= pOpt->LTH_D_I_CUT;
		m_dPullZ	= pOpt->LTH_D_I_PULLZ;
		m_dPullX	= pOpt->LTH_D_I_PULLX;
		m_dMargin	= pOpt->LTH_D_I_MARGIN;
		m_nMargin	= pOpt->LTH_I_I_MARGIN;
	}

	UpdateData(FALSE);
}
