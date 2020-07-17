// NCdata.cpp: CNCdata クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "NCdata.h"
#include "ViewOption.h"

//#define	_DEBUGDRAW_NCD		// 描画処理もﾛｸﾞ
#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

using namespace boost;

extern	const	DWORD		g_dwSetValFlags[];	// NCDoc.cpp
extern	const	PENSTYLE	g_penStyle[];		// ViewOption.cpp

// CalcRoundPoint() ｻﾌﾞ
// --- 円同士の内外半径計算
static	optional<double>	CalcRoundPoint_CircleInOut(const CPointD&, const CPointD&, int, int, double);

//////////////////////////////////////////////////////////////////////
// NCﾃﾞｰﾀの基礎ﾃﾞｰﾀｸﾗｽ
//////////////////////////////////////////////////////////////////////

// 初回登録用ｺﾝｽﾄﾗｸﾀ
CNCdata::CNCdata(LPNCARGV lpArgv)
{
	int		i;
	Constracter(lpArgv);
	for ( i=0; i<VALUESIZE; i++ )
		m_nc.dValue[i] = lpArgv->nc.dValue[i];
	for ( i=0; i<NCXYZ; i++ ) {
		m_pRead->m_ptValOrg[i] = m_ptValS[i] = m_ptValE[i] = m_nc.dValue[i];
	}
	m_pt2D = m_ptValE.PointConvert();
	m_enType = NCDBASEDATA;
}

// 切削(描画)ｺｰﾄﾞ以外のｺﾝｽﾄﾗｸﾀ
CNCdata::CNCdata(const CNCdata* pData, LPNCARGV lpArgv, const CPoint3D& ptOffset)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CNCdata", DBG_MAGENTA);
#endif
	int		i;

	Constracter(lpArgv);
	// 座標指定のないﾃﾞｰﾀは前回計算座標から取得
	for ( i=0; i<NCXYZ; i++ ) {
		// 指定されている分だけ代入
		m_nc.dValue[i] = m_nc.dwValFlags & g_dwSetValFlags[i] ?
			lpArgv->nc.dValue[i] : pData->GetValue(i);
	}
	m_ptValS = m_ptValE = pData->GetEndPoint();
	m_pRead->m_ptValOrg = pData->GetOriginalEndPoint();
	m_pRead->m_ptOffset = ptOffset;
	// 座標値以外も指定されている分は代入
	for ( ; i<VALUESIZE; i++ )
		m_nc.dValue[i] = m_nc.dwValFlags & g_dwSetValFlags[i] ?
			lpArgv->nc.dValue[i] : pData->GetValue(i);
	// 前回の計算値を引き継ぎ
	m_pt2D   = pData->Get2DPoint();
	m_rcMax  = pData->GetMaxRect();
	m_enType = NCDBASEDATA;

#ifdef _DEBUG
	DbgDump();
	Dbg_sep();
#endif
}

// 派生ｸﾗｽ用ｺﾝｽﾄﾗｸﾀ
CNCdata::CNCdata
	(ENNCDTYPE enType, const CNCdata* pData, LPNCARGV lpArgv, const CPoint3D& ptOffset)
{
	int	i;

	Constracter(lpArgv);
	// 座標指定のないﾃﾞｰﾀは前回純粋座標から補間
	for ( i=0; i<NCXYZ; i++ ) {
		// 指定されている分だけ代入
		if ( m_nc.dwValFlags & g_dwSetValFlags[i] ) {
			m_nc.dValue[i] = lpArgv->nc.dValue[i];
			if ( !lpArgv->bAbs )		// ｲﾝｸﾘﾒﾝﾀﾙ補正
				m_nc.dValue[i] += pData->GetOriginalEndValue(i);	// ｵﾘｼﾞﾅﾙ値で加算
		}
		else
			m_nc.dValue[i] = pData->GetOriginalEndValue(i);
	}
	for ( ; i<VALUESIZE; i++ )
		m_nc.dValue[i] = m_nc.dwValFlags & g_dwSetValFlags[i] ?
			lpArgv->nc.dValue[i] : 0.0;
	// ---------------------------------------------------------------
	// m_ptValS, m_ptValE, m_ptValOrg, m_pt2D, m_rcMax は派生ｸﾗｽで代入
	// m_nc.dLength は TH_Cuttime.cpp にてセット
	// ---------------------------------------------------------------
	m_enType = enType;
	m_pRead->m_ptOffset = ptOffset;
}

// 複製用ｺﾝｽﾄﾗｸﾀ
CNCdata::CNCdata(const CNCdata* pData)
{
	int		i;
	m_enType = pData->GetType();
	m_nc.nErrorCode	= pData->GetNCObjErrorCode();
	m_nc.nLine		= pData->GetBlockLineNo();
	m_nc.nGtype		= pData->GetGtype();
	m_nc.nGcode		= pData->GetGcode();
	m_nc.enPlane	= pData->GetPlane();
	m_nc.dLength	= pData->GetCutLength();
	m_nc.dwValFlags	= pData->GetValFlags();
	for ( i=0; i<VALUESIZE; i++ )
		m_nc.dValue[i] = pData->GetValue(i);
	for ( i=0; i<NCXYZ; i++ )
		m_dMove[i] = pData->GetMove(i);
	m_dFeed		= pData->GetFeed();
	m_ptValS	= pData->GetStartPoint();
	m_ptValE	= pData->GetEndPoint();
	m_pRead = new CNCread;
	m_pRead->m_ptValOrg = pData->GetOriginalEndPoint();
	m_pRead->m_ptOffset = pData->GetOffsetPoint();
	memcpy(&(m_pRead->m_g68), &(pData->GetReadData()->m_g68), sizeof(G68ROUND));
}

CNCdata::~CNCdata()
{
	for ( int i=0; i<m_obCdata.GetSize(); i++ )
		delete	m_obCdata[i];
	if ( m_pRead )
		delete	m_pRead;
}

CPointD CNCdata::GetPlaneValue(const CPoint3D& ptVal) const
{
	CPointD	pt;
	switch ( GetPlane() ) {
	case XY_PLANE:
		pt = ptVal.GetXY();
		break;
	case XZ_PLANE:
		pt = ptVal.GetXZ();
		break;
	case YZ_PLANE:
		pt = ptVal.GetYZ();
		break;
	}
	return pt;
}

void CNCdata::SetPlaneValue(const CPointD& pt, CPoint3D& ptResult)
{
	switch ( GetPlane() ) {
	case XY_PLANE:
		ptResult.x = pt.x;
		ptResult.y = pt.y;
		break;
	case XZ_PLANE:
		ptResult.x = pt.x;
		ptResult.z = pt.y;
		break;
	case YZ_PLANE:
		ptResult.y = pt.x;
		ptResult.z = pt.y;
		break;
	}
}

CPointD	CNCdata::GetPlaneValueOrg(const CPoint3D& pt1, const CPoint3D& pt2) const
{
	CPointD	pt;
	switch ( GetPlane() ) {
	case XY_PLANE:
		pt = pt1.GetXY() - pt2.GetXY();
		break;
	case XZ_PLANE:
		pt = pt1.GetXZ() - pt2.GetXZ();
		break;
	case YZ_PLANE:
		pt = pt1.GetYZ() - pt2.GetYZ();
		break;
	}
	return pt;
}

void CNCdata::CalcG68Round(LPG68ROUND lpG68, CPoint3D& ptResult)
{
	CPoint3D	ptOrg(lpG68->dOrg[NCA_X], lpG68->dOrg[NCA_Y], lpG68->dOrg[NCA_Z]);
	CPointD		pt(GetPlaneValueOrg(ptResult, ptOrg));
	pt.RoundPoint(lpG68->dRound);
	SetPlaneValue(pt, ptResult);
	ptResult += ptOrg;
}

CNCdata* CNCdata::NC_CopyObject(void)
{
	CNCdata*	pData;

	switch ( GetType() ) {
	case NCDLINEDATA:
		pData = new CNCline(this);
		break;
	case NCDARCDATA:
		pData = new CNCcircle(this);
		break;
	default:
		pData = NULL;	// 他は工具径補正に無関係(ｴﾗｰ)
	}

	return pData;
}

void CNCdata::DrawTuning(double f)
{
	for ( int i=0; i<m_obCdata.GetSize(); i++ )
		m_obCdata[i]->DrawTuning(f);
}

void CNCdata::DrawTuningXY(double f)
{
	for ( int i=0; i<m_obCdata.GetSize(); i++ )
		m_obCdata[i]->DrawTuningXY(f);
}

void CNCdata::DrawTuningXZ(double f)
{
	for ( int i=0; i<m_obCdata.GetSize(); i++ )
		m_obCdata[i]->DrawTuningXZ(f);
}

void CNCdata::DrawTuningYZ(double f)
{
	for ( int i=0; i<m_obCdata.GetSize(); i++ )
		m_obCdata[i]->DrawTuningYZ(f);
}

void CNCdata::Draw(CDC* pDC, BOOL bSelect, BOOL bCorrect) const
{
	for ( int i=0; i<m_obCdata.GetSize(); i++ )
		m_obCdata[i]->Draw(pDC, bSelect, bCorrect);
}

void CNCdata::DrawXY(CDC* pDC, BOOL bSelect, BOOL bCorrect) const
{
	for ( int i=0; i<m_obCdata.GetSize(); i++ )
		m_obCdata[i]->DrawXY(pDC, bSelect, bCorrect);
}

void CNCdata::DrawXZ(CDC* pDC, BOOL bSelect, BOOL bCorrect) const
{
	for ( int i=0; i<m_obCdata.GetSize(); i++ )
		m_obCdata[i]->DrawXZ(pDC, bSelect, bCorrect);
}

void CNCdata::DrawYZ(CDC* pDC, BOOL bSelect, BOOL bCorrect) const
{
	for ( int i=0; i<m_obCdata.GetSize(); i++ )
		m_obCdata[i]->DrawYZ(pDC, bSelect, bCorrect);
}

void CNCdata::DrawGL(BOOL bCorrect) const
{
	for ( int i=0; i<m_obCdata.GetSize(); i++ )
		m_obCdata[i]->DrawGL(bCorrect);
}

#ifdef _DEBUG
void CNCdata::DbgDump(void)
{
	CMagaDbg	dbg("CNCdata", DBG_MAGENTA);
	extern	LPCTSTR	g_szGdelimiter;	// "GSMOF" from NCDoc.cpp
	extern	LPCTSTR	g_szNdelimiter; // "XYZRIJKPLDH";

	CString	strBuf, strTmp;
	if ( GetGtype()<0 || GetGtype()>GTYPESIZE )
		strBuf.Format("%s%d: ", "NO_TYPE:", GetGcode());
	else
		strBuf.Format("%c%02d: ", g_szGdelimiter[GetGtype()], GetGcode());
	for ( int i=0; i<VALUESIZE; i++ ) {
		if ( GetValFlags() & g_dwSetValFlags[i] ) {
			strTmp.Format("%c%.3f", g_szNdelimiter[i], GetValue(i));
			strBuf += strTmp;
		}
	}
	dbg.printf("%s", strBuf);
}
#endif

//////////////////////////////////////////////////////////////////////
// CNCline クラス
//////////////////////////////////////////////////////////////////////
CNCline::CNCline(const CNCdata* pData, LPNCARGV lpArgv, const CPoint3D& ptOffset) :
	CNCdata(NCDLINEDATA, pData, lpArgv, ptOffset)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CNCline", DBG_MAGENTA);
#endif
	// 描画始点を前回の計算値から取得
	m_pt2Ds  = pData->Get2DPoint();
	m_ptValS = pData->GetEndPoint();
	// 最終座標(==指定座標)ｾｯﾄ
	m_ptValE.SetPoint(GetValue(NCA_X), GetValue(NCA_Y), GetValue(NCA_Z));
	m_pRead->m_ptValOrg = m_ptValE;
	m_ptValE += ptOffset;
	// 座標回転
	if ( lpArgv->g68.bG68 )
		CalcG68Round(&(lpArgv->g68), m_ptValE);
	// 描画終点を計算し保存
	m_pt2D = m_ptValE.PointConvert();

	// 空間占有矩形
	SetMaxRect();

#ifdef _DEBUG
	DbgDump();
	Dbg_sep();
