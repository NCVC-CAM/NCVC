// MKNCSetup3.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "resource.h"
#include "NCMakeOption.h"
#include "MKNCSetup.h"

#include "MagaDbgMac.h"
#include "MKNCSetup3.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CMKNCSetup3, CPropertyPage)
	//{{AFX_MSG_MAP(CMKNCSetup3)
	ON_CBN_SELCHANGE(IDC_MKNC3_MAKEEND, OnSelchangeMakeEnd)
	ON_CBN_SELCHANGE(IDC_MKNC3_APROCESS, OnSelchangeProcess)
	ON_CBN_SELCHANGE(IDC_MKNC3_CPROCESS, OnSelchangeProcess)
	ON_BN_CLICKED(IDC_MKNC3_DEEP, OnDeep)
	ON_BN_CLICKED(IDC_MKNC3_FINISH, OnDeepFinish)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMKNCSetup3 プロパティ ページ

CMKNCSetup3::CMKNCSetup3() : CPropertyPage(CMKNCSetup3::IDD),
	m_dFeed(TRUE), m_dMakeFeed(TRUE)	// CFloatEditﾌﾗｸﾞ設定
{
	m_psp.dwFlags &= ~PSP_HASHELP;
	//{{AFX_DATA_INIT(CMKNCSetup3)
	m_nMakeEnd	= -1;
	m_nAProcess	= -1;
	m_nCProcess	= -1;
	m_nZProcess	= -1;
	m_bDeep		= FALSE;
	m_bHelical	= FALSE;
	m_bFinish	= FALSE;
	//}}AFX_DATA_INIT
}

CMKNCSetup3::~CMKNCSetup3()
{
}

void CMKNCSetup3::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMKNCSetup3)
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
	DDX_CBIndex(pDX, IDC_MKNC3_APROCESS, m_nAProcess);
	DDX_CBIndex(pDX, IDC_MKNC3_CPROCESS, m_nCProcess);
	DDX_CBIndex(pDX, IDC_MKNC3_ZPROCESS, m_nZProcess);
	DDX_Check(pDX, IDC_MKNC3_DEEP, m_bDeep);
	DDX_Check(pDX, IDC_MKNC3_HELICAL, m_bHelical);
	DDX_Check(pDX, IDC_MKNC3_FINISH, m_bFinish);
	//}}AFX_DATA_MAP
}

void CMKNCSetup3::EnableControl_MakeEnd(void)
{
	static	LPCTSTR	szMakeLabel[] = {
		"-----", "ｵﾌｾｯﾄ値(&V)", "固定Z値(&V)"
	};

	if ( m_nMakeEnd<0 || m_nMakeEnd>2 )
		m_nMakeEnd = 0;
	m_ctMakeLabel1.SetWindowText(szMakeLabel[m_nMakeEnd]);
	m_dMakeValue.EnableWindow(m_nMakeEnd == 0 ? FALSE : TRUE);
}

void CMKNCSetup3::EnableControl_Deep(void)
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

void CMKNCSetup3::EnableControl_Helical(void)
{
	m_ctHelical.EnableWindow(m_bDeep && m_nAProcess==1);
}

void CMKNCSetup3::EnableControl_Finish(void)
{
	BOOL	bEnable = m_bDeep && m_bFinish ? TRUE : FALSE;
	m_nSpindle.EnableWindow(bEnable);
	m_dFeed.EnableWindow(bEnable);
}

/////////////////////////////////////////////////////////////////////////////
// CMKNCSetup3 メッセージ ハンドラ

BOOL CMKNCSetup3::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	// ｶｽﾀﾑｺﾝﾄﾛｰﾙはｺﾝｽﾄﾗｸﾀで初期化できない
	// + GetParent() ﾎﾟｲﾝﾀを取得できない
	CMKNCSetup*	pParent = static_cast<CMKNCSetup *>(GetParent());
	CNCMakeOption* pOpt = pParent->GetNCMakeOption();
	m_nMakeEnd		= pOpt->m_nMakeEnd;
	m_dMakeValue	= pOpt->m_dMakeValue;
	m_dMakeFeed		= pOpt->m_dMakeFeed;
	m_bDeep			= pOpt->m_bDeep;
	m_dDeep			= pOpt->m_dDeep;
	m_dZStep		= pOpt->m_dZStep;
	m_nZProcess		= pOpt->m_nDeepZProcess;
	m_nAProcess		= pOpt->m_nDeepAProcess;
	m_nCProcess		= pOpt->m_nDeepCProcess;
	m_bHelical		= pOpt->m_bHelical;
	m_bFinish		= pOpt->m_bDeepFinish;
	m_nSpindle		= pOpt->m_nDeepSpindle;
	m_dFeed			= pOpt->m_dDeepFeed;
	EnableControl_MakeEnd();
	EnableControl_Deep();

	// ｺﾝｽﾄﾗｸﾀでは初期化できない変数
	if ( ::IsWindow(pParent->m_dlg1.m_hWnd) ) {
		m_dZCut		= (double)(pParent->m_dlg1.m_dZCut);
		m_dZG0Stop	= (double)(pParent->m_dlg1.m_dZG0Stop);
	}
	else {
		m_dZCut		= pOpt->m_dZCut;
		m_dZG0Stop	= pOpt->m_dZG0Stop;
	}

	UpdateData(FALSE);

	return TRUE;  // コントロールにフォーカスを設定しないとき、戻り値は TRUE となります
	              // 例外: OCX プロパティ ページの戻り値は FALSE となります
}

