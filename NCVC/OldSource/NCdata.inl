// NCdata.inl: CNCdata クラスのインライン関数
//
//////////////////////////////////////////////////////////////////////

#if !defined(___NCDATA_INL___)
#define ___NCDATA_INL___

#include <math.h>

/////////////////////////////////////////////////////////////////////////////
// ＮＣデータのベースクラス
/////////////////////////////////////////////////////////////////////////////
inline void CNCdata::Constracter(LPNCARGV lpArgv)
{
	m_nc.dwFlags	= lpArgv->nc.dwFlags;
	m_nc.nLine		= lpArgv->nc.nLine;
	m_nc.nGtype		= lpArgv->nc.nGtype;
	m_nc.nGcode		= lpArgv->nc.nGcode;
	m_nc.enPlane	= lpArgv->nc.enPlane;
	m_nc.dwValFlags	= lpArgv->nc.dwValFlags;
	m_dFeed			= lpArgv->dFeed;
}

inline ENNCDTYPE CNCdata::GetType(void) const
{
	return m_enType;
}

inline DWORD CNCdata::GetNCFlags(void) const
{
	return	m_nc.dwFlags;
}

inline void CNCdata::SetNCFlags(DWORD dwFlags)
{
	m_nc.dwFlags  = dwFlags;
}

inline int CNCdata::GetStrLine(void) const
{
	return	m_nc.nLine;
}

inline int CNCdata::GetGtype(void) const
{
	return	m_nc.nGtype;
}

inline int CNCdata::GetGcode(void) const
{
	return	m_nc.nGcode;
}

inline ENPLANE CNCdata::GetPlane(void) const
{
	ASSERT(m_nc.enPlane==XY_PLANE || m_nc.enPlane==XZ_PLANE || m_nc.enPlane==YZ_PLANE);
	return	m_nc.enPlane;
}

inline DWORD CNCdata::GetValFlags(void) const
{
	return	m_nc.dwValFlags;
}

inline double CNCdata::GetValue(size_t a) const
{
	ASSERT(a>=0 && a<=SIZEOF(m_nc.dValue));
	return	m_nc.dValue[a];
}

inline const CPoint3D CNCdata::GetStartPoint(void) const
{
	return m_ptValS;
}

inline const CPoint3D CNCdata::GetEndPoint(void) const
{
	return m_ptValE;
}

inline double CNCdata::GetEndValue(size_t a) const
{
	return m_ptValE[a];
}

inline const CPointD CNCdata::Get2DPoint(void) const
{
	return	m_pt2D;
}

inline double CNCdata::GetCutLength(void) const
{
	return	m_nc.dLength;
}

inline double CNCdata::GetFeed(void) const
{
	return m_dFeed;
}

inline double CNCdata::GetMove(size_t a) const
{
	ASSERT(a>=0 && a<=SIZEOF(m_dMove));
	return m_dMove[a];
}

inline const CRect3D CNCdata::GetMaxRect(void) const
{
	return m_rcMax;
}

inline BOOL CNCdata::CalcRoundPoint(const CNCdata*, double, CPointD&, double&, double&)
{
	return FALSE;	// G01,G02,G03以外はｴﾗｰ
}

inline BOOL CNCdata::SetChamferingPoint(BOOL, double, CPointD&)
{
	return FALSE;
}

inline double CNCdata::CalcBetweenAngle(const CPoint3D&, const CNCdata*)
{
	return 0.0;
}

inline int CNCdata::CalcOffsetSign(const CPoint3D&)
{
	return 0;
}

inline CPointD CNCdata::CalcPerpendicularPoint(const CPoint3D&, const CPoint3D&, double, int)
{
	return CPointD(HUGE_VAL);
}

inline CPointD CNCdata::CalcOffsetIntersectionPoint(const CPoint3D&, const CNCdata*, double, int, int)
{
	return CPointD(HUGE_VAL);
}

inline CPointD CNCdata::CalcOffsetIntersectionPoint2(const CPoint3D&, const CNCdata*, double, int, int)
{
	return CPointD(HUGE_VAL);
}

