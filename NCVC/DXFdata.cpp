// DXFdata.cpp: CDXFdata クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "DXFdata.h"
#include "DXFDoc.h"
#include "Layer.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
//#define	_DEBUGDRAW_DXF			// 描画座標情報
//#define	_DEBUGDRAW_DXF_EDGE_	// 端点描画
#endif

IMPLEMENT_SERIAL(CCAMHead, CObject, 1)
IMPLEMENT_DYNAMIC(CDXFdata, CObject)
IMPLEMENT_SERIAL(CDXFpoint, CDXFdata, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFline, CDXFpoint, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFcircle, CDXFline, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFcircleEx, CDXFcircle, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFarc, CDXFcircle, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFellipse, CDXFarc, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFpolyline, CDXFline, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)
IMPLEMENT_SERIAL(CDXFtext, CDXFpoint, NCVCSERIALVERSION|VERSIONABLE_SCHEMA)

using namespace boost;

/////////////////////////////////////////////////////////////////////////////
// 静的変数の初期化
CPointD		CDXFdata::ms_ptOrg(HUGE_VAL, HUGE_VAL);
BOOL		CDXFdata::ms_fXRev = FALSE;
BOOL		CDXFdata::ms_fYRev = FALSE;
CDXFdata*	CDXFdata::ms_pData = NULL;
DWORD		CDXFdata::ms_nSerialSeq = 0;

PFNORGDRILLTUNING	CDXFpoint::ms_pfnOrgDrillTuning = &CDXFpoint::OrgTuning_Seq;

//////////////////////////////////////////////////////////////////////
// ＣＡＭデータのヘッダークラス
//////////////////////////////////////////////////////////////////////
void CCAMHead::Serialize(CArchive& ar)
{
	extern	DWORD	g_dwCamVer;		// NCVC.cpp
	static	TCHAR	ss_szID[] = "NCVC_CAM_DATA...";
	char	szID[sizeof(ss_szID)-1];	// 識別用(最後の \0 を除く)
	CString	strComment;					// ｺﾒﾝﾄ文字列

	if ( ar.IsStoring() ) {
		// 識別子
		ar.Write(ss_szID, sizeof(ss_szID)-1);
		// ﾊﾞｰｼﾞｮﾝNo.，ｺﾒﾝﾄ
		ar << (DWORD)NCVCSERIALVERSION << strComment;
	}
	else {
		CString	strMsg;
		// 識別子
		ar.Read(szID, sizeof(szID));
		if ( memcmp(szID, ss_szID, sizeof(szID)) != 0 ) {
			strMsg.Format(IDS_ERR_CAMDATA, ar.GetFile()->GetFilePath());
			AfxMessageBox(strMsg, MB_OK|MB_ICONSTOP);
			AfxThrowUserException();
		}
		// ﾊﾞｰｼﾞｮﾝNo.
		ar >> g_dwCamVer;
		if ( g_dwCamVer > NCVCSERIALVERSION ) {
			strMsg.Format(IDS_ERR_CAMVER, ar.GetFile()->GetFilePath());
			AfxMessageBox(strMsg, MB_OK|MB_ICONSTOP);
			AfxThrowUserException();
		}
		else if ( g_dwCamVer < NCVCSERIALVERSION_1503 ) {
			strMsg.Format(IDS_ERR_CAMOLD, ar.GetFile()->GetFilePath());
			AfxMessageBox(strMsg, MB_OK|MB_ICONSTOP);
			AfxThrowUserException();
		}
		// ｺﾒﾝﾄ
		ar >> strComment;
	}
}

//////////////////////////////////////////////////////////////////////
// ＤＸＦデータのベースクラス
//////////////////////////////////////////////////////////////////////
CDXFdata::CDXFdata(ENDXFTYPE enType, CLayerData* pLayer, int nPoint)
{
	m_enType	= m_enMakeType = enType;
	m_dwFlags	= 0;
	m_pParentLayer = pLayer;
	m_pParentMap   = NULL;
	m_dwSelect	= 0;
	m_nPoint	= nPoint;
	if ( nPoint > 0 ) {
		m_pt		= new CPointD[nPoint];
		m_ptTun		= new CPointD[nPoint];
		m_ptMake	= new CPointD[nPoint];
	}
	else {
		m_pt		= NULL;
		m_ptTun		= NULL;
		m_ptMake	= NULL;
	}
}

CDXFdata::~CDXFdata()
{
	if ( m_nPoint > 0 ) {
		delete[]	m_pt;
		delete[]	m_ptTun;
		delete[]	m_ptMake;
	}
}

void CDXFdata::Serialize(CArchive& ar)
{
	if ( ar.IsStoring() ) {
		ar << (m_dwSelect & ~DXFSEL_SELECT);	// 選択状態除く
		m_nSerialSeq = ms_nSerialSeq++;
	}
	else {
		ar >> m_dwSelect;
		m_pParentLayer = reinterpret_cast<CLayerData *>(ar.m_pDocument);
	}
}

CPen* CDXFdata::GetDrawPen(void) const
{
	CPen*	pDrawPen[] = {
		AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_CUTTER),
		AfxGetNCVCMainWnd()->GetPenCom(COMPEN_SEL)
	};
	return pDrawPen[ m_dwSelect & DXFSEL_SELECT ? 1 : 0];
}

void CDXFdata::XRev(void)		// X軸の符号反転
{
	for ( int i=0; i<m_nPoint; i++ ) {
		m_ptTun[i].x  = -m_ptTun[i].x;
		m_ptMake[i].x = -m_ptMake[i].x;
	}
}

void CDXFdata::YRev(void)		// Y軸の符号反転
{
	for ( int i=0; i<m_nPoint; i++ ) {
		m_ptTun[i].y  = -m_ptTun[i].y;
		m_ptMake[i].y = -m_ptMake[i].y;
	}
}

void CDXFdata::OrgTuningBase(void)
{
	m_dwFlags = 0;			// 生成，検索ﾌﾗｸﾞのｸﾘｱ
	for ( int i=0; i<m_nPoint; i++ )
		m_ptMake[i] = m_ptTun[i].RoundUp();
	if ( ms_fXRev ) XRev();		// 符号反転
	if ( ms_fYRev ) YRev();		// 符号反転
}

BOOL CDXFdata::GetDirectionArraw_Line(const CPointD pt[], CPointD ptResult[][3]) const
{
	double	lqs = atan2(pt[1].y - pt[0].y, pt[1].x - pt[0].x),
			lqe = lqs + 180.0*RAD;
	double	lq[][2] = { {lqs + ARRAWANGLE, lqs - ARRAWANGLE},
						{lqe + ARRAWANGLE, lqe - ARRAWANGLE} };
	for ( int i=0; i<2; i++ ) {
		ptResult[i][0].x = ARRAWLENGTH * cos(lq[i][0]);	// + pt[i] は拡大率反映後
		ptResult[i][0].y = ARRAWLENGTH * sin(lq[i][0]);
		ptResult[i][1]   = pt[i];
		ptResult[i][2].x = ARRAWLENGTH * cos(lq[i][1]);
		ptResult[i][2].y = ARRAWLENGTH * sin(lq[i][1]);
	}
	return TRUE;
}

BOOL CDXFdata::GetDirectionArraw_Circle
	(BOOL bRound, const double q[], const CPointD pt[], CPointD ptResult[][3]) const
{
	double	lq[][2] = { {q[0] + ARRAWANGLE, q[0] - ARRAWANGLE},
						{q[1] + ARRAWANGLE, q[1] - ARRAWANGLE} };
	for ( int i=0; i<2; i++ ) {
		ptResult[i][0].x = ARRAWLENGTH * cos(lq[i][0]);
		ptResult[i][0].y = ARRAWLENGTH * sin(lq[i][0]);
		ptResult[i][1]   = pt[i];
		ptResult[i][2].x = ARRAWLENGTH * cos(lq[i][1]);
		ptResult[i][2].y = ARRAWLENGTH * sin(lq[i][1]);
	}

	return TRUE;
}

//////////////////////////////////////////////////////////////////////
// ＤＸＦデータのPointクラス
//////////////////////////////////////////////////////////////////////
CDXFpoint::CDXFpoint() : CDXFdata(DXFPOINTDATA, NULL, 1)
{
}

CDXFpoint::CDXFpoint(ENDXFTYPE enType, CLayerData* pLayer, int nPoint) :
	CDXFdata(enType, pLayer, nPoint)
{
}

CDXFpoint::CDXFpoint(LPDXFPARGV lpPoint) : CDXFdata(DXFPOINTDATA, lpPoint->pLayer, 1)
{
	m_pt[0] = lpPoint->c;
	SetMaxRect();
}

CDXFpoint::CDXFpoint(CLayerData* pLayer, const CDXFpoint* pData, LPDXFBLOCK lpBlock) :
	CDXFdata(DXFPOINTDATA, pLayer, 1)
{
	m_pt[0] = pData->GetNativePoint(0);
	if ( lpBlock->dwBlockFlg & DXFBLFLG_X )
		m_pt[0].x *= lpBlock->dMagni[NCA_X];
	if ( lpBlock->dwBlockFlg & DXFBLFLG_Y )
		m_pt[0].y *= lpBlock->dMagni[NCA_Y];
	if ( lpBlock->dwBlockFlg & DXFBLFLG_R )
		m_pt[0].RoundPoint(lpBlock->dRound*RAD);
	m_pt[0] += lpBlock->ptOrg;
	SetMaxRect();
}

void CDXFpoint::Serialize(CArchive& ar)
{
	CDXFdata::Serialize(ar);
	if ( ar.IsStoring() )
		ar << m_pt[0].x << m_pt[0].y;
	else {
		ar >> m_pt[0].x >> m_pt[0].y;
		SetMaxRect();
	}
}

void CDXFpoint::DrawTuning(double f)
{
	CPointD	pt( m_pt[0] * f);
	m_ptDraw = pt;
	// 位置を表す丸印は常に2.5論理理位
	m_rcDraw.TopLeft()		= pt - LOMETRICFACTOR*2.5;
	m_rcDraw.BottomRight()	= pt + LOMETRICFACTOR*2.5;
}

void CDXFpoint::Draw(CDC* pDC) const
{
#ifdef _DEBUGDRAW_DXF
	CMagaDbg	dbg("CDXFpoint::Draw()", DBG_RED);
	dbg.printStruct((LPRECT)&m_rcDraw, "m_rcDraw");
#endif
	CPoint	pt(m_rcDraw.CenterPoint());
	pDC->MoveTo(m_rcDraw.right-1, pt.y);
	pDC->LineTo(m_rcDraw.left,  pt.y);
	pDC->LineTo(pt.x, m_rcDraw.top+1);	// topが下側
	pDC->LineTo(pt.x, m_rcDraw.bottom);
	pDC->LineTo(m_rcDraw.right-1, pt.y);
	pDC->Ellipse(&m_rcDraw);
}

double CDXFpoint::OrgTuning(BOOL bCalc/*=TRUE*/)
{
	ASSERT(ms_pData);
	m_ptTun[0] = m_pt[0] - ms_ptOrg;
	OrgTuningBase();
#ifdef _DEBUG
	CMagaDbg	dbg;
	dbg.printf("OrgTuning Point cx=%f cy=%f", m_ptTun[0].x, m_ptTun[0].y);
#endif
	return bCalc ? (*ms_pfnOrgDrillTuning)(this) : 0.0;	// 基準軸にあわせた近接座標 : dummy
}

double CDXFpoint::OrgTuning_Seq(const CDXFpoint* pData)
{
	// 指定ｵﾌﾞｼﾞｪｸﾄまでの距離計算
	return GAPCALC(pData->m_ptTun[0] - ms_pData->GetEndCutterPoint());
}

double CDXFpoint::OrgTuning_XY(const CDXFpoint*)
{
	return 0.0;		// 基準軸ごとに並べ替えするのでﾀﾞﾐｰを返す
}

double CDXFpoint::GetSelectPointGap(const CPointD&)
{
	return HUGE_VAL;
}

BOOL CDXFpoint::GetDirectionArraw(const CPointD&, CPointD[][3]) const
{
	return FALSE;
}

void CDXFpoint::SetDirectionFixed(const CPointD&)
{
}

int CDXFpoint::GetIntersectionPoint(const CDXFdata*, CPointD[], BOOL) const
{
	return 0;
}

optional<CPointD>
CDXFpoint::CalcOffsetIntersectionPoint(const CDXFdata*, double, BOOL) const
{
	return optional<CPointD>();
}

int CDXFpoint::CheckIntersectionCircle(const CPointD&, double) const
{
	return 0;
}

optional<CPointD>
CDXFpoint::CalcExpandPoint(const CDXFdata*) const
{
	return optional<CPointD>();
}

