// ViewSetup1.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "ViewOption.h"
#include "ViewSetup.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CViewSetup1, CPropertyPage)
	//{{AFX_MSG_MAP(CViewSetup1)
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_VIEWSETUP1_DEFCOLOR, OnDefColor)
	ON_BN_CLICKED(IDC_VIEWSETUP1_BT_RECT, OnColorButton)
	ON_BN_CLICKED(IDC_VIEWSETUP1_BT_SEL, OnColorButton)
	ON_BN_CLICKED(IDC_VIEWSETUP1_WHEEL, OnWheel)
	ON_BN_CLICKED(IDC_VIEWSETUP1_NtoP, OnChange)
	ON_BN_CLICKED(IDC_VIEWSETUP1_PtoN, OnChange)
	ON_CBN_SELCHANGE(IDC_VIEWSETUP1_CB_RECT, OnChange)
	ON_CBN_SELCHANGE(IDC_VIEWSETUP1_CB_SEL, OnChange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CViewSetup1 プロパティ ページ

CViewSetup1::CViewSetup1() : CPropertyPage(CViewSetup1::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;
	//{{AFX_DATA_INIT(CViewSetup1)
	//}}AFX_DATA_INIT
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	m_bMouseWheel	= pOpt->m_bMouseWheel;
	m_nWheelType	= pOpt->m_nWheelType;
	for ( int i=0; i<SIZEOF(m_colView); i++ ) {
		m_colView[i] = pOpt->m_colView[i];
		m_brColor[i].CreateSolidBrush( m_colView[i] );
	}
}

CViewSetup1::~CViewSetup1()
{
	for ( int i=0; i<SIZEOF(m_brColor); i++ )
		m_brColor[i].DeleteObject();
}

void CViewSetup1::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CViewSetup1)
	DDX_Check(pDX, IDC_VIEWSETUP1_WHEEL, m_bMouseWheel);
	DDX_Radio(pDX, IDC_VIEWSETUP1_PtoN, m_nWheelType);
	DDX_Control(pDX, IDC_VIEWSETUP1_PtoN, m_ctMouseWheel[0]);
	DDX_Control(pDX, IDC_VIEWSETUP1_NtoP, m_ctMouseWheel[1]);
	//}}AFX_DATA_MAP
	int		i;
	for ( i=0; i<SIZEOF(m_cbLineType); i++ )
		DDX_Control(pDX, i + IDC_VIEWSETUP1_CB_RECT, m_cbLineType[i]);
	for ( i=0; i<SIZEOF(m_ctColor); i++ )
		DDX_Control(pDX, i + IDC_VIEWSETUP1_ST_RECT, m_ctColor[i]);
}

void CViewSetup1::EnableControl(void)
{
	m_ctMouseWheel[0].EnableWindow(m_bMouseWheel);
	m_ctMouseWheel[1].EnableWindow(m_bMouseWheel);
}

/////////////////////////////////////////////////////////////////////////////
// CViewSetup1 メッセージ ハンドラ

BOOL CViewSetup1::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	// 線属性ﾀｲﾌﾟの選択肢登録
	extern	const	PENSTYLE	g_penStyle[];
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	int		i, j;
	for ( i=0; i<SIZEOF(m_cbLineType); i++ ) {
		for ( j=0; j<MAXPENSTYLE; j++ )
			m_cbLineType[i].AddString(g_penStyle[j].lpszPenName);
		m_cbLineType[i].SetCurSel(pOpt->GetDrawType(i));
	}

	EnableControl();

	return TRUE;  // コントロールにフォーカスを設定しないとき、戻り値は TRUE となります
	              // 例外: OCX プロパティ ページの戻り値は FALSE となります
}

BOOL CViewSetup1::OnApply() 
{
	int		i;
	CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();
	pOpt->m_bMouseWheel	= m_bMouseWheel;
	pOpt->m_nWheelType	= m_nWheelType;
	for ( i=0; i<SIZEOF(m_colView); i++ )
		pOpt->m_colView[i] = m_colView[i];
	for ( i=0; i<SIZEOF(m_cbLineType); i++ )
		pOpt->m_nLineType[i] = m_cbLineType[i].GetCurSel();

	SetModified(FALSE);

	return TRUE;
}

HBRUSH CViewSetup1::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	if ( nCtlColor == CTLCOLOR_STATIC ) {
		int	nID = pWnd->GetDlgCtrlID();
		if ( IDC_VIEWSETUP1_ST_RECT<=nID && nID<=IDC_VIEWSETUP1_ST_SEL )
			return m_brColor[nID-IDC_VIEWSETUP1_ST_RECT];
	}
	return CPropertyPage::OnCtlColor(pDC, pWnd, nCtlColor);
}

void CViewSetup1::OnChange() 
{
	SetModified();
}

void CViewSetup1::OnWheel() 
{
	UpdateData();
	EnableControl();
	SetModified();
}

void CViewSetup1::OnColorButton() 
{
	int	nIndex = GetFocus()->GetDlgCtrlID() - IDC_VIEWSETUP1_BT_RECT;
	if ( 0<=nIndex && nIndex<SIZEOF(m_colView) ) {
		CColorDialog	dlg(m_colView[nIndex]);
		dlg.m_cc.lpCustColors = AfxGetNCVCApp()->GetViewOption()->GetCustomColor();
		if ( dlg.DoModal() == IDOK ) {
			m_colView[nIndex] = dlg.GetColor();
			m_brColor[nIndex].DeleteObject();
			m_brColor[nIndex].CreateSolidBrush( m_colView[nIndex] );
			m_ctColor[nIndex].Invalidate();
			SetModified();
		}
	}
}

void CViewSetup1::OnDefColor() 
{
	extern	LPCTSTR			g_szViewColDef[];
	extern	const	int		g_nViewLineTypeDef[];
	int		i;
	COLORREF	clr;
	BOOL		bChange = FALSE;

	for ( i=0; i<SIZEOF(m_colView); i++ ) {
		clr = ConvertSTRtoRGB(g_szViewColDef[i]);
		if ( m_colView[i] != clr ) {
			m_colView[i] = clr;
			m_brColor[i].DeleteObject();
			m_brColor[i].CreateSolidBrush( m_colView[i] );
			m_ctColor[i].Invalidate();
			bChange = TRUE;
		}
	}
	for ( i=0; i<SIZEOF(m_cbLineType); i++ ) {
		if ( m_cbLineType[i].GetCurSel() != g_nViewLineTypeDef[i] ) {
			m_cbLineType[i].SetCurSel(g_nViewLineTypeDef[i]);
			bChange = TRUE;
		}
	}

	if ( bChange )
		SetModified();
}
