// MachineSetup1.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "MachineOption.h"
#include "MachineSetup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CMachineSetup1, CPropertyPage)
	//{{AFX_MSG_MAP(CMachineSetup1)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMachineSetup1 プロパティ ページ

CMachineSetup1::CMachineSetup1() : CPropertyPage(CMachineSetup1::IDD),
	m_dFspeed(TRUE)	// CFloatEditﾌﾗｸﾞ設定
{
	m_psp.dwFlags &= ~PSP_HASHELP;
	//{{AFX_DATA_INIT(CMachineSetup1)
	m_nFselect = -1;
	//}}AFX_DATA_INIT
	for ( int i=0; i<MODALGROUP; m_nModalGroup[i++]=-1 );
}

void CMachineSetup1::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMachineSetup1)
	DDX_Control(pDX, IDC_MCST1_FSPEED, m_dFspeed);
	DDX_Control(pDX, IDC_MCST1_BLOCK, m_dBlock);
	DDX_Text(pDX, IDC_MCST1_NAME, m_strName);
	DDX_CBIndex(pDX, IDC_MCST1_FSEC, m_nFselect);
	//}}AFX_DATA_MAP
	int		i;
	for ( i=0; i<MODALGROUP; i++ )
		DDX_CBIndex(pDX, i+IDC_MCST1_MGROUP_A, m_nModalGroup[i]);
	for ( i=0; i<NCXYZ; i++ )
		DDX_Control(pDX, i+IDC_MCST1_XSPEED, m_nMoveSpeed[i]);
}

/////////////////////////////////////////////////////////////////////////////
// CMachineSetup1 メッセージ ハンドラ

BOOL CMachineSetup1::OnInitDialog() 
{
	__super::OnInitDialog();

	int			i;
	const CMachineOption*	pMCopt = AfxGetNCVCApp()->GetMachineOption();
	m_strName = pMCopt->m_strMCname;
	// ｺﾝｽﾄﾗｸﾀではできないｺﾝﾄﾛｰﾙの初期化
	for ( i=0; i<MODALGROUP; i++ )
		m_nModalGroup[i] = pMCopt->m_nModal[i];
	for ( i=0; i<NCXYZ; i++ )
		m_nMoveSpeed[i] = pMCopt->m_nG0Speed[i];
	m_nFselect	= pMCopt->m_nFDot;
	m_dFspeed	= pMCopt->m_dFeed;
	m_dBlock	= pMCopt->m_dBlock;
	UpdateData(FALSE);

	return TRUE;  // コントロールにフォーカスを設定しないとき、戻り値は TRUE となります
	              // 例外: OCX プロパティ ページの戻り値は FALSE となります
}

BOOL CMachineSetup1::OnApply() 
{
	int		i;
	CMachineSetup*	pParent = static_cast<CMachineSetup *>(GetParentSheet());
	CMachineOption*	pMCopt = AfxGetNCVCApp()->GetMachineOption();

	pMCopt->m_strMCname = m_strName;

	// 再計算or再読込ﾁｪｯｸ
	for ( i=0; i<MODALGROUP; i++ ) {
		if ( pMCopt->m_nModal[i] != m_nModalGroup[i] ) {
			pMCopt->m_nModal[i] = m_nModalGroup[i];
			pParent->m_bReload = TRUE;		// 再読込が必要
		}
	}
	for ( i=0; i<NCXYZ; i++ ) {
		if ( pMCopt->m_nG0Speed[i] != m_nMoveSpeed[i] ) {
			pMCopt->m_nG0Speed[i] = m_nMoveSpeed[i];
			pParent->m_bCalcThread = TRUE;	// 再計算が必要
		}
	}
	if ( pMCopt->m_nFDot != m_nFselect ) {
		pMCopt->m_nFDot = m_nFselect;
		pParent->m_bReload = TRUE;
	}
	if ( pMCopt->m_dFeed != m_dFspeed ) {
		pMCopt->m_dFeed = m_dFspeed;
		pParent->m_bReload = TRUE;
	}
	if ( pMCopt->m_dBlock != m_dBlock ) {
		pMCopt->m_dBlock = m_dBlock;
		pParent->m_bCalcThread = TRUE;
	}

	return TRUE;
}

BOOL CMachineSetup1::OnKillActive() 
{
	if ( !__super::OnKillActive() )
		return FALSE;

	if ( m_dFspeed <= 0 ) {
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_dFspeed.SetFocus();
		m_dFspeed.SetSel(0, -1);
		return FALSE;
	}
	if ( m_dBlock < 0 ) {
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_dBlock.SetFocus();
		m_dBlock.SetSel(0, -1);
		return FALSE;
	}
	for ( int i=0; i<NCXYZ; i++ ) {
		if ( m_nMoveSpeed[i] <= 0 ) {
			AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
			m_nMoveSpeed[i].SetFocus();
			m_nMoveSpeed[i].SetSel(0, -1);
			return FALSE;
		}
	}

	return TRUE;
}