#endif
}

CNCline::CNCline(const CNCdata* pData) : CNCdata(pData)
{
	m_pt2Ds	= m_ptValS.PointConvert();
	m_pt2D	= m_ptValE.PointConvert();
}

double CNCline::SetCalcLength(void)
{
	CPoint3D	pt;

	if ( m_obCdata.IsEmpty() ) {
		// 各軸ごとの移動長(早送り時間の計算用)
		pt = m_ptValE - m_ptValS;
		m_dMove[NCA_X] = fabs(pt.x);
		m_dMove[NCA_Y] = fabs(pt.y);
		m_dMove[NCA_Z] = fabs(pt.z);
		// 移動長
		m_nc.dLength = pt.hypot();
	}
	else {
		m_nc.dLength = m_dMove[NCA_X] = m_dMove[NCA_Y] = m_dMove[NCA_Z] = 0.0;
		// 各補正要素の合計
		for ( int i=0; i<m_obCdata.GetSize(); i++ ) {
			pt = m_obCdata[i]->GetEndPoint() - m_obCdata[i]->GetStartPoint();
			m_dMove[NCA_X] += fabs(pt.x);
			m_dMove[NCA_Y] += fabs(pt.y);
			m_dMove[NCA_Z] += fabs(pt.z);
			m_nc.dLength += pt.hypot();
		}
	}

	return m_nc.dLength;
}

void CNCline::DrawTuning(double f)
{
	m_ptDrawS[0] = m_pt2Ds * f;
	m_ptDrawE[0] = m_pt2D  * f;
	CNCdata::DrawTuning(f);
}

void CNCline::DrawTuningXY(double f)
{
	m_ptDrawS[1] = m_ptValS.GetXY() * f;
	m_ptDrawE[1] = m_ptValE.GetXY() * f;
	CNCdata::DrawTuningXY(f);
}

void CNCline::DrawTuningXZ(double f)
{
	m_ptDrawS[2] = m_ptValS.GetXZ() * f;
	m_ptDrawE[2] = m_ptValE.GetXZ() * f;
	CNCdata::DrawTuningXZ(f);
}

void CNCline::DrawTuningYZ(double f)
{
	m_ptDrawS[3] = m_ptValS.GetYZ() * f;
	m_ptDrawE[3] = m_ptValE.GetYZ() * f;
	CNCdata::DrawTuningYZ(f);
}

void CNCline::Draw(CDC* pDC, BOOL bSelect, BOOL bCorrect) const
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCline::Draw()", DBG_RED);
	dbg.printf("Line=%d", GetBlockLineNo());
#endif
	DrawLine(pDC, 0, bSelect, bCorrect);
	CNCdata::Draw(pDC, bSelect, TRUE);
}

void CNCline::DrawXY(CDC* pDC, BOOL bSelect, BOOL bCorrect) const
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCline::DrawXY()", DBG_RED);
	dbg.printf("Line=%d", GetBlockLineNo());
#endif
	DrawLine(pDC, 1, bSelect, bCorrect);
	CNCdata::DrawXY(pDC, bSelect, TRUE);
}

void CNCline::DrawXZ(CDC* pDC, BOOL bSelect, BOOL bCorrect) const
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCline::DrawXZ()", DBG_RED);
	dbg.printf("Line=%d", GetBlockLineNo());
#endif
	DrawLine(pDC, 2, bSelect, bCorrect);
	CNCdata::DrawXZ(pDC, bSelect, TRUE);
}

void CNCline::DrawYZ(CDC* pDC, BOOL bSelect, BOOL bCorrect) const
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCline::DrawYZ()", DBG_RED);
	dbg.printf("Line=%d", GetBlockLineNo());
#endif
	DrawLine(pDC, 3, bSelect, bCorrect);
	CNCdata::DrawYZ(pDC, bSelect, TRUE);
}

void CNCline::DrawLine(CDC* pDC, size_t n, BOOL bSelect, BOOL bCorrect) const
{
	CPen*	pOldPen;
	if ( bSelect )
		pOldPen = AfxGetNCVCMainWnd()->GetPenCom(COMPEN_SEL);
	else
		pOldPen = AfxGetNCVCMainWnd()->GetPenNC(bCorrect ? NCPEN_CORRECT : GetPenType());
	pOldPen = pDC->SelectObject(pOldPen);
	pDC->MoveTo(m_ptDrawS[n]);
	pDC->LineTo(m_ptDrawE[n]);
	pDC->SelectObject(pOldPen);
}

void CNCline::DrawGL(BOOL bCorrect) const
{
	const CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();
	COLORREF	col = pOpt->GetNcDrawColor(bCorrect ? NCCOL_CORRECT : (GetPenType()+NCCOL_G0));
	::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(GetLineType())].nGLpattern);
	::glBegin(GL_LINES);
	::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
	::glVertex3d(m_ptValS.x, m_ptValS.y, m_ptValS.z);
	::glVertex3d(m_ptValE.x, m_ptValE.y, m_ptValE.z);
	::glEnd();
	CNCdata::DrawGL(TRUE);
}

tuple<BOOL, CPointD, double, double> CNCline::CalcRoundPoint
	(const CNCdata* pNext, double r) const
{
	BOOL	bResult;
	double	rr1, rr2;
	// ２線の交点(自身の終点)が原点になるように補正
	CPointD	pt, pts( GetPlaneValueOrg(m_ptValS, m_ptValE) );

	if ( pNext->GetType() == NCDARCDATA ) {
		const CNCcircle* pCircle = static_cast<const CNCcircle *>(pNext);
		CPointD	pto( GetPlaneValueOrg(pCircle->GetOrg(), m_ptValE) );
		// ｵﾌｾｯﾄ分平行移動させた交点を求める
		double	rr, xa, ya, rn = fabs(pCircle->GetR());
		tie(bResult, pt, r) = ::CalcOffsetIntersectionPoint_LC(pts, pto, rn, r,
						pCircle->CalcOffsetSign(), 0, 0);
		if ( bResult ) {
			// 面取りに相当するC値の計算
			rr1 = sqrt(pt.x*pt.x + pt.y*pt.y - r*r);
			rr = rn + r;
			if ( r > 0 ) {
				// 内分点
				xa = (pt.x*rn+pto.x*r) / rr;
				ya = (pt.y*rn+pto.y*r) / rr;
			}
			else {
				// 外分点
				r = fabs(r);
				xa = (pt.x*rn-pto.x*r) / rr;
				ya = (pt.y*rn-pto.y*r) / rr;
			}
			rr2 = _hypot(xa, ya);
		}
	}
	else {
		CPointD	pte( GetPlaneValueOrg(pNext->GetEndPoint(), m_ptValE) );
		// ｵﾌｾｯﾄ分平行移動させた交点を求める
		optional<CPointD> ptResult = ::CalcOffsetIntersectionPoint_LL(pts, pte, 0, 0, r);
		if ( ptResult ) {
			bResult = TRUE;
			pt = *ptResult;
			// 面取りに相当するC値の計算
			rr1 = rr2 = sqrt(pt.x*pt.x + pt.y*pt.y - r*r);
		}
		else
			bResult = FALSE;
	}

	// 原点補正
	if ( bResult )
		pt += GetPlaneValue(m_ptValE);

	return make_tuple(bResult, pt, rr1, rr2);
}

optional<CPointD> CNCline::SetChamferingPoint(BOOL bStart, double c)
{
	// 長さが足らないときはｴﾗｰ
	if ( c >= SetCalcLength() )
		return optional<CPointD>();

	CPointD		pt, pto, pte;
	CPoint3D&	ptValS = bStart ? m_ptValS : m_ptValE;	// 参照
	CPoint3D&	ptValE = bStart ? m_ptValE : m_ptValS;
	
	pto = GetPlaneValue(ptValS);
	pte = GetPlaneValue(ptValE);

	// pto を中心とした円と pte の交点
	pt = ::CalcIntersectionPoint_TC(pto, c, pte);

	// 自分自身の点も更新
	SetPlaneValue(pt, ptValS);

	if ( bStart )
		m_pt2Ds = m_ptValS.PointConvert();
	else {
		m_pRead->m_ptValOrg = m_ptValE;
		if ( m_pRead->m_g68.bG68 ) {
			// m_ptValE はG68回転済み座標のため回転を元に戻してｵﾌｾｯﾄ減算
			m_pRead->m_g68.dRound = -m_pRead->m_g68.dRound;
			CalcG68Round(&(m_pRead->m_g68), m_pRead->m_ptValOrg);
			m_pRead->m_g68.dRound = -m_pRead->m_g68.dRound;
		}
		m_pRead->m_ptValOrg -= m_pRead->m_ptOffset;	// m_ptValEがG68の場合、おかしくなる
		m_pt2D = m_ptValE.PointConvert();
	}

	return pt;
}

double CNCline::CalcBetweenAngle(const CNCdata* pNext) const
{
	// ２線の交点(自身の終点)が原点になるように補正
	CPointD		pt1( GetPlaneValueOrg(m_ptValS, m_ptValE) ), pt2;

	if ( pNext->GetType() == NCDARCDATA ) {
		const CNCcircle* pCircle = static_cast<const CNCcircle *>(pNext);
		// 次のｵﾌﾞｼﾞｪｸﾄが円弧なら中心をｾｯﾄし
		CPointD	pt( GetPlaneValueOrg(pCircle->GetOrg(), m_ptValE) );
		// 始点の接線計算
		int k = pCircle->CalcOffsetSign();
		pt2.x = -pt.y*k;	// G02:+90°
		pt2.y =  pt.x*k;	// G03:-90°
	}
	else {
		pt2 = GetPlaneValueOrg(pNext->GetEndPoint(), m_ptValE);
	}
	
	// ２線(または円弧の接線)がなす角度を求める
	return ::CalcBetweenAngle(pt1, pt2);
}

int CNCline::CalcOffsetSign(void) const
{
	// 始点を原点に進行方向の角度を計算
	return ::CalcOffsetSign( GetPlaneValueOrg(m_ptValE, m_ptValS) );
}

optional<CPointD> CNCline::CalcPerpendicularPoint
	(ENPOINTORDER enPoint, double r, int nSign) const
{
	const CPoint3D&	pts = enPoint==STARTPOINT ? m_ptValS : m_ptValE;
	const CPoint3D&	pte = enPoint==STARTPOINT ? m_ptValE : m_ptValS;
	if ( enPoint == ENDPOINT )
		nSign = -nSign;		// 終点では-90°
	// 線の傾きを計算して90°回転
	CPointD	pt( GetPlaneValueOrg(pte, pts) );
	double	q = atan2(pt.y, pt.x);
	CPointD	pt1(r*cos(q), r*sin(q));
	CPointD	pt2(-pt1.y*nSign, pt1.x*nSign);
	pt2 += GetPlaneValue(pts);

	return pt2;
}

optional<CPointD> CNCline::CalcOffsetIntersectionPoint
	(const CNCdata* pNext, double r, int k1, int k2) const
{
	BOOL	bResult;
	// ２線の交点(自身の終点)が原点になるように補正
	CPointD		pt, pt1( GetPlaneValueOrg(m_ptValS, m_ptValE) ), pt2;

	// ｵﾌｾｯﾄ分平行移動させた交点を求める
	if ( pNext->GetType() == NCDARCDATA ) {
		const CNCcircle* pCircle = static_cast<const CNCcircle *>(pNext);
		pt2 = GetPlaneValueOrg(pCircle->GetOrg(), m_ptValE);
		tie(bResult, pt, r) = ::CalcOffsetIntersectionPoint_LC(pt1, pt2, fabs(pCircle->GetR()), r,
						pCircle->CalcOffsetSign(), k1, k2);
	}
	else {
		pt2 = GetPlaneValueOrg(pNext->GetEndPoint(), m_ptValE);
		optional<CPointD> ptResult = ::CalcOffsetIntersectionPoint_LL(pt1, pt2, k1, k2, r);
		if ( ptResult ) {
			bResult = TRUE;
			pt = *ptResult;
		}
		else
			bResult = FALSE;
	}

	// 原点補正
	if ( bResult ) {
		pt += GetPlaneValue(m_ptValE);
		return pt;
	}

	return optional<CPointD>();
}