//////////////////////////////////////////////////////////////////////
// ＤＸＦデータのLineクラス
//////////////////////////////////////////////////////////////////////
CDXFline::CDXFline() : CDXFpoint(DXFLINEDATA, NULL, 2)
{
}

CDXFline::CDXFline(ENDXFTYPE enType, CLayerData* pLayer, int nPoint) :
	CDXFpoint(enType, pLayer, nPoint)
{
}

CDXFline::CDXFline(LPDXFLARGV lpLine) : CDXFpoint(DXFLINEDATA, lpLine->pLayer, 2)
{
	m_pt[0] = lpLine->s;
	m_pt[1] = lpLine->e;
	SetMaxRect();
}

CDXFline::CDXFline(CLayerData* pLayer, const CDXFline* pData, LPDXFBLOCK lpBlock) :
	CDXFpoint(DXFLINEDATA, pLayer, 2)
{
	m_pt[0] = pData->GetNativePoint(0);
	m_pt[1] = pData->GetNativePoint(1);
	if ( lpBlock->dwBlockFlg & DXFBLFLG_X ) {
		m_pt[0].x *= lpBlock->dMagni[NCA_X];
		m_pt[1].x *= lpBlock->dMagni[NCA_X];
	}
	if ( lpBlock->dwBlockFlg & DXFBLFLG_Y ) {
		m_pt[0].y *= lpBlock->dMagni[NCA_Y];
		m_pt[1].y *= lpBlock->dMagni[NCA_Y];
	}
	if ( lpBlock->dwBlockFlg & DXFBLFLG_R ) {
		double	dRound = lpBlock->dRound*RAD;
		m_pt[0].RoundPoint(dRound);
		m_pt[1].RoundPoint(dRound);
	}
	m_pt[0] += lpBlock->ptOrg;
	m_pt[1] += lpBlock->ptOrg;
	SetMaxRect();
}

void CDXFline::Serialize(CArchive& ar)
{
	CDXFdata::Serialize(ar);
	if ( ar.IsStoring() )
		ar << m_pt[0].x << m_pt[0].y << m_pt[1].x << m_pt[1].y;
	else {
		ar >> m_pt[0].x >> m_pt[0].y >> m_pt[1].x >> m_pt[1].y;
		SetMaxRect();
	}
}

void CDXFline::DrawTuning(double f)
{
	m_ptDrawS = m_pt[0] * f;
	m_ptDrawE = m_pt[1] * f;
}

void CDXFline::Draw(CDC* pDC) const
{
#ifdef _DEBUGDRAW_DXF
	CMagaDbg	dbg("CDXFline::Draw()", DBG_RED);
	dbg.printf("pts.x=%d pts.y=%d pte.x=%d pte.y=%d",
		m_ptDrawS.x, m_ptDrawS.y, m_ptDrawE.x, m_ptDrawE.y);
#endif
	pDC->MoveTo(m_ptDrawS);
	pDC->LineTo(m_ptDrawE);
#ifdef _DEBUGDRAW_DXF_EDGE_
	CRect	rc(m_ptDrawE.x-10, m_ptDrawE.y-10, m_ptDrawE.x+10, m_ptDrawE.y+10);
	pDC->Ellipse(&rc);
#endif
}

double CDXFline::OrgTuning(BOOL bCalc/*=TRUE*/)
{
	ASSERT(ms_pData);
	m_ptTun[0] = m_pt[0] - ms_ptOrg;
	m_ptTun[1] = m_pt[1] - ms_ptOrg;
	OrgTuningBase();
#ifdef _DEBUG
	CMagaDbg	dbg;
	dbg.printf("OrgTuning Line sx=%f sy=%f ex=%f ey=%f",
					m_ptTun[0].x, m_ptTun[0].y, m_ptTun[1].x, m_ptTun[1].y);
#endif
	return bCalc ? GetEdgeGap(ms_pData->GetEndCutterPoint()) : 0.0;
}

double CDXFline::GetSelectPointGap(const CPointD& pt)
{
	return GetSelectPointGap_Line(m_rcMax, m_pt[0], m_pt[1], pt);
}

double CDXFline::GetSelectPointGap_Line
	(const CRectD& rcMax, const CPointD& pts, const CPointD& pte, const CPointD& pt) const
{
	double	dResult = HUGE_VAL;
	CPointD	pt1, pt2;

	// 水平垂直の場合，PtInRect() ＮＧ
	if ( fabs(pts.x-pte.x) < EPS ) {
		if ( min(pts.y, pte.y)<=pt.y && pt.y<=max(pts.y, pte.y) )
			dResult = fabs(pt.x - pts.x);
	}
	else if ( fabs(pts.y-pte.y) < EPS ) {
		if ( min(pts.x, pte.x)<=pt.x && pt.x<=max(pts.x, pte.x) )
			dResult = fabs(pt.y - pts.y);
	}
	else if ( rcMax.PtInRect(pt) ) {
		// 範囲内の場合
		pt1 = pt  - pts;	// 始点を原点に
		pt2 = pte - pts;
		double	l  = pt1.hypot(),
				qp = atan2(pt1.y, pt1.x),
				qs = atan2(pt2.y, pt2.x);
		dResult = l * fabs( sin(qp - qs) );
	}

	if ( dResult == HUGE_VAL ) {
		// 範囲外の場合，端点からの距離計算
		double	d1 = GAPCALC(pts-pt), d2 = GAPCALC(pte-pt);
		dResult = sqrt(d1<d2 ? d1 : d2);	// 近い方の距離を返す
	}

	return dResult;
}

BOOL CDXFline::GetDirectionArraw(const CPointD&, CPointD pt[][3]) const
{
	return GetDirectionArraw_Line(m_pt, pt);
}

void CDXFline::SetDirectionFixed(const CPointD& pts)
{
	// 固有座標を近い方に入れ替え
	if ( GAPCALC(m_pt[0]-pts) > GAPCALC(m_pt[1]-pts) )
		std::swap(m_pt[0], m_pt[1]);
}

int CDXFline::GetIntersectionPoint(const CDXFdata* pData, CPointD pt[], BOOL bEdge/*=TRUE*/) const
{
	int		nResult = 0;
	CPointD	pt1(pData->GetNativePoint(0)), pt2(pData->GetNativePoint(1));
	const CDXFcircle*	pCircle;
	const CDXFellipse*	pEllipse;
	optional<CPointD>	ptResult;

	// bEdge==TRUE : 端点が同じ場合は交点なしとする
	switch ( pData->GetType() ) {
	case DXFLINEDATA:
		if ( bEdge &&
			(sqrt(GAPCALC(pt1-m_pt[0])) < EPS || sqrt(GAPCALC(pt1-m_pt[1])) < EPS ||
			 sqrt(GAPCALC(pt2-m_pt[0])) < EPS || sqrt(GAPCALC(pt2-m_pt[1])) < EPS) ) {
			break;
		}
		ptResult = ::CalcIntersectionPoint_LL(m_pt[0], m_pt[1], pt1, pt2);
		if ( ptResult ) {
			pt[0] = *ptResult;
			nResult = 1;
		}
		break;
	case DXFARCDATA:
		if ( bEdge &&
			(sqrt(GAPCALC(pt1-m_pt[0])) < EPS || sqrt(GAPCALC(pt1-m_pt[1])) < EPS ||
			 sqrt(GAPCALC(pt2-m_pt[0])) < EPS || sqrt(GAPCALC(pt2-m_pt[1])) < EPS) ) {
			break;
		}
		// through
	case DXFCIRCLEDATA:
		pCircle = static_cast<const CDXFcircle*>(pData);
		tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_LC(m_pt[0], m_pt[1],
				pCircle->GetCenter(), pCircle->GetR());
		if ( pData->GetType() == DXFARCDATA ) {
			if ( nResult > 1 ) {
				if ( !pCircle->IsRangeAngle(pt2) )
					nResult--;
			}
			if ( nResult > 0 ) {
				if ( !pCircle->IsRangeAngle(pt1) ) {
					nResult--;
					if ( nResult > 0 )
						std::swap(pt1, pt2);
				}
			}
		}
		if ( nResult > 1 )
			pt[1] = pt2;
		if ( nResult > 0 )
			pt[0] = pt1;
		break;
	case DXFELLIPSEDATA:
		pEllipse = static_cast<const CDXFellipse*>(pData);
		if ( bEdge && pEllipse->IsArc() &&
			(sqrt(GAPCALC(pt1-m_pt[0])) < EPS || sqrt(GAPCALC(pt1-m_pt[1])) < EPS ||
			 sqrt(GAPCALC(pt2-m_pt[0])) < EPS || sqrt(GAPCALC(pt2-m_pt[1])) < EPS) ) {
			break;
		}
		tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_LE(m_pt[0], m_pt[1],
				pEllipse->GetCenter(), pEllipse->GetLongLength(), pEllipse->GetShortLength(), pEllipse->GetLean());
		if ( pEllipse->IsArc() ) {
			if ( nResult > 1 ) {
				if ( !pEllipse->IsRangeAngle(pt2) )
					nResult--;
			}
			if ( nResult > 0 ) {
				if ( !pEllipse->IsRangeAngle(pt1) ) {
					nResult--;
					if ( nResult > 0 )
						std::swap(pt1, pt2);
				}
			}
		}
		if ( nResult > 1 )
			pt[1] = pt2;
		if ( nResult > 0 )
			pt[0] = pt1;
		break;
	}

	return nResult;
}

optional<CPointD>
CDXFline::CalcOffsetIntersectionPoint
	(const CDXFdata* pNext, double r, BOOL bLeft) const
{
	CPointD	pto( GetNativePoint(1) ), p1, p2, pts( GetNativePoint(0) ), pt;
	int		k1, k2, nRound;
	BOOL	bResult;
	const CDXFarc*		pArc;
	const CDXFellipse*	pEllipse;
	optional<CPointD>	ptResult;

	// 交点を原点に
	p1 = pts - pto;

	// 進行方向への符号
	pt = pto - pts;
	k1 = ::CalcOffsetSign(pt);
	if ( !bLeft )
		k1 = -k1;

	switch ( pNext->GetMakeType() ) {
	case DXFLINEDATA:
		p2 = pNext->GetNativePoint(1) - pto;
		k2 = ::CalcOffsetSign(p2);
		if ( !bLeft )
			k2 = -k2;
		ptResult = ::CalcOffsetIntersectionPoint_LL(p1, p2, k1, k2, r);
		break;
	case DXFARCDATA:
		pArc = static_cast<const CDXFarc*>(pNext);
		p2 = pArc->GetCenter() - pto;
		k2 = nRound = pArc->GetRoundOrig() ? -1 : 1;	// 反時計はﾏｲﾅｽ符号
		if ( !bLeft )
			k2 = -k2;
		tie(bResult, pt, r) = ::CalcOffsetIntersectionPoint_LC(p1, p2,
				pArc->GetR(), r, nRound, k1, k2);
		if ( bResult )
			ptResult = pt;
		break;
	case DXFELLIPSEDATA:
		pEllipse = static_cast<const CDXFellipse*>(pNext);
		p2 = pEllipse->GetCenter() - pto;
		ptResult = ::CalcOffsetIntersectionPoint_LE(p1, p2,
			pEllipse->GetLongLength(), pEllipse->GetShortLength(), pEllipse->GetLean(), r,
			pEllipse->GetRoundOrig(), bLeft);
		break;
	}

	if ( ptResult ) {
		pt = *ptResult + pto;
		return pt;
	}
	return ptResult;
}

int CDXFline::CheckIntersectionCircle(const CPointD& ptc, double r) const
{
	int	nResult;
	CPointD	pt1, pt2;
	tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_LC(m_pt[0], m_pt[1], ptc, r);
	// 「接する」場合の厳密ﾁｪｯｸ
	if ( nResult==1 && pt1!=pt2 )
		nResult = 2;	// 交点ありに解を変更
	return nResult;
}

optional<CPointD>
CDXFline::CalcExpandPoint(const CDXFdata* pData) const
{
	// 伸縮計算くずれ（範囲外の交点計算）
	// 基本的には GetIntersectionPoint() と同じ
	optional<CPointD>	ptResult;

	switch ( pData->GetType() ) {
	case DXFLINEDATA:
		ptResult = ::CalcIntersectionPoint_LL(m_pt[0], m_pt[1],
						pData->GetNativePoint(0), pData->GetNativePoint(1), FALSE);
		break;
	case DXFARCDATA:
		{
			int		nResult;
			CPointD	pt1, pt2;
			const CDXFarc* pArc = static_cast<const CDXFarc*>(pData);
			tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_LC(m_pt[0], m_pt[1],
						pArc->GetCenter(), pArc->GetR(), FALSE);
			if ( nResult > 1 ) {
				// 終点に近い方を選択
				ptResult = GAPCALC(pt1-m_pt[1]) < GAPCALC(pt2-m_pt[1]) ? pt1 : pt2;
			}
			else if ( nResult > 0 ) {
				ptResult = pt1;
			}
		}
		break;
	}

	return ptResult;
}