void CMKNCSetup3::OnSelchangeMakeEnd() 
{
	UpdateData();
	EnableControl_MakeEnd();
}

void CMKNCSetup3::OnSelchangeProcess()
{
	UpdateData();
	EnableControl_Helical();
}

void CMKNCSetup3::OnDeep() 
{
	UpdateData();
	EnableControl_Deep();
}

void CMKNCSetup3::OnDeepFinish() 
{
	UpdateData();
	EnableControl_Finish();
}

BOOL CMKNCSetup3::OnSetActive() 
{
	CMKNCSetup*	pParent = static_cast<CMKNCSetup *>(GetParent());
	// 基本ﾍﾟｰｼﾞから「切り込み」ﾃﾞｰﾀを取得
	if ( ::IsWindow(pParent->m_dlg1.m_hWnd) ) {
		m_dZCut		= (double)(pParent->m_dlg1.m_dZCut);
		m_dZG0Stop	= (double)(pParent->m_dlg1.m_dZG0Stop);
	}
	else {
		CNCMakeOption* pOpt = pParent->GetNCMakeOption();
		m_dZCut		= pOpt->m_dZCut;
		m_dZG0Stop	= pOpt->m_dZG0Stop;
	}
	return CPropertyPage::OnSetActive();
}

BOOL CMKNCSetup3::OnApply() 
{
	// ﾍﾟｰｼﾞ間の依存関係
	// OnKillActive() ではﾍﾟｰｼﾞを切り替えられないのでうっとおしい
	if ( m_nMakeEnd == 2 ) {	// Fix
		if ( (double)m_dMakeValue > (double)m_dZG0Stop ) {
			AfxMessageBox(IDS_ERR_DEEPFIXR, MB_OK|MB_ICONEXCLAMATION);
			m_dMakeValue.SetFocus();
			m_dMakeValue.SetSel(0, -1);
			return FALSE;
		}
		if ( (double)m_dMakeValue <= (double)m_dZCut ) {
			AfxMessageBox(IDS_ERR_DEEPFIXZ, MB_OK|MB_ICONEXCLAMATION);
			m_dMakeValue.SetFocus();
			m_dMakeValue.SetSel(0, -1);
			return FALSE;
		}
	}
	if ( m_bDeep && (double)m_dZCut<(double)m_dDeep ) {
		AfxMessageBox(IDS_ERR_DEEPFINAL, MB_OK|MB_ICONEXCLAMATION);
		m_dDeep.SetFocus();
		m_dDeep.SetSel(0, -1);
		return FALSE;
	}

	CNCMakeOption* pOpt = static_cast<CMKNCSetup *>(GetParent())->GetNCMakeOption();
	pOpt->m_nMakeEnd		= m_nMakeEnd;
	pOpt->m_dMakeValue		= m_dMakeValue;
	pOpt->m_dMakeFeed		= m_dMakeFeed;
	pOpt->m_bDeep			= m_bDeep;
	pOpt->m_dDeep			= m_dDeep;
	pOpt->m_dZStep			= m_dZStep;
	pOpt->m_nDeepZProcess	= m_nZProcess;
	pOpt->m_nDeepAProcess	= m_nAProcess;
	pOpt->m_nDeepCProcess	= m_nCProcess;
	pOpt->m_bHelical		= m_bHelical;
	pOpt->m_bDeepFinish		= m_bFinish;
	pOpt->m_nDeepSpindle	= m_nSpindle;
	pOpt->m_dDeepFeed		= m_dFeed;

	return TRUE;
}

BOOL CMKNCSetup3::OnKillActive() 
{
	if ( !CPropertyPage::OnKillActive() )
		return FALSE;

	if ( m_nMakeEnd == 1 ) {	// Offset
		if ( m_dMakeValue <= 0 ) {
			AfxMessageBox(IDS_ERR_DEEPVALUE, MB_OK|MB_ICONEXCLAMATION);
			m_dMakeValue.SetFocus();
			m_dMakeValue.SetSel(0, -1);
			return FALSE;
		}
	}
	if ( m_nMakeEnd!=0 || (m_bDeep && m_nZProcess!=0) ) {
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
