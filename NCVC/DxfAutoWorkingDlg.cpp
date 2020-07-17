// DxfAutoWorkingDlg.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "DXFDoc.h"
#include "DxfAutoWorkingDlg.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CDxfAutoWorkingDlg, CDialog)
	//{{AFX_MSG_MAP(CDxfAutoWorkingDlg)
	ON_BN_CLICKED(IDC_AUTO_SELECT_OUTLINE, &CDxfAutoWorkingDlg::OnBnClickedSelect)
	ON_BN_CLICKED(IDC_AUTO_SELECT_POCKET, &CDxfAutoWorkingDlg::OnBnClickedSelect)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDxfAutoWorkingDlg ダイアログ

CDxfAutoWorkingDlg::CDxfAutoWorkingDlg(AUTOWORKINGDATA* pAuto)
	: CDialog(CDxfAutoWorkingDlg::IDD, NULL)
{
	//{{AFX_DATA_INIT(CDxfAutoWorkingDlg)
	m_nSelect	= pAuto->nSelect;
	m_dOffset	= pAuto->dOffset;
	m_bAcuteRound = pAuto->bAcuteRound;
	m_nLoopCnt	= pAuto->nLoopCnt;
	m_nScan		= pAuto->nScanLine;
	m_bCircle	= pAuto->bCircleScroll;
	//}}AFX_DATA_INIT
}

void CDxfAutoWorkingDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDxfAutoWorkingDlg)
	DDX_Control(pDX, IDC_AUTO_OFFSET, m_ctOffset);
	DDX_Control(pDX, IDC_AUTO_ACUTEROUND, m_ctAcuteRound);
	DDX_Control(pDX, IDC_AUTO_LOOPCNT, m_ctLoop);
	DDX_Control(pDX, IDC_AUTO_SCANLINE, m_ctScan);
	DDX_Control(pDX, IDC_AUTO_CIRCLESCROLL, m_ctCircle);
	DDX_Radio(pDX, IDC_AUTO_SELECT_OUTLINE, m_nSelect);
	DDX_Check(pDX, IDC_AUTO_ACUTEROUND, m_bAcuteRound);
	DDX_CBIndex(pDX, IDC_AUTO_SCANLINE, m_nScan);
	DDX_Check(pDX, IDC_AUTO_CIRCLESCROLL, m_bCircle);
	//}}AFX_DATA_MAP
}

void CDxfAutoWorkingDlg::SetDetailCtrl(void)
{
	if ( m_nSelect > 0 ) {		// ﾎﾟｹｯﾄ加工
		m_ctScan.EnableWindow(TRUE);
		m_ctCircle.EnableWindow(TRUE);
	}
	else {						// 輪郭加工
		m_ctScan.EnableWindow(FALSE);
		m_ctCircle.EnableWindow(FALSE);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CDxfAutoWorkingDlg メッセージ ハンドラ

BOOL CDxfAutoWorkingDlg::OnInitDialog() 
{
	__super::OnInitDialog();
	m_ctOffset = m_dOffset;	// ﾃﾞﾌｫﾙﾄｵﾌｾｯﾄ
	m_ctLoop   = m_nLoopCnt;
	SetDetailCtrl();
	return TRUE;
}

void CDxfAutoWorkingDlg::OnOK() 
{
	UpdateData();
	if ( m_ctOffset <= 0 ) {
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_ctOffset.SetFocus();
		m_ctOffset.SetSel(0, -1);
		return;
	}
	if ( m_ctLoop <= 0 ) {
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_ctLoop.SetFocus();
		m_ctLoop.SetSel(0, -1);
		return;
	}
	m_dOffset  = m_ctOffset;
	m_nLoopCnt = m_ctLoop;

	EndDialog(IDOK);
}

void CDxfAutoWorkingDlg::OnBnClickedSelect()
{
	UpdateData();
	SetDetailCtrl();
}
