// ViewSetup2.cpp : インプリメンテーション ファイル
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

extern	const	double	g_dDefaultGuideLength;	// 50.0 (ViewOption.cpp)

BEGIN_MESSAGE_MAP(CViewSetup2, CPropertyPage)
	//{{AFX_MSG_MAP(CViewSetup2)
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_VIEWSETUP1_DEFCOLOR, &CViewSetup2::OnDefColor)
	ON_BN_CLICKED(IDC_VIEWSETUP2_DRAWREVISE, &CViewSetup2::OnChange)
	ON_BN_CLICKED(IDC_VIEWSETUP2_DRAWCIRCLECENTER, &CViewSetup2::OnChange)
	ON_BN_CLICKED(IDC_VIEWSETUP2_BT_BACKGROUND1, &CViewSetup2::OnColorButton)
	ON_BN_CLICKED(IDC_VIEWSETUP2_SCALE, &CViewSetup2::OnScale)
	ON_BN_CLICKED(IDC_VIEWSETUP2_GUIDE, &CViewSetup2::OnChange)
	ON_BN_CLICKED(IDC_VIEWSETUP2_BT_BACKGROUND2, &CViewSetup2::OnColorButton)
	ON_BN_CLICKED(IDC_VIEWSETUP2_BT_PANE, &CViewSetup2::OnColorButton)
	ON_BN_CLICKED(IDC_VIEWSETUP2_BT_X, &CViewSetup2::OnColorButton)
	ON_BN_CLICKED(IDC_VIEWSETUP2_BT_Y, &CViewSetup2::OnColorButton)
	ON_BN_CLICKED(IDC_VIEWSETUP2_BT_Z, &CViewSetup2::OnColorButton)
	ON_BN_CLICKED(IDC_VIEWSETUP2_BT_G0, &CViewSetup2::OnColorButton)
	ON_BN_CLICKED(IDC_VIEWSETUP2_BT_G1, &CViewSetup2::OnColorButton)
	ON_BN_CLICKED(IDC_VIEWSETUP2_BT_G1Z, &CViewSetup2::OnColorButton)
	ON_BN_CLICKED(IDC_VIEWSETUP2_BT_CORRECT, &CViewSetup2::OnColorButton)
	ON_BN_CLICKED(IDC_VIEWSETUP2_BT_CYCLE, &CViewSetup2::OnColorButton)
	ON_BN_CLICKED(IDC_VIEWSETUP2_BT_CENTERCIRCLE, &CViewSetup2::OnColorButton)
	ON_BN_CLICKED(IDC_VIEWSETUP2_BT_WORK, &CViewSetup2::OnColorButton)
	ON_BN_CLICKED(IDC_VIEWSETUP2_BT_MAXCUT, &CViewSetup2::OnColorButton)
	ON_CBN_SELCHANGE(IDC_VIEWSETUP2_CB_X, &CViewSetup2::OnChange)
	ON_CBN_SELCHANGE(IDC_VIEWSETUP2_CB_Y, &CViewSetup2::OnChange)
	ON_CBN_SELCHANGE(IDC_VIEWSETUP2_CB_Z, &CViewSetup2::OnChange)
	ON_CBN_SELCHANGE(IDC_VIEWSETUP2_CB_G0, &CViewSetup2::OnChange)
	ON_CBN_SELCHANGE(IDC_VIEWSETUP2_CB_G1, &CViewSetup2::OnChange)
	ON_CBN_SELCHANGE(IDC_VIEWSETUP2_CB_G1Z, &CViewSetup2::OnChange)
	ON_CBN_SELCHANGE(IDC_VIEWSETUP2_CB_CYCLE, &CViewSetup2::OnChange)
	ON_CBN_SELCHANGE(IDC_VIEWSETUP2_CB_WORK, &CViewSetup2::OnChange)
	ON_CBN_SELCHANGE(IDC_VIEWSETUP2_CB_MAXCUT, &CViewSetup2::OnChange)
	ON_EN_CHANGE(IDC_VIEWSETUP2_X, &CViewSetup2::OnChange)
	ON_EN_CHANGE(IDC_VIEWSETUP2_Y, &CViewSetup2::OnChange)
	ON_EN_CHANGE(IDC_VIEWSETUP2_Z, &CViewSetup2::OnChange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CViewSetup2 プロパティ ページ

CViewSetup2::CViewSetup2() : CPropertyPage(CViewSetup2::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;
	//{{AFX_DATA_INIT(CViewSetup2)
	//}}AFX_DATA_INIT
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	m_bDraw[0] = pOpt->m_bDrawRevise;
	m_bDraw[1] = pOpt->m_bDrawCircleCenter;
	m_bGuide[0] = pOpt->m_bScale;
	m_bGuide[1] = pOpt->m_bGuide;
	for ( int i=0; i<SIZEOF(m_colView); i++ ) {
		m_colView[i] = pOpt->m_colNCView[i];
		m_brColor[i].CreateSolidBrush( m_colView[i] );
	}
}

CViewSetup2::~CViewSetup2()
{
	for ( int i=0; i<SIZEOF(m_brColor); i++ )
		m_brColor[i].DeleteObject();
}

void CViewSetup2::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CViewSetup2)
	DDX_Control(pDX, IDC_VIEWSETUP2_GUIDE, m_ctGuide);
	//}}AFX_DATA_MAP
	int		i;
	for ( i=0; i<SIZEOF(m_bDraw); i++ )
		DDX_Check(pDX, i+IDC_VIEWSETUP2_DRAWREVISE, m_bDraw[i]);
	for ( i=0; i<SIZEOF(m_bGuide); i++ )
		DDX_Check(pDX, i+IDC_VIEWSETUP2_SCALE, m_bGuide[i]);
	for ( i=0; i<SIZEOF(m_cbLineType); i++ )
		DDX_Control(pDX, i+IDC_VIEWSETUP2_CB_X, m_cbLineType[i]);
	for ( i=0; i<SIZEOF(m_ctColor); i++ )
		DDX_Control(pDX, i + IDC_VIEWSETUP2_ST_BACKGROUND1, m_ctColor[i]);
	for ( i=0; i<NCXYZ; i++ )
		DDX_Control(pDX, i+IDC_VIEWSETUP2_X, m_dGuide[i]);
}

