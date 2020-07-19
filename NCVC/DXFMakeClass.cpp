// DXFMakeClass.cpp: CDXFMake クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "NCdata.h"
#include "DXFdata.h"
#include "DocBase.h"
#include "NCDoc.h"
#include "DXFDoc.h"
#include "ViewOption.h"
#include "DXFkeyword.h"
#include "DXFMakeOption.h"
#include "DXFMakeClass.h"
#include "boost/array.hpp"

#ifdef _DEBUG
#define new DEBUG_NEW
//#define	_DEBUGDUMP
#endif

using std::vector;
using namespace boost;

// ｾｸｼｮﾝ名 from ReadDXF.cpp
extern	LPCTSTR	g_szSection[];
	//	"SECTION", "ENDSEC", "EOF"
// 改行 from stdafx.cpp
extern	LPCTSTR	gg_szReturn;	// "\n"
// DXF線種 from ViewOption.cpp
extern	const	PENSTYLE	g_penStyle[];

// よく使う変数や呼び出しの簡略置換
#define	GetFlg(a)		ms_pMakeOpt->GetFlag(a)
#define	GetNum(a)		ms_pMakeOpt->GetNum(a)
#define	GetDbl(a)		ms_pMakeOpt->GetDbl(a)
#define	GetStr(a)		ms_pMakeOpt->GetStr(a)

//////////////////////////////
// 静的変数の初期化

const CDXFMakeOption*	CDXFMake::ms_pMakeOpt = NULL;
PFNMAKEVALUE		CDXFMake::ms_pfnMakeValueLine = NULL;
PFNMAKEVALUE		CDXFMake::ms_pfnMakeValueCircleToLine = NULL;
PFNMAKEVALUECIRCLE	CDXFMake::ms_pfnMakeValueCircle = NULL;
PFNMAKEVALUECYCLE	CDXFMake::ms_pfnMakeValueCycle = NULL;

static	LPCTSTR	pszZero = "0";
static	LPCTSTR	pszWork = "WORK00";

// DXFｲﾝﾃﾞｯｸｽｶﾗｰ
struct INDEXCOLOR
{
	int		n;
	int		r, g, b;
	// 代入補助
	void	set(int nn, int rr, int gg, int bb) {
		n = nn;
		r = rr;		g = gg;		b = bb;
	}
	// sort補助関数
	bool operator<(const INDEXCOLOR& right) const {
		if ( r < right.r )
			return TRUE;
		else if ( r == right.r ) {
			if ( g < right.g )
				return TRUE;
			else if ( g == right.g ) {
				if ( b < right.b )
					return TRUE;
			}
		}
		return FALSE;
	}
#ifdef _DEBUGDUMP
	void	dump() const {
		TRACE("id=%d : rgb=%d,%d,%d\n", n, r, g, b);
	}
#endif
};
static	boost::array<INDEXCOLOR, 256>	_INDEXCOLOR;
static	int		SearchIndexColor(COLORREF);

//////////////////////////////////////////////////////////////////////

static inline CString _GROUPCODE(int nCode)
{
	// ｸﾞﾙｰﾌﾟｺｰﾄﾞを3桁右詰にする
	CString	strResult;
	strResult.Format(IDS_MAKEDXF_CODE, nCode);
	return strResult;
}

// ｾｸｼｮﾝ生成(共通)
static inline CString _MakeSection(enSECNAME enType)
{
	extern	LPCTSTR	g_szSectionName[];
		// "HEADER", "TABLES", "BLOCKS", "ENTITIES"
	ASSERT(enType > SEC_NOSECNAME);
	// ﾊﾟﾀｰﾝ組み立て
	return _GROUPCODE(0)+g_szSection[SEC_SECTION]+gg_szReturn+
			_GROUPCODE(2)+g_szSectionName[enType]+gg_szReturn;
}

static inline CString _MakeEndSec(void)
{
	return _GROUPCODE(0)+g_szSection[SEC_ENDSEC]+gg_szReturn;
}

static inline CString _MakeValue(int nCode, float dVal)
{
	CString	strResult, strFormat;
	strFormat.Format(IDS_MAKENCD_FORMAT, dVal);
	strResult = _GROUPCODE(nCode)+strFormat+gg_szReturn;
	return strResult;
}

static inline CString _MakeValue(DWORD dwFlags, float dVal[])
{
	extern	const	DWORD	g_dwValSet[];
	extern	int		g_nValueGroupCode[];
	CString	strResult;
	for ( int i=0; i<DXFMAXVALUESIZE; i++ ) {
		if ( dwFlags & g_dwValSet[i] )
			strResult += _MakeValue(g_nValueGroupCode[i], dVal[i]);
	}
	return strResult;
}

static inline CString _MakeValue(int nCode, int nVal)
{
	CString	strResult, strFormat;
	strFormat.Format(IDS_MAKEDXF_INTVALUE, nVal);
	strResult = _GROUPCODE(nCode)+strFormat;
	return strResult;
}

static inline CString _MakeLayer(LPCTSTR pszLayer)
{
	return _GROUPCODE(8)+pszLayer+gg_szReturn;
}

static inline CString _MakeFigure(int nType, const CString& strLayer)
{
	extern	LPCTSTR	g_szEntitiesKey[];
		// "POINT", "LINE", "CIRCLE", "ARC", "ELLIPSE", "POLYLINE", "TEXT",
		// "INSERT", "LWPOLYLINE", "VIEWPORT"
	return _GROUPCODE(0)+g_szEntitiesKey[nType]+gg_szReturn+
			_MakeLayer(strLayer);
}

static inline CString _MakeVertex(LPCTSTR pszLayer, const CPointF* pt)
{
	extern	LPCTSTR	g_szPolyline[];
		// "VERTEX", "SEQEND"
	if ( pt ) {
		float	dVal[DXFMAXVALUESIZE];
		dVal[VALUE10] = pt->x;
		dVal[VALUE20] = pt->y;
		return _GROUPCODE(0)+g_szPolyline[0]+gg_szReturn+
				_MakeLayer(pszLayer)+
				_MakeValue(VALFLG_START, dVal);
	}
	else
		return _GROUPCODE(0)+g_szPolyline[1]+gg_szReturn+
				_MakeLayer(pszLayer);
}

static inline int _SetDXFtype(ENPLANE enPlane, const CNCdata* pData)
{
	int		nType;
	if ( pData->GetPlane() != enPlane )
		nType = TYPE_LINE;
	else
		nType = pData->GetValFlags() & (NCD_X|NCD_Y|NCD_Z) ? TYPE_ARC : TYPE_CIRCLE;
	return nType;
}

static inline float _AngleTuning_D(float q)
{
	while ( q < 0 )		q += 360.0;
	while ( 360.0 < q )	q -= 360.0;
	return q;
}

static inline float _AngleTuning_R(float q)
{
	while ( q < 0 )		q += PI2;	// RAD(360.0)
	while ( PI2 < q )	q -= PI2;;
	return q;
}

//////////////////////////////////////////////////////////////////////

static CString MakeValueLine_XY(const CNCdata* pData)
{
	float	dVal[DXFMAXVALUESIZE];
	CPoint3F	pts(pData->GetStartPoint()), pte(pData->GetEndPoint());
	dVal[VALUE10] = pts.x;	dVal[VALUE20] = pts.y;
	dVal[VALUE11] = pte.x;	dVal[VALUE21] = pte.y;
	return _MakeValue(VALFLG_LINE, dVal);
}

static CString MakeValueLine_XZ(const CNCdata* pData)
{
	float	dVal[DXFMAXVALUESIZE];
	CPoint3F	pts(pData->GetStartPoint()), pte(pData->GetEndPoint());
	dVal[VALUE10] = pts.x;	dVal[VALUE20] = pts.z;
	dVal[VALUE11] = pte.x;	dVal[VALUE21] = pte.z;
	return _MakeValue(VALFLG_LINE, dVal);
}

static CString	MakeValueLine_YZ(const CNCdata* pData)
{
	float	dVal[DXFMAXVALUESIZE];
	CPoint3F	pts(pData->GetStartPoint()), pte(pData->GetEndPoint());
	dVal[VALUE10] = pts.y;	dVal[VALUE20] = pts.z;
	dVal[VALUE11] = pte.y;	dVal[VALUE21] = pte.z;
	return _MakeValue(VALFLG_LINE, dVal);
}

static CString MakeValueCircle_XY(const CNCcircle* pData)
{
	float	dVal[DXFMAXVALUESIZE];
	CPoint3F	pto( pData->GetOrg() );
	dVal[VALUE10] = pto.x;	dVal[VALUE20] = pto.y;
	dVal[VALUE40] = fabs( pData->GetR() );
	return _MakeValue(VALFLG_CIRCLE, dVal);
}

static CString MakeValueCircle_XZ(const CNCcircle* pData)
{
	float	dVal[DXFMAXVALUESIZE];
	CPoint3F	pto( pData->GetOrg() );
	dVal[VALUE10] = pto.x;	dVal[VALUE20] = pto.z;
	dVal[VALUE40] = fabs( pData->GetR() );
	return _MakeValue(VALFLG_CIRCLE, dVal);
}

