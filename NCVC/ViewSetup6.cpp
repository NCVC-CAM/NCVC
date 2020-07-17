// ViewSetup5.cpp : 実装ファイル
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

BEGIN_MESSAGE_MAP(CViewSetup6, CPropertyPage)
	ON_BN_CLICKED(IDC_VIEWSETUP1_DEFCOLOR, &CViewSetup6::OnDefColor)
	ON_CBN_SELCHANGE(IDC_VIEWSETUP6_LT, &CViewSetup6::OnChange)
	ON_CBN_SELCHANGE(IDC_VIEWSETUP6_RT, &CViewSetup6::OnChange)
	ON_CBN_SELCHANGE(IDC_VIEWSETUP6_LB, &CViewSetup6::OnChange)
	ON_CBN_SELCHANGE(IDC_VIEWSETUP6_RB, &CViewSetup6::OnChange)
	ON_CBN_SELCHANGE(IDC_VIEWSETUP6_L1, &CViewSetup6::OnChange)
	ON_CBN_SELCHANGE(IDC_VIEWSETUP6_L2, &CViewSetup6::OnChange)
	ON_CBN_SELCHANGE(IDC_VIEWSETUP6_L3, &CViewSetup6::OnChange)
	ON_CBN_SELCHANGE(IDC_VIEWSETUP6_RR, &CViewSetup6::OnChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CViewSetup6 プロパティ ページ

CViewSetup6::CViewSetup6() : CPropertyPage(CViewSetup6::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;

	int		i;
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	for ( i=0; i<SIZEOF(m_nForceView01); i++ )
		m_nForceView01[i] = pOpt->m_nForceView01[i];
	for ( i=0; i<SIZEOF(m_nForceView02); i++ )
		m_nForceView02[i] = pOpt->m_nForceView02[i];
}

void CViewSetup6::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	int		i;
	for ( i=0; i<SIZEOF(m_nForceView01); i++ )
		DDX_CBIndex(pDX, IDC_VIEWSETUP6_LT+i, m_nForceView01[i]);
	for ( i=0; i<SIZEOF(m_nForceView02); i++ )
		DDX_CBIndex(pDX, IDC_VIEWSETUP6_L1+i, m_nForceView02[i]);
}

/////////////////////////////////////////////////////////////////////////////
// CViewSetup6 メッセージ ハンドラ

BOOL CViewSetup6::OnApply()
{
	int		i;
	CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();
	for ( i=0; i<SIZEOF(m_nForceView01); i++ )
		pOpt->m_nForceView01[i] = m_nForceView01[i];
	for ( i=0; i<SIZEOF(m_nForceView02); i++ )
		pOpt->m_nForceView02[i] = m_nForceView02[i];

	SetModified(FALSE);

	return TRUE;
}

void CViewSetup6::OnChange()
{
	SetModified();
}

void CViewSetup6::OnDefColor()
{
	extern	const	int		g_nForceView01[];
	extern	const	int		g_nForceView02[];
	int			i;
	for ( i=0; i<SIZEOF(m_nForceView01); i++ ) {
		if ( m_nForceView01[i] != g_nForceView01[i] ) {
			m_nForceView01[i] = g_nForceView01[i];
			SetModified();
		}
	}
	for ( i=0; i<SIZEOF(m_nForceView02); i++ ) {
		if ( m_nForceView02[i] != g_nForceView02[i] ) {
			m_nForceView02[i] = g_nForceView02[i];
			SetModified();
		}
	}
}