//////////////////////////////////////////////////////////////////////
// ＤＸＦデータのCircleクラス
//////////////////////////////////////////////////////////////////////
CDXFcircle::CDXFcircle() : CDXFline(DXFCIRCLEDATA, NULL, 4)
{
	m_nArrayExt = 0;
	m_bRound = FALSE;	// 円ﾃﾞｰﾀの回転方向は方向指示のみ
	m_bRoundFixed = FALSE;
}

CDXFcircle::CDXFcircle(ENDXFTYPE enType, CLayerData* pLayer,
	const CPointD& c, double r, BOOL bRound, int nPoint) :
		CDXFline(enType, pLayer, nPoint)
{
	m_nArrayExt = 0;
	m_ct	= c;
	m_r		= r;
	m_rMake	= ::RoundUp(m_r);
	m_bRound = bRound;
	m_bRoundFixed = FALSE;
}

CDXFcircle::CDXFcircle(LPDXFCARGV lpCircle) :
	CDXFline(DXFCIRCLEDATA, lpCircle->pLayer, 4)
{
	m_nArrayExt = 0;
	m_ct	= lpCircle->c;
	m_r		= lpCircle->r;
	m_rMake	= ::RoundUp(m_r);
	m_bRound = FALSE;
	m_bRoundFixed = FALSE;
	SetCirclePoint();
	SetMaxRect();
}

CDXFcircle::CDXFcircle(CLayerData* pLayer, const CDXFcircle* pData, LPDXFBLOCK lpBlock) :
	CDXFline(DXFCIRCLEDATA, pLayer, 4)
{
	m_nArrayExt = 0;
	m_ct	= pData->GetCenter();
	m_r		= pData->GetR();
	// 円の拡大率はX方向だけ ->
	// 　各軸で拡大縮小すると楕円になるが，ﾌﾞﾛｯｸからの生成処理(DXFDoc2.cpp)で調整
	if ( lpBlock->dwBlockFlg & DXFBLFLG_X ) {
		m_ct *= lpBlock->dMagni[NCA_X];
		m_r  *= lpBlock->dMagni[NCA_X];
	}
	if ( lpBlock->dwBlockFlg & DXFBLFLG_R )
		m_ct.RoundPoint(lpBlock->dRound*RAD);
	m_ct += lpBlock->ptOrg;
	m_rMake = pData->GetMakeR();
	m_bRound = pData->GetRound();
	m_bRoundFixed = pData->IsRoundFixed();
	SetCirclePoint();
	SetMaxRect();
}

void CDXFcircle::Serialize(CArchive& ar)
{
	CDXFdata::Serialize(ar);
	if ( ar.IsStoring() )
		ar << m_r << m_ct.x << m_ct.y;
	else {
		ar >> m_r >> m_ct.x >> m_ct.y;
		m_rMake	= ::RoundUp(m_r);
		SetCirclePoint();
		SetMaxRect();
	}
}

void CDXFcircle::XRev(void)
{
	if ( GetMakeType() == DXFPOINTDATA ) {
		m_ptTun[0].x  = -m_ptTun[0].x;
		m_ptMake[0].x = -m_ptMake[0].x;
	}
	else {
		CDXFdata::XRev();
		m_ctTun.x = -m_ctTun.x;
	}
}

void CDXFcircle::YRev(void)
{
	if ( GetMakeType() == DXFPOINTDATA ) {
		m_ptTun[0].y  = -m_ptTun[0].y;
		m_ptMake[0].y = -m_ptMake[0].y;
	}
	else {
		CDXFdata::YRev();
		m_ctTun.y = -m_ctTun.y;
	}
}

BOOL CDXFcircle::IsRangeAngle(const CPointD&) const
{
	return TRUE;
}

void CDXFcircle::DrawTuning(double f)
{
	m_rcDraw.TopLeft()		= (m_ct - m_r) * f;
	m_rcDraw.BottomRight()	= (m_ct + m_r) * f;
}

void CDXFcircle::Draw(CDC* pDC) const
{
#ifdef _DEBUGDRAW_DXF
	CMagaDbg	dbg("CDXFcircle::Draw()", DBG_RED);
	dbg.printStruct((LPRECT)&m_rcDraw, "m_rcDraw");
#endif
	pDC->Ellipse(m_rcDraw);
}

double CDXFcircle::OrgTuning(BOOL bCalc/*=TRUE*/)
{
	ASSERT(ms_pData);
	if ( GetMakeType() == DXFPOINTDATA ) {
		m_ptTun[0] = m_ct - ms_ptOrg;
		for ( int i=1; i<SIZEOF(m_ptTun); i++ )
			m_ptTun[i] = m_ptTun[0];
	}
	else {
		m_ctTun = m_ct - ms_ptOrg;
		// 0～1 -> X軸
		m_ptTun[0].x = m_ctTun.x + m_r;
		m_ptTun[1].x = m_ctTun.x - m_r;
		m_ptTun[0].y = m_ptTun[1].y = m_ctTun.y;
		// 2～3 -> Y軸
		m_ptTun[2].y = m_ctTun.y + m_r;
		m_ptTun[3].y = m_ctTun.y - m_r;
		m_ptTun[2].x = m_ptTun[3].x = m_ctTun.x;
		//
		m_nArrayExt = 0;
	}
	m_bRoundFixed = FALSE;	// 方向指示の解除
	OrgTuningBase();
#ifdef _DEBUG
	CMagaDbg	dbg;
	dbg.printf("OrgTuning Circle s1=(%f, %f) s2=(%f, %f)", 
					m_ptTun[0].x, m_ptTun[0].y, m_ptTun[1].x, m_ptTun[1].y);
	dbg.printf("                 s3=(%f, %f) s4=(%f, %f)", 
					m_ptTun[2].x, m_ptTun[2].y, m_ptTun[3].x, m_ptTun[3].y);
#endif
	if ( GetMakeType() == DXFPOINTDATA )
		return bCalc ? (*ms_pfnOrgDrillTuning)(this) : 0.0;	// CDXFpoint::OrgTuning()
	else
		return bCalc ? GetEdgeGap(ms_pData->GetEndCutterPoint()) : 0.0;
}

double CDXFcircle::GetSelectPointGap(const CPointD& pt)
{
	return GetSelectPointGap_Circle(pt, 0.0, 360.0*RAD);
}

BOOL CDXFcircle::GetDirectionArraw(const CPointD& ptClick, CPointD ptResult[][3]) const
{
	CPointD	pt[2];
	// 1/4始点終点を取得
	GetQuarterPoint(ptClick, pt);
	// 接線の傾きを求める -> 点ptに直交する線の傾き
	double	q[] = {
		-atan2(pt[0].x - m_ct.x, pt[0].y - m_ct.y)+180.0*RAD,
		-atan2(pt[1].x - m_ct.x, pt[1].y - m_ct.y)
	};
	// 接線から矢印座標の計算
	return GetDirectionArraw_Circle(TRUE, q, pt, ptResult);
}

void CDXFcircle::SetDirectionFixed(const CPointD&)
{
	// 座標入れ替え不要
}

void CDXFcircle::SetRoundFixed(const CPointD& pts, const CPointD& pte)
{
	int	nLoop = GetPointNumber() - 1;
	// 回転方向の判定
	for ( int i=0; i<nLoop; i++ ) {
		if ( m_pt[i] == pts ) {
			m_bRound = m_pt[i+1] == pte ? TRUE : FALSE;
			return;
		}
	}
	if ( m_pt[i] == pts )	// 270°ｽﾀｰﾄ
		m_bRound = m_pt[i-1] == pte ? FALSE : TRUE;
	// 回転方向の固定
	m_bRoundFixed = TRUE;
}

int CDXFcircle::GetIntersectionPoint(const CDXFdata* pData, CPointD pt[], BOOL) const
{
	int		nResult = 0;
	CPointD	pt1, pt2;
	const CDXFcircle*	pCircle;

	switch ( pData->GetType() ) {
	case DXFLINEDATA:
		tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_LC(
				pData->GetNativePoint(0), pData->GetNativePoint(1), m_ct, m_r);
		break;
	case DXFCIRCLEDATA:
	case DXFARCDATA:
		pCircle = static_cast<const CDXFcircle*>(pData);
		tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_CC(
				m_ct, pCircle->GetCenter(), m_r, pCircle->GetR());
		if ( pData->GetType() == DXFARCDATA ) {
			if ( nResult > 1 ) {
				if ( !pCircle->IsRangeAngle(pt2) )
					nResult--;
			}
			if ( nResult > 0 ) {
				if ( !pCircle->IsRangeAngle(pt1) ) {
					nResult--;
					if ( nResult > 0 )
						std::swap(pt1, pt2);
				}
			}
		}
		break;
	case DXFELLIPSEDATA:
		// 数値計算完了後、再ｺｰﾃﾞｨﾝｸﾞ
		pt[0] = pt[1] = 0;
		return 2;	// とりあえず交点があることにしておく
	}

	if ( nResult > 1 )
		pt[1] = pt2;
	if ( nResult > 0 )
		pt[0] = pt1;

	return nResult;
}

optional<CPointD>
CDXFcircle::CalcOffsetIntersectionPoint(const CDXFdata*, double, BOOL) const
{
	return optional<CPointD>();
}

int CDXFcircle::CheckIntersectionCircle(const CPointD&, double) const
{
	return 0;
}

optional<CPointD>
CDXFcircle::CalcExpandPoint(const CDXFdata*) const
{
	return optional<CPointD>();
}

//////////////////////////////////////////////////////////////////////
// ＤＸＦデータのCircleExクラス
//////////////////////////////////////////////////////////////////////
CDXFcircleEx::CDXFcircleEx() : CDXFcircle(DXFCIRCLEDATA, NULL, CPointD(), 0.0, FALSE, 0)
{
}

CDXFcircleEx::CDXFcircleEx(ENDXFTYPE2 enType2, CLayerData* pLayer, const CPointD& pt, double r) :
	CDXFcircle(DXFCIRCLEDATA, pLayer, pt, r, FALSE, 0)
{
	m_enType2 = enType2;
	SetMaxRect();
}

void CDXFcircleEx::Serialize(CArchive& ar)
{
	int		nType;

	CDXFdata::Serialize(ar);
	if ( ar.IsStoring() ) {
		nType = m_enType2;
		ar << nType;
		ar << m_r << m_ct.x << m_ct.y;
	}
	else {
		ar >> nType;
		ar >> m_r >> m_ct.x >> m_ct.y;
		if ( nType < 0 || nType > 1 )	// DXFORGDATA ～ DXFSTADATA
			AfxThrowUserException();
		m_rMake	= ::RoundUp(m_r);
		m_enType2 = (ENDXFTYPE2)nType;
		SetMaxRect();
	}
}

void CDXFcircleEx::XRev(void)
{
	m_ctTun.x  = -m_ctTun.x;
	m_ctMake.x = -m_ctMake.x;
}

void CDXFcircleEx::YRev(void)
{
	m_ctTun.y  = -m_ctTun.y;
	m_ctMake.y = -m_ctMake.y;
}

void CDXFcircleEx::Draw(CDC* pDC) const
{
#ifdef _DEBUGDRAW_DXF
	CMagaDbg	dbg("CDXFcircleEx::Draw()", DBG_RED);
	dbg.printStruct((LPRECT)&m_rcDraw, "m_rcDraw");
#endif
	if ( m_enType2 == DXFSTADATA ) {
		// 加工開始位置を表す円
		pDC->Ellipse(&m_rcDraw);
	}
	// 原点を表す十字
	CPoint	pt(m_rcDraw.CenterPoint());
	pDC->MoveTo(m_rcDraw.left,  pt.y);
	pDC->LineTo(m_rcDraw.right, pt.y);
	pDC->MoveTo(pt.x, m_rcDraw.top);
	pDC->LineTo(pt.x, m_rcDraw.bottom);
}

double CDXFcircleEx::OrgTuning(BOOL/*=TRUE*/)
{
	m_ctTun = m_ct - ms_ptOrg;
	m_ctMake = m_ctTun.RoundUp();
	OrgTuningBase();
	return 0.0;		// dummy
}

//////////////////////////////////////////////////////////////////////
// ＤＸＦデータのArcクラス
//////////////////////////////////////////////////////////////////////
CDXFarc::CDXFarc() : CDXFcircle(DXFARCDATA, NULL, CPointD(), 0.0, TRUE, 2)
{
}

