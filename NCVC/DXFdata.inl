// DXFdata.inl: CDXFdata クラスのインライン関数
//
//////////////////////////////////////////////////////////////////////

#pragma once

//#define	_DEBUGDRAW_DXF		// 描画処理もﾛｸﾞ
#include "MagaDbgMac.h"

/////////////////////////////////////////////////////////////////////////////
// ＤＸＦデータのベースクラス
/////////////////////////////////////////////////////////////////////////////
inline ENDXFTYPE CDXFdata::GetType(void) const
{
	return m_enType;
}

inline ENDXFTYPE CDXFdata::GetMakeType(void) const
{
	return m_enMakeType;
}

inline DWORD CDXFdata::GetSerializeSeq(void) const
{
	return m_nSerialSeq;
}

inline void CDXFdata::ChangeMakeType(ENDXFTYPE enType)
{
	m_enMakeType = enType;
}

inline CLayerData* CDXFdata::GetParentLayer(void) const
{
	ASSERT( m_pParentLayer );
	return m_pParentLayer;
}

inline CDXFshape* CDXFdata::GetParentMap(void) const
{
	ASSERT( m_pParentMap );
	return m_pParentMap;
}

inline void CDXFdata::SetParentMap(CDXFshape* pShape)
{
	m_pParentMap = pShape;
}

inline BOOL CDXFdata::IsMakeFlg(void) const
{
	return m_dwFlags & DXFFLG_MAKE;
}

inline BOOL CDXFdata::IsSearchFlg(void) const
{
	return m_dwFlags & DXFFLG_SEARCH;
}

inline void CDXFdata::SetMakeFlg(void)
{
	m_dwFlags |= DXFFLG_MAKE;
}

inline void CDXFdata::SetSearchFlg(void)
{
	m_dwFlags |= DXFFLG_SEARCH;
}

inline void CDXFdata::ClearMakeFlg(void)
{
	m_dwFlags &= ~DXFFLG_MAKE;
}

inline void CDXFdata::ClearSearchFlg(void)
{
	m_dwFlags &= ~DXFFLG_SEARCH;
}

inline const CRect3D CDXFdata::GetMaxRect(void) const
{
	return m_rcMax;
}

inline DWORD CDXFdata::GetSelectFlg(void) const
{
	return m_dwSelect;
}

inline void CDXFdata::SetWorkingFlag(DWORD dwFlags, BOOL bSet/*=TRUE*/)
{
	if ( bSet )
		m_dwSelect |=  dwFlags;
	else
		m_dwSelect &= ~dwFlags;
}

inline void CDXFdata::SetSelectFlg(BOOL bSelect)
{
	SetWorkingFlag(DXFSEL_SELECT, bSelect);
}

inline int CDXFdata::GetPointNumber(void) const
{
	return m_nPoint;
}

inline const CPointD CDXFdata::GetNativePoint(size_t a) const
{
	ASSERT( a>=0 && a<(size_t)m_nPoint );
	return m_pt[a];
}

inline const CPointD CDXFdata::GetTunPoint(size_t a) const
{
	ASSERT( a>=0 && a<(size_t)m_nPoint );
	return m_ptTun[a];
}

inline const CPointD CDXFdata::GetMakePoint(size_t a) const
{
	ASSERT( a>=0 && a<(size_t)m_nPoint );
	return m_ptMake[a];
}

inline BOOL CDXFdata::IsMatchObject(const CDXFdata* pData)
{
	return IsMatchPoint( pData->GetEndMakePoint() );	// virtual指定
}

inline double CDXFdata::GetEdgeGap(const CDXFdata* pData, BOOL bSwap/*=TRUE*/)
{
	return GetEdgeGap(pData->GetEndCutterPoint(), bSwap);	// virtual指定
}

