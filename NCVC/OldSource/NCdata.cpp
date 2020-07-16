// NCdata.cpp: CNCdata クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "NCdata.h"
#include "ViewOption.h"

#include <math.h>
#include <float.h>

//#define	_DEBUGDRAW_NCD		// 描画処理もﾛｸﾞ
#include "MagaDbgMac.h"
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

// CalcRoundPoint() ｻﾌﾞ
// --- 円同士の内外半径計算
static	double	CalcRoundPoint_CircleInOut(const CPointD&, const CPointD&, int, int, double);

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
		m_ptValS[i] = m_ptValE[i] = m_nc.dValue[i];
		m_dMove[i] = 0.0;
	}
	m_nc.dLength = 0.0;
	m_pt2D = 0.0;
	m_enType = NCDBASEDATA;
}

// 切削(描画)ｺｰﾄﾞ以外のｺﾝｽﾄﾗｸﾀ
CNCdata::CNCdata(const CNCdata* pData, LPNCARGV lpArgv)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CNCdata", DBG_MAGENTA);
#endif
	// 指定された値のﾌﾗｸﾞ
	extern	const	DWORD	g_dwSetValFlags[];
	int		i;

	Constracter(lpArgv);
	// 座標指定のないﾃﾞｰﾀは前回座標から取得
	for ( i=0; i<NCXYZ; i++ ) {
		// 指定されている分だけ代入
		if ( m_nc.dwValFlags & g_dwSetValFlags[i] ) {
			m_nc.dValue[i] = lpArgv->nc.dValue[i];
			m_ptValS[i] = m_ptValE[i] =
				m_nc.nGcode==92 ? lpArgv->nc.dValue[i] : pData->GetEndValue(i);
		}
		else
			m_ptValS[i] = m_ptValE[i] = m_nc.dValue[i] = pData->GetEndValue(i);
		m_dMove[i] = 0.0;
	}
	// 座標値以外はそのまま引き継ぎ
	for ( ; i<VALUESIZE; i++ )
		m_nc.dValue[i] = pData->GetValue(i);
	m_nc.dLength = 0.0;
	// G92なら新規計算，でなければ前回の計算値を引き継ぎ
	m_pt2D = m_nc.nGcode==92 ? m_ptValE.PointConvert() : pData->Get2DPoint();
	m_enType = NCDBASEDATA;

#ifdef _DEBUG
	DbgDump();
	Dbg_sep();
#endif
}

// 派生ｸﾗｽ用ｺﾝｽﾄﾗｸﾀ
CNCdata::CNCdata(ENNCDTYPE enType, const CNCdata* pData, LPNCARGV lpArgv)
{
	extern	const	DWORD	g_dwSetValFlags[];
	int	i;

	Constracter(lpArgv);
	// 座標指定のないﾃﾞｰﾀは前回座標から計算
	for ( i=0; i<NCXYZ; i++ ) {
		// 指定されているときだけ代入
		if ( m_nc.dwValFlags & g_dwSetValFlags[i] )
			m_nc.dValue[i] = lpArgv->bAbs ?		// 座標の補正
				lpArgv->nc.dValue[i] : (pData->GetEndValue(i)+lpArgv->nc.dValue[i]);
		else
			m_nc.dValue[i] = pData->GetEndValue(i);
	}
	for ( ; i<VALUESIZE; i++ )
		m_nc.dValue[i] = m_nc.dwValFlags & g_dwSetValFlags[i] ?
			lpArgv->nc.dValue[i] : 0.0;
/*
	m_ptValS, m_ptValE,
	m_nc.dLength, m_pt2D, m_rcMax は派生ｸﾗｽで代入
*/
	m_enType = enType;
}

CPointD CNCdata::GetPlaneValue(const CPoint3D& ptVal)
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
	default:
		pt = HUGE_VAL;
	}
	return pt;
}

CPointD	CNCdata::GetPlaneValueOrg(const CPoint3D& pt1, const CPoint3D& pt2)
{
	CPointD	pt;
	switch ( GetPlane() ) {
	case XY_PLANE:
		pt = pt2.GetXY() - pt1.GetXY();
		break;
	case XZ_PLANE:
		pt = pt2.GetXZ() - pt1.GetXZ();
		break;
	case YZ_PLANE:
		pt = pt2.GetYZ() - pt1.GetYZ();
		break;
	default:
		pt = HUGE_VAL;
	}
	return pt;
}

#ifdef _DEBUG
void CNCdata::DbgDump(void)
{
	CMagaDbg	dbg("CNCdata", DBG_MAGENTA);
	extern	const	DWORD	g_dwSetValFlags[];
	extern	LPCTSTR	g_szGtester[];
	extern	LPCTSTR	g_szNtester[];

	CString	strBuf, strTmp;
	if ( GetGtype()<0 || GetGtype()>GTYPESIZE )
		strBuf.Format("%s%d: ", "NO_TYPE:", GetGcode());
	else
		strBuf.Format("%s%d: ", g_szGtester[GetGtype()], GetGcode());
	for ( int i=0; i<VALUESIZE; i++ ) {
		if ( GetValFlags() & g_dwSetValFlags[i] ) {
			strTmp.Format("%s%.3f", g_szNtester[i], GetValue(i));
			strBuf += strTmp;
		}
	}
	dbg.printf("%s", strBuf);
}
#endif

//////////////////////////////////////////////////////////////////////
// CNCline クラス
//////////////////////////////////////////////////////////////////////
CNCline::CNCline(const CNCdata* pData, LPNCARGV lpArgv) :
		CNCdata(NCDLINEDATA, pData, lpArgv)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CNCline", DBG_MAGENTA);
#endif
	// 描画始点を前回の計算値から取得
	m_pts = pData->Get2DPoint();
	m_ptValS = pData->GetEndPoint();
	// 最終座標(==指定座標)ｾｯﾄ
	m_ptValE.SetPoint(GetValue(NCA_X), GetValue(NCA_Y), GetValue(NCA_Z));
	// 移動長，切削長の計算
	CalcLength();
	// 描画終点を計算し保存
	m_pt2D = m_pte = m_ptValE.PointConvert();

	// 空間占有矩形
	SetMaxRect();

#ifdef _DEBUG
	DbgDump();
	dbg.printf("Length = %f", m_nc.dLength);
	Dbg_sep();
#endif
}

void CNCline::DrawTuning(double f)
{
	m_ptDrawS = m_pts * f;
	m_ptDrawE = m_pte * f;
}

void CNCline::DrawTuningXY(double f)
{
	m_ptDrawS_XY = m_ptValS.GetXY() * f;
	m_ptDrawE_XY = m_ptValE.GetXY() * f;
}

void CNCline::DrawTuningXZ(double f)
{
	m_ptDrawS_XZ = m_ptValS.GetXZ() * f;
	m_ptDrawE_XZ = m_ptValE.GetXZ() * f;
}

void CNCline::DrawTuningYZ(double f)
{
	m_ptDrawS_YZ = m_ptValS.GetYZ() * f;
	m_ptDrawE_YZ = m_ptValE.GetYZ() * f;
}

void CNCline::Draw(CDC* pDC)
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCline::Draw()", DBG_RED);
	dbg.printf("Line=%d", GetStrLine());
#endif
	CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(GetPenType()));
	pDC->MoveTo(m_ptDrawS);
	pDC->LineTo(m_ptDrawE);
	pDC->SelectObject(pOldPen);
}

void CNCline::DrawXY(CDC* pDC)
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCline::DrawXY()", DBG_RED);
	dbg.printf("Line=%d", GetStrLine());
#endif
	CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(GetPenType()));
	pDC->MoveTo(m_ptDrawS_XY);
	pDC->LineTo(m_ptDrawE_XY);
	pDC->SelectObject(pOldPen);
}

void CNCline::DrawXZ(CDC* pDC)
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCline::DrawXZ()", DBG_RED);
	dbg.printf("Line=%d", GetStrLine());
#endif
	CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(GetPenType()));
	pDC->MoveTo(m_ptDrawS_XZ);
	pDC->LineTo(m_ptDrawE_XZ);
	pDC->SelectObject(pOldPen);
}

