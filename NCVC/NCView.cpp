// NCView.cpp : CNCView クラスの動作の定義を行います。
//

#include "stdafx.h"
#include "NCVC.h"
#include "NCdata.h"
#include "NCDoc.h"
#include "NCView.h"
#include "ViewOption.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

#define	IsThumbnail()	GetDocument()->IsDocFlag(NCDOC_THUMBNAIL)

/////////////////////////////////////////////////////////////////////////////
// CNCView

IMPLEMENT_DYNCREATE(CNCView, CNCViewBase)

BEGIN_MESSAGE_MAP(CNCView, CNCViewBase)
	//{{AFX_MSG_MAP(CNCView)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNCView クラスの構築/消滅

CNCView::CNCView() : CNCViewBase(NCDRAWVIEW_XYZ)
{
#ifdef _DEBUG_FILEOPEN
	g_dbg.printf("CNCView::CNCView() Start");
#endif
}

/////////////////////////////////////////////////////////////////////////////
// CNCView クラスのオーバライド関数

void CNCView::OnInitialUpdate()
{
	extern	LPCTSTR	g_szNdelimiter;	// "XYZUVWIJKRPLDH" from NCDoc.cpp

	CView::OnInitialUpdate();

	// 描画関数の決定と表示平面の案内文字列をｾｯﾄ
	m_pfnDrawProc = GetDocument()->IsDocFlag(NCDOC_WIRE) ?
		&(CNCdata::DrawWire) : &(CNCdata::Draw);
	if ( GetDocument()->IsDocFlag(NCDOC_LATHE) ) {
		m_strGuide  = g_szNdelimiter[NCA_Z];
		m_strGuide += g_szNdelimiter[NCA_Y];
		m_strGuide += g_szNdelimiter[NCA_X];
	}
	else {
		m_strGuide = g_szNdelimiter;
		m_strGuide.Delete(NCXYZ, 999);
	}

	CNCViewBase::OnInitialUpdate();
}

/////////////////////////////////////////////////////////////////////////////
// CNCView クラスのメンバ関数

void CNCView::SetGuideData(void)
{
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	float	dSrc = pOpt->GetNCViewFlg(NCVIEWFLG_GUIDELENGTH) ?
						m_dFactor : LOMETRICFACTOR;
	CPoint3F	pt;
	float		dLength;

	// Ｘ軸のガイド初期化（左から右へ）
	dLength = pOpt->GetGuideLength(NCA_X);
	pt.SetPoint(-dLength, 0.0, 0.0);
	m_ptGuide[NCA_X][0] = pt.PointConvert() * dSrc;
	pt.x = dLength;
	m_ptGuide[NCA_X][1] = pt.PointConvert() * dSrc;
	// Ｙ軸のガイド初期化（奥から手前へ）
	dLength = pOpt->GetGuideLength(NCA_Y);
	pt.SetPoint(0.0, dLength, 0.0);
	m_ptGuide[NCA_Y][0] = pt.PointConvert() * dSrc;
	pt.y = -dLength;
	m_ptGuide[NCA_Y][1] = pt.PointConvert() * dSrc;
	// Ｚ軸のガイド初期化（上から下へ）
	dLength = pOpt->GetGuideLength(NCA_Z);
	pt.SetPoint(0.0, 0.0, dLength);
	m_ptGuide[NCA_Z][0] = pt.PointConvert() * dSrc;
	pt.z = -dLength;
	m_ptGuide[NCA_Z][1] = pt.PointConvert() * dSrc;
}

