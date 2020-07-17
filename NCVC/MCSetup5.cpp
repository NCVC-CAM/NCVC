// MCSetup5.cpp : �C���v�������e�[�V���� �t�@�C��
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

BEGIN_MESSAGE_MAP(CMCSetup5, CPropertyPage)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMCSetup5 �v���p�e�B �y�[�W

CMCSetup5::CMCSetup5() : CPropertyPage(CMCSetup5::IDD)
{
	m_psp.dwFlags &= ~PSP_HASHELP;
	const CMCOption*	pMCopt = AfxGetNCVCApp()->GetMCOption();
	m_bL0Cycle = pMCopt->m_bL0Cycle;
}

CMCSetup5::~CMCSetup5()
{
}

void CMCSetup5::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_MCST5_L0CYCLE, m_bL0Cycle);
}

/////////////////////////////////////////////////////////////////////////////
// CMCSetup5 ���b�Z�[�W �n���h��

BOOL CMCSetup5::OnApply() 
{
	CMCSetup*	pParent = static_cast<CMCSetup *>(GetParent());
	CMCOption*	pMCopt = AfxGetNCVCApp()->GetMCOption();

	// �ēǍ�����
	if ( pMCopt->m_bL0Cycle != m_bL0Cycle ) {
		pMCopt->m_bL0Cycle = m_bL0Cycle;
		pParent->m_bReload = TRUE;		// �ēǍ����K�v
	}

	return TRUE;
}
