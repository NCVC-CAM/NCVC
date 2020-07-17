// NCdata.inl: CNCdata クラスのインライン関数
//
//////////////////////////////////////////////////////////////////////

#pragma once

/////////////////////////////////////////////////////////////////////////////
inline void WIREDRAW::clear(void)
{
	vpt.clear();
	vnr.clear();
	vvef.clear();
	vwl.clear();
	vLen.clear();
}

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
	m_nc.dLength	= 0.0f;
	m_nSpindle		= lpArgv->nSpindle;
	m_dFeed			= (float)lpArgv->dFeed;
	m_dEndmill		= (float)lpArgv->dEndmill;
	switch ( lpArgv->nEndmillType ) {
	case 1:
		m_dwFlags = NCMIL_BALL;
		break;
	case 2:
		m_dwFlags = NCMIL_CHAMFER;
		break;
	default:
		m_dwFlags = 0;
		break;
	}
	if ( lpArgv->bG98 )
		m_dwFlags |= NCFLG_G98;
	else
		m_dwFlags &= ~NCFLG_G98;
	ZEROCLR(m_dMove);
	m_pRead = new CNCread;
//	memcpy(&(m_pRead->m_g68),   &(lpArgv->g68),   sizeof(G68ROUND));
//	memcpy(&(m_pRead->m_taper), &(lpArgv->taper), sizeof(TAPER));
	if ( lpArgv->g68.bG68 )
		m_pRead->m_pG68 = new _G68ROUND(lpArgv->g68);
	else
		m_pRead->m_pG68 = NULL;
	if ( lpArgv->taper.nTaper!=0 || lpArgv->taper.bTonly )
		m_pRead->m_pTaper = new _TAPER(lpArgv->taper);
	else
		m_pRead->m_pTaper = NULL;
	m_pWireObj = NULL;
}

inline const CNCread* CNCdata::GetReadData(void) const
{
	return m_pRead;
}