optional<CPointD> CNCline::CalcOffsetIntersectionPoint2
	(const CNCdata* pNext, double r, int k1, int k2) const
{
	// ２線の交点(自身の終点)が原点になるように補正
	CPointD		pt, pt1( GetPlaneValueOrg(m_ptValS, m_ptValE) ), pt2;

	// ｵﾌｾｯﾄ分平行移動させた交点を求める
	if ( pNext->GetType() == NCDARCDATA ) {
		// 円弧は始点接線とのｵﾌｾｯﾄ交点計算
		const CNCcircle* pCircle = static_cast<const CNCcircle *>(pNext);
		pt = GetPlaneValueOrg(pCircle->GetOrg(), m_ptValE);
		int k = pCircle->CalcOffsetSign();
		pt2.x = -pt.y * k;
		pt2.y =  pt.x * k;
		// 接線用に符号を再計算
		k2 *= k * ::CalcOffsetSign(pt2);
	}
	else {
		pt2 = GetPlaneValueOrg(pNext->GetEndPoint(), m_ptValE);
	}

	// 直線同士のｵﾌｾｯﾄ交点計算
	optional<CPointD> ptResult = ::CalcOffsetIntersectionPoint_LL(pt1, pt2, k1, k2, r);
	// 原点補正
	if ( ptResult ) {
		pt = *ptResult + GetPlaneValue(m_ptValE);
		return pt;
	}
	return ptResult;
}

void CNCline::SetCorrectPoint(ENPOINTORDER enPoint, const CPointD& pt, double)
{
	CPoint3D&	ptVal    = enPoint==STARTPOINT ? m_ptValS : m_ptValE;
	CPointD&	ptResult = enPoint==STARTPOINT ? m_pt2Ds : m_pt2D;

	SetPlaneValue(pt, ptVal);
	ptResult = ptVal.PointConvert();
}

//////////////////////////////////////////////////////////////////////
// CNCcycle クラス
//////////////////////////////////////////////////////////////////////
CNCcycle::CNCcycle
	(const CNCdata* pData, LPNCARGV lpArgv, const CPoint3D& ptOffset, BOOL bL0Cycle) :
		CNCline(NCDCYCLEDATA, pData, lpArgv, ptOffset)
{
/*
	Z, R, P 値は，TH_NCRead.cpp でも補間していることに注意
*/
#ifdef _DEBUG
	CMagaDbg	dbg("CNCcycle", DBG_MAGENTA);
#endif
	double	dx, dy,	dox, doy,	// 基準平面の移動距離
			dR, dI,				// R点座標, ｲﾆｼｬﾙ座標
			dRLength, dZLength;		// 移動長，切削長
	CPoint3D	pt;
	int		i, x, y, z,
			nH, nV;		// 縦横の繰り返し数

	// 初期化
	for ( i=0; i<SIZEOF(m_Cycle); m_Cycle[i++]=NULL );
	m_Cycle3D = NULL;

	// 基準平面による座標設定
	switch ( GetPlane() ) {
	case XY_PLANE:
		x = NCA_X;
		y = NCA_Y;
		z = NCA_Z;
		break;
	case XZ_PLANE:
		x = NCA_X;
		y = NCA_Z;
		z = NCA_Y;
		break;
	case YZ_PLANE:
		x = NCA_Y;
		y = NCA_Z;
		z = NCA_X;
		break;
	}

	// 描画始点を前回の計算値から取得
	m_pt2Ds  = pData->Get2DPoint();
	// 以降ｵﾌｾｯﾄ無しで計算！
	m_ptValS = pData->GetEndPoint() - pData->GetOffsetPoint();
	// 縦繰り返し数取得
	if ( GetValFlags() & NCD_K )
		nV = max(0, (int)GetValue(NCA_K));
	else if ( GetValFlags() & NCD_L )
		nV = max(0, (int)GetValue(NCA_L));
	else
		nV = 1;
	// 復帰座標(前回のｵﾌﾞｼﾞｪｸﾄが固定ｻｲｸﾙかどうか)
	m_dInitial = pData->GetType()!=NCDCYCLEDATA ? m_ptValS[z] :
				(static_cast<const CNCcycle*>(pData)->GetInitialValue() - pData->GetOffsetPoint()[z]);
	// ｲﾝｸﾘﾒﾝﾀﾙ補正(R座標はﾍﾞｰｽｸﾗｽで座標補正の対象外)
	if ( lpArgv->bAbs ) {
		dR = GetValFlags() & NCD_R ? GetValue(NCA_R) : m_ptValS[z];
		m_nDrawCnt = nH = min(1, nV);	// ｱﾌﾞｿﾘｭｰﾄなら横へは(0 or 1)回のみ
	}
	else {
		dR = GetValFlags() & NCD_R ? m_dInitial + GetValue(NCA_R) : m_dInitial;
		// !!! Z値もR点からのｲﾝｸﾘﾒﾝﾄに補正 !!!
		if ( GetValFlags() & g_dwSetValFlags[z] )
			m_nc.dValue[z] = dR + lpArgv->nc.dValue[z];
		m_nDrawCnt = nH = nV;	// ｲﾝｸﾘﾒﾝﾀﾙなら横へも繰り返し
	}
	dI = lpArgv->bInitial ? m_dInitial : dR;

	// 繰り返し数ｾﾞﾛなら
	if ( m_nDrawCnt <= 0 ) {
		if ( bL0Cycle ) {
			// 移動ﾃﾞｰﾀだけ作成
			nH = 1;
			dI = dR = m_ptValS[z];
		}
		else {
			// 以降の計算は不要
			for ( i=0; i<NCXYZ; m_dMove[i++]=0.0 );
			m_dDwell = 0.0;
			m_nc.dLength = m_dCycleMove = 0.0;
			m_ptValI = m_ptValR = m_ptValE = m_ptValS = pData->GetEndPoint();
			m_pRead->m_ptValOrg = pData->GetOriginalEndPoint();
			m_dInitial += ptOffset[z];
			m_pt2D = m_pt2Ds;
			return;
		}
	}

	m_ptValE.SetPoint(GetValue(NCA_X), GetValue(NCA_Y), GetValue(NCA_Z));
	m_pRead->m_ptValOrg = m_ptValE;
	pt = pData->GetOriginalEndPoint();
	// 座標回転
	if ( lpArgv->g68.bG68 )
		CalcG68Round(&(lpArgv->g68), m_ptValE);
	// 各軸ごとの移動長計算
	dx = m_ptValE[x] - m_ptValS[x];		dox = m_pRead->m_ptValOrg[x] - pt[x];
	dy = m_ptValE[y] - m_ptValS[y];		doy = m_pRead->m_ptValOrg[y] - pt[y];
	dRLength = fabs(dI - dR);
	dZLength = fabs(dR - m_ptValE[z]);
	m_dMove[x] = fabs(dx) * nH;
	m_dMove[y] = fabs(dy) * nH;
	m_dMove[z] = fabs(m_ptValS[z] - dR);	// 初回下降分
	m_dMove[z] += dRLength * (nV-1);
	// 移動長計算
	m_dCycleMove = _hypot(m_dMove[x], m_dMove[y]);
	m_dCycleMove += m_dMove[z];
	// 切削長
	m_nc.dLength = dZLength * nV;
	// 各座標値の設定
	switch ( GetPlane() ) {
	case XY_PLANE:
		m_ptValI.SetPoint(m_ptValE.x, m_ptValE.y, m_ptValS.z);	// 描画用
		m_ptValR.SetPoint(m_ptValE.x, m_ptValE.y, dR);
		m_ptValE.SetPoint(m_ptValS.x+dx*nH, m_ptValS.y+dy*nH, dI);
		m_pRead->m_ptValOrg.SetPoint(pt.x+dox*nH, pt.y+doy*nH, dI);
		break;
	case XZ_PLANE:
		m_ptValI.SetPoint(m_ptValE.x, m_ptValS.y, m_ptValE.z);
		m_ptValR.SetPoint(m_ptValE.x, dR, m_ptValE.z);
		m_ptValE.SetPoint(m_ptValS.x+dx*nH, dI, m_ptValS.z+dy*nH);
		m_pRead->m_ptValOrg.SetPoint(pt.x+dox*nH, dI, pt.z+doy*nH);
		break;
	case YZ_PLANE:
		m_ptValI.SetPoint(m_ptValS.x, m_ptValE.y, m_ptValE.z);
		m_ptValR.SetPoint(dR, m_ptValE.y, m_ptValE.z);
		m_ptValE.SetPoint(dI, m_ptValS.y+dx*nH, m_ptValS.z+dy*nH);
		m_pRead->m_ptValOrg.SetPoint(dI, pt.y+dox*nH, pt.z+doy*nH);
		break;
	}
	m_ptValS += ptOffset;
	m_ptValE += ptOffset;
	m_ptValI += ptOffset;
	m_ptValR += ptOffset;
	m_dInitial += ptOffset[z];
	m_pt2D = m_ptValE.PointConvert();

	// 切り込み値が指定されていなければｴﾗｰ(補間はTH_NCRead.cppにて)
	if ( !bL0Cycle && !(GetValFlags() & g_dwSetValFlags[z]) ) {
		m_nc.nErrorCode = IDS_ERR_NCBLK_NOTCYCLEZ;
		m_nDrawCnt = 0;
		m_dMove[z] = 0.0;
		m_dDwell = 0.0;
		return;
	}

#ifdef _DEBUG
	dbg.printf("StartPoint x=%.3f y=%.3f z=%.3f",
		m_ptValS.x, m_ptValS.y, m_ptValS.z);
	dbg.printf("           R-Point=%.3f C-Point=%.3f", dR, GetValue(z));
	dbg.printf("FinalPoint x=%.3f y=%.3f z=%.3f",
		m_ptValE[NCA_X], m_ptValE[NCA_Y], m_ptValE[NCA_Z]);
	dbg.printf("m_nDrawCnt=%d", m_nDrawCnt);
#endif

	if ( m_nDrawCnt <= 0 ) {
		SetMaxRect();
		return;		// bL0Cycle でもここまで
	}

	// 座標格納領域確保
	for ( i=0; i<SIZEOF(m_Cycle); i++ )
		m_Cycle[i] = new PTCYCLE[m_nDrawCnt];
	m_Cycle3D = new PTCYCLE3D[m_nDrawCnt];

	pt = m_ptValS;
	for ( i=0; i<m_nDrawCnt; i++ ) {
		pt[x] += dx;	pt[y] += dy;
#ifdef _DEBUG
		dbg.printf("           No.%d [x]=%.3f [y]=%.3f", i+1, pt[x], pt[y]);
#endif
		// 各平面ごとに座標設定
		pt[z] = dI;
		m_Cycle3D[i].ptI  = pt;
		m_Cycle[0][i].ptI = pt.PointConvert();
		m_Cycle[1][i].ptI = pt.GetXY();
		m_Cycle[2][i].ptI = pt.GetXZ();
		m_Cycle[3][i].ptI = pt.GetYZ();
		pt[z] = dR;
		m_Cycle3D[i].ptR  = pt;
		m_Cycle[0][i].ptR = pt.PointConvert();
		m_Cycle[1][i].ptR = pt.GetXY();
		m_Cycle[2][i].ptR = pt.GetXZ();
		m_Cycle[3][i].ptR = pt.GetYZ();
		pt[z] = GetValue(z);
		m_Cycle3D[i].ptC  = pt;
		m_Cycle[0][i].ptC = pt.PointConvert();
		m_Cycle[1][i].ptC = pt.GetXY();
		m_Cycle[2][i].ptC = pt.GetXZ();
		m_Cycle[3][i].ptC = pt.GetYZ();
	}
	
	// 上昇分の移動・切削長計算
	double	dResult;
	switch ( GetGcode() ) {
	case 84:	// R点まで切削復帰，ｲﾆｼｬﾙ点まで早送り復帰
	case 85:
	case 87:
	case 88:
	case 89:
		if ( lpArgv->bInitial ) {
			dResult = dRLength * nV;
			m_nc.dLength += dZLength * nV;
			m_dMove[z] += dResult;
			m_dCycleMove += dResult;
		}
		else
			m_nc.dLength += dZLength * nV;
		break;
	default:	// それ以外は早送り復帰
		dResult = (dRLength + dZLength) * nV;
		m_dMove[z] += dResult;
		m_dCycleMove += dResult;
		break;
	}

	// ﾄﾞｳｪﾙ時間
	if ( GetValFlags() & NCD_P &&
		(GetGcode()==82 || GetGcode()==88 || GetGcode()==89) )
		m_dDwell = GetValue(NCA_P) * nV;
	else
		m_dDwell = 0.0;

#ifdef _DEBUG
	DbgDump();
	Dbg_sep();
#endif

	// 空間占有矩形
	SetMaxRect();
}

