// ViewSetup4.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "ViewOption.h"
#include "ViewSetup.h"
#include <memory.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CViewSetup4, CPropertyPage)
	//{{AFX_MSG_MAP(CViewSetup4)
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_VIEWSETUP1_DEFCOLOR, &CViewSetup4::OnDefColor)
	ON_BN_CLICKED(IDC_VIEWSETUP4_FONT, &CViewSetup4::OnFontChange)
	ON_BN_CLICKED(IDC_VIEWSETUP4_BT_BACKGROUND1, &CViewSetup4::OnColorButton)
	ON_BN_CLICKED(IDC_VIEWSETUP4_BT_BACKGROUND2, &CViewSetup4::OnColorButton)
	ON_BN_CLICKED(IDC_VIEWSETUP4_BT_TEXT, &CViewSetup4::OnColorButton)
	ON_EN_CHANGE(IDC_VIEWSETUP4_TRACE0, &CViewSetup4::OnChange)
	ON_EN_CHANGE(IDC_VIEWSETUP4_TRACE1, &CViewSetup4::OnChange)
	ON_EN_CHANGE(IDC_VIEWSETUP4_TRACE2, &CViewSetup4::OnChange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

#define	GetParentSheet()	static_cast<CViewSetup *>(GetParentSheet())

/////////////////////////////////////////////////////////////////////////////
// CViewSetup4 �v���p�e�B �y�[�W

CViewSetup4::CViewSetup4() : CPropertyPage(CViewSetup4::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;
	//{{AFX_DATA_INIT(CViewSetup4)
	//}}AFX_DATA_INIT
	CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();	// Can't const (GetLogFont)
	for ( int i=0; i<SIZEOF(m_colView); i++ ) {
		m_colView[i] = pOpt->m_colNCInfoView[i];
		m_brColor[i].CreateSolidBrush( m_colView[i] );
	}
	memcpy(&m_lfFont, pOpt->GetLogFont(TYPE_NCD), sizeof(m_lfFont));
}

CViewSetup4::~CViewSetup4()
{
	for ( int i=0; i<SIZEOF(m_brColor); i++ )
		m_brColor[i].DeleteObject();
}

void CViewSetup4::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CViewSetup4)
	DDX_Control(pDX, IDC_VIEWSETUP4_FONT, m_ctFontButton);
	//}}AFX_DATA_MAP
	int	i;
	for ( i=0; i<SIZEOF(m_ctColor); i++ )
		DDX_Control(pDX, i + IDC_VIEWSETUP4_ST_BACKGROUND1, m_ctColor[i]);
	for ( i=0; i<SIZEOF(m_nTraceSpeed); i++ )
		DDX_Control(pDX, i+IDC_VIEWSETUP4_TRACE0, m_nTraceSpeed[i]);
}

/////////////////////////////////////////////////////////////////////////////
// CViewSetup4 ���b�Z�[�W �n���h��

BOOL CViewSetup4::OnInitDialog() 
{
	__super::OnInitDialog();

	// ����÷�Ă̕ύX
	m_ctFontButton.SetWindowText(
		GetParentSheet()->GetChangeFontButtonText(&m_lfFont) );

	// �ڰ���߰��
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	for ( int i=0; i<SIZEOF(m_nTraceSpeed); i++ )
		m_nTraceSpeed[i] = pOpt->GetTraceSpeed(i);

	return TRUE;  // �R���g���[���Ƀt�H�[�J�X��ݒ肵�Ȃ��Ƃ��A�߂�l�� TRUE �ƂȂ�܂�
	              // ��O: OCX �v���p�e�B �y�[�W�̖߂�l�� FALSE �ƂȂ�܂�
}

BOOL CViewSetup4::OnApply() 
{
	int		i;
	CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();
	memcpy(&(pOpt->m_lfFont[TYPE_NCD]), &m_lfFont, sizeof(m_lfFont));
	for ( i=0; i<SIZEOF(m_colView); i++ )
		pOpt->m_colNCInfoView[i] = m_colView[i];
	for ( i=0; i<SIZEOF(m_nTraceSpeed); i++ )
		pOpt->m_nTraceSpeed[i] = m_nTraceSpeed[i];

	SetModified(FALSE);

	return TRUE;
}

BOOL CViewSetup4::OnKillActive() 
{
	if ( !__super::OnKillActive() )
		return FALSE;

	for ( int i=0; i<SIZEOF(m_nTraceSpeed); i++ ) {
		if ( m_nTraceSpeed[i] < 0 ) {
			AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
			m_nTraceSpeed[i].SetFocus();
			m_nTraceSpeed[i].SetSel(0, -1);
			return FALSE;
		}
	}

	return TRUE;
}

HBRUSH CViewSetup4::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	if ( nCtlColor == CTLCOLOR_STATIC ) {
		int	nID = pWnd->GetDlgCtrlID();
		if ( IDC_VIEWSETUP4_ST_BACKGROUND1<=nID && nID<=IDC_VIEWSETUP4_ST_TEXT )
			return m_brColor[nID-IDC_VIEWSETUP4_ST_BACKGROUND1];
	}
	return __super::OnCtlColor(pDC, pWnd, nCtlColor);
}

void CViewSetup4::OnColorButton() 
{
	int	nIndex = GetFocus()->GetDlgCtrlID() - IDC_VIEWSETUP4_BT_BACKGROUND1;
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

void CViewSetup4::OnFontChange() 
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

void CViewSetup4::OnChange() 
{
	SetModified();
}

void CViewSetup4::OnDefColor() 
{
	extern	LPCTSTR			g_szNcInfoViewColDef[];
	extern	const	int		g_nTraceSpeed[];
	int			i;
	COLORREF	clr;
	BOOL		bChange = FALSE;

	for ( i=0; i<SIZEOF(m_colView); i++ ) {
		clr = ConvertSTRtoRGB(g_szNcInfoViewColDef[i]);
		if ( m_colView[i] != clr ) {
			m_colView[i] = clr;
			m_brColor[i].DeleteObject();
			m_brColor[i].CreateSolidBrush( m_colView[i] );
			m_ctColor[i].Invalidate();
			bChange = TRUE;
		}
	}
	for ( i=0; i<SIZEOF(m_nTraceSpeed); i++ ) {
		if ( m_nTraceSpeed[i] != g_nTraceSpeed[i] ) {
			m_nTraceSpeed[i] = g_nTraceSpeed[i];
			bChange = TRUE;
		}
	}

	if ( bChange )
		SetModified();
}
