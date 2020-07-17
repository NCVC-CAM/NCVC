// NCViewYZ.cpp : インプリメンテーション ファイル
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
// CNCViewYZ クラスの構築/消滅

CNCViewYZ::CNCViewYZ() : CNCViewBase(NCDRAWVIEW_YZ)
{
#ifdef _DEBUG_FILEOPEN
	g_dbg.printf("CNCViewYZ::CNCViewYZ() Start");
#endif
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewYZ クラスのオーバライド関数

void CNCViewYZ::OnInitialUpdate()
{
	extern	LPCTSTR	g_szNdelimiter;	// "XYZUVWIJKRPLDH" from NCDoc.cpp

	CView::OnInitialUpdate();

	// 描画関数の決定と表示平面の案内文字列をｾｯﾄ
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
	// Ｙ軸のガイド初期化（奥から手前へ）
	m_ptGuide[0][0].x = (int)( pOpt->GetGuideLength(NCA_Y) * dSrc);
	m_ptGuide[0][0].y = 0;
	m_ptGuide[0][1].x = (int)(-pOpt->GetGuideLength(NCA_Y) * dSrc);
	m_ptGuide[0][1].y = 0;
	// Ｚ軸のガイド初期化（上から下へ）
	m_ptGuide[1][0].x = 0;
	m_ptGuide[1][0].y = (int)( pOpt->GetGuideLength(NCA_Z) * dSrc);
	m_ptGuide[1][1].x = 0;
	m_ptGuide[1][1].y = (int)(-pOpt->GetGuideLength(NCA_Z) * dSrc);
}
