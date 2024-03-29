// MakeNCSetup3.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "resource.h"
#include "NCMakeMillOpt.h"
#include "MakeNCSetup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CMakeNCSetup3, CPropertyPage)
	//{{AFX_MSG_MAP(CMakeNCSetup3)
	ON_CBN_SELCHANGE(IDC_MKNC3_MAKEEND, &CMakeNCSetup3::OnSelchangeMakeEnd)
	ON_CBN_SELCHANGE(IDC_MKNC3_APROCESS, &CMakeNCSetup3::OnSelchangeProcess)
	ON_CBN_SELCHANGE(IDC_MKNC3_CPROCESS, &CMakeNCSetup3::OnSelchangeProcess)
	ON_BN_CLICKED(IDC_MKNC3_DEEP, &CMakeNCSetup3::OnDeep)
	ON_BN_CLICKED(IDC_MKNC3_FINISH, &CMakeNCSetup3::OnDeepFinish)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

#define	GetParentSheet()	static_cast<CMakeNCSetup *>(GetParentSheet())

/////////////////////////////////////////////////////////////////////////////
// CMakeNCSetup3 プロパティ ページ

CMakeNCSetup3::CMakeNCSetup3() : CPropertyPage(CMakeNCSetup3::IDD),
	m_dFeed(TRUE), m_dMakeFeed(TRUE)	// CFloatEditﾌﾗｸﾞ設定
{
	m_psp.dwFlags &= ~PSP_HASHELP;
	//{{AFX_DATA_INIT(CMakeNCSetup3)
	m_nMakeEnd		= -1;
	m_nDeepAll		= -1;
	m_nDeepRound	= -1;
	m_nDeepReturn	= -1;
	m_bDeep			= FALSE;
	m_bHelical		= FALSE;
	m_bFinish		= FALSE;
	//}}AFX_DATA_INIT
}

void CMakeNCSetup3::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMakeNCSetup3)
	DDX_Control(pDX, IDC_MKNC3_FEED, m_dFeed);
	DDX_Control(pDX, IDC_MKNC3_MAKEEND_FEED, m_dMakeFeed);
	DDX_Control(pDX, IDC_MKNC3_HELICAL, m_ctHelical);
	DDX_Control(pDX, IDC_MKNC3_FINISH, m_ctFinish);
	DDX_Control(pDX, IDC_MKNC3_APROCESS, m_ctAProcess);
	DDX_Control(pDX, IDC_MKNC3_CPROCESS, m_ctCProcess);
	DDX_Control(pDX, IDC_MKNC3_ZPROCESS, m_ctZProcess);
	DDX_Control(pDX, IDC_MKNC3_MAKEEND_LB1, m_ctMakeLabel1);
	DDX_Control(pDX, IDC_MKNC3_SPINDLE, m_nSpindle);
	DDX_Control(pDX, IDC_MKNC1_ZCUT, m_dZCut);
	DDX_Control(pDX, IDC_MKNC1_R, m_dZG0Stop);
	DDX_Control(pDX, IDC_MKNC3_MAKEEND_VALUE, m_dMakeValue);
	DDX_Control(pDX, IDC_MKNC3_ZSTEP, m_dZStep);
	DDX_Control(pDX, IDC_MKNC3_DEEPFINAL, m_dDeep);
	DDX_CBIndex(pDX, IDC_MKNC3_MAKEEND, m_nMakeEnd);
	DDX_CBIndex(pDX, IDC_MKNC3_APROCESS, m_nDeepAll);
	DDX_CBIndex(pDX, IDC_MKNC3_CPROCESS, m_nDeepRound);
	DDX_CBIndex(pDX, IDC_MKNC3_ZPROCESS, m_nDeepReturn);
	DDX_Check(pDX, IDC_MKNC3_DEEP, m_bDeep);
	DDX_Check(pDX, IDC_MKNC3_HELICAL, m_bHelical);
	DDX_Check(pDX, IDC_MKNC3_FINISH, m_bFinish);
	//}}AFX_DATA_MAP
}

void CMakeNCSetup3::EnableControl_MakeEnd(void)
{
	static	LPCTSTR	szMakeLabel[] = {
		"-----", "ｵﾌｾｯﾄ値(&V)", "固定Z値(&V)"
	};

	if ( m_nMakeEnd<0 || m_nMakeEnd>2 )
		m_nMakeEnd = 0;
	m_ctMakeLabel1.SetWindowText(szMakeLabel[m_nMakeEnd]);
	m_dMakeValue.EnableWindow(m_nMakeEnd == 0 ? FALSE : TRUE);
}