CDXFarc::CDXFarc(ENDXFTYPE enType, CLayerData* pLayer,
	const CPointD& c, double r, double sq, double eq, BOOL bRound, int nPoint) :
		CDXFcircle(enType, pLayer, c, r, bRound, nPoint)
{
	m_bRoundOrig = m_bRound;
	m_sq = sq;
	m_eq = eq;
	AngleTuning();
	m_sqDraw = m_sq;
	m_eqDraw = m_eq;
}

CDXFarc::CDXFarc(LPDXFAARGV lpArc) :
	CDXFcircle(DXFARCDATA, lpArc->pLayer, lpArc->c, lpArc->r, TRUE, 2)
{
	m_bRoundOrig = m_bRound;
	m_sq = lpArc->sq * RAD;
	m_eq = lpArc->eq * RAD;
	AngleTuning();
	m_sqDraw = m_sq;
	m_eqDraw = m_eq;
	m_pt[0].SetPoint(m_r * cos(m_sq), m_r * sin(m_sq));
	m_pt[1].SetPoint(m_r * cos(m_eq), m_r * sin(m_eq));
	m_pt[0] += m_ct;
	m_pt[1] += m_ct;
#ifdef _DEBUG
	CMagaDbg	dbg("CDXFarc::CDXFarc()", DBG_RED);
	dbg.printf("sx=%f sy=%f", m_pt[0].x, m_pt[0].y);
	dbg.printf("ex=%f ey=%f", m_pt[1].x, m_pt[1].y);
#endif
	SetMaxRect();
	SetRsign();
}

CDXFarc::CDXFarc(LPDXFAARGV lpArc, BOOL bRound, const CPointD& pts, const CPointD& pte) :
	CDXFcircle(DXFARCDATA, lpArc->pLayer, lpArc->c, lpArc->r, bRound, 2)
{
	m_bRoundOrig = m_bRound;
	m_sqDraw = m_sq = lpArc->sq;		// 既にﾗｼﾞｱﾝ単位，調整済み
	m_eqDraw = m_eq = lpArc->eq;
	m_pt[0]  = pts;
	m_pt[1]  = pte;
	SetMaxRect();
	SetRsign();
}

CDXFarc::CDXFarc(CLayerData* pLayer, const CDXFarc* pData, LPDXFBLOCK lpBlock) :
	CDXFcircle(DXFARCDATA, pLayer, pData->GetCenter(), pData->GetR(), pData->GetRoundOrig(), 2)
{
	m_bRoundOrig = m_bRound;
	m_sq	= pData->GetStartAngle();
	m_eq	= pData->GetEndAngle();
	m_pt[0]	= pData->GetNativePoint(0);
	m_pt[1]	= pData->GetNativePoint(1);
	// 円弧の拡大率はX方向だけ ->
	// 　各軸で拡大縮小すると楕円弧になるが，ﾌﾞﾛｯｸからの生成処理(DXFDoc2.cpp)で調整
	if ( lpBlock->dwBlockFlg & DXFBLFLG_X ) {
		m_ct	*= lpBlock->dMagni[NCA_X];
		m_pt[0]	*= lpBlock->dMagni[NCA_X];
		m_pt[1]	*= lpBlock->dMagni[NCA_X];
		m_r		*= lpBlock->dMagni[NCA_X];
		m_rMake = ::RoundUp(m_r);
	}
	if ( lpBlock->dwBlockFlg & DXFBLFLG_R ) {
		double	dRound = lpBlock->dRound*RAD;
		m_ct.RoundPoint(dRound);
		m_sq += dRound;		m_eq += dRound;
		m_pt[0].RoundPoint(dRound);
		m_pt[1].RoundPoint(dRound);
	}
	m_sqDraw = m_sq;
	m_eqDraw = m_eq;
	SetRsign();
	m_ct	+= lpBlock->ptOrg;
	m_pt[0]	+= lpBlock->ptOrg;
	m_pt[1]	+= lpBlock->ptOrg;
	// 円弧の SetMaxRect() は処理が重いので尺度等が指定されたときだけ再計算
	if ( lpBlock->dwBlockFlg&DXFBLFLG_X || lpBlock->dwBlockFlg&DXFBLFLG_R )
		SetMaxRect();
	else {
		m_rcMax	= pData->GetMaxRect();
		m_rcMax.OffsetRect(lpBlock->ptOrg);
	}
}

void CDXFarc::Serialize(CArchive& ar)
{
	CDXFdata::Serialize(ar);
	if ( ar.IsStoring() )
		ar << m_r << m_ct.x << m_ct.y << m_sqDraw << m_eqDraw << m_bRoundOrig;
	else {
		ar >> m_r >> m_ct.x >> m_ct.y >> m_sqDraw >> m_eqDraw >> m_bRoundOrig;
		m_rMake	= ::RoundUp(m_r);
		m_sq = m_sqDraw;
		m_eq = m_eqDraw;
		SetRsign();
		m_pt[0].SetPoint(m_r * cos(m_sq), m_r * sin(m_sq));
		m_pt[1].SetPoint(m_r * cos(m_eq), m_r * sin(m_eq));
		m_pt[0] += m_ct;
		m_pt[1] += m_ct;
		m_bRound = m_bRoundOrig;
		SetMaxRect();
	}
}

void CDXFarc::SetMaxRect(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CDXFarc::SetMaxRect()", DBG_RED);
#endif
	CPointD	pts, pte, ptInit[4];
	double	sq, eq;

	// 始点終点と角度の調整
	if ( m_bRoundOrig ) {
		pts = m_pt[0] - m_ct;
		pte = m_pt[1] - m_ct;
		sq = m_sqDraw;	eq = m_eqDraw;
	}
	else {
		pts = m_pt[1] - m_ct;
		pte = m_pt[0] - m_ct;
		sq = m_eqDraw;	eq = m_sqDraw;
	}

	// 各象限の軸最大値
	ptInit[0].SetPoint(  m_r,    0 );
	ptInit[1].SetPoint(    0,  m_r );
	ptInit[2].SetPoint( -m_r,    0 );
	ptInit[3].SetPoint(    0, -m_r );

	// ２点の矩形は必ず通るので，
	// 初期値として最大値・最小値を代入
	// ﾃﾞｶﾙﾄ座標なので、topとbottomは逆
	m_rcMax.left	= min(pts.x, pte.x);
	m_rcMax.top		= max(pts.y, pte.y);
	m_rcMax.right	= max(pts.x, pte.x);
	m_rcMax.bottom	= min(pts.y, pte.y);

	// 角度の調整と開始終了象限(i,j)の設定
	int	i = 0, j = 0;
	while ( sq >= 90.0*RAD ) {
		sq -= 90.0*RAD;
		i++;
	}
	while ( eq >= 90.0*RAD ) {
		eq -= 90.0*RAD;
		j++;
	}
	// i から見て j が何象限先にあるか
	int	nCnt = ( j - i ) % 4;
	if ( nCnt==0 && sq>=eq )
		nCnt = 4;

	// 象限通過ごとに軸最大値(r)を代入(topとbottomは逆)
	int	a;
	for  ( j=1; j<=nCnt; j++ ) {
		a = ( i + j ) % 4;
		if ( m_rcMax.left > ptInit[a].x )
			m_rcMax.left = ptInit[a].x;
		if ( m_rcMax.top < ptInit[a].y )
			m_rcMax.top = ptInit[a].y;
		if ( m_rcMax.right < ptInit[a].x )
			m_rcMax.right = ptInit[a].x;
		if ( m_rcMax.bottom > ptInit[a].y )
			m_rcMax.bottom = ptInit[a].y;
	}

	// 中心座標補正
	m_rcMax.OffsetRect(m_ct);
	m_rcMax.NormalizeRect();
#ifdef _DEBUG
	dbg.printf("l=%.3f t=%.3f r=%.3f b=%.3f",
		m_rcMax.left, m_rcMax.top, m_rcMax.right, m_rcMax.bottom);
#endif
}

void CDXFarc::XRev(void)
{
	CDXFdata::XRev();
	m_ctTun.x = -m_ctTun.x;
	SwapRound();	// 円弧の場合は回転方向も変更
}

void CDXFarc::YRev(void)
{
	CDXFdata::YRev();
	m_ctTun.y = -m_ctTun.y;
	SwapRound();
}

BOOL CDXFarc::IsRangeAngle(const CPointD& pt) const
{
	CPointD	ptr( pt - m_ct );
	double	q = atan2(ptr.y, ptr.x);
	if ( q < 0 )
		q += 360.0*RAD;

	if ( m_bRoundOrig ) {
		if ( m_sq <= q && q <= m_eq )
			return TRUE;
	}
	else {
		if ( m_sq >= q && q >= m_eq )
			return TRUE;
	}

	return FALSE;
}

void CDXFarc::DrawTuning(double f)
{
	m_rDraw  = m_r * f;
	m_ptDraw = m_ct * f;
}

void CDXFarc::Draw(CDC* pDC) const
{
	// CDC::Arc() を使うとどうしても表示がズレる．微細線分による近似
#ifdef _DEBUGDRAW_DXF
	CMagaDbg	dbg("CDXFarc::Draw()", DBG_RED);
#endif
	double	sq, eq;
	if ( m_bRoundOrig ) {
		sq = m_sqDraw;	eq = m_eqDraw;
	}
	else {
		sq = m_eqDraw;	eq = m_sqDraw;
	}
	CPointD	pt(m_rDraw * cos(sq) + m_ptDraw.x, m_rDraw * sin(sq) + m_ptDraw.y);
	CPointD	ptBak(pt);
	pDC->MoveTo(pt);
#ifdef _DEBUGDRAW_DXF
	dbg.printf("pts.x=%d pts.y=%d", (int)pt.x, (int)pt.y);
#endif
	for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
		pt.SetPoint(m_rDraw * cos(sq), m_rDraw * sin(sq));
		pt += m_ptDraw;
		pDC->LineTo(pt);
	}
	pt.SetPoint(m_rDraw * cos(eq), m_rDraw * sin(eq));
	pt += m_ptDraw;
	pDC->LineTo(pt);
#ifdef _DEBUGDRAW_DXF
	dbg.printf("pte.x=%d pte.y=%d", (int)pt.x, (int)pt.y);
#endif
#ifdef _DEBUGDRAW_DXF_EDGE_
	pt.SetPoint(m_rDraw * cos(m_eqDraw), m_rDraw * sin(m_eqDraw));
	pt += m_ptDraw;
	CRect	rc(pt.x-10, pt.y-10, pt.x+10, pt.y+10);
	pDC->Ellipse(&rc);
#endif
}

double CDXFarc::OrgTuning(BOOL bCalc/*=TRUE*/)
{
	m_bRound = m_bRoundOrig;	// 回転方向を初期状態に戻す
	m_sq = m_sqDraw;
	m_eq = m_eqDraw;
	m_ctTun = m_ct - ms_ptOrg;
	// 以下 CDXFline と同じ
#ifdef _DEBUG
	double	dResult = CDXFline::OrgTuning(bCalc);
	CMagaDbg	dbg;
	dbg.printf("   Round=%s cx=%f cy=%f", m_bRound ? "CCW" : "CW", m_ctTun.x, m_ctTun.y);
	dbg.printf("   sq=%f eq=%f", m_sq*DEG, m_eq*DEG);
	return dResult;
#else
	return CDXFline::OrgTuning(bCalc);
#endif
}

double CDXFarc::GetSelectPointGap(const CPointD& pt)
{
	double	sq, eq;
	if ( m_bRoundOrig ) {
		sq = m_sqDraw;	eq = m_eqDraw;
	}
	else {
		sq = m_eqDraw;	eq = m_sqDraw;
	}
	return GetSelectPointGap_Circle(pt, sq, eq);
}

BOOL CDXFarc::GetDirectionArraw(const CPointD&, CPointD pt[][3]) const
{
	// 接線の傾きを求める -> 点ptに直交する線の傾き
	double	q[] = {
		-atan2(m_pt[0].x - m_ct.x, m_pt[0].y - m_ct.y)+180.0*RAD,
		-atan2(m_pt[1].x - m_ct.x, m_pt[1].y - m_ct.y)
	};
	// 接線から矢印座標の計算
	return GetDirectionArraw_Circle(m_bRoundOrig, q, m_pt, pt);
}

void CDXFarc::SetDirectionFixed(const CPointD& pts)
{
	// 固有座標の入れ替えと回転方向の判定
	if ( GAPCALC(m_pt[0]-pts) > GAPCALC(m_pt[1]-pts) ) {
		std::swap(m_pt[0], m_pt[1]);
		// 回転方向
		m_bRound = m_bRoundOrig = !m_bRoundOrig;
		std::swap(m_sq, m_eq);
		std::swap(m_sqDraw, m_eqDraw);
	}
}

