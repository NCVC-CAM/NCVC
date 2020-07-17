// DXFdata.inl: CDXFdata ÉNÉâÉXÇÃÉCÉìÉâÉCÉìä÷êî
//
//////////////////////////////////////////////////////////////////////

#pragma once

//#define	_DEBUGDRAW_DXF		// ï`âÊèàóùÇ‡€∏ﬁ
#include "MagaDbgMac.h"

/////////////////////////////////////////////////////////////////////////////
// ÇcÇwÇeÉfÅ[É^ÇÃÉxÅ[ÉXÉNÉâÉX
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
	return m_pParentLayer;
}

inline CDXFshape* CDXFdata::GetParentMap(void) const
{
	return m_pParentMap;
}

inline void CDXFdata::SetParentMap(CDXFshape* pShape)
{
	m_pParentMap = pShape;
}

inline DWORD CDXFdata::GetDxfFlg(void) const
{
	return m_dwFlags;
}

inline void CDXFdata::SetDxfFlg(DWORD dwFlags, BOOL bSet/*=TRUE*/)
{
	if ( bSet )
		m_dwFlags |=  dwFlags;
	else
		m_dwFlags &= ~dwFlags;
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
	m_dwFlags |= DXFFLG_MAKE;		// SetDxfFlg(DXFFLG_MAKE);
}

inline void CDXFdata::SetSearchFlg(void)
{
	m_dwFlags |= DXFFLG_SEARCH;		// SetDxfFlg(DXFFLG_SEARCH);
}

inline void CDXFdata::ClearMakeFlg(void)
{
	m_dwFlags &= ~DXFFLG_MAKE;		// SetDxfFlg(DXFFLG_MAKE, FALSE);
}

inline void CDXFdata::ClearSearchFlg(void)
{
	m_dwFlags &= ~DXFFLG_SEARCH;	// SetDxfFlg(DXFFLG_SEARCH, FALSE);
}