inline void CDXFdata::SwapPt(int n)		// m_ptTun の入れ替え
{
#ifdef _DEBUG
	CMagaDbg	dbg("SwapPt()", DBG_CYAN);
#endif
	if ( m_nPoint > 1 ) {	// 念のためﾁｪｯｸ
#ifdef _DEBUG
		dbg.printf("calling ok");
#endif
		swap(m_ptTun[n],  m_ptTun[n+1]);
		swap(m_ptMake[n], m_ptMake[n+1]);
	}
#ifdef _DEBUG
	else
		dbg.printf("Missing call! m_nPoint <= 1");
#endif
}

inline void CDXFdata::ReversePt(void)
{
	SwapPt(0);
}

/////////////////////////////////////////////////////////////////////////////
// ＤＸＦデータのPointクラス
/////////////////////////////////////////////////////////////////////////////
inline void CDXFpoint::SetDrawRect(const CPointD& pt, double r, double f)
{
	m_rcDraw.left   = (int)((pt.x - r) * f);
	m_rcDraw.top    = (int)((pt.y - r) * f);
	m_rcDraw.right  = (int)((pt.x + r) * f);
	m_rcDraw.bottom = (int)((pt.y + r) * f);
}

inline void CDXFpoint::SetMaxRect(void)
{
	m_rcMax.TopLeft()     = m_pt[0];
	m_rcMax.BottomRight() = m_pt[0];
	m_rcMax.NormalizeRect();
#ifdef _DEBUG
	CMagaDbg	dbg("CDXFpoint::SetMaxRect()", DBG_RED);
	dbg.printf("m_rcMax(left, top   )=(%f, %f)", m_rcMax.left, m_rcMax.top);
#endif
}

inline const CPoint CDXFpoint::GetDrawPoint(void) const
{
	return m_ptDraw;
}

inline BOOL CDXFpoint::IsMatchPoint(const CPointD& pt)
{
	return m_ptMake[0] == pt ? TRUE : FALSE;
}

inline BOOL CDXFpoint::IsStartEqEnd(void) const
{
	return TRUE;	// たぶん呼ばれることはない
}

inline double CDXFpoint::GetEdgeGap(const CPointD& pt, BOOL bSwap/*=TRUE*/)
{
	return GAPCALC(m_ptTun[0] - pt);
}

inline const CPointD CDXFpoint::GetStartCutterPoint(void) const
{
	return GetTunPoint(0);
}

inline const CPointD CDXFpoint::GetStartMakePoint(void) const
{
	return GetMakePoint(0);
}

inline const CPointD CDXFpoint::GetEndCutterPoint(void) const
{
	return GetTunPoint(0);
}

inline const CPointD CDXFpoint::GetEndMakePoint(void) const
{
	return GetMakePoint(0);
}

//////////////////////////////////////////////////////////////////////
// ＤＸＦデータのLineクラス
//////////////////////////////////////////////////////////////////////
inline void CDXFline::SetMaxRect(void)
{
	m_rcMax.TopLeft()     = m_pt[0];
	m_rcMax.BottomRight() = m_pt[1];
	m_rcMax.NormalizeRect();
#ifdef _DEBUG
	CMagaDbg	dbg("CDXFline::SetMaxRect()", DBG_RED);
	dbg.printf("m_rcMax(left, top   )=(%f, %f)", m_rcMax.left, m_rcMax.top);
	dbg.printf("m_rcMax(right,bottom)=(%f, %f)", m_rcMax.right, m_rcMax.bottom);
#endif
}

inline BOOL CDXFline::IsMatchPoint(const CPointD& pt)
{
	// 指定位置とこのｵﾌﾞｼﾞｪｸﾄの
	if ( m_ptMake[0] == pt )		// 始点と等しい
		return TRUE;
	if ( m_ptMake[1] == pt ) {		// 終点と等しい
		SwapPt(0);
		return TRUE;
	}
	return FALSE;
}

inline BOOL CDXFline::IsStartEqEnd(void) const
{
	return FALSE;
}

