// DxfAutoOutlineDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "DXFDoc.h"
#include "DxfAutoOutlineDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CDxfAutoOutlineDlg, CDialogEx)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDxfAutoOutlineDlg ダイアログ

CDxfAutoOutlineDlg::CDxfAutoOutlineDlg(LPAUTOWORKINGDATA pAuto)
	: CDialogEx(CDxfAutoOutlineDlg::IDD, NULL)
{
	m_pAuto = pAuto;
}

void CDxfAutoOutlineDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_AUTO_OFFSET, m_dOffset);
	DDX_Control(pDX, IDC_AUTO_LOOPCNT, m_nLoop);
	DDX_Control(pDX, IDC_AUTO_GATE, m_dGate);
	DDX_Control(pDX, IDC_AUTO_APPROACH, m_dApproach);
}

/////////////////////////////////////////////////////////////////////////////
// CDxfAutoOutlineDlg メッセージ ハンドラー

BOOL CDxfAutoOutlineDlg::OnInitDialog() 
{
	__super::OnInitDialog();
	m_dOffset	= m_pAuto->dOffset;
	m_nLoop		= m_pAuto->nLoopCnt;
	m_dGate		= m_pAuto->dGate;
	m_dApproach	= m_pAuto->dApproach;

	return TRUE;
}

void CDxfAutoOutlineDlg::OnOK() 
{
	UpdateData();
	if ( m_dOffset < 0.0f ) {	// 輪郭はゼロOK
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

	m_pAuto->dOffset	= m_dOffset;
	m_pAuto->nLoopCnt	= m_nLoop;
	m_pAuto->dGate		= m_dGate;
	m_pAuto->dApproach	= m_dApproach;

	EndDialog(IDOK);
}