static CString MakeValueCircle_YZ(const CNCcircle* pData)
{
	float	dVal[DXFMAXVALUESIZE];
	CPoint3F	pto( pData->GetOrg() );
	dVal[VALUE10] = pto.y;	dVal[VALUE20] = pto.z;
	dVal[VALUE40] = fabs( pData->GetR() );
	return _MakeValue(VALFLG_CIRCLE, dVal);
}

static CString MakeValueCircleToLine_XY(const CNCdata* pData)
{
	float	dVal[DXFMAXVALUESIZE];
	CRect3F	rc( pData->GetMaxRect() );
	dVal[VALUE10] = rc.left;	dVal[VALUE20] = rc.top;
	dVal[VALUE11] = rc.right;	dVal[VALUE21] = rc.bottom;
	return _MakeValue(VALFLG_LINE, dVal);
}

static CString	MakeValueCircleToLine_XZ(const CNCdata* pData)
{
	float	dVal[DXFMAXVALUESIZE];
	CRect3F	rc( pData->GetMaxRect() );
	dVal[VALUE10] = rc.left;	dVal[VALUE20] = rc.low;
	dVal[VALUE11] = rc.right;	dVal[VALUE21] = rc.high;
	return _MakeValue(VALFLG_LINE, dVal);
}

static CString	MakeValueCircleToLine_YZ(const CNCdata* pData)
{
	float	dVal[DXFMAXVALUESIZE];
	CRect3F	rc( pData->GetMaxRect() );
	dVal[VALUE10] = rc.top;		dVal[VALUE20] = rc.low;
	dVal[VALUE11] = rc.bottom;	dVal[VALUE21] = rc.high;
	return _MakeValue(VALFLG_LINE, dVal);
}

///

static CString	MakeValueDXF(const CDXFpoint* pData, const CPointF& pto)
{
	float	dVal[DXFMAXVALUESIZE];
	CPointF	pt(pData->GetNativePoint(0) + pto);
	dVal[VALUE10] = pt.x;	dVal[VALUE20] = pt.y;
	return _MakeValue(VALFLG_POINT, dVal);
}

static CString	MakeValueDXF(const CDXFline* pData, const CPointF& pto)
{
	float	dVal[DXFMAXVALUESIZE];
	CPointF	pts(pData->GetNativePoint(0) + pto),
			pte(pData->GetNativePoint(1) + pto);
	dVal[VALUE10] = pts.x;	dVal[VALUE20] = pts.y;
	dVal[VALUE11] = pte.x;	dVal[VALUE21] = pte.y;
	return _MakeValue(VALFLG_LINE, dVal);
}

static CString	MakeValueDXF(const CDXFcircle* pData, const CPointF& pto)
{
	float	dVal[DXFMAXVALUESIZE];
	CPointF	ptc(pData->GetCenter() + pto);
	dVal[VALUE10] = ptc.x;	dVal[VALUE20] = ptc.y;
	dVal[VALUE40] = pData->GetR();
	return _MakeValue(VALFLG_CIRCLE, dVal);
}

static CString	MakeValueDXF(const CDXFarc* pData, const CPointF& pto)
{
	float	dVal[DXFMAXVALUESIZE], sq, eq;
	CPointF	ptc(pData->GetCenter() + pto);
	dVal[VALUE10] = ptc.x;	dVal[VALUE20] = ptc.y;
	dVal[VALUE40] = pData->GetR();
	if ( pData->GetRoundOrig() ) {
		sq = DEG(pData->GetStartAngle());
		eq = DEG(pData->GetEndAngle());
	}
	else {
		sq = DEG(pData->GetEndAngle());
		eq = DEG(pData->GetStartAngle());
	}
	dVal[VALUE50] = _AngleTuning_D(sq);
	dVal[VALUE51] = _AngleTuning_D(eq);
	return _MakeValue(VALFLG_ARC, dVal);
}

static CString	MakeValueDXF(const CDXFellipse* pData, const CPointF& pto)
{
	float	dVal[DXFMAXVALUESIZE], sq, eq;
	CPointF	ptc(pData->GetCenter() + pto),
			ptl(pData->GetLongPoint());
	dVal[VALUE10] = ptc.x;	dVal[VALUE20] = ptc.y;
	dVal[VALUE11] = ptl.x;	dVal[VALUE21] = ptl.y;
	dVal[VALUE40] = pData->GetShortMagni();
	if ( pData->GetRoundOrig() ) {
		sq = _AngleTuning_R(pData->GetStartAngle());
		eq = _AngleTuning_R(pData->GetEndAngle());
	}
	else {
		sq = _AngleTuning_R(pData->GetEndAngle());
		eq = _AngleTuning_R(pData->GetStartAngle());
	}
	if ( sq > eq )
		sq -= PI2;	// JW_CAD対策
	dVal[VALUE41] = sq;
	dVal[VALUE42] = eq;
	return _MakeValue(VALFLG_ELLIPSE, dVal);
}

static CString	MakeValueDXF(const CDXFpolyline* pPoly, const CPointF& pto)
{
	CString		strLayer(pPoly->GetParentLayer()->GetLayerName()),
				strResult;
	float		dVal, sq, eq;
	CPointF		pt;
	CDXFdata*	pData;
	CDXFarc*	pArc;

	// 閉じたﾎﾟﾘﾗｲﾝ
	if ( pPoly->IsStartEqEnd() )
		strResult = _MakeValue(70, 1);
	// 頂点処理
	POSITION pos = pPoly->GetFirstVertex();
	while ( pos ) {
		pData = pPoly->GetNextVertex(pos);
		switch ( pData->GetType() ) {
		case DXFPOINTDATA:
			pt = pData->GetNativePoint(0) + pto;
			strResult += _MakeVertex(strLayer, &pt);
			break;
		case DXFARCDATA:
			pArc = static_cast<CDXFarc *>(pData);
			pt = pArc->GetNativePoint(1) + pto;
			sq = _AngleTuning_R(pArc->GetStartAngle());
			eq = _AngleTuning_R(pArc->GetEndAngle());
			dVal = tan( (eq-sq)/4.0f );
			strResult += _MakeVertex(strLayer, &pt) + _MakeValue(42, dVal);
			// 終点分を飛ばす
			ASSERT( pos );
			pData = pPoly->GetNextVertex(pos);
			ASSERT( pData->GetType() == DXFPOINTDATA );
			break;
		}
	}

	// SEQEND
	strResult += _MakeVertex(strLayer, NULL);

	return strResult;
}

static CString	MakeValueDXF(const CDXFtext* pData, const CPointF& pto)
{
	float	dVal[DXFMAXVALUESIZE];
	CPointF	pt(pData->GetNativePoint(0) + pto);
	dVal[VALUE10] = pt.x;	dVal[VALUE20] = pt.y;
	return _MakeValue(VALFLG_POINT, dVal) +
			_GROUPCODE(1) + pData->GetStrValue() + gg_szReturn;
}

//////////////////////////////////////////////////////////////////////
// 構築/消滅

CDXFMake::CDXFMake(enSECNAME nType, const CDocBase* pDoc)
{
	switch ( nType ) {
	case SEC_HEADER:
		MakeSection_Header(pDoc);
		break;
	case SEC_TABLES:
		MakeSection_Tables(pDoc);
		break;
	case SEC_BLOCKS:
		MakeSection_Blocks();
		break;
	case SEC_ENTITIES:
		MakeSection_Entities();
		break;
	case SEC_NOSECTION:
		MakeSection_EOF();
		break;
	}
}

CDXFMake::CDXFMake(const CNCdata* pData, BOOL bCorrect/*=FALSE*/)
{
	// 各平面に対する有効なGxxだけが呼ばれる
	// ｵﾌﾞｼﾞｪｸﾄ種別
	switch ( pData->GetType() ) {
	case NCDLINEDATA:	// 直線補間
		MakeDXF_NCtoLine(static_cast<const CNCline*>(pData), bCorrect);
		break;
	case NCDARCDATA:	// 円弧補間
		if ( bCorrect || GetFlg(MKDX_FLG_OUT_C) )
			MakeDXF_NCtoArc(static_cast<const CNCcircle*>(pData), bCorrect);
		break;
	case NCDCYCLEDATA:	// 固定ｻｲｸﾙ
		MakeDXF_NCtoCycle(static_cast<const CNCcycle*>(pData));
		break;
	}
}