inline void CNCdata::SetWireObj(CNCdata* pData)
{
//	ASSERT( m_pWireObj );
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

inline DWORD CNCdata::GetFlags(void) const
{
	return m_dwFlags;
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
	return GetGtype()==G_TYPE && 1<=GetGcode() && GetGcode()<=3;
}

inline BOOL CNCdata::IsCircle(void) const
{
	return GetGtype()==G_TYPE && 2<=GetGcode() && GetGcode()<=3;
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

inline float CNCdata::GetValue(size_t a) const
{
	ASSERT(a>=0 && a<=SIZEOF(m_nc.dValue));
//	return	(float)m_nc.dValue[a];
	return	m_nc.dValue[a];
}

inline const CPoint3F CNCdata::GetStartPoint(void) const
{
	return m_ptValS;
}

inline const CPoint3F CNCdata::GetEndPoint(void) const
{
	return m_ptValE;
}

inline float CNCdata::GetEndValue(size_t a) const
{
	return m_ptValE[a];
}

inline const CPoint3F CNCdata::GetOriginalEndPoint() const
{
	ASSERT( m_pRead );
	return m_pRead->m_ptValOrg;
}

inline float CNCdata::GetOriginalEndValue(size_t a) const
{
	ASSERT( m_pRead );
	return m_pRead->m_ptValOrg[a];
}

inline const CPoint3F CNCdata::GetOffsetPoint(void) const
{
	ASSERT( m_pRead );
	return m_pRead->m_ptOffset;
}

inline const CPoint3F CNCdata::GetEndCorrectPoint(void) const
{
	return m_obCdata.IsEmpty() ? GetEndPoint() : m_obCdata.GetTail()->GetEndPoint();
}

inline CNCdata* CNCdata::GetEndCorrectObject(void)
{
	return m_obCdata.IsEmpty() ? this : m_obCdata.GetTail();
}

inline const CPointF CNCdata::Get2DPoint(void) const
{
	return	m_pt2D;
}

inline float CNCdata::GetCutLength(void) const
{
	return	(float)m_nc.dLength;
}

inline int CNCdata::GetSpindle(void) const
{
	return m_nSpindle;
}

inline float CNCdata::GetFeed(void) const
{
	return m_dFeed;
}

inline float CNCdata::GetMove(size_t a) const
{
	ASSERT(a>=0 && a<=SIZEOF(m_dMove));
	return m_dMove[a];
}

inline float CNCdata::SetMove(size_t a, float m)
{
	ASSERT(a>=0 && a<=SIZEOF(m_dMove));
	m_dMove[a] = m;
	return m_dMove[a];
}

inline float CNCdata::GetEndmill(void) const
{
	return m_dEndmill;
}

inline int CNCdata::GetEndmillType(void) const
{
	return (int)(GetFlags() & NCFLG_ENDMILL);
}

inline BOOL CNCdata::GetG98(void) const
{
	return GetFlags() & NCFLG_G98;
}

inline CRect3F CNCdata::GetMaxRect(void) const
{
	CRect3F	rcResult;
	rcResult.SetRectMinimum();
	return rcResult;
}

inline CRect3F CNCdata::GetMaxCutRect(void) const
{
	return CNCdata::GetMaxRect();
}

inline CNCarray* CNCdata::GetCorrectArray(void)
{
	return &m_obCdata;
}

inline boost::tuple<BOOL, CPointF, float, float> CNCdata::CalcRoundPoint(const CNCdata*, float) const
{
	BOOL	bResult = FALSE;
	float	rr1 = 0.0f, rr2 = 0.0f;
	return boost::make_tuple(bResult, CPointF(), rr1, rr2);
}

inline boost::optional<CPointF> CNCdata::SetChamferingPoint(BOOL, float)
{
	return boost::optional<CPointF>();
}

inline float CNCdata::CalcBetweenAngle(const CNCdata*) const
{
	return 0.0f;
}

inline int CNCdata::CalcOffsetSign(void) const
{
	return 0;
}

inline boost::optional<CPointF> CNCdata::CalcPerpendicularPoint(ENPOINTORDER, float, int) const
{
	return boost::optional<CPointF>();
}

inline boost::optional<CPointF> CNCdata::CalcOffsetIntersectionPoint(const CNCdata*, float, float, BOOL) const
{
	return boost::optional<CPointF>();
}

inline boost::optional<CPointF> CNCdata::CalcOffsetIntersectionPoint2(const CNCdata*, float, BOOL) const
{
	return boost::optional<CPointF>();
}

inline void CNCdata::SetCorrectPoint(ENPOINTORDER, const CPointF&, float)
{
}

inline float CNCdata::SetCalcLength(void)
{
	ZEROCLR(m_dMove);
	m_nc.dLength = 0.0f;
	return 0.0f;
}

/////////////////////////////////////////////////////////////////////////////
// ＮＣデータの直線補間クラス
/////////////////////////////////////////////////////////////////////////////
inline EN_NCPEN CNCline::GetPenType(void) const
{
	if ( GetGcode() == 1 )
		return ( GetValFlags()&NCD_Z && !(GetValFlags()&(NCD_X|NCD_Y)) ) ?
				NCPEN_G1Z : NCPEN_G1;
	else
		return NCPEN_G0;	// 固定ｻｲｸﾙ含む
}

inline int CNCline::GetLineType(void) const
{
	if ( GetGcode() == 1 )
		return ( GetValFlags()&NCD_Z && !(GetValFlags()&(NCD_X|NCD_Y)) ) ?
				NCCOLLINE_G1Z : NCCOLLINE_G1;
	else
		return NCCOLLINE_G0;
}

inline CRect3F CNCline::GetMaxRect(void) const
{
	CRect3F	rcResult;
	
	rcResult.TopLeft()		= m_ptValS.GetXY();
	rcResult.BottomRight()	= m_ptValE.GetXY();
	rcResult.high			= m_ptValS.z;
	rcResult.low			= m_ptValE.z;
	rcResult.NormalizeRect();

	return rcResult;
}

inline CRect3F CNCline::GetMaxCutRect(void) const
{
	return ( GetGcode() == 1 ) ?
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
inline void PTCYCLE::DrawTuning(const float f)
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

inline const CPoint3F CNCcycle::GetIPoint(void) const
{
	return m_ptValI;
}

inline const CPoint3F CNCcycle::GetRPoint(void) const
{
	return m_ptValR;
}

inline float CNCcycle::GetInitialValue(void) const
{
	return m_dInitial;
}

inline float CNCcycle::GetCycleMove(void) const
{
	return m_dCycleMove;
}

inline float CNCcycle::GetDwell(void) const
{
	return m_dDwell;
}

inline CRect3F CNCcycle::GetMaxRect(void) const
{
	CRect3F	rcResult(CNCline::GetMaxRect());
	rcResult |= CNCcycle::GetMaxCutRect();
	return rcResult;
}

inline CRect3F CNCcycle::GetMaxCutRect(void) const
{
	CRect3F	rcResult;
	rcResult.SetRectMinimum();

	if ( m_nDrawCnt > 0 ) {
		CRect3F	rc(
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

inline boost::tuple<BOOL, CPointF, float, float> CNCcycle::CalcRoundPoint(const CNCdata*, float) const
{
	BOOL	bResult = FALSE;
	float	rr1 = 0.0f, rr2 = 0.0f;
	return boost::make_tuple(bResult, CPointF(), rr1, rr2);
}

inline boost::optional<CPointF> CNCcycle::SetChamferingPoint(BOOL, float)
{
	return boost::optional<CPointF>();
}

inline float CNCcycle::CalcBetweenAngle(const CNCdata*) const
{
	return 0.0f;
}

inline int CNCcycle::CalcOffsetSign(void) const
{
	return 0;
}

inline boost::optional<CPointF> CNCcycle::CalcPerpendicularPoint(ENPOINTORDER, float, int) const
{
	return boost::optional<CPointF>();
}

inline boost::optional<CPointF> CNCcycle::CalcOffsetIntersectionPoint(const CNCdata*, float, float, BOOL) const
{
	return boost::optional<CPointF>();
}

inline boost::optional<CPointF> CNCcycle::CalcOffsetIntersectionPoint2(const CNCdata*, float, BOOL) const
{
	return boost::optional<CPointF>();
}

inline void CNCcycle::SetCorrectPoint(ENPOINTORDER, const CPointF&, float)
{
}

inline float CNCcycle::SetCalcLength(void)
{
	// ｺﾝｽﾄﾗｸﾀで計算済み
	return (float)m_nc.dLength;
}

/////////////////////////////////////////////////////////////////////////////
// ＮＣデータの円弧補間クラス
/////////////////////////////////////////////////////////////////////////////
inline BOOL CNCcircle::GetG03(void) const
{
	return GetFlags() & NCFLG_G02G03;
}

inline boost::tuple<float, float> CNCcircle::GetSqEq(void) const
{
	return ( GetG03() ) ?
		boost::make_tuple(m_sq, m_eq) :
		boost::make_tuple(m_eq, m_sq);
}

inline const CPoint3F CNCcircle::GetOrg(void) const
{
	return m_ptOrg;
}

inline float CNCcircle::GetR(void) const
{
	return m_r;
}

inline float CNCcircle::GetStartAngle(void) const
{
	return m_sq;
}

inline float CNCcircle::GetEndAngle(void) const
{
	return m_eq;
}

inline CRect3F CNCcircle::GetMaxCutRect(void) const
{
	return CNCcircle::GetMaxRect();
}

/////////////////////////////////////////////////////////////////////////////
// ＮＣデータのブロッククラス
/////////////////////////////////////////////////////////////////////////////
inline CNCblock::CNCblock
	(const CString& strBuf, DWORD dwFlags/*=0*/)
{
	extern	LPCTSTR	g_szLineDelimiter;	// "%N0123456789" from NCDoc.cpp
	m_strLine	= strBuf.SpanIncluding(g_szLineDelimiter);
	m_strGcode	= strBuf.Mid(m_strLine.GetLength()).Trim();
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

inline size_t CNCblock::GetBlockToNCdataArrayNo(void) const
{
	return m_nArray;
}

inline void CNCblock::SetBlockToNCdata(CNCdata* pData, size_t nArray)
{
	m_pData	 = pData;
	m_nArray = nArray;
}