CNCcycle::~CNCcycle()
{
	if ( m_Cycle[0] ) {
		for ( int i=0; i<SIZEOF(m_Cycle); i++ )
			delete[] m_Cycle[i];
	}
	if ( m_Cycle3D )
		delete m_Cycle3D;
}

void CNCcycle::DrawTuning(double f)
{
	CNCline::DrawTuning(f);
	m_ptDrawI[0] = m_ptValI.PointConvert() * f;
	m_ptDrawR[0] = m_ptValR.PointConvert() * f;
	if ( m_nDrawCnt > 0 ) {
		for ( int i=0; i<m_nDrawCnt; i++ )
			m_Cycle[0][i].DrawTuning(f);
	}
}

void CNCcycle::DrawTuningXY(double f)
{
	CNCline::DrawTuningXY(f);
	m_ptDrawI[1] = m_ptValI.GetXY() * f;
	m_ptDrawR[1] = m_ptValR.GetXY() * f;
	if ( m_nDrawCnt > 0 ) {
		for ( int i=0; i<m_nDrawCnt; i++ )
			m_Cycle[1][i].DrawTuning(f);
	}
}

void CNCcycle::DrawTuningXZ(double f)
{
	CNCline::DrawTuningXZ(f);
	m_ptDrawI[2] = m_ptValI.GetXZ() * f;
	m_ptDrawR[2] = m_ptValR.GetXZ() * f;
	if ( m_nDrawCnt > 0 ) {
		for ( int i=0; i<m_nDrawCnt; i++ )
			m_Cycle[2][i].DrawTuning(f);
	}
}

void CNCcycle::DrawTuningYZ(double f)
{
	CNCline::DrawTuningYZ(f);
	m_ptDrawI[3] = m_ptValI.GetYZ() * f;
	m_ptDrawR[3] = m_ptValR.GetYZ() * f;
	if ( m_nDrawCnt > 0 ) {
		for ( int i=0; i<m_nDrawCnt; i++ )
			m_Cycle[3][i].DrawTuning(f);
	}
}

void CNCcycle::Draw(CDC* pDC, BOOL bSelect, BOOL) const
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCcycle::Draw()", DBG_RED);
	dbg.printf("Line=%d", GetBlockLineNo());
#endif
	DrawCycle(pDC, 0, bSelect);
}

void CNCcycle::DrawXY(CDC* pDC, BOOL bSelect, BOOL) const
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCcycle::DrawXY()", DBG_RED);
	dbg.printf("Line=%d", GetBlockLineNo());
#endif
	if ( GetPlane() == XY_PLANE ) {
		CNCline::DrawXY(pDC, bSelect);
		DrawCyclePlane(pDC, 1, bSelect);
	}
	else
		DrawCycle(pDC, 1, bSelect);
}

void CNCcycle::DrawXZ(CDC* pDC, BOOL bSelect, BOOL) const
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCcycle::DrawXZ()", DBG_RED);
	dbg.printf("Line=%d", GetBlockLineNo());
#endif
	if ( GetPlane() == XZ_PLANE ) {
		CNCline::DrawXZ(pDC, bSelect);
		DrawCyclePlane(pDC, 2, bSelect);
	}
	else
		DrawCycle(pDC, 2, bSelect);
}

void CNCcycle::DrawYZ(CDC* pDC, BOOL bSelect, BOOL) const
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCcycle::DrawYZ()", DBG_RED);
	dbg.printf("Line=%d", GetBlockLineNo());
#endif
	if ( GetPlane() == YZ_PLANE ) {
		CNCline::DrawYZ(pDC, bSelect);
		DrawCyclePlane(pDC, 3, bSelect);
	}
	else
		DrawCycle(pDC, 3, bSelect);
}

void CNCcycle::DrawCyclePlane(CDC* pDC, size_t n, BOOL bSelect) const
{
	CPen*	pOldPen = bSelect ?
		AfxGetNCVCMainWnd()->GetPenCom(COMPEN_SEL) :
		AfxGetNCVCMainWnd()->GetPenNC(NCPEN_CYCLE);
	pOldPen = pDC->SelectObject(pOldPen);
	CBrush*	pOldBrush = pDC->SelectObject(AfxGetNCVCMainWnd()->GetBrushNC(NCBRUSH_CYCLEXY));
	for ( int i=0; i<m_nDrawCnt; i++ )
		pDC->Ellipse(&m_Cycle[n][i].rcDraw);
	pDC->SelectObject(pOldBrush);
	pDC->SelectObject(pOldPen);
}

void CNCcycle::DrawCycle(CDC* pDC, size_t n, BOOL bSelect) const
{
	CPen*	pPenM = AfxGetNCVCMainWnd()->GetPenNC(NCPEN_G0);
	CPen*	pPenC = AfxGetNCVCMainWnd()->GetPenNC(NCPEN_CYCLE);
	CPen*	pOldPen;
	if ( bSelect )
		pPenM = pPenC = pOldPen = AfxGetNCVCMainWnd()->GetPenCom(COMPEN_SEL);
	else
		pOldPen = pPenM;
	pOldPen = pDC->SelectObject(pOldPen);
	// 前回位置から１点目の穴加工までの移動
	pDC->MoveTo(m_ptDrawS[n]);
	pDC->LineTo(m_ptDrawI[n]);		// 現在位置から１点目のｲﾆｼｬﾙ点
	pDC->LineTo(m_ptDrawR[n]);		// ｲﾆｼｬﾙ点からR点
	pDC->LineTo(m_ptDrawE[n]);		// R点から最後の穴加工
	// 切削方向の描画
	for ( int i=0; i<m_nDrawCnt; i++ ) {
		pDC->SelectObject(pPenM);
		pDC->MoveTo(m_Cycle[n][i].ptDrawI);
		pDC->LineTo(m_Cycle[n][i].ptDrawR);
		pDC->SelectObject(pPenC);
		pDC->LineTo(m_Cycle[n][i].ptDrawC);
	}
	pDC->SelectObject(pOldPen);
}

void CNCcycle::DrawGL(BOOL) const
{
	const CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();
	COLORREF	colG0 = pOpt->GetNcDrawColor( NCCOL_G0 ),
				colCY = pOpt->GetNcDrawColor( NCCOL_CYCLE );
	BYTE	bG0r = GetRValue(colG0), bG0g = GetGValue(colG0), bG0b = GetBValue(colG0),
			bCYr = GetRValue(colCY), bCYg = GetGValue(colCY), bCYb = GetBValue(colCY);
	::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(NCCOLLINE_G0)].nGLpattern);
	::glBegin(GL_LINE_STRIP);
	::glColor3ub( bG0r, bG0g, bG0b );
	::glVertex3d(m_ptValS.x, m_ptValS.y, m_ptValS.z);
	::glVertex3d(m_ptValI.x, m_ptValI.y, m_ptValI.z);
	::glVertex3d(m_ptValR.x, m_ptValR.y, m_ptValR.z);
	::glVertex3d(m_ptValE.x, m_ptValE.y, m_ptValE.z);
	::glEnd();
	for ( int i=0; i<m_nDrawCnt; i++ ) {
		::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(NCCOLLINE_G0)].nGLpattern);
		::glBegin(GL_LINES);
		::glColor3ub( bG0r, bG0g, bG0b );
		::glVertex3d(m_Cycle3D[i].ptI.x, m_Cycle3D[i].ptI.y, m_Cycle3D[i].ptI.z);
		::glVertex3d(m_Cycle3D[i].ptR.x, m_Cycle3D[i].ptR.y, m_Cycle3D[i].ptR.z);
		::glEnd();
		::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(NCCOLLINE_CYCLE)].nGLpattern);
		::glBegin(GL_LINES);
		::glColor3ub( bCYr, bCYg, bCYb );
		::glVertex3d(m_Cycle3D[i].ptR.x, m_Cycle3D[i].ptR.y, m_Cycle3D[i].ptR.z);
		::glVertex3d(m_Cycle3D[i].ptC.x, m_Cycle3D[i].ptC.y, m_Cycle3D[i].ptC.z);
		::glEnd();
	}
}

//////////////////////////////////////////////////////////////////////
// CNCcircle クラス
//////////////////////////////////////////////////////////////////////
CNCcircle::CNCcircle(const CNCdata* pData, LPNCARGV lpArgv, const CPoint3D& ptOffset) :
	CNCdata(NCDARCDATA, pData, lpArgv, ptOffset)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CNCcircle", DBG_MAGENTA);
#endif
	BOOL		fError = TRUE;	// Error

	m_nG23 = GetGcode() - 2;	// G2=0, G3=1
/*
	XZ平面は(Z->X, X->Y)なのでそのまま計算すると -90°回転させる必要がある
	簡単に対応するには符号を反対にすればよい -> もうちょっとマシな対応を！
*/
	if ( GetPlane() == XZ_PLANE )
		m_nG23 = 1 - m_nG23;	// 0->1 , 1->0;

	// ﾈｲﾃｨﾌﾞの座標ﾃﾞｰﾀで中心を計算してから座標回転
	m_ptValS = pData->GetOriginalEndPoint();	// あとで設定し直し
	m_ptValE.SetPoint(GetValue(NCA_X), GetValue(NCA_Y), GetValue(NCA_Z));
	m_pRead->m_ptValOrg = m_ptValE;

	// 平面座標取得
	CPointD	pts( GetPlaneValue(m_ptValS) ),
			pte( GetPlaneValue(m_ptValE) ),
			pto;

	// 半径と中心座標の計算(R優先)
	if ( GetValFlags() & NCD_R ) {
		m_r = GetValue(NCA_R);
		fError = CalcCenter(pts, pte);
	}
	else if ( GetValFlags() & (NCD_I|NCD_J|NCD_K) ) {
		m_ptOrg = m_ptValS;
		switch ( GetPlane() ) {
		case XY_PLANE:
			m_r = _hypot(GetValue(NCA_I), GetValue(NCA_J));
			m_ptOrg.x += GetValue(NCA_I);
			m_ptOrg.y += GetValue(NCA_J);
			pto = m_ptOrg.GetXY();
			break;
		case XZ_PLANE:
			m_r = _hypot(GetValue(NCA_I), GetValue(NCA_K));
			m_ptOrg.x += GetValue(NCA_I);
			m_ptOrg.z += GetValue(NCA_K);
			pto = m_ptOrg.GetXZ();
			break;
		case YZ_PLANE:
			m_r = _hypot(GetValue(NCA_J), GetValue(NCA_K));
			m_ptOrg.y += GetValue(NCA_J);
			m_ptOrg.z += GetValue(NCA_K);
			pto = m_ptOrg.GetYZ();
			break;
		}
		pts -= pto;		// 角度調整用の原点補正
		pte -= pto;
		AngleTuning(pts, pte);
		fError = FALSE;
	}

	m_ptValS = pData->GetEndPoint();	// 前回の計算値を補間
	m_ptValE += ptOffset;
	m_ptOrg  += ptOffset;
	// 座標回転
	if ( lpArgv->g68.bG68 ) {
		CalcG68Round(&(lpArgv->g68), m_ptValE);
		CalcG68Round(&(lpArgv->g68), m_ptOrg);
		// 角度の再調整( [m_sq|m_eq] += lpArgv->g68.dRound; でもOK )
		AngleTuning(GetPlaneValueOrg(m_ptValS, m_ptOrg), GetPlaneValueOrg(m_ptValE, m_ptOrg));
	}

	// 描画終点を計算し保存
	m_pt2D = m_ptValE.PointConvert();

	// 描画関数の決定とﾍﾘｶﾙ移動量の計算
	Constracter();

