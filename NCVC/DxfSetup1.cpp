// DxfSetup1.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "DXFOption.h"
#include "DxfSetup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace boost;

BEGIN_MESSAGE_MAP(CDxfSetup1, CPropertyPage)
	//{{AFX_MSG_MAP(CDxfSetup1)
	ON_BN_CLICKED(IDC_DXF_RELOAD, &CDxfSetup1::OnReload)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDxfSetup1 �v���p�e�B �y�[�W

CDxfSetup1::CDxfSetup1() : CPropertyPage(CDxfSetup1::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;
	//{{AFX_DATA_INIT(CDxfSetup1)
	//}}AFX_DATA_INIT
}

void CDxfSetup1::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDxfSetup1)
	DDX_Control(pDX, IDC_DXF_CAMLINE, m_ctCamLayer);
	DDX_Control(pDX, IDC_DXF_ORIGIN, m_ctOrgLayer);
	DDX_Control(pDX, IDC_DXF_RELOAD, m_ctReload);
	DDX_Text(pDX, IDC_DXF_CAMLINE, m_strCamLayer);
	DDX_Text(pDX, IDC_DXF_ORIGIN, m_strOrgLayer);
	DDX_CBIndex(pDX, IDC_DXF_ORGTYPE, m_nOrgType);
	//}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////
// ���ފ֐�

void CDxfSetup1::EnableReloadButton(void)
{
	m_ctReload.EnableWindow(
		AfxGetNCVCApp()->GetDXFOpenDocumentCount() <= 0 ? FALSE : TRUE );
}

/////////////////////////////////////////////////////////////////////////////
// CDxfSetup1 ���b�Z�[�W �n���h��

BOOL CDxfSetup1::OnInitDialog() 
{
	__super::OnInitDialog();

	const CDXFOption* pOpt = AfxGetNCVCApp()->GetDXFOption();
	m_strOrgLayer	= pOpt->m_strReadLayer[DXFORGLAYER];
	m_strCamLayer	= pOpt->m_strReadLayer[DXFCAMLAYER];
	m_nOrgType		= pOpt->m_nOrgType;
	UpdateData(FALSE);

	// DXF�޷���Ă��J����Ă��邩�ǂ���
	EnableReloadButton();

	return TRUE;  // �R���g���[���Ƀt�H�[�J�X��ݒ肵�Ȃ��Ƃ��A�߂�l�� TRUE �ƂȂ�܂�
	              // ��O: OCX �v���p�e�B �y�[�W�̖߂�l�� FALSE �ƂȂ�܂�
}

BOOL CDxfSetup1::OnApply() 
{
	CDXFOption*	pOpt = AfxGetNCVCApp()->GetDXFOption();
	pOpt->m_strReadLayer[DXFORGLAYER]	= m_strOrgLayer;
	pOpt->m_strReadLayer[DXFCAMLAYER]	= m_strCamLayer;
	pOpt->m_nOrgType	= m_nOrgType;
	try {
		pOpt->m_regCutter = xpressive::cregex::compile(LPCTSTR(m_strCamLayer));
	}
	catch (xpressive::regex_error&) {
		AfxMessageBox(IDS_ERR_REGEX, MB_OK|MB_ICONEXCLAMATION);
		m_ctCamLayer.SetFocus();
		return FALSE;
	}

	return TRUE;
}

BOOL CDxfSetup1::OnSetActive() 
{
	EnableReloadButton();
	return __super::OnSetActive();
}

BOOL CDxfSetup1::OnKillActive() 
{
	if ( !__super::OnKillActive() )
		return FALSE;

	if ( m_strCamLayer.IsEmpty() ) {
		AfxMessageBox(IDS_ERR_DXFLAYER, MB_OK|MB_ICONEXCLAMATION);
		m_ctCamLayer.SetFocus();
		return FALSE;
	}
	if ( m_strOrgLayer.IsEmpty() ) {
		AfxMessageBox(IDS_ERR_DXFLAYER, MB_OK|MB_ICONEXCLAMATION);
		m_ctOrgLayer.SetFocus();
		return FALSE;
	}

	return TRUE;
}

void CDxfSetup1::OnReload() 
{
	if ( !OnKillActive() )	// UpdateData() & �ް�����
		return;
	OnApply();
	if ( static_cast<CDxfSetup *>(GetParentSheet())->OnReload(this) ) {
		m_ctCamLayer.SetFocus();
		m_ctCamLayer.SetSel(0, -1);
		EnableReloadButton();
	}
}
