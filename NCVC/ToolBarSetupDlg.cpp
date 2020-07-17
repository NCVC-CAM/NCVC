// ToolBarSetupDlg.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "resource.h"
#include "MainFrm.h"
#include "ToolBarSetupDlg.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CToolBarSetupDlg, CDialog)
	//{{AFX_MSG_MAP(CToolBarSetupDlg)
	ON_BN_CLICKED(IDC_TOOLBAR_C1, &CToolBarSetupDlg::OnToolBarCustomize)
	ON_BN_CLICKED(IDC_TOOLBAR_C2, &CToolBarSetupDlg::OnToolBarCustomize)
	ON_BN_CLICKED(IDC_TOOLBAR_C3, &CToolBarSetupDlg::OnToolBarCustomize)
	ON_BN_CLICKED(IDC_TOOLBAR_C4, &CToolBarSetupDlg::OnToolBarCustomize)
	ON_BN_CLICKED(IDC_TOOLBAR_C5, &CToolBarSetupDlg::OnToolBarCustomize)
	ON_BN_CLICKED(IDC_TOOLBAR_C6, &CToolBarSetupDlg::OnToolBarCustomize)
	ON_BN_CLICKED(IDC_TOOLBAR_C7, &CToolBarSetupDlg::OnToolBarCustomize)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CToolBarSetupDlg �_�C�A���O

CToolBarSetupDlg::CToolBarSetupDlg(DWORD dwViewFlg)
	: CDialog(CToolBarSetupDlg::IDD)
{
	//{{AFX_DATA_INIT(CToolBarSetupDlg)
	//}}AFX_DATA_INIT

	// dwViewFlg �ޯēW�J
	for ( int i=0; i<SIZEOF(m_bToolBar); i++ )
		m_bToolBar[i] = (dwViewFlg & (1 << i)) ? TRUE : FALSE;
}

void CToolBarSetupDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CToolBarSetupDlg)
	//}}AFX_DATA_MAP
	for ( int i=0; i<SIZEOF(m_bToolBar); i++ )
		DDX_Check(pDX, i+IDC_TOOLBAR_1, m_bToolBar[i]);
}

/////////////////////////////////////////////////////////////////////////////
// CToolBarSetupDlg ���b�Z�[�W �n���h��

void CToolBarSetupDlg::OnToolBarCustomize() 
{
	static_cast<CMainFrame *>(GetParent())->
		CustomizedToolBar( GetFocus()->GetDlgCtrlID() - IDC_TOOLBAR_C1 );
}