#ifdef _DEBUG
	dbg.printf("gcode=%d", m_nG23);
	dbg.printf("sx=%.3f sy=%.3f sz=%.3f ex=%.3f ey=%.3f ez=%.3f r=%.3f",
		m_ptValS.x, m_ptValS.y, m_ptValS.z,
		m_ptValE.x, m_ptValE.y, m_ptValE.z, m_r);
	dbg.printf("px=%.3f py=%.3f pz=%.3f sq=%f eq=%f",
		m_ptOrg.x, m_ptOrg.y, m_ptOrg.z, m_sq*DEG, m_eq*DEG);
#endif
	// 空間占有矩形
	SetMaxRect();

	if ( fError )
		m_nc.nErrorCode = IDS_ERR_NCBLK_CIRCLECENTER;
#ifdef _DEBUG
	DbgDump();
	Dbg_sep();
#endif
}

CNCcircle::CNCcircle(const CNCdata* pData) : CNCdata(pData)
{
	m_pt2D	= m_ptValE.PointConvert();
	m_nG23	= static_cast<const CNCcircle *>(pData)->GetG23();
	m_ptOrg	= static_cast<const CNCcircle *>(pData)->GetOrg();
	m_r		= static_cast<const CNCcircle *>(pData)->GetR();
	m_sq	= static_cast<const CNCcircle *>(pData)->GetStartAngle();
	m_eq	= static_cast<const CNCcircle *>(pData)->GetEndAngle();
	Constracter();
}

void CNCcircle::Constracter(void)
{
	// 描画関数の決定とﾍﾘｶﾙ移動量の計算
	switch ( GetPlane() ) {
	case XY_PLANE:
		m_pfnCircleDraw = &CNCcircle::Draw_G17;
		m_dHelicalStep = GetValFlags() & NCD_Z ?
			(m_ptValE.z - m_ptValS.z) / ((m_eq-m_sq)/ARCSTEP) : 0.0;
		break;
	case XZ_PLANE:
		m_pfnCircleDraw = &CNCcircle::Draw_G18;
		m_dHelicalStep = GetValFlags() & NCD_Y ?
			(m_ptValE.y - m_ptValS.y) / ((m_eq-m_sq)/ARCSTEP) : 0.0;
		break;
	case YZ_PLANE:
		m_pfnCircleDraw = &CNCcircle::Draw_G19;
		m_dHelicalStep = GetValFlags() & NCD_X ?
			(m_ptValE.x - m_ptValS.x) / ((m_eq-m_sq)/ARCSTEP) : 0.0;
		break;
	}

	if ( m_nG23 == 0 )
		m_dHelicalStep = -m_dHelicalStep;		// 符号反転
}

BOOL CNCcircle::CalcCenter(const CPointD& pts, const CPointD& pte)
{
#ifdef _DEBUG
	CMagaDbg	dbg("Center()", DBG_RED);
#endif
	// R 指定で始点終点が同じ場合はエラー
	if ( pts == pte )
		return TRUE;	// ｴﾗｰは真で返す

	// 円の方程式より，(s.x, s.y)，(e.x, e.y) を中心とした
	// ２つの円の交点を求める
	CPointD	pt1, pt2;
	int		nResult;
	tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_CC(pts, pte, m_r, m_r);
	if ( nResult < 1 )
		return TRUE;

	// どちらの解を採用するか
	AngleTuning(pts-pt1, pte-pt1);	// まず一方の中心座標から角度を求める
	if ( nResult==1 ||
		(m_r>0.0 && m_eq-m_sq<180.0*RAD) ||
		(m_r<0.0 && m_eq-m_sq>180.0*RAD) ) {
		SetCenter(pt1);
	}
	else {
		// 条件がﾏｯﾁしなかったので他方の解を採用
		AngleTuning(pts-pt2, pte-pt2);
		SetCenter(pt2);
	}
	return FALSE;
}

void CNCcircle::SetCenter(const CPointD& pt)
{
	switch ( GetPlane() ) {
	case XY_PLANE:
		m_ptOrg.x = pt.x;
		m_ptOrg.y = pt.y;
		m_ptOrg.z = GetValue(NCA_Z);
		break;
	case XZ_PLANE:
		m_ptOrg.x = pt.x;
		m_ptOrg.y = GetValue(NCA_Y);
		m_ptOrg.z = pt.y;
		break;
	case YZ_PLANE:
		m_ptOrg.x = GetValue(NCA_X);
		m_ptOrg.y = pt.x;
		m_ptOrg.z = pt.y;
		break;
	}
}

void CNCcircle::AngleTuning(const CPointD& pts, const CPointD& pte)
{
	if ( (m_sq=atan2(pts.y, pts.x)) < 0.0 )
		m_sq += 360.0*RAD;
	if ( (m_eq=atan2(pte.y, pte.x)) < 0.0 )
		m_eq += 360.0*RAD;
	// 常に s<e (反時計回り) とする
	if ( m_nG23 == 0 )	// G02 なら開始角度と終了角度を入れ替え
		std::swap(m_sq, m_eq);
	while ( m_sq >= m_eq )
		m_eq += 360.0*RAD;
}

double CNCcircle::SetCalcLength(void)
{
	// 切削長のみ計算
	m_dMove[NCA_X] = m_dMove[NCA_Y] = m_dMove[NCA_Z] = 0.0;

	if ( m_obCdata.IsEmpty() )
		m_nc.dLength = fabs(m_r * (m_eq - m_sq));
	else {
		m_nc.dLength = 0.0;
		// 各補正要素の合計
		CNCdata*	pData;
		CNCcircle*	pCircle;
		CPoint3D	pt;
		for ( int i=0; i<m_obCdata.GetSize(); i++ ) {
			pData = m_obCdata[i];
			switch ( pData->GetType() ) {
			case NCDLINEDATA:
				pt = pData->GetEndPoint() - pData->GetStartPoint();
				m_nc.dLength += pt.hypot();
				break;
			case NCDARCDATA:
				pCircle = static_cast<CNCcircle *>(pData);
				m_nc.dLength += fabs(pCircle->GetR() * (pCircle->GetEndAngle() - pCircle->GetStartAngle()));
				break;
			}
		}
	}

	return m_nc.dLength;
}

void CNCcircle::DrawTuning(double f)
{
	m_dFactor = f;
	CNCdata::DrawTuning(f);
}

void CNCcircle::DrawTuningXY(double f)
{
	m_dFactorXY = f;
	CNCdata::DrawTuningXY(f);
}

void CNCcircle::DrawTuningXZ(double f)
{
	m_dFactorXZ = f;
	CNCdata::DrawTuningXZ(f);
}

void CNCcircle::DrawTuningYZ(double f)
{
	m_dFactorYZ = f;
	CNCdata::DrawTuningYZ(f);
}

void CNCcircle::Draw(CDC* pDC, BOOL bSelect, BOOL bCorrect) const
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCcircle::Draw()", DBG_RED);
	dbg.printf("Line=%d", GetBlockLineNo());
#endif
	// 平面ごとの描画関数
	CPen*	pOldPen;
	if ( bSelect )
		pOldPen = AfxGetNCVCMainWnd()->GetPenCom(COMPEN_SEL);
	else
		pOldPen = AfxGetNCVCMainWnd()->GetPenNC(bCorrect ? NCPEN_CORRECT : NCPEN_G1);
	pOldPen = pDC->SelectObject(pOldPen);
	(this->*m_pfnCircleDraw)(NCCIRCLEDRAW_XYZ, pDC);
	pDC->SelectObject(pOldPen);
	// 中心座標(黄色)
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	if ( pOpt->IsDrawCircleCenter() )
		pDC->SetPixelV(m_ptOrg.PointConvert()*m_dFactor,
			pOpt->GetNcDrawColor(NCCOL_CENTERCIRCLE));
	CNCdata::Draw(pDC, bSelect, TRUE);
}

void CNCcircle::DrawXY(CDC* pDC, BOOL bSelect, BOOL bCorrect) const
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCcircle::DrawXY()", DBG_RED);
	dbg.printf("Line=%d", GetBlockLineNo());
#endif
	CPen*	pOldPen;
	if ( bSelect )
		pOldPen = AfxGetNCVCMainWnd()->GetPenCom(COMPEN_SEL);
	else
		pOldPen = AfxGetNCVCMainWnd()->GetPenNC(bCorrect ? NCPEN_CORRECT : NCPEN_G1);
	pOldPen = pDC->SelectObject(pOldPen);
	(this->*m_pfnCircleDraw)(NCCIRCLEDRAW_XY, pDC);
	pDC->SelectObject(pOldPen);
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	if ( pOpt->IsDrawCircleCenter() )
		pDC->SetPixelV(m_ptOrg.GetXY()*m_dFactorXY,
			pOpt->GetNcDrawColor(NCCOL_CENTERCIRCLE));
	CNCdata::DrawXY(pDC, bSelect, TRUE);
}

void CNCcircle::DrawXZ(CDC* pDC, BOOL bSelect, BOOL bCorrect) const
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCcircle::DrawXZ()", DBG_RED);
	dbg.printf("Line=%d", GetBlockLineNo());
#endif
	CPen*	pOldPen;
	if ( bSelect )
		pOldPen = AfxGetNCVCMainWnd()->GetPenCom(COMPEN_SEL);
	else
		pOldPen = AfxGetNCVCMainWnd()->GetPenNC(bCorrect ? NCPEN_CORRECT : NCPEN_G1);
	pOldPen = pDC->SelectObject(pOldPen);
	(this->*m_pfnCircleDraw)(NCCIRCLEDRAW_XZ, pDC);
	pDC->SelectObject(pOldPen);
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	if ( pOpt->IsDrawCircleCenter() )
		pDC->SetPixelV(m_ptOrg.GetXZ()*m_dFactorXZ,
			pOpt->GetNcDrawColor(NCCOL_CENTERCIRCLE));
	CNCdata::DrawXZ(pDC, bSelect, TRUE);
}

void CNCcircle::DrawYZ(CDC* pDC, BOOL bSelect, BOOL bCorrect) const
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCcircle::DrawYZ()", DBG_RED);
	dbg.printf("Line=%d", GetBlockLineNo());
#endif
	CPen*	pOldPen;
	if ( bSelect )
		pOldPen = AfxGetNCVCMainWnd()->GetPenCom(COMPEN_SEL);
	else
		pOldPen = AfxGetNCVCMainWnd()->GetPenNC(bCorrect ? NCPEN_CORRECT : NCPEN_G1);
	pOldPen = pDC->SelectObject(pOldPen);
	(this->*m_pfnCircleDraw)(NCCIRCLEDRAW_YZ, pDC);
	pDC->SelectObject(pOldPen);
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	if ( pOpt->IsDrawCircleCenter() )
		pDC->SetPixelV(m_ptOrg.GetYZ()*m_dFactorYZ,
			pOpt->GetNcDrawColor(NCCOL_CENTERCIRCLE));
	CNCdata::DrawYZ(pDC, bSelect, TRUE);
}