int CDXFarc::GetIntersectionPoint(const CDXFdata* pData, CPointD pt[], BOOL bEdge/*=TRUE*/) const
{
	int		nResult = 0;
	CPointD	pt1(pData->GetNativePoint(0)), pt2(pData->GetNativePoint(1));
	const CDXFcircle*	pCircle;
	const CDXFellipse*	pEllipse;

	// bEdge==TRUE : 端点が同じ場合は交点なしとする
	switch ( pData->GetType() ) {
	case DXFLINEDATA:
		if ( bEdge &&
			(sqrt(GAPCALC(pt1-m_pt[0])) < EPS || sqrt(GAPCALC(pt1-m_pt[1])) < EPS ||
			 sqrt(GAPCALC(pt2-m_pt[0])) < EPS || sqrt(GAPCALC(pt2-m_pt[1])) < EPS) ) {
			break;
		}
		tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_LC(pt1, pt2, m_ct, m_r);
		break;
	case DXFARCDATA:
		if ( bEdge &&
			(sqrt(GAPCALC(pt1-m_pt[0])) < EPS || sqrt(GAPCALC(pt1-m_pt[1])) < EPS ||
			 sqrt(GAPCALC(pt2-m_pt[0])) < EPS || sqrt(GAPCALC(pt2-m_pt[1])) < EPS) ) {
			break;
		}
		// through
	case DXFCIRCLEDATA:
		pCircle = static_cast<const CDXFcircle*>(pData);
		tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_CC(m_ct, pCircle->GetCenter(), m_r, pCircle->GetR());
		break;
	case DXFELLIPSEDATA:
		pEllipse = static_cast<const CDXFellipse*>(pData);
		if ( bEdge && pEllipse->IsArc() &&
			(sqrt(GAPCALC(pt1-m_pt[0])) < EPS || sqrt(GAPCALC(pt1-m_pt[1])) < EPS ||
			 sqrt(GAPCALC(pt2-m_pt[0])) < EPS || sqrt(GAPCALC(pt2-m_pt[1])) < EPS) ) {
			break;
		}
		// 数値計算完了後、再ｺｰﾃﾞｨﾝｸﾞ
		pt[0] = pt[1] = 0;
		return 2;	// とりあえず交点があることにしておく
	}

	if ( nResult > 1 ) {
		if ( !IsRangeAngle(pt2) ||
				(pData->GetType()==DXFARCDATA && !pCircle->IsRangeAngle(pt2)) )
			nResult--;
	}
	if ( nResult > 0 ) {
		if ( !IsRangeAngle(pt1) ||
				(pData->GetType()==DXFARCDATA && !pCircle->IsRangeAngle(pt1)) ) {
			nResult--;
			if ( nResult > 0 )
				std::swap(pt1, pt2);
		}
	}

	if ( nResult > 1 )
		pt[1] = pt2;
	if ( nResult > 0 )
		pt[0] = pt1;

	return nResult;
}

optional<CPointD>
CDXFarc::CalcOffsetIntersectionPoint
	(const CDXFdata* pNext, double r, BOOL bLeft) const
{
	CPointD	pto( GetNativePoint(1) ), p1, p2, ptResult;
	int		k1, k2, nResult, nRound;
	BOOL	bResult = FALSE;
	const CDXFarc*	pArc;

	switch ( pNext->GetMakeType() ) {
	case DXFLINEDATA:
		// 直線ｽﾀｰﾄで考えるため、回転方向などを反対にする
		p1 = pNext->GetNativePoint(1) - pto;
		p2 = m_ct - pto;
		k1 = -(::CalcOffsetSign(p1));			// 始点終点が逆なのでﾏｲﾅｽ符号
		k2 = nRound = m_bRoundOrig ? 1 : -1;	// 反時計はﾌﾟﾗｽ符号
		if ( bLeft ) {		// 進行方向が逆
			k1 = -k1;
			k2 = -k2;
		}
		tie(bResult, ptResult, r) = ::CalcOffsetIntersectionPoint_LC(p1, p2, m_r, r,
				nRound, k1, k2);
		if ( bResult )
			ptResult += pto;
		break;
	case DXFARCDATA:
		pArc = static_cast<const CDXFarc*>(pNext);
		k1 = m_bRoundOrig ? -1 : 1;
		k2 = pArc->GetRoundOrig() ? -1 : 1;
		if ( !bLeft ) {
			k1 = -k1;
			k2 = -k2;
		}
		tie(nResult, p1, p2) = ::CalcIntersectionPoint_CC(m_ct, pArc->GetCenter(),
				m_r+r*k1, pArc->GetR()+r*k2);
		if ( nResult > 0 ) {
			ptResult = GAPCALC(p1-pto) < GAPCALC(p2-pto) ? p1 : p2;
			bResult = TRUE;
		}
		else
			bResult = FALSE;
		break;
	}

	return bResult ? ptResult : optional<CPointD>();
}

int CDXFarc::CheckIntersectionCircle(const CPointD& ptc, double r) const
{
	int	nResult;
	CPointD	pt1, pt2;
	tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_CC(m_ct, ptc, m_r, r);
	// 交点を厳密に求めるわけではない
	if ( nResult > 1 && !IsRangeAngle(pt2) )
		nResult--;
	if ( nResult > 0 && !IsRangeAngle(pt1) )
		nResult--;

	return nResult;
}

optional<CPointD>
CDXFarc::CalcExpandPoint(const CDXFdata* pData) const
{
	int		nResult;
	CPointD	pt1, pt2;
	optional<CPointD>	ptResult;

	switch ( pData->GetType() ) {
	case DXFLINEDATA:
		tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_LC(
				pData->GetNativePoint(0), pData->GetNativePoint(1),
				m_ct, m_r, FALSE);
		break;
	case DXFARCDATA:
		tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_CC(
				m_ct, static_cast<const CDXFarc*>(pData)->GetCenter(),
				m_r,  static_cast<const CDXFarc*>(pData)->GetR());
		break;
	}

	if ( nResult > 1 ) {
		// 終点に近い方を選択
		ptResult = GAPCALC(pt1-m_pt[1]) < GAPCALC(pt2-m_pt[1]) ? pt1 : pt2;
	}
	else if ( nResult > 0 ) {
		ptResult = pt1;
	}

	return ptResult;
}

//////////////////////////////////////////////////////////////////////
// ＤＸＦデータのEllipseクラス
//////////////////////////////////////////////////////////////////////
CDXFellipse::CDXFellipse() : CDXFarc(DXFELLIPSEDATA, NULL, CPointD(), 0, 0, 0, TRUE, 4)
{
}

CDXFellipse::CDXFellipse(LPDXFEARGV lpEllipse) :
	CDXFarc(DXFELLIPSEDATA, lpEllipse->pLayer,
		lpEllipse->c, 0.0, lpEllipse->sq, lpEllipse->eq, lpEllipse->bRound, 4)
{
	m_ptLong = lpEllipse->l;
	m_dShort = lpEllipse->s;
	// 楕円の傾きを計算
	m_lqMake = m_lq = atan2(m_ptLong.y, m_ptLong.x);
	// 初期化処理
	Construct();
	// 各種計算
	EllipseCalc();
}

CDXFellipse::CDXFellipse(CLayerData* pLayer, const CDXFellipse* pData, LPDXFBLOCK lpBlock) :
	CDXFarc(DXFELLIPSEDATA, pLayer, pData->GetCenter(), pData->GetR(),
		pData->GetStartAngle(), pData->GetEndAngle(), pData->GetRoundOrig(), 4 )
{
	m_bArc   = pData->IsArc();
	m_ptLong = pData->GetLongPoint();
	m_dShort = pData->GetShortMagni();
	if ( lpBlock->dwBlockFlg & DXFBLFLG_X ) {
		m_ct.x *= lpBlock->dMagni[NCA_X];
		m_ptLong.x *= lpBlock->dMagni[NCA_X];
	}
	if ( lpBlock->dwBlockFlg & DXFBLFLG_Y ) {
		m_ct.y *= lpBlock->dMagni[NCA_Y];
		m_ptLong.y *= lpBlock->dMagni[NCA_Y];
	}
	if ( lpBlock->dwBlockFlg & DXFBLFLG_R ) {
		double	dRound = lpBlock->dRound*RAD;
		m_ct.RoundPoint(dRound);
		// 楕円の回転は，傾き(m_lq)でｶﾊﾞｰ
		m_ptLong.RoundPoint(dRound);
	}
	m_ct += lpBlock->ptOrg;
	if ( lpBlock->dwBlockFlg&DXFBLFLG_X || lpBlock->dwBlockFlg&DXFBLFLG_Y || lpBlock->dwBlockFlg&DXFBLFLG_R ) {
		m_dLongLength = m_ptLong.hypot();
		double	len = m_dLongLength * m_dShort;
		m_r = max(m_dLongLength, len);
		m_rMake = ::RoundUp(m_r);
		m_lqMake = m_lq = atan2(m_ptLong.y, m_ptLong.x);
		EllipseCalc();
	}
	else {
		m_dLongLength	= pData->GetLongLength();
		m_lqMake = m_lq	= pData->GetLean();
		m_lqDrawCos = m_lqMakeCos = pData->GetLeanCos();
		m_lqDrawSin = m_lqMakeSin = pData->GetLeanSin();
		CPointD	pt;
		m_nPoint = pData->GetPointNumber();
		for ( int i=0; i<m_nPoint; i++ )
			m_pt[i] = pData->GetNativePoint(i) + lpBlock->ptOrg;
		m_rcMax = pData->GetMaxRect();
		m_rcMax.OffsetRect(lpBlock->ptOrg);
	}
}

void CDXFellipse::Serialize(CArchive& ar)
{
	CDXFdata::Serialize(ar);
	if ( ar.IsStoring() ) {
		ar << m_ct.x << m_ct.y << m_sqDraw << m_eqDraw << m_bRoundOrig;
		ar << m_ptLong.x << m_ptLong.y << m_dShort << m_lq;
	}
	else {
		ar >> m_ct.x >> m_ct.y >> m_sqDraw >> m_eqDraw >> m_bRoundOrig;
		ar >> m_ptLong.x >> m_ptLong.y >> m_dShort >> m_lq;
		m_sq = m_sqDraw;
		m_eq = m_eqDraw;
		m_bRound = m_bRoundOrig;
		Construct();
		EllipseCalc();
	}
}

void CDXFellipse::Construct(void)
{
	m_dLongLength = m_ptLong.hypot();
	// ｺﾝｽﾄﾗｸﾀでは m_r=0.0 を渡したが，
	// 便宜上長軸短軸の長い方を半径と見なす
	double	len = m_dLongLength * m_dShort;
	m_r = max(m_dLongLength, len);
	m_rMake = ::RoundUp(m_r);
	// 楕円か楕円弧か
	m_bArc = fabs(m_eq-m_sq)*DEG+NCMIN < 360.0 ? TRUE : FALSE;
	if ( !m_bArc ) {
		m_sqDraw = m_sq = 0;
		m_eqDraw = m_eq = 360.0*RAD;
		AngleTuning();
	}
}

void CDXFellipse::EllipseCalc(void)
{
	int		i;
	double	dShort = m_dLongLength * m_dShort;

	// 傾き計算
	m_lqDrawCos = m_lqMakeCos = cos(m_lq);
	m_lqDrawSin = m_lqMakeSin = sin(m_lq);
	// 楕円か楕円弧かで設定する値が違う
	if ( m_bArc ) {
		// 円弧と同等に始点・終点の計算
		CPointD	pt[2];
		pt[0].SetPoint(m_dLongLength * cos(m_sq), dShort * sin(m_sq));
		pt[1].SetPoint(m_dLongLength * cos(m_eq), dShort * sin(m_eq));
		for ( i=0; i<SIZEOF(pt); i++ ) {
			m_pt[i].x = pt[i].x * m_lqDrawCos - pt[i].y * m_lqDrawSin + m_ct.x;
			m_pt[i].y = pt[i].x * m_lqDrawSin + pt[i].y * m_lqDrawCos + m_ct.y;
		}
		// 使わない座標は封印
		m_nPoint = 2;
	}
	else {
		// 円と同等(各軸４点使用)
		m_pt[0].SetPoint( m_dLongLength,     0.0 );	//   0°
		m_pt[1].SetPoint(           0.0,  dShort );	//  90°
		m_pt[2].SetPoint(-m_dLongLength,     0.0 );	// 180°
		m_pt[3].SetPoint(           0.0, -dShort );	// 270°
		CPointD	pt;
		for ( i=0; i<m_nPoint; i++ ) {
			pt = m_pt[i];
			m_pt[i].x = pt.x * m_lqDrawCos - pt.y * m_lqDrawSin + m_ct.x;
			m_pt[i].y = pt.x * m_lqDrawSin + pt.y * m_lqDrawCos + m_ct.y;
		}
	}
	// 矩形の計算
	SetMaxRect();
}