inline double CDXFline::GetEdgeGap(const CPointD& pt, BOOL bSwap/*=TRUE*/)
{
	double	dGap1 = GAPCALC(m_ptTun[0] - pt);
	double	dGap2 = GAPCALC(m_ptTun[1] - pt);
	if ( dGap1 > dGap2 ) {
		if ( bSwap )
			SwapPt(0);
		dGap1 = dGap2;
	}
	return dGap1;
}

inline const CPointD CDXFline::GetStartCutterPoint(void) const
{
	return GetTunPoint(0);
}

inline const CPointD CDXFline::GetStartMakePoint(void) const
{
	return GetMakePoint(0);
}

inline const CPointD CDXFline::GetEndCutterPoint(void) const
{
	return GetTunPoint(1);
}

inline const CPointD CDXFline::GetEndMakePoint(void) const
{
	return GetMakePoint(1);
}

//////////////////////////////////////////////////////////////////////
// ＤＸＦデータのCircleクラス
//////////////////////////////////////////////////////////////////////
inline void CDXFcircle::SetCirclePoint(void)
{
	m_pt[0].x = m_ct.x + m_r;	// 0°
	m_pt[0].y = m_ct.y;
	m_pt[1].x = m_ct.x;			// 90°
	m_pt[1].y = m_ct.y + m_r;
	m_pt[2].x = m_ct.x - m_r;	// 180°
	m_pt[2].y = m_ct.y;
	m_pt[3].x = m_ct.x;			// 270°
	m_pt[3].y = m_ct.y - m_r;
}

inline void CDXFcircle::GetQuarterPoint(const CPointD& ptClick, CPointD pt[]) const
{
	// ｸﾘｯｸﾎﾟｲﾝﾄの角度を取得
	double	lq = atan2(ptClick.y - m_ct.y, ptClick.x - m_ct.x);
	if ( lq < 0 )
		lq += 360.0*RAD;
	// 角度(位置)から1/4の始点終点座標を取得(反時計回りで考える)
	if ( lq>=0 && lq<90.0*RAD ) {
		pt[0] = m_pt[0];
		pt[1] = m_pt[1];
	}
	else if ( lq>=90.0*RAD && lq<180.0*RAD ) {
		pt[0] = m_pt[1];
		pt[1] = m_pt[2];
	}
	else if ( lq>=180.0*RAD && lq<270.0*RAD ) {
		pt[0] = m_pt[2];
		pt[1] = m_pt[3];
	}
	else {
		pt[0] = m_pt[3];
		pt[1] = m_pt[0];
	}
}

inline void CDXFcircle::SetEllipseArgv_Circle
	(const LPDXFBLOCK lpBlock, LPDXFEARGV lpArgv, double sq, double eq, BOOL bRound)
{
	lpArgv->pLayer = NULL;
	// 中心座標
	lpArgv->c.x = m_ct.x * lpBlock->dMagni[NCA_X];
	lpArgv->c.y = m_ct.y * lpBlock->dMagni[NCA_Y];
	// 長径座標と短径倍率(３時方向を基準に)
	double	rx = m_r * lpBlock->dMagni[NCA_X];
	double	ry = m_r * lpBlock->dMagni[NCA_Y];
	lpArgv->l.x = rx;
	lpArgv->l.y = 0;
	lpArgv->s   = ry / rx;
	// 角度
	lpArgv->sq = sq;
	lpArgv->eq = eq;
	// ﾌﾞﾛｯｸ回転
	if ( lpBlock->dwBlockFlg & DXFBLFLG_R ) {
		double	dRound = lpBlock->dRound * RAD;
		lpArgv->c.RoundPoint(dRound);
		lpArgv->l.RoundPoint(dRound);
	}
	// ﾌﾞﾛｯｸｵﾌｾｯﾄ
	lpArgv->c += lpBlock->ptOrg;
	// 方向
	lpArgv->bRound = bRound;
}

