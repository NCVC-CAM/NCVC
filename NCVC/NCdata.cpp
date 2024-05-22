// NCdata.cpp: CNCdata クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "NCdata.h"
#include "ViewOption.h"

//#define	_DEBUGDRAW_NCD		// 描画処理もﾛｸﾞ
#ifdef _DEBUG
#define new DEBUG_NEW
#ifdef _DEBUG_DUMP
#include "boost/format.hpp"
using std::string;
#endif
#endif

using namespace boost;
using namespace boost::core;

extern	const	DWORD		g_dwSetValFlags[];	// NCDoc.cpp

// CalcRoundPoint() ｻﾌﾞ
// --- ｵﾌｾｯﾄ方向の決定
static	int		_CalcRoundPoint_OffsetFlag(const CPointF&, const CPointF&, BOOL);
// --- 円同士の内外半径計算
static	optional<float>	_CalcRoundPoint_CircleInOut(const CPointF&, const CPointF&, BOOL, BOOL, float);

//////////////////////////////////////////////////////////////////////

G68ROUND_F::G68ROUND_F(const G68ROUND_F* pG68) {
	enPlane	= pG68->enPlane;
	dRound	= pG68->dRound;
	for ( int i=0; i<SIZEOF(dOrg); i++ )
		dOrg[i]	= pG68->dOrg[i];
}

G68ROUND_F::G68ROUND_F(const G68ROUND& G68)
{
	enPlane	= G68.enPlane;
	dRound	= (float)G68.dRound;
	for ( int i=0; i<SIZEOF(dOrg); i++ )
		dOrg[i]	= (float)G68.dOrg[i];
}

TAPER_F::TAPER_F(const TAPER_F* pTP)
{
	nTaper	= pTP->nTaper;
	dTaper	= pTP->dTaper;
	nDiff	= pTP->nDiff;
	bTonly	= pTP->bTonly;
}

TAPER_F::TAPER_F(const TAPER& TP)
{
	nTaper	= TP.nTaper;
	dTaper	= (float)TP.dTaper;
	nDiff	= TP.nDiff;
	bTonly	= TP.bTonly;
}

CNCread::~CNCread()
{
	if ( m_pG68 )
		delete	m_pG68;
	if ( m_pTaper )
		delete	m_pTaper;
}

//////////////////////////////////////////////////////////////////////
// NCﾃﾞｰﾀの基礎ﾃﾞｰﾀｸﾗｽ
//////////////////////////////////////////////////////////////////////

// 初回登録用ｺﾝｽﾄﾗｸﾀ
CNCdata::CNCdata(LPNCARGV lpArgv)
{
	int		i;

	Constracter(lpArgv);

	for ( i=0; i<VALUESIZE; i++ )
		m_nc.dValue[i] = (float)lpArgv->nc.dValue[i];
	for ( i=0; i<NCXYZ; i++ )
		m_pRead->m_ptValOrg[i] = m_ptValS[i] = m_ptValE[i] = (float)lpArgv->nc.dValue[i];
	m_pt2D = m_ptValE.PointConvert();

	m_enType = NCDBASEDATA;
}

// 切削(描画)ｺｰﾄﾞ以外のｺﾝｽﾄﾗｸﾀ
CNCdata::CNCdata(const CNCdata* pData, LPNCARGV lpArgv, const CPoint3F& ptOffset)
{
	int		i;

	Constracter(lpArgv);

	// 座標指定のないﾃﾞｰﾀは前回計算座標から取得
	for ( i=0; i<NCXYZ; i++ ) {
		// 指定されている分だけ代入(XYZのみ)
		m_nc.dValue[i] = lpArgv->nc.dwValFlags & g_dwSetValFlags[i] ?
			(float)lpArgv->nc.dValue[i] : pData->GetOriginalEndValue(i);
	}
	// 座標値以外(UVW含む)も指定されている分は代入
	for ( ; i<VALUESIZE; i++ )
		m_nc.dValue[i] = lpArgv->nc.dwValFlags & g_dwSetValFlags[i] ?
//			lpArgv->nc.dValue[i] : pData->GetValue(i);
			(float)lpArgv->nc.dValue[i] : 0.0f;

	m_pRead->m_ptOffset = ptOffset;
	if ( m_nc.nGcode == 92 ) {
		// NCDocのｵﾌｾｯﾄ(m_ptNcWorkOrg)で処理されるのでG92指定値をｾｯﾄ
		m_pRead->m_ptValOrg.SetPoint(GetValue(NCA_X), GetValue(NCA_Y), GetValue(NCA_Z));
		m_ptValS = m_ptValE = m_pRead->m_ptValOrg + ptOffset;
		m_pt2D = m_ptValE.PointConvert();
	}
	else {
		if ( lpArgv->nc.dwValFlags & (NCD_X|NCD_Y|NCD_Z) ) {
			m_pRead->m_ptValOrg.SetPoint(GetValue(NCA_X), GetValue(NCA_Y), GetValue(NCA_Z));
			m_ptValS = pData->GetEndPoint();
			m_ptValE = m_pRead->m_ptValOrg + ptOffset;
			if ( lpArgv->g68.bG68 )
				CalcG68Round(&(lpArgv->g68), m_ptValE);
			m_pt2D = m_ptValE.PointConvert();
		}
		else {
			m_pRead->m_ptValOrg = pData->GetOriginalEndPoint();
			m_ptValS = m_ptValE = pData->GetEndPoint();
			m_pt2D   = pData->Get2DPoint();
		}
	}

	m_enType = NCDBASEDATA;

#ifdef _DEBUG_DUMP
	DbgDump();
	Dbg_sep();
#endif
}

// 派生ｸﾗｽ用ｺﾝｽﾄﾗｸﾀ
CNCdata::CNCdata
	(ENNCDTYPE enType, const CNCdata* pData, LPNCARGV lpArgv, const CPoint3F& ptOffset)
{
	int		i;

	Constracter(lpArgv);

	// 座標指定のないﾃﾞｰﾀは前回純粋座標から補間
	for ( i=0; i<NCXYZ; i++ ) {
		// 指定されている分だけ代入(XYZのみ)
		if ( lpArgv->nc.dwValFlags & g_dwSetValFlags[i] ) {
			m_nc.dValue[i] = (float)lpArgv->nc.dValue[i];
			if ( !lpArgv->bAbs )		// ｲﾝｸﾘﾒﾝﾀﾙ補正
				m_nc.dValue[i] += pData->GetOriginalEndValue(i);	// ｵﾘｼﾞﾅﾙ値で加算
		}
		else
			m_nc.dValue[i] = pData->GetOriginalEndValue(i);
	}
	// 上記以外
	for ( ; i<VALUESIZE; i++ ) {
		m_nc.dValue[i] = lpArgv->nc.dwValFlags & g_dwSetValFlags[i] ?
//			lpArgv->nc.dValue[i] : pData->GetValue(i);
			(float)lpArgv->nc.dValue[i] : 0.0f;
	}
	// ---------------------------------------------------------------
	// m_ptValS, m_ptValE, m_ptValOrg, m_pt2D は派生ｸﾗｽで代入
	// m_nc.dLength は TH_Cuttime.cpp にてセット
	// ---------------------------------------------------------------
	m_enType = enType;
	m_pRead->m_ptOffset = ptOffset;
}

// 複製用ｺﾝｽﾄﾗｸﾀ
CNCdata::CNCdata(const CNCdata* pData)
{
	int		i;
	m_enType		= pData->GetType();
	m_dwFlags		= pData->GetFlags();
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
	m_dEndmill	= pData->GetEndmill();
	m_ptValS = pData->GetStartPoint();
	m_ptValE = pData->GetEndPoint();
	m_pRead = new CNCread;
	m_pRead->m_ptOffset = pData->GetOffsetPoint();
	m_pRead->m_ptValOrg = pData->GetOriginalEndPoint();
//	memcpy(&(m_pRead->m_g68),   &(pData->GetReadData()->m_g68),   sizeof(G68ROUND));
//	memcpy(&(m_pRead->m_taper), &(pData->GetReadData()->m_taper), sizeof(TAPER));
	if ( pData->GetReadData()->m_pG68 )
		m_pRead->m_pG68 = new G68ROUND_F(pData->GetReadData()->m_pG68);
	else
		m_pRead->m_pG68 = NULL;
	if ( pData->GetReadData()->m_pTaper )
		m_pRead->m_pTaper = new TAPER_F(pData->GetReadData()->m_pTaper);
	else
		m_pRead->m_pTaper = NULL;
	m_pWireObj = NULL;
}

CNCdata::~CNCdata()
{
	for ( int i=0; i<m_obCdata.GetSize(); i++ )
		delete	m_obCdata[i];
	if ( m_pRead )
		delete	m_pRead;
	if ( m_pWireObj )
		delete	m_pWireObj;
}

void CNCdata::DeleteReadData(void)
{
	for ( int i=0; i<m_obCdata.GetSize(); i++ )
		m_obCdata[i]->DeleteReadData();
	if ( m_pRead ) {
		delete	m_pRead;
		m_pRead = NULL;
	}
	if ( m_pWireObj )
		m_pWireObj->DeleteReadData();
}

