﻿// 3dScanSetupDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "3dOption.h"
#include "3dModelDoc.h"
#include "3dScanSetupDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(C3dScanSetupDlg, CDialog)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// C3dScanSetupDlg ダイアログ

C3dScanSetupDlg::C3dScanSetupDlg(C3dModelDoc* pDoc, CWnd*) : CDialog(C3dScanSetupDlg::IDD, NULL)
{
	m_pDoc = pDoc;
	m_bZOrigin = TRUE;
}

void C3dScanSetupDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_3DSCAN_BALLENDMILL, m_dBallEndmill);
	DDX_Control(pDX, IDC_3DSCAN_HEIGHT, m_dHeight);
	DDX_Control(pDX, IDC_3DSCAN_ZCUT, m_dZCut);
	DDX_Control(pDX, IDC_3DSCAN_LINESPLIT, m_nLineSplit);
	DDX_Check(pDX, IDC_3DSCAN_ZORIGIN, m_bZOrigin);
}

/////////////////////////////////////////////////////////////////////////////
// C3dScanSetupDlg メッセージ ハンドラー

BOOL C3dScanSetupDlg::OnInitDialog() 
{
	__super::OnInitDialog();

	C3dOption*	pOpt = m_pDoc->Get3dOption();

	m_dBallEndmill	= pOpt->m_dBallEndmill;
	m_dHeight		= pOpt->m_dWorkHeight;
	m_dZCut			= pOpt->m_dZCut;
	m_nLineSplit	= pOpt->m_nLineSplit;
	m_bZOrigin		= pOpt->m_bZOrigin;

	UpdateData(FALSE);

	return TRUE;
}

void C3dScanSetupDlg::OnOK()
{
	UpdateData();
	if ( m_nLineSplit >= 100 ) {
		AfxMessageBox(IDS_ERR_SETTING, MB_OK|MB_ICONEXCLAMATION);
		m_nLineSplit.SetFocus();
		m_nLineSplit.SetSel(0, -1);
		return;
	}

	C3dOption*	pOpt = m_pDoc->Get3dOption();

	pOpt->m_dBallEndmill	= m_dBallEndmill;
	pOpt->m_dWorkHeight		= m_dHeight;
	pOpt->m_dZCut			= m_dZCut;
	pOpt->m_nLineSplit		= m_nLineSplit;
	pOpt->m_bZOrigin		= m_bZOrigin;

	EndDialog(IDOK);
}
