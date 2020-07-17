// ShapePropDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"	// DXFView.h
#include "DXFdata.h"
#include "DXFshape.h"
#include "DXFView.h"	// DXFTREETYPE
#include "ShapePropDlg.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

using namespace boost;

extern	LPTSTR	gg_RootTitle[];	// from DXFShapeView.cpp

BEGIN_MESSAGE_MAP(CShapePropDlg, CDialog)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CShapePropDlg ダイアログ

CShapePropDlg::CShapePropDlg
	(DXFTREETYPE& vSelect, BOOL bChain, int nShape, optional<double>& dOffsetInit, BOOL bAcute)
		: CDialog(CShapePropDlg::IDD, NULL)
{
	m_vSelect = vSelect;
	m_nShape  = nShape;
	m_bChain  = bChain;
	m_dOffsetInit = dOffsetInit;
	m_bAcuteRound = bAcute;
}

CShapePropDlg::~CShapePropDlg()
{
}

void CShapePropDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_CBIndex(pDX, IDC_SHAPEPROP_SHAPE, m_nShape);
	DDX_Control(pDX, IDC_SHAPEPROP_SHAPE, m_ctShape);
	DDX_Control(pDX, IDC_SHAPEPROP_OFFSET, m_ctOffset);
	DDX_Control(pDX, IDC_SHAPEPROP_NAME, m_ctName);
	DDX_Text(pDX, IDC_SHAPEPROP_NAME, m_strShapeName);
	DDX_Control(pDX, IDC_SHAPEPROP_ACUTE, m_ctAcuteRound);
	DDX_Check(pDX, IDC_SHAPEPROP_ACUTE, m_bAcuteRound);
}

/////////////////////////////////////////////////////////////////////////////
// CShapePropDlg メッセージ ハンドラ

BOOL CShapePropDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	// ｺﾝﾄﾛｰﾙ初期設定
	switch ( m_vSelect.which() ) {
	case DXFTREETYPE_MUSTER:
		m_ctShape.EnableWindow(FALSE);
		// through
	case DXFTREETYPE_LAYER:
		m_ctName.EnableWindow(FALSE);
		break;
	case DXFTREETYPE_SHAPE:
		m_strShapeName = get<CDXFshape*>(m_vSelect)->GetShapeName();
		break;
	}
	if ( m_vSelect.which() != DXFTREETYPE_MUSTER ) {
		// 選択できる集合の登録
		int s = m_bChain ? 0 : 1;
		for ( int i=s; i<3; i++ )
			m_ctShape.AddString(gg_RootTitle[i]);
		// ｵﾌｾｯﾄ入力できるかどうか
		if ( !m_dOffsetInit ) {
			m_ctOffset.EnableWindow(FALSE);
			m_ctAcuteRound.EnableWindow(FALSE);
		}
	}
	if ( m_dOffsetInit && *m_dOffsetInit != HUGE_VAL )
		m_ctOffset = *m_dOffsetInit;
	UpdateData(FALSE);

	return TRUE;
}

void CShapePropDlg::OnOK() 
{
	UpdateData();
	if ( m_vSelect.which()==DXFTREETYPE_SHAPE && m_strShapeName.IsEmpty() ) {
		AfxMessageBox(IDS_ERR_SETTING, MB_OK|MB_ICONEXCLAMATION);
		m_ctName.SetFocus();
		return;
	}
	if ( m_ctOffset <= 0 ) {
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_ctOffset.SetFocus();
		m_ctOffset.SetSel(0, -1);
		return;
	}
	m_dOffset = m_ctOffset;
	EndDialog(IDOK);
}