inline double CDXFcircle::GetSelectPointGap_Circle
	(const CPointD& pt, double sq, double eq) const
{
	CPointD	pt1(pt - m_ct);
	double	q1, q2;
	if ( (q1=atan2(pt1.y, pt1.x)) < 0.0 )
		q1 += 360.0*RAD;
	q2 = q1 + 360.0*RAD;
	return (sq <= q1 && q1 <= eq) || (sq <= q2 && q2 <= eq) ?
		fabs(m_r - pt1.hypot()) : HUGE_VAL;
}

inline void CDXFcircle::SetMaxRect(void)
{
	m_rcMax.TopLeft()     = m_ct - m_r;
	m_rcMax.BottomRight() = m_ct + m_r;
	m_rcMax.NormalizeRect();
#ifdef _DEBUG
	CMagaDbg	dbg("CDXFcircle::SetMaxRect()", DBG_RED);
	dbg.printf("m_rcMax(left, top   )=(%f, %f)", m_rcMax.left, m_rcMax.top);
	dbg.printf("m_rcMax(right,bottom)=(%f, %f)", m_rcMax.right, m_rcMax.bottom);
#endif
}

inline BOOL CDXFcircle::IsRoundFixed(void) const
{
	return m_bRoundFixed;
}

inline double CDXFcircle::GetR(void) const
{
	return m_r;
}

inline double CDXFcircle::GetMakeR(void) const
{
	return m_rMake;
}

inline BOOL CDXFcircle::GetRound(void) const
{
	return m_bRound;
}

inline int CDXFcircle::GetG(void) const
{
	return	(m_bRound ? 3 : 2);
}

inline int CDXFcircle::GetBaseAxis(void) const
{
	return m_nArrayExt;
}

inline double CDXFcircle::GetIJK(int nType) const
{
	if ( nType == NCA_I )
		return ::RoundUp( m_ctTun.x - m_ptTun[m_nArrayExt].x );
	else if ( nType == NCA_J )
		return ::RoundUp( m_ctTun.y - m_ptTun[m_nArrayExt].y );
	// NCA_K 無視
	return 0.0;
}

inline const CPointD CDXFcircle::GetCenter(void) const
{
	return m_ct;
}

inline const CPointD CDXFcircle::GetMakeCenter(void) const
{
	return m_ctTun;
}

inline void CDXFcircle::SetEllipseArgv(const LPDXFBLOCK lpBlock, LPDXFEARGV lpArgv)
{
	SetEllipseArgv_Circle(lpBlock, lpArgv, 0.0, 360.0*RAD, TRUE);
}

inline BOOL CDXFcircle::IsMatchPoint(const CPointD& pt)
{
	if ( GetMakeType() == DXFPOINTDATA )
		return CDXFpoint::IsMatchPoint(pt);

	for ( int i=0; i<GetPointNumber(); i++ ) {
		if ( m_ptMake[i] == pt ) {
			m_nArrayExt = i & 0xfe;		// 0 or 2 (下位1ﾋﾞｯﾄﾏｽｸ)
			// 奇数(180, 270)の時だけ入れ替え
			if ( i & 0x01 )
				SwapPt( m_nArrayExt );
			return TRUE;
		}
	}
	return FALSE;
}

inline BOOL CDXFcircle::IsStartEqEnd(void) const
{
	return TRUE;
}

inline double CDXFcircle::GetEdgeGap(const CPointD& pt, BOOL bSwap/*=TRUE*/)
{
	if ( GetMakeType() == DXFPOINTDATA )
		return CDXFpoint::GetEdgeGap(pt, bSwap);

	int		i, a;
	double	dGap, dGapMin = HUGE_VAL;
	for ( i=0; i<GetPointNumber(); i++ ) {
		dGap = GAPCALC(m_ptTun[i] - pt);
		if ( dGap < dGapMin ) {
			dGapMin = dGap;
			a = i;
		}
	}
	if ( bSwap ) {
		m_nArrayExt = a & 0xfe;
		if ( a & 0x01 )
			SwapPt( m_nArrayExt );
	}
	return dGapMin;
}