CPointF CNCdata::GetPlaneValue(const CPoint3F& ptVal) const
{
	CPointF	pt;
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

void CNCdata::SetPlaneValue(const CPointF& pt, CPoint3F& ptResult) const
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

CPointF	CNCdata::GetPlaneValueOrg(const CPoint3F& pt1, const CPoint3F& pt2) const
{
	CPointF	pt;
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

void CNCdata::CalcG68Round(LPG68ROUND lpG68, CPoint3F& ptResult) const
{
	CPoint3F	ptOrg((float)lpG68->dOrg[NCA_X], (float)lpG68->dOrg[NCA_Y], (float)lpG68->dOrg[NCA_Z]);
	CPointF		pt(GetPlaneValueOrg(ptResult, ptOrg));
	pt.RoundPoint((float)lpG68->dRound);
	SetPlaneValue(pt, ptResult);
	ptResult += ptOrg;
}

void CNCdata::CalcG68Round(LPG68ROUND_F lpG68, CPoint3F& ptResult) const
{
	CPoint3F	ptOrg(lpG68->dOrg[NCA_X], lpG68->dOrg[NCA_Y], lpG68->dOrg[NCA_Z]);
	CPointF		pt(GetPlaneValueOrg(ptResult, ptOrg));
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

void CNCdata::DrawTuning(float f)
{
	for ( int i=0; i<m_obCdata.GetSize(); i++ )
		m_obCdata[i]->DrawTuning(f);
	if ( m_pWireObj )
		m_pWireObj->DrawTuning(f);
}

void CNCdata::DrawTuningXY(float f)
{
	for ( int i=0; i<m_obCdata.GetSize(); i++ )
		m_obCdata[i]->DrawTuningXY(f);
	if ( m_pWireObj )
		m_pWireObj->DrawTuningXY(f);
}

void CNCdata::DrawTuningXZ(float f)
{
	for ( int i=0; i<m_obCdata.GetSize(); i++ )
		m_obCdata[i]->DrawTuningXZ(f);
	if ( m_pWireObj )
		m_pWireObj->DrawTuningXZ(f);
}

void CNCdata::DrawTuningYZ(float f)
{
	for ( int i=0; i<m_obCdata.GetSize(); i++ )
		m_obCdata[i]->DrawTuningYZ(f);
	if ( m_pWireObj )
		m_pWireObj->DrawTuningYZ(f);
}

void CNCdata::Draw(CDC* pDC, BOOL bSelect) const
{
	for ( int i=0; i<m_obCdata.GetSize(); i++ )
		m_obCdata[i]->Draw(pDC, bSelect);
}

void CNCdata::DrawXY(CDC* pDC, BOOL bSelect) const
{
	for ( int i=0; i<m_obCdata.GetSize(); i++ )
		m_obCdata[i]->DrawXY(pDC, bSelect);
}

void CNCdata::DrawXZ(CDC* pDC, BOOL bSelect) const
{
	for ( int i=0; i<m_obCdata.GetSize(); i++ )
		m_obCdata[i]->DrawXZ(pDC, bSelect);
}

void CNCdata::DrawYZ(CDC* pDC, BOOL bSelect) const
{
	for ( int i=0; i<m_obCdata.GetSize(); i++ )
		m_obCdata[i]->DrawYZ(pDC, bSelect);
}

void CNCdata::DrawWire(CDC* pDC, BOOL bSelect) const
{
}

void CNCdata::DrawWireXY(CDC* pDC, BOOL bSelect) const
{
}

void CNCdata::DrawWireXZ(CDC* pDC, BOOL bSelect) const
{
}

void CNCdata::DrawWireYZ(CDC* pDC, BOOL bSelect) const
{
}

CPoint CNCdata::GetDrawStartPoint(size_t n) const
{
	ASSERT( FALSE );	// ｴﾗｰ
	return CPoint();
}

CPoint CNCdata::GetDrawEndPoint(size_t n) const
{
	ASSERT( FALSE );
	return CPoint();
}

void CNCdata::DrawWireLine(ENNCDRAWVIEW enDraw, CDC* pDC, BOOL bSelect) const
{
	ASSERT( FALSE );
}

#ifdef _DEBUG_DUMP
void CNCdata::DbgDump(void)
{
	extern	LPCTSTR	g_szGdelimiter;	// "GSMOF" from NCDoc.cpp
	extern	LPCTSTR	g_szNdelimiter; // "XYZUVWIJKRPLDH";

	string	strBuf;
	if ( GetGtype()<0 || GetGtype()>GTYPESIZE )
		strBuf = "NO_TYPE:" + lexical_cast<string>(GetGcode());
	else
		strBuf = str(format("%c%02d") % g_szGdelimiter[GetGtype()] % GetGcode());
	for ( int i=0; i<VALUESIZE; i++ ) {
		if ( GetValFlags() & g_dwSetValFlags[i] )
			strBuf += str(format("%c%.3f") % g_szNdelimiter[i] % GetValue(i));
	}
	printf("CNCdata %s", strBuf.c_str());
}
#endif

//////////////////////////////////////////////////////////////////////
// CNCline クラス
//////////////////////////////////////////////////////////////////////

CNCline::CNCline(const CNCdata* pData, LPNCARGV lpArgv, const CPoint3F& ptOffset) :
	CNCdata(NCDLINEDATA, pData, lpArgv, ptOffset)
{
	// 描画始点を前回の計算値から取得
	m_ptValS = pData->GetEndPoint();
	m_pt2Ds  = pData->Get2DPoint();
	// 最終座標(==指定座標)ｾｯﾄ
	m_pRead->m_ptValOrg.SetPoint(GetValue(NCA_X), GetValue(NCA_Y), GetValue(NCA_Z));
	m_ptValE = m_pRead->m_ptValOrg + ptOffset;
	// 座標回転
	if ( lpArgv->g68.bG68 )
		CalcG68Round(&(lpArgv->g68), m_ptValE);
	// 描画終点を計算し保存
	m_pt2D = m_ptValE.PointConvert();

#ifdef _DEBUG_DUMP
	DbgDump();
	Dbg_sep();
#endif
}

CNCline::CNCline(const CNCdata* pData) : CNCdata(pData)
{
	m_pt2Ds	= m_ptValS.PointConvert();
	m_pt2D	= m_ptValE.PointConvert();
}

float CNCline::SetCalcLength(void)
{
	CPoint3F	pt;

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
		CNCdata*	pData;
		m_nc.dLength = 0.0f;
		ZEROCLR(m_dMove);
		// 各補正要素の合計
		for ( int i=0; i<m_obCdata.GetSize(); i++ ) {
			pData = m_obCdata[i];
			pt = pData->GetEndPoint() - pData->GetStartPoint();
			// DrawBottomFace() で移動量が必要なため
			// 補正ﾃﾞｰﾀに対しても m_dMove をｾｯﾄ
			m_dMove[NCA_X] += pData->SetMove(NCA_X, fabs(pt.x));
			m_dMove[NCA_Y] += pData->SetMove(NCA_Y, fabs(pt.y));
			m_dMove[NCA_Z] += pData->SetMove(NCA_Z, fabs(pt.z));
			m_nc.dLength += pt.hypot();
		}
	}

	float	dResult = 0;
	if ( m_pWireObj )
		dResult = m_pWireObj->SetCalcLength();

	return max((float)m_nc.dLength, dResult);
}

void CNCline::DrawTuning(float f)
{
	m_ptDrawS[NCDRAWVIEW_XYZ] = m_pt2Ds * f;
	m_ptDrawE[NCDRAWVIEW_XYZ] = m_pt2D  * f;
	CNCdata::DrawTuning(f);
}

void CNCline::DrawTuningXY(float f)
{
	m_ptDrawS[NCDRAWVIEW_XY] = m_ptValS.GetXY() * f;
	m_ptDrawE[NCDRAWVIEW_XY] = m_ptValE.GetXY() * f;
	CNCdata::DrawTuningXY(f);
}

void CNCline::DrawTuningXZ(float f)
{
	m_ptDrawS[NCDRAWVIEW_XZ] = m_ptValS.GetXZ() * f;
	m_ptDrawE[NCDRAWVIEW_XZ] = m_ptValE.GetXZ() * f;
	CNCdata::DrawTuningXZ(f);
}

void CNCline::DrawTuningYZ(float f)
{
	m_ptDrawS[NCDRAWVIEW_YZ] = m_ptValS.GetYZ() * f;
	m_ptDrawE[NCDRAWVIEW_YZ] = m_ptValE.GetYZ() * f;
	CNCdata::DrawTuningYZ(f);
}

void CNCline::Draw(CDC* pDC, BOOL bSelect) const
{
#ifdef _DEBUGDRAW_NCD
	printf("Line Draw()=%d\n", GetBlockLineNo()+1);
#endif
	if ( m_obCdata.IsEmpty() ||
			AfxGetNCVCApp()->GetViewOption()->GetNCViewFlg(GLOPTFLG_DRAWREVISE) )
		DrawLine(NCDRAWVIEW_XYZ, pDC, bSelect);
	CNCdata::Draw(pDC, bSelect);
}

void CNCline::DrawXY(CDC* pDC, BOOL bSelect) const
{
#ifdef _DEBUGDRAW_NCD
	printf("Line DrawXY()=%d\n", GetBlockLineNo()+1);
#endif
	if ( m_obCdata.IsEmpty() ||
			AfxGetNCVCApp()->GetViewOption()->GetNCViewFlg(GLOPTFLG_DRAWREVISE) )
		DrawLine(NCDRAWVIEW_XY, pDC, bSelect);
	CNCdata::DrawXY(pDC, bSelect);
}

void CNCline::DrawXZ(CDC* pDC, BOOL bSelect) const
{
#ifdef _DEBUGDRAW_NCD
	printf("Line DrawXZ()=%d\n", GetBlockLineNo()+1);
#endif
	if ( m_obCdata.IsEmpty() ||
			AfxGetNCVCApp()->GetViewOption()->GetNCViewFlg(GLOPTFLG_DRAWREVISE) )
		DrawLine(NCDRAWVIEW_XZ, pDC, bSelect);
	CNCdata::DrawXZ(pDC, bSelect);
}

void CNCline::DrawYZ(CDC* pDC, BOOL bSelect) const
{
#ifdef _DEBUGDRAW_NCD
	printf("Line DrawYZ()=%d\n", GetBlockLineNo()+1);
#endif
	if ( m_obCdata.IsEmpty() ||
			AfxGetNCVCApp()->GetViewOption()->GetNCViewFlg(GLOPTFLG_DRAWREVISE) )
		DrawLine(NCDRAWVIEW_YZ, pDC, bSelect);
	CNCdata::DrawYZ(pDC, bSelect);
}

void CNCline::DrawWire(CDC* pDC, BOOL bSelect) const
{
	DrawWireLine(NCDRAWVIEW_XYZ, pDC, bSelect);
}

void CNCline::DrawWireXY(CDC* pDC, BOOL bSelect) const
{
	DrawWireLine(NCDRAWVIEW_XY, pDC, bSelect);
}

void CNCline::DrawWireXZ(CDC* pDC, BOOL bSelect) const
{
	DrawWireLine(NCDRAWVIEW_XZ, pDC, bSelect);
}

void CNCline::DrawWireYZ(CDC* pDC, BOOL bSelect) const
{
	DrawWireLine(NCDRAWVIEW_YZ, pDC, bSelect);
}

void CNCline::DrawLine(ENNCDRAWVIEW enDraw, CDC* pDC, BOOL bSelect) const
{
	CPen*	pOldPen;
	if ( bSelect )
		pOldPen = AfxGetNCVCMainWnd()->GetPenCom(COMPEN_SEL);
	else
		pOldPen = AfxGetNCVCMainWnd()->GetPenNC(
			m_obCdata.IsEmpty() ? GetPenType() : NCPEN_CORRECT );
	pOldPen = pDC->SelectObject(pOldPen);
	pDC->MoveTo(m_ptDrawS[enDraw]);
	pDC->LineTo(m_ptDrawE[enDraw]);
	pDC->SelectObject(pOldPen);
}

void CNCline::DrawWireLine(ENNCDRAWVIEW enDraw, CDC* pDC, BOOL bSelect) const
{
	CPen*	pOldPen;
	if ( bSelect )
		pOldPen = AfxGetNCVCMainWnd()->GetPenCom(COMPEN_SEL);
	else
		pOldPen = AfxGetNCVCMainWnd()->GetPenNC(
			m_obCdata.IsEmpty() ? GetPenType() : NCPEN_CORRECT );
	pOldPen = pDC->SelectObject(pOldPen);
	// XY
	pDC->MoveTo(m_ptDrawS[enDraw]);
	pDC->LineTo(m_ptDrawE[enDraw]);
	// UV
	if ( m_pWireObj ) {
		// m_pWireObjがCNClineとは限らない
		m_pWireObj->DrawWireLine(enDraw, pDC, bSelect);
		// XYとUVの接続
		pDC->MoveTo(m_ptDrawS[enDraw]);
		pDC->LineTo(m_pWireObj->GetDrawStartPoint(enDraw));
		pDC->MoveTo(m_ptDrawE[enDraw]);
		pDC->LineTo(m_pWireObj->GetDrawEndPoint(enDraw));
	}
	pDC->SelectObject(pOldPen);
}

tuple<BOOL, CPointF, float, float> CNCline::CalcRoundPoint(const CNCdata* pNext, float r) const
{
	BOOL		bResult = FALSE;
	float		rr1, rr2;
	CPointF		pt, pts;

	if ( pNext->GetType() == NCDARCDATA ) {
		const CNCcircle* pCircle = static_cast<const CNCcircle *>(pNext);
		// 計算原点補正
		pts = GetPlaneValueOrg(m_ptValS, m_ptValE);
		CPointF	ptc( GetPlaneValueOrg(pCircle->GetOrg(), m_ptValE) );
		// ｵﾌｾｯﾄ方向を決定
		int nOffset = _CalcRoundPoint_OffsetFlag(pts, ptc, pCircle->GetG03());
		if ( nOffset != 0 ) {
			// ｵﾌｾｯﾄ分平行移動させた交点を求める
			float	rr, xa, ya, rn = fabs(pCircle->GetR());
			optional<CPointF> ptResult = ::CalcOffsetIntersectionPoint_LC(pts, ptc, rn, r, r,
							pCircle->GetG03(), nOffset>0);
			if ( ptResult ) {
				pt = *ptResult;
				// 面取りに相当するC値の計算
				rr1 = sqrt(pt.x*pt.x + pt.y*pt.y - r*r);
				if ( nOffset > 0 )
					nOffset = pCircle->GetG03() ? -1 : 1;
				else
					nOffset = pCircle->GetG03() ? 1 : -1;
				rr = rn + r*nOffset;
				if ( nOffset > 0 ) {
					// 内分点(+r)
					xa = (pt.x*rn+ptc.x*r) / rr;
					ya = (pt.y*rn+ptc.y*r) / rr;
				}
				else {
					// 外分点(-r)
					xa = (pt.x*rn-ptc.x*r) / rr;
					ya = (pt.y*rn-ptc.y*r) / rr;
				}
				rr2 = _hypotf(xa, ya);
				bResult = TRUE;
			}
		}
	}
	else {
		// 計算原点補正
		pts = GetPlaneValueOrg(m_ptValS, m_ptValE);
		CPointF	pte( GetPlaneValueOrg(pNext->GetEndPoint(), m_ptValE) );
		// pts を X軸上に回転
		float	q = pts.arctan();
		pte.RoundPoint(-q);
		// ２つの線の角度÷２
		float	p = pte.arctan() / 2.0f,
				pp = fabs(p);
		if ( pp < RAD(90.0f) ) {
			pt.x = rr1 = rr2 = r / tan(pp);	// 面取りに相当するC値は回転復元前のX座標と同じ
			pt.y = copysign(r, p);			// y(高さ) = r、符号は角度による
			// 回転を復元
			pt.RoundPoint(q);
			bResult = TRUE;
		}
	}

	if ( bResult )
		pt += GetPlaneValue(m_ptValE);

	return make_tuple(bResult, pt, rr1, rr2);
}

optional<CPointF> CNCline::SetChamferingPoint(BOOL bStart, float c)
{
	// 長さが足らないときはｴﾗｰ
	if ( c >= SetCalcLength() )
		return boost::none;

	CPointF		pt;
	CPointF		pto, pte;
	CPoint3F&	ptValS = bStart ? m_ptValS : m_ptValE;	// 代入もあるので参照型(別名)
	CPoint3F&	ptValE = bStart ? m_ptValE : m_ptValS;
	
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
		if ( m_pRead->m_pG68 ) {
			// m_ptValE はG68回転済み座標のため回転を元に戻してｵﾌｾｯﾄ減算
			m_pRead->m_pG68->dRound = -m_pRead->m_pG68->dRound;
			CalcG68Round(m_pRead->m_pG68, m_pRead->m_ptValOrg);
			m_pRead->m_pG68->dRound = -m_pRead->m_pG68->dRound;
		}
		m_pRead->m_ptValOrg -= m_pRead->m_ptOffset;	// m_ptValEがG68の場合、おかしくなる
		m_pt2D = m_ptValE.PointConvert();
	}

	return pt;
}

float CNCline::CalcBetweenAngle(const CNCdata* pNext) const
{
	// ２線の交点(自身の終点)が原点になるように補正
	CPointF		pt1( GetPlaneValueOrg(m_ptValS, m_ptValE) ), pt2;

	if ( pNext->GetType() == NCDARCDATA ) {
		const CNCcircle* pCircle = static_cast<const CNCcircle *>(pNext);
		// 次のｵﾌﾞｼﾞｪｸﾄが円弧なら中心をｾｯﾄし
		CPointF	pt( GetPlaneValueOrg(pCircle->GetOrg(), m_ptValE) );
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

optional<CPointF> CNCline::CalcPerpendicularPoint
	(ENPOINTORDER enPoint, float r, int nSign) const
{
	CPoint3F	pts, pte;
	if ( enPoint == STARTPOINT ) {
		pts = m_ptValS;
		pte = m_ptValE;
	}
	else {
		pts = m_ptValE;
		pte = m_ptValS;
		nSign = -nSign;		// 終点では-90°
	}
	// 線の傾きを計算して90°回転
	CPointF	pt( GetPlaneValueOrg(pte, pts) );
	float	q = pt.arctan();
	CPointF	pt1(r*cos(q), r*sin(q));
	CPointF	pt2(-pt1.y*nSign, pt1.x*nSign);
	pt2 += GetPlaneValue(pts);

	return pt2;
}

optional<CPointF> CNCline::CalcOffsetIntersectionPoint
	(const CNCdata* pNext, float t1, float t2, BOOL bLeft) const
{
	optional<CPointF>	ptResult;
	// ２線の交点(自身の終点)が原点になるように補正
	CPointF		pt, pt1( GetPlaneValueOrg(m_ptValS, m_ptValE) ), pt2;

	// ｵﾌｾｯﾄ分平行移動させた交点を求める
	if ( pNext->GetType() == NCDARCDATA ) {
		const CNCcircle* pCircle = static_cast<const CNCcircle *>(pNext);
		pt2 = GetPlaneValueOrg(pCircle->GetOrg(), m_ptValE);
		ptResult = ::CalcOffsetIntersectionPoint_LC(pt1, pt2, fabs(pCircle->GetR()), t1, t2,
						pCircle->GetG03(), bLeft);
	}
	else {
		pt2 = GetPlaneValueOrg(pNext->GetEndPoint(), m_ptValE);
		ptResult = ::CalcOffsetIntersectionPoint_LL(pt1, pt2, t1, t2, bLeft);
	}

	// 原点補正
	if ( ptResult ) {
		pt = *ptResult + GetPlaneValue(m_ptValE);
		return pt;
	}

	return boost::none;
}

optional<CPointF> CNCline::CalcOffsetIntersectionPoint2
	(const CNCdata* pNext, float r, BOOL bLeft) const
{
	// ２線の交点(自身の終点)が原点になるように補正
	CPointF		pt, pt1( GetPlaneValueOrg(m_ptValS, m_ptValE) ), pt2;

	// ｵﾌｾｯﾄ分平行移動させた交点を求める
	if ( pNext->GetType() == NCDARCDATA ) {
		// 円弧は始点接線とのｵﾌｾｯﾄ交点計算
		const CNCcircle* pCircle = static_cast<const CNCcircle *>(pNext);
		pt = GetPlaneValueOrg(pCircle->GetOrg(), m_ptValE);
		int k = pCircle->CalcOffsetSign();
		pt2.x = -pt.y * k;
		pt2.y =  pt.x * k;
	}
	else {
		pt2 = GetPlaneValueOrg(pNext->GetEndPoint(), m_ptValE);
	}

	// 直線同士のｵﾌｾｯﾄ交点計算
	optional<CPointF> ptResult = ::CalcOffsetIntersectionPoint_LL(pt1, pt2, r, r, bLeft);
	// 原点補正
	if ( ptResult ) {
		pt  = *ptResult;
		pt += GetPlaneValue(m_ptValE);
		return pt;
	}
	return boost::none;
}

void CNCline::SetCorrectPoint(ENPOINTORDER enPoint, const CPointF& ptSrc, float)
{
	CPoint3F&	ptVal    = enPoint==STARTPOINT ? m_ptValS : m_ptValE;	// 参照型
	CPointF&	ptResult = enPoint==STARTPOINT ? m_pt2Ds  : m_pt2D;

	SetPlaneValue(ptSrc, ptVal);
	ptResult = ptVal.PointConvert();

	if ( enPoint == ENDPOINT ) {
		ASSERT( m_pRead );
		SetPlaneValue(ptSrc, m_pRead->m_ptValOrg + m_pRead->m_ptOffset);
	}
}

//////////////////////////////////////////////////////////////////////
// CNCcycle クラス
//////////////////////////////////////////////////////////////////////

CNCcycle::CNCcycle
	(const CNCdata* pData, LPNCARGV lpArgv, const CPoint3F& ptOffset, BOOL bL0Cycle, NCMAKETYPE enType) :
		CNCline(NCDCYCLEDATA, pData, lpArgv, ptOffset)
{
	//	!!! Z, R, P 値は，TH_NCRead.cpp でも補間していることに注意 !!!
	CPoint3F	pt;
	float	dx, dy, dox, doy,	// 基準平面の移動距離
			dR, dI,				// R点座標, ｲﾆｼｬﾙ座標
			dRLength, dZLength,	// 移動長，切削長
			dResult;
	int		i, x, y, z,
			nH, nV;		// 縦横の繰り返し数

	// 初期化
	ZEROCLR(m_Cycle);	// m_Cycle[i++]=NULL
	m_Cycle3D = NULL;

	// 基準平面による座標設定
	switch ( GetPlane() ) {
	case XY_PLANE:
		x = NCA_X;
		y = NCA_Y;
		z = NCA_Z;
		break;
	case XZ_PLANE:
		if ( enType == NCMAKELATHE ) {
			// TH_NCRead.cpp で軸変換しているので、それに対応した補正
			x = NCA_Z;
			y = NCA_Z;
			z = NCA_X;
		}
		else {
			x = NCA_X;
			y = NCA_Z;
			z = NCA_Y;
		}
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
	if ( enType == NCMAKELATHE ) {
		nV = 1;
	}
	else {
		if ( GetValFlags() & NCD_K )
			nV = max(0, (int)lpArgv->nc.dValue[NCA_K]);
		else if ( GetValFlags() & NCD_L )
			nV = max(0, (int)lpArgv->nc.dValue[NCA_L]);
		else
			nV = 1;
	}
	// 復帰座標(前回のｵﾌﾞｼﾞｪｸﾄが固定ｻｲｸﾙかどうか)
	m_dInitial = pData->GetType()==NCDCYCLEDATA ?
				static_cast<const CNCcycle*>(pData)->GetInitialValue() - pData->GetOffsetPoint()[z] :
				m_ptValS[z];
	// ｲﾝｸﾘﾒﾝﾀﾙ補正(R座標はﾍﾞｰｽｸﾗｽで座標補正の対象外)
	if ( lpArgv->bAbs ) {
		dR = GetValFlags() & NCD_R ? (float)lpArgv->nc.dValue[NCA_R] : m_ptValS[z];
		m_nDrawCnt = nH = min(1, nV);	// ｱﾌﾞｿﾘｭｰﾄなら横へは(0 or 1)回のみ
	}
	else {
		dR = GetValFlags() & NCD_R ? m_dInitial + (float)lpArgv->nc.dValue[NCA_R] : m_dInitial;
		// !!! Z値もR点からのｲﾝｸﾘﾒﾝﾄに補正 !!!
		if ( GetValFlags() & g_dwSetValFlags[z] )
			m_nc.dValue[z] = dR + (float)lpArgv->nc.dValue[z];
		m_nDrawCnt = nH = nV;	// ｲﾝｸﾘﾒﾝﾀﾙなら横へも繰り返し
	}
	if ( enType == NCMAKELATHE )
		dI = m_dInitial;	// 旋盤ﾓｰﾄﾞでG98/G99は別の意味
	else
		dI = lpArgv->bG98 ? m_dInitial : dR;

	// 繰り返し数ｾﾞﾛなら
	if ( m_nDrawCnt <= 0 ) {
		if ( bL0Cycle ) {
			// 移動ﾃﾞｰﾀだけ作成
			nH = 1;
			dI = dR = m_ptValS[z];
		}
		else {
			// 以降の計算は不要
			ZEROCLR(m_dMove);
			m_dDwell = 0.0f;
			m_nc.dLength = m_dCycleMove = 0.0f;
			m_ptValI = m_ptValR = m_ptValE = m_ptValS = pData->GetEndPoint();
			m_pRead->m_ptValOrg = pData->GetOriginalEndPoint();
			m_dInitial += ptOffset[z];
			m_pt2D = m_pt2Ds;
			return;
		}
	}

	m_pRead->m_ptValOrg.SetPoint(GetValue(NCA_X), GetValue(NCA_Y), GetValue(NCA_Z));
	m_ptValE = m_pRead->m_ptValOrg;
	pt = pData->GetOriginalEndPoint();
	// 座標回転
	if ( lpArgv->g68.bG68 && enType!=NCMAKELATHE )
		CalcG68Round(&(lpArgv->g68), m_ptValE);
	// 各軸ごとの移動長計算
	dx = m_ptValE[x] - m_ptValS[x];		dox = m_pRead->m_ptValOrg[x] - pt[x];
	dy = m_ptValE[y] - m_ptValS[y];		doy = m_pRead->m_ptValOrg[y] - pt[y];
	dRLength = fabs(dI - dR);
	dZLength = fabs(dR - m_ptValE[z]);
	m_dMove[x]  = fabs(dx) * nH;
	m_dMove[y]  = fabs(dy) * nH;
	m_dMove[z]  = fabs(m_ptValS[z] - dR);	// 初回下降分
	m_dMove[z] += dRLength * (nV-1);
	// 移動長計算
	m_dCycleMove = _hypotf(m_dMove[x], m_dMove[y]);
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
		if ( enType == NCMAKELATHE ) {
			// 旋盤ﾓｰﾄﾞでの縦横繰り返しは未ｻﾎﾟｰﾄ
			m_ptValI.SetPoint(dI, m_ptValS.y, m_ptValE.z);
			m_ptValR.SetPoint(dR, m_ptValS.y, m_ptValE.z);
			m_ptValE.SetPoint(dI, m_ptValS.y, m_ptValE.z);
			m_pRead->m_ptValOrg.SetPoint(dI, m_ptValS.y, pt.z);
		}
		else {
			m_ptValI.SetPoint(m_ptValE.x, m_ptValS.y, m_ptValE.z);
			m_ptValR.SetPoint(m_ptValE.x, dR, m_ptValE.z);
			m_ptValE.SetPoint(m_ptValS.x+dx*nH, dI, m_ptValS.z+dy*nH);
			m_pRead->m_ptValOrg.SetPoint(pt.x+dox*nH, dI, pt.z+doy*nH);
		}
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
	dI += ptOffset[z];
	dR += ptOffset[z];
	m_pt2D = m_ptValE.PointConvert();

	// 切り込み値が指定されていなければｴﾗｰ(補間はTH_NCRead.cppにて)
	if ( !bL0Cycle && !(GetValFlags() & (g_dwSetValFlags[z]|NCFLG_LATHE_HOLE)) ) {
		m_nc.nErrorCode = IDS_ERR_NCBLK_NOTCYCLEZ;
		m_nDrawCnt = 0;
		m_dMove[z] = 0.0f;
		m_dDwell = 0.0f;
		return;
	}

#ifdef _DEBUG_DUMP
	printf("CNCcycle\n");
	printf("StartPoint x=%.3f y=%.3f z=%.3f\n",
		m_ptValS.x, m_ptValS.y, m_ptValS.z);
	printf("           R-Point=%.3f C-Point=%.3f\n", dR, GetValue(z)+ptOffset[z]);
	printf("FinalPoint x=%.3f y=%.3f z=%.3f\n",
		m_ptValE.x, m_ptValE.y, m_ptValE.z);
	printf("m_nDrawCnt=%d\n", m_nDrawCnt);
#endif

	if ( m_nDrawCnt <= 0 )
		return;		// bL0Cycle でもここまで

	// 座標格納領域確保
	for ( i=0; i<SIZEOF(m_Cycle); i++ )
		m_Cycle[i] = new PTCYCLE[m_nDrawCnt];
	m_Cycle3D = new PTCYCLE3D[m_nDrawCnt];

	pt = m_ptValS;
	int zz = enType==NCMAKELATHE ? NCA_X : z;
	for ( i=0; i<m_nDrawCnt; i++ ) {
		pt[x] += dx;	pt[y] += dy;
#ifdef _DEBUG_DUMP
		printf("           No.%d [x]=%.3f [y]=%.3f\n", i+1, pt[x], pt[y]);
#endif
		// 各平面ごとに座標設定
		pt[zz] = dI;
		m_Cycle3D[i].ptI  = pt;
		m_Cycle[NCDRAWVIEW_XYZ][i].ptI = pt.PointConvert();
		m_Cycle[NCDRAWVIEW_XY ][i].ptI = pt.GetXY();
		m_Cycle[NCDRAWVIEW_XZ ][i].ptI = pt.GetXZ();
		m_Cycle[NCDRAWVIEW_YZ ][i].ptI = pt.GetYZ();
		pt[zz] = dR;
		m_Cycle3D[i].ptR  = pt;
		m_Cycle[NCDRAWVIEW_XYZ][i].ptR = pt.PointConvert();
		m_Cycle[NCDRAWVIEW_XY ][i].ptR = pt.GetXY();
		m_Cycle[NCDRAWVIEW_XZ ][i].ptR = pt.GetXZ();
		m_Cycle[NCDRAWVIEW_YZ ][i].ptR = pt.GetYZ();
		pt[zz] = GetValue(zz) + ptOffset[zz];
		m_Cycle3D[i].ptC  = pt;
		m_Cycle[NCDRAWVIEW_XYZ][i].ptC = pt.PointConvert();
		m_Cycle[NCDRAWVIEW_XY ][i].ptC = pt.GetXY();
		m_Cycle[NCDRAWVIEW_XZ ][i].ptC = pt.GetXZ();
		m_Cycle[NCDRAWVIEW_YZ ][i].ptC = pt.GetYZ();
	}
	
	// 上昇分の移動・切削長計算
	switch ( GetGcode() ) {
	case 84:	// R点まで切削復帰，ｲﾆｼｬﾙ点まで早送り復帰
	case 85:
	case 87:
	case 88:
	case 89:
		if ( lpArgv->bG98 && enType!=NCMAKELATHE ) {
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
//	if ( GetValFlags() & NCD_P &&
//		(GetGcode()==82 || GetGcode()==88 || GetGcode()==89) )
	if ( GetValFlags() & NCD_P )	// P_あればコードに関係なく加算に仕様変更
		m_dDwell = (float)lpArgv->nc.dValue[NCA_P] * nV;
	else
		m_dDwell = 0.0f;

#ifdef _DEBUG_DUMP
	DbgDump();
	Dbg_sep();
#endif
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

void CNCcycle::DrawTuning(float f)
{
	CNCline::DrawTuning(f);
	m_ptDrawI[NCDRAWVIEW_XYZ] = m_ptValI.PointConvert() * f;
	m_ptDrawR[NCDRAWVIEW_XYZ] = m_ptValR.PointConvert() * f;
	if ( m_nDrawCnt > 0 ) {
		for ( int i=0; i<m_nDrawCnt; i++ )
			m_Cycle[NCDRAWVIEW_XYZ][i].DrawTuning(f);
	}
}

void CNCcycle::DrawTuningXY(float f)
{
	CNCline::DrawTuningXY(f);
	m_ptDrawI[NCDRAWVIEW_XY] = m_ptValI.GetXY() * f;
	m_ptDrawR[NCDRAWVIEW_XY] = m_ptValR.GetXY() * f;
	if ( m_nDrawCnt > 0 ) {
		for ( int i=0; i<m_nDrawCnt; i++ )
			m_Cycle[NCDRAWVIEW_XY][i].DrawTuning(f);
	}
}

void CNCcycle::DrawTuningXZ(float f)
{
	CNCline::DrawTuningXZ(f);
	m_ptDrawI[NCDRAWVIEW_XZ] = m_ptValI.GetXZ() * f;
	m_ptDrawR[NCDRAWVIEW_XZ] = m_ptValR.GetXZ() * f;
	if ( m_nDrawCnt > 0 ) {
		for ( int i=0; i<m_nDrawCnt; i++ )
			m_Cycle[NCDRAWVIEW_XZ][i].DrawTuning(f);
	}
}

void CNCcycle::DrawTuningYZ(float f)
{
	CNCline::DrawTuningYZ(f);
	m_ptDrawI[NCDRAWVIEW_YZ] = m_ptValI.GetYZ() * f;
	m_ptDrawR[NCDRAWVIEW_YZ] = m_ptValR.GetYZ() * f;
	if ( m_nDrawCnt > 0 ) {
		for ( int i=0; i<m_nDrawCnt; i++ )
			m_Cycle[NCDRAWVIEW_YZ][i].DrawTuning(f);
	}
}

void CNCcycle::Draw(CDC* pDC, BOOL bSelect) const
{
#ifdef _DEBUGDRAW_NCD
	printf("Cycle Draw()=%d\n", GetBlockLineNo()+1);
#endif
	DrawCycle(NCDRAWVIEW_XYZ, pDC, bSelect);
}

void CNCcycle::DrawXY(CDC* pDC, BOOL bSelect) const
{
#ifdef _DEBUGDRAW_NCD
	printf("Cycle DrawXY()=%d\n", GetBlockLineNo()+1);
#endif
	if ( GetPlane() == XY_PLANE ) {
		CNCline::DrawXY(pDC, bSelect);
		DrawCyclePlane(NCDRAWVIEW_XY, pDC, bSelect);
	}
	else
		DrawCycle(NCDRAWVIEW_XY, pDC, bSelect);
}

void CNCcycle::DrawXZ(CDC* pDC, BOOL bSelect) const
{
#ifdef _DEBUGDRAW_NCD
	printf("Cycle DrawXZ()=%d\n", GetBlockLineNo()+1);
#endif
	if ( GetPlane() == XZ_PLANE ) {
		if ( GetValFlags() & NCFLG_LATHE_HOLE )
			DrawCycle(NCDRAWVIEW_XZ, pDC, bSelect);
		else {
			CNCline::DrawXZ(pDC, bSelect);
			DrawCyclePlane(NCDRAWVIEW_XZ, pDC, bSelect);
		}
	}
	else
		DrawCycle(NCDRAWVIEW_XZ, pDC, bSelect);
}

void CNCcycle::DrawYZ(CDC* pDC, BOOL bSelect) const
{
#ifdef _DEBUGDRAW_NCD
	printf("Cycle DrawYZ()=%d\n", GetBlockLineNo()+1);
#endif
	if ( GetPlane() == YZ_PLANE ) {
		CNCline::DrawYZ(pDC, bSelect);
		DrawCyclePlane(NCDRAWVIEW_YZ, pDC, bSelect);
	}
	else
		DrawCycle(NCDRAWVIEW_YZ, pDC, bSelect);
}

void CNCcycle::DrawWire(CDC* pDC, BOOL bSelect) const
{
}

void CNCcycle::DrawWireXY(CDC* pDC, BOOL bSelect) const
{
}

void CNCcycle::DrawWireXZ(CDC* pDC, BOOL bSelect) const
{
}

void CNCcycle::DrawWireYZ(CDC* pDC, BOOL bSelect) const
{
}

void CNCcycle::DrawCyclePlane(ENNCDRAWVIEW enDraw, CDC* pDC, BOOL bSelect) const
{
	CPen*	pOldPen = bSelect ?
		AfxGetNCVCMainWnd()->GetPenCom(COMPEN_SEL) :
		AfxGetNCVCMainWnd()->GetPenNC(NCPEN_CYCLE);
	pOldPen = pDC->SelectObject(pOldPen);
	CBrush*	pOldBrush = pDC->SelectObject(AfxGetNCVCMainWnd()->GetBrushNC(NCBRUSH_CYCLEXY));
	for ( int i=0; i<m_nDrawCnt; i++ )
		pDC->Ellipse(&m_Cycle[enDraw][i].rcDraw);
	pDC->SelectObject(pOldBrush);
	pDC->SelectObject(pOldPen);
}

void CNCcycle::DrawCycle(ENNCDRAWVIEW enDraw, CDC* pDC, BOOL bSelect) const
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
	pDC->MoveTo(m_ptDrawS[enDraw]);
	pDC->LineTo(m_ptDrawI[enDraw]);		// 現在位置から１点目のｲﾆｼｬﾙ点
	pDC->LineTo(m_ptDrawR[enDraw]);		// ｲﾆｼｬﾙ点からR点
	pDC->LineTo(m_ptDrawE[enDraw]);		// R点から最後の穴加工
	// 切削方向の描画
	for ( int i=0; i<m_nDrawCnt; i++ ) {
		pDC->SelectObject(pPenM);
		pDC->MoveTo(m_Cycle[enDraw][i].ptDrawI);
		pDC->LineTo(m_Cycle[enDraw][i].ptDrawR);
		pDC->SelectObject(pPenC);
		pDC->LineTo(m_Cycle[enDraw][i].ptDrawC);
	}
	pDC->SelectObject(pOldPen);
}

//////////////////////////////////////////////////////////////////////
// CNCcircle クラス
//////////////////////////////////////////////////////////////////////

CNCcircle::CNCcircle
(const CNCdata* pData, LPNCARGV lpArgv, const CPoint3F& ptOffset, NCMAKETYPE enType) :
	CNCline(NCDARCDATA, pData, lpArgv, ptOffset)
{
	BOOL	bError = TRUE;				// Error

	if ( GetGcode() == 2 )
		m_dwFlags &= ~NCFLG_G02G03;		// 0:G02
	else
		m_dwFlags |=  NCFLG_G02G03;		// 1:G03

	if ( enType != NCMAKELATHE ) {
		// XZ平面は(Z->X, X->Y)なのでそのまま計算すると -90°回転させる必要がある
		// 簡単に対応するには回転方向を反対にすればよい -> もうちょっとマシな対応を！
		if ( GetPlane() == XZ_PLANE )
			m_dwFlags ^= NCFLG_G02G03;
	}

	// ﾈｲﾃｨﾌﾞの座標ﾃﾞｰﾀで中心を計算してから座標回転
	m_pRead->m_ptValOrg.SetPoint(GetValue(NCA_X), GetValue(NCA_Y), GetValue(NCA_Z));
	m_ptValS = pData->GetOriginalEndPoint();	// あとで設定し直し
	m_ptValE = m_pRead->m_ptValOrg;

	// 平面座標取得
	CPointF	pts( GetPlaneValue(m_ptValS) ),
			pte( GetPlaneValue(m_ptValE) ),
			pto;

	// 半径と中心座標の計算(R優先 ただし、ﾜｲﾔﾓｰﾄﾞは無視)
	if ( GetValFlags()&NCD_R && enType!=NCMAKEWIRE ) {
		m_r = (float)lpArgv->nc.dValue[NCA_R];
		bError = !CalcCenter(pts, pte);
	}
	else if ( GetValFlags() & (NCD_I|NCD_J|NCD_K) ) {
		float	i = GetValFlags() & NCD_I ? (float)lpArgv->nc.dValue[NCA_I] : 0.0f,
				j = GetValFlags() & NCD_J ? (float)lpArgv->nc.dValue[NCA_J] : 0.0f,
				k = GetValFlags() & NCD_K ? (float)lpArgv->nc.dValue[NCA_K] : 0.0f;
		m_ptOrg = m_ptValS;
		switch ( GetPlane() ) {
		case XY_PLANE:
			m_r = _hypotf(i, j);
			m_ptOrg.x += i;
			m_ptOrg.y += j;
			pto = m_ptOrg.GetXY();
			break;
		case XZ_PLANE:
			m_r = _hypotf(i, k);
			m_ptOrg.x += i;
			m_ptOrg.z += k;
			pto = m_ptOrg.GetXZ();
			break;
		case YZ_PLANE:
			m_r = _hypotf(j, k);
			m_ptOrg.y += j;
			m_ptOrg.z += k;
			pto = m_ptOrg.GetYZ();
			break;
		}
		pts -= pto;		// 角度調整用の原点補正
		pte -= pto;
		AngleTuning(pts, pte);
		bError = FALSE;
	}

	// 描画関数の決定とﾍﾘｶﾙ移動量の計算
	Constracter();

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

	if ( bError ) {
		m_nc.nErrorCode = IDS_ERR_NCBLK_CIRCLECENTER;
	}

#ifdef _DEBUG_DUMP
	printf("CNCcircle gcode=%d\n", GetGcode());
	printf("sx=%.3f sy=%.3f sz=%.3f / ex=%.3f ey=%.3f ez=%.3f / r=%.3f\n",
		m_ptValS.x, m_ptValS.y, m_ptValS.z,
		m_ptValE.x, m_ptValE.y, m_ptValE.z, m_r);
	printf("px=%.3f py=%.3f pz=%.3f / sq=%f eq=%f\n",
		m_ptOrg.x, m_ptOrg.y, m_ptOrg.z, DEG(m_sq), DEG(m_eq));
	DbgDump();
	Dbg_sep();
#endif
}

CNCcircle::CNCcircle(const CNCdata* pData) : CNCline(pData)
{
	m_pt2D	= m_ptValE.PointConvert();
	const CNCcircle*	pCircle = static_cast<const CNCcircle *>(pData);
	m_ptOrg	= pCircle->GetOrg();
	m_r		= pCircle->GetR();
	m_sq	= pCircle->GetStartAngle();
	m_eq	= pCircle->GetEndAngle();
	Constracter();
}

void CNCcircle::Constracter(void)
{
	// 描画関数の決定とﾍﾘｶﾙ移動量の計算
	switch ( GetPlane() ) {
	case XY_PLANE:
		m_pfnCircleDraw = &CNCcircle::DrawG17;
		m_dHelicalStep = GetValFlags() & NCD_Z ?
			(m_ptValE.z - m_ptValS.z) / ((m_eq - m_sq)/ARCSTEP) : 0.0f;
		break;
	case XZ_PLANE:
		m_pfnCircleDraw = &CNCcircle::DrawG18;
		m_dHelicalStep = GetValFlags() & NCD_Y ?
			(m_ptValE.y - m_ptValS.y) / ((m_eq - m_sq)/ARCSTEP) : 0.0f;
		break;
	case YZ_PLANE:
		m_pfnCircleDraw = &CNCcircle::DrawG19;
		m_dHelicalStep = GetValFlags() & NCD_X ?
			(m_ptValE.x - m_ptValS.x) / ((m_eq - m_sq)/ARCSTEP) : 0.0f;
		break;
	}
}

BOOL CNCcircle::CalcCenter(const CPointF& pts, const CPointF& pte)
{
	// R 指定で始点終点が同じ場合はエラー
	if ( pts == pte )
		return FALSE;

	// ２つの円の交点を求める
	CPointF	pt1, pt2;
	int		nResult;
	tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_CC(pts, pte, m_r, m_r);
	if ( nResult < 1 )
		return FALSE;

	// どちらの解を採用するか
	AngleTuning(pts-pt1, pte-pt1);	// まず一方の中心座標から角度を求める
	float	q = ::RoundUp(DEG(m_eq-m_sq));
	if ( nResult==1 ||
			(m_r>0.0f && q<=180.0f) ||	// 180°以下
			(m_r<0.0f && q> 180.0f) ) {	// 180°超える
		SetCenter(pt1);
	}
	else {
		// 条件がﾏｯﾁしなかったので他方の解を採用
		AngleTuning(pts-pt2, pte-pt2);
		SetCenter(pt2);
	}
	return TRUE;
}

void CNCcircle::SetCenter(const CPointF& pt)
{
	switch ( GetPlane() ) {
	case XY_PLANE:
		m_ptOrg.x = pt.x;
		m_ptOrg.y = pt.y;
		m_ptOrg.z = m_ptValS.z;		// 始点の開始位置
		break;
	case XZ_PLANE:
		m_ptOrg.x = pt.x;
		m_ptOrg.y = m_ptValS.y;
		m_ptOrg.z = pt.y;
		break;
	case YZ_PLANE:
		m_ptOrg.x = m_ptValS.x;
		m_ptOrg.y = pt.x;
		m_ptOrg.z = pt.y;
		break;
	}
}

tuple<float, float>	CNCcircle::CalcAngle(BOOL bG03, const CPointF& pts, const CPointF& pte) const
{
	float	sq, eq, q;

	if ( (sq=pts.arctan()) < 0.0f )
		sq += PI2;
	if ( (eq=pte.arctan()) < 0.0f )
		eq += PI2;

	// 常に s<e (反時計回り) とする
	if ( !bG03 )	// G02 なら開始角度と終了角度を入れ替え
		invoke_swap(sq, eq);

	// 微細円弧に注意
	q = ::RoundUp(DEG(sq));
	while ( q > ::RoundUp(DEG(eq)) )	// CDXFarc::AngleTuning()と同じ条件に変更
		eq += PI2;

	// 角度調整
	if ( q>=360.0f && ::RoundUp(DEG(eq))>=360.0f ) {
		sq -= PI2;
		eq -= PI2;
	}

	return make_tuple(sq, eq);
}

void CNCcircle::AngleTuning(const CPointF& pts, const CPointF& pte)
{
	tie(m_sq, m_eq) = CalcAngle(GetG03(), pts, pte);
	if ( pts == pte ) {	
		// 始点終点が同じ真円なら
		// 終点角度 = 開始角度 + 360°に強制置換
		m_eq = m_sq + PI2;
	}
}

float CNCcircle::SetCalcLength(void)
{
	// 切削長のみ計算
	ZEROCLR(m_dMove);

	if ( m_obCdata.IsEmpty() )
		m_nc.dLength = fabs(m_r * (m_eq - m_sq));
	else {
		m_nc.dLength = 0.0f;
		// 各補正要素の合計
		for ( int i=0; i<m_obCdata.GetSize(); i++ )
			m_nc.dLength += m_obCdata[i]->SetCalcLength();
	}

	float	dResult = 0.0f;
	if ( m_pWireObj )
		dResult = m_pWireObj->SetCalcLength();

	return max((float)m_nc.dLength, dResult);
}

void CNCcircle::DrawTuning(float f)
{
	// 計算しながら拡大係数を与える
	m_dFactor = f;

	// ﾜｲﾔ加工表示用の始点終点
	float		sq, eq, r = fabs(m_r) * f;
	CPoint3F	pt3D, ptOrg(m_ptOrg);	ptOrg *= f;
	tie(sq, eq) = GetSqEq();
	pt3D.x = r * cos(sq) + ptOrg.x;
	pt3D.y = r * sin(sq) + ptOrg.y;
	pt3D.z = ptOrg.z;
	m_ptDrawS[NCDRAWVIEW_XYZ] = pt3D.PointConvert();
	pt3D.x = r * cos(eq) + ptOrg.x;
	pt3D.y = r * sin(eq) + ptOrg.y;
	pt3D.z = m_ptValE.z * f;
	m_ptDrawE[NCDRAWVIEW_XYZ] = pt3D.PointConvert();

	CNCdata::DrawTuning(f);
}

void CNCcircle::DrawTuningXY(float f)
{
	m_dFactorXY = f;

	float		sq, eq, r = fabs(m_r) * f;
	CPoint3F	ptOrg(m_ptOrg);	ptOrg *= f;
	tie(sq, eq) = GetSqEq();
	m_ptDrawS[NCDRAWVIEW_XY].x = (int)(r * cos(sq) + ptOrg.x);
	m_ptDrawS[NCDRAWVIEW_XY].y = (int)(r * sin(sq) + ptOrg.y);
	m_ptDrawE[NCDRAWVIEW_XY].x = (int)(r * cos(eq) + ptOrg.x);
	m_ptDrawE[NCDRAWVIEW_XY].y = (int)(r * sin(eq) + ptOrg.y);

	CNCdata::DrawTuningXY(f);
}

void CNCcircle::DrawTuningXZ(float f)
{
	m_dFactorXZ = f;

	float		sq, eq, r = fabs(m_r) * f;
	CPoint3F	ptOrg(m_ptOrg);	ptOrg *= f;
	tie(sq, eq) = GetSqEq();
	m_ptDrawS[NCDRAWVIEW_XZ].x = (int)(r * cos(sq) + ptOrg.x);
	m_ptDrawS[NCDRAWVIEW_XZ].y = (int)(ptOrg.z);
	m_ptDrawE[NCDRAWVIEW_XZ].x = (int)(r * cos(eq) + ptOrg.x);
	m_ptDrawE[NCDRAWVIEW_XZ].y = (int)(m_ptValE.z * f);

	CNCdata::DrawTuningXZ(f);
}

void CNCcircle::DrawTuningYZ(float f)
{
	m_dFactorYZ = f;

	float		sq, eq, r = fabs(m_r) * f;
	CPoint3F	ptOrg(m_ptOrg);	ptOrg *= f;
	tie(sq, eq) = GetSqEq();
	m_ptDrawS[NCDRAWVIEW_YZ].x = (int)(r * sin(sq) + ptOrg.y);
	m_ptDrawS[NCDRAWVIEW_YZ].y = (int)(ptOrg.z);
	m_ptDrawE[NCDRAWVIEW_YZ].x = (int)(r * sin(eq) + ptOrg.y);
	m_ptDrawE[NCDRAWVIEW_YZ].y = (int)(m_ptValE.z * f);

	CNCdata::DrawTuningYZ(f);
}

void CNCcircle::Draw(CDC* pDC, BOOL bSelect) const
{
#ifdef _DEBUGDRAW_NCD
	printf("Circle Draw()=%d\n", GetBlockLineNo()+1);
#endif
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();

	// 平面ごとの描画関数
	if ( m_obCdata.IsEmpty() || pOpt->GetNCViewFlg(GLOPTFLG_DRAWREVISE) ) {
		CPen*	pOldPen;
		if ( bSelect )
			pOldPen = AfxGetNCVCMainWnd()->GetPenCom(COMPEN_SEL);
		else
			pOldPen = AfxGetNCVCMainWnd()->GetPenNC(
				m_obCdata.IsEmpty() ? NCPEN_G1 : NCPEN_CORRECT);
		pOldPen = pDC->SelectObject(pOldPen);
		(this->*m_pfnCircleDraw)(NCDRAWVIEW_XYZ, pDC);
		pDC->SelectObject(pOldPen);
	}
	// 中心座標(黄色)
	if ( pOpt->GetNCViewFlg(GLOPTFLG_DRAWCIRCLECENTER) )
		pDC->SetPixelV(m_ptOrg.PointConvert()*m_dFactor,
			pOpt->GetNcDrawColor(NCCOL_CENTERCIRCLE));
	// 径補正ﾃﾞｰﾀの描画
	CNCdata::Draw(pDC, bSelect);
}

void CNCcircle::DrawXY(CDC* pDC, BOOL bSelect) const
{
#ifdef _DEBUGDRAW_NCD
	printf("Circle DrawXY()=%d\n", GetBlockLineNo()+1);
#endif
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();

	if ( m_obCdata.IsEmpty() || pOpt->GetNCViewFlg(GLOPTFLG_DRAWREVISE) ) {
		CPen*	pOldPen;
		if ( bSelect )
			pOldPen = AfxGetNCVCMainWnd()->GetPenCom(COMPEN_SEL);
		else
			pOldPen = AfxGetNCVCMainWnd()->GetPenNC(
				m_obCdata.IsEmpty() ? NCPEN_G1 : NCPEN_CORRECT);
		pOldPen = pDC->SelectObject(pOldPen);
		(this->*m_pfnCircleDraw)(NCDRAWVIEW_XY, pDC);
		pDC->SelectObject(pOldPen);
	}
	if ( pOpt->GetNCViewFlg(GLOPTFLG_DRAWCIRCLECENTER) )
		pDC->SetPixelV(m_ptOrg.GetXY()*m_dFactorXY,
			pOpt->GetNcDrawColor(NCCOL_CENTERCIRCLE));
	CNCdata::DrawXY(pDC, bSelect);
}

void CNCcircle::DrawXZ(CDC* pDC, BOOL bSelect) const
{
#ifdef _DEBUGDRAW_NCD
	printf("Circle DrawXZ()=%d\n", GetBlockLineNo()+1);
#endif
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();

	if ( m_obCdata.IsEmpty() || pOpt->GetNCViewFlg(GLOPTFLG_DRAWREVISE) ) {
		CPen*	pOldPen;
		if ( bSelect )
			pOldPen = AfxGetNCVCMainWnd()->GetPenCom(COMPEN_SEL);
		else
			pOldPen = AfxGetNCVCMainWnd()->GetPenNC(
				m_obCdata.IsEmpty() ? NCPEN_G1 : NCPEN_CORRECT);
		pOldPen = pDC->SelectObject(pOldPen);
		(this->*m_pfnCircleDraw)(NCDRAWVIEW_XZ, pDC);
		pDC->SelectObject(pOldPen);
	}
	if ( pOpt->GetNCViewFlg(GLOPTFLG_DRAWCIRCLECENTER) )
		pDC->SetPixelV(m_ptOrg.GetXZ()*m_dFactorXZ,
			pOpt->GetNcDrawColor(NCCOL_CENTERCIRCLE));
	CNCdata::DrawXZ(pDC, bSelect);
}

void CNCcircle::DrawYZ(CDC* pDC, BOOL bSelect) const
{
#ifdef _DEBUGDRAW_NCD
	printf("Circle DrawYZ()=%d\n", GetBlockLineNo()+1);
#endif
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();

	if ( m_obCdata.IsEmpty() || pOpt->GetNCViewFlg(GLOPTFLG_DRAWREVISE) ) {
		CPen*	pOldPen;
		if ( bSelect )
			pOldPen = AfxGetNCVCMainWnd()->GetPenCom(COMPEN_SEL);
		else
			pOldPen = AfxGetNCVCMainWnd()->GetPenNC(
				m_obCdata.IsEmpty() ? NCPEN_G1 : NCPEN_CORRECT);
		pOldPen = pDC->SelectObject(pOldPen);
		(this->*m_pfnCircleDraw)(NCDRAWVIEW_YZ, pDC);
		pDC->SelectObject(pOldPen);
	}
	if ( pOpt->GetNCViewFlg(GLOPTFLG_DRAWCIRCLECENTER) )
		pDC->SetPixelV(m_ptOrg.GetYZ()*m_dFactorYZ,
			pOpt->GetNcDrawColor(NCCOL_CENTERCIRCLE));
	CNCdata::DrawYZ(pDC, bSelect);
}

void CNCcircle::DrawWire(CDC* pDC, BOOL bSelect) const
{
	DrawWireLine(NCDRAWVIEW_XYZ, pDC, bSelect);
}

void CNCcircle::DrawWireXY(CDC* pDC, BOOL bSelect) const
{
	DrawWireLine(NCDRAWVIEW_XY, pDC, bSelect);
}

void CNCcircle::DrawWireXZ(CDC* pDC, BOOL bSelect) const
{
	DrawWireLine(NCDRAWVIEW_XZ, pDC, bSelect);
}

void CNCcircle::DrawWireYZ(CDC* pDC, BOOL bSelect) const
{
	DrawWireLine(NCDRAWVIEW_YZ, pDC, bSelect);
}

void CNCcircle::DrawWireLine(ENNCDRAWVIEW enDraw, CDC* pDC, BOOL bSelect) const
{
	CPen*	pOldPen;
	if ( bSelect )
		pOldPen = AfxGetNCVCMainWnd()->GetPenCom(COMPEN_SEL);
	else
		pOldPen = AfxGetNCVCMainWnd()->GetPenNC(
			m_obCdata.IsEmpty() ? NCPEN_G1 : NCPEN_CORRECT);
	pOldPen = pDC->SelectObject(pOldPen);
	// XY
	DrawG17(enDraw, pDC);
	// UV
	if ( m_pWireObj ) {
		// m_pWireObjがCNCcircleとは限らない
		m_pWireObj->DrawWireLine(enDraw, pDC, bSelect);
		// XYとUVの接続
		pDC->MoveTo(m_ptDrawS[enDraw]);
		pDC->LineTo(m_pWireObj->GetDrawStartPoint(enDraw));
		pDC->MoveTo(m_ptDrawE[enDraw]);
		pDC->LineTo(m_pWireObj->GetDrawEndPoint(enDraw));
	}
	pDC->SelectObject(pOldPen);
}

// CDC::Arc() を使うとどうしても表示がズレる．
// 同一平面であっても微細線分による近似を行う
void CNCcircle::DrawG17(ENNCDRAWVIEW enDraw, CDC* pDC) const	// XY_PLANE
{
	float		sq, eq,
				dHelical = m_dHelicalStep, r = fabs(m_r);
	CPoint3F	pt3D, ptDrawOrg(m_ptOrg);
	CPointF		ptDraw;

	tie(sq, eq) = GetSqEq();

	switch ( enDraw ) {
	case NCDRAWVIEW_XYZ:
		r *= m_dFactor;
		dHelical  *= m_dFactor;
		ptDrawOrg *= m_dFactor;
		pt3D.x = r * cos(sq) + ptDrawOrg.x;
		pt3D.y = r * sin(sq) + ptDrawOrg.y;
		pt3D.z = ptDrawOrg.z;		// ﾍﾘｶﾙ開始座標
		ptDraw = pt3D.PointConvert();
		pDC->MoveTo(ptDraw);	// 開始点へ移動
		// ARCSTEP づつ微細線分で描画
		if ( GetG03() ) {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				pt3D.x = r * cos(sq) + ptDrawOrg.x;
				pt3D.y = r * sin(sq) + ptDrawOrg.y;
				pt3D.z += dHelical;
				ptDraw = pt3D.PointConvert();
				pDC->LineTo(ptDraw);
			}
		}
		else {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
				pt3D.x = r * cos(sq) + ptDrawOrg.x;
				pt3D.y = r * sin(sq) + ptDrawOrg.y;
				pt3D.z += dHelical;
				ptDraw = pt3D.PointConvert();
				pDC->LineTo(ptDraw);
			}
		}
		// 端数分描画
		pt3D.x = r * cos(eq) + ptDrawOrg.x;
		pt3D.y = r * sin(eq) + ptDrawOrg.y;
		pt3D.z = m_ptValE.z * m_dFactor;
		ptDraw = pt3D.PointConvert();
		pDC->LineTo(ptDraw);
		break;

	case NCDRAWVIEW_XY:	// Don't ARC()
		r *= m_dFactorXY;
		ptDrawOrg *= m_dFactorXY;
		ptDraw.x = r * cos(sq) + ptDrawOrg.x;
		ptDraw.y = r * sin(sq) + ptDrawOrg.y;
		pDC->MoveTo(ptDraw);
		if ( GetG03() ) {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				ptDraw.x = r * cos(sq) + ptDrawOrg.x;
				ptDraw.y = r * sin(sq) + ptDrawOrg.y;
				pDC->LineTo(ptDraw);
			}
		}
		else {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
				ptDraw.x = r * cos(sq) + ptDrawOrg.x;
				ptDraw.y = r * sin(sq) + ptDrawOrg.y;
				pDC->LineTo(ptDraw);
			}
		}
		ptDraw.x = r * cos(eq) + ptDrawOrg.x;
		ptDraw.y = r * sin(eq) + ptDrawOrg.y;
		pDC->LineTo(ptDraw);
		break;

	case NCDRAWVIEW_XZ:	// 占有矩形の頂点を結ぶだけではﾍﾘｶﾙ切削の描画に対応できない
		r *= m_dFactorXZ;
		dHelical  *= m_dFactorXZ;
		ptDrawOrg *= m_dFactorXZ;
		ptDraw.x = r * cos(sq) + ptDrawOrg.x;
		ptDraw.y = ptDrawOrg.z;
		pDC->MoveTo(ptDraw);
		if ( GetG03() ) {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				ptDraw.x  = r * cos(sq) + ptDrawOrg.x;
				ptDraw.y += dHelical;
				pDC->LineTo(ptDraw);
			}
		}
		else {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
				ptDraw.x  = r * cos(sq) + ptDrawOrg.x;
				ptDraw.y += dHelical;
				pDC->LineTo(ptDraw);
			}
		}
		ptDraw.x = r * cos(eq) + ptDrawOrg.x;
		ptDraw.y = m_ptValE.z * m_dFactorXZ;
		pDC->LineTo(ptDraw);
		break;

	case NCDRAWVIEW_YZ:
		r *= m_dFactorYZ;
		dHelical  *= m_dFactorYZ;
		ptDrawOrg *= m_dFactorYZ;
		ptDraw.x = r * sin(sq) + ptDrawOrg.y;
		ptDraw.y = ptDrawOrg.z;
		pDC->MoveTo(ptDraw);
		if ( GetG03() ) {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				ptDraw.x  = r * sin(sq) + ptDrawOrg.y;
				ptDraw.y += dHelical;
				pDC->LineTo(ptDraw);
			}
		}
		else {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
				ptDraw.x  = r * sin(sq) + ptDrawOrg.y;
				ptDraw.y += dHelical;
				pDC->LineTo(ptDraw);
			}
		}
		ptDraw.x = r * sin(eq) + ptDrawOrg.y;
		ptDraw.y = m_ptValE.z * m_dFactorYZ;
		pDC->LineTo(ptDraw);
		break;
	}
}

