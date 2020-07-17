// MCSetup2.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "MCOption.h"
#include "MCSetup.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CMCSetup2, CPropertyPage)
	//{{AFX_MSG_MAP(CMCSetup2)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMCSetup2 �v���p�e�B �y�[�W

CMCSetup2::CMCSetup2() : CPropertyPage(CMCSetup2::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;
	//{{AFX_DATA_INIT(CMCSetup2)
		// ���� - ClassWizard �͂��̈ʒu�Ƀ����o�̏�����������ǉ����܂��B
	//}}AFX_DATA_INIT
}

CMCSetup2::~CMCSetup2()
{
}

void CMCSetup2::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMCSetup2)
		// ���� - ClassWizard �͂��̈ʒu�Ƀ}�b�s���O�p�̃}�N����ǉ��܂��͍폜���܂��B
	//}}AFX_DATA_MAP
	int		i, j;
	for ( i=0; i<NCXYZ; i++ )
		DDX_Control(pDX, i+IDC_MCST2_X, m_dInitialXYZ[i]);
	for ( i=0; i<WORKOFFSET; i++ ) {
		for ( j=0; j<NCXYZ; j++ ) {
			DDX_Control(pDX, i*NCXYZ+j+IDC_MCST2_G54X, m_dWorkOffset[i][j]);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// CMCSetup2 ���b�Z�[�W �n���h��

BOOL CMCSetup2::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	int			i, j;
	const CMCOption*	pMCopt = AfxGetNCVCApp()->GetMCOption();
	// �ݽ�׸��ł͂ł��Ȃ����۰ق̏�����
	for ( i=0; i<NCXYZ; i++ )
		m_dInitialXYZ[i] = pMCopt->m_dInitialXYZ[i];
	for ( i=0; i<WORKOFFSET; i++ ) {
		for ( j=0; j<NCXYZ; j++ ) {
			m_dWorkOffset[i][j] = pMCopt->m_dWorkOffset[i][j];
		}
	}
	UpdateData(FALSE);

	return TRUE;  // �R���g���[���Ƀt�H�[�J�X��ݒ肵�Ȃ��Ƃ��A�߂�l�� TRUE �ƂȂ�܂�
	              // ��O: OCX �v���p�e�B �y�[�W�̖߂�l�� FALSE �ƂȂ�܂�
}

BOOL CMCSetup2::OnApply() 
{
	int		i, j;
	CMCSetup*	pParent = static_cast<CMCSetup *>(GetParent());
	CMCOption*	pMCopt = AfxGetNCVCApp()->GetMCOption();

	// �ēǍ�����
	for ( i=0; i<NCXYZ; i++ ) {
		if ( pMCopt->m_dInitialXYZ[i] != m_dInitialXYZ[i] ) {
			pMCopt->m_dInitialXYZ[i] = m_dInitialXYZ[i];
			pParent->m_bReload = TRUE;		// �ēǍ����K�v
		}
	}
	for ( i=0; i<WORKOFFSET; i++ ) {
		for ( j=0; j<NCXYZ; j++ ) {
			if ( pMCopt->m_dWorkOffset[i][j] != m_dWorkOffset[i][j] ) {
				pMCopt->m_dWorkOffset[i][j] = m_dWorkOffset[i][j];
				pParent->m_bReload = TRUE;
			}
		}
	}

	return TRUE;
}