CDXFMake::CDXFMake(const CDXFdata* pData, const CPointF& pt)
{
	CString	strResult(
		_MakeFigure(pData->GetType(), pData->GetParentLayer()->GetLayerName())
	);

	switch ( pData->GetType() ) {
	case DXFPOINTDATA:
		strResult += MakeValueDXF(static_cast<const CDXFpoint *>(pData), pt);
		break;
	case DXFLINEDATA:
		strResult += MakeValueDXF(static_cast<const CDXFline *>(pData), pt);
		break;
	case DXFCIRCLEDATA:
		strResult += MakeValueDXF(static_cast<const CDXFcircle *>(pData), pt);
		break;
	case DXFARCDATA:
		strResult += MakeValueDXF(static_cast<const CDXFarc *>(pData), pt);
		break;
	case DXFELLIPSEDATA:
		strResult += MakeValueDXF(static_cast<const CDXFellipse *>(pData), pt);
		break;
	case DXFPOLYDATA:
		{
			const CDXFpolyline* pPoly = static_cast<const CDXFpolyline *>(pData);
			if ( pPoly->GetVertexCount() <= 1 )
				return;
			if ( pPoly->GetPolyFlag() & DXFPOLY_ELLIPSE ) {
				// POLYLINE解体
				MakeDXF_PolylineDismantle(pPoly, pt);
				return;
			}
			else
				strResult += MakeValueDXF(pPoly, pt);
		}
		break;
	case DXFTEXTDATA:
		strResult += MakeValueDXF(static_cast<const CDXFtext *>(pData), pt);
		break;
	}

	m_strDXFarray.Add( strResult );
}

CDXFMake::CDXFMake(const CPoint3F& pt, float r)
{
	// r!=0.0f <- CDXFDoc::MakeDXF()
	// r==0.0f <- CNCDoc::MakeDXF()
	float	dVal[DXFMAXVALUESIZE];

	// 原点(円)情報出力
	if ( r!=0.0f || GetFlg(MKDX_FLG_ORGCIRCLE) ) {
		// ｵﾌﾞｼﾞｪｸﾄ情報
		m_strDXFarray.Add( MakeDXF_Figure(TYPE_CIRCLE, MKDX_STR_ORIGIN) );
		// 座標値
		if ( r != 0.0f ) {
			dVal[VALUE10] = pt.x;	dVal[VALUE20] = pt.z;
			dVal[VALUE40] = r;
		}
		else {
			switch ( GetNum(MKDX_NUM_PLANE) ) {
			case 1:		// XZ
				dVal[VALUE10] = pt.x;	dVal[VALUE20] = pt.z;
				break;
			case 2:		// YZ
				dVal[VALUE10] = pt.y;	dVal[VALUE20] = pt.z;
				break;
			default:	// XY
				dVal[VALUE10] = pt.x;	dVal[VALUE20] = pt.y;
				break;
			}
			dVal[VALUE40] = GetDbl(MKDX_DBL_ORGLENGTH);
		}
		m_strDXFarray.Add( _MakeValue(VALFLG_CIRCLE, dVal) );
#ifdef _DEBUGDUMP
		dump();
#endif
	}

	// 原点(ｸﾛｽ)情報出力
	if ( r==0.0f && GetFlg(MKDX_FLG_ORGCROSS) ) {
		float	d1 = GetDbl(MKDX_DBL_ORGLENGTH), d2 = 0.0f;
		for ( int i=0; i<2; i++ ) {
			// ｵﾌﾞｼﾞｪｸﾄ情報
			m_strDXFarray.Add( MakeDXF_Figure(TYPE_LINE, MKDX_STR_ORIGIN) );
			// 座標値
			switch ( GetNum(MKDX_NUM_PLANE) ) {
			case 1:		// XZ
				dVal[VALUE10] = pt.x - d1;	dVal[VALUE11] = pt.x + d1;
				dVal[VALUE20] = pt.z - d2;	dVal[VALUE21] = pt.z + d2;
				break;
			case 2:		// YZ
				dVal[VALUE10] = pt.y - d1;	dVal[VALUE11] = pt.y + d1;
				dVal[VALUE20] = pt.z - d2;	dVal[VALUE21] = pt.z + d2;
				break;
			default:	// XY
				dVal[VALUE10] = pt.x - d1;	dVal[VALUE11] = pt.x + d1;
				dVal[VALUE20] = pt.y - d2;	dVal[VALUE21] = pt.y + d2;
				break;
			}
			m_strDXFarray.Add( _MakeValue(VALFLG_LINE, dVal) );
			// 始点終点調整座標入れ替え
			d1 = 0.0f;	d2 = GetDbl(MKDX_DBL_ORGLENGTH);
		}
	}
}

CDXFMake::CDXFMake(const CRectF& rc)
{
	CString	strResult( _MakeFigure(TYPE_POLYLINE, pszWork)+_MakeValue(70, 1) );
	CPointF	pt(rc.TopLeft());

	strResult += _MakeVertex(pszWork, &pt);
	pt.x = rc.right;
	strResult += _MakeVertex(pszWork, &pt);
	pt.y = rc.bottom;
	strResult += _MakeVertex(pszWork, &pt);
	pt.x = rc.left;
	strResult += _MakeVertex(pszWork, &pt) + _MakeVertex(pszWork, NULL);

	m_strDXFarray.Add( strResult );
}

//////////////////////////////////////////////////////////////////////
// ﾒﾝﾊﾞ関数

void CDXFMake::MakeSection_Header(const CDocBase* pDoc)
{
	// ﾍｯﾀﾞｰ変数名
	extern	LPCTSTR	g_szHeader[];
		//	"$ACADVER", "$EXTMIN", "$EXTMAX", "$LIMMIN", "$LIMMAX"
	// ｾｸｼｮﾝ定義
	m_strDXFarray.Add(_MakeSection(SEC_HEADER));
	// HEADERｾｸｼｮﾝ値
	float	dMin[DXFMAXVALUESIZE], dMax[DXFMAXVALUESIZE];
	CRect3F	rc( pDoc->GetMaxRect() );
	switch ( GetNum(MKDX_NUM_PLANE) ) {
	case 1:		// XZ
		dMin[VALUE10] = rc.left;	dMin[VALUE20] = rc.low;
		dMax[VALUE10] = rc.right;	dMax[VALUE20] = rc.high;
		break;
	case 2:		// YZ
		dMin[VALUE10] = rc.top;		dMin[VALUE20] = rc.low;
		dMax[VALUE10] = rc.bottom;	dMax[VALUE20] = rc.high;
		break;
	default:	// XY
		dMin[VALUE10] = rc.left;	dMin[VALUE20] = rc.top;
		dMax[VALUE10] = rc.right;	dMax[VALUE20] = rc.bottom;
		break;
	}
	m_strDXFarray.Add(
		_GROUPCODE(9)+g_szHeader[HEAD_ACADVER]+gg_szReturn+
		_GROUPCODE(1)+"AC1009\n"+
		_GROUPCODE(9)+g_szHeader[HEAD_EXTMIN]+gg_szReturn+
			_MakeValue(VALFLG_START, dMin)+
		_GROUPCODE(9)+g_szHeader[HEAD_EXTMAX]+gg_szReturn+
			_MakeValue(VALFLG_START, dMax)+
		_GROUPCODE(9)+g_szHeader[HEAD_LIMMIN]+gg_szReturn+
			_MakeValue(VALFLG_START, dMin)+
		_GROUPCODE(9)+g_szHeader[HEAD_LIMMAX]+gg_szReturn+
			_MakeValue(VALFLG_START, dMax)
	);
	// ｾｸｼｮﾝ終了
	m_strDXFarray.Add(_MakeEndSec());
}

