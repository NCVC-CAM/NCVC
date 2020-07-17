// NCViewXY.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "NCChild.h"
#include "NCdata.h"
#include "NCDoc.h"
#include "NCViewTab.h"
#include "NCInfoTab.h"
#include "NCViewXY.h"
#include "NCListView.h"
#include "ViewOption.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNCViewXY

IMPLEMENT_DYNCREATE(CNCViewXY, CNCViewBase)

BEGIN_MESSAGE_MAP(CNCViewXY, CNCViewBase)
	//{{AFX_MSG_MAP(CNCViewXY)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNCViewXY �N���X�̍\�z/����

CNCViewXY::CNCViewXY() : CNCViewBase(NCDRAWVIEW_XY)
{
#ifdef _DEBUG_FILEOPEN
	g_dbg.printf("CNCViewXY::CNCViewXY() Start");
#endif
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewXY �N���X�̃I�[�o���C�h�֐�

void CNCViewXY::OnInitialUpdate()
{
	extern	LPCTSTR	g_szNdelimiter;	// "XYZUVWIJKRPLDH" from NCDoc.cpp

	CView::OnInitialUpdate();

	// �`��֐��̌���ƕ\�����ʂ̈ē���������
	m_pfnDrawProc = GetDocument()->IsDocFlag(NCDOC_WIRE) ?
		&(CNCdata::DrawWireXY) : &(CNCdata::DrawXY);
	if ( GetDocument()->IsDocFlag(NCDOC_LATHE) ) {
		m_strGuide  = g_szNdelimiter[NCA_Z];
		m_strGuide += g_szNdelimiter[NCA_Y];
	}
	else {
		m_strGuide = g_szNdelimiter;
		m_strGuide.Delete(NCA_Z, 999);
	}

	CNCViewBase::OnInitialUpdate();
}

void CNCViewXY::SetGuideData(void)
{
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	double	dSrc = pOpt->GetNCViewFlg(NCVIEWFLG_GUIDELENGTH) ?
					m_dFactor*LOMETRICFACTOR : LOMETRICFACTOR;
	// �w���̃K�C�h�������i������E�ցj
	m_ptGuide[0][0].x = (int)(-pOpt->GetGuideLength(NCA_X) * dSrc);
	m_ptGuide[0][0].y = 0;
	m_ptGuide[0][1].x = (int)( pOpt->GetGuideLength(NCA_X) * dSrc);
	m_ptGuide[0][1].y = 0;
	// �x���̃K�C�h�������i�������O�ցj
	m_ptGuide[1][0].x = 0;
	m_ptGuide[1][0].y = (int)( pOpt->GetGuideLength(NCA_Y) * dSrc);
	m_ptGuide[1][1].x = 0;
	m_ptGuide[1][1].y = (int)(-pOpt->GetGuideLength(NCA_Y) * dSrc);
}