inline BOOL CNCdata::SetCorrectPoint(BOOL, const CPointD&, double)
{
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// ＮＣデータの直線補間クラス
/////////////////////////////////////////////////////////////////////////////
inline EN_NCPEN CNCline::GetPenType(void)
{
	if ( m_nc.nGcode == 1 )
		return ( GetValFlags()&NCD_Z && !(GetValFlags()&(NCD_X|NCD_Y)) ) ?
				NCPEN_G1Z : NCPEN_G1;
	else
		return NCPEN_G0;	// 固定ｻｲｸﾙ含む
}

inline void CNCline::CalcLength(void)
{
	// 各軸ごとの移動長(早送り時間の計算用)
	m_dMove[NCA_X] = fabs(m_ptValE.x - m_ptValS.x);
	m_dMove[NCA_Y] = fabs(m_ptValE.y - m_ptValS.y);
	m_dMove[NCA_Z] = fabs(m_ptValE.z - m_ptValS.z);
	m_nc.dLength = sqrt(
			m_dMove[NCA_X]*m_dMove[NCA_X] +
			m_dMove[NCA_Y]*m_dMove[NCA_Y] +
			m_dMove[NCA_Z]*m_dMove[NCA_Z] );
}

inline void CNCline::SetMaxRect(void)
{
	m_rcMax.TopLeft()		= m_ptValS;
	m_rcMax.BottomRight()	= m_ptValE;
	m_rcMax.high	= m_ptValS.z;
	m_rcMax.low		= m_ptValE.z;
	m_rcMax.NormalizeRect();
}

/////////////////////////////////////////////////////////////////////////////
// ＮＣデータの固定サイクルクラス
/////////////////////////////////////////////////////////////////////////////
inline void PTCYCLE::DrawTuning(double f)
{
	ptDrawI = ptI * f;
	ptDrawR = ptR * f;
	ptDrawZ = ptZ * f;
}

inline void PTCYCLE_XY::DrawTuning(double f)
{
	rcDraw.TopLeft() = pt * f - LOMETRICFACTOR;
	rcDraw.BottomRight() = pt * f + LOMETRICFACTOR;
}

inline int CNCcycle::GetDrawCnt(void) const
{
	return m_nDrawCnt;
}

inline const PTCYCLE_XY* CNCcycle::GetCycleInsideXY(void) const
{
	return m_CycleXY;
}

inline const PTCYCLE* CNCcycle::GetCycleInsideXZ(void) const
{
	return m_CycleXZ;
}

inline const PTCYCLE* CNCcycle::GetCycleInsideYZ(void) const
{
	return m_CycleYZ;
}

inline const CPoint3D CNCcycle::GetIPoint(void) const
{
	return m_ptValI;
}

inline const CPoint3D CNCcycle::GetRPoint(void) const
{
	return m_ptValR;
}

inline double CNCcycle::GetInitialValue(void) const
{
	return m_dInitial;
}

inline double CNCcycle::GetCycleMove(void) const
{
	return m_dCycleMove;
}

inline double CNCcycle::GetDwell(void) const
{
	return m_dDwell;
}

inline BOOL CNCcycle::CalcRoundPoint(const CNCdata*, double, CPointD&, double&, double&)
{
	return FALSE;
}

inline BOOL CNCcycle::SetChamferingPoint(BOOL, double, CPointD&)
{
	return FALSE;
}

inline double CNCcycle::CalcBetweenAngle(const CPoint3D&, const CNCdata*)
{
	return 0.0;
}

inline int CNCcycle::CalcOffsetSign(const CPoint3D&)
{
	return 0;
}

inline CPointD CNCcycle::CalcPerpendicularPoint(const CPoint3D&, const CPoint3D&, double, int)
{
	return CPointD(HUGE_VAL);
}

inline CPointD CNCcycle::CalcOffsetIntersectionPoint(const CPoint3D&, const CNCdata*, double, int, int)
{
	return CPointD(HUGE_VAL);
}

inline CPointD CNCcycle::CalcOffsetIntersectionPoint2(const CPoint3D&, const CNCdata*, double, int, int)
{
	return CPointD(HUGE_VAL);
}

inline BOOL CNCcycle::SetCorrectPoint(BOOL, const CPointD&, double)
{
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// ＮＣデータの円弧補間クラス
/////////////////////////////////////////////////////////////////////////////
inline void CNCcircle::CalcLength(void)
{
	m_nc.dLength = fabs(m_r * (m_eq - m_sq));
}

inline int CNCcircle::GetG23(void) const
{
	return m_nG23;
}

inline const CPoint3D CNCcircle::GetOrg(void) const
{
	return m_ptOrg;
}

inline double CNCcircle::GetR(void) const
{
	return m_r;
}

inline double CNCcircle::GetStartAngle(void) const
{
	return m_sq;
}

inline double CNCcircle::GetEndAngle(void) const
{
	return m_eq;
}

/////////////////////////////////////////////////////////////////////////////
// ＮＣデータのブロッククラス
/////////////////////////////////////////////////////////////////////////////
inline CNCblock::CNCblock(CString strLine, CString strBlock, DWORD dwFlags/*=0*/)
{
	m_strLine	= strLine;
	m_strGcode	= strBlock;
	m_dwFlags	= dwFlags;
	m_pData		= NULL;
}

inline CString CNCblock::GetStrLine(void) const
{
	return m_strLine;
}

inline CString CNCblock::GetStrGcode(void) const
{
	return m_strGcode;
}

inline CString CNCblock::GetStrBlock(void) const
{
	return m_strLine + m_strGcode;
}

inline DWORD CNCblock::GetBlockFlag(void) const
{
	return m_dwFlags;
}

inline void CNCblock::SetBlockFlag(DWORD dwFlags, BOOL bAdd/*=TRUE*/)
{
	m_dwFlags = ( bAdd ? m_dwFlags : 0 ) | dwFlags;
}

inline CNCdata* CNCblock::GetBlockToNCdata(void) const
{
	return m_pData;
}

inline int CNCblock::GetBlockToNCdataArrayNo(void) const
{
	return m_nArray;
}

inline void CNCblock::SetBlockToNCdata(CNCdata* pData, int nArray)
{
	m_pData	 = pData;
	m_nArray = nArray;
}

#endif
