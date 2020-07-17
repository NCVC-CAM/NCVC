// MKNCSetup8.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "resource.h"
#include "NCMakeMillOpt.h"
#include "MKNCSetup.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CMKNCSetup8, CPropertyPage)
	//{{AFX_MSG_MAP(CMKNCSetup8)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

#define	GetParentSheet()	static_cast<CMKNCSetup *>(GetParentSheet())

/////////////////////////////////////////////////////////////////////////////
// CMKNCSetup8 �v���p�e�B �y�[�W

CMKNCSetup8::CMKNCSetup8() : CPropertyPage(CMKNCSetup8::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;
	//{{AFX_DATA_INIT(CMKNCSetup8)
	m_bLayerComment	= FALSE;
	m_bL0Cycle		= FALSE;
	m_nMoveZ		= 0;
	m_strCustMoveB = _T("");
	m_strCustMoveA = _T("");
	//}}AFX_DATA_INIT
}

void CMKNCSetup8::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMKNCSetup8)
	DDX_Check(pDX, IDC_MKNC8_LAYERCOMMENT, m_bLayerComment);
	DDX_Check(pDX, IDC_MKNC8_L0CYCLE, m_bL0Cycle);
	DDX_CBIndex(pDX, IDC_MKNC8_MOVEZ, m_nMoveZ);
	DDX_Text(pDX, IDC_MKNC8_CUSTMOVEB, m_strCustMoveB);
	DDX_Text(pDX, IDC_MKNC8_CUSTMOVEA, m_strCustMoveA);
	//}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////
// CMKNCSetup8 ���b�Z�[�W �n���h��

BOOL CMKNCSetup8::OnInitDialog() 
{
	__super::OnInitDialog();

	// ���Ѻ��۰قͺݽ�׸��ŏ������ł��Ȃ�
	// + GetParentSheet() �߲�����擾�ł��Ȃ�
	CNCMakeMillOpt* pOpt = GetParentSheet()->GetNCMakeOption();
	m_bLayerComment	= pOpt->m_bLayerComment;
	m_bL0Cycle		= pOpt->m_bL0Cycle;
	m_nMoveZ		= pOpt->m_nMoveZ;
	m_strCustMoveB	= pOpt->m_strOption[MKNC_STR_CUSTMOVE_B];
	m_strCustMoveA	= pOpt->m_strOption[MKNC_STR_CUSTMOVE_A];

	UpdateData(FALSE);

	return TRUE;  // �R���g���[���Ƀt�H�[�J�X��ݒ肵�Ȃ��Ƃ��A�߂�l�� TRUE �ƂȂ�܂�
	              // ��O: OCX �v���p�e�B �y�[�W�̖߂�l�� FALSE �ƂȂ�܂�
}

BOOL CMKNCSetup8::OnApply() 
{
	CNCMakeMillOpt* pOpt = GetParentSheet()->GetNCMakeOption();
	pOpt->m_bLayerComment	= m_bLayerComment;
	pOpt->m_bL0Cycle		= m_bL0Cycle;
	pOpt->m_nMoveZ			= m_nMoveZ;
	pOpt->m_strOption[MKNC_STR_CUSTMOVE_B] = m_strCustMoveB;
	pOpt->m_strOption[MKNC_STR_CUSTMOVE_A] = m_strCustMoveA;

	return TRUE;
}
