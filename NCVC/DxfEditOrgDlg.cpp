// DxfEditOrgDlg.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "DxfEditOrgDlg.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CDxfEditOrgDlg, CDialog)
	//{{AFX_MSG_MAP(CDxfEditOrgDlg)
	ON_BN_CLICKED(IDC_DXFEDIT_SEL_NUM, OnSelect)
	ON_BN_CLICKED(IDC_DXFEDIT_SEL_RECT, OnSelect)
	ON_BN_CLICKED(IDC_DXFEDIT_SEL_ORIG, OnSelect)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*
	選択情報：小さい情報のため，(ﾚｼﾞｽﾄﾘへの)保存はしない
		NCVC稼働中に限り保持する
*/
static	int		ss_nSelect = 0;
static	int		ss_nRectType = 0;
static	CString	ss_strNumeric;

/////////////////////////////////////////////////////////////////////////////
// CDxfEditOrgDlg ダイアログ

CDxfEditOrgDlg::CDxfEditOrgDlg(DWORD dwControl)
	: CDialog(CDxfEditOrgDlg::IDD, NULL)
{
	m_dwControl = dwControl;
	//{{AFX_DATA_INIT(CDxfEditOrgDlg)
	//}}AFX_DATA_INIT
	if ( m_dwControl & EDITORG_NUMERIC )
		m_nSelect = 1;	// 矩形指示
	else
		m_nSelect = ss_nSelect;
	m_strNumeric = ss_strNumeric;
	m_nRectType = ss_nRectType;
}

void CDxfEditOrgDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDxfEditOrgDlg)
	DDX_Control(pDX, IDC_DXFEDIT_SEL_NUM, m_ctSelNumeric);
	DDX_Control(pDX, IDC_DXFEDIT_SEL_ORIG, m_ctSelOriginal);
	DDX_Control(pDX, IDC_DXF_ORGTYPE, m_ctRectType);
	DDX_Control(pDX, IDC_DXFEDIT_NUM, m_ctNumeric);
	DDX_Radio(pDX, IDC_DXFEDIT_SEL_NUM, m_nSelect);
	DDX_Text(pDX, IDC_DXFEDIT_NUM, m_strNumeric);
	DDX_CBIndex(pDX, IDC_DXF_ORGTYPE, m_nRectType);
	//}}AFX_DATA_MAP
}

void CDxfEditOrgDlg::SelectControl(int nIndex)
{
	BOOL	bNumeric, bRectType;

	switch ( nIndex ) {
	case 0:		// IDC_DXFEDIT_SEL_NUM
		bNumeric  = TRUE;
		bRectType = FALSE;
		break;
	case 1:		// IDC_DXF_ORGTYPE
		bNumeric  = FALSE;
		bRectType = TRUE;
		break;
	default:
		bNumeric  = FALSE;
		bRectType = FALSE;
		break;
	}
	m_ctNumeric.EnableWindow(bNumeric);
	m_ctRectType.EnableWindow(bRectType);
}

/////////////////////////////////////////////////////////////////////////////
// CDxfEditOrgDlg メッセージ ハンドラ

BOOL CDxfEditOrgDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	SelectControl( m_nSelect );

	if ( m_dwControl & EDITORG_NUMERIC ) {
		m_ctSelNumeric.EnableWindow(FALSE);
		m_ctNumeric.EnableWindow(FALSE);
	}
	if ( m_dwControl & EDITORG_ORIGINAL )
		m_ctSelOriginal.EnableWindow(FALSE);

	return TRUE;  // コントロールにフォーカスを設定しないとき、戻り値は TRUE となります
	              // 例外: OCX プロパティ ページの戻り値は FALSE となります
}

void CDxfEditOrgDlg::OnSelect() 
{
	SelectControl( GetFocus()->GetDlgCtrlID() - IDC_DXFEDIT_SEL_NUM );
}

void CDxfEditOrgDlg::OnOK() 
{
	UpdateData();

	// 静的変数に記憶
	ss_nSelect = m_nSelect;
	ss_strNumeric = m_strNumeric;
	ss_nRectType = m_nRectType;

//	CDialog::OnOK();
	EndDialog(IDOK);
}
