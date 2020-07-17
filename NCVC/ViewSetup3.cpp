// ViewSetup3.cpp : �C���v�������e�[�V���� �t�@�C��
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

BEGIN_MESSAGE_MAP(CViewSetup3, CPropertyPage)
	//{{AFX_MSG_MAP(CViewSetup3)
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_VIEWSETUP1_DEFCOLOR, &CViewSetup3::OnDefColor)
	ON_BN_CLICKED(IDC_VIEWSETUP3_BT_BACKGROUND1, &CViewSetup3::OnColorButton)
	ON_BN_CLICKED(IDC_VIEWSETUP3_BT_BACKGROUND2, &CViewSetup3::OnColorButton)
	ON_BN_CLICKED(IDC_VIEWSETUP3_BT_ORIGIN, &CViewSetup3::OnColorButton)
	ON_BN_CLICKED(IDC_VIEWSETUP3_BT_CUTTER, &CViewSetup3::OnColorButton)
	ON_BN_CLICKED(IDC_VIEWSETUP3_BT_START, &CViewSetup3::OnColorButton)
	ON_BN_CLICKED(IDC_VIEWSETUP3_BT_MOVE, &CViewSetup3::OnColorButton)
	ON_BN_CLICKED(IDC_VIEWSETUP3_BT_TEXT, &CViewSetup3::OnColorButton)
	ON_BN_CLICKED(IDC_VIEWSETUP3_BT_OUTLINE, &CViewSetup3::OnColorButton)
	ON_BN_CLICKED(IDC_VIEWSETUP3_BT_WORK, &CViewSetup3::OnColorButton)
	ON_BN_CLICKED(IDC_VIEWSETUP3_FONT, &CViewSetup3::OnFontChange)
	ON_CBN_SELCHANGE(IDC_VIEWSETUP3_CB_ORIGIN, &CViewSetup3::OnChange)
	ON_CBN_SELCHANGE(IDC_VIEWSETUP3_CB_CUTTER, &CViewSetup3::OnChange)
	ON_CBN_SELCHANGE(IDC_VIEWSETUP3_CB_START, &CViewSetup3::OnChange)
	ON_CBN_SELCHANGE(IDC_VIEWSETUP3_CB_MOVE, &CViewSetup3::OnChange)
	ON_CBN_SELCHANGE(IDC_VIEWSETUP3_CB_OUTLINE, &CViewSetup3::OnChange)
	ON_CBN_SELCHANGE(IDC_VIEWSETUP3_CB_WORK, &CViewSetup3::OnChange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

#define	GetParentSheet()	static_cast<CViewSetup *>(GetParentSheet())

/////////////////////////////////////////////////////////////////////////////
// CViewSetup3 �v���p�e�B �y�[�W

CViewSetup3::CViewSetup3() : CPropertyPage(CViewSetup3::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;
	//{{AFX_DATA_INIT(CViewSetup3)
		// ���� - ClassWizard �͂��̈ʒu�Ƀ����o�̏�����������ǉ����܂��B
	//}}AFX_DATA_INIT
	CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();	// Can't const(GetLogFont)
	for ( int i=0; i<SIZEOF(m_colView); i++ ) {
		m_colView[i] = pOpt->m_colDXFView[i];
		m_brColor[i].CreateSolidBrush( m_colView[i] );
	}
	// MM_LOMETRIC �ւ�̫�đ����̂��߁C�ϊ����K�v
	memcpy(&m_lfFont, pOpt->GetLogFont(TYPE_DXF), sizeof(m_lfFont));
	CClientDC	dc(AfxGetMainWnd());
	dc.SetMapMode(MM_LOMETRIC);
	CPoint		pt(0, m_lfFont.lfHeight);
	dc.LPtoDP(&pt);
	m_lfFont.lfHeight = -abs(pt.y);
}

CViewSetup3::~CViewSetup3()
{
	for ( int i=0; i<SIZEOF(m_brColor); i++ )
		m_brColor[i].DeleteObject();
}

void CViewSetup3::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CViewSetup3)
	DDX_Control(pDX, IDC_VIEWSETUP3_FONT, m_ctFontButton);
	//}}AFX_DATA_MAP
	int		i;
	for ( i=0; i<SIZEOF(m_cbLineType); i++ )
		DDX_Control(pDX, i + IDC_VIEWSETUP3_CB_ORIGIN, m_cbLineType[i]);
	for ( i=0; i<SIZEOF(m_ctColor); i++ )
		DDX_Control(pDX, i + IDC_VIEWSETUP3_ST_BACKGROUND1, m_ctColor[i]);
}

