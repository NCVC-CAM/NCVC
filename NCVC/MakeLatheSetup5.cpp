// MakeLatheSetup5.cpp : 実装ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "NCMakeLatheOpt.h"
#include "MakeLatheSetup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CMakeLatheSetup5, CPropertyPage)
END_MESSAGE_MAP()

#define	GetParentSheet()	static_cast<CMakeLatheSetup *>(GetParentSheet())

/////////////////////////////////////////////////////////////////////////////
// CMakeLatheSetup5 プロパティ ページ

CMakeLatheSetup5::CMakeLatheSetup5() : CPropertyPage(CMakeLatheSetup5::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;
	m_nTool = -1;
}

void CMakeLatheSetup5::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_MKLA2_CUSTOMCODE, m_strCustom);
	DDX_Control(pDX, IDC_MKNC1_SPINDLE, m_nSpindle);
	DDX_Control(pDX, IDC_MKNC1_FEED, m_dFeed);
	DDX_Control(pDX, IDC_MKLA1_XFEED, m_dXFeed);
	DDX_Control(pDX, IDC_MKLA1_PULL_X, m_dPullX);
	DDX_Control(pDX, IDC_MKNC4_DWELL, m_nDwell);
	DDX_Control(pDX, IDC_MKLA5_WIDTH, m_dWidth);
	DDX_CBIndex(pDX, IDC_MKLA5_TOOL, m_nTool);
}

/////////////////////////////////////////////////////////////////////////////
// CMakeLatheSetup5 メッセージ ハンドラー


BOOL CMakeLatheSetup5::OnInitDialog() 
{
	__super::OnInitDialog();

	CNCMakeLatheOpt* pOpt = GetParentSheet()->GetNCMakeOption();
	m_strCustom	= pOpt->LTH_S_G_CUSTOM;
	m_nSpindle	= pOpt->LTH_I_G_SPINDLE;
	m_dFeed		= pOpt->LTH_D_G_FEED;
	m_dXFeed	= pOpt->LTH_D_G_FEEDX;
	m_dPullX	= pOpt->LTH_D_G_PULLX * 2.0f;
	m_nDwell	= (int)(pOpt->LTH_D_G_DWELL);
	m_dWidth	= pOpt->LTH_D_GROOVEWIDTH;
	m_nTool		= pOpt->LTH_I_GROOVETOOL;

	UpdateData(FALSE);

	return TRUE;
}

BOOL CMakeLatheSetup5::OnApply() 
{
	CNCMakeLatheOpt* pOpt = GetParentSheet()->GetNCMakeOption();
	pOpt->LTH_S_G_CUSTOM	= m_strCustom;
	pOpt->LTH_I_G_SPINDLE	= m_nSpindle;
	pOpt->LTH_D_G_FEED		= m_dFeed;
	pOpt->LTH_D_G_FEEDX		= m_dXFeed;
	pOpt->LTH_D_G_PULLX		= m_dPullX / 2.0f;
	pOpt->LTH_D_G_DWELL		= (float)m_nDwell;
	pOpt->LTH_D_GROOVEWIDTH	= m_dWidth;
	pOpt->LTH_I_GROOVETOOL	= m_nTool;

	return TRUE;
}

BOOL CMakeLatheSetup5::OnKillActive() 
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
	if ( m_dPullX <= 0 ) {
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_dPullX.SetFocus();
		m_dPullX.SetSel(0, -1);
		return FALSE;
	}

	return TRUE;
}
