// MakeNCSetup8.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "resource.h"
#include "NCMakeMillOpt.h"
#include "MakeNCSetup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CMakeNCSetup8, CPropertyPage)
	//{{AFX_MSG_MAP(CMakeNCSetup8)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

#define	GetParentSheet()	static_cast<CMakeNCSetup *>(GetParentSheet())

/////////////////////////////////////////////////////////////////////////////
// CMakeNCSetup8 プロパティ ページ

CMakeNCSetup8::CMakeNCSetup8() : CPropertyPage(CMakeNCSetup8::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;
	//{{AFX_DATA_INIT(CMakeNCSetup8)
	m_bLayerComment	= FALSE;
	m_bL0Cycle		= FALSE;
	m_nMoveZ		= 0;
	m_strCustMoveB = _T("");
	m_strCustMoveA = _T("");
	//}}AFX_DATA_INIT
}

void CMakeNCSetup8::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMakeNCSetup8)
	DDX_Check(pDX, IDC_MKNC8_LAYERCOMMENT, m_bLayerComment);
	DDX_Check(pDX, IDC_MKNC8_L0CYCLE, m_bL0Cycle);
	DDX_CBIndex(pDX, IDC_MKNC8_MOVEZ, m_nMoveZ);
	DDX_Text(pDX, IDC_MKNC8_CUSTMOVEB, m_strCustMoveB);
	DDX_Text(pDX, IDC_MKNC8_CUSTMOVEA, m_strCustMoveA);
	//}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////
// CMakeNCSetup8 メッセージ ハンドラ

BOOL CMakeNCSetup8::OnInitDialog() 
{
	__super::OnInitDialog();

	// ｶｽﾀﾑｺﾝﾄﾛｰﾙはｺﾝｽﾄﾗｸﾀで初期化できない
	// + GetParentSheet() ﾎﾟｲﾝﾀを取得できない
	CNCMakeMillOpt* pOpt = GetParentSheet()->GetNCMakeOption();
	m_bLayerComment	= pOpt->MIL_F_LAYERCOMMENT;
	m_bL0Cycle		= pOpt->MIL_F_L0CYCLE;
	m_nMoveZ		= pOpt->MIL_I_MOVEZ;
	m_strCustMoveB	= pOpt->MIL_S_CUSTMOVE_B;
	m_strCustMoveA	= pOpt->MIL_S_CUSTMOVE_A;

	UpdateData(FALSE);

	return TRUE;  // コントロールにフォーカスを設定しないとき、戻り値は TRUE となります
	              // 例外: OCX プロパティ ページの戻り値は FALSE となります
}

BOOL CMakeNCSetup8::OnApply() 
{
	CNCMakeMillOpt* pOpt = GetParentSheet()->GetNCMakeOption();
	pOpt->MIL_F_LAYERCOMMENT= m_bLayerComment;
	pOpt->MIL_F_L0CYCLE		= m_bL0Cycle;
	pOpt->MIL_I_MOVEZ		= m_nMoveZ;
	pOpt->MIL_S_CUSTMOVE_B	= m_strCustMoveB;
	pOpt->MIL_S_CUSTMOVE_A	= m_strCustMoveA;

	return TRUE;
}