inline const CPointD CDXFcircle::GetStartCutterPoint(void) const
{
	return GetTunPoint( GetMakeType()==DXFPOINTDATA ? 0 : m_nArrayExt );
}

inline const CPointD CDXFcircle::GetStartMakePoint(void) const
{
	return GetMakePoint( GetMakeType()==DXFPOINTDATA ? 0 : m_nArrayExt );
}

inline const CPointD CDXFcircle::GetEndCutterPoint(void) const
{
	// 派生ｸﾗｽを考慮し "CDXFcircle::" を付与
	return CDXFcircle::GetStartCutterPoint();	// 円ﾃﾞｰﾀは加工終点も始点
}

inline const CPointD CDXFcircle::GetEndMakePoint(void) const
{
	return CDXFcircle::GetStartMakePoint();
}

inline void CDXFcircle::ReversePt(void)
{
	// 何もしない
}

//////////////////////////////////////////////////////////////////////
// ＤＸＦデータのCircleExクラス
//////////////////////////////////////////////////////////////////////
inline BOOL CDXFcircleEx::IsMatchPoint(const CPointD& pt)
{
	return GetEndMakePoint() == pt;
}

inline double CDXFcircleEx::GetEdgeGap(const CPointD& pt, BOOL bSwap/*=TRUE*/)
{
	return GAPCALC(m_ctTun - pt);
}

inline const CPointD CDXFcircleEx::GetStartCutterPoint(void) const
{
	return m_ctTun;	// Exの加工終点は中心
}

inline const CPointD CDXFcircleEx::GetStartMakePoint(void) const
{
	return m_ctMake;
}

inline const CPointD CDXFcircleEx::GetEndCutterPoint(void) const
{
	return GetStartCutterPoint();
}

inline const CPointD CDXFcircleEx::GetEndMakePoint(void) const
{
	return GetStartMakePoint();
}

//////////////////////////////////////////////////////////////////////
// ＤＸＦデータのArcクラス
//////////////////////////////////////////////////////////////////////
inline void CDXFarc::SetRsign(void)
{
	// 回転角度が 180°を越えるﾃﾞｰﾀは生成ﾃﾞｰﾀ用の半径をﾏｲﾅｽ
	if ( fabs(m_eqDraw-m_sqDraw)*DEG - 180.0 > EPS )
		m_rMake = -m_rMake;
}

inline void CDXFarc::AngleTuning(void)
{
	if ( m_sq < 0.0 )
		m_sq += 360.0*RAD;
	if ( m_eq < 0.0 )
		m_eq += 360.0*RAD;
	if ( m_bRound ) {
		while ( m_sq - m_eq >= 0 )
			m_eq += 360.0*RAD;
	}
	else {
		while ( m_eq - m_sq >= 0 )
			m_sq += 360.0*RAD;
	}
}

inline BOOL CDXFarc::GetRoundOrig(void) const
{
	return m_bRoundOrig;
}

inline void CDXFarc::SwapRound(void)
{
	m_bRound = !m_bRound;	// 回転方向の反転(円弧専用)
}

inline void CDXFarc::SwapAngle(void)
{
	// 角度の入れ替えと調整
	swap(m_sq, m_eq);
	AngleTuning();
}

inline double CDXFarc::GetStartAngle(void) const
{
	return m_sq;
}

inline double CDXFarc::GetEndAngle(void) const
{
	return m_eq;
}

inline void CDXFarc::SetEllipseArgv(const LPDXFBLOCK lpBlock, LPDXFEARGV lpArgv)
{
	SetEllipseArgv_Circle(lpBlock, lpArgv, m_sqDraw, m_eqDraw, m_bRoundOrig);
}

inline void CDXFarc::SwapPt(int n)	// ﾎﾟｲﾝﾄの入れ替え(+回転方向の反転)
{
	// 計算値の入れ替え
	CDXFdata::SwapPt(n);
	// 回転方向の反転
	SwapRound();
	// 角度の入れ替え
	SwapAngle();
}

