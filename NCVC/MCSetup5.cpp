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
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMCSetup5 プロパティ ページ

CMCSetup5::CMCSetup5() : CPropertyPage(CMCSetup5::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;
	const CMCOption*	pMCopt = AfxGetNCVCApp()->GetMCOption();
	m_bL0Cycle		= pMCopt->m_bL0Cycle;
	m_strAutoBreak	= pMCopt->m_strAutoBreak;
}

CMCSetup5::~CMCSetup5()
{
}

void CMCSetup5::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_MCST5_L0CYCLE, m_bL0Cycle);
	DDX_Text(pDX, IDC_MCST5_AUTOBREAK, m_strAutoBreak);
}

/////////////////////////////////////////////////////////////////////////////
// CMCSetup5 メッセージ ハンドラ

BOOL CMCSetup5::OnApply() 
{
	CMCSetup*	pParent = static_cast<CMCSetup *>(GetParent());
	CMCOption*	pMCopt = AfxGetNCVCApp()->GetMCOption();

	// 再読込ﾁｪｯｸ
	if ( pMCopt->m_bL0Cycle != m_bL0Cycle ) {
		pMCopt->m_bL0Cycle = m_bL0Cycle;
		pParent->m_bReload = TRUE;		// 再読込が必要
	}
	if ( pMCopt->m_strAutoBreak != m_strAutoBreak ) {
		pMCopt->m_strAutoBreak = m_strAutoBreak;
		pParent->m_bReload = TRUE;
	}

	return TRUE;
}
