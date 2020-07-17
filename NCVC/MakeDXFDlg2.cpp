// MakeDXFDlg2.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "NCDoc.h"
#include "MakeDXFDlg.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CMakeDXFDlg2, CPropertyPage)
	//{{AFX_MSG_MAP(CMakeDXFDlg2)
	ON_CBN_SELCHANGE(IDC_MKDX2_CYCLE, &CMakeDXFDlg2::OnSelchangeCycle)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

#define	GetParentSheet()	static_cast<CMakeDXFDlg *>(GetParentSheet())

/////////////////////////////////////////////////////////////////////////////
// CMakeDXFDlg2 プロパティ ページ

CMakeDXFDlg2::CMakeDXFDlg2() : CPropertyPage(CMakeDXFDlg2::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;
	//{{AFX_DATA_INIT(CMakeDXFDlg2)
	m_bOrgCircle	= FALSE;
	m_bOrgCross		= FALSE;
	m_nCycle		= -1;
	//}}AFX_DATA_INIT
}

void CMakeDXFDlg2::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMakeDXFDlg2)
	DDX_Control(pDX, IDC_MKDX2_CYCLE_R, m_dCycleR);
	DDX_Control(pDX, IDC_MKDX2_LENGTH, m_dLength);
	DDX_Check(pDX, IDC_MKDX2_CIRCLE, m_bOrgCircle);
	DDX_Check(pDX, IDC_MKDX2_CROSS, m_bOrgCross);
	DDX_CBIndex(pDX, IDC_MKDX2_CYCLE, m_nCycle);
	//}}AFX_DATA_MAP
}

void CMakeDXFDlg2::EnableControl_CycleR(void)
{
	m_dCycleR.EnableWindow(m_nCycle == 0 ? TRUE : FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// CMakeDXFDlg2 メッセージ ハンドラ

BOOL CMakeDXFDlg2::OnInitDialog() 
{
	__super::OnInitDialog();

	CDXFMakeOption*	pDXFMake = GetParentSheet()->GetDXFMakeOption();
	m_dLength		= pDXFMake->m_dOrgLength;
	m_bOrgCircle	= pDXFMake->m_bOrgCircle;
	m_bOrgCross		= pDXFMake->m_bOrgCross;
	m_nCycle		= pDXFMake->m_nCycle;
	m_dCycleR		= pDXFMake->m_dCycleR;

	UpdateData(FALSE);
	EnableControl_CycleR();

	return TRUE;  // コントロールにフォーカスを設定しないとき、戻り値は TRUE となります
	              // 例外: OCX プロパティ ページの戻り値は FALSE となります
}

void CMakeDXFDlg2::OnSelchangeCycle() 
{
	UpdateData();
	EnableControl_CycleR();
}

BOOL CMakeDXFDlg2::OnApply() 
{
	CDXFMakeOption*	pDXFMake = GetParentSheet()->GetDXFMakeOption();
	pDXFMake->m_dOrgLength	= m_dLength;
	pDXFMake->m_bOrgCircle	= m_bOrgCircle;
	pDXFMake->m_bOrgCross	= m_bOrgCross;
	pDXFMake->m_nCycle		= m_nCycle;
	pDXFMake->m_dCycleR		= m_dCycleR;

	return TRUE;
}

BOOL CMakeDXFDlg2::OnKillActive() 
{
	if ( !__super::OnKillActive() )
		return FALSE;

	if ( m_dLength <= 0 ) {
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_dLength.SetFocus();
		m_dLength.SetSel(0, -1);
		return FALSE;
	}
	if ( m_nCycle==0 && m_dCycleR<=0 ) {
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_dCycleR.SetFocus();
		m_dCycleR.SetSel(0, -1);
		return FALSE;
	}

	return TRUE;
}