void CNCcircle::DrawG18(ENNCDRAWVIEW enDraw, CDC* pDC) const	// XZ_PLANE
{
	float		sq, eq,
				dHelical = m_dHelicalStep, r = fabs(m_r);
	CPoint3F	pt3D, ptDrawOrg(m_ptOrg);
	CPointF		ptDraw;

	tie(sq, eq) = GetSqEq();

	switch ( enDraw ) {
	case NCDRAWVIEW_XYZ:
		r *= m_dFactor;
		dHelical  *= m_dFactor;
		ptDrawOrg *= m_dFactor;
		pt3D.x = r * cos(sq) + ptDrawOrg.x;
		pt3D.y = ptDrawOrg.y;
		pt3D.z = r * sin(sq) + ptDrawOrg.z;
		ptDraw = pt3D.PointConvert();
		pDC->MoveTo(ptDraw);
		if ( GetG03() ) {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				pt3D.x = r * cos(sq) + ptDrawOrg.x;
				pt3D.y += dHelical;
				pt3D.z = r * sin(sq) + ptDrawOrg.z;
				ptDraw = pt3D.PointConvert();
				pDC->LineTo(ptDraw);
			}
		}
		else {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
				pt3D.x = r * cos(sq) + ptDrawOrg.x;
				pt3D.y += dHelical;
				pt3D.z = r * sin(sq) + ptDrawOrg.z;
				ptDraw = pt3D.PointConvert();
				pDC->LineTo(ptDraw);
			}
		}
		pt3D.x = r * cos(eq) + ptDrawOrg.x;
		pt3D.y = m_ptValE.y * m_dFactor;
		pt3D.z = r * sin(eq) + ptDrawOrg.z;
		ptDraw = pt3D.PointConvert();
		pDC->LineTo(ptDraw);
		break;

	case NCDRAWVIEW_XY:
		r *= m_dFactorXY;
		dHelical  *= m_dFactorXY;
		ptDrawOrg *= m_dFactorXY;
		ptDraw.x = r * cos(sq) + ptDrawOrg.x;
		ptDraw.y = ptDrawOrg.y;
		pDC->MoveTo(ptDraw);
		if ( GetG03() ) {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				ptDraw.x  = r * cos(sq) + ptDrawOrg.x;
				ptDraw.y += dHelical;
				pDC->LineTo(ptDraw);
			}
		}
		else {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
				ptDraw.x  = r * cos(sq) + ptDrawOrg.x;
				ptDraw.y += dHelical;
				pDC->LineTo(ptDraw);
			}
		}
		ptDraw.x = r * cos(eq) + ptDrawOrg.x;
		ptDraw.y = m_ptValE.y * m_dFactorXY;
		pDC->LineTo(ptDraw);
		break;

	case NCDRAWVIEW_XZ:	// Don't ARC()
		r *= m_dFactorXZ;
		ptDrawOrg *= m_dFactorXZ;
		ptDraw.x = r * cos(sq) + ptDrawOrg.x;
		ptDraw.y = r * sin(sq) + ptDrawOrg.z;
		pDC->MoveTo(ptDraw);
		if ( GetG03() ) {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				ptDraw.x = r * cos(sq) + ptDrawOrg.x;
				ptDraw.y = r * sin(sq) + ptDrawOrg.z;
				pDC->LineTo(ptDraw);
			}
		}
		else {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
				ptDraw.x = r * cos(sq) + ptDrawOrg.x;
				ptDraw.y = r * sin(sq) + ptDrawOrg.z;
				pDC->LineTo(ptDraw);
			}
		}
		ptDraw.x = r * cos(eq) + ptDrawOrg.x;
		ptDraw.y = r * sin(eq) + ptDrawOrg.z;
		pDC->LineTo(ptDraw);
		break;

	case NCDRAWVIEW_YZ:
		r *= m_dFactorYZ;
		dHelical  *= m_dFactorYZ;
		ptDrawOrg *= m_dFactorYZ;
		ptDraw.x = ptDrawOrg.y;
		ptDraw.y = r * sin(sq) + ptDrawOrg.z;
		pDC->MoveTo(ptDraw);
		if ( GetG03() ) {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				ptDraw.x += dHelical;
				ptDraw.y  = r * sin(sq) + ptDrawOrg.z;
				pDC->LineTo(ptDraw);
			}
		}
		else {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
				ptDraw.x += dHelical;
				ptDraw.y  = r * sin(sq) + ptDrawOrg.z;
				pDC->LineTo(ptDraw);
			}
		}
		ptDraw.x = m_ptValE.y * m_dFactorYZ;
		ptDraw.y = r * sin(eq) + ptDrawOrg.z;
		pDC->LineTo(ptDraw);
		break;
	}
}