void CNCline::DrawYZ(CDC* pDC)
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCline::DrawYZ()", DBG_RED);
	dbg.printf("Line=%d", GetStrLine());
#endif
	CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(GetPenType()));
	pDC->MoveTo(m_ptDrawS_YZ);
	pDC->LineTo(m_ptDrawE_YZ);
	pDC->SelectObject(pOldPen);
}

BOOL CNCline::CalcRoundPoint
	(const CNCdata* pNext, double r, CPointD& pt, double& rr1, double& rr2)
{
	// ２線の交点(自身の終点)が原点になるように補正
	CPointD	pts( GetPlaneValueOrg(m_ptValE, m_ptValS) );

	if ( pNext->GetGcode() > 1 ) {
		CNCcircle*	pCircle = (CNCcircle *)pNext;
		CPointD	pto( GetPlaneValueOrg(m_ptValE, pCircle->GetOrg()) );
		// ｵﾌｾｯﾄ分平行移動させた交点を求める
		double	rr, xa, ya, rn = fabs(pCircle->GetR());
		pt = ::CalcOffsetIntersectionPoint(pts, pto, rn, r,
					pCircle->GetG23()==0 ? 1 : -1, 0, 0);
		if ( pt == HUGE_VAL )
			return FALSE;
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
		rr2 = sqrt(xa*xa + ya*ya);
	}
	else {
		CPointD	pte( GetPlaneValueOrg(m_ptValE, pNext->GetEndPoint()) );
		// ｵﾌｾｯﾄ分平行移動させた交点を求める
		pt = ::CalcOffsetIntersectionPoint(pts, pte, 0, 0, r);
		if ( pt == HUGE_VAL )
			return FALSE;
		// 面取りに相当するC値の計算
		rr1 = rr2 = sqrt(pt.x*pt.x + pt.y*pt.y - r*r);
	}

	// 原点補正
	pt += GetPlaneValue(m_ptValE);

	return TRUE;
}

BOOL CNCline::SetChamferingPoint(BOOL bStart, double c, CPointD& pt)
{
	// 長さが足らないときはｴﾗｰ
	if ( c >= GetCutLength() )
		return FALSE;

	CPoint3D&	ptValS = bStart ? m_ptValS : m_ptValE,
				ptValE = bStart ? m_ptValE : m_ptValS;
	CPointD		pto, pte;
	switch ( GetPlane() ) {
	case XY_PLANE:
		pto = ptValS.GetXY();
		pte = ptValE.GetXY();
		break;
	case XZ_PLANE:
		pto = ptValS.GetXZ();
		pte = ptValE.GetXZ();
		break;
	case YZ_PLANE:
		pto = ptValS.GetYZ();
		pte = ptValE.GetYZ();
		break;
	default:
		return FALSE;
	}

	// pto を中心とした円と pte の交点
	pt = ::CalcIntersectionPoint(pto, c, pte);

	// 自分自身の点も更新
	switch ( GetPlane() ) {
	case XY_PLANE:
		ptValS.x = pt.x;
		ptValS.y = pt.y;
		break;
	case XZ_PLANE:
		ptValS.x = pt.x;
		ptValS.z = pt.y;
		break;
	case YZ_PLANE:
		ptValS.y = pt.x;
		ptValS.z = pt.y;
		break;
	}
	if ( bStart )
		m_pts = m_ptValS.PointConvert();
	else
		m_pt2D = m_pte = m_ptValE.PointConvert();
	// 移動長，切削長の計算
	CalcLength();

	return TRUE;
}

double CNCline::CalcBetweenAngle(const CPoint3D& pts, const CNCdata* pNext)
{
	// ２線の交点(自身の終点)が原点になるように補正
	CPoint3D	pto(GetValue(NCA_X), GetValue(NCA_Y), GetValue(NCA_Z));
	CPointD		pt1( GetPlaneValueOrg(pto, pts) ), pt2;

	if ( pNext->GetGcode() > 1 ) {
		CNCcircle*	pCircle = (CNCcircle *)pNext;
		// 次のｵﾌﾞｼﾞｪｸﾄが円弧なら中心をｾｯﾄし
		CPointD	pt( GetPlaneValueOrg(pto, pCircle->GetOrg()) );
		// 接線計算
		int k = pCircle->GetG23()==0 ? 1 : -1;	// G02
		pt2.x = -pt.y*k;	// G02:+90°
		pt2.y =  pt.x*k;	// G03:-90°
	}
	else {
		CPoint3D	pte(pNext->GetValue(NCA_X), pNext->GetValue(NCA_Y), pNext->GetValue(NCA_Z));
		pt2 = GetPlaneValueOrg(pto, pte);
	}
	
	// ２線(または円弧の接線)がなす角度を求める
	return ::CalcBetweenAngleTwoLines(pt1, pt2);
}

int CNCline::CalcOffsetSign(const CPoint3D& pts)
{
	// 始点を原点に進行方向の角度を計算
	CPoint3D	pte(GetValue(NCA_X), GetValue(NCA_Y), GetValue(NCA_Z));
	CPointD		pt( GetPlaneValueOrg(pts, pte) );
	double	dAngle = atan2(pt.y, pt.x);
	if ( dAngle < 0.0 )
		dAngle += 360.0*RAD;

	// G41の補正符号を決める
	return 90.0*RAD<=dAngle && dAngle<270.0*RAD ? -1 : 1;
}

CPointD CNCline::CalcPerpendicularPoint
	(const CPoint3D& pts, const CPoint3D& pte, double r, int nSign)
{
	// 線の傾きを計算して90°回転
	CPointD	pt( GetPlaneValueOrg(pts, pte) );
	double	q = atan2(pt.y, pt.x);
	CPointD	pt1(r*cos(q), r*sin(q));
	CPointD	pt2(-pt1.y*nSign, pt1.x*nSign);
	switch ( GetPlane() ) {
	case XY_PLANE:
		pt2 += pts.GetXY();
		break;
	case XZ_PLANE:
		pt2 += pts.GetXZ();
		break;
	case YZ_PLANE:
		pt2 += pts.GetYZ();
		break;
	}

	return pt2;
}

CPointD CNCline::CalcOffsetIntersectionPoint
	(const CPoint3D& pts, const CNCdata* pNext, double r, int k1, int k2)
{
	// ２線の交点(自身の終点)が原点になるように補正
	CPoint3D	pto(GetValue(NCA_X), GetValue(NCA_Y), GetValue(NCA_Z));
	CPointD		pt, pt1( GetPlaneValueOrg(pto, pts) ), pt2;

	// ｵﾌｾｯﾄ分平行移動させた交点を求める
	if ( pNext->GetGcode() > 1 ) {
		CNCcircle*	pCircle = (CNCcircle *)pNext;
		pt2 = GetPlaneValueOrg(pto, pCircle->GetOrg());
		pt = ::CalcOffsetIntersectionPoint(pt1, pt2, fabs(pCircle->GetR()), r,
					pCircle->GetG23()==0 ? 1 : -1, k1, k2);
	}
	else {
		CPoint3D	pte(pNext->GetValue(NCA_X), pNext->GetValue(NCA_Y), pNext->GetValue(NCA_Z));
		pt2 = GetPlaneValueOrg(pto, pte);
		pt = ::CalcOffsetIntersectionPoint(pt1, pt2, k1, k2, r);
	}

	// 原点補正
	if ( pt != HUGE_VAL )
		pt += GetPlaneValue(pto);
	return pt;
}