void CViewSetup2::EnableControl(void)
{
	if ( m_bGuide[0] && !m_bGuide[1] ) {
		m_bGuide[1] = TRUE;		//　目盛表示は「拡大率に同期させる」が必須
		UpdateData(FALSE);
	}
	m_ctGuide.EnableWindow(!m_bGuide[0]);
	for ( int i=0; i<NCXYZ; i++ )
		m_cbLineType[i].EnableWindow(!m_bGuide[0]);
}

/////////////////////////////////////////////////////////////////////////////
// CViewSetup2 メッセージ ハンドラ

BOOL CViewSetup2::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	// 線属性ﾀｲﾌﾟの選択肢登録
	extern	const	PENSTYLE	g_penStyle[];
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	int		i, j;
	for ( i=0; i<SIZEOF(m_cbLineType); i++ ) {
		for ( j=0; j<MAXPENSTYLE; j++ )
			m_cbLineType[i].AddString(g_penStyle[j].lpszPenName);
		m_cbLineType[i].SetCurSel(pOpt->GetNcDrawType(i));
	}

	// ｶﾞｲﾄﾞの長さ
	for ( i=0; i<NCXYZ; i++ )
		m_dGuide[i] = pOpt->GetGuideLength(i);

	EnableControl();

	return TRUE;  // コントロールにフォーカスを設定しないとき、戻り値は TRUE となります
	              // 例外: OCX プロパティ ページの戻り値は FALSE となります
}

