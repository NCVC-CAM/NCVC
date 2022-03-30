// C3dContourScanSetupDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "3dOption.h"
#include "3dModelDoc.h"
#include "3dContourScanSetupDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(C3dContourScanSetupDlg, CDialog)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// C3dScanSetupDlg ダイアログ

C3dContourScanSetupDlg::C3dContourScanSetupDlg(C3dModelDoc* pDoc, CWnd*) :
	CDialog(C3dContourScanSetupDlg::IDD, NULL)
{
	m_pDoc = pDoc;
	m_bZOrigin = TRUE;
}

void C3dContourScanSetupDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_3DSCAN_BALLENDMILL, m_dBallEndmill);
	DDX_Control(pDX, IDC_3DSCAN_SPACE, m_dSpace);
	DDX_Control(pDX, IDC_3DSCAN_ZMAX, m_dZmax);
	DDX_Control(pDX, IDC_3DSCAN_ZMIN, m_dZmin);
	DDX_Control(pDX, IDC_3DSCAN_SHIFT, m_dShift);
	DDX_Check(pDX, IDC_3DSCAN_ZORIGIN, m_bZOrigin);
}

/////////////////////////////////////////////////////////////////////////////
// C3dScanSetupDlg メッセージ ハンドラー

BOOL C3dContourScanSetupDlg::OnInitDialog() 
{
	__super::OnInitDialog();

	C3dOption*	pOpt = m_pDoc->Get3dOption();

	m_dBallEndmill	= pOpt->m_dContourBallEndmill;
	m_dSpace		= pOpt->m_dContourSpace;
	m_dZmax			= pOpt->m_dContourZmax;
	m_dZmin			= pOpt->m_dContourZmin;
	m_dShift		= pOpt->m_dContourShift;
	m_bZOrigin		= pOpt->m_bContourZOrigin;

	UpdateData(FALSE);

	return TRUE;
}

void C3dContourScanSetupDlg::OnOK()
{
	UpdateData();
	if ( m_dSpace<0.1f || 2.0f<m_dSpace ) {
		AfxMessageBox(IDS_ERR_SETTING, MB_OK|MB_ICONEXCLAMATION);
		m_dSpace.SetFocus();
		m_dSpace.SetSel(0, -1);
		return;
	}
	if ( m_dShift <= 0.0f ) {
		// これがマイナスだと処理が無限ループ
		AfxMessageBox(IDS_ERR_SETTING, MB_OK|MB_ICONEXCLAMATION);
		m_dShift.SetFocus();
		m_dShift.SetSel(0, -1);
		return;
	}
	if ( m_dShift < m_dSpace*2.0 ) {
		// 近接点を検索するためのマージン
		AfxMessageBox(IDS_ERR_CONTOUR, MB_OK|MB_ICONEXCLAMATION);
		m_dShift.SetFocus();
		m_dShift.SetSel(0, -1);
		return;
	}

	C3dOption*	pOpt = m_pDoc->Get3dOption();

	if ( pOpt->m_bRoughZOrigin && m_bZOrigin ) {
		// 荒加工，仕上げ，ともにZ軸を補正する場合で
		// 補正値が違うとき
		if ( pOpt->m_dWorkHeight != m_dZmax ) {
			if ( AfxMessageBox(IDS_ANA_CONTOUR_ZOFFSET, MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2) != IDYES )
				return;
		}
	}

	pOpt->m_dContourBallEndmill	= m_dBallEndmill;
	pOpt->m_dContourSpace		= m_dSpace;
	pOpt->m_dContourZmax		= m_dZmax;
	pOpt->m_dContourZmin		= m_dZmin;
	pOpt->m_dContourShift		= m_dShift;
	pOpt->m_bContourZOrigin		= m_bZOrigin;

	EndDialog(IDOK);
}
