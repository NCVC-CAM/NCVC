// NCViewXY.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "NCdata.h"
#include "NCDoc.h"
#include "NCViewXY.h"
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
// CNCViewXY クラスの構築/消滅

CNCViewXY::CNCViewXY() : CNCViewBase(NCDRAWVIEW_XY)
{
#ifdef _DEBUG_FILEOPEN
	g_dbg.printf("CNCViewXY::CNCViewXY() Start");
#endif
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewXY クラスのオーバライド関数

void CNCViewXY::OnInitialUpdate()
{
	extern	LPCTSTR	g_szNdelimiter;	// "XYZUVWIJKRPLDH" from NCDoc.cpp

	CView::OnInitialUpdate();

	// 描画関数の決定と表示平面の案内文字列をｾｯﾄ
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

/////////////////////////////////////////////////////////////////////////////
// CNCViewXY クラスのメンバ関数

void CNCViewXY::SetGuideData(void)
{
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	double	dSrc = pOpt->GetNCViewFlg(NCVIEWFLG_GUIDELENGTH) ?
					m_dFactor*LOMETRICFACTOR : LOMETRICFACTOR;
	// Ｘ軸のガイド初期化（左から右へ）
	m_ptGuide[0][0].x = (int)(-pOpt->GetGuideLength(NCA_X) * dSrc);
	m_ptGuide[0][0].y = 0;
	m_ptGuide[0][1].x = (int)( pOpt->GetGuideLength(NCA_X) * dSrc);
	m_ptGuide[0][1].y = 0;
	// Ｙ軸のガイド初期化（奥から手前へ）
	m_ptGuide[1][0].x = 0;
	m_ptGuide[1][0].y = (int)( pOpt->GetGuideLength(NCA_Y) * dSrc);
	m_ptGuide[1][1].x = 0;
	m_ptGuide[1][1].y = (int)(-pOpt->GetGuideLength(NCA_Y) * dSrc);
}

void CNCViewXY::SetWorkCylinder(void)
{
	CRectD		rc(GetDocument()->GetWorkRect());
	CPointD		ptc(rc.CenterPoint());
	int			i;
	double		r = rc.Width()/2.0, q;

	for ( i=0, q=0; i<ARCCOUNT; i++, q+=ARCSTEP ) {
		m_ptdWorkCylinder[i].x = r * cos(q) + ptc.x;
		m_ptdWorkCylinder[i].y = r * sin(q) + ptc.y;
	}
}

void CNCViewXY::ConvertWorkCylinder(void)
{
	for ( int i=0; i<SIZEOF(m_ptDrawWorkCylinder); i++ )
		m_ptDrawWorkCylinder[i] = DrawConvert(m_ptdWorkCylinder[i]);
}

void CNCViewXY::DrawWorkCylinder(CDC* pDC)
{
	pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(NCPEN_WORK));

	pDC->MoveTo(m_ptDrawWorkCylinder[0]);
	for ( int i=1; i<SIZEOF(m_ptDrawWorkCylinder); i++ )
		pDC->LineTo(m_ptDrawWorkCylinder[i]);
	pDC->LineTo(m_ptDrawWorkCylinder[0]);
}