// CDC::Arc() を使うとどうしても表示がズレる．
// 同一平面であっても微細線分による近似を行う
// 違う平面のときは，空間占有矩形 m_rcMax を利用して線分出力
void CNCcircle::Draw_G17(EN_NCCIRCLEDRAW enType, CDC* pDC) const	// XY_PLANE
{
	double		sq, eq, r = fabs(m_r), dFinalVal = m_ptValE.z;
	CPoint3D	pt3D, ptDrawOrg(m_ptOrg);
	CPointD		ptDraw, ptDraw2;

	if ( m_nG23 == 0 ) {
		sq = m_eq;
		eq = m_sq;
	}
	else {
		sq = m_sq;
		eq = m_eq;
	}

	switch ( enType ) {
	case NCCIRCLEDRAW_XYZ:
		r *= m_dFactor;
		ptDrawOrg *= m_dFactor;
		dFinalVal *= m_dFactor;
		pt3D.x = r * cos(sq) + ptDrawOrg.x;
		pt3D.y = r * sin(sq) + ptDrawOrg.y;
		pt3D.z = m_nG23==0 ? dFinalVal : ptDrawOrg.z;	// ﾍﾘｶﾙ開始座標
		ptDraw = pt3D.PointConvert();
		pDC->MoveTo(ptDraw);	// 開始点へ移動
		// ARCSTEP づつ微細線分で描画
		if ( m_nG23 == 0 ) {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
				pt3D.x = r * cos(sq) + ptDrawOrg.x;
				pt3D.y = r * sin(sq) + ptDrawOrg.y;
				pt3D.z += m_dHelicalStep;
				ptDraw = pt3D.PointConvert();
				pDC->LineTo(ptDraw);
			}
		}
		else {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				pt3D.x = r * cos(sq) + ptDrawOrg.x;
				pt3D.y = r * sin(sq) + ptDrawOrg.y;
				pt3D.z += m_dHelicalStep;
				ptDraw = pt3D.PointConvert();
				pDC->LineTo(ptDraw);
			}
		}
		// 端数分描画
		pt3D.x = r * cos(eq) + ptDrawOrg.x;
		pt3D.y = r * sin(eq) + ptDrawOrg.y;
		pt3D.z = m_nG23==0 ? ptDrawOrg.z : dFinalVal;	// ﾍﾘｶﾙ終了座標
		ptDraw = pt3D.PointConvert();
		pDC->LineTo(ptDraw);
		break;

	case NCCIRCLEDRAW_XY:	// Don't ARC()
		r *= m_dFactorXY;
		ptDrawOrg *= m_dFactorXY;
		ptDraw.x = r * cos(sq) + ptDrawOrg.x;
		ptDraw.y = r * sin(sq) + ptDrawOrg.y;
		pDC->MoveTo(ptDraw);
		if ( m_nG23 == 0 ) {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
				ptDraw.x = r * cos(sq) + ptDrawOrg.x;
				ptDraw.y = r * sin(sq) + ptDrawOrg.y;
				pDC->LineTo(ptDraw);
			}
		}
		else {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				ptDraw.x = r * cos(sq) + ptDrawOrg.x;
				ptDraw.y = r * sin(sq) + ptDrawOrg.y;
				pDC->LineTo(ptDraw);
			}
		}
		ptDraw.x = r * cos(eq) + ptDrawOrg.x;
		ptDraw.y = r * sin(eq) + ptDrawOrg.y;
		pDC->LineTo(ptDraw);
		break;

	case NCCIRCLEDRAW_XZ:
		ptDraw.x  = m_rcMax.left;	ptDraw.y  = m_rcMax.low;
		ptDraw2.x = m_rcMax.right;	ptDraw2.y = m_rcMax.high;
		ptDraw *= m_dFactorXZ;		ptDraw2 *= m_dFactorXZ;
		pDC->MoveTo(ptDraw);
		pDC->LineTo(ptDraw2);
		break;

	case NCCIRCLEDRAW_YZ:
		ptDraw.x  = m_rcMax.top;	ptDraw.y  = m_rcMax.low;
		ptDraw2.x = m_rcMax.bottom;	ptDraw2.y = m_rcMax.high;
		ptDraw *= m_dFactorYZ;		ptDraw2 *= m_dFactorYZ;
		pDC->MoveTo(ptDraw);
		pDC->LineTo(ptDraw2);
		break;
	}
}

void CNCcircle::Draw_G18(EN_NCCIRCLEDRAW enType, CDC* pDC) const	// XZ_PLANE
{
	double		sq, eq, r = fabs(m_r), dFinalVal = m_ptValE.y;
	CPoint3D	pt3D, ptDrawOrg(m_ptOrg);
	CPointD		ptDraw, ptDraw2;

	if ( m_nG23 == 0 ) {
		sq = m_eq;
		eq = m_sq;
	}
	else {
		sq = m_sq;
		eq = m_eq;
	}

	switch ( enType ) {
	case NCCIRCLEDRAW_XYZ:
		r *= m_dFactor;
		ptDrawOrg *= m_dFactor;
		dFinalVal *= m_dFactor;
		pt3D.x = r * cos(sq) + ptDrawOrg.x;
		pt3D.y = m_nG23==0 ? dFinalVal : ptDrawOrg.y;
		pt3D.z = r * sin(sq) + ptDrawOrg.z;
		ptDraw = pt3D.PointConvert();
		pDC->MoveTo(ptDraw);
		if ( m_nG23 == 0 ) {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
				pt3D.x = r * cos(sq) + ptDrawOrg.x;
				pt3D.y += m_dHelicalStep;
				pt3D.z = r * sin(sq) + ptDrawOrg.z;
				ptDraw = pt3D.PointConvert();
				pDC->LineTo(ptDraw);
			}
		}
		else {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				pt3D.x = r * cos(sq) + ptDrawOrg.x;
				pt3D.y += m_dHelicalStep;
				pt3D.z = r * sin(sq) + ptDrawOrg.z;
				ptDraw = pt3D.PointConvert();
				pDC->LineTo(ptDraw);
			}
		}
		pt3D.x = r * cos(eq) + ptDrawOrg.x;
		pt3D.y = m_nG23==0 ? ptDrawOrg.y : dFinalVal;
		pt3D.z = r * sin(eq) + ptDrawOrg.z;
		ptDraw = pt3D.PointConvert();
		pDC->LineTo(ptDraw);
		break;

	case NCCIRCLEDRAW_XY:
		ptDraw  = m_rcMax.TopLeft() * m_dFactorXY;
		ptDraw2 = m_rcMax.BottomRight() * m_dFactorXY;
		pDC->MoveTo(ptDraw);
		pDC->LineTo(ptDraw2);
		break;

	case NCCIRCLEDRAW_XZ:	// Don't ARC()
		r *= m_dFactorXZ;
		ptDrawOrg *= m_dFactorXZ;
		ptDraw.x = r * cos(sq) + ptDrawOrg.x;
		ptDraw.y = r * sin(sq) + ptDrawOrg.z;
		pDC->MoveTo(ptDraw);
		if ( m_nG23 == 0 ) {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
				ptDraw.x = r * cos(sq) + ptDrawOrg.x;
				ptDraw.y = r * sin(sq) + ptDrawOrg.z;
				pDC->LineTo(ptDraw);
			}
		}
		else {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				ptDraw.x = r * cos(sq) + ptDrawOrg.x;
				ptDraw.y = r * sin(sq) + ptDrawOrg.z;
				pDC->LineTo(ptDraw);
			}
		}
		ptDraw.x = r * cos(eq) + ptDrawOrg.x;
		ptDraw.y = r * sin(eq) + ptDrawOrg.z;
		pDC->LineTo(ptDraw);
		break;

	case NCCIRCLEDRAW_YZ:
		ptDraw.x  = m_rcMax.top;	ptDraw.y  = m_rcMax.low;
		ptDraw2.x = m_rcMax.bottom;	ptDraw2.y = m_rcMax.high;
		ptDraw *= m_dFactorYZ;		ptDraw2 *= m_dFactorYZ;
		pDC->MoveTo(ptDraw);
		pDC->LineTo(ptDraw2);
		break;
	}
}

void CNCcircle::Draw_G19(EN_NCCIRCLEDRAW enType, CDC* pDC) const	// YZ_PLANE
{
	double		sq, eq, r = fabs(m_r), dFinalVal = m_ptValE.x;
	CPoint3D	pt3D, ptDrawOrg(m_ptOrg);
	CPointD		ptDraw, ptDraw2;

	if ( m_nG23 == 0 ) {
		sq = m_eq;
		eq = m_sq;
	}
	else {
		sq = m_sq;
		eq = m_eq;
	}

	switch ( enType ) {
	case NCCIRCLEDRAW_XYZ:
		r *= m_dFactor;
		ptDrawOrg *= m_dFactor;
		dFinalVal *= m_dFactor;
		pt3D.x = m_nG23==0 ? dFinalVal : ptDrawOrg.x;
		pt3D.y = r * cos(sq) + ptDrawOrg.y;
		pt3D.z = r * sin(sq) + ptDrawOrg.z;
		ptDraw = pt3D.PointConvert();
		pDC->MoveTo(ptDraw);
		if ( m_nG23 == 0 ) {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
				pt3D.x += m_dHelicalStep;
				pt3D.y = r * cos(sq) + ptDrawOrg.y;
				pt3D.z = r * sin(sq) + ptDrawOrg.z;
				ptDraw = pt3D.PointConvert();
				pDC->LineTo(ptDraw);
			}
		}
		else {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				pt3D.x += m_dHelicalStep;
				pt3D.y = r * cos(sq) + ptDrawOrg.y;
				pt3D.z = r * sin(sq) + ptDrawOrg.z;
				ptDraw = pt3D.PointConvert();
				pDC->LineTo(ptDraw);
			}
		}
		pt3D.x = m_nG23==0 ? ptDrawOrg.x : dFinalVal;
		pt3D.y = r * cos(eq) + ptDrawOrg.y;
		pt3D.z = r * sin(eq) + ptDrawOrg.z;
		ptDraw = pt3D.PointConvert();
		pDC->LineTo(ptDraw);
		break;

	case NCCIRCLEDRAW_XY:
		ptDraw  = m_rcMax.TopLeft() * m_dFactorXY;
		ptDraw2 = m_rcMax.BottomRight() * m_dFactorXY;
		pDC->MoveTo(ptDraw);
		pDC->LineTo(ptDraw2);
		break;

	case NCCIRCLEDRAW_XZ:
		ptDraw.x  = m_rcMax.left;	ptDraw.y  = m_rcMax.low;
		ptDraw2.x = m_rcMax.right;	ptDraw2.y = m_rcMax.high;
		ptDraw *= m_dFactorXZ;		ptDraw2 *= m_dFactorXZ;
		pDC->MoveTo(ptDraw);
		pDC->LineTo(ptDraw2);
		break;

	case NCCIRCLEDRAW_YZ:	// Don't ARC()
		r *= m_dFactorYZ;
		ptDrawOrg *= m_dFactorYZ;
		ptDraw.x = r * cos(sq) + ptDrawOrg.y;
		ptDraw.y = r * sin(sq) + ptDrawOrg.z;
		pDC->MoveTo(ptDraw);
		if ( m_nG23 == 0 ) {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
				ptDraw.x = r * cos(sq) + ptDrawOrg.y;
				ptDraw.y = r * sin(sq) + ptDrawOrg.z;
				pDC->LineTo(ptDraw);
			}
		}
		else {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				ptDraw.x = r * cos(sq) + ptDrawOrg.y;
				ptDraw.y = r * sin(sq) + ptDrawOrg.z;
				pDC->LineTo(ptDraw);
			}
		}
		ptDraw.x = r * cos(eq) + ptDrawOrg.y;
		ptDraw.y = r * sin(eq) + ptDrawOrg.z;
		pDC->LineTo(ptDraw);
		break;
	}
}

