// DXFdata.inl: CDXFdata ƒNƒ‰ƒX‚ÌƒCƒ“ƒ‰ƒCƒ“ŠÖ”
//
//////////////////////////////////////////////////////////////////////

#pragma once

//#define	_DEBUGDRAW_DXF		// •`‰æˆ—‚àÛ¸Ş
#include "MagaDbgMac.h"

/////////////////////////////////////////////////////////////////////////////
// ‚c‚w‚eƒf[ƒ^‚Ìƒx[ƒXƒNƒ‰ƒX
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

inline BOOL CDXFdata::IsMatchObject(const CDXFdata* pData)
{
	return IsMatchPoint( pData->GetEndMakePoint() );	// virtualw’è
}

inline double CDXFdata::GetEdgeGap(const CDXFdata* pData, BOOL bSwap/*=TRUE*/)
{
	return GetEdgeGap(pData->GetEndCutterPoint(), bSwap);	// virtualw’è
}

inline void CDXFdata::SwapPt(int n)		// m_ptTun ‚Ì“ü‚ê‘Ö‚¦
{
#ifdef _DEBUG
	CMagaDbg	dbg("SwapPt()", DBG_CYAN);
#endif
	if ( m_nPoint > n+1 ) {	// ”O‚Ì‚½‚ßÁª¯¸
#ifdef _DEBUG
		dbg.printf("calling ok");
#endif
		std::swap(m_ptTun[n],  m_ptTun[n+1]);
		std::swap(m_ptMake[n], m_ptMake[n+1]);
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
// ‚c‚w‚eƒf[ƒ^‚ÌPointƒNƒ‰ƒX
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

inline BOOL CDXFpoint::IsMatchPoint(const CPointD& pt)
{
	return m_ptMake[0] == pt ? TRUE : FALSE;
}

inline BOOL CDXFpoint::IsStartEqEnd(void) const
{
	return TRUE;	// ‚½‚Ô‚ñŒÄ‚Î‚ê‚é‚±‚Æ‚Í‚È‚¢
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
// ‚c‚w‚eƒf[ƒ^‚ÌLineƒNƒ‰ƒX
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

inline BOOL CDXFline::IsMatchPoint(const CPointD& pt)
{
	// w’èˆÊ’u‚Æ‚±‚ÌµÌŞ¼Şª¸Ä‚Ì
	if ( m_ptMake[0] == pt )		// n“_‚Æ“™‚µ‚¢
		return TRUE;
	if ( m_ptMake[1] == pt ) {		// I“_‚Æ“™‚µ‚¢
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

inline double CDXFline::GetLength(void) const
{
	CPointD	pt(m_pt[1] - m_pt[0]);
	return pt.hypot();
}

//////////////////////////////////////////////////////////////////////
// ‚c‚w‚eƒf[ƒ^‚ÌCircleƒNƒ‰ƒX
//////////////////////////////////////////////////////////////////////
inline void CDXFcircle::SetCirclePoint(void)
{
	m_pt[0].x = m_ct.x + m_r;	// 0‹
	m_pt[0].y = m_ct.y;
	m_pt[1].x = m_ct.x;			// 90‹
	m_pt[1].y = m_ct.y + m_r;
	m_pt[2].x = m_ct.x - m_r;	// 180‹
	m_pt[2].y = m_ct.y;
	m_pt[3].x = m_ct.x;			// 270‹
	m_pt[3].y = m_ct.y - m_r;
}

inline void CDXFcircle::GetQuarterPoint(const CPointD& ptClick, CPointD pt[]) const
{
	// ¸Ø¯¸Îß²İÄ‚ÌŠp“x‚ğæ“¾
	double	lq = atan2(ptClick.y - m_ct.y, ptClick.x - m_ct.x);
	if ( lq < 0 )
		lq += 360.0*RAD;
	// Šp“x(ˆÊ’u)‚©‚ç1/4‚Ìn“_I“_À•W‚ğæ“¾(”½Œv‰ñ‚è‚Ål‚¦‚é)
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
	// ’†SÀ•W
	lpArgv->c.x = m_ct.x * lpBlock->dMagni[NCA_X];
	lpArgv->c.y = m_ct.y * lpBlock->dMagni[NCA_Y];
	// ’·ŒaÀ•W‚Æ’ZŒa”{—¦(‚R•ûŒü‚ğŠî€‚É)
	double	rx = m_r * lpBlock->dMagni[NCA_X];
	double	ry = m_r * lpBlock->dMagni[NCA_Y];
	lpArgv->l.x = rx;
	lpArgv->l.y = 0;
	lpArgv->s   = ry / rx;
	// Šp“x
	lpArgv->sq = sq;
	lpArgv->eq = eq;
	// ÌŞÛ¯¸‰ñ“]
	if ( lpBlock->dwBlockFlg & DXFBLFLG_R ) {
		double	dRound = lpBlock->dRound * RAD;
		lpArgv->c.RoundPoint(dRound);
		lpArgv->l.RoundPoint(dRound);
	}
	// ÌŞÛ¯¸µÌ¾¯Ä
	lpArgv->c += lpBlock->ptOrg;
	// •ûŒü
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
	// NCA_K –³‹
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
		if ( m_ptMake[i]==pt && m_nArrayExt!=i ) {
			m_nArrayExt = i;
			SwapPt( i & 0xfe );		// 0 or 2 (‰ºˆÊ1ËŞ¯ÄÏ½¸) => dummy
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

	int		i, a = 0;
	double	dGap, dGapMin = HUGE_VAL;
	for ( i=0; i<GetPointNumber(); i++ ) {
		dGap = GAPCALC(m_ptTun[i] - pt);
		if ( dGap < dGapMin ) {
			dGapMin = dGap;
			a = i;
		}
	}
	if ( bSwap && m_nArrayExt!=a ) {
		m_nArrayExt = a;
		SwapPt( a & 0xfe );			// 0 or 2 (‰ºˆÊ1ËŞ¯ÄÏ½¸) => dummy
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
	// ”h¶¸×½‚ğl—¶‚µ "CDXFcircle::" ‚ğ•t—^
	return CDXFcircle::GetStartCutterPoint();	// ‰~ÃŞ°À‚Í‰ÁHI“_‚àn“_
}

inline const CPointD CDXFcircle::GetEndMakePoint(void) const
{
	return CDXFcircle::GetStartMakePoint();
}

inline double CDXFcircle::GetLength(void) const
{
	return 2 * PI * m_r;
}

inline void CDXFcircle::SwapPt(int)
{
	// ‰½‚à‚µ‚È‚¢
}

inline void CDXFcircle::ReversePt(void)
{
	// ‰½‚à‚µ‚È‚¢
}

//////////////////////////////////////////////////////////////////////
// ‚c‚w‚eƒf[ƒ^‚ÌCircleExƒNƒ‰ƒX
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
	return m_ctTun;	// Ex‚Ì‰ÁHI“_‚Í’†S
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
// ‚c‚w‚eƒf[ƒ^‚ÌArcƒNƒ‰ƒX
//////////////////////////////////////////////////////////////////////
inline void CDXFarc::SetRsign(void)
{
	// ‰ñ“]Šp“x‚ª 180‹‚ğ‰z‚¦‚éÃŞ°À‚Í¶¬ÃŞ°À—p‚Ì”¼Œa‚ğÏ²Å½
	if ( fabs(m_eqDraw-m_sqDraw)*DEG - 180.0 > EPS )
		m_rMake = -m_rMake;
}

inline void CDXFarc::AngleTuning(void)
{
	if ( m_sq < 0.0 )
		m_sq += 360.0*RAD;
	if ( m_eq < 0.0 )
		m_eq += 360.0*RAD;
	double	d;
	if ( m_bRound ) {
		// ”÷–­‚ÈŒë·‚Ì‹zû(=>”÷×‰~ŒÊ‚ª‘å‚«‚È‰~‚É•Ï‚í‚Á‚Ä‚µ‚Ü‚¤)
		d = ::RoundUp(m_sq*DEG);
		while ( d >= ::RoundUp(m_eq*DEG) )
			m_eq += 360.0*RAD;
	}
	else {
		d = ::RoundUp(m_eq*DEG);
		while ( d >= ::RoundUp(m_sq*DEG) )
			m_sq += 360.0*RAD;
	}
}

inline BOOL CDXFarc::GetRoundOrig(void) const
{
	return m_bRoundOrig;
}

inline void CDXFarc::SwapRound(void)
{
	m_bRound = !m_bRound;	// ‰ñ“]•ûŒü‚Ì”½“](‰~ŒÊê—p)
}

inline void CDXFarc::SwapAngle(void)
{
	// Šp“x‚Ì“ü‚ê‘Ö‚¦‚Æ’²®
	std::swap(m_sq, m_eq);
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

inline void CDXFarc::SwapPt(int n)	// Îß²İÄ‚Ì“ü‚ê‘Ö‚¦(+‰ñ“]•ûŒü‚Ì”½“])
{
	// ŒvZ’l‚Ì“ü‚ê‘Ö‚¦
	CDXFdata::SwapPt(n);
	// ‰ñ“]•ûŒü‚Ì”½“]
	SwapRound();
	// Šp“x‚Ì“ü‚ê‘Ö‚¦
	SwapAngle();
}

inline void CDXFarc::SetNativePoint(size_t a, const CPointD& pt)
{
	CDXFdata::SetNativePoint(a, pt);
	// Šp“x‚Ì’²®
	if ( a == 0 )
		m_sq = atan2(m_pt[0].y-m_ct.y, m_pt[0].x-m_ct.x);
	else
		m_eq = atan2(m_pt[1].y-m_ct.y, m_pt[1].x-m_ct.x);
	AngleTuning();
	m_sqDraw = m_sq;
	m_eqDraw = m_eq;
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

inline void CDXFarc::ReversePt(void)
{
	SwapPt(0);
}

//////////////////////////////////////////////////////////////////////
// ‚c‚w‚eƒf[ƒ^‚ÌEllipseƒNƒ‰ƒX
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

inline BOOL CDXFellipse::IsArc(void) const
{
	return m_bArc;
}

inline void CDXFellipse::SetRoundFixed(BOOL bRound)
{
	m_bRound = bRound;
	SwapAngle();
}

inline BOOL CDXFellipse::IsMatchPoint(const CPointD& pt)
{
	return !m_bArc || GetMakeType()==DXFPOINTDATA || GetMakeType()==DXFCIRCLEDATA ?
		CDXFcircle::IsMatchPoint(pt) : CDXFarc::IsMatchPoint(pt);
}

inline BOOL CDXFellipse::IsStartEqEnd(void) const
{
	return !m_bArc;		// ‘È‰~‚©‘È‰~ŒÊ‚©
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
	if ( !m_bArc || GetMakeType()==DXFPOINTDATA || GetMakeType()==DXFCIRCLEDATA ) {
		// ‰ñ“]•ûŒü‚Ì”½“]
		SwapRound();
		// Šp“x‚Ì“ü‚ê‘Ö‚¦
		SwapAngle();
	}
	else
		CDXFarc::ReversePt();
}

//////////////////////////////////////////////////////////////////////
// ‚c‚w‚eƒf[ƒ^‚ÌPolylineƒNƒ‰ƒX
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
	return ( m_nPolyFlag & 1 );		// •ÂÙ°Ìß
}

//////////////////////////////////////////////////////////////////////
// ‚c‚w‚eƒf[ƒ^‚ÌTextƒNƒ‰ƒX
//////////////////////////////////////////////////////////////////////
inline CString CDXFtext::GetStrValue(void) const
{
	return m_strValue;
}

inline double CDXFtext::GetEdgeGap(const CPointD&, BOOL/*=TRUE*/)
{
	return HUGE_VAL;	// IsMatchPoint() ˆÈŠO‚Í‹ßÚ”»’f‚Ì•K—v‚È‚µ
}
