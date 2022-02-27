// 3dScanSetupDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "3dScanSetupDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(C3dScanSetupDlg, CDialog)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// C3dScanSetupDlg ダイアログ

C3dScanSetupDlg::C3dScanSetupDlg(CWnd*) : CDialog(C3dScanSetupDlg::IDD, NULL)
{
	m_bOrigin = TRUE;
}

void C3dScanSetupDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_3DSCAN_BALLENDMILL, m_dBallEndmill);
	DDX_Control(pDX, IDC_3DSCAN_HEIGHT, m_dHeight);
	DDX_Control(pDX, IDC_3DSCAN_ZCUT, m_dZCut);
	DDX_Control(pDX, IDC_3DSCAN_LINESPLIT, m_nLineSplit);
	DDX_Check(pDX, IDC_3DSCAN_ORIGIN, m_bOrigin);
}

/////////////////////////////////////////////////////////////////////////////
// C3dScanSetupDlg メッセージ ハンドラー

BOOL C3dScanSetupDlg::OnInitDialog() 
{
	__super::OnInitDialog();

	return TRUE;
}

void C3dScanSetupDlg::OnOK()
{
	m.dBallEndmill	= m_dBallEndmill;
	m.dHeight		= m_dHeight;
	m.dZCut			= m_dZCut;
	m.nLineSplit	= m_nLineSplit;
	m.bOrigin		= m_bOrigin;
}