CPointD CNCline::CalcOffsetIntersectionPoint2
	(const CPoint3D& pts, const CNCdata* pNext, double r, int k1, int k2)
{
	// ２線の交点(自身の終点)が原点になるように補正
	CPoint3D	pto(GetValue(NCA_X), GetValue(NCA_Y), GetValue(NCA_Z));
	CPointD		pt, pt1( GetPlaneValueOrg(pto, pts) ), pt2;

	// ｵﾌｾｯﾄ分平行移動させた交点を求める
	if ( pNext->GetGcode() > 1 ) {
		// 円弧は接線とのｵﾌｾｯﾄ交点計算
		CNCcircle*	pCircle = (CNCcircle *)pNext;
		pt = GetPlaneValueOrg(pto, pCircle->GetOrg());
		int k = pCircle->GetG23()==0 ? 1 : -1;
		pt2.x = -pt.y * k;
		pt2.y =  pt.x * k;
		// 接線用に符号を再計算
		double	dAngle = atan2(pt2.y, pt2.x);
		if ( dAngle < 0.0 )
			dAngle += 360.0*RAD;
		k2 = 90.0*RAD<=dAngle && dAngle<270.0*RAD ? -1 : 1;
	}
	else {
		CPoint3D	pte(pNext->GetValue(NCA_X), pNext->GetValue(NCA_Y), pNext->GetValue(NCA_Z));
		pt2 = GetPlaneValueOrg(pto, pte);
	}

	// 直線同士のｵﾌｾｯﾄ交点計算
	pt = ::CalcOffsetIntersectionPoint(pt1, pt2, k1, k2, r);
	// 原点補正
	if ( pt != HUGE_VAL )
		pt += GetPlaneValue(pto);
	return pt;
}

BOOL CNCline::SetCorrectPoint(BOOL bStart, const CPointD& pt, double)
{
	CPoint3D&	ptVal = bStart ? m_ptValS : m_ptValE;
	switch ( GetPlane() ) {
	case XY_PLANE:
		ptVal.x = pt.x;
		ptVal.y = pt.y;
		break;
	case XZ_PLANE:
		ptVal.x = pt.x;
		ptVal.z = pt.y;
		break;
	case YZ_PLANE:
		ptVal.y = pt.x;
		ptVal.z = pt.y;
		break;
	default:
		return FALSE;
	}

	if ( bStart )
		m_pts = m_ptValS.PointConvert();
	else
		m_pt2D = m_pte = m_ptValE.PointConvert();
	CalcLength();

	return TRUE;
}

//////////////////////////////////////////////////////////////////////
// CNCcycle クラス
//////////////////////////////////////////////////////////////////////
CNCcycle::CNCcycle(const CNCdata* pData, LPNCARGV lpArgv) :
		CNCline(NCDCYCLEDATA, pData, lpArgv)
{
/*
	Z, R, P 値は，TH_NCRead.cpp でも補間していることに注意
*/
#ifdef _DEBUG
	CMagaDbg	dbg("CNCcycle", DBG_MAGENTA);
#endif
	double		dx, dy,		// XY各軸の移動距離
				dR, dI,		// R点座標, ｲﾆｼｬﾙ座標
				dRLength, dZLength;		// 移動長，切削長
	int			i, nH, nV;	// 縦横の繰り返し数

	m_Cycle = NULL;
	m_CycleXY = NULL;
	m_CycleXZ = NULL;
	m_CycleYZ = NULL;

	// 描画始点を前回の計算値から取得
	m_pts = pData->Get2DPoint();
	m_ptValS = pData->GetEndPoint();
	// 縦繰り返し数取得
	if ( GetValFlags() & NCD_K )
		nV = max(0, (int)GetValue(NCA_K));
	else if ( GetValFlags() & NCD_L )
		nV = max(0, (int)GetValue(NCA_L));
	else
		nV = 1;
	// 復帰座標(前回のｵﾌﾞｼﾞｪｸﾄが固定ｻｲｸﾙかどうか)
	m_dInitial = pData->GetType()!=NCDCYCLEDATA ? m_ptValS.z :
							((CNCcycle *)pData)->GetInitialValue();
	// ｲﾝｸﾘﾒﾝﾀﾙ補正(R座標はﾍﾞｰｽｸﾗｽで座標補正の対象外)
	if ( lpArgv->bAbs ) {
		dR = GetValFlags() & NCD_R ? GetValue(NCA_R) : m_ptValS.z;
		m_nDrawCnt = nH = min(1, nV);	// ｱﾌﾞｿﾘｭｰﾄなら横へは(0 or 1)回のみ
	}
	else {
		dR = GetValFlags() & NCD_R ? m_dInitial + GetValue(NCA_R) : m_dInitial;
		// !!! Z値もR点からのｲﾝｸﾘﾒﾝﾄに補正 !!!
		if ( GetValFlags() & NCD_Z )
			m_nc.dValue[NCA_Z] = dR + lpArgv->nc.dValue[NCA_Z];
		m_nDrawCnt = nH = nV;	// ｲﾝｸﾘﾒﾝﾀﾙなら横へも繰り返し
	}
	dI = lpArgv->bInitial ? m_dInitial : dR;

	// 繰り返し数ｾﾞﾛなら以降の計算は不要
	if ( m_nDrawCnt <= 0 ) {
		for ( i=0; i<NCXYZ; m_dMove[i++]=0.0 );
		m_dDwell = 0.0;
		m_nc.dLength = m_dCycleMove = 0.0;
		m_ptValE = m_ptValS;
		m_pt2D = m_pte = m_pts;
		return;
	}

	// 各軸ごとの移動長計算
	dx = GetValue(NCA_X) - m_ptValS.x;
	dy = GetValue(NCA_Y) - m_ptValS.y;
	dRLength = fabs(dI - dR);
	dZLength = fabs(dR - GetValue(NCA_Z));
	m_dMove[NCA_X] = fabs(dx) * nH;
	m_dMove[NCA_Y] = fabs(dy) * nH;
	m_dMove[NCA_Z] = fabs(m_ptValS.z - dR);	// 初回下降分
	m_dMove[NCA_Z] += dRLength * (nV-1);
	// 移動長計算
	m_dCycleMove = sqrt(
			m_dMove[NCA_X]*m_dMove[NCA_X] +
			m_dMove[NCA_Y]*m_dMove[NCA_Y] );
	m_dCycleMove += m_dMove[NCA_Z];
	// 切削長
	m_nc.dLength = dZLength * nV;
	// 各座標値の設定
	m_ptValE.SetPoint(m_ptValS.x + dx * nH, m_ptValS.y + dy * nH, dI);
	m_ptValI.SetPoint(GetValue(NCA_X), GetValue(NCA_Y), m_ptValS.z);	// 描画用
	m_ptValR.SetPoint(GetValue(NCA_X), GetValue(NCA_Y), dI);
	m_pte = m_ptValE.PointConvert();
	m_pt2D = m_pte;

	// Zが指定されていなければｴﾗｰ(補間はTH_NCRead.cppにて)
	if ( !(GetValFlags() & NCD_Z) ) {
		SetNCFlags(NCF_ERROR);
		m_nDrawCnt = 0;
		m_dMove[NCA_Z] = 0.0;
		m_dDwell = 0.0;
		return;
	}

#ifdef _DEBUG
	dbg.printf("StartPoint x=%.3f y=%.3f z=%.3f",
		m_ptValS.x, m_ptValS.y, m_ptValS.z);
	dbg.printf("           R-Point=%.3f Z-Point=%.3f", dR, GetValue(NCA_Z));
	dbg.printf("FinalPoint x=%.3f y=%.3f z=%.3f",
		m_ptValE[NCA_X], m_ptValE[NCA_Y], m_ptValE[NCA_Z]);
	dbg.printf("m_nDrawCnt=%d", m_nDrawCnt);
#endif

	// 座標格納領域確保
	m_Cycle = new PTCYCLE[m_nDrawCnt];
	m_CycleXY = new PTCYCLE_XY[m_nDrawCnt];
	m_CycleXZ = new PTCYCLE[m_nDrawCnt];
	m_CycleYZ = new PTCYCLE[m_nDrawCnt];

	CPoint3D	pt(m_ptValS);	// 現在位置で初期化
	for ( i=0; i<m_nDrawCnt; i++ ) {
		pt.x += dx;		pt.y += dy;
#ifdef _DEBUG
		dbg.printf("           No.%d x=%.3f y=%.3f", i+1, pt.x, pt.y);
#endif
		// XYZ, XZ, YZ
		pt.z = dI;
		m_Cycle[i].ptI = pt.PointConvert();
		m_CycleXZ[i].ptI = pt.GetXZ();
		m_CycleYZ[i].ptI = pt.GetYZ();
		pt.z = dR;
		m_Cycle[i].ptR = pt.PointConvert();
		m_CycleXZ[i].ptR = pt.GetXZ();
		m_CycleYZ[i].ptR = pt.GetYZ();
		pt.z = GetValue(NCA_Z);
		m_Cycle[i].ptZ = pt.PointConvert();
		m_CycleXZ[i].ptZ = pt.GetXZ();
		m_CycleYZ[i].ptZ = pt.GetYZ();
		// XY
		m_CycleXY[i].pt = pt;	// Z無視
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
			m_dMove[NCA_Z] += dResult;
			m_dCycleMove += dResult;
		}
		else
			m_nc.dLength += dZLength * nV;
		break;
	default:	// それ以外は早送り復帰
		dResult = (dRLength + dZLength) * nV;
		m_dMove[NCA_Z] += dResult;
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
	dbg.printf("Move=%f Cut=%f", m_dCycleMove, m_nc.dLength);
	Dbg_sep();
#endif

	// 空間占有矩形
	SetMaxRect();
}

