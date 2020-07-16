// NCdata.inl: CNCdata クラスのインライン関数
//
//////////////////////////////////////////////////////////////////////

#pragma once

/////////////////////////////////////////////////////////////////////////////
// ＮＣデータのベースクラス
/////////////////////////////////////////////////////////////////////////////
inline void CNCdata::Constracter(LPNCARGV lpArgv)
{
	m_nc.nErrorCode	= lpArgv->nc.nErrorCode;
	m_nc.nLine		= lpArgv->nc.nLine;
	m_nc.nGtype		= lpArgv->nc.nGtype;
	m_nc.nGcode		= lpArgv->nc.nGcode;
	m_nc.enPlane	= lpArgv->nc.enPlane;
	m_nc.dwValFlags	= lpArgv->nc.dwValFlags;
	m_nc.dLength	= 0.0;
	m_dFeed			= lpArgv->dFeed;
	m_dMove[NCA_X] = m_dMove[NCA_Y] = m_dMove[NCA_Z] = 0.0;
}

inline void CNCdata::AddCorrectObject(CNCdata* pData)
{
	m_obCdata.Add(pData);
}

inline ENNCDTYPE CNCdata::GetType(void) const
{
	return m_enType;
}

inline UINT CNCdata::GetNCObjErrorCode(void) const
{
	return	m_nc.nErrorCode;
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

inline const CPoint3D CNCdata::GetEndCorrectPoint(void) const
{
	return m_obCdata.IsEmpty() ? GetEndPoint() : m_obCdata.GetTail()->GetEndPoint();
}

inline CNCdata* CNCdata::GetEndCorrectObject(void)
{
	return m_obCdata.IsEmpty() ? this : m_obCdata.GetTail();
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

inline boost::tuple<BOOL, CPointD, double, double> CNCdata::CalcRoundPoint(const CNCdata*, double) const
{
	return boost::make_tuple(FALSE, CPointD(), 0.0, 0.0);
}

inline boost::optional<CPointD> CNCdata::SetChamferingPoint(BOOL, double)
{
	return boost::optional<CPointD>();
}

inline double CNCdata::CalcBetweenAngle(const CNCdata*) const
{
	return 0.0;
}

inline int CNCdata::CalcOffsetSign(void) const
{
	return 0;
}

inline boost::optional<CPointD> CNCdata::CalcPerpendicularPoint(ENPOINTORDER, double, int) const
{
	return boost::optional<CPointD>();
}

inline boost::optional<CPointD> CNCdata::CalcOffsetIntersectionPoint(const CNCdata*, double, int, int) const
{
	return boost::optional<CPointD>();
}

inline boost::optional<CPointD> CNCdata::CalcOffsetIntersectionPoint2(const CNCdata*, double, int, int) const
{
	return boost::optional<CPointD>();
}

inline void CNCdata::SetCorrectPoint(ENPOINTORDER, const CPointD&, double)
{
}

inline double CNCdata::SetCalcLength(void)
{
	m_dMove[NCA_X] = m_dMove[NCA_Y] = m_dMove[NCA_Z] = 0.0;
	m_nc.dLength = 0.0;
	return 0.0;
}

/////////////////////////////////////////////////////////////////////////////
// ＮＣデータの直線補間クラス
/////////////////////////////////////////////////////////////////////////////
inline EN_NCPEN CNCline::GetPenType(void) const
{
	if ( m_nc.nGcode == 1 )
		return ( GetValFlags()&NCD_Z && !(GetValFlags()&(NCD_X|NCD_Y)) ) ?
				NCPEN_G1Z : NCPEN_G1;
	else
		return NCPEN_G0;	// 固定ｻｲｸﾙ含む
}

inline void CNCline::SetMaxRect(void)
{
	m_rcMax.TopLeft()		= m_ptValS.GetXY();
	m_rcMax.BottomRight()	= m_ptValE.GetXY();
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
	ptDrawC = ptC * f;
	rcDraw.TopLeft() = ptI * f - LOMETRICFACTOR;
	rcDraw.BottomRight() = ptI * f + LOMETRICFACTOR;
}

inline int CNCcycle::GetDrawCnt(void) const
{
	return m_nDrawCnt;
}

inline const PTCYCLE* CNCcycle::GetCycleInside(size_t n) const
{
	return m_Cycle[n];
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

inline boost::tuple<BOOL, CPointD, double, double> CNCcycle::CalcRoundPoint(const CNCdata*, double) const
{
	return boost::make_tuple(FALSE, CPointD(), 0.0, 0.0);
}

inline boost::optional<CPointD> CNCcycle::SetChamferingPoint(BOOL, double)
{
	return boost::optional<CPointD>();
}

inline double CNCcycle::CalcBetweenAngle(const CNCdata*) const
{
	return 0.0;
}

inline int CNCcycle::CalcOffsetSign(void) const
{
	return 0;
}

inline boost::optional<CPointD> CNCcycle::CalcPerpendicularPoint(ENPOINTORDER, double, int) const
{
	return boost::optional<CPointD>();
}

inline boost::optional<CPointD> CNCcycle::CalcOffsetIntersectionPoint(const CNCdata*, double, int, int) const
{
	return boost::optional<CPointD>();
}

inline boost::optional<CPointD> CNCcycle::CalcOffsetIntersectionPoint2(const CNCdata*, double, int, int) const
{
	return boost::optional<CPointD>();
}

inline void CNCcycle::SetCorrectPoint(ENPOINTORDER, const CPointD&, double)
{
}

inline double CNCcycle::SetCalcLength(void)
{
	// ｺﾝｽﾄﾗｸﾀで計算済み
	return m_nc.dLength;
}

/////////////////////////////////////////////////////////////////////////////
// ＮＣデータの円弧補間クラス
/////////////////////////////////////////////////////////////////////////////
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
inline CNCblock::CNCblock
	(const CString& strLine, const CString& strBlock, DWORD dwFlags/*=0*/)
{
	m_strLine	= strLine;
	m_strGcode	= strBlock;
	m_dwFlags	= dwFlags;
	m_nError	= 0;
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
	if ( bAdd )
		m_dwFlags |= dwFlags;
	else
		m_dwFlags  = dwFlags;
}

inline void CNCblock::SetNCBlkErrorCode(UINT nError)
{
	if ( nError == 0 || m_nError == 0 )	// ｸﾘｱか初回ｴﾗｰｺｰﾄﾞのみｾｯﾄ
		m_nError = nError;
}

inline UINT CNCblock::GetNCBlkErrorCode(void) const
{
	return m_nError;
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