BOOL CViewSetup2::OnApply() 
{
	int		i;
	CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();

	if ( pOpt->m_bDrawRevise != m_bDraw[0] )
		pOpt->m_dwUpdateFlg |= VIEWUPDATE_DISPLAYLIST;
	pOpt->m_bDrawRevise = m_bDraw[0];
	if ( pOpt->m_bDrawCircleCenter != m_bDraw[1] )
		pOpt->m_dwUpdateFlg |= VIEWUPDATE_DISPLAYLIST;
	pOpt->m_bDrawCircleCenter = m_bDraw[1];
	pOpt->m_bScale = m_bGuide[0];
	pOpt->m_bGuide = m_bGuide[1];
	for ( i=0; i<SIZEOF(m_colView); i++ ) {
		if ( pOpt->m_colNCView[i] != m_colView[i] )
			pOpt->m_dwUpdateFlg |= VIEWUPDATE_DISPLAYLIST;
		pOpt->m_colNCView[i] = m_colView[i];
	}
	for ( i=0; i<SIZEOF(m_cbLineType); i++ ) {
		if ( pOpt->m_nNCLineType[i] != m_cbLineType[i].GetCurSel() )
			pOpt->m_dwUpdateFlg |= VIEWUPDATE_DISPLAYLIST;
		pOpt->m_nNCLineType[i] = m_cbLineType[i].GetCurSel();
	}
	for ( i=0; i<NCXYZ; i++ ) {
		if ( pOpt->m_dGuide[i] != m_dGuide[i] )
			pOpt->m_dwUpdateFlg |= VIEWUPDATE_REDRAW;
		pOpt->m_dGuide[i] = m_dGuide[i];
	}

	SetModified(FALSE);

	return TRUE;
}

BOOL CViewSetup2::OnKillActive() 
{
	if ( !CPropertyPage::OnKillActive() )
		return FALSE;

	for ( int i=0; i<NCXYZ; i++ ) {
		if ( m_dGuide[i] < 0 ) {
			AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
			m_dGuide[i].SetFocus();
			m_dGuide[i].SetSel(0, -1);
			return FALSE;
		}
	}

	return TRUE;
}

HBRUSH CViewSetup2::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	if ( nCtlColor == CTLCOLOR_STATIC ) {
		int	nID = pWnd->GetDlgCtrlID();
		if ( IDC_VIEWSETUP2_ST_BACKGROUND1<=nID && nID<=IDC_VIEWSETUP2_ST_CORRECT )
			return m_brColor[nID-IDC_VIEWSETUP2_ST_BACKGROUND1];
	}
	return CPropertyPage::OnCtlColor(pDC, pWnd, nCtlColor);
}

void CViewSetup2::OnColorButton() 
{
	int	nIndex = GetFocus()->GetDlgCtrlID() - IDC_VIEWSETUP2_BT_BACKGROUND1;
	if ( 0<=nIndex && nIndex<SIZEOF(m_colView) ) {
		CColorDialog	dlg(m_colView[nIndex]);
		dlg.m_cc.lpCustColors = (COLORREF *)&(AfxGetNCVCApp()->GetViewOption()->m_colCustom);
		if ( dlg.DoModal() == IDOK ) {
			m_colView[nIndex] = dlg.GetColor();
			m_brColor[nIndex].DeleteObject();
			m_brColor[nIndex].CreateSolidBrush( m_colView[nIndex] );
			m_ctColor[nIndex].Invalidate();
			SetModified();
		}
	}
}

void CViewSetup2::OnChange() 
{
	SetModified();
}

void CViewSetup2::OnScale()
{
	UpdateData();
	EnableControl();
	SetModified();
}

void CViewSetup2::OnDefColor() 
{
	extern	LPCTSTR			g_szNcViewColDef[];
	extern	const	int		g_nNcViewLineTypeDef[];
	int		i;
	COLORREF	clr;
	BOOL		bChange = FALSE;

	for ( i=0; i<SIZEOF(m_colView); i++ ) {
		clr = ConvertSTRtoRGB(g_szNcViewColDef[i]);
		if ( m_colView[i] != clr ) {
			m_colView[i] = clr;
			m_brColor[i].DeleteObject();
			m_brColor[i].CreateSolidBrush( m_colView[i] );
			m_ctColor[i].Invalidate();
			bChange = TRUE;
		}
	}
	for ( i=0; i<SIZEOF(m_cbLineType); i++ ) {
		if ( m_cbLineType[i].GetCurSel() != g_nNcViewLineTypeDef[i] ) {
			m_cbLineType[i].SetCurSel(g_nNcViewLineTypeDef[i]);
			bChange = TRUE;
		}
	}
	for ( i=0; i<NCXYZ; i++ ) {
		if ( m_dGuide[i] != g_dDefaultGuideLength ) {
			m_dGuide[i] = g_dDefaultGuideLength;	// 50.0
			bChange = TRUE;
		}
	}

	if ( bChange )
		SetModified();
}
