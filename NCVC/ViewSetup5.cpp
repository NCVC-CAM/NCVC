// ViewSetup5.cpp : �����t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "ViewOption.h"
#include "ViewSetup5.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CViewSetup5, CPropertyPage)
	ON_BN_CLICKED(IDC_VIEWSETUP5_SOLIDVIEW, OnSolidClick)
	ON_BN_CLICKED(IDC_VIEWSETUP5_G00VIEW, OnChange)
	ON_BN_CLICKED(IDC_VIEWSETUP5_DRAGRENDER, OnChange)
	ON_BN_CLICKED(IDC_VIEWSETUP5_MILL_T, OnChange)
	ON_BN_CLICKED(IDC_VIEWSETUP5_MILL_C, OnChange)
	ON_CBN_SELCHANGE(IDC_VIEWSETUP5_MILL_TYPE, OnChange)
	ON_EN_CHANGE(IDC_VIEWSETUP5_DEFAULTENDMILL, OnChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CViewSetup5 �v���p�e�B �y�[�W

CViewSetup5::CViewSetup5() : CPropertyPage(CViewSetup5::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;

	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	m_bSolid	= pOpt->m_bSolidView;
	m_bG00View	= pOpt->m_bG00View;
	m_bDrag		= pOpt->m_bDragRender;
	m_bMillT	= pOpt->m_bMillT;
	m_bMillC	= pOpt->m_bMillC;
	m_nMillType	= pOpt->m_nMillType;
}

CViewSetup5::~CViewSetup5()
{
}

void CViewSetup5::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_VIEWSETUP5_DEFAULTENDMILL, m_dEndmill);
	DDX_Control(pDX, IDC_VIEWSETUP5_G00VIEW, m_ctG00View);
	DDX_Control(pDX, IDC_VIEWSETUP5_DRAGRENDER, m_ctDrag);
	DDX_Check(pDX, IDC_VIEWSETUP5_SOLIDVIEW, m_bSolid);
	DDX_Check(pDX, IDC_VIEWSETUP5_G00VIEW, m_bG00View);
	DDX_Check(pDX, IDC_VIEWSETUP5_DRAGRENDER, m_bDrag);
	DDX_Check(pDX, IDC_VIEWSETUP5_MILL_T, m_bMillT);
	DDX_Check(pDX, IDC_VIEWSETUP5_MILL_C, m_bMillC);
	DDX_CBIndex(pDX, IDC_VIEWSETUP5_MILL_TYPE, m_nMillType);
}

void CViewSetup5::EnableControl(void)
{
	m_ctG00View.EnableWindow(m_bSolid);
	m_ctDrag.EnableWindow(m_bSolid);
}

/////////////////////////////////////////////////////////////////////////////
// CViewSetup5 ���b�Z�[�W �n���h��

BOOL CViewSetup5::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	m_dEndmill = pOpt->m_dDefaultEndmill * 2.0;
	EnableControl();

	return TRUE;  // return TRUE unless you set the focus to a control
	// ��O : OCX �v���p�e�B �y�[�W�͕K�� FALSE ��Ԃ��܂��B
}

BOOL CViewSetup5::OnApply()
{
	CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();
	pOpt->m_bSolidView	= m_bSolid;
	pOpt->m_bG00View	= m_bG00View;
	pOpt->m_bDragRender	= m_bDrag;
	pOpt->m_bMillT		= m_bMillT;
	pOpt->m_bMillC		= m_bMillC;
	pOpt->m_nMillType	= m_nMillType;
	pOpt->m_dDefaultEndmill = m_dEndmill / 2.0;

	SetModified(FALSE);
	// Ҳ��ڰсC�e�ޭ��ւ̍X�V�ʒm
	AfxGetNCVCApp()->ChangeViewOption();

	return TRUE;
}

BOOL CViewSetup5::OnKillActive()
{
	if ( !CPropertyPage::OnKillActive() )
		return FALSE;

	if ( m_dEndmill < 0 ) {
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_dEndmill.SetFocus();
		m_dEndmill.SetSel(0, -1);
		return FALSE;
	}

	return TRUE;
}

void CViewSetup5::OnSolidClick()
{
	UpdateData();
	EnableControl();
	SetModified();
}

void CViewSetup5::OnChange()
{
	SetModified();
}
