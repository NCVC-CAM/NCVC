// MakeNCSetup6.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "resource.h"
#include "NCMakeMillOpt.h"
#include "NCMakeLatheOpt.h"
#include "NCMakeWireOpt.h"
#include "MakeNCSetup.h"
#include "MakeLatheSetup.h"
#include "MakeWireSetup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CMakeNCSetup6, CPropertyPage)
	//{{AFX_MSG_MAP(CMakeNCSetup6)
	ON_BN_CLICKED(IDC_MKNC6_R, &CMakeNCSetup6::OnCircleR)
	ON_BN_CLICKED(IDC_MKNC6_IJ, &CMakeNCSetup6::OnCircleIJ)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMakeNCSetup6 プロパティ ページ

CMakeNCSetup6::CMakeNCSetup6() : CPropertyPage(CMakeNCSetup6::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;
	//{{AFX_DATA_INIT(CMakeNCSetup6)
	m_nDot			= -1;
	m_nFDot			= -1;
	m_bZeroCut		= FALSE;
	m_nCircleCode	= -1;
	m_nIJ			= -1;
	m_bCircleHalf	= FALSE;
	m_bZeroCutIJ	= TRUE;
	m_bEllipse		= FALSE;
	//}}AFX_DATA_INIT
}

void CMakeNCSetup6::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMakeNCSetup6)
	DDX_Control(pDX, IDC_MKNC6_KOUSA, m_dEllipse);
	DDX_Control(pDX, IDC_MKNC6_CGRP, m_ctCircleGroup);
	DDX_Control(pDX, IDC_MKNC6_R, m_ctCircleR);
	DDX_Control(pDX, IDC_MKNC6_IJ, m_ctCircleIJ);
	DDX_Control(pDX, IDC_MKNC6_CIRCLEHALF, m_ctCircleHalf);
	DDX_Control(pDX, IDC_MKNC6_ZEROCUT_IJ, m_ctZeroCutIJ);
	DDX_CBIndex(pDX, IDC_MKNC6_POINT, m_nDot);
	DDX_CBIndex(pDX, IDC_MKNC6_FEED, m_nFDot);
	DDX_Radio(pDX, IDC_MKNC6_CIRCLECODE1, m_nCircleCode);
	DDX_Radio(pDX, IDC_MKNC6_R, m_nIJ);
	DDX_Check(pDX, IDC_MKNC6_ZEROCUT, m_bZeroCut);
	DDX_Check(pDX, IDC_MKNC6_CIRCLEHALF, m_bCircleHalf);
	DDX_Check(pDX, IDC_MKNC6_ZEROCUT_IJ, m_bZeroCutIJ);
	DDX_Check(pDX, IDC_MKNC6_ELL2CIR, m_bEllipse);
	//}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////
// CMakeNCSetup6 メッセージ ハンドラ

BOOL CMakeNCSetup6::OnInitDialog() 
{
	__super::OnInitDialog();
	
	// ｶｽﾀﾑｺﾝﾄﾛｰﾙはｺﾝｽﾄﾗｸﾀで初期化できない
	// + GetParentSheet() ﾎﾟｲﾝﾀを取得できない
	CWnd*	pParent = GetParentSheet();
	if ( pParent->IsKindOf(RUNTIME_CLASS(CMakeNCSetup)) ) {
		CNCMakeMillOpt* pOpt = static_cast<CMakeNCSetup *>(pParent)->GetNCMakeOption();
		m_nDot			= pOpt->MIL_I_DOT;
		m_nFDot			= pOpt->MIL_I_FDOT;
		m_bZeroCut		= pOpt->MIL_F_ZEROCUT;
		m_nCircleCode	= pOpt->MIL_I_CIRCLECODE;
		m_nIJ			= pOpt->MIL_I_IJ;
		m_bCircleHalf	= pOpt->MIL_F_CIRCLEHALF;
		m_bZeroCutIJ	= pOpt->MIL_F_ZEROCUT_IJ;
		m_dEllipse		= pOpt->MIL_D_ELLIPSE;
		m_bEllipse		= pOpt->MIL_F_ELLIPSE;
	}
	else if ( pParent->IsKindOf(RUNTIME_CLASS(CMakeLatheSetup)) ) {
		// 旋盤ﾓｰﾄﾞ
		CNCMakeLatheOpt* pOpt = static_cast<CMakeLatheSetup *>(pParent)->GetNCMakeOption();
		m_nDot			= pOpt->LTH_I_DOT;
		m_nFDot			= pOpt->LTH_I_FDOT;
		m_bZeroCut		= pOpt->LTH_F_ZEROCUT;
		m_nCircleCode	= pOpt->LTH_I_CIRCLECODE;
		m_nIJ			= pOpt->LTH_I_IJ;
		m_bCircleHalf	= pOpt->LTH_F_CIRCLEHALF;
		m_bZeroCutIJ	= pOpt->LTH_F_ZEROCUT_IJ;
		m_dEllipse		= pOpt->LTH_D_ELLIPSE;
		m_bEllipse		= pOpt->LTH_F_ELLIPSE;
	}
	else {
		// ﾜｲﾔ放電加工機ﾓｰﾄﾞ
		m_ctCircleGroup.ShowWindow(SW_HIDE);
		m_ctCircleR.ShowWindow(SW_HIDE);
		m_ctCircleIJ.ShowWindow(SW_HIDE);
		m_ctCircleHalf.ShowWindow(SW_HIDE);
		m_ctZeroCutIJ.ShowWindow(SW_HIDE);
		CNCMakeWireOpt* pOpt = static_cast<CMakeWireSetup *>(pParent)->GetNCMakeOption();
		m_nDot			= pOpt->WIR_I_DOT;
		m_nFDot			= pOpt->WIR_I_FDOT;
		m_bZeroCut		= pOpt->WIR_F_ZEROCUT;
		m_nCircleCode	= pOpt->WIR_I_CIRCLECODE;
		m_dEllipse		= pOpt->WIR_D_ELLIPSE;
		m_bEllipse		= pOpt->WIR_F_ELLIPSE;
	}

	if ( m_nIJ == 0 )
		OnCircleR();

	UpdateData(FALSE);

	return TRUE;  // コントロールにフォーカスを設定しないとき、戻り値は TRUE となります
	              // 例外: OCX プロパティ ページの戻り値は FALSE となります
}