inline BOOL CDXFarc::IsMatchPoint(const CPointD& pt)
{
	return CDXFline::IsMatchPoint(pt);
}

inline BOOL CDXFarc::IsStartEqEnd(void) const
{
	return FALSE;	// CDXFline::IsStartEqEnd()
}

inline double CDXFarc::GetEdgeGap(const CPointD& pt, BOOL bSwap/*=TRUE*/)
{
	return CDXFline::GetEdgeGap(pt, bSwap);
}

inline const CPointD CDXFarc::GetStartCutterPoint(void) const
{
	return CDXFline::GetStartCutterPoint();
}

inline const CPointD CDXFarc::GetStartMakePoint(void) const
{
	return CDXFline::GetStartMakePoint();
}

inline const CPointD CDXFarc::GetEndCutterPoint(void) const
{
	return CDXFline::GetEndCutterPoint();
}

inline const CPointD CDXFarc::GetEndMakePoint(void) const
{
	return CDXFline::GetEndMakePoint();
}

inline void CDXFarc::ReversePt(void)
{
	SwapPt(0);
}

//////////////////////////////////////////////////////////////////////
// ＤＸＦデータのEllipseクラス
//////////////////////////////////////////////////////////////////////
inline const CPointD CDXFellipse::GetLongPoint(void) const
{
	return m_ptLong;
}

inline double CDXFellipse::GetShortMagni(void) const
{
	return m_dShort;
}

inline double CDXFellipse::GetLongLength(void) const
{
	return m_dLongLength;
}

inline double CDXFellipse::GetShortLength(void) const
{
	return m_dLongLength * m_dShort;
}

inline double CDXFellipse::GetLean(void) const
{
	return m_lq;
}

inline double CDXFellipse::GetLeanCos(void) const
{
	return m_lqMakeCos;
}

inline double CDXFellipse::GetLeanSin(void) const
{
	return m_lqMakeSin;
}

inline BOOL CDXFellipse::GetArc(void) const
{
	return m_bArc;
}

inline void CDXFellipse::SwapPt(int n)
{
	if ( !m_bArc || GetMakeType()==DXFPOINTDATA || GetMakeType()==DXFCIRCLEDATA ) {
		CDXFdata::SwapPt(n);	// CDXFcircle::SwapPt()
		if ( !m_bArc ) {
			m_sq += 180.0*RAD;	// 軸の向きを180°回転
			m_eq += 180.0*RAD;
		}
	}
	else
		CDXFarc::SwapPt(n);
}

inline void CDXFellipse::SetEllipseTunPoint(void)
{
	double	dShort = m_dLongLength * m_dShort;
	// 0〜1 -> X軸
	m_ptTun[0].x =  m_dLongLength;
	m_ptTun[1].x = -m_dLongLength;
	m_ptTun[0].y = m_ptTun[1].y = 0;
	// 2〜3 -> Y軸
	m_ptTun[2].y =  dShort;
	m_ptTun[3].y = -dShort;
	m_ptTun[2].x = m_ptTun[3].x = 0;
	// 回転
	CPointD	pt;
	for ( int i=0; i<GetPointNumber(); i++ ) {
		pt = m_ptTun[i];
		m_ptTun[i].x = pt.x * m_lqMakeCos - pt.y * m_lqMakeSin + m_ctTun.x;
		m_ptTun[i].y = pt.x * m_lqMakeSin + pt.y * m_lqMakeCos + m_ctTun.y;
	}
}

inline BOOL CDXFellipse::IsMatchPoint(const CPointD& pt)
{
	return !m_bArc || GetMakeType()==DXFPOINTDATA || GetMakeType()==DXFCIRCLEDATA ?
		CDXFcircle::IsMatchPoint(pt) : CDXFarc::IsMatchPoint(pt);
}

inline BOOL CDXFellipse::IsStartEqEnd(void) const
{
	return !m_bArc;		// 楕円か楕円弧か
}

