// NCViewXZ.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "NCdata.h"
#include "NCDoc.h"
#include "NCViewXZ.h"
#include "ViewOption.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNCViewXZ

IMPLEMENT_DYNCREATE(CNCViewXZ, CNCViewBase)

BEGIN_MESSAGE_MAP(CNCViewXZ, CNCViewBase)
	//{{AFX_MSG_MAP(CNCViewXZ)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNCViewXZ �N���X�̍\�z/����

CNCViewXZ::CNCViewXZ() : CNCViewBase(NCDRAWVIEW_XZ)
{
#ifdef _DEBUG_FILEOPEN
	g_dbg.printf("CNCViewXZ::CNCViewXZ() Start");
#endif
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewXZ �N���X�̃I�[�o���C�h�֐�

void CNCViewXZ::OnInitialUpdate()
{
	extern	LPCTSTR	g_szNdelimiter;	// "XYZUVWIJKRPLDH" from NCDoc.cpp

	CView::OnInitialUpdate();

	// �`��֐��̌���ƕ\�����ʂ̈ē���������
	m_pfnDrawProc = GetDocument()->IsDocFlag(NCDOC_WIRE) ?
		&(CNCdata::DrawWireXZ) : &(CNCdata::DrawXZ);
	if ( GetDocument()->IsDocFlag(NCDOC_LATHE) ) {
		m_strGuide  = g_szNdelimiter[NCA_Z];
		m_strGuide += g_szNdelimiter[NCA_X];
	}
	else {
		m_strGuide  = g_szNdelimiter[NCA_X];
		m_strGuide += g_szNdelimiter[NCA_Z];
	}

	CNCViewBase::OnInitialUpdate();
}

void CNCViewXZ::SetGuideData(void)
{
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	float	dSrc = pOpt->GetNCViewFlg(NCVIEWFLG_GUIDELENGTH) ?
						m_dFactor : LOMETRICFACTOR;
	// �w���̃K�C�h�������i������E�ցj
	m_ptGuide[0][0].x = (int)(-pOpt->GetGuideLength(NCA_X) * dSrc);
	m_ptGuide[0][0].y = 0;
	m_ptGuide[0][1].x = (int)( pOpt->GetGuideLength(NCA_X) * dSrc);
	m_ptGuide[0][1].y = 0;
	// �y���̃K�C�h�������i�ォ�牺�ցj
	m_ptGuide[1][0].x = 0;
	m_ptGuide[1][0].y = (int)( pOpt->GetGuideLength(NCA_Z) * dSrc);
	m_ptGuide[1][1].x = 0;
	m_ptGuide[1][1].y = (int)(-pOpt->GetGuideLength(NCA_Z) * dSrc);
}
