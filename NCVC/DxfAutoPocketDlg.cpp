// DxfAutoPocketDlg.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "DXFDoc.h"
#include "DxfAutoPocketDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CDxfAutoPocketgDlg, CDialogEx)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDxfAutoPocketgDlg ダイアログ

CDxfAutoPocketgDlg::CDxfAutoPocketgDlg(LPAUTOWORKINGDATA pAuto)
	: CDialogEx(CDxfAutoPocketgDlg::IDD, NULL)
{
	m_pAuto = pAuto;
	m_bAcuteRound	= pAuto->bAcuteRound;
	m_nScan			= pAuto->nScanLine;
	m_bCircle		= pAuto->bCircleScroll;
}

void CDxfAutoPocketgDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDxfAutoPocketgDlg)
	DDX_Control(pDX, IDC_AUTO_OFFSET, m_dOffset);
	DDX_Control(pDX, IDC_AUTO_LOOPCNT, m_nLoop);
	DDX_Check(pDX, IDC_AUTO_ACUTEROUND, m_bAcuteRound);
	DDX_CBIndex(pDX, IDC_AUTO_SCANLINE, m_nScan);
	DDX_Check(pDX, IDC_AUTO_CIRCLESCROLL, m_bCircle);
	//}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////
// CDxfAutoPocketgDlg メッセージ ハンドラ

BOOL CDxfAutoPocketgDlg::OnInitDialog() 
{
	__super::OnInitDialog();
	m_dOffset	= m_pAuto->dOffset;
	m_nLoop		= m_pAuto->nLoopCnt;

	return TRUE;
}

void CDxfAutoPocketgDlg::OnOK() 
{
	UpdateData();
	if ( m_dOffset <= 0.0f ) {
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_dOffset.SetFocus();
		m_dOffset.SetSel(0, -1);
		return;
	}
	if ( m_nLoop <= 0 ) {
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_nLoop.SetFocus();
		m_nLoop.SetSel(0, -1);
		return;
	}

	m_pAuto->dOffset		= m_dOffset;
	m_pAuto->nLoopCnt		= m_nLoop;
	m_pAuto->bAcuteRound	= m_bAcuteRound;
	m_pAuto->nScanLine		= m_nScan;
	m_pAuto->bCircleScroll	= m_bCircle;

	EndDialog(IDOK);
}
