// MCSetup1.cpp : �C���v�������e�[�V���� �t�@�C��
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

BEGIN_MESSAGE_MAP(CMCSetup1, CPropertyPage)
	//{{AFX_MSG_MAP(CMCSetup1)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMCSetup1 �v���p�e�B �y�[�W

CMCSetup1::CMCSetup1() : CPropertyPage(CMCSetup1::IDD),
	m_dFspeed(TRUE)	// CFloatEdit�׸ސݒ�
{
	m_psp.dwFlags &= ~PSP_HASHELP;
	//{{AFX_DATA_INIT(CMCSetup1)
	m_nFselect = -1;
	//}}AFX_DATA_INIT
	for ( int i=0; i<MODALGROUP; i++ )
		m_nModalGroup[i] = -1;
}

void CMCSetup1::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMCSetup1)
	DDX_Control(pDX, IDC_MCST1_FSPEED, m_dFspeed);
	DDX_Control(pDX, IDC_MCST1_BLOCK, m_dBlock);
	DDX_Text(pDX, IDC_MCST1_NAME, m_strName);
	DDX_CBIndex(pDX, IDC_MCST1_FSEC, m_nFselect);
	//}}AFX_DATA_MAP
	int		i;
	for ( i=0; i<MODALGROUP; i++ )
		DDX_CBIndex(pDX, i+IDC_MCST1_MGROUP_A, m_nModalGroup[i]);
	for ( i=0; i<NCXYZ; i++ )
		DDX_Control(pDX, i+IDC_MCST1_XSPEED, m_nMoveSpeed[i]);
}

/////////////////////////////////////////////////////////////////////////////
// CMCSetup1 ���b�Z�[�W �n���h��

BOOL CMCSetup1::OnInitDialog() 
{
	__super::OnInitDialog();

	int			i;
	const CMCOption*	pMCopt = AfxGetNCVCApp()->GetMCOption();
	m_strName = pMCopt->m_strMCname;
	// �ݽ�׸��ł͂ł��Ȃ����۰ق̏�����
	for ( i=0; i<MODALGROUP; i++ )
		m_nModalGroup[i] = pMCopt->m_nModal[i];
	for ( i=0; i<NCXYZ; i++ )
		m_nMoveSpeed[i] = pMCopt->m_nG0Speed[i];
	m_nFselect	= pMCopt->m_nFDot;
	m_dFspeed	= pMCopt->m_dFeed;
	m_dBlock	= pMCopt->m_dBlock;
	UpdateData(FALSE);

	return TRUE;  // �R���g���[���Ƀt�H�[�J�X��ݒ肵�Ȃ��Ƃ��A�߂�l�� TRUE �ƂȂ�܂�
	              // ��O: OCX �v���p�e�B �y�[�W�̖߂�l�� FALSE �ƂȂ�܂�
}

BOOL CMCSetup1::OnApply() 
{
	int		i;
	CMCSetup*	pParent = static_cast<CMCSetup *>(GetParentSheet());
	CMCOption*	pMCopt = AfxGetNCVCApp()->GetMCOption();

	pMCopt->m_strMCname = m_strName;

	// �Čv�Zor�ēǍ�����
	for ( i=0; i<MODALGROUP; i++ ) {
		if ( pMCopt->m_nModal[i] != m_nModalGroup[i] ) {
			pMCopt->m_nModal[i] = m_nModalGroup[i];
			pParent->m_bReload = TRUE;		// �ēǍ����K�v
		}
	}
	for ( i=0; i<NCXYZ; i++ ) {
		if ( pMCopt->m_nG0Speed[i] != m_nMoveSpeed[i] ) {
			pMCopt->m_nG0Speed[i] = m_nMoveSpeed[i];
			pParent->m_bCalcThread = TRUE;	// �Čv�Z���K�v
		}
	}
	if ( pMCopt->m_nFDot != m_nFselect ) {
		pMCopt->m_nFDot = m_nFselect;
		pParent->m_bReload = TRUE;
	}
	if ( pMCopt->m_dFeed != m_dFspeed ) {
		pMCopt->m_dFeed = m_dFspeed;
		pParent->m_bReload = TRUE;
	}
	if ( pMCopt->m_dBlock != m_dBlock ) {
		pMCopt->m_dBlock = m_dBlock;
		pParent->m_bCalcThread = TRUE;
	}

	return TRUE;
}

BOOL CMCSetup1::OnKillActive() 
{
	if ( !__super::OnKillActive() )
		return FALSE;

	if ( m_dFspeed <= 0 ) {
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_dFspeed.SetFocus();
		m_dFspeed.SetSel(0, -1);
		return FALSE;
	}
	if ( m_dBlock < 0 ) {
		AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
		m_dBlock.SetFocus();
		m_dBlock.SetSel(0, -1);
		return FALSE;
	}
	for ( int i=0; i<NCXYZ; i++ ) {
		if ( m_nMoveSpeed[i] <= 0 ) {
			AfxMessageBox(IDS_ERR_UNDERZERO, MB_OK|MB_ICONEXCLAMATION);
			m_nMoveSpeed[i].SetFocus();
			m_nMoveSpeed[i].SetSel(0, -1);
			return FALSE;
		}
	}

	return TRUE;
}