void CDXFellipse::SetMaxRect(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CDXFellipse::SetMaxRect()", DBG_RED);
#endif
	CPointD	pt, pts, pte, ptc[4];
	double	sq, eq, dShort = m_dLongLength * m_dShort;

	// 角度調整
	if ( m_bRoundOrig ) {
		sq = m_sqDraw;	eq = m_eqDraw;
	}
	else {
		sq = m_eqDraw;	eq = m_sqDraw;
	}

	// 始点終点計算(m_ptには傾きが入っているので再計算)
	if ( m_bArc ) {
		pts.SetPoint( m_dLongLength * cos(sq), dShort * sin(sq) );
		pte.SetPoint( m_dLongLength * cos(eq), dShort * sin(eq) );
	}
	else {
		pte.x = pts.x = m_dLongLength;
//		pte.y = pts.y = 0.0;
	}

	// 各象限の軸最大値
	ptc[0].SetPoint(  m_dLongLength,       0 );
	ptc[1].SetPoint(              0,  dShort );
	ptc[2].SetPoint( -m_dLongLength,       0 );
	ptc[3].SetPoint(              0, -dShort );

	// ２点の矩形は必ず通るので，
	// 初期値として最大値・最小値を代入
	// ﾃﾞｶﾙﾄ座標なので、topとbottomは逆
	CRectD	rcMax( min(pts.x, pte.x), max(pts.y, pte.y),
				   max(pts.x, pte.x), min(pts.y, pte.y) );

	// 角度の調整と開始終了象限(i,j)の設定
	int	i = 0, j = 0;
	while ( sq >= 90.0*RAD ) {
		sq -= 90.0*RAD;
		i++;
	}
	while ( eq >= 90.0*RAD ) {
		eq -= 90.0*RAD;
		j++;
	}
	// i から見て j が何象限先にあるか
	int	nCnt = ( j - i ) % 4;
	if ( nCnt==0 && sq>=eq )
		nCnt = 4;

	// 象限通過ごとに軸最大値(r)を代入(topとbottomは逆)
	int	a;
	for  ( j=1; j<=nCnt; j++ ) {
		a = ( i + j ) % 4;
		if ( rcMax.left > ptc[a].x )
			rcMax.left = ptc[a].x;
		if ( rcMax.top < ptc[a].y )
			rcMax.top = ptc[a].y;
		if ( rcMax.right < ptc[a].x )
			rcMax.right = ptc[a].x;
		if ( rcMax.bottom > ptc[a].y )
			rcMax.bottom = ptc[a].y;
	}
	// 外接する四角形の４角座標
	ptc[0].SetPoint( rcMax.right, rcMax.top );
	ptc[1].SetPoint( rcMax.left,  rcMax.top );
	ptc[2].SetPoint( rcMax.left,  rcMax.bottom );
	ptc[3].SetPoint( rcMax.right, rcMax.bottom );

	// 外接する四角形を回転させて占有矩形を計算
	m_rcMax.SetRectMinimum();
	for ( i=0; i<SIZEOF(ptc); i++ ) {
		pt.x = ptc[i].x * m_lqDrawCos - ptc[i].y * m_lqDrawSin;
		pt.y = ptc[i].x * m_lqDrawSin + ptc[i].y * m_lqDrawCos;
		if ( m_rcMax.left   > pt.x )	m_rcMax.left = pt.x;
		if ( m_rcMax.top    > pt.y )	m_rcMax.top = pt.y;
		if ( m_rcMax.right  < pt.x )	m_rcMax.right = pt.x;
		if ( m_rcMax.bottom < pt.y )	m_rcMax.bottom = pt.y;
	}

	// 中心座標補正
	m_rcMax.NormalizeRect();
	m_rcMax.OffsetRect(m_ct);
#ifdef _DEBUG
	dbg.printf("l=%.3f t=%.3f r=%.3f b=%.3f",
		m_rcMax.left, m_rcMax.top, m_rcMax.right, m_rcMax.bottom);
#endif
}

void CDXFellipse::XRev(void)
{
	if ( GetMakeType()==DXFPOINTDATA || GetMakeType()==DXFCIRCLEDATA )
		CDXFcircle::XRev();
	else {
		// 傾きのない始点終点座標，かつ偏平率の無い「円」で再計算
		CPointD	pts(m_dLongLength * cos(m_sq), m_dLongLength * sin(m_sq)),
				pte(m_dLongLength * cos(m_eq), m_dLongLength * sin(m_eq));
		pts.x = -pts.x;
		pte.x = -pte.x;
		m_ctTun.x = -m_ctTun.x;
		XYRev(pts, pte);
	}
}

void CDXFellipse::YRev(void)
{
	if ( GetMakeType()==DXFPOINTDATA || GetMakeType()==DXFCIRCLEDATA )
		CDXFcircle::YRev();
	else {
		CPointD	pts(m_dLongLength * cos(m_sq), m_dLongLength * sin(m_sq)),
				pte(m_dLongLength * cos(m_eq), m_dLongLength * sin(m_eq));
		pts.y = -pts.y;
		pte.y = -pte.y;
		m_ctTun.y = -m_ctTun.y;
		XYRev(pts, pte);
	}
}

void CDXFellipse::XYRev(const CPointD& pts, const CPointD& pte)
{
	int		i;

	// 回転方向の反転
	SwapRound();
	// 角度の再計算
	m_sq = atan2(pts.y, pts.x);
	m_eq = atan2(pte.y, pte.x);
	AngleTuning();
	// 傾きの再計算(軸反転なので符号反転)
	m_lqMake = -m_lqMake;
	m_lqMakeCos = cos(m_lqMake);
	m_lqMakeSin = sin(m_lqMake);
	if ( m_bArc ) {
		// (角度再計算後)偏平率の反映
		CPointD	pt1[2], pt;
		pt1[0].SetPoint(pts.x, pts.y * m_dShort);
		pt1[1].SetPoint(pte.x, pte.y * m_dShort);
		for ( i=0; i<SIZEOF(pt1); i++ ) {		// ２点のみ
			pt = pt1[i];
			// 生成ﾃﾞｰﾀへ反映
			m_ptTun[i].x = pt.x * m_lqMakeCos - pt.y * m_lqMakeSin + m_ctTun.x;
			m_ptTun[i].y = pt.x * m_lqMakeSin + pt.y * m_lqMakeCos + m_ctTun.y;
			m_ptMake[i] = m_ptTun[i].RoundUp();
		}
	}
	else {
		SetEllipseTunPoint();
		for ( i=0; i<4; i++ )		// ４点
			m_ptMake[i] = m_ptTun[i].RoundUp();
	}
}

void CDXFellipse::SwapPt(int n)
{
	if ( !m_bArc || GetMakeType()==DXFPOINTDATA || GetMakeType()==DXFCIRCLEDATA ) {
		// 座標の入れ替えは必要なし
//		CDXFdata::SwapPt(n);	// CDXFcircle::SwapPt()
		if ( !m_bArc ) {
			// 楕円の場合、開始・終了角度を調整
			switch ( m_nArrayExt ) {
			case 1:		// 180°
				m_sq = 180.0*RAD;
				break;
			case 2:		// 90°
				m_sq = 90.0*RAD;
				break;
			case 3:		// 270°
				m_sq = 270.0*RAD;
				break;
			default:	// 0°
				m_sq = 0.0;
				break;
			}
			m_eq = m_sq + 360.0*RAD;
			AngleTuning();
		}
	}
	else
		CDXFarc::SwapPt(n);
}

void CDXFellipse::SetEllipseTunPoint(void)
{
	double	dShort = m_dLongLength * m_dShort;
	// 0～1 -> X軸
	m_ptTun[0].x =  m_dLongLength;
	m_ptTun[1].x = -m_dLongLength;
	m_ptTun[0].y = m_ptTun[1].y = 0;
	// 2～3 -> Y軸
	m_ptTun[2].y =  dShort;
	m_ptTun[3].y = -dShort;
	m_ptTun[2].x = m_ptTun[3].x = 0;
	//
	m_nArrayExt = 0;
	// 回転
	CPointD	pt;
	for ( int i=0; i<GetPointNumber(); i++ ) {
		pt = m_ptTun[i];
		m_ptTun[i].x = pt.x * m_lqMakeCos - pt.y * m_lqMakeSin + m_ctTun.x;
		m_ptTun[i].y = pt.x * m_lqMakeSin + pt.y * m_lqMakeCos + m_ctTun.y;
	}
}

void CDXFellipse::DrawTuning(double f)
{
	m_dDrawLongLength = m_dLongLength * f;
	// 中心座標の調整
	CDXFarc::DrawTuning(f);
}

void CDXFellipse::Draw(CDC* pDC) const
{
#ifdef _DEBUGDRAW_DXF
	CMagaDbg	dbg("CDXFellipse::Draw()", DBG_RED);
#endif
	double	sq, eq,
			dShort = m_dDrawLongLength * m_dShort;
	if ( m_bRoundOrig ) {
		sq = m_sqDraw;	eq = m_eqDraw;
	}
	else {
		sq = m_eqDraw;	eq = m_sqDraw;
	}
	CPointD	pt(m_dDrawLongLength * cos(sq), dShort * sin(sq));
	CPointD	ptDraw(pt.x * m_lqDrawCos - pt.y * m_lqDrawSin + m_ptDraw.x,
					pt.x * m_lqDrawSin + pt.y * m_lqDrawCos + m_ptDraw.y );
	CPointD	ptBak(ptDraw);
	pDC->MoveTo(ptDraw);
#ifdef _DEBUGDRAW_DXF
	dbg.printf("pts.x=%d pts.y=%d", (int)pt.x, (int)pt.y);
#endif
	for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
		pt.SetPoint(m_dDrawLongLength * cos(sq), dShort * sin(sq));
		ptDraw.SetPoint(pt.x * m_lqDrawCos - pt.y * m_lqDrawSin,
						pt.x * m_lqDrawSin + pt.y * m_lqDrawCos);
		ptDraw += m_ptDraw;
		pDC->LineTo(ptDraw);
	}
	pt.SetPoint(m_dDrawLongLength * cos(eq), dShort * sin(eq));
	ptDraw.SetPoint(pt.x * m_lqDrawCos - pt.y * m_lqDrawSin,
					pt.x * m_lqDrawSin + pt.y * m_lqDrawCos);
	ptDraw += m_ptDraw;
	pDC->LineTo(ptDraw);
#ifdef _DEBUGDRAW_DXF
	dbg.printf("pte.x=%d pte.y=%d", (int)pt.x, (int)pt.y);
#endif
}

double CDXFellipse::OrgTuning(BOOL bCalc/*=TRUE*/)
{
	m_lqMake = m_lq;
	m_lqMakeCos = m_lqDrawCos;
	m_lqMakeSin = m_lqDrawSin;
	double	dResult;
	if ( GetMakeType()==DXFPOINTDATA || GetMakeType()==DXFCIRCLEDATA )
		dResult = CDXFcircle::OrgTuning(bCalc);
	else if ( m_bArc )
		dResult = CDXFarc::OrgTuning(bCalc);
	else {
		m_bRound = m_bRoundOrig;
		m_sq = m_sqDraw;
		m_eq = m_eqDraw;
		m_ctTun = m_ct - ms_ptOrg;
		SetEllipseTunPoint();
		OrgTuningBase();
		dResult = bCalc ? GetEdgeGap(ms_pData->GetEndCutterPoint()) : 0.0;
	}
#ifdef _DEBUG
	CMagaDbg	dbg;
	dbg.printf("   lx=%f ly=%f LongLen=%f Short=%f",
			m_ptLong.x, m_ptLong.y, m_dLongLength, m_dShort);
	dbg.printf("   lq=%f", m_lq*DEG);
#endif
	return dResult;
}

double CDXFellipse::GetSelectPointGap(const CPointD& pt)
{
	double	sq, eq, q1, q2;
	if ( m_bRoundOrig ) {
		sq = m_sqDraw;	eq = m_eqDraw;
	}
	else {
		sq = m_eqDraw;	eq = m_sqDraw;
	}
	CPointD	pt1(pt - m_ct);
	// 傾きが無い状態で判定するようｸﾘｯｸﾎﾟｲﾝﾄを補正
	if ( fabs(m_lq) > 0.0 )
		pt1.RoundPoint(-m_lq);
	// 偏平率の補正後，角度計算
	pt1.y /= m_dShort;
	if ( (q1=atan2(pt1.y, pt1.x)) < 0.0 )
		q1 += 360.0*RAD;
	q2 = q1 + 360.0*RAD;
	// ｸﾘｯｸﾎﾟｲﾝﾄが角度の範囲内にあるか
	return ( !m_bArc || (sq <= q1 && q1 <= eq) || (sq <= q2 && q2 <= eq) ) ?
		fabs(m_dLongLength - pt1.hypot()) : HUGE_VAL;
}