void CDXFMake::MakeSection_Tables(const CDocBase* pDoc)
{
	// ﾃｰﾌﾞﾙｻﾌﾞｷｰ
	extern	LPCTSTR	g_szTables[];
		//	"TABLE", "ENDTAB",
		//		"LTYPE", "LAYER", "VPORT"
	int		i, j;
	CRect3F	rc( pDoc->GetMaxRect() );
	CPointF	pt;

	// ｾｸｼｮﾝ定義
	m_strDXFarray.Add(_MakeSection(SEC_TABLES));
	// TABLESｾｸｼｮﾝ値
	float	dVal[DXFMAXVALUESIZE], d1, d2;
	CString	strGroup0(_GROUPCODE(0)),
			strGroup2(_GROUPCODE(2)),
			strGroup3(_GROUPCODE(3)),
			strGroup70(_MakeValue(70, 64)),
			strGroup72(_MakeValue(72, 65));
	CString	strTABLE(strGroup0+g_szTables[TABLES_TABLE]+gg_szReturn),
			strENDTAB(strGroup0+g_szTables[TABLES_ENDTAB]+gg_szReturn),
			strLTYPE(g_szTables[TABLEKEY_LTYPE]),
			strLAYER(g_szTables[TABLEKEY_LAYER]),
			strVPORT(g_szTables[TABLEKEY_VPORT]);
	strLTYPE += gg_szReturn;	strLAYER += gg_szReturn;
	strVPORT += gg_szReturn;

	// VPORT画面表示ｴﾘｱ設定
	m_strDXFarray.Add(	
		strTABLE+strGroup2+strVPORT+_MakeValue(70, 1)+
		strGroup0+strVPORT+strGroup2+"*ACTIVE\n"+_MakeValue(70, 0)
	);
	dVal[VALUE10] = 0.0f;	dVal[VALUE20] = 0.0f;
	dVal[VALUE11] = 1.0f;	dVal[VALUE21] = 1.0f;
	m_strDXFarray.Add( _MakeValue(VALFLG_LINE, dVal) );
	switch ( GetNum(MKDX_NUM_PLANE) ) {
	case 1:		// XZ
		dVal[VALUE10] = (rc.right + rc.left) / 2.0f;		// VALUE10は仮代入要素
		dVal[VALUE20] = (rc.high  + rc.low)  / 2.0f;
		d1 = rc.right - rc.left;
		d2 = rc.high  - rc.low;
		dVal[VALUE40] = max( d1, d2 ) * 1.1f;
		break;
	case 2:		// YZ
		dVal[VALUE10] = (rc.bottom + rc.top) / 2.0f;
		dVal[VALUE20] = (rc.high   + rc.low) / 2.0f;
		d1 = rc.bottom - rc.top;
		d2 = rc.high   - rc.low;
		dVal[VALUE40] = max( d1, d2 ) * 1.1f;
		break;
	default:	// XY
		pt = rc.CenterPoint();
		dVal[VALUE10] = pt.x;		dVal[VALUE20] = pt.y;
		d1 = rc.Width();
		d2 = rc.Height();
		dVal[VALUE40] = max( d1, d2 ) * 1.1f;
		break;
	}
	dVal[VALUE41] = 1.5376f;		// 1280x1024縦横比
	dVal[VALUE42] = 50.0f;
	m_strDXFarray.Add(
		_MakeValue(12, dVal[VALUE10]) +
		_MakeValue(22, dVal[VALUE20]) +
		_MakeValue(13,  0.0f) + _MakeValue(23,  0.0f) +
		_MakeValue(14, 10.0f) + _MakeValue(24, 10.0f) +
		_MakeValue(15, 10.0f) + _MakeValue(25, 10.0f) +
		_MakeValue(16,  0.0f) + _MakeValue(26,  0.0f) + _MakeValue(36,  1.0f) +
		_MakeValue(17,  0.0f) + _MakeValue(27,  0.0f) + _MakeValue(37,  1.0f) +
		_MakeValue(VALFLG40|VALFLG41|VALFLG42, dVal) +
		_MakeValue(43,  0.0f) + _MakeValue(44,  0.0f) +
		_MakeValue(50,  0.0f) + _MakeValue(51,  0.0f) +
		_MakeValue(71, 0) + _MakeValue(72, 1000) +
		_MakeValue(73, 1) + _MakeValue(74, 3) +
		_MakeValue(75, 0) + _MakeValue(76, 0) +
		_MakeValue(77, 0) + _MakeValue(78, 0)
	);
	m_strDXFarray.Add(strENDTAB);

	// 線種ﾃｰﾌﾞﾙ数
	m_strDXFarray.Add(	
		strTABLE+strGroup2+strLTYPE+_MakeValue(70, MAXPENSTYLE)
	);
	for ( i=0; i<MAXPENSTYLE; i++ ) {
		dVal[VALUE40] = g_penStyle[i].dDXFpattern;
		m_strDXFarray.Add(
			strGroup0+strLTYPE+strGroup2+g_penStyle[i].lpszDXFname+gg_szReturn+	// 線種名
			strGroup70+strGroup3+g_penStyle[i].lpszDXFpattern+gg_szReturn+		// ﾊﾟﾀｰﾝ
			strGroup72+_MakeValue(73, g_penStyle[i].nDXFdash)+					// ﾀﾞｯｼｭ項目数
			_MakeValue(VALFLG40, dVal)											// ﾊﾟﾀｰﾝ長さ
		);
		for ( j=0; j<g_penStyle[i].nDXFdash; j++ )
			m_strDXFarray.Add( _MakeValue(49, g_penStyle[i].dDXFdash[j]) );
	}
	m_strDXFarray.Add(strENDTAB);

	// TABLESｾｸｼｮﾝ値(ﾚｲﾔ情報)
	VTABLELAYERINFO		v;
	v.push_back( TABLELAYERINFO(pszZero, g_penStyle[0].lpszDXFname, 7) );	// "0"ﾚｲﾔ強制出力
	if ( pDoc->IsKindOf(RUNTIME_CLASS(CDXFDoc)) ) {
		const CDXFDoc* pDXF = static_cast<const CDXFDoc*>(pDoc);
		if ( pDXF->IsDocFlag(DXFDOC_BINDPARENT) ) {
			// ﾜｰｸ矩形用ﾚｲﾔの登録
			const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
			v.push_back( TABLELAYERINFO(
				pszWork,
				g_penStyle[pOpt->GetDxfDrawType(DXFPEN_WORK)].lpszDXFname,
				SearchIndexColor(pOpt->GetDxfDrawColor(DXFCOL_WORK)) )
			);
			// 子ﾄﾞｷｭﾒﾝﾄﾙｰﾌﾟ
			for ( i=0; i<pDXF->GetBindInfoCnt(); i++ )
				MakeSection_TableLayer_DXF(v, pDXF->GetBindInfoData(i)->pDoc);
		}
		else
			MakeSection_TableLayer_DXF(v, pDXF);
	}
	else
		MakeSection_TableLayer_NCD(v);
	// ﾚｲﾔ情報登録
	m_strDXFarray.Add(strTABLE+strGroup2+strLAYER+_MakeValue(70, (int)v.size()));
	BOOST_FOREACH( const auto& e, v ) {
		m_strDXFarray.Add(strGroup0+strLAYER+
			strGroup2+e.strLayer+gg_szReturn+
			_MakeValue(70, 0)+_MakeValue(62, e.nColor)+
			_GROUPCODE(6)+e.strType+gg_szReturn);
	}
	m_strDXFarray.Add(strENDTAB);

	// ｾｸｼｮﾝ終了
	m_strDXFarray.Add(_MakeEndSec());
}

void CDXFMake::MakeSection_TableLayer_NCD(VTABLELAYERINFO& v)
{
	VTABLELAYERINFO::iterator	it;
	CString		strLayer, strType;
	int			i, nColor;

	// 等しいﾚｲﾔ名は統合し、後発ﾃﾞｰﾀで置換
	for ( i=0; i<MKDX_STR_NUMS; i++ ) {
		strLayer = GetStr(i+MKDX_STR_ORIGIN);
		strType  = g_penStyle[GetNum(i+MKDX_NUM_LTYPE_O)].lpszDXFname;
		nColor   = GetNum(i+MKDX_NUM_LCOL_O)+1;
		if ( strLayer.IsEmpty() )
			strLayer = pszZero;
		else
			strLayer.MakeUpper();
		for ( it=v.begin(); it!=v.end(); ++it ) {
			if ( (*it).strLayer == strLayer )
				break;
		}
		if ( it == v.end() )
			v.push_back( TABLELAYERINFO(strLayer, strType, nColor) );
		else {
			(*it).strLayer = strLayer;
			(*it).strType  = strType;
			(*it).nColor   = nColor;
		}
	}
}

void CDXFMake::MakeSection_TableLayer_DXF(VTABLELAYERINFO& v, const CDXFDoc* pDoc)
{
	VTABLELAYERINFO::iterator	it;
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	CLayerData*	pLayer;
	CString		strLayer, strType;
	int			i, nLayer, nColor;

	for ( i=0; i<pDoc->GetLayerCnt(); i++ ) {
		pLayer   = pDoc->GetLayerData(i);
		strLayer = pLayer->GetLayerName();
		nLayer   = pLayer->GetLayerType();
		strType  = g_penStyle[pOpt->GetDxfDrawType(
					(nLayer<DXFCOMLAYER) ? (nLayer+DXFPEN_ORIGIN) : 0)].lpszDXFname;
		nColor   = SearchIndexColor(pOpt->GetDxfDrawColor(nLayer+DXFCOL_ORIGIN));
		if ( strLayer.IsEmpty() )
			strLayer = pszZero;
		else
			strLayer.MakeUpper();
		for ( it=v.begin(); it!=v.end(); ++it ) {
			if ( (*it).strLayer == strLayer )
				break;
		}
		if ( it == v.end() )
			v.push_back( TABLELAYERINFO(strLayer, strType, nColor) );
		else {
			(*it).strLayer = strLayer;
			(*it).strType  = strType;
			(*it).nColor   = nColor;
		}
	}
}

void CDXFMake::MakeSection_Blocks(void)
{
	// ｾｸｼｮﾝ定義
	m_strDXFarray.Add(_MakeSection(SEC_BLOCKS));
	// ｾｸｼｮﾝ終了
	m_strDXFarray.Add(_MakeEndSec());
}

void CDXFMake::MakeSection_Entities(void)
{
	// ｾｸｼｮﾝ定義
	m_strDXFarray.Add(_MakeSection(SEC_ENTITIES));
	// ｾｸｼｮﾝ終了はﾃﾞｰﾀ出力後なので省略
}