void CMakeNCSetup3::EnableControl_Deep(void)
{
	m_dDeep.EnableWindow(m_bDeep);
	m_dZStep.EnableWindow(m_bDeep);
	m_ctZProcess.EnableWindow(m_bDeep);
	m_ctAProcess.EnableWindow(m_bDeep);
	m_ctCProcess.EnableWindow(m_bDeep);
	m_ctFinish.EnableWindow(m_bDeep);
	EnableControl_Helical();
	EnableControl_Finish();
}

void CMakeNCSetup3::EnableControl_Helical(void)
{
	m_ctHelical.EnableWindow(m_bDeep && m_nDeepAll==1);
}

void CMakeNCSetup3::EnableControl_Finish(void)
{
	BOOL	bEnable = m_bDeep && m_bFinish ? TRUE : FALSE;
	m_nSpindle.EnableWindow(bEnable);
	m_dFeed.EnableWindow(bEnable);
}

/////////////////////////////////////////////////////////////////////////////
// CMakeNCSetup3 メッセージ ハンドラ

BOOL CMakeNCSetup3::OnInitDialog() 
{
	__super::OnInitDialog();

	// ｶｽﾀﾑｺﾝﾄﾛｰﾙはｺﾝｽﾄﾗｸﾀで初期化できない
	// + GetParentSheet() ﾎﾟｲﾝﾀを取得できない
	CMakeNCSetup*	pParent = GetParentSheet();
	CNCMakeMillOpt* pOpt = pParent->GetNCMakeOption();
	m_nMakeEnd		= pOpt->MIL_I_MAKEEND;
	m_dMakeValue	= pOpt->MIL_D_MAKEEND;
	m_dMakeFeed		= pOpt->MIL_D_MAKEENDFEED;
	m_bDeep			= pOpt->MIL_F_DEEP;
	m_dDeep			= pOpt->MIL_D_DEEP;
	m_dZStep		= pOpt->MIL_D_ZSTEP;
	m_nDeepReturn	= pOpt->MIL_I_DEEPRETURN;
	m_nDeepAll		= pOpt->MIL_I_DEEPALL;
	m_nDeepRound	= pOpt->MIL_I_DEEPROUND;
	m_bHelical		= pOpt->MIL_F_HELICAL;
	m_bFinish		= pOpt->MIL_F_DEEPFINISH;
	m_nSpindle		= pOpt->MIL_I_DEEPSPINDLE;
	m_dFeed			= pOpt->MIL_D_DEEPFEED;
	EnableControl_MakeEnd();
	EnableControl_Deep();

	// ｺﾝｽﾄﾗｸﾀでは初期化できない変数
	if ( ::IsWindow(pParent->m_dlg1.m_hWnd) ) {
		m_dZCut		= (float)(pParent->m_dlg1.m_dZCut);
		m_dZG0Stop	= (float)(pParent->m_dlg1.m_dZG0Stop);
	}
	else {
		m_dZCut		= pOpt->MIL_D_ZCUT;
		m_dZG0Stop	= pOpt->MIL_D_ZG0STOP;
	}

	UpdateData(FALSE);

	return TRUE;  // コントロールにフォーカスを設定しないとき、戻り値は TRUE となります
	              // 例外: OCX プロパティ ページの戻り値は FALSE となります
}

void CMakeNCSetup3::OnSelchangeMakeEnd() 
{
	UpdateData();
	EnableControl_MakeEnd();
}

void CMakeNCSetup3::OnSelchangeProcess()
{
	UpdateData();
	EnableControl_Helical();
}

void CMakeNCSetup3::OnDeep() 
{
	UpdateData();
	EnableControl_Deep();
}

void CMakeNCSetup3::OnDeepFinish() 
{
	UpdateData();
	EnableControl_Finish();
}

BOOL CMakeNCSetup3::OnSetActive() 
{
	CMakeNCSetup*	pParent = GetParentSheet();
	// 基本ﾍﾟｰｼﾞから「切り込み」ﾃﾞｰﾀを取得
	if ( ::IsWindow(pParent->m_dlg1.m_hWnd) ) {
		m_dZCut		= (float)(pParent->m_dlg1.m_dZCut);
		m_dZG0Stop	= (float)(pParent->m_dlg1.m_dZG0Stop);
	}
	else {
		CNCMakeMillOpt* pOpt = pParent->GetNCMakeOption();
		m_dZCut		= pOpt->MIL_D_ZCUT;
		m_dZG0Stop	= pOpt->MIL_D_ZG0STOP;
	}
	return __super::OnSetActive();
}