BOOL CDXFellipse::GetDirectionArraw(const CPointD& ptClick, CPointD ptResult[][3]) const
{
	CPointD	pt[2], pt1;
	double	q[2];

	if ( m_bArc ) {
		// 偏平率と傾き角を考慮
		for ( int i=0; i<SIZEOF(q); i++ ) {
			pt1 = pt[i] = m_pt[i];
			pt1 -= m_ct;
			if ( fabs(m_lq) > 0.0 )
				pt1.RoundPoint(-m_lq);
			q[i] = -atan2(pt1.x, pt1.y);
			pt1.x = cos(q[i]);
			pt1.y = sin(q[i]);
			q[i] = atan2(pt1.y*m_dShort, pt1.x) + m_lq;
		}
		q[0] += 180.0*RAD;
	}
	else {
		// ｸﾘｯｸﾎﾟｲﾝﾄを補正
		pt1 = ptClick;
		if ( fabs(m_lq) > 0.0 ) {
			pt1 -= m_ct;
			pt1.RoundPoint(-m_lq);
			pt1 += m_ct;
		}
		GetQuarterPoint(pt1, pt);
		q[0] = -atan2(pt[0].x - m_ct.x, pt[0].y - m_ct.y)+180.0*RAD;
		q[1] = -atan2(pt[1].x - m_ct.x, pt[1].y - m_ct.y);
	}

	return GetDirectionArraw_Circle(m_bRoundOrig, q, pt, ptResult);
}

void CDXFellipse::SetDirectionFixed(const CPointD& pts)
{
	if ( m_bArc )	// 楕円弧の場合だけ
		CDXFarc::SetDirectionFixed(pts);
	// 楕円の場合は円同様、座標入れ替え不要
}

int CDXFellipse::GetIntersectionPoint(const CDXFdata* pData, CPointD pt[], BOOL bEdge/*=TRUE*/) const
{
	int		nResult = 0;
	CPointD	pt1(pData->GetNativePoint(0)), pt2(pData->GetNativePoint(1));
	const CDXFcircle*	pCircle;

	// bEdge==TRUE : 端点が同じ場合は交点なしとする
	switch ( pData->GetType() ) {
	case DXFLINEDATA:
		if ( bEdge && m_bArc &&
			(sqrt(GAPCALC(pt1-m_pt[0])) < EPS || sqrt(GAPCALC(pt1-m_pt[1])) < EPS ||
			 sqrt(GAPCALC(pt2-m_pt[0])) < EPS || sqrt(GAPCALC(pt2-m_pt[1])) < EPS) ) {
			break;
		}
		tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_LE(pt1, pt2,
				m_ct, m_dLongLength, m_dLongLength*m_dShort, m_lq);
		break;
	case DXFARCDATA:
		if ( bEdge && m_bArc &&
			(sqrt(GAPCALC(pt1-m_pt[0])) < EPS || sqrt(GAPCALC(pt1-m_pt[1])) < EPS ||
			 sqrt(GAPCALC(pt2-m_pt[0])) < EPS || sqrt(GAPCALC(pt2-m_pt[1])) < EPS) ) {
			break;
		}
		// through
	case DXFCIRCLEDATA:
		pCircle = static_cast<const CDXFcircle*>(pData);
		// 数値計算完了後、再ｺｰﾃﾞｨﾝｸﾞ
		pt[0] = pt[1] = 0;
		return 2;	// とりあえず交点があることにしておく
	}

	if ( m_bArc ) {
		if ( nResult > 1 ) {
			if ( !IsRangeAngle(pt2) ||
					(pData->GetType()==DXFARCDATA && !pCircle->IsRangeAngle(pt2)) )
				nResult--;
		}
		if ( nResult > 0 ) {
			if ( !IsRangeAngle(pt1) ||
					(pData->GetType()==DXFARCDATA && !pCircle->IsRangeAngle(pt1)) ) {
				nResult--;
				if ( nResult > 0 )
					std::swap(pt1, pt2);
			}
		}
	}

	if ( nResult > 1 )
		pt[1] = pt2;
	if ( nResult > 0 )
		pt[0] = pt1;

	return nResult;
}

optional<CPointD>
CDXFellipse::CalcOffsetIntersectionPoint
	(const CDXFdata* pNext, double r, BOOL bLeft) const
{
	CPointD	pto( GetNativePoint(1) ), p1, p2, pt;
	optional<CPointD>	ptResult;

	switch ( pNext->GetMakeType() ) {
	case DXFLINEDATA:
		p1 = pNext->GetNativePoint(1) - pto;
		p2 = m_ct - pto;
		ptResult = ::CalcOffsetIntersectionPoint_LE(p1, p2,
			m_dLongLength, m_dLongLength*m_dShort, m_lq, r,
			!m_bRoundOrig, !bLeft);
		break;
	}

	if ( ptResult ) {
		pt = *ptResult + pto;
		return pt;
	}
	return ptResult;
}

int CDXFellipse::CheckIntersectionCircle(const CPointD&, double) const
{
	return 0;
}

optional<CPointD>
CDXFellipse::CalcExpandPoint(const CDXFdata*) const
{
	return optional<CPointD>();
}

//////////////////////////////////////////////////////////////////////
// ＤＸＦデータのPolylineクラス
//////////////////////////////////////////////////////////////////////
CDXFpolyline::CDXFpolyline() : CDXFline(DXFPOLYDATA, NULL, 2)
{
	m_nPolyFlag = 0;
	m_bSeq = m_bSeqBak = TRUE;		// Default
	m_posSel = NULL;
	for ( int i=0; i<SIZEOF(m_nObjCnt); i++ )
		m_nObjCnt[i] = 0;
}

CDXFpolyline::CDXFpolyline(CLayerData* pLayer, const CDXFpolyline* pPoly, LPDXFBLOCK lpBlock) :
	CDXFline(DXFPOLYDATA, pLayer, 2)
{
	m_nPolyFlag = pPoly->GetPolyFlag();
	m_bSeq = m_bSeqBak = pPoly->GetSequence();
	m_posSel = NULL;
	for ( int i=0; i<SIZEOF(m_nObjCnt); i++ )
		m_nObjCnt[i] = 0;
	// 例外ｽﾛｰは上位でｷｬｯﾁ
	CDXFdata*		pDataSrc;
	CDXFdata*		pData;
	DXFEARGV		dxfEllipse;
	for ( POSITION pos=pPoly->m_ltVertex.GetHeadPosition(); pos; ) {
		pDataSrc = pPoly->m_ltVertex.GetNext(pos);
		switch ( pDataSrc->GetType() ) {
		case DXFPOINTDATA:
			pData = new CDXFpoint(NULL, static_cast<CDXFpoint*>(pDataSrc), lpBlock);
			ASSERT(pData);
			m_ltVertex.AddTail(pData);
			break;
		case DXFARCDATA:
			// 各軸独自の拡大率は CDXFarc -> CDXFellipse
			if ( lpBlock->dMagni[NCA_X] != lpBlock->dMagni[NCA_Y] ) {
				(static_cast<CDXFarc*>(pDataSrc))->SetEllipseArgv(lpBlock, &dxfEllipse);
				pData = new CDXFellipse(&dxfEllipse); 
				ASSERT(pData);
				m_ltVertex.AddTail(pData);
			}
			else {
				pData = new CDXFarc(NULL, static_cast<CDXFarc*>(pDataSrc), lpBlock);
				ASSERT(pData);
				m_ltVertex.AddTail(pData);
			}
			break;
		}
	}
	EndSeq();
}

CDXFpolyline::~CDXFpolyline()
{
	for ( POSITION pos = m_ltVertex.GetHeadPosition(); pos; )
		delete	m_ltVertex.GetNext(pos);
}

void CDXFpolyline::Serialize(CArchive& ar)
{
	CDXFdata::Serialize(ar);
	if ( ar.IsStoring() )
		ar << m_nPolyFlag << m_bSeqBak;
	else {
		ar >> m_nPolyFlag >> m_bSeqBak;
		m_bSeq = m_bSeqBak;
	}
	m_ltVertex.Serialize(ar);
	if ( !ar.IsStoring() )
		EndSeq();
}

void CDXFpolyline::XRev(void)
{
	// 各ｵﾌﾞｼﾞｪｸﾄは OrgTuning() から既に呼ばれている
	CDXFdata::XRev();
}

void CDXFpolyline::YRev(void)
{
	// 各ｵﾌﾞｼﾞｪｸﾄは OrgTuning() から既に呼ばれている
	CDXFdata::YRev();
}

void CDXFpolyline::SwapPt(int)
{
	// 自分自身の座標入れ替え
	m_bSeq = !m_bSeq;
	CDXFdata::SwapPt(0);

	// 構成ｵﾌﾞｼﾞｪｸﾄの座標入れ替え
	CDXFdata* pData;
	for ( POSITION pos=m_ltVertex.GetHeadPosition(); pos; ) {
		// CDXFpoint以外は回転方向をﾁｪﾝｼﾞ
		pData = m_ltVertex.GetNext(pos);
		if ( pData->GetType() != DXFPOINTDATA )
			pData->ReversePt();		// SwapPt() は protected宣言
	}
}

BOOL CDXFpolyline::SetVertex(LPDXFPARGV lpArgv)
{
	lpArgv->pLayer = NULL;
	CDXFpoint*	pPoint = new CDXFpoint(lpArgv);
	ASSERT(pPoint);
	m_ltVertex.AddTail(pPoint);
	return TRUE;
}

BOOL CDXFpolyline::SetVertex(LPDXFPARGV lpArgv, double dBow, const CPointD& pts)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CDXFpolyline::SetVertex()", DBG_RED);
#endif

	double	q = fabs(4 * atan(dBow));
	double	d = _hypot(lpArgv->c.x - pts.x, lpArgv->c.y - pts.y);
	// ２点が等しい場合はｵﾌﾞｼﾞｪｸﾄを生成しない
	if ( d < NCMIN )
		return TRUE;
	double	r = fabs( (d/2) / sin(q/2) );
	// 円の方程式より，(s.x, s.y)，(e.x, e.y) を中心とした
	// ２つの円の交点を求める
	CPointD	pt1, pt2;
	int		nResult;
	tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_CC(pts, lpArgv->c, r, r);
	if ( nResult < 1 )
		return FALSE;
	// CDXFarc登録準備
	DXFAARGV	dxfArc;
	dxfArc.r		= r;
	dxfArc.pLayer	= NULL;
	// どちらの解を採用するか
	BOOL	bRound;
	double	sq1, eq1, sq2, eq2;
	if ( (sq1=atan2(pts.y - pt1.y, pts.x - pt1.x)) < 0.0 )
		sq1 += 360.0*RAD;
	if ( (eq1=atan2(lpArgv->c.y - pt1.y, lpArgv->c.x - pt1.x)) < 0.0 )
		eq1 += 360.0*RAD;
	if ( nResult == 1 ) {	// 重根
		sq2 = sq1;	eq2 = eq1;
	}
	else {
		if ( (sq2=atan2(pts.y - pt2.y, pts.x - pt2.x)) < 0.0 )
			sq2 += 360.0*RAD;
		if ( (eq2=atan2(lpArgv->c.y - pt2.y, lpArgv->c.x - pt2.x)) < 0.0 )
			eq2 += 360.0*RAD;
	}
#ifdef _DEBUG
	dbg.printf("pts x=%f y=%f pte x=%f y=%f", pts.x, pts.y, lpArgv->c.x, lpArgv->c.y);
	dbg.printf("q=%f d=%f r=%f", q*DEG, d/2, r);
	dbg.printf("ptc1 x=%f y=%f sq1=%f eq1=%f", pt1.x, pt1.y, sq1*DEG, eq1*DEG);
	dbg.printf("ptc2 x=%f y=%f sq2=%f eq2=%f", pt2.x, pt2.y, sq2*DEG, eq2*DEG);
