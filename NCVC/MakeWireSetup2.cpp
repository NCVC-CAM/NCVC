// MakeWireSetup2.cpp : 実装ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "NCMakeWireOpt.h"
#include "MakeWireSetup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CMakeWireSetup2, CPropertyPage)
END_MESSAGE_MAP()

#define	GetParentSheet()	static_cast<CMakeWireSetup *>(GetParentSheet())

/////////////////////////////////////////////////////////////////////////////
// CMakeWireSetup2 プロパティ ページ

CMakeWireSetup2::CMakeWireSetup2() : CPropertyPage(CMakeWireSetup2::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;
	//
	m_bAWFstart = m_bAWFend = FALSE;
}

void CMakeWireSetup2::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MKWI2_RL, m_dAWFcircleLo);
	DDX_Control(pDX, IDC_MKWI2_RH, m_dAWFcircleHi);
	DDX_Check(pDX, IDC_MKWI2_AWFSTART, m_bAWFstart);
	DDX_Check(pDX, IDC_MKWI2_AWFEND, m_bAWFend);
	DDX_Text(pDX, IDC_MKWI2_AWFCNT, m_strAwfCnt);
	DDX_Text(pDX, IDC_MKWI2_AWFCUT, m_strAwfCut);
}

/////////////////////////////////////////////////////////////////////////////
// CMakeWireSetup2 メッセージ ハンドラ

BOOL CMakeWireSetup2::OnInitDialog() 
{
	__super::OnInitDialog();

	// ｶｽﾀﾑｺﾝﾄﾛｰﾙはｺﾝｽﾄﾗｸﾀで初期化できない
	// + GetParentSheet() ﾎﾟｲﾝﾀを取得できない
	CNCMakeWireOpt* pOpt = GetParentSheet()->GetNCMakeOption();
	m_dAWFcircleLo	= pOpt->WIR_D_AWFCIRCLE_LO;
	m_dAWFcircleHi	= pOpt->WIR_D_AWFCIRCLE_HI;
	m_bAWFstart		= pOpt->WIR_F_AWFSTART;
	m_bAWFend		= pOpt->WIR_F_AWFEND;
	m_strAwfCnt		= pOpt->WIR_S_AWFCNT;
	m_strAwfCut		= pOpt->WIR_S_AWFCUT;

	UpdateData(FALSE);

	return TRUE;
}

BOOL CMakeWireSetup2::OnApply() 
{
	CNCMakeWireOpt* pOpt = GetParentSheet()->GetNCMakeOption();
	pOpt->WIR_D_AWFCIRCLE_LO= m_dAWFcircleLo;
	pOpt->WIR_D_AWFCIRCLE_HI= m_dAWFcircleHi;
	pOpt->WIR_F_AWFSTART	= m_bAWFstart;
	pOpt->WIR_F_AWFEND		= m_bAWFend;
	pOpt->WIR_S_AWFCNT		= m_strAwfCnt;
	pOpt->WIR_S_AWFCUT		= m_strAwfCut;

	return TRUE;
}

BOOL CMakeWireSetup2::OnKillActive() 
{
	if ( !__super::OnKillActive() )
		return FALSE;

	if ( m_dAWFcircleLo<0 || m_dAWFcircleHi<0.0f ) {
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_dAWFcircleLo.SetFocus();
		m_dAWFcircleLo.SetSel(0, -1);
		return FALSE;
	}

	return TRUE;
}