CNCcycle::~CNCcycle()
{
	if ( m_Cycle )
		delete[] m_Cycle;
	if ( m_CycleXY )
		delete[] m_CycleXY;
	if ( m_CycleXZ )
		delete[]	m_CycleXZ;
	if ( m_CycleYZ )
		delete[] m_CycleYZ;
}

void CNCcycle::DrawTuning(double f)
{
	if ( m_nDrawCnt > 0 ) {
		CNCline::DrawTuning(f);
		m_ptDrawI = m_ptValI.PointConvert() * f;
		m_ptDrawR = m_ptValR.PointConvert() * f;
		for ( int i=0; i<m_nDrawCnt; i++ )
			m_Cycle[i].DrawTuning(f);
	}
}

void CNCcycle::DrawTuningXY(double f)
{
	if ( m_nDrawCnt > 0 ) {
		CNCline::DrawTuningXY(f);	// Z座標は無関係
		for ( int i=0; i<m_nDrawCnt; i++ )
			m_CycleXY[i].DrawTuning(f);
	}
}

void CNCcycle::DrawTuningXZ(double f)
{
	if ( m_nDrawCnt > 0 ) {
		CNCline::DrawTuningXZ(f);
		m_ptDrawI_XZ = m_ptValI.GetXZ() * f;
		m_ptDrawR_XZ = m_ptValR.GetXZ() * f;
		for ( int i=0; i<m_nDrawCnt; i++ )
			m_CycleXZ[i].DrawTuning(f);
	}
}

void CNCcycle::DrawTuningYZ(double f)
{
	if ( m_nDrawCnt > 0 ) {
		CNCline::DrawTuningYZ(f);
		m_ptDrawI_YZ = m_ptValI.GetYZ() * f;
		m_ptDrawR_YZ = m_ptValR.GetYZ() * f;
		for ( int i=0; i<m_nDrawCnt; i++ )
			m_CycleYZ[i].DrawTuning(f);
	}
}

void CNCcycle::Draw(CDC* pDC)
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCcycle::Draw()", DBG_RED);
#endif
	if ( m_nDrawCnt > 0 ) {
#ifdef _DEBUGDRAW_NCD
		dbg.printf("Line=%d", GetStrLine());
#endif
		CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(NCPEN_G0));
		// 前回位置から１点目の穴加工までの移動
		pDC->MoveTo(m_ptDrawS);
		pDC->LineTo(m_ptDrawI);		// 現在位置から１点目のｲﾆｼｬﾙ点
		pDC->LineTo(m_ptDrawR);		// ｲﾆｼｬﾙ点からR点
		pDC->LineTo(m_ptDrawE);		// R点から最後の穴加工
		// Z方向の切削
		for ( int i=0; i<m_nDrawCnt; i++ ) {
			pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(NCPEN_G0));
			pDC->MoveTo(m_Cycle[i].ptDrawI);
			pDC->LineTo(m_Cycle[i].ptDrawR);
			pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(NCPEN_CYCLE));
			pDC->LineTo(m_Cycle[i].ptDrawZ);
		}
		pDC->SelectObject(pOldPen);
	}
#ifdef _DEBUGDRAW_NCD
	else
		dbg.printf("Line=%d NoDraw!", GetStrLine());
#endif
}

void CNCcycle::DrawXY(CDC* pDC)
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCcycle::DrawXY()", DBG_RED);
#endif
	if ( m_nDrawCnt > 0 ) {
#ifdef _DEBUGDRAW_NCD
		dbg.printf("Line=%d", GetStrLine());
#endif
		CNCline::DrawXY(pDC);
		CPen* pOldPen = (CPen *)(pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(NCPEN_CYCLE)));
		CBrush* pOldBrush = (CBrush *)(pDC->SelectObject(AfxGetNCVCMainWnd()->GetBrushNC(NCBRUSH_CYCLEXY)));
		for ( int i=0; i<m_nDrawCnt; i++ )
			pDC->Ellipse(&m_CycleXY[i].rcDraw);
		pDC->SelectObject(pOldBrush);
		pDC->SelectObject(pOldPen);
	}
#ifdef _DEBUGDRAW_NCD
	else
		dbg.printf("Line=%d NoDraw!", GetStrLine());
#endif
}

void CNCcycle::DrawXZ(CDC* pDC)
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCcycle::DrawXZ()", DBG_RED);
#endif
	if ( m_nDrawCnt > 0 ) {
#ifdef _DEBUGDRAW_NCD
		dbg.printf("Line=%d", GetStrLine());
#endif
		CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(NCPEN_G0));
		pDC->MoveTo(m_ptDrawS_XZ);
		pDC->LineTo(m_ptDrawI_XZ);
		pDC->LineTo(m_ptDrawR_XZ);
		pDC->LineTo(m_ptDrawE_XZ);
		for ( int i=0; i<m_nDrawCnt; i++ ) {
			pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(NCPEN_G0));
			pDC->MoveTo(m_CycleXZ[i].ptDrawI);
			pDC->LineTo(m_CycleXZ[i].ptDrawR);
			pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(NCPEN_CYCLE));
			pDC->LineTo(m_CycleXZ[i].ptDrawZ);
		}
		pDC->SelectObject(pOldPen);
	}
#ifdef _DEBUGDRAW_NCD
	else
		dbg.printf("Line=%d NoDraw!", GetStrLine());
#endif
}

void CNCcycle::DrawYZ(CDC* pDC)
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCcycle::DrawYZ()", DBG_RED);
#endif
	if ( m_nDrawCnt > 0 ) {
#ifdef _DEBUGDRAW_NCD
		dbg.printf("Line=%d", GetStrLine());
#endif
		CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(NCPEN_G0));
		pDC->MoveTo(m_ptDrawS_YZ);
		pDC->LineTo(m_ptDrawI_YZ);
		pDC->LineTo(m_ptDrawR_YZ);
		pDC->LineTo(m_ptDrawE_YZ);
		for ( int i=0; i<m_nDrawCnt; i++ ) {
			pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(NCPEN_G0));
			pDC->MoveTo(m_CycleYZ[i].ptDrawI);
			pDC->LineTo(m_CycleYZ[i].ptDrawR);
			pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(NCPEN_CYCLE));
			pDC->LineTo(m_CycleYZ[i].ptDrawZ);
		}
		pDC->SelectObject(pOldPen);
	}
#ifdef _DEBUGDRAW_NCD
	else
		dbg.printf("Line=%d NoDraw!", GetStrLine());
#endif
}

//////////////////////////////////////////////////////////////////////
// CNCcircle クラス
//////////////////////////////////////////////////////////////////////
CNCcircle::CNCcircle(const CNCdata* pData, LPNCARGV lpArgv) :
		CNCdata(NCDARCDATA, pData, lpArgv)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CNCcircle", DBG_MAGENTA);
#endif
	BOOL		fError;

	m_nG23 = GetGcode() - 2;	// G2=0, G3=1