void CNCcircle::DrawGL(BOOL bCorrect) const
{
	double		sq, eq, r = fabs(m_r), dFinalVal;
	CPoint3D	pt;

	if ( m_nG23 == 0 ) {
		sq = m_eq;
		eq = m_sq;
	}
	else {
		sq = m_sq;
		eq = m_eq;
	}

	const CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();
	COLORREF	col = pOpt->GetNcDrawColor(bCorrect ? NCCOL_CORRECT : NCCOL_G1);
	::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(NCCOLLINE_G1)].nGLpattern);
	::glBegin(GL_LINE_STRIP);
	::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );

	switch ( GetPlane() ) {
	case XY_PLANE:
		dFinalVal = m_ptValE.z;
		pt.x = r * cos(sq) + m_ptOrg.x;
		pt.y = r * sin(sq) + m_ptOrg.y;
		pt.z = m_nG23==0 ? dFinalVal : m_ptOrg.z;	// ﾍﾘｶﾙ開始座標
		::glVertex3d(pt.x, pt.y, pt.z);	// 開始点
		// ARCSTEP づつ微細線分で描画
		if ( m_nG23 == 0 ) {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
				pt.x = r * cos(sq) + m_ptOrg.x;
				pt.y = r * sin(sq) + m_ptOrg.y;
				pt.z += m_dHelicalStep;
				::glVertex3d(pt.x, pt.y, pt.z);
			}
		}
		else {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				pt.x = r * cos(sq) + m_ptOrg.x;
				pt.y = r * sin(sq) + m_ptOrg.y;
				pt.z += m_dHelicalStep;
				::glVertex3d(pt.x, pt.y, pt.z);
			}
		}
		// 端数分描画
		pt.x = r * cos(eq) + m_ptOrg.x;
		pt.y = r * sin(eq) + m_ptOrg.y;
		pt.z = m_nG23==0 ? m_ptOrg.z : dFinalVal;	// ﾍﾘｶﾙ終了座標
		::glVertex3d(pt.x, pt.y, pt.z);
		break;

	case XZ_PLANE:
		dFinalVal = m_ptValE.y;
		pt.x = r * cos(sq) + m_ptOrg.x;
		pt.y = m_nG23==0 ? dFinalVal : m_ptOrg.y;
		pt.z = r * sin(sq) + m_ptOrg.z;
		::glVertex3d(pt.x, pt.y, pt.z);
		if ( m_nG23 == 0 ) {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
				pt.x = r * cos(sq) + m_ptOrg.x;
				pt.y += m_dHelicalStep;
				pt.z = r * sin(sq) + m_ptOrg.z;
				::glVertex3d(pt.x, pt.y, pt.z);
			}
		}
		else {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				pt.x = r * cos(sq) + m_ptOrg.x;
				pt.y += m_dHelicalStep;
				pt.z = r * sin(sq) + m_ptOrg.z;
				::glVertex3d(pt.x, pt.y, pt.z);
			}
		}
		pt.x = r * cos(eq) + m_ptOrg.x;
		pt.y = m_nG23==0 ? m_ptOrg.y : dFinalVal;
		pt.z = r * sin(eq) + m_ptOrg.z;
		::glVertex3d(pt.x, pt.y, pt.z);
		break;

	case YZ_PLANE:
		dFinalVal = m_ptValE.x;
		pt.x = m_nG23==0 ? dFinalVal : m_ptOrg.x;
		pt.y = r * cos(sq) + m_ptOrg.y;
		pt.z = r * sin(sq) + m_ptOrg.z;
		::glVertex3d(pt.x, pt.y, pt.z);
		if ( m_nG23 == 0 ) {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
				pt.x += m_dHelicalStep;
				pt.y = r * cos(sq) + m_ptOrg.y;
				pt.z = r * sin(sq) + m_ptOrg.z;
				::glVertex3d(pt.x, pt.y, pt.z);
			}
		}
		else {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				pt.x += m_dHelicalStep;
				pt.y = r * cos(sq) + m_ptOrg.y;
				pt.z = r * sin(sq) + m_ptOrg.z;
				::glVertex3d(pt.x, pt.y, pt.z);
			}
		}
		pt.x = m_nG23==0 ? m_ptOrg.x : dFinalVal;
		pt.y = r * cos(eq) + m_ptOrg.y;
		pt.z = r * sin(eq) + m_ptOrg.z;
		::glVertex3d(pt.x, pt.y, pt.z);
		break;
	}

	::glEnd();

	CNCdata::DrawGL(TRUE);
}

void CNCcircle::SetMaxRect(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("SetMaxRect()", DBG_RED);
#endif
	// 外接する四角形
	double	r = fabs(m_r), sq = m_sq, eq = m_eq;
	CRectD	rcMax;
	CPointD	ptInit[4];

	// 始点・終点の開始位置
	// m_ptValS, m_ptValE を使うと，平面ごとの処理が必要なので
	// m_ptOrg を原点(0,0) とした始点終点を計算
	CPointD	pts(r*cos(sq), r*sin(sq));
	CPointD	pte(r*cos(eq), r*sin(eq));

	// 各象限の軸最大値
	ptInit[0].SetPoint(  r,  0 );
	ptInit[1].SetPoint(  0,  r );
	ptInit[2].SetPoint( -r,  0 );
	ptInit[3].SetPoint(  0, -r );

	// ２点の矩形は必ず通るので，
	// 初期値として最大値・最小値を代入
	// ﾃﾞｶﾙﾄ座標なので、topとbottomは逆
	rcMax.left		= min(pts.x, pte.x);
	rcMax.top		= max(pts.y, pte.y);
	rcMax.right		= max(pts.x, pte.x);
	rcMax.bottom	= min(pts.y, pte.y);

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

	// 象限通過ごとに軸最大値(r)を代入
	int	a;
	for  ( j=1; j<=nCnt; j++ ) {
		a = ( i + j ) % 4;
		if ( rcMax.left > ptInit[a].x )
			rcMax.left = ptInit[a].x;
		if ( rcMax.top < ptInit[a].y )
			rcMax.top = ptInit[a].y;
		if ( rcMax.right < ptInit[a].x )
			rcMax.right = ptInit[a].x;
		if ( rcMax.bottom > ptInit[a].y )
			rcMax.bottom = ptInit[a].y;
	}
	rcMax.NormalizeRect();

	// 空間占有矩形座標設定
	switch ( GetPlane() ) {
	case XY_PLANE:
		rcMax.OffsetRect(m_ptOrg.GetXY());
		m_rcMax.left	= rcMax.left;
		m_rcMax.top		= rcMax.top;
		m_rcMax.right	= rcMax.right;
		m_rcMax.bottom	= rcMax.bottom;
		m_rcMax.low		= m_ptValS.z;
		m_rcMax.high	= m_ptValE.z;
		break;
	case XZ_PLANE:
		rcMax.OffsetRect(m_ptOrg.GetXZ());
		m_rcMax.left	= rcMax.left;
		m_rcMax.top		= m_ptValS.y;
		m_rcMax.right	= rcMax.right;
		m_rcMax.bottom	= m_ptValE.y;
		m_rcMax.low		= rcMax.top;
		m_rcMax.high	= rcMax.bottom;
		break;
	case YZ_PLANE:
		rcMax.OffsetRect(m_ptOrg.GetYZ());
		m_rcMax.left	= m_ptValS.x;
		m_rcMax.top		= rcMax.left;
		m_rcMax.right	= m_ptValE.x;
		m_rcMax.bottom	= rcMax.right;
		m_rcMax.low		= rcMax.top;
		m_rcMax.high	= rcMax.bottom;
		break;
	}
	m_rcMax.NormalizeRect();
#ifdef _DEBUG
	dbg.printf("m_rcMax(left, top   )=(%f, %f)", m_rcMax.left, m_rcMax.top);
	dbg.printf("m_rcMax(right,bottom)=(%f, %f)", m_rcMax.right, m_rcMax.bottom);
	dbg.printf("m_rcMax(high, low   )=(%f, %f)", m_rcMax.high, m_rcMax.low);
#endif
}

tuple<BOOL, CPointD, double, double> CNCcircle::CalcRoundPoint
	(const CNCdata* pNext, double r) const
{
	// ２線の交点(自身の終点)が原点になるように補正
	CPointD	pt, pts( GetPlaneValueOrg(m_ptOrg, m_ptValE) ), pte;
	double	rr1, rr2, xa, ya, r0 = fabs(m_r);
	int		nResult;
	BOOL	bResult = TRUE;

	if ( pNext->GetType() == NCDARCDATA ) {
		const CNCcircle* pCircle = static_cast<const CNCcircle *>(pNext);
		int	nG23next = 1 - pCircle->GetG23();	// 交点への進入は反対回転
		pte = GetPlaneValueOrg(pCircle->GetOrg(), m_ptValE);
		CPointD	p1, p2;
		double	r1, r2, rn = fabs(pCircle->GetR());
		// 自身の円と他方の接線で自身の±r符号を計算
		optional<double> dResult = CalcRoundPoint_CircleInOut(pts, pte, m_nG23, nG23next, r);
		if ( dResult )
			r1 = *dResult;
		else {
			bResult = FALSE;
			return make_tuple(bResult, pt, rr1, rr2);
		}
		// 他方の円と自身の接線で他方の±r符号を計算
		dResult = CalcRoundPoint_CircleInOut(pte, pts, nG23next, m_nG23, r);
		if ( dResult )
			r2 = *dResult;
		else {
			bResult = FALSE;
			return make_tuple(bResult, pt, rr1, rr2);
		}
		// ２つの円の交点を求める -> 解が２つないと面取り出来ないと判断する
		rr1 = r0 + r1;		rr2 = rn + r2;
		tie(nResult, p1, p2) = ::CalcIntersectionPoint_CC(pts, pte, rr1, rr2);
		if ( nResult != 2 ) {
			bResult = FALSE;
			return make_tuple(bResult, pt, rr1, rr2);
		}
		// 解の選択
		double	sx = fabs(pts.x), sy = fabs(pts.y), ex = fabs(pte.x), ey = fabs(pte.y);
		if ( (sx<EPS && ex<EPS) || (sy<EPS && ey<EPS) ||
				(sx>EPS && ex>EPS && fabs(pts.y/pts.x - pte.y/pte.x)<EPS) ) {
			// 中心が同一線上にあるとき，接線と符号が同じ方を選択
			if ( sy < EPS )
				pt = (m_nG23==0 ? -pts.x : pts.x) * p1.y > 0 ? p1 : p2;
			else
				pt = (m_nG23==0 ? pts.y : -pts.y) * p1.x > 0 ? p1 : p2;
		}
		else
			pt = p1.x*p1.x+p1.y*p1.y < p2.x*p2.x+p2.y*p2.y ? p1 : p2;
		// 面取りに相当するC値の計算
		if ( r1 > 0 ) {
			xa = (pt.x*r0+pts.x*r1) / rr1;
			ya = (pt.y*r0+pts.y*r1) / rr1;
		}
		else {
			r1 = fabs(r1);
			xa = (pt.x*r0-pts.x*r1) / rr1;
			ya = (pt.y*r0-pts.y*r1) / rr1;
		}
		rr1 = _hypot(xa, ya);
		if ( r2 > 0 ) {
			xa = (pt.x*rn+pte.x*r2) / rr2;
			ya = (pt.y*rn+pte.y*r2) / rr2;
		}
		else {
			r2 = fabs(r2);
			xa = (pt.x*rn-pte.x*r2) / rr2;
			ya = (pt.y*rn-pte.y*r2) / rr2;
		}
		rr2 = _hypot(xa, ya);
	}
	else {
		pte = GetPlaneValueOrg(pNext->GetEndPoint(), m_ptValE);
		// ｵﾌｾｯﾄ分平行移動させた交点を求める
		tie(bResult, pt, r) = ::CalcOffsetIntersectionPoint_LC(pte, pts,// 直線からのｱﾌﾟﾛｰﾁで
								r0, r, -CalcOffsetSign(), 0, 0);		// 回転方向を反転
		if ( !bResult )
			return make_tuple(bResult, pt, rr1, rr2);
		// 面取りに相当するC値の計算
		rr1 = r0 + r;
		if ( r > 0 ) {
			// 内分点
			xa = (pt.x*r0+pts.x*r) / rr1;
			ya = (pt.y*r0+pts.y*r) / rr1;
		}
		else {
			// 外分点
			r = fabs(r);
			xa = (pt.x*r0-pts.x*r) / rr1;
			ya = (pt.y*r0-pts.y*r) / rr1;
		}
		rr1 = _hypot(xa, ya);
		rr2 = sqrt(pt.x*pt.x + pt.y*pt.y - r*r);
	}

	// 原点補正
	pt += GetPlaneValue(m_ptValE);

	return make_tuple(bResult, pt, rr1, rr2);
}