void CNCView::SetDataMaxRect(void)
{
	extern	const	float	g_dDefaultGuideLength;	// 50.0 (ViewOption.cpp)
	CRect3F		rc(GetDocument()->GetMaxRect());
	// 占有矩形の補正(不正表示の防止)
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	float	dLength;
	if ( rc.Width() <= NCMIN ) {
		dLength = pOpt->GetGuideLength(NCA_X);
		if ( dLength == 0.0 )
			dLength = g_dDefaultGuideLength;
		rc.left  = -dLength;
		rc.right =  dLength;
	}
	if ( rc.Height() <= NCMIN ) {
		dLength = pOpt->GetGuideLength(NCA_Y);
		if ( dLength == 0.0 )
			dLength = g_dDefaultGuideLength;
		rc.top    = -dLength;
		rc.bottom =  dLength;
	}
	if ( rc.Depth() <= NCMIN ) {
		dLength = pOpt->GetGuideLength(NCA_Z);
		if ( dLength == 0.0 )
			dLength = g_dDefaultGuideLength;
		rc.low  = -dLength;
		rc.high =  dLength;
	}

	CPoint3F	pt;
	pt.SetPoint(rc.left,  rc.top,    rc.low);
	m_ptdMaxRect[0][0] = pt.PointConvert();
	pt.SetPoint(rc.right, rc.top,    rc.low);
	m_ptdMaxRect[0][1] = pt.PointConvert();
	pt.SetPoint(rc.right, rc.bottom, rc.low);
	m_ptdMaxRect[0][2] = pt.PointConvert();
	pt.SetPoint(rc.left,  rc.bottom, rc.low);
	m_ptdMaxRect[0][3] = pt.PointConvert();
	pt.SetPoint(rc.left,  rc.top,    rc.high);
	m_ptdMaxRect[1][0] = pt.PointConvert();
	pt.SetPoint(rc.right, rc.top,    rc.high);
	m_ptdMaxRect[1][1] = pt.PointConvert();
	pt.SetPoint(rc.right, rc.bottom, rc.high);
	m_ptdMaxRect[1][2] = pt.PointConvert();
	pt.SetPoint(rc.left,  rc.bottom, rc.high);
	m_ptdMaxRect[1][3] = pt.PointConvert();
	// ﾃﾞｨｽﾌﾟﾚｲに映し出される2D最大矩形
	m_rcDataMax.SetRectMinimum();
	for ( int i=0; i<SIZEOF(m_ptdMaxRect); i++ ) {
		for ( int j=0; j<SIZEOF(m_ptdMaxRect[0]); j++ ) {
			if ( m_rcDataMax.left > m_ptdMaxRect[i][j].x )
				m_rcDataMax.left = m_ptdMaxRect[i][j].x;
			if ( m_rcDataMax.top > m_ptdMaxRect[i][j].y )
				m_rcDataMax.top = m_ptdMaxRect[i][j].y;
			if ( m_rcDataMax.right < m_ptdMaxRect[i][j].x )
				m_rcDataMax.right = m_ptdMaxRect[i][j].x;
			if ( m_rcDataMax.bottom < m_ptdMaxRect[i][j].y )
				m_rcDataMax.bottom = m_ptdMaxRect[i][j].y;
		}
	}
}

void CNCView::SetWorkRect(void)
{
	CRect3F		rc(GetDocument()->GetWorkRect());
	CPoint3F	pt;

	// ﾜｰｸ領域の3Dﾃﾞｰﾀ
	pt.SetPoint(rc.left,  rc.top,    rc.high);
	m_ptdWorkRect[0][0] = pt.PointConvert();
	pt.z = rc.low;
	m_ptdWorkRect[1][0] = pt.PointConvert();
	//
	pt.SetPoint(rc.right, rc.top,    rc.high);
	m_ptdWorkRect[0][1] = pt.PointConvert();
	pt.z = rc.low;
	m_ptdWorkRect[1][1] = pt.PointConvert();
	//
	pt.SetPoint(rc.right, rc.bottom, rc.high);
	m_ptdWorkRect[0][2] = pt.PointConvert();
	pt.z = rc.low;
	m_ptdWorkRect[1][2] = pt.PointConvert();
	//
	pt.SetPoint(rc.left,  rc.bottom, rc.high);
	m_ptdWorkRect[0][3] = pt.PointConvert();
	pt.z = rc.low;
	m_ptdWorkRect[1][3] = pt.PointConvert();
}

void CNCView::SetWorkCylinder(void)
{
	CRect3F		rc(GetDocument()->GetWorkRect());
	CPointF		ptc(rc.CenterPoint());
	CPoint3F	pt;
	int			i;
	float		r = rc.Width()/2.0f, q;

	for ( i=0, q=0; i<ARCCOUNT; i++, q+=ARCSTEP ) {
		pt.x = r * cos(q) + ptc.x;
		pt.y = r * sin(q) + ptc.y;
		pt.z = rc.high;
		m_ptdWorkCylinder[0][i] = pt.PointConvert();
		pt.z = rc.low;
		m_ptdWorkCylinder[1][i] = pt.PointConvert();
	}
}

