// NCViewYZ.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "NCdata.h"
#include "NCDoc.h"
#include "NCViewYZ.h"
#include "ViewOption.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNCViewYZ

IMPLEMENT_DYNCREATE(CNCViewYZ, CNCViewBase)

BEGIN_MESSAGE_MAP(CNCViewYZ, CNCViewBase)
	//{{AFX_MSG_MAP(CNCViewYZ)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNCViewYZ �N���X�̍\�z/����

CNCViewYZ::CNCViewYZ() : CNCViewBase(NCDRAWVIEW_YZ)
{
#ifdef _DEBUG_FILEOPEN
	g_dbg.printf("CNCViewYZ::CNCViewYZ() Start");
#endif
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewYZ �N���X�̃I�[�o���C�h�֐�

void CNCViewYZ::OnInitialUpdate()
{
	extern	LPCTSTR	g_szNdelimiter;	// "XYZUVWIJKRPLDH" from NCDoc.cpp

	CView::OnInitialUpdate();

	// �`��֐��̌���ƕ\�����ʂ̈ē���������
	m_pfnDrawProc = GetDocument()->IsDocFlag(NCDOC_WIRE) ?
		&(CNCdata::DrawWireYZ) : &(CNCdata::DrawYZ);
	if ( GetDocument()->IsDocFlag(NCDOC_LATHE) ) {
		m_strGuide  = g_szNdelimiter[NCA_Y];
		m_strGuide += g_szNdelimiter[NCA_X];
	}
	else {
		m_strGuide  = g_szNdelimiter[NCA_Y];
		m_strGuide += g_szNdelimiter[NCA_Z];
	}

	CNCViewBase::OnInitialUpdate();
}

void CNCViewYZ::SetGuideData(void)
{
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	double	dSrc = pOpt->GetNCViewFlg(NCVIEWFLG_GUIDELENGTH) ?
					m_dFactor*LOMETRICFACTOR : LOMETRICFACTOR;
	// �x���̃K�C�h�������i�������O�ցj
	m_ptGuide[0][0].x = (int)( pOpt->GetGuideLength(NCA_Y) * dSrc);
	m_ptGuide[0][0].y = 0;
	m_ptGuide[0][1].x = (int)(-pOpt->GetGuideLength(NCA_Y) * dSrc);
	m_ptGuide[0][1].y = 0;
	// �y���̃K�C�h�������i�ォ�牺�ցj
	m_ptGuide[1][0].x = 0;
	m_ptGuide[1][0].y = (int)( pOpt->GetGuideLength(NCA_Z) * dSrc);
	m_ptGuide[1][1].x = 0;
	m_ptGuide[1][1].y = (int)(-pOpt->GetGuideLength(NCA_Z) * dSrc);
}
