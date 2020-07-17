// DxfSetup2.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "DXFOption.h"
#include "DxfSetup.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CDxfSetup2, CPropertyPage)
	//{{AFX_MSG_MAP(CDxfSetup2)
	ON_BN_CLICKED(IDC_DXF_RELOAD, &CDxfSetup2::OnReload)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDxfSetup2 �v���p�e�B �y�[�W

CDxfSetup2::CDxfSetup2() : CPropertyPage(CDxfSetup2::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;
	//{{AFX_DATA_INIT(CDxfSetup2)
	//}}AFX_DATA_INIT
	const CDXFOption* pOpt = AfxGetNCVCApp()->GetDXFOption();
	m_strStartLayer		= pOpt->m_strReadLayer[DXFSTRLAYER];
	m_strMoveLayer		= pOpt->m_strReadLayer[DXFMOVLAYER];
	m_strCommentLayer	= pOpt->m_strReadLayer[DXFCOMLAYER];
}

void CDxfSetup2::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDxfSetup2)
	DDX_Control(pDX, IDC_DXF_RELOAD, m_ctReload);
	DDX_Control(pDX, IDC_DXF_START, m_ctStartLayer);
	DDX_Text(pDX, IDC_DXF_START, m_strStartLayer);
	DDX_Text(pDX, IDC_DXF_MOVE, m_strMoveLayer);
	DDX_Text(pDX, IDC_DXF_COMMENT, m_strCommentLayer);
	//}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////
// ���ފ֐�

void CDxfSetup2::EnableReloadButton(void)
{
	if ( AfxGetNCVCApp()->GetDXFOpenDocumentCount() <= 0 )
		m_ctReload.EnableWindow(FALSE);
	else
		m_ctReload.EnableWindow(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// CDxfSetup2 ���b�Z�[�W �n���h��

BOOL CDxfSetup2::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	// DXF�޷���Ă��J����Ă��邩�ǂ���
	EnableReloadButton();

	return TRUE;  // �R���g���[���Ƀt�H�[�J�X��ݒ肵�Ȃ��Ƃ��A�߂�l�� TRUE �ƂȂ�܂�
	              // ��O: OCX �v���p�e�B �y�[�W�̖߂�l�� FALSE �ƂȂ�܂�
}

BOOL CDxfSetup2::OnApply() 
{
	CDXFOption*	pOpt = AfxGetNCVCApp()->GetDXFOption();
	pOpt->m_strReadLayer[DXFSTRLAYER] = m_strStartLayer;
	pOpt->m_strReadLayer[DXFMOVLAYER] = m_strMoveLayer;
	pOpt->m_strReadLayer[DXFCOMLAYER] = m_strCommentLayer;

	return TRUE;
}

void CDxfSetup2::OnReload() 
{
	UpdateData();
	OnApply();
	CDxfSetup*	pParent = static_cast<CDxfSetup *>(GetParent());
	if ( pParent->OnReload(this) ) {
		m_ctStartLayer.SetFocus();
		m_ctStartLayer.SetSel(0, -1);
		EnableReloadButton();
	}
}

BOOL CDxfSetup2::OnSetActive() 
{
	EnableReloadButton();
	return CPropertyPage::OnSetActive();
}