void CMakeNCSetup6::OnCircleR() 
{
	m_ctCircleHalf.EnableWindow(FALSE);
	m_ctZeroCutIJ.EnableWindow(FALSE);
}

void CMakeNCSetup6::OnCircleIJ() 
{
	m_ctCircleHalf.EnableWindow(TRUE);
	m_ctZeroCutIJ.EnableWindow(TRUE);
}

BOOL CMakeNCSetup6::OnApply() 
{
	CWnd*	pParent = GetParentSheet();
	if ( pParent->IsKindOf(RUNTIME_CLASS(CMakeNCSetup)) ) {
		CNCMakeMillOpt* pOpt = static_cast<CMakeNCSetup *>(pParent)->GetNCMakeOption();
		pOpt->MIL_I_DOT			= m_nDot;
		pOpt->MIL_I_FDOT		= m_nFDot;
		pOpt->MIL_F_ZEROCUT		= m_bZeroCut;
		pOpt->MIL_I_CIRCLECODE	= m_nCircleCode;
		pOpt->MIL_I_IJ			= m_nIJ;
		pOpt->MIL_F_CIRCLEHALF	= m_bCircleHalf;
		pOpt->MIL_F_ZEROCUT_IJ	= m_bZeroCutIJ;
		pOpt->MIL_D_ELLIPSE		= m_dEllipse;
		pOpt->MIL_F_ELLIPSE		= m_bEllipse;
	}
	else if ( pParent->IsKindOf(RUNTIME_CLASS(CMakeLatheSetup)) ) {
		CNCMakeLatheOpt* pOpt = static_cast<CMakeLatheSetup *>(pParent)->GetNCMakeOption();
		pOpt->LTH_I_DOT			= m_nDot;
		pOpt->LTH_I_FDOT		= m_nFDot;
		pOpt->LTH_F_ZEROCUT		= m_bZeroCut;
		pOpt->LTH_I_CIRCLECODE	= m_nCircleCode;
		pOpt->LTH_I_IJ			= m_nIJ;
		pOpt->LTH_F_CIRCLEHALF	= m_bCircleHalf;
		pOpt->LTH_F_ZEROCUT_IJ	= m_bZeroCutIJ;
		pOpt->LTH_D_ELLIPSE		= m_dEllipse;
		pOpt->LTH_F_ELLIPSE		= m_bEllipse;
	}
	else {
		CNCMakeWireOpt* pOpt = static_cast<CMakeWireSetup *>(pParent)->GetNCMakeOption();
		pOpt->WIR_I_DOT			= m_nDot;
		pOpt->WIR_I_FDOT		= m_nFDot;
		pOpt->WIR_F_ZEROCUT		= m_bZeroCut;
		pOpt->WIR_I_CIRCLECODE	= m_nCircleCode;
		pOpt->WIR_D_ELLIPSE		= m_dEllipse;
		pOpt->WIR_F_ELLIPSE		= m_bEllipse;
	}

	return TRUE;
}

BOOL CMakeNCSetup6::OnKillActive() 
{
	if ( !__super::OnKillActive() )
		return FALSE;

	if ( m_dEllipse < NCMIN ) {
		AfxMessageBox(IDS_ERR_ELLIPSE_KOUSA, MB_OK|MB_ICONEXCLAMATION);
		m_dEllipse.SetFocus();
		m_dEllipse.SetSel(0, -1);
		return FALSE;
	}

	return TRUE;
}