void CNCcircle::DrawG19(ENNCDRAWVIEW enDraw, CDC* pDC) const	// YZ_PLANE
{
	float		sq, eq,
				dHelical = m_dHelicalStep, r = fabs(m_r);
	CPoint3F	pt3D, ptDrawOrg(m_ptOrg);
	CPointF		ptDraw;

	tie(sq, eq) = GetSqEq();

	switch ( enDraw ) {
	case NCDRAWVIEW_XYZ:
		r *= m_dFactor;
		dHelical  *= m_dFactor;
		ptDrawOrg *= m_dFactor;
		pt3D.x = ptDrawOrg.x;
		pt3D.y = r * cos(sq) + ptDrawOrg.y;
		pt3D.z = r * sin(sq) + ptDrawOrg.z;
		ptDraw = pt3D.PointConvert();
		pDC->MoveTo(ptDraw);
		if ( GetG03() ) {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				pt3D.x += dHelical;
				pt3D.y = r * cos(sq) + ptDrawOrg.y;
				pt3D.z = r * sin(sq) + ptDrawOrg.z;
				ptDraw = pt3D.PointConvert();
				pDC->LineTo(ptDraw);
			}
		}
		else {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
				pt3D.x += dHelical;
				pt3D.y = r * cos(sq) + ptDrawOrg.y;
				pt3D.z = r * sin(sq) + ptDrawOrg.z;
				ptDraw = pt3D.PointConvert();
				pDC->LineTo(ptDraw);
			}
		}
		pt3D.x = m_ptValE.x * m_dFactor;
		pt3D.y = r * cos(eq) + ptDrawOrg.y;
		pt3D.z = r * sin(eq) + ptDrawOrg.z;
		ptDraw = pt3D.PointConvert();
		pDC->LineTo(ptDraw);
		break;

	case NCDRAWVIEW_XY:
		r *= m_dFactorXY;
		dHelical  *= m_dFactorXY;
		ptDrawOrg *= m_dFactorXY;
		ptDraw.x = ptDrawOrg.x;
		ptDraw.y = r * cos(sq) + ptDrawOrg.y;
		pDC->MoveTo(ptDraw);
		if ( GetG03() ) {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				ptDraw.x += dHelical;
				ptDraw.y  = r * cos(sq) + ptDrawOrg.y;
				pDC->LineTo(ptDraw);
			}
		}
		else {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
				ptDraw.x += dHelical;
				ptDraw.y  = r * cos(sq) + ptDrawOrg.y;
				pDC->LineTo(ptDraw);
			}
		}
		ptDraw.x = m_ptValE.x * m_dFactorXY;
		ptDraw.y = r * cos(eq) + ptDrawOrg.y;
		pDC->LineTo(ptDraw);
		break;

	case NCDRAWVIEW_XZ:
		r *= m_dFactorXZ;
		dHelical  *= m_dFactorXZ;
		ptDrawOrg *= m_dFactorXZ;
		ptDraw.x = ptDrawOrg.x;
		ptDraw.y = r * sin(sq) + ptDrawOrg.z;
		pDC->MoveTo(ptDraw);
		if ( GetG03() ) {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				ptDraw.x += dHelical;
				ptDraw.y  = r * sin(sq) + ptDrawOrg.z;
				pDC->LineTo(ptDraw);
			}
		}
		else {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
				ptDraw.x += dHelical;
				ptDraw.y  = r * sin(sq) + ptDrawOrg.z;
				pDC->LineTo(ptDraw);
			}
		}
		ptDraw.x = m_ptValE.x * m_dFactorXZ;
		ptDraw.y = r * sin(eq) + ptDrawOrg.z;
		pDC->LineTo(ptDraw);
		break;

	case NCDRAWVIEW_YZ:	// Don't ARC()
		r *= m_dFactorYZ;
		ptDrawOrg *= m_dFactorYZ;
		ptDraw.x = r * cos(sq) + ptDrawOrg.y;
		ptDraw.y = r * sin(sq) + ptDrawOrg.z;
		pDC->MoveTo(ptDraw);
		if ( GetG03() ) {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				ptDraw.x = r * cos(sq) + ptDrawOrg.y;
				ptDraw.y = r * sin(sq) + ptDrawOrg.z;
				pDC->LineTo(ptDraw);
			}
		}
		else {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
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

inline void _SetMaxRect_(const CPointF& pt, CRectF& rc)
{
	if ( rc.left > pt.x )
		rc.left = pt.x;
	if ( rc.right < pt.x )
		rc.right = pt.x;
	if ( rc.top > pt.y )
		rc.top = pt.y;
	if ( rc.bottom < pt.y )
		rc.bottom = pt.y;
}

CRect3F CNCcircle::GetMaxRect(void) const
{
	// 外接する四角形
	CRect3F	rcResult;
	CRectF	rcMax;
	CPointF	pt;
	float	sq, eq, r = fabs(m_r);

	tie(sq, eq) = GetSqEq();

	if ( fabs(eq-sq) >= RAD(270.0f) ) {
		rcMax.SetRect(-r, -r, r, r);
	}
	else {
		pt.x = r * cos(sq);
		pt.y = r * sin(sq);
		rcMax.SetRect(pt, 0, 0);
		if ( GetG03() ) {
			for ( sq+=ARCSTEP; sq<eq; sq+=ARCSTEP ) {
				pt.x = r * cos(sq);
				pt.y = r * sin(sq);
				_SetMaxRect_(pt, rcMax);
			}
		}
		else {
			for ( sq-=ARCSTEP; sq>eq; sq-=ARCSTEP ) {
				pt.x = r * cos(sq);
				pt.y = r * sin(sq);
				_SetMaxRect_(pt, rcMax);
			}
		}
		pt.x = r * cos(eq);
		pt.y = r * sin(eq);
		_SetMaxRect_(pt, rcMax);
	}
	rcMax.NormalizeRect();

	// 空間占有矩形座標設定
	switch ( GetPlane() ) {
	case XY_PLANE:
		rcMax.OffsetRect(m_ptOrg.GetXY());
		rcResult.left	= rcMax.left;
		rcResult.top	= rcMax.top;
		rcResult.right	= rcMax.right;
		rcResult.bottom	= rcMax.bottom;
		rcResult.low	= m_ptValS.z;
		rcResult.high	= m_ptValE.z;
		break;
	case XZ_PLANE:
		rcMax.OffsetRect(m_ptOrg.GetXZ());
		rcResult.left	= rcMax.left;
		rcResult.top	= m_ptValS.y;
		rcResult.right	= rcMax.right;
		rcResult.bottom	= m_ptValE.y;
		rcResult.low	= rcMax.top;
		rcResult.high	= rcMax.bottom;
		break;
	case YZ_PLANE:
		rcMax.OffsetRect(m_ptOrg.GetYZ());
		rcResult.left	= m_ptValS.x;
		rcResult.top	= rcMax.left;
		rcResult.right	= m_ptValE.x;
		rcResult.bottom	= rcMax.right;
		rcResult.low	= rcMax.top;
		rcResult.high	= rcMax.bottom;
		break;
	}
	rcResult.NormalizeRect();

#ifdef _DEBUGOLD
	printf("CNCcircle::GetMaxRect()\n");
	printf(" rcResult(left, top   )=(%f, %f)\n", rcResult.left, rcResult.top);
	printf(" rcResult(right,bottom)=(%f, %f)\n", rcResult.right, rcResult.bottom);
	printf(" rcResult(high, low   )=(%f, %f)\n", rcResult.high, rcResult.low);
#endif

	return rcResult;
}

tuple<BOOL, CPointF, float, float> CNCcircle::CalcRoundPoint(const CNCdata* pNext, float r) const
{
	BOOL		bResult = FALSE;
	int			nResult;
	float		rr1, rr2, xa, ya, r0 = fabs(m_r);
	CPointF		pt, pts, pte, pt1, pt2;

	pts = GetPlaneValueOrg(m_ptOrg, m_ptValE);

	if ( pNext->GetType() == NCDARCDATA ) {
		const CNCcircle* pCircle = static_cast<const CNCcircle *>(pNext);
		pte = GetPlaneValueOrg(pCircle->GetOrg(), m_ptValE);
		float	r1, r2, rn = fabs(pCircle->GetR());
		// 自身の円と他方の接線で自身の±r符号を計算（交点への進入は反対回転）
		optional<float> dResult = _CalcRoundPoint_CircleInOut(pts, pte, GetG03(), !pCircle->GetG03(), r);
		if ( dResult )
			r1 = *dResult;
		else
			return make_tuple(bResult, pt, rr1, rr2);
		// 他方の円と自身の接線で他方の±r符号を計算
		dResult = _CalcRoundPoint_CircleInOut(pte, pts, !pCircle->GetG03(), GetG03(), r);
		if ( dResult )
			r2 = *dResult;
		else
			return make_tuple(bResult, pt, rr1, rr2);
		// ２つの円の交点を求める -> 解が２つないと面取り出来ないと判断する
		rr1 = r0 + r1;		rr2 = rn + r2;
		tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_CC(pts, pte, rr1, rr2);
		if ( nResult != 2 )
			return make_tuple(bResult, pt, rr1, rr2);
		// 解の選択
		float	sx = fabs(pts.x), sy = fabs(pts.y), ex = fabs(pte.x), ey = fabs(pte.y);
		if ( (sx<NCMIN && ex<NCMIN) || (sy<NCMIN && ey<NCMIN) ||
				(sx>NCMIN && ex>NCMIN && fabs(pts.y/pts.x - pte.y/pte.x)<NCMIN) ) {
			// 中心が同一線上にあるとき，接線と符号が同じ方を選択
			if ( sy < NCMIN )
				pt = (GetG03() ? pts.x : -pts.x) * pt1.y > 0 ? pt1 : pt2;
			else
				pt = (GetG03() ? -pts.y : pts.y) * pt1.x > 0 ? pt1 : pt2;
		}
		else
			pt = pt1.x*pt1.x+pt1.y*pt1.y < pt2.x*pt2.x+pt2.y*pt2.y ? pt1 : pt2;
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
		rr1 = _hypotf(xa, ya);
		if ( r2 > 0 ) {
			xa = (pt.x*rn+pte.x*r2) / rr2;
			ya = (pt.y*rn+pte.y*r2) / rr2;
		}
		else {
			r2 = fabs(r2);
			xa = (pt.x*rn-pte.x*r2) / rr2;
			ya = (pt.y*rn-pte.y*r2) / rr2;
		}
		rr2 = _hypotf(xa, ya);
		bResult = TRUE;
	}
	else {
		pte = GetPlaneValueOrg(pNext->GetEndPoint(), m_ptValE);
		// ｵﾌｾｯﾄ方向を決定
		int	nOffset = _CalcRoundPoint_OffsetFlag(pte, pts, GetG03());
		if ( nOffset == 0 )
			return make_tuple(bResult, pt, rr1, rr2);
		// ｵﾌｾｯﾄ分平行移動させた交点を求める
		optional<CPointF> ptResult = ::CalcOffsetIntersectionPoint_LC(pte, pts,	// 直線からのｱﾌﾟﾛｰﾁで
								r0, r, r, !GetG03(), nOffset>0);						// 回転方向を反転
		if ( ptResult ) {
			pt = *ptResult;
			// 面取りに相当するC値の計算
			if ( nOffset > 0 )
				nOffset = GetG03() ? 1 : -1;
			else
				nOffset = GetG03() ? -1 : 1;
			rr1 = r0 + r*nOffset;
			if ( nOffset > 0 ) {
				// 内分点
				xa = (pt.x*r0+pts.x*r) / rr1;
				ya = (pt.y*r0+pts.y*r) / rr1;
			}
			else {
				// 外分点
				xa = (pt.x*r0-pts.x*r) / rr1;
				ya = (pt.y*r0-pts.y*r) / rr1;
			}
			rr1 = _hypotf(xa, ya);
			rr2 = sqrt(pt.x*pt.x + pt.y*pt.y - r*r);
			bResult = TRUE;
		}
	}

	if ( bResult )
		pt += GetPlaneValue(m_ptValE);

	return make_tuple(bResult, pt, rr1, rr2);
}

optional<CPointF> CNCcircle::SetChamferingPoint(BOOL bStart, float c)
{
	CPoint3F	ptOrg3D( bStart ? m_ptValS : m_ptValE );
	CPointF		pt, ptOrg1, ptOrg2, pt1, pt2;
	float		pa, pb, ps;

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
	if ( m_eq - m_sq > PI ) {
		// 180°を超えるときは直径と比較
		if ( c >= fabs(m_r)*2 )
			return boost::none;
	}
	else {
		// 180°未満の場合は弦の長さと比較
		if ( c >= pt1.hypot(&pt2) )
			return boost::none;
	}

	// ２つの円の交点を求める -> 解が２つないと面取り出来ないと判断する
	int	nResult;
	tie(nResult, pt1, pt2) = ::CalcIntersectionPoint_CC(ptOrg1, ptOrg2, fabs(m_r), c);
	if ( nResult != 2 )
		return boost::none;

	// 時計回りの場合，始角と終角が入れ替わっているので一時的に元に戻す
	if ( !GetG03() )
		invoke_swap(m_sq, m_eq);

	// 解の選択
	ps = bStart ? m_sq : m_eq;
	if ( (pa=ptOrg1.arctan(pt1)) < 0.0f )
		pa += PI2;
	if ( (pb=ptOrg1.arctan(pt2)) < 0.0f )
		pb += PI2;
	// 180度以上の差は補正
	if ( fabs(ps-pa) > PI ) {
		if ( ps > pa )
			ps -= PI2;
		else
			pa -= PI2;
	}

	// 始角・終角に近い解の選択と自分自身の点を更新
	if ( bStart ) {
		if ( GetG03() ) {	// 反時計回りの時は，大きい方を選択
			if ( ps < pa ) {
				pt = pt1;
			}
			else {
				pt = pt2;
				pa = pb;
			}
		}
		else {				// 時計回りの時は，元角よりも小さい方を
			if ( ps > pa ) {
				pt = pt1;
			}
			else {
				pt = pt2;
				pa = pb;
			}
		}
		SetPlaneValue(pt, m_ptValS);
		m_sq = pa;
	}
	else {
		if ( GetG03() ) {
			if ( ps > pa ) {
				pt = pt1;
			}
			else {
				pt = pt2;
				pa = pb;
			}
		}
		else {
			if ( ps < pa ) {
				pt = pt1;
			}
			else {
				pt = pt2;
				pa = pb;
			}
		}
		SetPlaneValue(pt, m_ptValE);
		m_pRead->m_ptValOrg = m_ptValE;
		if ( m_pRead->m_pG68 ) {
			// m_ptValE はG68回転済み座標のため回転を元に戻してｵﾌｾｯﾄ減算
			m_pRead->m_pG68->dRound = -m_pRead->m_pG68->dRound;
			CalcG68Round(m_pRead->m_pG68, m_pRead->m_ptValOrg);
			m_pRead->m_pG68->dRound = -m_pRead->m_pG68->dRound;
		}
		m_pRead->m_ptValOrg -= m_pRead->m_ptOffset;
		m_eq = pa;
		m_pt2D = m_ptValE.PointConvert();
	}

	// 角度補正
	if ( !GetG03() )
		invoke_swap(m_sq, m_eq);
	ps = ::RoundUp(DEG(m_sq));
	while ( ps >= ::RoundUp(DEG(m_eq)) )
		m_eq += PI2;

	return pt;
}

float CNCcircle::CalcBetweenAngle(const CNCdata* pNext) const
{
	// ２線の交点(自身の終点)が原点になるように補正
	CPointF		pt( GetPlaneValueOrg(m_ptOrg, m_ptValE) ), pt1, pt2;

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
	return GetG03() ? -1 : 1;
}

optional<CPointF> CNCcircle::CalcPerpendicularPoint
	(ENPOINTORDER enPoint, float r, int nSign) const
{
	const CPoint3F	pts( enPoint==STARTPOINT ? m_ptValS : m_ptValE );
	// 始点終点関係なく 回転方向ｘ補正符号
	// ptsと中心の傾きを計算して半径±r
	CPointF	pt( GetPlaneValueOrg(pts, m_ptOrg) );
	float	q = pt.arctan(),
			rr = fabs(m_r) + r * CalcOffsetSign() * nSign;
	CPointF	pt1(rr*cos(q), rr*sin(q));
	pt1 += GetPlaneValue(m_ptOrg);

	return pt1;
}

optional<CPointF> CNCcircle::CalcOffsetIntersectionPoint
	(const CNCdata* pNext, float t1, float t2, BOOL bLeft) const
{
	BOOL	bResult = FALSE;
	// ２線の交点(自身の終点)が原点になるように補正
	CPointF	pt;

	if ( pNext->GetType() == NCDARCDATA ) {
		const CNCcircle* pCircle = static_cast<const CNCcircle *>(pNext);
		CPointF	pto1( GetPlaneValueOrg(m_ptOrg, m_ptValE) ),
				pto2( GetPlaneValueOrg(pCircle->GetOrg(), m_ptValE) ),
				p1, p2;
		int		k1 = CalcOffsetSign(),
				k2 = pCircle->CalcOffsetSign();
		if ( !bLeft ) {
			k1 = -k1;
			k2 = -k2;
		}
		float	r1 = fabs(m_r)+t1*k1, r2 = fabs(pCircle->GetR())+t2*k2;
		// 同一円か判断
		if ( pto1.IsMatchPoint(&pto2) && fabs(r1-r2)<NCMIN ) {
			// 円の交点は求められないので、単純ｵﾌｾｯﾄ座標計算
			float	q = GetG03() ? m_eq : m_sq;	// 終点角度
			CPointF	pto( GetPlaneValue(m_ptOrg) );
			pt.x = r1 * cos(q) + pto.x;
			pt.y = r1 * sin(q) + pto.y;
			return pt;	// m_ptValE の補正不要
		}
		else {
			// ｵﾌｾｯﾄ分半径を調節して２つの円の交点を求める
			int		nResult;
			tie(nResult, p1, p2) = ::CalcIntersectionPoint_CC(pto1, pto2, r1, r2);
			// 解の選択
			if ( nResult > 1 ) {
				pt = GAPCALC(p1) < GAPCALC(p2) ? p1 : p2;
				bResult = TRUE;
			}
			else if ( nResult > 0 ) {
				pt = p1;
				bResult = TRUE;
			}
		}
	}
	else {
		// ｵﾌｾｯﾄ分平行移動させた交点を求める
		CPointF	pt1( GetPlaneValueOrg(pNext->GetEndPoint(), m_ptValE) ),
				pt2( GetPlaneValueOrg(m_ptOrg, m_ptValE) );
		// 線からのｱﾌﾟﾛｰﾁで回転方向を反転
		optional<CPointF> ptResult = ::CalcOffsetIntersectionPoint_LC(pt1, pt2,
								fabs(m_r), t1, t2, !GetG03(), !bLeft);
		if ( ptResult ) {
			pt = *ptResult;
			bResult = TRUE;
		}
	}

	// 原点補正
	if ( bResult ) {
		pt += GetPlaneValue(m_ptValE);
		return pt;
	}

	return boost::none;
}

optional<CPointF> CNCcircle::CalcOffsetIntersectionPoint2
	(const CNCdata* pNext, float r, BOOL bLeft) const
{
	int		k;
	// ２線の交点(自身の終点)が原点になるように補正
	CPointF	pt, pt1, pt2;

	// 始点接線座標
	pt = GetPlaneValueOrg(m_ptOrg, m_ptValE);
	k = -CalcOffsetSign();
	pt1.x = -pt.y * k;
	pt1.y =  pt.x * k;

	// 他方の座標計算
	if ( pNext->GetType() == NCDARCDATA ) {
		const CNCcircle* pCircle = static_cast<const CNCcircle *>(pNext);
		pt = GetPlaneValueOrg(pCircle->GetOrg(), m_ptValE);
		k = pCircle->CalcOffsetSign();
		pt2.x = -pt.y * k;
		pt2.y =  pt.x * k;
	}
	else {
		pt2 = GetPlaneValueOrg(pNext->GetEndPoint(), m_ptValE);
	}

	// 直線同士のｵﾌｾｯﾄ交点計算
	optional<CPointF> ptResult = ::CalcOffsetIntersectionPoint_LL(pt1, pt2, r, r, bLeft);
	// 原点補正
	if ( ptResult ) {
		pt  = *ptResult;
		pt += GetPlaneValue(m_ptValE);
		return pt;
	}
	return boost::none;
}

void CNCcircle::SetCorrectPoint(ENPOINTORDER enPoint, const CPointF& ptSrc, float rr)
{
	CPoint3F&	ptVal = enPoint==STARTPOINT ? m_ptValS : m_ptValE;	// 参照型
	CPointF		pt;

	SetPlaneValue(ptSrc, ptVal);
	pt = GetPlaneValueOrg(ptVal, m_ptOrg);

	// 角度調整
	if ( enPoint == STARTPOINT ) {
		float&	q = GetG03() ? m_sq : m_eq;	// 参照型
		if ( (q=pt.arctan()) < 0.0f )
			q += PI2;
	}
	else {
		m_r = copysign(fabs(m_r)+rr, m_r);		// 終点の時だけ半径補正
		float&	q = GetG03() ? m_eq : m_sq;
		if ( (q=pt.arctan()) < 0.0f )
			q += PI2;
		m_pt2D = m_ptValE.PointConvert();
	}
	float	sq = ::RoundUp(DEG(m_sq));
	while ( sq >= ::RoundUp(DEG(m_eq)) )
		m_eq += PI2;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

// ｵﾌｾｯﾄ方向の決定
int _CalcRoundPoint_OffsetFlag(const CPointF& pts, const CPointF& pto, BOOL bG03)
{
	// 回転符号
	// Issue #12 ただしもう少し検証が必要
	int		k = bG03 ? 1 : -1;		// G02:-90°,G03:+90°
	// 直線の角度
	float	q = pts.arctan();
	// 円の接線
	CPointF	pte(-pto.y*k, pto.x);
	// 直線の角度で補正
	pte.RoundPoint(-q);
	// y の符号でｵﾌｾｯﾄ方向を判断
	if ( fabs(pte.y) < NCMIN )
		k = 0;		// 解なし
	else if ( pte.y > 0.0f )
		k = -1;		// 右側
	else
		k = 1;		// 左側

	return k;
}

// 円同士の内外半径計算
optional<float> _CalcRoundPoint_CircleInOut
	(const CPointF& pts, const CPointF& pte, BOOL bG03, BOOL bG03next, float r)
{
	CPointF	pto;
	float	sx = fabs(pts.x), sy = fabs(pts.y), ex = fabs(pte.x), ey = fabs(pte.y), rr;
	int		k1 = bG03 ? 1 : -1, k2 = bG03next==0 ? 1 : -1;

	// 特殊解の判断
	if ( (sx<NCMIN && ex<NCMIN && pts.y*pte.y>0) ||
			(sy<NCMIN && ey<NCMIN && pts.x*pte.x>0) ||
			(sx>NCMIN && ex>NCMIN && fabs(pts.y/pts.x - pte.y/pte.x)<NCMIN && pts.x*pte.x>0) ) {
		// 中心が同一線上にあり，かつ，x,y の符号が同じとき
		float	l1 = pts.x*pts.x + pts.y*pts.y;
		float	l2 = pte.x*pte.x + pte.y*pte.y;
		if ( fabs(l1 - l2) < NCMIN )	// 距離が等しい==同軌跡円
			return boost::none;
		else if ( l1 > l2 )
			return -r;
		else
			return r;
	}
	// 自身の円と他方の接線で自身の±r符号を計算
	pto.x = -pte.y*k2;	// G02:-90°
	pto.y =  pte.x*k2;	// G03:+90°

	// 線と円弧の場合と考え方(処理方法)は同じ
	if ( fabs(pto.x) < NCMIN && sy < NCMIN )
		rr = copysign(r, pto.y*pts.x*k1);
	else if ( fabs(pto.y) < NCMIN && sx < NCMIN )
		rr = copysign(r, -pto.x*pts.y*k1);
	else
		rr = copysign(r, -(pto.x*pts.x + pto.y*pts.y));

	return rr;
}