void CDXFMake::MakeSection_EOF(void)
{
	// Entitiesｾｸｼｮﾝ終了分
	m_strDXFarray.Add(_MakeEndSec());
	// EOF
	m_strDXFarray.Add(_GROUPCODE(0)+g_szSection[SEC_EOF]+gg_szReturn);
}

//////////////////////////////////////////////////////////////////////

CString	CDXFMake::MakeDXF_Figure(int nType, int nLayer)
{
	CString	strResult( _MakeFigure(nType, GetStr(nLayer)) );

	// 線種と色
	int		nLType, nLCol;
	switch ( nLayer ) {
	case MKDX_STR_ORIGIN:
		nLType = MKDX_NUM_LTYPE_O;
		nLCol  = MKDX_NUM_LCOL_O;
		break;
	case MKDX_STR_CAMLINE:
		nLType = MKDX_NUM_LTYPE_C;
		nLCol  = MKDX_NUM_LCOL_C;
		break;
	case MKDX_STR_MOVE:
		nLType = MKDX_NUM_LTYPE_M;
		nLCol  = MKDX_NUM_LCOL_M;
		break;
	case MKDX_STR_CORRECT:
		nLType = MKDX_NUM_LTYPE_H;
		nLCol  = MKDX_NUM_LCOL_H;
		break;
	}
	if ( nType != TYPE_POINT )
		strResult += _GROUPCODE(6)+g_penStyle[GetNum(nLType)].lpszDXFname+gg_szReturn +
			_MakeValue(62, GetNum(nLCol)+1);

	return strResult;
}

CString CDXFMake::MakeValueCycle
	(const CNCcycle* pData, int nIndex, int nType, ENPLANE enPlane)
{
	const	PTCYCLE*	pCycleInside = pData->GetCycleInside(enPlane+1);
	int		nStart = nIndex - 1;
	float	dVal[DXFMAXVALUESIZE];
	CPointF	pts, pti, ptr;
	CString	strResult;
	if ( GetFlg(MKDX_FLG_OUT_M) )
		strResult = MakeDXF_Figure(TYPE_LINE, MKDX_STR_MOVE);

	// 呼び出し関数の動的決定
	CPointF	(CPoint3F::*pfnGetAxis)(void) const;
	switch ( enPlane ) {
	case XY_PLANE:
		pfnGetAxis = &(CPoint3F::GetXY);
		break;
	case XZ_PLANE:
		pfnGetAxis = &(CPoint3F::GetXZ);
		break;
	case YZ_PLANE:
		pfnGetAxis = &(CPoint3F::GetYZ);
		break;
	}
	
	// ｵﾌﾞｼﾞｪｸﾄの平面と指示平面が等しいかどうか
	if ( pData->GetPlane() == enPlane ) {
		if ( GetFlg(MKDX_FLG_OUT_M) ) {
			// 移動ﾃﾞｰﾀ
			if ( nStart < 0 ) {
				// 現在位置から1点目のｲﾆｼｬﾙ点
				pts = (pData->GetStartPoint().*pfnGetAxis)();
				dVal[VALUE10] = pts.x;
				dVal[VALUE20] = pts.y;
			}
			else {
				dVal[VALUE10] = pCycleInside[nStart].ptI.x;
				dVal[VALUE20] = pCycleInside[nStart].ptI.y;
			}
			dVal[VALUE11] = pCycleInside[nIndex].ptI.x;
			dVal[VALUE21] = pCycleInside[nIndex].ptI.y;
			strResult += _MakeValue(VALFLG_LINE, dVal);
		}
		if ( GetFlg(MKDX_FLG_OUT_C) ) {
			// 切削ﾃﾞｰﾀ(円または実点)
			strResult += MakeDXF_Figure(nType, MKDX_STR_CAMLINE);
			dVal[VALUE10] = pCycleInside[nIndex].ptI.x;
			dVal[VALUE20] = pCycleInside[nIndex].ptI.y;
			DWORD dwFlags = VALFLG_START;
			if ( nType == TYPE_CIRCLE ) {
				dVal[VALUE40] = GetDbl(MKDX_DBL_CYCLER);
				dwFlags |= VALFLG40;
			}
			strResult += _MakeValue(dwFlags, dVal);
		}
	}
	else {
		if ( GetFlg(MKDX_FLG_OUT_M) ) {
			if ( nStart < 0 ) {
				pts = (pData->GetStartPoint().*pfnGetAxis)();
				pti = (pData->GetIPoint().*pfnGetAxis)();
				ptr = (pData->GetRPoint().*pfnGetAxis)();
				dVal[VALUE10] = pts.x;
				dVal[VALUE20] = pts.y;
				dVal[VALUE11] = pti.x;
				dVal[VALUE21] = pti.y;
				strResult += _MakeValue(VALFLG_LINE, dVal);
				if ( pti != ptr ) {
					// ｲﾆｼｬﾙ点からR点
					strResult += MakeDXF_Figure(TYPE_LINE, MKDX_STR_MOVE);
					dVal[VALUE10] = pti.x;
					dVal[VALUE20] = pti.y;
					dVal[VALUE11] = ptr.x;
					dVal[VALUE21] = ptr.y;
					strResult += _MakeValue(VALFLG_LINE, dVal);
				}
			}
			else {
				pts = pCycleInside[nStart].ptI;
				pti = pCycleInside[nIndex].ptI;
				ptr = pCycleInside[nIndex].ptR;
				dVal[VALUE10] = pts.x;
				dVal[VALUE20] = pts.y;
				dVal[VALUE11] = pti.x;
				dVal[VALUE21] = pti.y;
				strResult += _MakeValue(VALFLG_LINE, dVal);
				if ( pti != ptr ) {
					strResult += MakeDXF_Figure(TYPE_LINE, MKDX_STR_MOVE);
					dVal[VALUE10] = pti.x;
					dVal[VALUE20] = pti.y;
					dVal[VALUE11] = ptr.x;
					dVal[VALUE21] = ptr.y;
					strResult += _MakeValue(VALFLG_LINE, dVal);
				}
			}
		}
		if ( GetFlg(MKDX_FLG_OUT_C) ) {
			// 切削ﾃﾞｰﾀ
			strResult += MakeDXF_Figure(TYPE_LINE, MKDX_STR_CAMLINE);
			dVal[VALUE10] = pCycleInside[nIndex].ptR.x;
			dVal[VALUE20] = pCycleInside[nIndex].ptR.y;
			dVal[VALUE11] = pCycleInside[nIndex].ptC.x;
			dVal[VALUE21] = pCycleInside[nIndex].ptC.y;
			strResult += _MakeValue(VALFLG_LINE, dVal);
		}
	}

	return strResult;
}

void CDXFMake::MakeDXF_NCtoLine(const CNCline* pData, BOOL bCorrect)
{
	CString	strResult;

	// ｵﾌﾞｼﾞｪｸﾄ情報(早送り or 切削送り)
	if ( bCorrect )
		strResult = MakeDXF_Figure(TYPE_LINE, MKDX_STR_CORRECT);
	else {
		if ( pData->GetGcode() == 0 ) {
			if ( GetFlg(MKDX_FLG_OUT_M) )
				strResult = MakeDXF_Figure(TYPE_LINE, MKDX_STR_MOVE);
		}
		else {
			if ( GetFlg(MKDX_FLG_OUT_C) )
				strResult = MakeDXF_Figure(TYPE_LINE, MKDX_STR_CAMLINE);
		}
	}

	if ( !strResult.IsEmpty() ) {
		m_strDXFarray.Add(strResult);
		// 座標値
		m_strDXFarray.Add( (*ms_pfnMakeValueLine)(pData) );
	}
}

void CDXFMake::MakeDXF_NCtoArc(const CNCcircle* pData, BOOL bCorrect)
{
	int		nType;

	// ｵﾌﾞｼﾞｪｸﾄ種別
	switch ( GetNum(MKDX_NUM_PLANE) ) {
	case 1:		// XZ
		nType = _SetDXFtype(XZ_PLANE, pData);	// ↑
		break;
	case 2:		// YZ
		nType = _SetDXFtype(YZ_PLANE, pData);
		break;
	default:	// XY
		nType = _SetDXFtype(XY_PLANE, pData);
		break;
	}
	// ｵﾌﾞｼﾞｪｸﾄ情報
	m_strDXFarray.Add( MakeDXF_Figure(nType,
		bCorrect ? MKDX_STR_CORRECT : MKDX_STR_CAMLINE) );
	// 座標値
	m_strDXFarray.Add( nType == TYPE_LINE ? 
		(*ms_pfnMakeValueCircleToLine)(pData) : (*ms_pfnMakeValueCircle)(pData) );
	// 円弧のみ角度の追加
	if ( nType == TYPE_ARC ) {
		float	dVal[DXFMAXVALUESIZE];
		// CNCcircle::AngleTuning() にて常に反時計回り
		dVal[VALUE50] = DEG(pData->GetStartAngle());
		dVal[VALUE51] = DEG(pData->GetEndAngle());
		while ( dVal[VALUE50] > 360.0 )
			dVal[VALUE50] -= 360.0;
		while ( dVal[VALUE51] > 360.0 )
			dVal[VALUE51] -= 360.0;
		m_strDXFarray.Add( _MakeValue(VALFLG50|VALFLG51, dVal) );
	}
}