/*
	XZ平面は(Z->X, X->Y)なのでそのまま計算すると -90°回転させる必要がある
	簡単に対応するには符号を反対にすればよい -> もうちょっとマシな対応を！
*/
	if ( GetPlane() == XZ_PLANE )
		m_nG23 = 1 - m_nG23;	// 0->1 , 1->0;

	m_ptValS = pData->GetEndPoint();
	m_ptValE.SetPoint(GetValue(NCA_X), GetValue(NCA_Y), GetValue(NCA_Z));
	CPointD	pts( m_ptValS ), pte( m_ptValE ), pto;

	// 半径と中心座標の計算(R優先)
	if ( GetValFlags() & NCD_R ) {
		m_r = GetValue(NCA_R);
		fError = CalcCenter(pts, pte);
	}
	else if ( GetValFlags() & (NCD_I|NCD_J|NCD_K) ) {
		m_ptOrg = m_ptValS;
		fError = FALSE;
		switch ( GetPlane() ) {
		case XY_PLANE:
			m_r = hypot(GetValue(NCA_I), GetValue(NCA_J));
			m_ptOrg.x += GetValue(NCA_I);
			m_ptOrg.y += GetValue(NCA_J);
			pto = m_ptOrg.GetXY();
			break;
		case XZ_PLANE:
			m_r = hypot(GetValue(NCA_I), GetValue(NCA_K));
			m_ptOrg.x += GetValue(NCA_I);
			m_ptOrg.z += GetValue(NCA_K);
			pto = m_ptOrg.GetXZ();
			break;
		case YZ_PLANE:
			m_r = hypot(GetValue(NCA_J), GetValue(NCA_K));
			m_ptOrg.y += GetValue(NCA_J);
			m_ptOrg.z += GetValue(NCA_K);
			pto = m_ptOrg.GetYZ();
			break;
		default:
			fError = TRUE;
		}
		if ( !fError ) {
			pts -= pto;
			pte -= pto;
			AngleTuning(pts, pte);
		}
	}
	else {
		fError = TRUE;
	}

	// 描画関数の決定とﾍﾘｶﾙ移動量の計算
	switch ( GetPlane() ) {
	case XY_PLANE:
		m_pfnCircleDraw = Draw_G17;		// 「&」は不要
		m_dHelicalStep = GetValFlags() & NCD_Z ?
			(m_ptValE.z - m_ptValS.z) / ((m_eq-m_sq)/ARCSTEP) : 0.0;
		break;
	case XZ_PLANE:
		m_pfnCircleDraw = Draw_G18;
		m_dHelicalStep = GetValFlags() & NCD_Y ?
			(m_ptValE.y - m_ptValS.y) / ((m_eq-m_sq)/ARCSTEP) : 0.0;
		break;
	case YZ_PLANE:
		m_pfnCircleDraw = Draw_G19;
		m_dHelicalStep = GetValFlags() & NCD_X ?
			(m_ptValE.x - m_ptValS.x) / ((m_eq-m_sq)/ARCSTEP) : 0.0;
		break;
	}

	if ( m_nG23 == 0 )
		m_dHelicalStep *= -1.0;		// 符号反転

	// 描画終点を計算し保存
	m_pt2D = m_ptValE.PointConvert();

#ifdef _DEBUG
	dbg.printf("gcode=%d", m_nG23);
	dbg.printf("sx=%.3f sy=%.3f sz=%.3f ex=%.3f ey=%.3f ez=%.3f r=%.3f",
		m_ptValS.x, m_ptValS.y, m_ptValS.z,
		m_ptValE.x, m_ptValE.y, m_ptValE.z, m_r);
	dbg.printf("px=%.3f py=%.3f pz=%.3f sq=%f eq=%f",
		m_ptOrg.x, m_ptOrg.y, m_ptOrg.z, m_sq*DEG, m_eq*DEG);
#endif
	// 切削長計算
	CalcLength();
	// 空間占有矩形
	SetMaxRect();

	if ( fError )
		SetNCFlags(NCF_ERROR);
#ifdef _DEBUG
	DbgDump();
	dbg.printf("Length = %f", m_nc.dLength);
	Dbg_sep();
#endif
}

BOOL CNCcircle::CalcCenter(const CPointD& pts, const CPointD& pte)
{
#ifdef _DEBUG
	CMagaDbg	dbg("Center()", DBG_RED);
#endif
	// R 指定で始点終点が同じ場合はエラー
	if ( pts == pte )
		return TRUE;

	// 円の方程式より，(s.x, s.y)，(e.x, e.y) を中心とした
	// ２つの円の交点を求める
	CPointD	pt1, pt2;
	int		nResult;
	if ( (nResult=::CalcIntersectionPoint(pts, pte, m_r, m_r, &pt1, &pt2)) <= 0 )
		return TRUE;

	// どちらの解を採用するか
	AngleTuning(pts-pt1, pte-pt1);	// まず一方の中心座標から角度を求める
	if ( nResult==1 ||
		(m_r>0.0 && fabs(m_sq - m_eq)<180.0*RAD) ||
		(m_r<0.0 && fabs(m_sq - m_eq)>180.0*RAD) ) {
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
	// 常に s<e (反時計回り) とする -> 描画に方向は関係ない
	if ( m_nG23 == 0 ) {	// G02 なら開始角度と終了角度を入れ替え
		double	dTmp = m_sq;
		m_sq = m_eq;
		m_eq = dTmp;
	}
	while ( m_sq >= m_eq )
		m_eq += 360.0*RAD;
}

void CNCcircle::DrawTuning(double f)
{
	m_dFactor = f;
}

void CNCcircle::DrawTuningXY(double f)
{
	m_dFactorXY = f;
}

void CNCcircle::DrawTuningXZ(double f)
{
	m_dFactorXZ = f;
}

void CNCcircle::DrawTuningYZ(double f)
{
	m_dFactorYZ = f;
}

void CNCcircle::Draw(CDC* pDC)
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCcircle::Draw()", DBG_RED);
	dbg.printf("Line=%d", GetStrLine());
#endif
	// 平面ごとの描画関数
	CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(NCPEN_G1));
	(this->*m_pfnCircleDraw)(NCCIRCLEDRAW_XYZ, pDC);
	pDC->SelectObject(pOldPen);
	// 中心座標(黄色)
	CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();
	if ( pOpt->IsDrawCircleCenter() )
		pDC->SetPixelV(m_ptOrg.PointConvert()*m_dFactor,
			pOpt->GetNcDrawColor(NCCOL_CENTERCIRCLE));
}

void CNCcircle::DrawXY(CDC* pDC)
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCcircle::DrawXY()", DBG_RED);
	dbg.printf("Line=%d", GetStrLine());
#endif
	CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(NCPEN_G1));
	(this->*m_pfnCircleDraw)(NCCIRCLEDRAW_XY, pDC);
	pDC->SelectObject(pOldPen);
	CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();
	if ( pOpt->IsDrawCircleCenter() )
		pDC->SetPixelV(m_ptOrg.GetXY()*m_dFactorXY,
			pOpt->GetNcDrawColor(NCCOL_CENTERCIRCLE));
}

void CNCcircle::DrawXZ(CDC* pDC)
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCcircle::DrawXZ()", DBG_RED);
	dbg.printf("Line=%d", GetStrLine());
#endif
	CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(NCPEN_G1));
	(this->*m_pfnCircleDraw)(NCCIRCLEDRAW_XZ, pDC);
	pDC->SelectObject(pOldPen);
	CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();
	if ( pOpt->IsDrawCircleCenter() )
		pDC->SetPixelV(m_ptOrg.GetXZ()*m_dFactorXZ,
			pOpt->GetNcDrawColor(NCCOL_CENTERCIRCLE));
}

void CNCcircle::DrawYZ(CDC* pDC)
{
#ifdef _DEBUGDRAW_NCD
	CMagaDbg	dbg("CNCcircle::DrawYZ()", DBG_RED);
	dbg.printf("Line=%d", GetStrLine());
#endif
	CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(NCPEN_G1));
	(this->*m_pfnCircleDraw)(NCCIRCLEDRAW_YZ, pDC);
	pDC->SelectObject(pOldPen);
	CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();
	if ( pOpt->IsDrawCircleCenter() )
		pDC->SetPixelV(m_ptOrg.GetYZ()*m_dFactorYZ,
			pOpt->GetNcDrawColor(NCCOL_CENTERCIRCLE));
}