#endif

	if ( dBow > 0 ) {	// 反時計回り指定
		bRound = TRUE;
		while ( sq1 > eq1 )
			eq1 += 360.0*RAD;
		while ( sq2 > eq2 )
			eq2 += 360.0*RAD;
		if ( fabs(eq1 - sq1 - q) < fabs(eq2 - sq2 - q) ) {
			dxfArc.c	= pt1;
			dxfArc.sq	= sq1;
			dxfArc.eq	= eq1;
#ifdef _DEBUG
			dbg.printf("CCW pt1");
#endif
		}
		else {
			dxfArc.c	= pt2;
			dxfArc.sq	= sq2;
			dxfArc.eq	= eq2;
#ifdef _DEBUG
			dbg.printf("CCW pt2");
#endif
		}
	}
	else {
		bRound = FALSE;
		while ( sq1 < eq1 )
			sq1 += 360.0*RAD;
		while ( sq2 < eq2 )
			sq2 += 360.0*RAD;
		if ( fabs(sq1 - eq1 - q) < fabs(sq2 - eq2 - q) ) {
			dxfArc.c	= pt1;
			dxfArc.sq	= sq1;
			dxfArc.eq	= eq1;
#ifdef _DEBUG
			dbg.printf("CW pt1");
#endif
		}
		else {
			dxfArc.c	= pt2;
			dxfArc.sq	= sq2;
			dxfArc.eq	= eq2;
#ifdef _DEBUG
			dbg.printf("CW pt2");
#endif
		}
	}
	CDXFarc*	pArc = new CDXFarc(&dxfArc, bRound, pts, lpArgv->c);
	ASSERT(pArc);
	m_ltVertex.AddTail(pArc);

	// NCｺｰﾄﾞを生成しやすいようにCDXFpointｵﾌﾞｼﾞｪｸﾄも生成
	DXFPARGV	dxfPoint;
	dxfPoint.c = lpArgv->c;
	CDXFpoint*	pPoint = new CDXFpoint(&dxfPoint);
	ASSERT(pPoint);
	m_ltVertex.AddTail(pPoint);

	return TRUE;
}

void CDXFpolyline::EndSeq(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CDXFpolyline::EndSeq()", DBG_RED);
#endif
	ASSERT( !m_ltVertex.IsEmpty() );
	ASSERT( m_ltVertex.GetHead()->GetType() == DXFPOINTDATA );
	CDXFdata*	pData;
	// 最初と最後の点は必ずCDXFpoint
	m_pt[0] = m_ltVertex.GetHead()->GetNativePoint(0);
	m_pt[1] = m_nPolyFlag & 1 ? m_pt[0] : m_ltVertex.GetTail()->GetNativePoint(0);
	
	// SetMaxRect() and DataCount
	m_rcMax.TopLeft()     = m_pt[0];
	m_rcMax.BottomRight() = m_pt[0];
	CPointD	pt;
	for ( POSITION pos=m_ltVertex.GetHeadPosition(); pos; ) {
		pData = m_ltVertex.GetNext(pos);
		switch ( pData->GetType() ) {
		case DXFPOINTDATA:
			pt = pData->GetNativePoint(0);
			if ( m_rcMax.left > pt.x )
				m_rcMax.left = pt.x;
			if ( m_rcMax.top > pt.y )
				m_rcMax.top = pt.y;
			if ( m_rcMax.right < pt.x )
				m_rcMax.right = pt.x;
			if ( m_rcMax.bottom < pt.y )
				m_rcMax.bottom = pt.y;
			m_nObjCnt[0]++;
			break;
		case DXFARCDATA:
			m_rcMax |= pData->GetMaxRect();
			m_nObjCnt[1]++;
			m_nObjCnt[0]--;		// 終点の分を減算
			break;
		case DXFELLIPSEDATA:
			m_rcMax |= pData->GetMaxRect();
			m_nObjCnt[2]++;
			m_nObjCnt[0]--;
			break;
		}
	}
	// 始点が含まれるため「-1」で線の数
	m_nObjCnt[0]--;
#ifdef _DEBUG
	dbg.printf("LineCnt=%d ArcCnt=%d Ellipse=%d",
			m_nObjCnt[0], m_nObjCnt[1], m_nObjCnt[2]);
	dbg.printf("l=%.3f t=%.3f r=%.3f b=%.3f",
		m_rcMax.left, m_rcMax.top, m_rcMax.right, m_rcMax.bottom);
#endif
}

void CDXFpolyline::DrawTuning(double f)
{
	for ( POSITION pos=m_ltVertex.GetHeadPosition(); pos; )
		m_ltVertex.GetNext(pos)->DrawTuning(f);
}

void CDXFpolyline::Draw(CDC* pDC) const
{
#ifdef _DEBUGDRAW_DXF
	CMagaDbg	dbg("CDXFpolyline::Draw()", DBG_RED);
	int		nDbgCnt;
	CPoint	ptDbg;
#endif
	CDXFdata*	pData;
	POSITION pos = m_ltVertex.GetHeadPosition();
	// １点目は必ずCDXFpoint．閉ﾙｰﾌﾟのための座標を取得
	CPoint	pt( static_cast<CDXFpoint*>(m_ltVertex.GetNext(pos))->GetDrawPoint() );
	pDC->MoveTo(pt);

	// ２点目からﾙｰﾌﾟ
#ifdef _DEBUGDRAW_DXF
	for ( nDbgCnt=0; pos; nDbgCnt++ ) {
#else
	while ( pos ) {
#endif
		pData = m_ltVertex.GetNext(pos);
		if ( pData->GetType() == DXFPOINTDATA ) {
#ifdef _DEBUGDRAW_DXF
			ptDbg = static_cast<CDXFpoint*>(pData)->GetDrawPoint();
			dbg.printf("No.%03d: x=%d y=%d", nDbgCnt, ptDbg.x, ptDbg.y);
#endif
			pDC->LineTo( static_cast<CDXFpoint*>(pData)->GetDrawPoint() );
		}
		else {
			pData->Draw(pDC);
			m_ltVertex.GetNext(pos);	// 終点分を飛ばす
		}
	}
	// 閉じたﾎﾟﾘﾗｲﾝなら
	if ( m_nPolyFlag & 1 )
		pDC->LineTo(pt);	// 開始座標まで作図
}

double CDXFpolyline::OrgTuning(BOOL)
{
	m_bSeq = m_bSeqBak;		// 順序を初期状態に戻す
	// 各要素は値の入れ替えや原点からの距離は必要ないので OrgTuning(FALSE) で呼ぶ
	for ( POSITION pos=m_ltVertex.GetHeadPosition(); pos; )
		m_ltVertex.GetNext(pos)->OrgTuning(FALSE);
	// 以下 CDXFline と同じ
	return CDXFline::OrgTuning();
}

double CDXFpolyline::GetSelectPointGap(const CPointD& pt)
{
	CDXFdata*	pData;
	CPointD		ptf, pts, pte;
	CRectD		rc;
	double		dGap, dGapMin = HUGE_VAL;
	POSITION	pos1 = m_ltVertex.GetHeadPosition(), pos2;
	// １点目は必ずCDXFpoint．閉ﾙｰﾌﾟのための座標を取得
	ptf = pts = m_ltVertex.GetNext(pos1)->GetNativePoint(0);
	rc.TopLeft() = pts;

	// ２点目からﾙｰﾌﾟ
	while ( pos2 = pos1 ) {
		pData = m_ltVertex.GetNext(pos1);
		if ( pData->GetType() == DXFPOINTDATA ) {
			pte = pData->GetNativePoint(0);
			rc.BottomRight() = pte;
			rc.NormalizeRect();
			dGap = GetSelectPointGap_Line(rc, pts, pte, pt);
		}
		else {
			dGap = pData->GetSelectPointGap(pt);
			pte = pData->GetNativePoint(1);
			m_ltVertex.GetNext(pos1);	// 終点分を飛ばす
		}
		if ( dGap < dGapMin ) {
			dGapMin = dGap;
			m_posSel = pos2;
		}
		rc.TopLeft() = pts = pte;
	}

	// 閉じたﾎﾟﾘﾗｲﾝなら
	if ( m_nPolyFlag & 1 ) {
		rc.BottomRight() = ptf;
		rc.NormalizeRect();
		dGap = GetSelectPointGap_Line(rc, pts, ptf, pt);
		if ( dGap < dGapMin ) {
			dGapMin = dGap;
			m_posSel = m_ltVertex.GetHeadPosition();
		}
	}

	return dGapMin;
}

BOOL CDXFpolyline::GetDirectionArraw(const CPointD& ptClick, CPointD ptResult[][3]) const
{
	// ﾎﾟﾘﾗｲﾝ全体の始点終点では「閉じたﾎﾟﾘﾗｲﾝ」の矢印が書けないので
	// 該当要素(ｸﾘｯｸ点)に近いｵﾌﾞｼﾞｪｸﾄの矢印座標を求める
	ASSERT( m_posSel );	// GetSelectPointGap() でｾｯﾄされているﾊｽﾞ

	POSITION	pos = m_posSel;
	BOOL	bResult;
	CPointD	pt[2];
	CDXFdata*	pData = m_ltVertex.GetPrev(pos);

	if ( pData->GetType() == DXFPOINTDATA ) {
		if ( pos ) {
			pt[1] = pData->GetNativePoint(0);	// 終点
			pData = m_ltVertex.GetPrev(pos);
			pt[0] = pData->GetNativePoint(0);	// 始点
		}
		else {
			// 閉じたﾎﾟﾘﾗｲﾝで最後のｵﾌﾞｼﾞｪｸﾄにﾋｯﾄ
			pt[0] = pData->GetNativePoint(0);					// 始点
			pt[1] = m_ltVertex.GetTail()->GetNativePoint(0);	// 終点
		}
		bResult = GetDirectionArraw_Line(pt, ptResult);
	}
	else
		bResult = pData->GetDirectionArraw(ptClick, ptResult);

	return bResult;
}

void CDXFpolyline::SetDirectionFixed(const CPointD& pts)
{
	// 固有座標の入れ替えと順序変更
	if ( GAPCALC(m_pt[0]-pts) > GAPCALC(m_pt[1]-pts) ) {
		std::swap(m_pt[0], m_pt[1]);
		// 順序変更
		m_bSeq = m_bSeqBak = !m_bSeqBak;
		CDXFdata*	pData;
		CPointD		pt(m_pt[0]);
		for ( POSITION pos=GetFirstVertex(); pos; ) {
			// CDXFpoint以外は固有座標を入れ替え
			pData = GetNextVertex(pos);
			if ( pData->GetType() != DXFPOINTDATA ) {
				pData->SetDirectionFixed(pt);
				pt = pData->GetNativePoint(1);
			}
			else
				pt = pData->GetNativePoint(0);
		}
	}
}

int CDXFpolyline::GetIntersectionPoint(const CDXFdata*, CPointD[], BOOL) const
{
	return 0;
}

optional<CPointD>
CDXFpolyline::CalcOffsetIntersectionPoint(const CDXFdata*, double, BOOL) const
{
	return optional<CPointD>();
}

int CDXFpolyline::CheckIntersectionCircle(const CPointD&, double) const
{
	return 0;
}

optional<CPointD>
CDXFpolyline::CalcExpandPoint(const CDXFdata*) const
{
	return optional<CPointD>();
}

//////////////////////////////////////////////////////////////////////
// ＤＸＦデータのTextクラス
//////////////////////////////////////////////////////////////////////
CDXFtext::CDXFtext() : CDXFpoint(DXFTEXTDATA, NULL, 1)
{
}

CDXFtext::CDXFtext(LPDXFTARGV lpText) : CDXFpoint(DXFTEXTDATA, lpText->pLayer, 1)
{
	m_pt[0] = lpText->c;
	m_strValue = lpText->strValue;
	SetMaxRect();
}

CDXFtext::CDXFtext(CLayerData* pLayer, const CDXFtext* pData, LPDXFBLOCK lpBlock) :
	CDXFpoint(DXFTEXTDATA, pLayer, 1)
{
	m_pt[0] = pData->GetNativePoint(0);
	if ( lpBlock->dwBlockFlg & DXFBLFLG_X )
		m_pt[0].x *= lpBlock->dMagni[NCA_X];
	if ( lpBlock->dwBlockFlg & DXFBLFLG_Y )
		m_pt[0].y *= lpBlock->dMagni[NCA_Y];
	if ( lpBlock->dwBlockFlg & DXFBLFLG_R )
		m_pt[0].RoundPoint(lpBlock->dRound*RAD);
	m_pt[0] += lpBlock->ptOrg;
	m_strValue = pData->GetStrValue();
	SetMaxRect();
}

void CDXFtext::Serialize(CArchive& ar)
{
	CDXFdata::Serialize(ar);
	if ( ar.IsStoring() )
		ar << m_pt[0].x << m_pt[0].y << m_strValue;
	else {
		ar >> m_pt[0].x >> m_pt[0].y >> m_strValue;
		SetMaxRect();
	}
}

void CDXFtext::Draw(CDC* pDC) const
{
#ifdef _DEBUGDRAW_DXF
	CMagaDbg	dbg("CDXFtext::Draw()", DBG_RED);
	dbg.printStruct((LPRECT)&m_ptDraw, "m_ptDraw");
#endif
	pDC->TextOut(m_ptDraw.x, m_ptDraw.y, m_strValue);
	pDC->Ellipse(&m_rcDraw);
}

double CDXFtext::OrgTuning(BOOL/*=TRUE*/)
{
	CDXFpoint::OrgTuning(FALSE);	// 距離計算の必要なし
	return HUGE_VAL;
}