void CDXFMake::MakeDXF_NCtoCycle(const CNCcycle* pData)
{
	if ( pData->GetDrawCnt() > 0 ) {
		for ( int i=0; i<pData->GetDrawCnt(); i++ )
			m_strDXFarray.Add( (this->*ms_pfnMakeValueCycle)(pData, i) );
	}
	else if ( pData->GetStartPoint()!=pData->GetEndPoint() && GetFlg(MKDX_FLG_OUT_M) ) {
		// L0 でも移動がある場合
		m_strDXFarray.Add( MakeDXF_Figure(TYPE_LINE, MKDX_STR_MOVE) );
		// CNCline として処理
		m_strDXFarray.Add( (*ms_pfnMakeValueLine)(pData) );
	}
}

CString CDXFMake::MakeValueCycle_XY_Circle(const CNCcycle* pData, int nIndex)
{
	return MakeValueCycle(pData, nIndex, TYPE_CIRCLE, XY_PLANE);
}

CString CDXFMake::MakeValueCycle_XZ_Circle(const CNCcycle* pData, int nIndex)
{
	return MakeValueCycle(pData, nIndex, TYPE_CIRCLE, XZ_PLANE);
}

CString CDXFMake::MakeValueCycle_YZ_Circle(const CNCcycle* pData, int nIndex)
{
	return MakeValueCycle(pData, nIndex, TYPE_CIRCLE, YZ_PLANE);
}

CString CDXFMake::MakeValueCycle_XY_Point(const CNCcycle* pData, int nIndex)
{
	return MakeValueCycle(pData, nIndex, TYPE_POINT, XY_PLANE);
}

CString CDXFMake::MakeValueCycle_XZ_Point(const CNCcycle* pData, int nIndex)
{
	return MakeValueCycle(pData, nIndex, TYPE_POINT, XZ_PLANE);
}

CString CDXFMake::MakeValueCycle_YZ_Point(const CNCcycle* pData, int nIndex)
{
	return MakeValueCycle(pData, nIndex, TYPE_POINT, YZ_PLANE);
}

