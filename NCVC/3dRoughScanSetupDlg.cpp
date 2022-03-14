// C3dRoughScanSetupDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "3dOption.h"
#include "3dModelDoc.h"
#include "3dRoughScanSetupDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(C3dRoughScanSetupDlg, CDialog)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// C3dScanSetupDlg ダイアログ

C3dRoughScanSetupDlg::C3dRoughScanSetupDlg(C3dModelDoc* pDoc, CWnd*) :
	CDialog(C3dRoughScanSetupDlg::IDD, NULL)
{
	m_pDoc = pDoc;
	m_bZOrigin = TRUE;
}

void C3dRoughScanSetupDlg::DoDataExchange(CDataExchange* pDX)
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

BOOL C3dRoughScanSetupDlg::OnInitDialog() 
{
	__super::OnInitDialog();

	C3dOption*	pOpt = m_pDoc->Get3dOption();

	m_dBallEndmill	= pOpt->m_dRoughBallEndmill;
	m_dHeight		= pOpt->m_dWorkHeight;
	m_dZCut			= pOpt->m_dRoughZCut;
	m_nLineSplit	= pOpt->m_nLineSplit;
	m_bZOrigin		= pOpt->m_bRoughZOrigin;

	UpdateData(FALSE);

	return TRUE;
}

void C3dRoughScanSetupDlg::OnOK()
{
	UpdateData();
	if ( m_nLineSplit >= 100 ) {
		AfxMessageBox(IDS_ERR_SETTING, MB_OK|MB_ICONEXCLAMATION);
		m_nLineSplit.SetFocus();
		m_nLineSplit.SetSel(0, -1);
		return;
	}

	C3dOption*	pOpt = m_pDoc->Get3dOption();

	pOpt->m_dRoughBallEndmill	= m_dBallEndmill;
	pOpt->m_dWorkHeight			= m_dHeight;
	pOpt->m_dRoughZCut			= m_dZCut;
	pOpt->m_nLineSplit			= m_nLineSplit;
	pOpt->m_bRoughZOrigin		= m_bZOrigin;

	EndDialog(IDOK);
}
