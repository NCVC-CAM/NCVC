// MKNCSetup6.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "resource.h"
#include "NCMakeOption.h"
#include "MKNCSetup.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CMKNCSetup6, CPropertyPage)
	//{{AFX_MSG_MAP(CMKNCSetup6)
	ON_BN_CLICKED(IDC_MKNC6_R, OnCircleR)
	ON_BN_CLICKED(IDC_MKNC6_IJ, OnCircleIJ)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMKNCSetup6 プロパティ ページ

CMKNCSetup6::CMKNCSetup6() : CPropertyPage(CMKNCSetup6::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;
	//{{AFX_DATA_INIT(CMKNCSetup6)
	m_nDot			= -1;
	m_nFDot			= -1;
	m_bZeroCut		= FALSE;
	m_nCircleCode	= -1;
	m_nIJ			= -1;
	m_bCircleHalf	= FALSE;
	m_bEllipse		= FALSE;
	//}}AFX_DATA_INIT
}

CMKNCSetup6::~CMKNCSetup6()
{
}

void CMKNCSetup6::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMKNCSetup6)
	DDX_Control(pDX, IDC_MKNC6_KOUSA, m_dEllipse);
	DDX_Control(pDX, IDC_MKNC6_CIRCLEHALF, m_ctCircleHalf);
	DDX_CBIndex(pDX, IDC_MKNC6_POINT, m_nDot);
	DDX_CBIndex(pDX, IDC_MKNC6_FEED, m_nFDot);
	DDX_Check(pDX, IDC_MKNC6_ZEROCUT, m_bZeroCut);
	DDX_Radio(pDX, IDC_MKNC6_CIRCLECODE1, m_nCircleCode);
	DDX_Radio(pDX, IDC_MKNC6_R, m_nIJ);
	DDX_Check(pDX, IDC_MKNC6_CIRCLEHALF, m_bCircleHalf);
	DDX_Check(pDX, IDC_MKNC6_ELL2CIR, m_bEllipse);
	//}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////
// CMKNCSetup6 メッセージ ハンドラ

BOOL CMKNCSetup6::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	// ｶｽﾀﾑｺﾝﾄﾛｰﾙはｺﾝｽﾄﾗｸﾀで初期化できない
	// + GetParent() ﾎﾟｲﾝﾀを取得できない
	CNCMakeOption* pOpt = static_cast<CMKNCSetup *>(GetParent())->GetNCMakeOption();
	m_nDot			= pOpt->m_nDot;
	m_nFDot			= pOpt->m_nFDot;
	m_bZeroCut		= pOpt->m_bZeroCut;
	m_nCircleCode	= pOpt->m_nCircleCode;
	m_nIJ			= pOpt->m_nIJ;
	m_bCircleHalf	= pOpt->m_bCircleHalf;
	if ( m_nIJ == 0 )
		m_ctCircleHalf.EnableWindow(FALSE);
	m_dEllipse		= pOpt->m_dEllipse;
	m_bEllipse		= pOpt->m_bEllipse;

	UpdateData(FALSE);

	return TRUE;  // コントロールにフォーカスを設定しないとき、戻り値は TRUE となります
	              // 例外: OCX プロパティ ページの戻り値は FALSE となります
}

void CMKNCSetup6::OnCircleR() 
{
	m_ctCircleHalf.EnableWindow(FALSE);
}

void CMKNCSetup6::OnCircleIJ() 
{
	m_ctCircleHalf.EnableWindow(TRUE);
}

BOOL CMKNCSetup6::OnApply() 
{
	CNCMakeOption* pOpt = static_cast<CMKNCSetup *>(GetParent())->GetNCMakeOption();
	pOpt->m_nDot		= m_nDot;
	pOpt->m_nFDot		= m_nFDot;
	pOpt->m_bZeroCut	= m_bZeroCut;
	pOpt->m_nCircleCode	= m_nCircleCode;
	pOpt->m_nIJ			= m_nIJ;
	pOpt->m_bCircleHalf	= m_bCircleHalf;
	pOpt->m_dEllipse	= m_dEllipse;
	pOpt->m_bEllipse	= m_bEllipse;

	return TRUE;
}

BOOL CMKNCSetup6::OnKillActive() 
{
	if ( !CPropertyPage::OnKillActive() )
		return FALSE;

	if ( m_dEllipse < NCMIN ) {
		AfxMessageBox(IDS_ERR_ELLIPSE_KOUSA, MB_OK|MB_ICONEXCLAMATION);
		m_dEllipse.SetFocus();
		m_dEllipse.SetSel(0, -1);
		return FALSE;
	}

	return TRUE;
}
