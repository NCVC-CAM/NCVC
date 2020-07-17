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
	m_nSpindle		= lpArgv->nSpindle;
	m_dFeed			= lpArgv->dFeed;
	m_dEndmill		= lpArgv->dEndmill;
	m_nEndmillType	= lpArgv->nEndmillType;
	m_bG98			= lpArgv->bG98;
	m_dMove[NCA_X] = m_dMove[NCA_Y] = m_dMove[NCA_Z] = 0.0;
	m_pRead = new CNCread;
	memcpy(&(m_pRead->m_g68),   &(lpArgv->g68),   sizeof(G68ROUND));
	memcpy(&(m_pRead->m_taper), &(lpArgv->taper), sizeof(TAPER));
	m_pWireObj = NULL;
}

inline const CNCread* CNCdata::GetReadData(void) const
{
	return m_pRead;
}

inline void CNCdata::SetWireObj(CNCdata* pData)
{
	m_pWireObj = pData;
}

inline CNCdata* CNCdata::GetWireObj(void) const
{
	return m_pWireObj;
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

inline int CNCdata::GetBlockLineNo(void) const
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

inline BOOL CNCdata::IsCutCode(void) const
{
	return 1<=m_nc.nGcode && m_nc.nGcode<=3;
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

inline const CPoint3D CNCdata::GetOriginalEndPoint() const
{
	ASSERT( m_pRead );
	return m_pRead->m_ptValOrg;
}

inline double CNCdata::GetOriginalEndValue(size_t a) const
{
	ASSERT( m_pRead );
	return m_pRead->m_ptValOrg[a];
}

inline const CPoint3D CNCdata::GetOffsetPoint(void) const
{
	ASSERT( m_pRead );
	return m_pRead->m_ptOffset;
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

inline int CNCdata::GetSpindle(void) const
{
	return m_nSpindle;
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

inline double  CNCdata::SetMove(size_t a, double m)
{
	ASSERT(a>=0 && a<=SIZEOF(m_dMove));
	m_dMove[a] = m;
	return m_dMove[a];
}

inline double CNCdata::GetEndmill(void) const
{
	return m_dEndmill;
}

inline int CNCdata::GetEndmillType(void) const
{
	return m_nEndmillType;
}

inline BOOL CNCdata::GetG98(void) const
{
	return m_bG98;
}

inline CRect3D CNCdata::GetMaxRect(void) const
{
	CRect3D	rcResult;
	rcResult.SetRectMinimum();
	return rcResult;
}

inline CRect3D CNCdata::GetMaxCutRect(void) const
{
	return CNCdata::GetMaxRect();
}

inline CNCarray* CNCdata::GetCorrectArray(void)
{
	return &m_obCdata;
}

inline boost::tuple<BOOL, CPointD, double, double> CNCdata::CalcRoundPoint(const CNCdata*, double) const
{
	BOOL	bResult = FALSE;
	double	rr1 = 0.0, rr2 = 0.0;
	return boost::make_tuple(bResult, CPointD(), rr1, rr2);
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

inline boost::optional<CPointD> CNCdata::CalcOffsetIntersectionPoint(const CNCdata*, double, BOOL) const
{
	return boost::optional<CPointD>();
}

inline boost::optional<CPointD> CNCdata::CalcOffsetIntersectionPoint2(const CNCdata*, double, BOOL) const
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

inline int CNCline::GetLineType(void) const
{
	if ( m_nc.nGcode == 1 )
		return ( GetValFlags()&NCD_Z && !(GetValFlags()&(NCD_X|NCD_Y)) ) ?
				NCCOLLINE_G1Z : NCCOLLINE_G1;
	else
		return NCCOLLINE_G0;
}

inline CRect3D CNCline::GetMaxRect(void) const
{
	CRect3D	rcResult;
	
	rcResult.TopLeft()		= m_ptValS.GetXY();
	rcResult.BottomRight()	= m_ptValE.GetXY();
	rcResult.high			= m_ptValS.z;
	rcResult.low			= m_ptValE.z;
	rcResult.NormalizeRect();

	return rcResult;
}

inline CRect3D CNCline::GetMaxCutRect(void) const
{
	return ( m_nc.nGcode == 1 ) ?
		CNCline::GetMaxRect() :
		CNCdata::GetMaxRect();
}

inline CPoint CNCline::GetDrawStartPoint(size_t n) const
{
	return m_ptDrawS[n];
}

inline CPoint CNCline::GetDrawEndPoint(size_t n) const
{
	return m_ptDrawE[n];
}

/////////////////////////////////////////////////////////////////////////////
// ＮＣデータの固定サイクルクラス
/////////////////////////////////////////////////////////////////////////////
inline void PTCYCLE::DrawTuning(const double f)
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

inline CRect3D CNCcycle::GetMaxRect(void) const
{
	CRect3D	rcResult(CNCline::GetMaxRect());
	rcResult |= CNCcycle::GetMaxCutRect();
	return rcResult;
}

inline CRect3D CNCcycle::GetMaxCutRect(void) const
{
	CRect3D	rcResult;
	rcResult.SetRectMinimum();

	if ( m_nDrawCnt > 0 ) {
		CRect3D	rc(
			m_Cycle3D[0].ptR.x,				// left
			m_Cycle3D[0].ptR.y,				// top
			m_Cycle3D[m_nDrawCnt-1].ptC.x,	// right
			m_Cycle3D[m_nDrawCnt-1].ptC.y,	// bottom
			m_Cycle3D[0].ptR.z,				// high
			m_Cycle3D[m_nDrawCnt-1].ptC.z	// low
		);
		rc.NormalizeRect();
		rcResult |= rc;
	}

	return rcResult;
}

inline boost::tuple<BOOL, CPointD, double, double> CNCcycle::CalcRoundPoint(const CNCdata*, double) const
{
	BOOL	bResult = FALSE;
	double	rr1 = 0.0, rr2 = 0.0;
	return boost::make_tuple(bResult, CPointD(), rr1, rr2);
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

inline boost::optional<CPointD> CNCcycle::CalcOffsetIntersectionPoint(const CNCdata*, double, BOOL) const
{
	return boost::optional<CPointD>();
}

inline boost::optional<CPointD> CNCcycle::CalcOffsetIntersectionPoint2(const CNCdata*, double, BOOL) const
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

inline boost::tuple<double, double> CNCcircle::GetSqEq(void) const
{
	using namespace boost;
	return ( m_nG23 == 0 ) ?
		make_tuple(m_eq, m_sq) :
		make_tuple(m_sq, m_eq);
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

inline CRect3D CNCcircle::GetMaxCutRect(void) const
{
	return CNCcircle::GetMaxRect();
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