void CDXFMake::MakeDXF_PolylineDismantle(const CDXFpolyline* pPoly, const CPointF& pto)
{
	// 楕円が含まれるPOLYLINE(ﾌﾞﾛｯｸによる拡大)は
	// DXFで再現できないため、個別図形に解体して出力
	CString		strResult, strLayer(pPoly->GetParentLayer()->GetLayerName());
	float		dVal[DXFMAXVALUESIZE];
	CPointF		pt;
	CDXFdata*	pData;

	POSITION pos = pPoly->GetFirstVertex();
	pData = pPoly->GetNextVertex(pos);
	pt = pData->GetNativePoint(0) + pto;
	// ２点目からﾙｰﾌﾟ
	while ( pos ) {
		pData = pPoly->GetNextVertex(pos);
		switch ( pData->GetType() ) {
		case DXFPOINTDATA:
			dVal[VALUE10] = pt.x;	dVal[VALUE20] = pt.y;
			pt = pData->GetNativePoint(0) + pto;
			dVal[VALUE11] = pt.x;	dVal[VALUE21] = pt.y;
			strResult = _MakeFigure(TYPE_LINE, strLayer) + _MakeValue(VALFLG_LINE, dVal);
			m_strDXFarray.Add( strResult );
			break;
		case DXFARCDATA:
			strResult = _MakeFigure(TYPE_ARC, strLayer) + MakeValueDXF(static_cast<CDXFarc*>(pData), pto);
			m_strDXFarray.Add( strResult );
			pt = pData->GetNativePoint(1);
			// 終点分を飛ばす
			ASSERT( pos );
			pData = pPoly->GetNextVertex(pos);
			ASSERT( pData->GetType() == DXFPOINTDATA );
			break;
		case DXFELLIPSEDATA:
			strResult = _MakeFigure(TYPE_ELLIPSE, strLayer) + MakeValueDXF(static_cast<CDXFellipse*>(pData), pto);
			m_strDXFarray.Add( strResult );
			pt = pData->GetNativePoint(1);
			// 終点分を飛ばす
			ASSERT( pos );
			pData = pPoly->GetNextVertex(pos);
			ASSERT( pData->GetType() == DXFPOINTDATA );
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////

void CDXFMake::SetStaticOption(const CDXFMakeOption* pDXFMake)
{
	ms_pMakeOpt = pDXFMake;
	switch ( GetNum(MKDX_NUM_PLANE) ) {
	case 1:		// XZ
		ms_pfnMakeValueLine			= &MakeValueLine_XZ;
		ms_pfnMakeValueCircle		= &MakeValueCircle_XZ;
		ms_pfnMakeValueCircleToLine	= &MakeValueCircleToLine_XZ;
		ms_pfnMakeValueCycle = GetNum(MKDX_NUM_CYCLE) == 0 ?
			&(CDXFMake::MakeValueCycle_XZ_Circle) : &(CDXFMake::MakeValueCycle_XZ_Point);
		break;
	case 2:		// YZ
		ms_pfnMakeValueLine			= &MakeValueLine_YZ;
		ms_pfnMakeValueCircle		= &MakeValueCircle_YZ;
		ms_pfnMakeValueCircleToLine	= &MakeValueCircleToLine_YZ;
		ms_pfnMakeValueCycle = GetNum(MKDX_NUM_CYCLE) == 0 ?
			&(CDXFMake::MakeValueCycle_YZ_Circle) : &(CDXFMake::MakeValueCycle_YZ_Point);
		break;
	default:	// XY
		ms_pfnMakeValueLine			= &MakeValueLine_XY;
		ms_pfnMakeValueCircle		= &MakeValueCircle_XY;
		ms_pfnMakeValueCircleToLine	= &MakeValueCircleToLine_XY;
		ms_pfnMakeValueCycle = GetNum(MKDX_NUM_CYCLE) == 0 ?
			&(CDXFMake::MakeValueCycle_XY_Circle) : &(CDXFMake::MakeValueCycle_XY_Point);
		break;
	}
}

void InitialColorIndex(void)
{
	// AutoCADの色パレット
	_INDEXCOLOR[0].set  (  7, 255,255,255);	// dummy
	_INDEXCOLOR[1].set  (  1, 255,0,0);
	_INDEXCOLOR[2].set  (  2, 255,255,0);
	_INDEXCOLOR[3].set  (  3, 0,255,0);
	_INDEXCOLOR[4].set  (  4, 0,255,255);
	_INDEXCOLOR[5].set  (  5, 0,0,255);
	_INDEXCOLOR[6].set  (  6, 255,0,255);
	_INDEXCOLOR[7].set  (  7, 0,0,0);
	_INDEXCOLOR[8].set  (  8, 128,128,128);
	_INDEXCOLOR[9].set  (  9, 192,192,192);
	_INDEXCOLOR[10].set ( 10, 255,0,0);
	_INDEXCOLOR[11].set ( 11, 255,127,127);
	_INDEXCOLOR[12].set ( 12, 204,0,0);
	_INDEXCOLOR[13].set ( 13, 204,102,102);
	_INDEXCOLOR[14].set ( 14, 153,0,0);
	_INDEXCOLOR[15].set ( 15, 153,76,76);
	_INDEXCOLOR[16].set ( 16, 127,0,0);
	_INDEXCOLOR[17].set ( 17, 127,63,63);
	_INDEXCOLOR[18].set ( 18, 76,0,0);
	_INDEXCOLOR[19].set ( 19, 76,38,38);
	_INDEXCOLOR[20].set ( 20, 255,63,0);
	_INDEXCOLOR[21].set ( 21, 255,159,127);
	_INDEXCOLOR[22].set ( 22, 204,51,0);
	_INDEXCOLOR[23].set ( 23, 204,127,102);
	_INDEXCOLOR[24].set ( 24, 153,38,0);
	_INDEXCOLOR[25].set ( 25, 153,95,76);
	_INDEXCOLOR[26].set ( 26, 127,31,0);
	_INDEXCOLOR[27].set ( 27, 127,79,63);
	_INDEXCOLOR[28].set ( 28, 76,19,0);
	_INDEXCOLOR[29].set ( 29, 76,47,38);
	_INDEXCOLOR[30].set ( 30, 255,127,0);
	_INDEXCOLOR[31].set ( 31, 255,191,127);
	_INDEXCOLOR[32].set ( 32, 204,102,0);
	_INDEXCOLOR[33].set ( 33, 204,153,102);
	_INDEXCOLOR[34].set ( 34, 153,76,0);
	_INDEXCOLOR[35].set ( 35, 153,114,76);
	_INDEXCOLOR[36].set ( 36, 127,63,0);
	_INDEXCOLOR[37].set ( 37, 127,95,63);
	_INDEXCOLOR[38].set ( 38, 76,38,0);
	_INDEXCOLOR[39].set ( 39, 76,57,38);
	_INDEXCOLOR[40].set ( 40, 255,191,0);
	_INDEXCOLOR[41].set ( 41, 255,223,127);
	_INDEXCOLOR[42].set ( 42, 204,153,0);
	_INDEXCOLOR[43].set ( 43, 204,178,102);
	_INDEXCOLOR[44].set ( 44, 153,114,0);
	_INDEXCOLOR[45].set ( 45, 153,133,76);
	_INDEXCOLOR[46].set ( 46, 127,95,0);
	_INDEXCOLOR[47].set ( 47, 127,111,63);
	_INDEXCOLOR[48].set ( 48, 76,57,0);
	_INDEXCOLOR[49].set ( 49, 76,66,38);
	_INDEXCOLOR[50].set ( 50, 255,255,0);
	_INDEXCOLOR[51].set ( 51, 255,255,127);
	_INDEXCOLOR[52].set ( 52, 204,204,0);
	_INDEXCOLOR[53].set ( 53, 204,204,102);
	_INDEXCOLOR[54].set ( 54, 153,153,0);
	_INDEXCOLOR[55].set ( 55, 153,153,76);
	_INDEXCOLOR[56].set ( 56, 127,127,0);
	_INDEXCOLOR[57].set ( 57, 127,127,63);
	_INDEXCOLOR[58].set ( 58, 76,76,0);
	_INDEXCOLOR[59].set ( 59, 76,76,38);
	_INDEXCOLOR[60].set ( 60, 191,255,0);
	_INDEXCOLOR[61].set ( 61, 223,255,127);
	_INDEXCOLOR[62].set ( 62, 153,204,0);
	_INDEXCOLOR[63].set ( 63, 178,204,102);
	_INDEXCOLOR[64].set ( 64, 114,153,0);
	_INDEXCOLOR[65].set ( 65, 133,153,76);
	_INDEXCOLOR[66].set ( 66, 95,127,0);
	_INDEXCOLOR[67].set ( 67, 111,127,63);
	_INDEXCOLOR[68].set ( 68, 57,76,0);
	_INDEXCOLOR[69].set ( 69, 66,76,38);
	_INDEXCOLOR[70].set ( 70, 127,255,0);
	_INDEXCOLOR[71].set ( 71, 191,255,127);
	_INDEXCOLOR[72].set ( 72, 102,204,0);
	_INDEXCOLOR[73].set ( 73, 153,204,102);
	_INDEXCOLOR[74].set ( 74, 76,153,0);
	_INDEXCOLOR[75].set ( 75, 114,153,76);
	_INDEXCOLOR[76].set ( 76, 63,127,0);
	_INDEXCOLOR[77].set ( 77, 95,127,63);
	_INDEXCOLOR[78].set ( 78, 38,76,0);
	_INDEXCOLOR[79].set ( 79, 57,76,38);
	_INDEXCOLOR[80].set ( 80, 63,255,0);
	_INDEXCOLOR[81].set ( 81, 159,255,127);
	_INDEXCOLOR[82].set ( 82, 51,204,0);
	_INDEXCOLOR[83].set ( 83, 127,204,102);
	_INDEXCOLOR[84].set ( 84, 38,153,0);
	_INDEXCOLOR[85].set ( 85, 94,153,76);
	_INDEXCOLOR[86].set ( 86, 31,127,0);
	_INDEXCOLOR[87].set ( 87, 79,127,63);
	_INDEXCOLOR[88].set ( 88, 19,76,0);
	_INDEXCOLOR[89].set ( 89, 47,76,38);
	_INDEXCOLOR[90].set ( 90, 0,255,0);
	_INDEXCOLOR[91].set ( 91, 127,255,127);
	_INDEXCOLOR[92].set ( 92, 0,204,0);
	_INDEXCOLOR[93].set ( 93, 102,204,102);
	_INDEXCOLOR[94].set ( 94, 0,153,0);
	_INDEXCOLOR[95].set ( 95, 76,153,76);
	_INDEXCOLOR[96].set ( 96, 0,127,0);
	_INDEXCOLOR[97].set ( 97, 63,127,63);
	_INDEXCOLOR[98].set ( 98, 0,76,0);
	_INDEXCOLOR[99].set ( 99, 38,76,38);
	_INDEXCOLOR[100].set(100, 0,255,63);
	_INDEXCOLOR[101].set(101, 127,255,159);
	_INDEXCOLOR[102].set(102, 0,204,51);
	_INDEXCOLOR[103].set(103, 102,204,127);
	_INDEXCOLOR[104].set(104, 0,153,38);
	_INDEXCOLOR[105].set(105, 76,153,95);
	_INDEXCOLOR[106].set(106, 0,127,31);
	_INDEXCOLOR[107].set(107, 63,127,79);
	_INDEXCOLOR[108].set(108, 0,76,19);
	_INDEXCOLOR[109].set(109, 38,76,47);
	_INDEXCOLOR[110].set(110, 0,255,127);
	_INDEXCOLOR[111].set(111, 127,255,191);
	_INDEXCOLOR[112].set(112, 0,204,102);
	_INDEXCOLOR[113].set(113, 102,204,153);
	_INDEXCOLOR[114].set(114, 0,153,76);
	_INDEXCOLOR[115].set(115, 76,153,114);
	_INDEXCOLOR[116].set(116, 0,127,63);
	_INDEXCOLOR[117].set(117, 63,127,95);
	_INDEXCOLOR[118].set(118, 0,76,38);
	_INDEXCOLOR[119].set(119, 38,76,57);
	_INDEXCOLOR[120].set(120, 0,255,191);
	_INDEXCOLOR[121].set(121, 127,255,223);
	_INDEXCOLOR[122].set(122, 0,204,153);
	_INDEXCOLOR[123].set(123, 102,204,178);
	_INDEXCOLOR[124].set(124, 0,153,114);
	_INDEXCOLOR[125].set(125, 76,153,133);
	_INDEXCOLOR[126].set(126, 0,127,95);
	_INDEXCOLOR[127].set(127, 63,127,111);
	_INDEXCOLOR[128].set(128, 0,76,57);
	_INDEXCOLOR[129].set(129, 38,76,66);
	_INDEXCOLOR[130].set(130, 0,255,255);
	_INDEXCOLOR[131].set(131, 127,255,255);
	_INDEXCOLOR[132].set(132, 0,204,204);
	_INDEXCOLOR[133].set(133, 102,204,204);
	_INDEXCOLOR[134].set(134, 0,153,153);
	_INDEXCOLOR[135].set(135, 76,153,153);
	_INDEXCOLOR[136].set(136, 0,127,127);
	_INDEXCOLOR[137].set(137, 63,127,127);
	_INDEXCOLOR[138].set(138, 0,76,76);
	_INDEXCOLOR[139].set(139, 38,76,76);
	_INDEXCOLOR[140].set(140, 0,191,255);
	_INDEXCOLOR[141].set(141, 127,223,255);
	_INDEXCOLOR[142].set(142, 0,153,204);
	_INDEXCOLOR[143].set(143, 102,178,204);
	_INDEXCOLOR[144].set(144, 0,114,153);
	_INDEXCOLOR[145].set(145, 76,133,153);
	_INDEXCOLOR[146].set(146, 0,95,127);
	_INDEXCOLOR[147].set(147, 63,111,127);
	_INDEXCOLOR[148].set(148, 0,57,76);
	_INDEXCOLOR[149].set(149, 38,66,76);
	_INDEXCOLOR[150].set(150, 0,127,255);
	_INDEXCOLOR[151].set(151, 127,191,255);
	_INDEXCOLOR[152].set(152, 0,102,204);
	_INDEXCOLOR[153].set(153, 102,153,204);
	_INDEXCOLOR[154].set(154, 0,76,153);
	_INDEXCOLOR[155].set(155, 76,114,153);
	_INDEXCOLOR[156].set(156, 0,63,127);
	_INDEXCOLOR[157].set(157, 63,95,127);
	_INDEXCOLOR[158].set(158, 0,38,76);
	_INDEXCOLOR[159].set(159, 38,57,76);
	_INDEXCOLOR[160].set(160, 0,63,255);
	_INDEXCOLOR[161].set(161, 127,159,255);
	_INDEXCOLOR[162].set(162, 0,51,204);
	_INDEXCOLOR[163].set(163, 102,127,204);
	_INDEXCOLOR[164].set(164, 0,38,153);
	_INDEXCOLOR[165].set(165, 76,95,153);
	_INDEXCOLOR[166].set(166, 0,31,127);
	_INDEXCOLOR[167].set(167, 63,79,127);
	_INDEXCOLOR[168].set(168, 0,19,76);
	_INDEXCOLOR[169].set(169, 38,47,76);
	_INDEXCOLOR[170].set(170, 0,0,255);
	_INDEXCOLOR[171].set(171, 127,127,255);
	_INDEXCOLOR[172].set(172, 0,0,204);
	_INDEXCOLOR[173].set(173, 102,102,204);
	_INDEXCOLOR[174].set(174, 0,0,153);
	_INDEXCOLOR[175].set(175, 76,76,153);
	_INDEXCOLOR[176].set(176, 0,0,127);
	_INDEXCOLOR[177].set(177, 63,63,127);
	_INDEXCOLOR[178].set(178, 0,0,76);
	_INDEXCOLOR[179].set(179, 38,38,76);
	_INDEXCOLOR[180].set(180, 63,0,255);
	_INDEXCOLOR[181].set(181, 159,127,255);
	_INDEXCOLOR[182].set(182, 51,0,204);
	_INDEXCOLOR[183].set(183, 127,102,204);
	_INDEXCOLOR[184].set(184, 38,0,153);
	_INDEXCOLOR[185].set(185, 95,76,153);
	_INDEXCOLOR[186].set(186, 31,0,127);
	_INDEXCOLOR[187].set(187, 79,63,127);
	_INDEXCOLOR[188].set(188, 19,0,76);
	_INDEXCOLOR[189].set(189, 47,38,76);
	_INDEXCOLOR[190].set(190, 127,0,255);
	_INDEXCOLOR[191].set(191, 191,127,255);
	_INDEXCOLOR[192].set(192, 102,0,204);
	_INDEXCOLOR[193].set(193, 153,102,204);
	_INDEXCOLOR[194].set(194, 76,0,153);
	_INDEXCOLOR[195].set(195, 114,76,153);
	_INDEXCOLOR[196].set(196, 63,0,127);
	_INDEXCOLOR[197].set(197, 95,63,127);
	_INDEXCOLOR[198].set(198, 38,0,76);
	_INDEXCOLOR[199].set(199, 57,38,76);
	_INDEXCOLOR[200].set(200, 191,0,255);
	_INDEXCOLOR[201].set(201, 223,127,255);
	_INDEXCOLOR[202].set(202, 153,0,204);
	_INDEXCOLOR[203].set(203, 178,102,204);
	_INDEXCOLOR[204].set(204, 114,0,153);
	_INDEXCOLOR[205].set(205, 133,76,153);
	_INDEXCOLOR[206].set(206, 95,0,127);
	_INDEXCOLOR[207].set(207, 111,63,127);
	_INDEXCOLOR[208].set(208, 57,0,76);
	_INDEXCOLOR[209].set(209, 66,38,76);
	_INDEXCOLOR[210].set(210, 255,0,255);
	_INDEXCOLOR[211].set(211, 255,127,255);
	_INDEXCOLOR[212].set(212, 204,0,204);
	_INDEXCOLOR[213].set(213, 204,102,204);
	_INDEXCOLOR[214].set(214, 153,0,153);
	_INDEXCOLOR[215].set(215, 153,76,153);
	_INDEXCOLOR[216].set(216, 127,0,127);
	_INDEXCOLOR[217].set(217, 127,63,127);
	_INDEXCOLOR[218].set(218, 76,0,76);
	_INDEXCOLOR[219].set(219, 76,38,76);
	_INDEXCOLOR[220].set(220, 255,0,191);
	_INDEXCOLOR[221].set(221, 255,127,223);
	_INDEXCOLOR[222].set(222, 204,0,153);
	_INDEXCOLOR[223].set(223, 204,102,178);
	_INDEXCOLOR[224].set(224, 153,0,114);
	_INDEXCOLOR[225].set(225, 153,76,133);
	_INDEXCOLOR[226].set(226, 127,0,95);
	_INDEXCOLOR[227].set(227, 127,63,111);
	_INDEXCOLOR[228].set(228, 76,0,57);
	_INDEXCOLOR[229].set(229, 76,38,66);
	_INDEXCOLOR[230].set(230, 255,0,127);
	_INDEXCOLOR[231].set(231, 255,127,191);
	_INDEXCOLOR[232].set(232, 204,0,102);
	_INDEXCOLOR[233].set(233, 204,102,153);
	_INDEXCOLOR[234].set(234, 153,0,76);
	_INDEXCOLOR[235].set(235, 153,76,114);
	_INDEXCOLOR[236].set(236, 127,0,63);
	_INDEXCOLOR[237].set(237, 127,63,95);
	_INDEXCOLOR[238].set(238, 76,0,38);
	_INDEXCOLOR[239].set(239, 76,38,57);
	_INDEXCOLOR[240].set(240, 255,0,63);
	_INDEXCOLOR[241].set(241, 255,127,159);
	_INDEXCOLOR[242].set(242, 204,0,51);
	_INDEXCOLOR[243].set(243, 204,102,127);
	_INDEXCOLOR[244].set(244, 153,0,38);
	_INDEXCOLOR[245].set(245, 153,76,95);
	_INDEXCOLOR[246].set(246, 127,0,31);
	_INDEXCOLOR[247].set(247, 127,63,79);
	_INDEXCOLOR[248].set(248, 76,0,19);
	_INDEXCOLOR[249].set(249, 76,38,47);
	_INDEXCOLOR[250].set(240, 51,51,51);
	_INDEXCOLOR[251].set(251, 91,91,91);
	_INDEXCOLOR[252].set(252, 132,132,132);
	_INDEXCOLOR[253].set(253, 173,173,173);
	_INDEXCOLOR[254].set(254, 214,214,214);
	_INDEXCOLOR[255].set(255, 255,255,255);
	// COLORREFで並べ替え
//	std::sort(_INDEXCOLOR.begin(), _INDEXCOLOR.end());
	boost::sort(_INDEXCOLOR);	// boost/range/algorithm.hpp
#ifdef _DEBUGDUMP
	BOOST_FOREACH(const auto& e, _INDEXCOLOR) {
		e.dump();
	}
#endif
}

int SearchIndexColor(COLORREF col)
{
	if ( col == RGB(255,255,255) )
		col = RGB(0,0,0);	// change white

	int	r = GetRValue(col), g = GetGValue(col), b = GetBValue(col),
		ref1, ref2, brk_r = _INDEXCOLOR[0].r, brk_g = _INDEXCOLOR[0].g;
	auto	it_r = _INDEXCOLOR.begin(), it_g = _INDEXCOLOR.begin(),
			it_t = _INDEXCOLOR.begin();

	for (auto it=_INDEXCOLOR.begin(); it!=_INDEXCOLOR.end(); ++it ) {
		if ( r == (*it).r ) {
			if ( g == (*it).g ) {
				if ( b == (*it).b ) {
					return (*it).n;
				}
				else if ( b < (*it).b ) {
					if ( (b - (*prior(it)).b) < ((*it).b - b) )
						return (*prior(it)).n;
					else
						return (*it).n;
				}
			}
			else if ( g < (*it).g ) {
				if ( (g - brk_g) < ((*it).g - g) ) {
					it_t = it_g;
					ref1 = brk_g;
				}
				else {
					it_t = it;
					ref1 = (*it).g;
				}
				for ( ; it_t!=_INDEXCOLOR.end() && ref1==(*it_t).g; ++it_t ) {
					if ( b == (*it_t).b ) {
						return (*it_t).n;
					}
					else if ( b < (*it_t).b ) {
						if ( (b - (*prior(it_t)).b) < ((*it_t).b - b) )
							return (*prior(it_t)).n;
						else
							return (*it_t).n;
					}
				}
				return (*prior(it_t)).n;
			}
			else if ( brk_g != (*it).g ) {
				it_g = it;
				brk_g = (*it).g;
			}
		}
		else if ( r < (*it).r ) {
			if ( (r - brk_r) < ((*it).r - r) ) {
				it_t = it_r;
				ref1 = brk_r;
			}
			else {
				it_t = it;
				ref1 = (*it).r;
			}
			for ( ; it_t!=_INDEXCOLOR.end() && ref1==(*it_t).r; ++it_t ) {
				if ( g == (*it_t).g ) {
					ref2 = (*it_t).g;
					break;
				}
				else if ( g < (*it_t).g ) {
					if ( (g - brk_g) < ((*it_t).g - g) ) {
						it_t = it_g;
						ref2 = brk_g;
					}
					else {
						ref2 = (*it_t).g;
					}
					break;
				}
				else if ( brk_g != (*it_t).g ) {
					it_g = it_t;
					brk_g = (*it_t).g;
				}
			}
			for ( ; it_t!=_INDEXCOLOR.end() && ref1==(*it_t).r && ref2==(*it_t).g; ++it_t ) {
				if ( b == (*it_t).b ) {
					return (*it_t).n;
				}
				else if ( b < (*it_t).b ) {
					if ( (b - (*prior(it_t)).b) < ((*it_t).b - b) )
						return (*prior(it_t)).n;
					else
						return (*it_t).n;
				}
			}
			return (*prior(it_t)).n;
		}
		else if ( brk_r != (*it).r ) {
			it_r = it;
			brk_r = (*it).r;
		}
	}
	return 7;	// white
}

#ifdef _DEBUGDUMP
void CDXFMake::dump(void)
{
	for ( int i=0; i<m_strDXFarray.GetSize(); i++ )
		TRACE( m_strDXFarray[i]+'\n' );
}
#endif