inline const CRect3D CDXFdata::GetMaxRect(void) const
{
	return m_rcMax;
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

inline void CDXFdata::SetNativePoint(size_t a, const CPointD& pt)
{
	ASSERT( a>=0 && a<(size_t)m_nPoint );
	m_pt[a] = pt;
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

inline BOOL CDXFdata::IsMatchPoint(const CPointD& pt) const
{
	for ( int i=0; i<m_nPoint; i++ ) {
		if ( m_pt[i].IsMatchPoint(&pt) )
			return TRUE;
	}
	return FALSE;
}

inline BOOL CDXFdata::IsMakeMatchObject(const CDXFdata* pData)
{
	return IsMakeMatchPoint( pData->GetEndMakePoint() );	// virtualéwíË
}

inline double CDXFdata::GetEdgeGap(const CDXFdata* pData, BOOL bSwap/*=TRUE*/)
{
	return GetEdgeGap(pData->GetEndCutterPoint(), bSwap);	// virtualéwíË
}

/////////////////////////////////////////////////////////////////////////////
// ÇcÇwÇeÉfÅ[É^ÇÃPointÉNÉâÉX
/////////////////////////////////////////////////////////////////////////////
inline void CDXFpoint::SetMaxRect(void)
{
	m_rcMax.TopLeft()     = m_pt[0];
	m_rcMax.BottomRight() = m_pt[0];
	m_rcMax.NormalizeRect();
#ifdef _DEBUG
	CMagaDbg	dbg("CDXFpoint::SetMaxRect()", DBG_RED);
	dbg.printf("l=%.3f t=%.3f", m_rcMax.left, m_rcMax.top);
#endif
}

inline const CPoint CDXFpoint::GetDrawPoint(void) const
{
	return m_ptDraw;
}

inline BOOL CDXFpoint::IsMakeTarget(void) const
{
	return TRUE;
}

inline BOOL CDXFpoint::IsMakeMatchPoint(const CPointD& pt)
{
	return m_ptMake[0] == pt ? TRUE : FALSE;
}

inline BOOL CDXFpoint::IsStartEqEnd(void) const
{
	return TRUE;	// ÇΩÇ‘ÇÒåƒÇŒÇÍÇÈÇ±Ç∆ÇÕÇ»Ç¢
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

inline double CDXFpoint::GetLength(void) const
{
	return 0;
}

//////////////////////////////////////////////////////////////////////
// ÇcÇwÇeÉfÅ[É^ÇÃLineÉNÉâÉX
//////////////////////////////////////////////////////////////////////
inline void CDXFline::SetMaxRect(void)
{
	m_rcMax.TopLeft()     = m_pt[0];
	m_rcMax.BottomRight() = m_pt[1];
	m_rcMax.NormalizeRect();
#ifdef _DEBUG
	CMagaDbg	dbg("CDXFline::SetMaxRect()", DBG_RED);
	dbg.printf("l=%.3f t=%.3f r=%.3f b=%.3f",
		m_rcMax.left, m_rcMax.top, m_rcMax.right, m_rcMax.bottom);
#endif
}

inline BOOL CDXFline::IsMakeTarget(void) const
{
	CPointD	pt(m_pt[1]-m_pt[0]);
	return pt.hypot() >= NCMIN;
}

inline BOOL CDXFline::IsMakeMatchPoint(const CPointD& pt)
{
	// éwíËà íuÇ∆Ç±ÇÃµÃﬁºﬁ™∏ƒÇÃ
	if ( m_ptMake[0] == pt )		// énì_Ç∆ìôÇµÇ¢
		return TRUE;
	if ( m_ptMake[1] == pt ) {		// èIì_Ç∆ìôÇµÇ¢
		SwapMakePt(0);
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
			SwapMakePt(0);
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

inline double CDXFline::GetLength(void) const
{
	CPointD	pt(m_pt[1] - m_pt[0]);
	return pt.hypot();
}

//////////////////////////////////////////////////////////////////////
// ÇcÇwÇeÉfÅ[É^ÇÃCircleÉNÉâÉX
//////////////////////////////////////////////////////////////////////
inline void CDXFcircle::SetCirclePoint(void)
{
	m_pt[0].x = m_ct.x + m_r;	// 0Åã
	m_pt[0].y = m_ct.y;
	m_pt[1].x = m_ct.x;			// 90Åã
	m_pt[1].y = m_ct.y + m_r;
	m_pt[2].x = m_ct.x - m_r;	// 180Åã
	m_pt[2].y = m_ct.y;
	m_pt[3].x = m_ct.x;			// 270Åã
	m_pt[3].y = m_ct.y - m_r;
}

inline void CDXFcircle::GetQuarterPoint(const CPointD& ptClick, CPointD pt[]) const
{
	// ∏ÿØ∏Œﬂ≤›ƒÇÃäpìxÇéÊìæ
	double	lq = atan2(ptClick.y - m_ct.y, ptClick.x - m_ct.x);
	if ( lq < 0 )
		lq += RAD(360.0);
	// äpìx(à íu)Ç©ÇÁ1/4ÇÃénì_èIì_ç¿ïWÇéÊìæ(îΩéûåvâÒÇËÇ≈çlÇ¶ÇÈ)
	if ( lq>=0 && lq<RAD(90.0) ) {
		pt[0] = m_pt[0];
		pt[1] = m_pt[1];
	}
	else if ( lq>=RAD(90.0) && lq<RAD(180.0) ) {
		pt[0] = m_pt[1];
		pt[1] = m_pt[2];
	}
	else if ( lq>=RAD(180.0) && lq<RAD(270.0) ) {
		pt[0] = m_pt[2];
		pt[1] = m_pt[3];
	}
	else {
		pt[0] = m_pt[3];
		pt[1] = m_pt[0];
	}
}

inline void CDXFcircle::SetEllipseArgv_Circle
	(LPCDXFBLOCK lpBlock, LPCDXFEARGV lpArgv, double sq, double eq, BOOL bRound)
{
	lpArgv->pLayer = GetParentLayer();
	// íÜêSç¿ïW
	lpArgv->c.x = m_ct.x * lpBlock->dMagni[NCA_X];
	lpArgv->c.y = m_ct.y * lpBlock->dMagni[NCA_Y];
	// í∑åaç¿ïWÇ∆íZåaî{ó¶(ÇRéûï˚å¸ÇäÓèÄÇ…)
	double	rx = m_r * lpBlock->dMagni[NCA_X];
	double	ry = m_r * lpBlock->dMagni[NCA_Y];
	lpArgv->l.x = rx;
	lpArgv->l.y = 0;
	lpArgv->s   = ry / rx;
	// äpìx
	lpArgv->sq = sq;
	lpArgv->eq = eq;
	// Ãﬁ€Ø∏âÒì]
	if ( lpBlock->dwBlockFlg & DXFBLFLG_R ) {
		double	dRound = RAD(lpBlock->dRound);
		lpArgv->c.RoundPoint(dRound);
		lpArgv->l.RoundPoint(dRound);
	}
	// Ãﬁ€Ø∏µÃæØƒ
	lpArgv->c += lpBlock->ptOrg;
	// ï˚å¸
	lpArgv->bRound = bRound;
}

inline double CDXFcircle::GetSelectPointGap_Circle
	(const CPointD& pt, double sq, double eq) const
{
	CPointD	pt1(pt - m_ct);
	double	q1, q2;
	if ( (q1=atan2(pt1.y, pt1.x)) < 0.0 )
		q1 += RAD(360.0);
	q2 = q1 + RAD(360.0);
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
	dbg.printf("l=%.3f t=%.3f r=%.3f b=%.3f",
		m_rcMax.left, m_rcMax.top, m_rcMax.right, m_rcMax.bottom);
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
	// NCA_K ñ≥éã
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

inline void CDXFcircle::SetEllipseArgv(LPCDXFBLOCK lpBlock, LPCDXFEARGV lpArgv)
{
	SetEllipseArgv_Circle(lpBlock, lpArgv, 0.0, RAD(360.0), TRUE);
}

inline BOOL CDXFcircle::IsMakeTarget(void) const
{
	return fabs(m_r) >= NCMIN;
}

inline BOOL CDXFcircle::IsMakeMatchPoint(const CPointD& pt)
{
	if ( GetMakeType() == DXFPOINTDATA )
		return CDXFpoint::IsMakeMatchPoint(pt);

	for ( int i=0; i<m_nPoint; i++ ) {
//		if ( m_ptMake[i]==pt && m_nArrayExt!=i ) {
		if ( m_ptMake[i]==pt ) {
			SwapMakePt(i);
			return TRUE;
		}
	}
	return FALSE;
}

inline BOOL CDXFcircle::IsStartEqEnd(void) const
{
	return TRUE;
}

inline BOOL CDXFcircle::IsUnderRadius(double d) const
{
	return GetMakeR() <= d;
}

inline double CDXFcircle::GetEdgeGap(const CPointD& pt, BOOL bSwap/*=TRUE*/)
{
	if ( GetMakeType() == DXFPOINTDATA )
		return CDXFpoint::GetEdgeGap(pt, bSwap);

	int		i, a = 0;
	double	dGap, dGapMin = HUGE_VAL;
	for ( i=0; i<m_nPoint; i++ ) {
		dGap = GAPCALC(m_ptTun[i] - pt);
		if ( dGap < dGapMin ) {
			dGapMin = dGap;
			a = i;
		}
	}
//	if ( bSwap && m_nArrayExt!=a )
	if ( bSwap )
		SwapMakePt(a);

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
	// îhê∂∏◊ΩÇçló∂Çµ "CDXFcircle::" Çïtó^
	return CDXFcircle::GetStartCutterPoint();	// â~√ﬁ∞¿ÇÕâ¡çHèIì_Ç‡énì_
}

inline const CPointD CDXFcircle::GetEndMakePoint(void) const
{
	return CDXFcircle::GetStartMakePoint();
}

inline double CDXFcircle::GetLength(void) const
{
	return 2.0 * PI * m_r;
}

//////////////////////////////////////////////////////////////////////
// ÇcÇwÇeÉfÅ[É^ÇÃCircleExÉNÉâÉX
//////////////////////////////////////////////////////////////////////
inline BOOL CDXFcircleEx::IsMakeTarget(void) const
{
	return FALSE;
}

inline BOOL CDXFcircleEx::IsMakeMatchPoint(const CPointD& pt)
{
	return GetEndMakePoint() == pt;
}

inline double CDXFcircleEx::GetEdgeGap(const CPointD& pt, BOOL bSwap/*=TRUE*/)
{
	return GAPCALC(m_ctTun - pt);
}

inline const CPointD CDXFcircleEx::GetStartCutterPoint(void) const
{
	return m_ctTun;	// ExÇÃâ¡çHèIì_ÇÕíÜêS
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
// ÇcÇwÇeÉfÅ[É^ÇÃArcÉNÉâÉX
//////////////////////////////////////////////////////////////////////
inline void CDXFarc::SetRsign(void)
{
	// âÒì]äpìxÇ™ 180ÅãÇâzÇ¶ÇÈ√ﬁ∞¿ÇÕê∂ê¨√ﬁ∞¿ópÇÃîºåaÇœ≤≈Ω
	if ( DEG(fabs(m_eqDraw-m_sqDraw)) - 180.0 >= NCMIN )
		m_rMake = -m_rMake;
}

inline BOOL CDXFarc::GetRoundOrig(void) const
{
	return m_bRoundOrig;
}

inline void CDXFarc::SwapRound(void)
{
	m_bRound = !m_bRound;	// âÒì]ï˚å¸ÇÃîΩì](â~å êÍóp)
}

inline void CDXFarc::SwapAngle(void)
{
	// äpìxÇÃì¸ÇÍë÷Ç¶Ç∆í≤êÆ
	boost::swap(m_sq, m_eq);
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

inline void CDXFarc::SetEllipseArgv(LPCDXFBLOCK lpBlock, LPCDXFEARGV lpArgv)
{
	SetEllipseArgv_Circle(lpBlock, lpArgv, m_sqDraw, m_eqDraw, m_bRoundOrig);
}

inline void CDXFarc::SetNativePoint(size_t a, const CPointD& pt)
{
	CDXFdata::SetNativePoint(a, pt);
	// äpìxÇÃçƒåvéZ
	double&	q = a==0 ? m_sq : m_eq;		// éQè∆å^
	q = atan2(m_pt[a].y-m_ct.y, m_pt[a].x-m_ct.x);
	// äpìxÇÃí≤êÆ
	AngleTuning();
	m_sqDraw = m_sq;
	m_eqDraw = m_eq;
}

inline BOOL CDXFarc::IsMakeTarget(void) const
{
	double	l = fabs(m_r * (m_eq - m_sq));
	return CDXFcircle::IsMakeTarget() && l >= NCMIN;
}

inline BOOL CDXFarc::IsMakeMatchPoint(const CPointD& pt)
{
	return CDXFline::IsMakeMatchPoint(pt);
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

inline double CDXFarc::GetLength(void) const
{
	double	s, e;
	if ( m_bRound ) {
		s = m_sq;	e = m_eq;
	}
	else {
		s = m_eq;	e = m_sq;
	}
	return m_r * ( e - s );
}

//////////////////////////////////////////////////////////////////////
// ÇcÇwÇeÉfÅ[É^ÇÃEllipseÉNÉâÉX
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

inline double CDXFellipse::GetMakeLean(void) const
{
	return m_lqMake;
}

inline double CDXFellipse::GetMakeLeanCos(void) const
{
	return m_lqMakeCos;
}

inline double CDXFellipse::GetMakeLeanSin(void) const
{
	return m_lqMakeSin;
}

inline BOOL CDXFellipse::IsArc(void) const
{
	return m_bArc;
}

inline BOOL CDXFellipse::IsMakeTarget(void) const
{
	return CDXFarc::IsMakeTarget();
}

inline BOOL CDXFellipse::IsMakeMatchPoint(const CPointD& pt)
{
	return !m_bArc || GetMakeType()==DXFPOINTDATA || GetMakeType()==DXFCIRCLEDATA ?
		CDXFcircle::IsMakeMatchPoint(pt) : CDXFarc::IsMakeMatchPoint(pt);
}

inline BOOL CDXFellipse::IsStartEqEnd(void) const
{
	return !m_bArc;		// ë»â~Ç©ë»â~å Ç©
}

inline BOOL	CDXFellipse::IsLongEqShort(void) const
{
	return fabs( ::RoundUp(GetLongLength()) - ::RoundUp(GetShortLength()) ) < NCMIN;
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

//////////////////////////////////////////////////////////////////////
// ÇcÇwÇeÉfÅ[É^ÇÃPolylineÉNÉâÉX
//////////////////////////////////////////////////////////////////////
inline void CDXFpolyline::SetPolyFlag(DWORD dwFlag)
{
	m_dwPolyFlags |= dwFlag;
}

inline DWORD CDXFpolyline::GetPolyFlag(void) const
{
	return m_dwPolyFlags;
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
	return m_dwPolyFlags & DXFPOLY_SEQ ?
		m_ltVertex.GetHeadPosition() : m_ltVertex.GetTailPosition();
}

inline CDXFdata* CDXFpolyline::GetNextVertex(POSITION& pos) const
{
	return m_dwPolyFlags & DXFPOLY_SEQ ?
		m_ltVertex.GetNext(pos) : m_ltVertex.GetPrev(pos);
}

inline const CPointD CDXFpolyline::GetFirstPoint(void) const
{
	CDXFdata*	pData = m_dwPolyFlags & DXFPOLY_SEQ ?
		m_ltVertex.GetHead() : m_ltVertex.GetTail();
	return pData->GetNativePoint(0);
}

inline void CDXFpolyline::SetParentLayer(CLayerData* pLayer)
{
	ASSERT( pLayer );
	m_pParentLayer = pLayer;
}

inline BOOL	CDXFpolyline::IsIntersection(void) const
{
	return ( m_dwPolyFlags & DXFPOLY_INTERSEC );	// é©ï™é©êgÇ…åì_Ç†ÇË
}

inline BOOL CDXFpolyline::IsMakeTarget(void) const
{
	return TRUE;	// â°íÖ
}

inline BOOL CDXFpolyline::IsStartEqEnd(void) const
{
	return ( m_dwPolyFlags & DXFPOLY_CLOSED );	// ï¬Ÿ∞Ãﬂ
}

//////////////////////////////////////////////////////////////////////
// ÇcÇwÇeÉfÅ[É^ÇÃTextÉNÉâÉX
//////////////////////////////////////////////////////////////////////
inline CString CDXFtext::GetStrValue(void) const
{
	return m_strValue;
}

inline double CDXFtext::GetEdgeGap(const CPointD&, BOOL/*=TRUE*/)
{
	return HUGE_VAL;	// IsMakeMatchPoint() à»äOÇÕãﬂê⁄îªífÇÃïKóvÇ»Çµ
}