optional<CPointD> CNCcircle::SetChamferingPoint(BOOL bStart, double c)
{
	CPoint3D	ptOrg3D( bStart ? m_ptValS : m_ptValE );
	CPointD		pt, ptOrg1, ptOrg2, pt1, pt2;
	double		pa, pb, ps;

	switch ( GetPlane() ) {	// 数が多いので GetPlaneValue() は使わない
	case XY_PLANE:
		ptOrg1 = m_ptOrg.GetXY();
		ptOrg2 = ptOrg3D.GetXY();
		pt1 = m_ptValS.GetXY();
		pt2 = m_ptValE.GetXY();
		break;
	case XZ_PLANE:
		ptOrg1 = m_ptOrg.GetXZ();
		ptOrg2 = ptOrg3D.GetXZ();
		pt1 = m_ptValS.GetXZ();
		pt2 = m_ptValE.GetXZ();
		break;
	case YZ_PLANE:
		ptOrg1 = m_ptOrg.GetYZ();
		ptOrg2 = ptOrg3D.GetYZ();
		pt1 = m_ptValS.GetYZ();
		pt2 = m_ptValE.GetYZ();
		break;
	}

	// 長さが足らないときはｴﾗｰ
	if ( m_eq - m_sq > 180.0*RAD ) {
		// 180°を超えるときは直径と比較
		if ( c >= fabs(m_r)*2 )
			return optional<CPointD>();
	}
	else {
		// 180°未満の場合は弦の長さと比較
		if ( c >= _hypot(pt1.x-pt2.x, pt1.y-pt2.y) )
			return optional<CPointD>();
	}

	// ２つの円の交点を求める -> 解が２つないと面取り出来ないと判断する
	int	nResult;
	tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_CC(ptOrg1, ptOrg2, fabs(m_r), c);
	if ( nResult != 2 )
		return optional<CPointD>();

	// 時計回りの場合，始角と終角が入れ替わっているので一時的に元に戻す
	if ( m_nG23 == 0 )
		std::swap(m_sq, m_eq);

	// 解の選択
	ps = bStart ? m_sq : m_eq;
	if ( (pa=atan2(pt1.y-ptOrg1.y, pt1.x-ptOrg1.x)) < 0.0 )
		pa += 360.0*RAD;
	if ( (pb=atan2(pt2.y-ptOrg1.y, pt2.x-ptOrg1.x)) < 0.0 )
		pb += 360.0*RAD;
	// 180度以上の差は補正
	if ( fabs(ps-pa) > 180.0*RAD ) {
		if ( ps > pa )
			ps -= 360.0*RAD;
		else
			pa -= 360.0*RAD;
	}

	// 始角・終角に近い解の選択と自分自身の点を更新
	if ( bStart ) {
		if ( m_nG23 == 0 ) {	// 時計回りの時は，元角よりも小さい方を
			if ( ps > pa )
				pt = pt1;
			else {
				pt = pt2;
				pa = pb;
			}
		}
		else {					// 反時計回りの時は，大きい方を選択
			if ( ps < pa )
				pt = pt1;
			else {
				pt = pt2;
				pa = pb;
			}
		}
		SetPlaneValue(pt, m_ptValS);
		m_sq = pa;
	}
	else {
		if ( m_nG23 == 0 ) {
			if ( ps < pa )
				pt = pt1;
			else {
				pt = pt2;
				pa = pb;
			}
		}
		else {
			if ( ps > pa )
				pt = pt1;
			else {
				pt = pt2;
				pa = pb;
			}
		}
		SetPlaneValue(pt, m_ptValE);
		m_pRead->m_ptValOrg = m_ptValE;
		if ( m_pRead->m_g68.bG68 ) {
			// m_ptValE はG68回転済み座標のため回転を元に戻してｵﾌｾｯﾄ減算
			m_pRead->m_g68.dRound = -m_pRead->m_g68.dRound;
			CalcG68Round(&(m_pRead->m_g68), m_pRead->m_ptValOrg);
			m_pRead->m_g68.dRound = -m_pRead->m_g68.dRound;
		}
		m_pRead->m_ptValOrg -= m_pRead->m_ptOffset;
		m_eq = pa;
		m_pt2D = m_ptValE.PointConvert();
	}

	// 角度補正
	if ( m_nG23 == 0 )
		std::swap(m_sq, m_eq);
	while ( m_sq >= m_eq )
		m_eq += 360.0*RAD;

	return pt;
}

double CNCcircle::CalcBetweenAngle(const CNCdata* pNext) const
{
	// ２線の交点(自身の終点)が原点になるように補正
	CPointD		pt( GetPlaneValueOrg(m_ptOrg, m_ptValE) ), pt1, pt2;

	// 終点接線計算
	int k = -CalcOffsetSign();
	pt1.x = -pt.y*k;	// G02:-90°
	pt1.y =  pt.x*k;	// G03:+90°

	if ( pNext->GetType() == NCDARCDATA ) {
		const CNCcircle* pCircle = static_cast<const CNCcircle *>(pNext);
		// 次のｵﾌﾞｼﾞｪｸﾄが円弧なら中心をｾｯﾄし
		pt = GetPlaneValueOrg(pCircle->GetOrg(), m_ptValE);
		// 始点接線計算
		k = pCircle->CalcOffsetSign();
		pt2.x = -pt.y*k;
		pt2.y =  pt.x*k;
	}
	else {
		pt2 = GetPlaneValueOrg(pNext->GetEndPoint(), m_ptValE);
	}

	// ２線(または円弧の接線)がなす角度を求める
	return ::CalcBetweenAngle(pt1, pt2);
}

int CNCcircle::CalcOffsetSign(void) const
{
	// 回転方向からG41の補正符号を決める
	return m_nG23==0 ? 1 : -1;
}

optional<CPointD> CNCcircle::CalcPerpendicularPoint
	(ENPOINTORDER enPoint, double r, int nSign) const
{
	const CPoint3D&	pts = (enPoint==STARTPOINT) ? m_ptValS : m_ptValE;
	// 始点終点関係なく 回転方向ｘ補正符号
	// ptsと中心の傾きを計算して半径±r
	CPointD	pt( GetPlaneValueOrg(pts, m_ptOrg) );
	double	q = atan2(pt.y, pt.x), rr = fabs(m_r) + r * CalcOffsetSign() * nSign;
	CPointD	pt1(rr*cos(q), rr*sin(q));
	pt1 += GetPlaneValue(m_ptOrg);

	return pt1;
}

optional<CPointD> CNCcircle::CalcOffsetIntersectionPoint
	(const CNCdata* pNext, double r, int k1, int k2) const
{
	BOOL	bResult;
	// ２線の交点(自身の終点)が原点になるように補正
	CPointD	pt;

	if ( pNext->GetType() == NCDARCDATA ) {
		const CNCcircle* pCircle = static_cast<const CNCcircle *>(pNext);
		CPointD	pto1( GetPlaneValueOrg(m_ptOrg, m_ptValE) ),
				pto2( GetPlaneValueOrg(pCircle->GetOrg(), m_ptValE) );
		// ｵﾌｾｯﾄ分半径を調節して２つの円の交点を求める
		CPointD	p1, p2;
		int		nResult;
		tie(nResult, p1, p2) = ::CalcIntersectionPoint_CC(pto1, pto2, fabs(m_r)+r*k1, fabs(pCircle->GetR())+r*k2);
		// 解の選択
		if ( nResult > 0 ) {
			pt = GAPCALC(p1) < GAPCALC(p2) ? p1 : p2;
			bResult = TRUE;
		}
		else
			bResult = FALSE;
	}
	else {
		// ｵﾌｾｯﾄ分平行移動させた交点を求める
		CPointD	pt1( GetPlaneValueOrg(pNext->GetEndPoint(), m_ptValE) ),
				pt2( GetPlaneValueOrg(m_ptOrg, m_ptValE) );
		tie(bResult, pt, r) = ::CalcOffsetIntersectionPoint_LC(pt1, pt2,	// 線からのｱﾌﾟﾛｰﾁで
								fabs(m_r), r, -CalcOffsetSign(), k2, k1);	// 回転方向を反転
	}

	// 原点補正
	if ( bResult ) {
		pt += GetPlaneValue(m_ptValE);
		return pt;
	}

	return optional<CPointD>();
}

optional<CPointD> CNCcircle::CalcOffsetIntersectionPoint2
	(const CNCdata* pNext, double r, int k1, int k2) const
{
	int		k;
	// ２線の交点(自身の終点)が原点になるように補正
	CPointD	pt, pt1, pt2;

	// 始点接線座標
	pt = GetPlaneValueOrg(m_ptOrg, m_ptValE);
	k = -CalcOffsetSign();
	pt1.x = -pt.y * k;
	pt1.y =  pt.x * k;
	// 接線用に符号を再計算
	k1 *= k * ::CalcOffsetSign(pt1);	// k で符号反転

	// 他方の座標計算
	if ( pNext->GetType() == NCDARCDATA ) {
		const CNCcircle* pCircle = static_cast<const CNCcircle *>(pNext);
		pt = GetPlaneValueOrg(pCircle->GetOrg(), m_ptValE);
		k = pCircle->CalcOffsetSign();
		pt2.x = -pt.y * k;
		pt2.y =  pt.x * k;
		k2 *= k * ::CalcOffsetSign(pt2);
	}
	else {
		pt2 = GetPlaneValueOrg(pNext->GetEndPoint(), m_ptValE);
	}

	// 直線同士のｵﾌｾｯﾄ交点計算
	optional<CPointD> ptResult = ::CalcOffsetIntersectionPoint_LL(pt1, pt2, k1, k2, r);
	// 原点補正
	if ( ptResult ) {
		pt = *ptResult + GetPlaneValue(m_ptValE);
		return pt;
	}
	return ptResult;
}

void CNCcircle::SetCorrectPoint(ENPOINTORDER enPoint, const CPointD& ptSrc, double rr)
{
	CPoint3D&	ptVal = enPoint==STARTPOINT ? m_ptValS : m_ptValE;
	CPointD		pt, ptOrg;
	switch ( GetPlane() ) {
	case XY_PLANE:
		ptVal.x = ptSrc.x;
		ptVal.y = ptSrc.y;
		pt = ptVal.GetXY();
		ptOrg = m_ptOrg.GetXY();
		break;
	case XZ_PLANE:
		ptVal.x = ptSrc.x;
		ptVal.z = ptSrc.y;
		pt = ptVal.GetXZ();
		ptOrg = m_ptOrg.GetXZ();
		break;
	case YZ_PLANE:
		ptVal.y = ptSrc.x;
		ptVal.z = ptSrc.y;
		pt = ptVal.GetYZ();
		ptOrg = m_ptOrg.GetYZ();
		break;
	}

	if ( enPoint == STARTPOINT ) {
		double&	q = m_nG23==0 ? m_eq : m_sq;
		if ( (q=atan2(pt.y-ptOrg.y, pt.x-ptOrg.x)) < 0.0 )
			q += 360.0*RAD;
	}
	else {
		m_r = _copysign(fabs(m_r)+rr, m_r);		// 終点の時だけ半径補正
		double&	q = m_nG23==0 ? m_sq : m_eq;
		if ( (q=atan2(pt.y-ptOrg.y, pt.x-ptOrg.x)) < 0.0 )
			q += 360.0*RAD;
		m_pt2D = m_ptValE.PointConvert();
	}
	while ( m_sq >= m_eq )
		m_eq += 360.0*RAD;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

// 円同士の内外半径計算
optional<double> CalcRoundPoint_CircleInOut
	(const CPointD& pts, const CPointD& pte, int nG23, int nG23next, double r)
{
	CPointD	pto;
	double	sx = fabs(pts.x), sy = fabs(pts.y), ex = fabs(pte.x), ey = fabs(pte.y), rr;
	int		k1 = nG23==0 ? -1 : 1, k2 = nG23next==0 ? -1 : 1;

	// 特殊解の判断
	if ( (sx<EPS && ex<EPS && pts.y*pte.y>0) ||
			(sy<EPS && ey<EPS && pts.x*pte.x>0) ||
			(sx>EPS && ex>EPS && fabs(pts.y/pts.x - pte.y/pte.x)<EPS && pts.x*pte.x>0) ) {
		// 中心が同一線上にあり，かつ，x,y の符号が同じとき
		double	l1 = pts.x*pts.x + pts.y*pts.y;
		double	l2 = pte.x*pte.x + pte.y*pte.y;
		if ( fabs(l1 - l2) < NCMIN )	// 距離が等しい==同軌跡円
			return optional<double>();
		else if ( l1 > l2 )
			return -r;
		else
			return r;
	}
	// 自身の円と他方の接線で自身の±r符号を計算
	pto.x = -pte.y*k2;	// G02:-90°
	pto.y =  pte.x*k2;	// G03:+90°

	// 線と円弧の場合と考え方(処理方法)は同じ
	if ( fabs(pto.x) < EPS && sy < EPS )
		rr = _copysign(r, pto.y*pts.x*k1);
	else if ( fabs(pto.y) < EPS && sx < EPS )
		rr = _copysign(r, -pto.x*pts.y*k1);
	else
		rr = _copysign(r, -(pto.x*pts.x + pto.y*pts.y));

	return rr;
}