BOOL CMakeNCSetup3::OnApply() 
{
	// ﾍﾟｰｼﾞ間の依存関係
	// OnKillActive() ではﾍﾟｰｼﾞを切り替えられないのでうっとおしい
	if ( m_nMakeEnd == 2 ) {	// Fix
		if ( (float)m_dMakeValue > (float)m_dZG0Stop ) {
			AfxMessageBox(IDS_ERR_DEEPFIXR, MB_OK|MB_ICONEXCLAMATION);
			m_dMakeValue.SetFocus();
			m_dMakeValue.SetSel(0, -1);
			return FALSE;
		}
		if ( (float)m_dMakeValue <= (float)m_dZCut ) {
			AfxMessageBox(IDS_ERR_DEEPFIXZ, MB_OK|MB_ICONEXCLAMATION);
			m_dMakeValue.SetFocus();
			m_dMakeValue.SetSel(0, -1);
			return FALSE;
		}
	}
	if ( m_bDeep && (float)m_dZCut<(float)m_dDeep ) {
		AfxMessageBox(IDS_ERR_DEEPFINAL, MB_OK|MB_ICONEXCLAMATION);
		m_dDeep.SetFocus();
		m_dDeep.SetSel(0, -1);
		return FALSE;
	}

	CNCMakeMillOpt* pOpt = GetParentSheet()->GetNCMakeOption();
	pOpt->MIL_I_MAKEEND		= m_nMakeEnd;
	pOpt->MIL_D_MAKEEND		= m_dMakeValue;
	pOpt->MIL_D_MAKEENDFEED	= m_dMakeFeed;
	pOpt->MIL_F_DEEP		= m_bDeep;
	pOpt->MIL_D_DEEP		= m_dDeep;
	pOpt->MIL_D_ZSTEP		= m_dZStep;
	pOpt->MIL_I_DEEPRETURN	= m_nDeepReturn;
	pOpt->MIL_I_DEEPALL		= m_nDeepAll;
	pOpt->MIL_I_DEEPROUND	= m_nDeepRound;
	pOpt->MIL_F_HELICAL		= m_bHelical;
	pOpt->MIL_F_DEEPFINISH	= m_bFinish;
	pOpt->MIL_I_DEEPSPINDLE	= m_nSpindle;
	pOpt->MIL_D_DEEPFEED	= m_dFeed;

	return TRUE;
}

BOOL CMakeNCSetup3::OnKillActive() 
{
	if ( !__super::OnKillActive() )
		return FALSE;

	if ( m_nMakeEnd == 1 ) {	// Offset
		if ( m_dMakeValue <= 0 ) {
			AfxMessageBox(IDS_ERR_DEEPVALUE, MB_OK|MB_ICONEXCLAMATION);
			m_dMakeValue.SetFocus();
			m_dMakeValue.SetSel(0, -1);
			return FALSE;
		}
	}
	if ( m_nMakeEnd!=0 || (m_bDeep && m_nDeepReturn!=0) ) {
		if ( m_dMakeFeed <= 0 ) {
			AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
			m_dMakeFeed.SetFocus();
			m_dMakeFeed.SetSel(0, -1);
			return FALSE;
		}
	}

	if ( m_bDeep ) {
		if ( m_dZStep >= 0 ) {
			AfxMessageBox(IDS_ERR_DEEPSTEP, MB_OK|MB_ICONEXCLAMATION);
			m_dZStep.SetFocus();
			m_dZStep.SetSel(0, -1);
			return FALSE;
		}
		if ( m_bFinish ) {
			if ( m_nSpindle <= 0 ) {
				AfxMessageBox(IDS_ERR_SETTING, MB_OK|MB_ICONEXCLAMATION);
				m_nSpindle.SetFocus();
				m_nSpindle.SetSel(0, -1);
				return FALSE;
			}
			if ( m_dFeed <= 0 ) {
				AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
				m_dFeed.SetFocus();
				m_dFeed.SetSel(0, -1);
				return FALSE;
			}
		}
	}

	return TRUE;
}