/////////////////////////////////////////////////////////////////////////////
// CViewSetup3 ���b�Z�[�W �n���h��

BOOL CViewSetup3::OnInitDialog() 
{
	__super::OnInitDialog();

	// ����÷�Ă̕ύX
	m_ctFontButton.SetWindowText(
		GetParentSheet()->GetChangeFontButtonText(&m_lfFont) );

	// ���������߂̑I�����o�^
	extern	const	PENSTYLE	g_penStyle[];
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	int		i, j;
	for ( i=0; i<SIZEOF(m_cbLineType); i++ ) {
		for ( j=0; j<MAXPENSTYLE; j++ )
			m_cbLineType[i].AddString(g_penStyle[j].lpszPenName);
		m_cbLineType[i].SetCurSel(pOpt->GetDxfDrawType(i));
	}

	return TRUE;  // �R���g���[���Ƀt�H�[�J�X��ݒ肵�Ȃ��Ƃ��A�߂�l�� TRUE �ƂȂ�܂�
	              // ��O: OCX �v���p�e�B �y�[�W�̖߂�l�� FALSE �ƂȂ�܂�
}

BOOL CViewSetup3::OnApply() 
{
	int		i;
	CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();

	// MM_LOMETRIC �ւ�̫�đ����̂��߁C�ϊ����K�v
	memcpy(&(pOpt->m_lfFont[TYPE_DXF]), &m_lfFont, sizeof(m_lfFont));
	CClientDC	dc(AfxGetMainWnd());
	dc.SetMapMode(MM_LOMETRIC);
	CPoint	pt(0, m_lfFont.lfHeight);
	dc.DPtoLP(&pt);
	pOpt->m_lfFont[TYPE_DXF].lfHeight = -abs(pt.y);

	for ( i=0; i<SIZEOF(m_colView); i++ )
		pOpt->m_colDXFView[i] = m_colView[i];
	for ( i=0; i<SIZEOF(m_cbLineType); i++ )
		pOpt->m_nDXFLineType[i] = m_cbLineType[i].GetCurSel();

	SetModified(FALSE);

	return TRUE;
}

HBRUSH CViewSetup3::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	if ( nCtlColor == CTLCOLOR_STATIC ) {
		int	nID = pWnd->GetDlgCtrlID();
		if ( IDC_VIEWSETUP3_ST_BACKGROUND1<=nID && nID<=IDC_VIEWSETUP3_ST_WORK )
			return m_brColor[nID-IDC_VIEWSETUP3_ST_BACKGROUND1];
	}
	return __super::OnCtlColor(pDC, pWnd, nCtlColor);
}

void CViewSetup3::OnColorButton() 
{
	int	nIndex = GetFocus()->GetDlgCtrlID() - IDC_VIEWSETUP3_BT_BACKGROUND1;
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

void CViewSetup3::OnFontChange() 
{
	CFontDialog	fontDlg(&m_lfFont, CF_SCREENFONTS|CF_NOVERTFONTS);
	if ( fontDlg.DoModal() == IDOK ) {
		memcpy(&m_lfFont, fontDlg.m_cf.lpLogFont, sizeof(m_lfFont));
		m_ctFontButton.SetWindowText(
			GetParentSheet()->GetChangeFontButtonText(&m_lfFont) );
		// �K�p���݂�L����
		SetModified();
	}
}

void CViewSetup3::OnChange() 
{
	SetModified();
}

void CViewSetup3::OnDefColor() 
{
	extern	LPCTSTR			g_szDxfViewColDef[];
	extern	const	int		g_nDxfViewLineTypeDef[];
	int		i;
	COLORREF	clr;
	BOOL		bChange = FALSE;

	for ( i=0; i<SIZEOF(m_colView); i++ ) {
		clr = ConvertSTRtoRGB(g_szDxfViewColDef[i]);
		if ( m_colView[i] != clr ) {
			m_colView[i] = clr;
			m_brColor[i].DeleteObject();
			m_brColor[i].CreateSolidBrush( m_colView[i] );
			m_ctColor[i].Invalidate();
			bChange = TRUE;
		}
	}
	for ( i=0; i<SIZEOF(m_cbLineType); i++ ) {
		if ( m_cbLineType[i].GetCurSel() != g_nDxfViewLineTypeDef[i] ) {
			m_cbLineType[i].SetCurSel(g_nDxfViewLineTypeDef[i]);
			bChange = TRUE;
		}
	}

	if ( bChange )
		SetModified();
}