inline double CDXFellipse::GetEdgeGap(const CPointD& pt, BOOL bSwap/*=TRUE*/)
{
	return !m_bArc || GetMakeType()==DXFPOINTDATA || GetMakeType()==DXFCIRCLEDATA ?
		CDXFcircle::GetEdgeGap(pt, bSwap) : CDXFarc::GetEdgeGap(pt, bSwap);
}

inline const CPointD CDXFellipse::GetStartCutterPoint(void) const
{
	return !m_bArc || GetMakeType()==DXFPOINTDATA || GetMakeType()==DXFCIRCLEDATA ?
		CDXFcircle::GetStartCutterPoint() : CDXFarc::GetStartCutterPoint();
}

inline const CPointD CDXFellipse::GetStartMakePoint(void) const
{
	return !m_bArc || GetMakeType()==DXFPOINTDATA || GetMakeType()==DXFCIRCLEDATA ?
		CDXFcircle::GetStartMakePoint() : CDXFarc::GetStartMakePoint();
}

inline const CPointD CDXFellipse::GetEndCutterPoint(void) const
{
	return !m_bArc || GetMakeType()==DXFPOINTDATA || GetMakeType()==DXFCIRCLEDATA ?
		CDXFcircle::GetEndCutterPoint() : CDXFarc::GetEndCutterPoint();
}

inline const CPointD CDXFellipse::GetEndMakePoint(void) const
{
	return !m_bArc || GetMakeType()==DXFPOINTDATA || GetMakeType()==DXFCIRCLEDATA ?
		CDXFcircle::GetEndMakePoint() : CDXFarc::GetEndMakePoint();
}

inline void CDXFellipse::ReversePt(void)
{
	if ( !m_bArc || GetMakeType()==DXFPOINTDATA || GetMakeType()==DXFCIRCLEDATA )
		CDXFcircle::ReversePt();	// 何もしない
	else
		CDXFarc::ReversePt();
}

//////////////////////////////////////////////////////////////////////
// ＤＸＦデータのPolylineクラス
//////////////////////////////////////////////////////////////////////
inline void CDXFpolyline::SetPolyFlag(int nFlag)
{
	m_nPolyFlag = nFlag;
}

inline int CDXFpolyline::GetPolyFlag(void) const
{
	return m_nPolyFlag;
}

inline BOOL CDXFpolyline::GetSequence(void) const
{
	return m_bSeq;
}

inline INT_PTR CDXFpolyline::GetVertexCount(void) const
{
	return m_ltVertex.GetCount();
}

inline int CDXFpolyline::GetObjectCount(int n) const
{
	ASSERT( n>=0 && n<SIZEOF(m_nObjCnt) );
	return m_nObjCnt[n];
}

inline POSITION CDXFpolyline::GetFirstVertex(void) const
{
	return m_bSeq ? m_ltVertex.GetHeadPosition() : m_ltVertex.GetTailPosition();
}

inline CDXFdata* CDXFpolyline::GetNextVertex(POSITION& pos) const
{
	return m_bSeq ? m_ltVertex.GetNext(pos) : m_ltVertex.GetPrev(pos);
}

inline const CPointD CDXFpolyline::GetFirstPoint(void) const
{
	return m_ltVertex.GetHead()->GetNativePoint(0);
}

inline void CDXFpolyline::SetParentLayer(CLayerData* pLayer)
{
	ASSERT( pLayer );
	m_pParentLayer = pLayer;
}

inline BOOL CDXFpolyline::IsStartEqEnd(void) const
{
	return ( m_nPolyFlag & 1 );		// 閉ﾙｰﾌﾟ
}

//////////////////////////////////////////////////////////////////////
// ＤＸＦデータのTextクラス
//////////////////////////////////////////////////////////////////////
inline CString CDXFtext::GetStrValue(void) const
{
	return m_strValue;
}

inline double CDXFtext::GetEdgeGap(const CPointD&, BOOL/*=TRUE*/)
{
	return HUGE_VAL;	// IsMatchPoint() 以外は近接判断の必要なし
}