// CDC::Arc() を使うとどうしても表示がズレる．
// 同一平面であっても微細線分による近似を行う
// 違う平面のときは，空間占有矩形 m_rcMax を利用して線分出力
void CNCcircle::Draw_G17(EN_NCCIRCLEDRAW enType, CDC* pDC)	// XY_PLANE
{
	double		q = m_sq, r = fabs(m_r), dFinalVal = m_ptValE.z;
	CPoint3D	pt3D, ptDrawOrg(m_ptOrg);
	CPointD		ptDraw, ptDraw2;

	switch ( enType ) {
	case NCCIRCLEDRAW_XYZ:
		r *= m_dFactor;
		ptDrawOrg *= m_dFactor;
		dFinalVal *= m_dFactor;
		pt3D.x = r * cos(q) + ptDrawOrg.x;
		pt3D.y = r * sin(q) + ptDrawOrg.y;
		pt3D.z = m_nG23==0 ? dFinalVal : ptDrawOrg.z;	// ﾍﾘｶﾙ開始座標
		ptDraw = pt3D.PointConvert();
		pDC->MoveTo(ptDraw);	// 開始点へ移動
		// ARCSTEP づつ微細線分で描画
		for ( q+=ARCSTEP; q<m_eq; q+=ARCSTEP ) {
			pt3D.x = r * cos(q) + ptDrawOrg.x;
			pt3D.y = r * sin(q) + ptDrawOrg.y;
			pt3D.z += m_dHelicalStep;
			ptDraw = pt3D.PointConvert();
			pDC->LineTo(ptDraw);
		}
		// 端数分描画
		pt3D.x = r * cos(m_eq) + ptDrawOrg.x;
		pt3D.y = r * sin(m_eq) + ptDrawOrg.y;
		pt3D.z = m_nG23==0 ? ptDrawOrg.z : dFinalVal;	// ﾍﾘｶﾙ終了座標
		ptDraw = pt3D.PointConvert();
		pDC->LineTo(ptDraw);
		break;

	case NCCIRCLEDRAW_XY:	// Don't ARC()
		r *= m_dFactorXY;
		ptDrawOrg *= m_dFactorXY;
		ptDraw.x = r * cos(q) + ptDrawOrg.x;
		ptDraw.y = r * sin(q) + ptDrawOrg.y;
		pDC->MoveTo(ptDraw);
		for ( q+=ARCSTEP; q<m_eq; q+=ARCSTEP ) {
			ptDraw.x = r * cos(q) + ptDrawOrg.x;
			ptDraw.y = r * sin(q) + ptDrawOrg.y;
			pDC->LineTo(ptDraw);
		}
		ptDraw.x = r * cos(m_eq) + ptDrawOrg.x;
		ptDraw.y = r * sin(m_eq) + ptDrawOrg.y;
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

void CNCcircle::Draw_G18(EN_NCCIRCLEDRAW enType, CDC* pDC)	// XZ_PLANE
{
	double		q = m_sq, r = fabs(m_r), dFinalVal = m_ptValE.y;
	CPoint3D	pt3D, ptDrawOrg(m_ptOrg);
	CPointD		ptDraw, ptDraw2;

	switch ( enType ) {
	case NCCIRCLEDRAW_XYZ:
		r *= m_dFactor;
		ptDrawOrg *= m_dFactor;
		dFinalVal *= m_dFactor;
		pt3D.x = r * cos(q) + ptDrawOrg.x;
		pt3D.y = m_nG23==0 ? dFinalVal : ptDrawOrg.y;
		pt3D.z = r * sin(q) + ptDrawOrg.z;
		ptDraw = pt3D.PointConvert();
		pDC->MoveTo(ptDraw);
		for ( q+=ARCSTEP; q<m_eq; q+=ARCSTEP ) {
			pt3D.x = r * cos(q) + ptDrawOrg.x;
			pt3D.y += m_dHelicalStep;
			pt3D.z = r * sin(q) + ptDrawOrg.z;
			ptDraw = pt3D.PointConvert();
			pDC->LineTo(ptDraw);
		}
		pt3D.x = r * cos(m_eq) + ptDrawOrg.x;
		pt3D.y = m_nG23==0 ? ptDrawOrg.y : dFinalVal;
		pt3D.z = r * sin(m_eq) + ptDrawOrg.z;
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
		ptDraw.x = r * cos(q) + ptDrawOrg.x;
		ptDraw.y = r * sin(q) + ptDrawOrg.z;
		pDC->MoveTo(ptDraw);
		for ( q+=ARCSTEP; q<m_eq; q+=ARCSTEP ) {
			ptDraw.x = r * cos(q) + ptDrawOrg.x;
			ptDraw.y = r * sin(q) + ptDrawOrg.z;
			pDC->LineTo(ptDraw);
		}
		ptDraw.x = r * cos(m_eq) + ptDrawOrg.x;
		ptDraw.y = r * sin(m_eq) + ptDrawOrg.z;
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

void CNCcircle::Draw_G19(EN_NCCIRCLEDRAW enType, CDC* pDC)	// YZ_PLANE
{
	double		q = m_sq, r = fabs(m_r), dFinalVal = m_ptValE.x;
	CPoint3D	pt3D, ptDrawOrg(m_ptOrg);
	CPointD		ptDraw, ptDraw2;

	switch ( enType ) {
	case NCCIRCLEDRAW_XYZ:
		r *= m_dFactor;
		ptDrawOrg *= m_dFactor;
		dFinalVal *= m_dFactor;
		pt3D.x = m_nG23==0 ? dFinalVal : ptDrawOrg.x;
		pt3D.y = r * cos(q) + ptDrawOrg.y;
		pt3D.z = r * sin(q) + ptDrawOrg.z;
		ptDraw = pt3D.PointConvert();
		pDC->MoveTo(ptDraw);
		for ( q+=ARCSTEP; q<m_eq; q+=ARCSTEP ) {
			pt3D.x += m_dHelicalStep;
			pt3D.y = r * cos(q) + ptDrawOrg.y;
			pt3D.z = r * sin(q) + ptDrawOrg.z;
			ptDraw = pt3D.PointConvert();
			pDC->LineTo(ptDraw);
		}
		pt3D.x = m_nG23==0 ? ptDrawOrg.x : dFinalVal;
		pt3D.y = r * cos(m_eq) + ptDrawOrg.y;
		pt3D.z = r * sin(m_eq) + ptDrawOrg.z;
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
		ptDraw.x = r * cos(q) + ptDrawOrg.y;
		ptDraw.y = r * sin(q) + ptDrawOrg.z;
		pDC->MoveTo(ptDraw);
		for ( q+=ARCSTEP; q<m_eq; q+=ARCSTEP ) {
			ptDraw.x = r * cos(q) + ptDrawOrg.y;
			ptDraw.y = r * sin(q) + ptDrawOrg.z;
			pDC->LineTo(ptDraw);
		}
		ptDraw.x = r * cos(m_eq) + ptDrawOrg.y;
		ptDraw.y = r * sin(m_eq) + ptDrawOrg.z;
		pDC->LineTo(ptDraw);
		break;
	}
}

void CNCcircle::SetMaxRect(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("SetMaxRect()", DBG_RED);
#endif
	// 外接する四角形(SetMaxRect)
	// ｱﾙｺﾞﾘｽﾞﾑは CDXFarc::Constracter() と同じ
	double	r = fabs(m_r), sq = m_sq, eq = m_eq;
	CRectD	rcMax, rcInit(r, r, -r, -r);

	// 始点・終点の開始位置
	// m_ptValS, m_ptValE を使うと，平面ごとの処理が必要なので
	// m_ptOrg を原点(0,0) とした始点終点を計算
	CPointD	pts(r*cos(sq), r*sin(sq));
	CPointD	pte(r*cos(eq), r*sin(eq));
	// ２点の矩形は必ず通るので，
	// 初期値として最大値・最小値を代入
	rcMax.left		= max(pts.x, pte.x);
	rcMax.top		= max(pts.y, pte.y);
	rcMax.right		= min(pts.x, pte.x);
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
		rcMax[a] = rcInit[a];
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
		m_rcMax.low		= GetValue(NCA_Z);
		m_rcMax.high	= m_ptOrg.z;
		break;
	case XZ_PLANE:
		rcMax.OffsetRect(m_ptOrg.GetXZ());
		m_rcMax.left	= rcMax.left;
		m_rcMax.top		= GetValue(NCA_Y);
		m_rcMax.right	= rcMax.right;
		m_rcMax.bottom	= m_ptOrg.y;
		m_rcMax.low		= rcMax.top;
		m_rcMax.high	= rcMax.bottom;
		break;
	case YZ_PLANE:
		rcMax.OffsetRect(m_ptOrg.GetYZ());
		m_rcMax.left	= GetValue(NCA_X);
		m_rcMax.top		= rcMax.left;
		m_rcMax.right	= m_ptOrg.x;
		m_rcMax.bottom	= rcMax.right;
		m_rcMax.low		= rcMax.top;
		m_rcMax.high	= rcMax.bottom;
		break;
	default:
		m_rcMax.SetRectEmpty();
		break;
	}
	m_rcMax.NormalizeRect();
#ifdef _DEBUG
	dbg.printf("m_rcMax(left, top   )=(%f, %f)", m_rcMax.left, m_rcMax.top);
	dbg.printf("m_rcMax(right,bottom)=(%f, %f)", m_rcMax.right, m_rcMax.bottom);
	dbg.printf("m_rcMax(high, low   )=(%f, %f)", m_rcMax.high, m_rcMax.low);
#endif
}

BOOL CNCcircle::CalcRoundPoint
	(const CNCdata* pNext, double r, CPointD& pt, double& rr1, double& rr2)
{
	// ２線の交点(自身の終点)が原点になるように補正
	CPointD	pts( GetPlaneValueOrg(m_ptValE, m_ptOrg) ), pte;
	double	xa, ya, r0 = fabs(m_r);

	if ( pNext->GetGcode() > 1 ) {
		CNCcircle*	pCircle = (CNCcircle *)pNext;
		int	nG23next = 1 - pCircle->GetG23();	// 交点への進入は反対回転
		double	r1, r2, rn = fabs(pCircle->GetR());
		pte = GetPlaneValueOrg(m_ptValE, pCircle->GetOrg());
		CPointD	p1, p2, pts_abs(fabs(pts.x), fabs(pts.y)), pte_abs(fabs(pte.x), fabs(pte.y));
		// 自身の円と他方の接線で自身の±r符号を計算
		if ( (r1=CalcRoundPoint_CircleInOut(pts, pte, m_nG23, nG23next, r)) == HUGE_VAL )
			return FALSE;
		// 他方の円と自身の接線で他方の±r符号を計算
		if ( (r2=CalcRoundPoint_CircleInOut(pte, pts, nG23next, m_nG23, r)) == HUGE_VAL )
			return FALSE;
		// ２つの円の交点を求める -> 解が２つないと面取り出来ないと判断する
		rr1 = r0 + r1;		rr2 = rn + r2;
		if ( ::CalcIntersectionPoint(pts, pte, rr1, rr2, &p1, &p2) != 2 )
			return FALSE;
		// 解の選択
		if ( (pts_abs.x<EPS && pte_abs.x<EPS) || (pts_abs.y<EPS && pte_abs.y<EPS) ||
				(pts_abs.x>EPS && pte_abs.x>EPS && fabs(pts.y/pts.x - pte.y/pte.x)<EPS) ) {
			// 中心が同一線上にあるとき，接線と符号が同じ方を選択
			if ( pts_abs.y < EPS )
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
		rr1 = sqrt(xa*xa + ya*ya);
		if ( r2 > 0 ) {
			xa = (pt.x*rn+pte.x*r2) / rr2;
			ya = (pt.y*rn+pte.y*r2) / rr2;
		}
		else {
			r2 = fabs(r2);
			xa = (pt.x*rn-pte.x*r2) / rr2;
			ya = (pt.y*rn-pte.y*r2) / rr2;
		}
		rr2 = sqrt(xa*xa + ya*ya);
	}
	else {
		pte = GetPlaneValueOrg(m_ptValE, pNext->GetEndPoint());
		// ｵﾌｾｯﾄ分平行移動させた交点を求める
		pt = ::CalcOffsetIntersectionPoint(pte, pts,	// 直線からのｱﾌﾟﾛｰﾁで
					r0, r, m_nG23==0 ? -1 : 1, 0, 0);	// 回転方向を反転
		if ( pt == HUGE_VAL )
			return FALSE;
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
		rr1 = sqrt(xa*xa + ya*ya);
		rr2 = sqrt(pt.x*pt.x + pt.y*pt.y - r*r);
	}

	// 原点補正
	pt += GetPlaneValue(m_ptValE);

	return TRUE;
}

BOOL CNCcircle::SetChamferingPoint(BOOL bStart, double c, CPointD& pt)
{
	CPoint3D	ptOrg3D( bStart ? m_ptValS : m_ptValE );
	CPointD		ptOrg1, ptOrg2, pt1, pt2;
	double		pa, pb, ps;

	switch ( GetPlane() ) {
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
	default:
		return FALSE;
	}

	// 長さが足らないときはｴﾗｰ
	if ( m_eq - m_sq > 180.0*RAD ) {
		// 180°を超えるときは直径と比較
		if ( c >= fabs(m_r)*2 )
			return FALSE;
	}
	else {
		// 180°未満の場合は弦の長さと比較
		if ( c >= hypot(pt1.x-pt2.x, pt1.y-pt2.y) )
			return FALSE;
	}

	// ２つの円の交点を求める -> 解が２つないと面取り出来ないと判断する
	if ( ::CalcIntersectionPoint(ptOrg1, ptOrg2, fabs(m_r), c, &pt1, &pt2) != 2 )
		return FALSE;

	// 時計回りの場合，始角と終角が入れ替わっているので一時的に元に戻す
	if ( m_nG23 == 0 ) {
		ps = m_sq;
		m_sq = m_eq;
		m_eq = ps;
	}

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
		switch ( GetPlane() ) {
		case XY_PLANE:
			m_ptValS.x = pt.x;
			m_ptValS.y = pt.y;
			break;
		case XZ_PLANE:
			m_ptValS.x = pt.x;
			m_ptValS.z = pt.y;
			break;
		case YZ_PLANE:
			m_ptValS.y = pt.x;
			m_ptValS.z = pt.y;
			break;
		}
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
		switch ( GetPlane() ) {
		case XY_PLANE:
			m_ptValE.x = pt.x;
			m_ptValE.y = pt.y;
			break;
		case XZ_PLANE:
			m_ptValE.x = pt.x;
			m_ptValE.z = pt.y;
			break;
		case YZ_PLANE:
			m_ptValE.y = pt.x;
			m_ptValE.z = pt.y;
			break;
		}
		m_eq = pa;
		m_pt2D = m_ptValE.PointConvert();
	}

	// 角度補正
	if ( m_nG23 == 0 ) {
		ps = m_sq;
		m_sq = m_eq;
		m_eq = ps;
	}
	while ( m_sq >= m_eq )
		m_eq += 360.0*RAD;

	// 切削長計算
	CalcLength();

	return TRUE;
}

double CNCcircle::CalcBetweenAngle(const CPoint3D&, const CNCdata* pNext)
{
	// ２線の交点(自身の終点)が原点になるように補正
	CPoint3D	pto(GetValue(NCA_X), GetValue(NCA_Y), GetValue(NCA_Z));
	CPointD		pt( GetPlaneValueOrg(pto, m_ptOrg) ), pt1, pt2;

	// 接線計算
	int k = m_nG23==0 ? -1 : 1;	// 終点では回転方向を反転
	pt1.x = -pt.y*k;	// G02:-90°
	pt1.y =  pt.x*k;	// G03:+90°

	if ( pNext->GetGcode() > 1 ) {
		CNCcircle*	pCircle = (CNCcircle *)pNext;
		// 次のｵﾌﾞｼﾞｪｸﾄが円弧なら中心をｾｯﾄし
		pt = GetPlaneValueOrg(pto, pCircle->GetOrg());
		// 接線計算
		k = pCircle->GetG23()==0 ? 1 : -1;
		pt2.x = -pt.y*k;
		pt2.y =  pt.x*k;
	}
	else {
		CPoint3D	pte(pNext->GetValue(NCA_X), pNext->GetValue(NCA_Y), pNext->GetValue(NCA_Z));
		pt2 = GetPlaneValueOrg(pto, pte);
	}

	// ２線(または円弧の接線)がなす角度を求める
	return ::CalcBetweenAngleTwoLines(pt1, pt2);
}

int CNCcircle::CalcOffsetSign(const CPoint3D&)
{
	// 回転方向からG41の補正符号を決める
	return m_nG23==0 ? 1 : -1;
}

CPointD CNCcircle::CalcPerpendicularPoint
	(const CPoint3D& pts, const CPoint3D&, double r, int)
{
	// 始点終点関係なくCalcOffsetSign()方向
	// ptsと中心の傾きを計算して半径±r
	CPointD	pt( GetPlaneValueOrg(m_ptOrg, pts) );
	double	q = atan2(pt.y, pt.x), rr = fabs(m_r) + r * CalcOffsetSign(CPoint3D());
	CPointD	pt1(rr*cos(q), rr*sin(q));
	pt1 += GetPlaneValue(m_ptOrg);

	return pt1;
}

CPointD CNCcircle::CalcOffsetIntersectionPoint
	(const CPoint3D& pts, const CNCdata* pNext, double r, int k1, int k2)
{
	// ２線の交点(自身の終点)が原点になるように補正
	CPoint3D	pto(GetValue(NCA_X), GetValue(NCA_Y), GetValue(NCA_Z));
	CPointD		pt;

	if ( pNext->GetGcode() > 1 ) {
		CNCcircle*	pCircle = (CNCcircle *)pNext;
		CPointD	pto1( GetPlaneValueOrg(pto, m_ptOrg) ),
				pto2( GetPlaneValueOrg(pto, pCircle->GetOrg()) );
		// ｵﾌｾｯﾄ分半径を調節して２つの円の交点を求める
		CPointD	p1, p2;
		int	n = ::CalcIntersectionPoint(pto1, pto2,
					fabs(m_r)+r*k1, fabs(pCircle->GetR())+r*k2, &p1, &p2);
		// 解の選択
		pt = p1.x*p1.x+p1.y*p1.y < p2.x*p2.x+p2.y*p2.y ? p1 : p2;
	}
	else {
		CPoint3D	pte(pNext->GetValue(NCA_X), pNext->GetValue(NCA_Y), pNext->GetValue(NCA_Z));
		// ｵﾌｾｯﾄ分平行移動させた交点を求める
		CPointD	pt1( GetPlaneValueOrg(pto, pte) ),
				pt2( GetPlaneValueOrg(pto, m_ptOrg) );
		pt = ::CalcOffsetIntersectionPoint(pt1, pt2,			// 線からのｱﾌﾟﾛｰﾁで
					fabs(m_r), r, m_nG23==0 ? -1 : 1, k2, k1);	// 回転方向を反転
	}

	// 原点補正
	if ( pt != HUGE_VAL )
		pt += pto;

	return pt;
}

CPointD CNCcircle::CalcOffsetIntersectionPoint2
	(const CPoint3D& pts, const CNCdata* pNext, double r, int k1, int k2)
{
	int		k;
	double	dAngle;
	// ２線の交点(自身の終点)が原点になるように補正
	CPoint3D	pto(GetValue(NCA_X), GetValue(NCA_Y), GetValue(NCA_Z));
	CPointD		pt, pt1, pt2;

	// 自身の接線座標
	pt = GetPlaneValueOrg(pto, m_ptOrg);
	k = m_nG23==0 ? -1 : 1;
	pt1.x = -pt.y * k;
	pt1.y =  pt.x * k;
	// 接線用に符号を再計算
	if ( (dAngle=atan2(pt1.y, pt1.x)) < 0.0 )
		dAngle += 360.0*RAD;
	k1 = 90.0*RAD<=dAngle && dAngle<270.0*RAD ? 1 : -1;		// 符号反転

	// 他方の座標計算
	if ( pNext->GetGcode() > 1 ) {
		CNCcircle*	pCircle = (CNCcircle *)pNext;
		pt = GetPlaneValueOrg(pto, pCircle->GetOrg());
		int k = pCircle->GetG23()==0 ? 1 : -1;
		pt2.x = -pt.y * k;
		pt2.y =  pt.x * k;
		// 接線用に符号を再計算
		if ( (dAngle=atan2(pt2.y, pt2.x)) < 0.0 )
			dAngle += 360.0*RAD;
		k2 = 90.0*RAD<=dAngle && dAngle<270.0*RAD ? -1 : 1;
	}
	else {
		CPoint3D	pte(pNext->GetValue(NCA_X), pNext->GetValue(NCA_Y), pNext->GetValue(NCA_Z));
		pt2 = GetPlaneValueOrg(pto, pte);
	}

	// 直線同士のｵﾌｾｯﾄ交点計算
	pt = ::CalcOffsetIntersectionPoint(pt1, pt2, k1, k2, r);
	// 原点補正
	if ( pt != HUGE_VAL )
		pt += pto;

	return pt;
}

BOOL CNCcircle::SetCorrectPoint(BOOL bStart, const CPointD& ptSrc, double rr)
{
	CPoint3D&	ptVal = bStart ? m_ptValS : m_ptValE;
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
	default:
		return FALSE;
	}

	if ( bStart ) {
		double&	q = m_nG23==0 ? m_eq : m_sq;
		if ( (q=atan2(pt.y-ptOrg.y, pt.x-ptOrg.x)) < 0.0 )
			q += 360.0*RAD;
	}
	else {
		m_r += rr;	// 終点の時だけ半径補正
		double&	q = m_nG23==0 ? m_sq : m_eq;
		if ( (q=atan2(pt.y-ptOrg.y, pt.x-ptOrg.x)) < 0.0 )
			q += 360.0*RAD;
		m_pt2D = m_ptValE.PointConvert();
	}
	while ( m_sq >= m_eq )
		m_eq += 360.0*RAD;
	CalcLength();

	return TRUE;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

// 円同士の内外半径計算
double CalcRoundPoint_CircleInOut
	(const CPointD& pts, const CPointD& pte, int nG23, int nG23next, double r)
{
	CPointD	pto, pts_abs(fabs(pts.x), fabs(pts.y)), pte_abs(fabs(pte.x), fabs(pte.y));
	double	rr;
	int		k1 = nG23==0 ? -1 : 1, k2 = nG23next==0 ? -1 : 1;

	// 特殊解の判断
	if ( (pts_abs.x<EPS && pte_abs.x<EPS && pts.y*pte.y>0) ||
			(pts_abs.y<EPS && pte_abs.y<EPS && pts.x*pte.x>0) ||
			(pts_abs.x>EPS && pte_abs.x>EPS && fabs(pts.y/pts.x - pte.y/pte.x)<EPS && pts.x*pte.x>0) ) {
		// 中心が同一線上にあり，かつ，x,y の符号が同じとき
		double	l1 = pts.x*pts.x + pts.y*pts.y;
		double	l2 = pte.x*pte.x + pte.y*pte.y;
		if ( fabs(l1 - l2) < EPS )	// 距離が等しい==同軌跡円
			return HUGE_VAL;
		else if ( l1 > l2 )
			return -r;
		else
			return r;
	}
	// 自身の円と他方の接線で自身の±r符号を計算
	pto.x = -pte.y*k2;	// G02:-90°
	pto.y =  pte.x*k2;	// G03:+90°

	// 線と円弧の場合と考え方(処理方法)は同じ
	if ( fabs(pto.x) < EPS && pts_abs.y < EPS )
		rr = _copysign(r, pto.y*pts.x*k1);
	else if ( fabs(pto.y) < EPS && pts_abs.x < EPS )
		rr = _copysign(r, -pto.x*pts.y*k1);
	else
		rr = _copysign(r, -(pto.x*pts.x + pto.y*pts.y));

	return rr;
}