void CNCView::ConvertMaxRect(void)
{
	for ( int i=0; i<SIZEOF(m_ptDrawMaxRect); i++ ) {
		for ( int j=0; j<SIZEOF(m_ptDrawMaxRect[0]); j++ )
			m_ptDrawMaxRect[i][j] = DrawConvert(m_ptdMaxRect[i][j]);
	}
}

void CNCView::ConvertWorkRect(void)
{
	for ( int i=0; i<SIZEOF(m_ptDrawWorkRect); i++ ) {
		for ( int j=0; j<SIZEOF(m_ptDrawWorkRect[0]); j++ )
			m_ptDrawWorkRect[i][j] = DrawConvert(m_ptdWorkRect[i][j]);
	}
}

void CNCView::ConvertWorkCylinder(void)
{
	for ( int i=0; i<SIZEOF(m_ptDrawWorkCylinder); i++ ) {
		for ( int j=0; j<SIZEOF(m_ptDrawWorkCylinder[0]); j++ )
			m_ptDrawWorkCylinder[i][j] = DrawConvert(m_ptdWorkCylinder[i][j]);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CNCView クラスの描画

void CNCView::OnDraw(CDC* pDC)
{
	ASSERT_VALID(GetDocument());
	const CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();
	pDC->SetROP2(R2_COPYPEN);
	CPen* pOldPen = (CPen *)pDC->SelectStockObject(NULL_PEN);

	// 平面案内
	DrawInfo(pDC);

	// ｶﾞｲﾄﾞ表示
	if ( !IsThumbnail() ) {
		pDC->SetTextAlign(TA_CENTER|TA_BOTTOM);
		for ( int i=0; i<SIZEOF(m_ptGuide); i++ ) {
			if ( pOpt->GetGuideLength(i) > 0 ) {
				pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenOrg(i));
				pDC->MoveTo(m_ptGuide[i][0]);
				pDC->LineTo(m_ptGuide[i][1]);
				pDC->SetTextColor(pOpt->GetNcDrawColor(i+NCCOL_GUIDEX));
				pDC->TextOut(m_ptGuide[i][0].x, m_ptGuide[i][0].y, m_strGuide.Mid(i, 1));
			}
		}
	}

	// NCﾃﾞｰﾀ描画
	DrawNCdata(pDC);

	// 補助矩形
	DrawSupportRect(pDC);
	
	pDC->SelectObject(pOldPen);
}

void CNCView::DrawMaxRect(CDC* pDC)
{
	pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(NCPEN_MAXCUT));
	pDC->Polygon(m_ptDrawMaxRect[0], SIZEOF(m_ptDrawMaxRect[0]));
	pDC->Polygon(m_ptDrawMaxRect[1], SIZEOF(m_ptDrawMaxRect[0]));
	for ( int i=0; i<SIZEOF(m_ptDrawMaxRect[0]); i++ ) {
		pDC->MoveTo(m_ptDrawMaxRect[0][i]);
		pDC->LineTo(m_ptDrawMaxRect[1][i]);
	}
}

void CNCView::DrawWorkRect(CDC* pDC)
{
	pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(NCPEN_WORK));
	pDC->Polygon(m_ptDrawWorkRect[0], SIZEOF(m_ptDrawWorkRect[0]));
	pDC->Polygon(m_ptDrawWorkRect[1], SIZEOF(m_ptDrawWorkRect[0]));
	for ( int i=0; i<SIZEOF(m_ptDrawWorkRect[0]); i++ ) {
		pDC->MoveTo(m_ptDrawWorkRect[0][i]);
		pDC->LineTo(m_ptDrawWorkRect[1][i]);
	}
}

void CNCView::DrawWorkCylinder(CDC* pDC)
{
	int		i, n;

	pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(NCPEN_WORK));

	for ( n=0; n<SIZEOF(m_ptDrawWorkCylinder); n++ ) {
		pDC->MoveTo(m_ptDrawWorkCylinder[n][0]);
		for ( i=1; i<SIZEOF(m_ptDrawWorkCylinder[0]); i++ )
			pDC->LineTo(m_ptDrawWorkCylinder[n][i]);
		pDC->LineTo(m_ptDrawWorkCylinder[n][0]);
	}
	for ( i=0; i<ARCCOUNT; i+=ARCCOUNT/4 ) {
		pDC->MoveTo(m_ptDrawWorkCylinder[0][i]);
		pDC->LineTo(m_ptDrawWorkCylinder[1][i]);
	}
}
