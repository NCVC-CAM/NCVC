// MKWISetup2.cpp : 実装ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "NCMakeWireOpt.h"
#include "MKWISetup.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CMKWISetup2, CPropertyPage)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMKWISetup1 プロパティ ページ

CMKWISetup2::CMKWISetup2() : CPropertyPage(CMKWISetup2::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;
	//
	m_bAWFstart = m_bAWFend = FALSE;
}

CMKWISetup2::~CMKWISetup2()
{
}

void CMKWISetup2::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MKWI2_RL, m_dAWFcircleLo);
	DDX_Control(pDX, IDC_MKWI2_RH, m_dAWFcircleHi);
	DDX_Check(pDX, IDC_MKWI2_AWFSTART, m_bAWFstart);
	DDX_Check(pDX, IDC_MKWI2_AWFEND, m_bAWFend);
	DDX_Text(pDX, IDC_MKWI2_AWFCNT, m_strAwfCnt);
	DDX_Text(pDX, IDC_MKWI2_AWFCUT, m_strAwfCut);
}

/////////////////////////////////////////////////////////////////////////////
// CMKWISetup2 メッセージ ハンドラ

BOOL CMKWISetup2::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	// ｶｽﾀﾑｺﾝﾄﾛｰﾙはｺﾝｽﾄﾗｸﾀで初期化できない
	// + GetParent() ﾎﾟｲﾝﾀを取得できない
	CNCMakeWireOpt* pOpt = static_cast<CMKWISetup *>(GetParent())->GetNCMakeOption();
	m_dAWFcircleLo	= pOpt->m_dAWFcircleLo;
	m_dAWFcircleHi	= pOpt->m_dAWFcircleHi;
	m_bAWFstart		= pOpt->m_bAWFstart;
	m_bAWFend		= pOpt->m_bAWFend;
	m_strAwfCnt		= pOpt->m_strOption[MKWI_STR_AWFCNT];
	m_strAwfCut		= pOpt->m_strOption[MKWI_STR_AWFCUT];

	UpdateData(FALSE);

	return TRUE;
}

BOOL CMKWISetup2::OnApply() 
{
	CNCMakeWireOpt* pOpt = static_cast<CMKWISetup *>(GetParent())->GetNCMakeOption();
	pOpt->m_dAWFcircleLo= m_dAWFcircleLo;
	pOpt->m_dAWFcircleHi= m_dAWFcircleHi;
	pOpt->m_bAWFstart	= m_bAWFstart;
	pOpt->m_bAWFend		= m_bAWFend;
	pOpt->m_strOption[MKWI_STR_AWFCNT] = m_strAwfCnt;
	pOpt->m_strOption[MKWI_STR_AWFCUT] = m_strAwfCut;

	return TRUE;
}

BOOL CMKWISetup2::OnKillActive() 
{
	if ( !CPropertyPage::OnKillActive() )
		return FALSE;

	if ( m_dAWFcircleLo<0 || m_dAWFcircleHi<0.0 ) {
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_dAWFcircleLo.SetFocus();
		m_dAWFcircleLo.SetSel(0, -1);
		return FALSE;
	}

	return TRUE;
}
