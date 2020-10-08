// ReadDXF.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "DXFOption.h"
#include "DXFdata.h"
#include "DXFDoc.h"
#include "DXFBlock.h"
#include "DXFkeyword.h"
#include "boost/lambda/lambda.hpp"

#ifdef _DEBUG
#define new DEBUG_NEW
//#define	_DEBUG_ARGV
static	ULONGLONG	g_dbgReadLine;
#endif

using namespace boost;

/*
	定義は DXFkeyword.h
		以下のDXFｷｰﾜｰﾄﾞは，DXFMakeClass.cpp でも使用
*/

// ｸﾞﾙｰﾌﾟｺｰﾄﾞ
extern	int		g_nGroupCode[] = {
	0, 1, 2, 3, 6, 8, 9, 70
};
extern	int		g_nValueGroupCode[] = {
	10, 20, 11, 21,
	40, 41, 42, 50, 51,
	210, 220, 230
};
extern	const	DWORD	g_dwValSet[] = {
	VALFLG10, VALFLG20, VALFLG11, VALFLG21,
	VALFLG40, VALFLG41, VALFLG42,
	VALFLG50, VALFLG51,
	VALFLG210, VALFLG220, VALFLG230
};

// ｾｸｼｮﾝ名
extern	LPCTSTR	g_szSection[] = {
	"SECTION", "ENDSEC", "EOF"
};
extern	LPCTSTR	g_szSectionName[] = {
	"HEADER", "TABLES", "BLOCKS", "ENTITIES"
};

// ﾍｯﾀﾞｰ変数名
extern	LPCTSTR	g_szHeader[] = {
	"$ACADVER", "$EXTMIN", "$EXTMAX", "$LIMMIN", "$LIMMAX"
};
// ﾃｰﾌﾞﾙｻﾌﾞｷｰ
extern	LPCTSTR	g_szTables[] = {
	"TABLE", "ENDTAB",
		"LTYPE", "LAYER", "VPORT"
};
// ﾌﾞﾛｯｸｻﾌﾞｷｰ
extern	LPCTSTR	g_szBlocks[] = {
	"BLOCK", "ENDBLK"
};
// ｴﾝﾃｨﾃｨｷｰﾜｰﾄﾞ
extern	LPCTSTR	g_szEntitiesKey[] = {
	"POINT", "LINE", "CIRCLE", "ARC", "ELLIPSE", "POLYLINE", "TEXT",
	"INSERT", "LWPOLYLINE", "VIEWPORT"
};
// Polylineｷｰﾜｰﾄﾞ
extern	LPCTSTR	g_szPolyline[] = {
	"VERTEX", "SEQEND"
};

//
static	int				g_nGroup;
static	CString			g_strOrder,
						g_strMissOrder;	// 未ｻﾎﾟｰﾄのｷｰﾜｰﾄﾞ一時保管
static	CMapStringToPtr	g_strMissEntiMap,	// ｻﾎﾟｰﾄしていないENTITIESｷｰﾜｰﾄﾞﾘｽﾄ
						g_strMissBlckMap;	// 見つからなかったﾌﾞﾛｯｸ名
static	CTypedPtrMap<CMapStringToPtr, CString, CDXFBlockData*>
						g_strBlockMap;	// BLOCKSｾｸｼｮﾝﾃﾞｰﾀ一時領域
										// ｷｰ検索高速化のためCMapｸﾗｽを使用
static	const	CDXFOption*		g_pOpt;	// DXFｵﾌﾟｼｮﾝ
static	float			g_dValue[DXFMAXVALUESIZE];
static	DWORD			g_dwValueFlg;
static	CString			g_strValue;

static	CDXFBlockData*	g_pBkData;		// BLOCK要素
static	CDXFpolyline*	g_pPolyline;	// Polylineﾃﾞｰﾀの先行生成
										// VERTEX溜まってからﾄﾞｷｭﾒﾝﾄﾃﾞｰﾀとして登録
static	BOOL	g_bVertex,		// Polylineの各頂点処理中
				g_bPuff;		// ふくらみ情報処理中
static	float	g_dPuff;		// 前のVERTEXのふくらみ値
static	CPointF	g_ptPuff;		// ふくらみ情報を計算するための前回位置
static	enENTITIESTYPE	g_nType;// TYPE_XXX
static	int		g_nBlock,		// (-1:未処理, 0:Block基点待ち, 1:Block処理中)
				g_nLayer;		// ﾚｲﾔ情報
static	CString	g_strLayer,		// ﾚｲﾔ名
				g_strBlock;		// ﾌﾞﾛｯｸ名

// g_strBlockMapをﾊﾞｯｸｸﾞﾗｳﾝﾄﾞで消去するｽﾚｯﾄﾞ
static	CCriticalSection	g_csRemoveBlockMap;
static	UINT	RemoveBlockMapThread(LPVOID);

//
static	BOOL	HeaderProcedure(void);
static	BOOL	EntitiesProcedure(CDXFDoc*);
static	void	SetEntitiesFromBlock(CDXFDoc*, CDXFBlockData*);
static	void	SetEntitiesInfo(CDXFDoc*);
static	BOOL	BlocksProcedure(CDXFDoc*);
static	BOOL	PolylineProcedure(CDXFDoc*);
static	BOOL	PolylineEndProcedure(CDXFDoc*);
static	BOOL	LWPolylineProcedure(CDXFDoc*, BOOL);

/////////////////////////////////////////////////////////////////////////////
//	ReadDXF() 補助関数

static inline ULONGLONG _DubleRead(CStdioFile& fp)
{
	CString		strBuf;
	ULONGLONG	dwResult = 0;
	BOOL		bResult = fp.ReadString(strBuf);

	if ( bResult ) {
#ifdef _DEBUG
		g_dbgReadLine++;
#endif
		dwResult = strBuf.GetLength() + 2;	// CR+LF分
		g_nGroup = atoi(strBuf);	// atoi()ではｾﾞﾛか失敗かわからん
//		g_nGroup = lexical_cast<int>(LPCTSTR(strBuf));		// 厳格杉
//		g_nGroup = lexical_cast<int>(LPCTSTR(strBuf.Trim()));
		// 命令に続く値を読み込み
		bResult = fp.ReadString(strBuf);
		if ( bResult ) {
#ifdef _DEBUG
			g_dbgReadLine++;
#endif
			dwResult += strBuf.GetLength() + 2;
			g_strOrder = strBuf.Trim();
		}
		else
			g_strOrder.Empty();
	}
	else {
		g_nGroup = -1;
		g_strOrder.Empty();
	}

	return dwResult;
}

static inline void _ClearValue(void)
{
	ZEROCLR(g_dValue);	// g_dValue[i++]=0.0
	g_dwValueFlg = 0;
}

static inline int _SetValue(void)
{
	for ( int i=0; i<SIZEOF(g_dValue); i++ ) {
		if ( g_nGroup == g_nValueGroupCode[i] ) {
			g_dValue[i] = (float)atof(LPCTSTR(g_strOrder));
			g_dwValueFlg |= g_dwValSet[i];
			return i;
		}
	}
	if ( g_nGroup == g_nGroupCode[GROUP1] ) {
		g_strValue = g_strOrder;
		return 0;
	}
	return -1;
}

static inline enSECTION _SectionCheck(void)
{
	if ( g_nGroup != g_nGroupCode[GROUP0] )
		return SEC_NOSECTION;
	// EOF除く検索(の条件があってboost::find_if使えない！！)
	auto f = std::find_if(begin(g_szSection), end(g_szSection)-1, lambda::_1==g_strOrder);
	return f==end(g_szSection)-1 ? SEC_NOSECTION : (enSECTION)(f-g_szSection);
}

static inline enSECNAME _SectionNameCheck(void)
{
	if ( g_nGroup != g_nGroupCode[GROUP2] )
		return SEC_NOSECNAME;
	auto f = find_if(g_szSectionName, lambda::_1==g_strOrder);
	return f==end(g_szSectionName) ? SEC_NOSECNAME : (enSECNAME)(f-g_szSectionName);
}

static inline void _ArbitraryAxis(CPointF& pt)	// 任意の軸のｱﾙｺﾞﾘｽﾞﾑ
{
	float	ax[NCXYZ], ay[NCXYZ];
	CPointF	ptResult;

	if ( fabs(g_dValue[VALUE210])<(1.0/64.0) && fabs(g_dValue[VALUE220])<(1.0/64.0) ) {
		ax[NCA_X] =  g_dValue[VALUE230];
		ax[NCA_Y] =  0.0f;
		ax[NCA_Z] = -g_dValue[VALUE210];
	}
	else {
		ax[NCA_X] = -g_dValue[VALUE220];
		ax[NCA_Y] =  g_dValue[VALUE210];
		ax[NCA_Z] =  0.0f;
	}
	ay[NCA_X] = g_dValue[VALUE220]*ax[NCA_Z] - g_dValue[VALUE230]*ax[NCA_Y];
	ay[NCA_Y] = g_dValue[VALUE230]*ax[NCA_X] - g_dValue[VALUE210]*ax[NCA_Z];
	ay[NCA_Z] = g_dValue[VALUE210]*ax[NCA_Y] - g_dValue[VALUE220]*ax[NCA_X];

	// 基準軸からﾏﾄﾘｸｽ計算(Z値は無視)
	ptResult.x = pt.x*ax[NCA_X] + pt.y*ax[NCA_Y];
	ptResult.y = pt.x*ay[NCA_X] + pt.y*ay[NCA_Y];

	pt = ptResult;
}

static inline BOOL _SetDxfArgv(LPDXFPARGV lpPoint)
{
#ifdef _DEBUG_ARGV
	printf("SetDxfArgv() Point Layer=%s\n", lpPoint->pLayer ? LPCTSTR(lpPoint->pLayer->GetLayerName()) : "?");
#endif
	if ( g_dwValueFlg & VALFLG_POINT ) {
		CPointF	pt(g_dValue[VALUE10], g_dValue[VALUE20]);
		if ( g_dwValueFlg & VALFLG_PLANE )
			_ArbitraryAxis(pt);		// OCS -> WCS 座標変換
		lpPoint->c = pt;
#ifdef _DEBUG_ARGV
		printf("      cx=%f cy=%f\n", lpPoint->c.x, lpPoint->c.y);
#endif
		return TRUE;
	}
	else {
#ifdef _DEBUG_ARGV
		printf("      error cx|cy\n");
#endif
		return FALSE;
	}
}

static inline BOOL _SetDxfArgv(LPDXFLARGV lpLine)
{
#ifdef _DEBUG_ARGV
	printf("SetDxfArgv() Line Layer=%s\n", lpLine->pLayer ? LPCTSTR(lpLine->pLayer->GetLayerName()) : "?");
#endif
	if ( g_dwValueFlg & VALFLG_LINE ) {
		CPointF	pts(g_dValue[VALUE10], g_dValue[VALUE20]),
				pte(g_dValue[VALUE11], g_dValue[VALUE21]);
		if ( g_dwValueFlg & VALFLG_PLANE ) {
			_ArbitraryAxis(pts);
			_ArbitraryAxis(pte);
		}
		lpLine->s = pts;
		lpLine->e = pte;
#ifdef _DEBUG_ARGV
		printf("     sx=%f sy=%f ex=%f ey=%f\n", 
			lpLine->s.x, lpLine->s.y, lpLine->e.x, lpLine->e.y);
#endif
		return TRUE;
	}
	else {
#ifdef _DEBUG_ARGV
		printf("     error sx|sy|ex|ey\n"); 
#endif
		return FALSE;
	}
}

static inline BOOL _SetDxfArgv(LPDXFCARGV lpCircle)
{
#ifdef _DEBUG_ARGV
	printf("SetDxfArgv() Circle Layer=%s\n", lpCircle->pLayer ? LPCTSTR(lpCircle->pLayer->GetLayerName()) : "?");
#endif
	if ( g_dwValueFlg & VALFLG_CIRCLE ) {
		CPointF	pt(g_dValue[VALUE10], g_dValue[VALUE20]);
		if ( g_dwValueFlg & VALFLG_PLANE )
			_ArbitraryAxis(pt);
		lpCircle->c = pt;
		lpCircle->r = g_dValue[VALUE40];
#ifdef _DEBUG_ARGV
		printf("       cx=%f cy=%f r=%f\n",
			lpCircle->c.x, lpCircle->c.y, lpCircle->r);
#endif
		return TRUE;
	}
	else {
#ifdef _DEBUG_ARGV
		printf("       error cx|cy|r\n");
#endif
		return FALSE;
	}
}

static inline BOOL _SetDxfArgv(LPDXFAARGV lpArc)
{
#ifdef _DEBUG_ARGV
	printf("SetDxfArgv() Arc Layer=%s\n", lpArc->pLayer ? LPCTSTR(lpArc->pLayer->GetLayerName()) : "?");
#endif
	if ( g_dwValueFlg & VALFLG_ARC ) {
		CPointF	pt(g_dValue[VALUE10], g_dValue[VALUE20]);
		if ( g_dwValueFlg & VALFLG_PLANE )
			_ArbitraryAxis(pt);
		lpArc->c = pt;
		lpArc->r = g_dValue[VALUE40];
		lpArc->sq  = g_dValue[VALUE50];
		lpArc->eq  = g_dValue[VALUE51];
#ifdef _DEBUG_ARGV
		printf("    cx=%f cy=%f r=%f sp=%f ep=%f\n", 
			lpArc->c.x, lpArc->c.y, lpArc->r, DEG(lpArc->sq), DEG(lpArc->eq));
#endif
		return TRUE;
	}
	else {
#ifdef _DEBUG_ARGV
		printf("    error cx|cy|r|sp|ep\n");
#endif
		return FALSE;
	}
}

static inline BOOL _SetDxfArgv(LPDXFEARGV lpEllipse)
{
#ifdef _DEBUG_ARGV
	printf("SetDxfArgv() Ellipse Layer=%s\n", lpEllipse->pLayer ? LPCTSTR(lpEllipse->pLayer->GetLayerName()) : "?");
#endif
	if ( g_dwValueFlg & VALFLG_ELLIPSE ) {
		CPointF	ptc(g_dValue[VALUE10], g_dValue[VALUE20]),
				ptl(g_dValue[VALUE11], g_dValue[VALUE21]);
		if ( g_dwValueFlg & VALFLG_PLANE ) {
			_ArbitraryAxis(ptc);
			_ArbitraryAxis(ptl);
		}
		lpEllipse->c = ptc;
		lpEllipse->l = ptl;
		lpEllipse->s = g_dValue[VALUE40];
		lpEllipse->sq = g_dValue[VALUE41];
		lpEllipse->eq = g_dValue[VALUE42];
		lpEllipse->bRound = TRUE;		// Default
#ifdef _DEBUG_ARGV
		printf("        cx=%f cy=%f lx=%f ly=%f s=%f\n", 
			lpEllipse->c.x, lpEllipse->c.y, lpEllipse->l.x, lpEllipse->l.y, lpEllipse->s);
		printf("        sp=%f ep=%f\n",
			DEG(lpEllipse->sq), DEG(lpEllipse->eq));
#endif
		return TRUE;
	}
	else {
#ifdef _DEBUG_ARGV
		printf("        error cx|cy|lx|ly|s|sp|ep\n"); 
#endif
		return FALSE;
	}
}

static inline BOOL _SetDxfArgv(LPDXFTARGV lpText)
{
#ifdef _DEBUG_ARGV
	printf("SetDxfArgv() Text Layer=%s\n", lpText->pLayer ? LPCTSTR(lpText->pLayer->GetLayerName()) : "?");
#endif
	if ( g_dwValueFlg & VALFLG_TEXT ) {
		CPointF	pt(g_dValue[VALUE10], g_dValue[VALUE20]);
		if ( g_dwValueFlg & VALFLG_PLANE )
			_ArbitraryAxis(pt);		// OCS -> WCS 座標変換
		lpText->strValue = g_strValue;
		lpText->c = pt;
#ifdef _DEBUG_ARGV
		printf("      cx=%f cy=%f\n", lpText->c.x, lpText->c.y);
		printf("      Value=%s\n", LPCTSTR(lpText->strValue));
#endif
		return TRUE;
	}
	else {
#ifdef _DEBUG_ARGV
		printf("      error cx|cy\n");
#endif
		return FALSE;
	}
}

static inline BOOL _SetBlockArgv(LPDXFBLOCK lpBlock)
{
	if ( g_dwValueFlg & VALFLG_POINT ) {
		CPointF	pt(g_dValue[VALUE10], g_dValue[VALUE20]);
		if ( g_dwValueFlg & VALFLG_PLANE )
			_ArbitraryAxis(pt);		// OCS -> WCS 座標変換
		lpBlock->ptOrg = pt;
		lpBlock->dwBlockFlg = 0;
		if ( g_dwValueFlg & VALFLG41 ) {
			lpBlock->dMagni[NCA_X] = g_dValue[VALUE41];
			lpBlock->dwBlockFlg |= DXFBLFLG_X;
		}
		else {
			lpBlock->dMagni[NCA_X] = 1.0f;
		}
		if ( g_dwValueFlg & VALFLG42 ) {
			lpBlock->dMagni[NCA_Y] = g_dValue[VALUE42];
			lpBlock->dwBlockFlg |= DXFBLFLG_Y;
		}
		else {
			lpBlock->dMagni[NCA_Y] = 1.0f;
		}
		if ( g_dwValueFlg & VALFLG50 ) {
			lpBlock->dRound = g_dValue[VALUE50];
			lpBlock->dwBlockFlg |= DXFBLFLG_R;
		}
		else {
			lpBlock->dRound = 0.0f;
		}
		return TRUE;
	}
	else
		return FALSE;
}

static inline void _CreatePolyline(void)
{
	if ( !g_pPolyline ) {
		// 例外処理はﾒｲﾝﾙｰﾌﾟで
		g_pPolyline = new CDXFpolyline();
		ASSERT( g_pPolyline );
#ifdef _DEBUG
		printf("CreatePolyline()\n");
#endif
	}
}

static inline void _DeletePolyline(void)
{
#ifdef _DEBUG
	printf("DeletePolyline()\n");
#endif
	delete	g_pPolyline;
	g_pPolyline = NULL;
	g_bVertex = g_bPuff = FALSE;
}

static inline void _NotsupportList(void)
{
	// 例外処理はﾒｲﾝﾙｰﾌﾟで
	g_strMissEntiMap.SetAt(g_strMissOrder, NULL);
	g_strMissOrder.Empty();
}

static inline void _InitialVariable(void)
{
	g_bVertex = g_bPuff = FALSE;
	g_ptPuff = g_dPuff = 0.0f;
	g_nType = TYPE_NOTSUPPORT;
	g_nBlock = g_nLayer = -1;
	g_strLayer.Empty();
	g_strBlock.Empty();
	g_strMissOrder.Empty();
	_ClearValue();
}

/////////////////////////////////////////////////////////////////////////////
//	Headerｾｸｼｮﾝ 補助関数

static inline int _HeaderVariableCheck(void)
{
/*	--- ﾍｯﾀﾞｰ読み込みは廃止
	if ( g_nGroup != g_nGroupCode[GROUP9] )
		return -2;
	for ( int i=0; i<SIZEOF(g_szHeader); i++ ) {
		if ( g_strOrder == g_szHeader[i] )
			return i;
	}
	return -1;
*/
	return -2;
}

BOOL HeaderProcedure(void)
{
/*
#ifdef _DEBUG
	printf("HeaderProcedure()\n");
#endif
	int nResult = _HeaderVariableCheck();
	
	switch ( nResult ) {
	case -2:	// Header変数ｸﾞﾙｰﾌﾟｺｰﾄﾞでない
		break;

	case -1:	// 認識する変数でない
		break;

	default:	// 認識した
		break;
	}
*/
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//	Entitiesｾｸｼｮﾝ 補助関数

static inline enENTITIESTYPE _EntitiesKeywordCheck(void)
{
	if ( g_nGroup != g_nGroupCode[GROUP0] )
		return TYPE_SECTION_ERR;
	auto f = find_if(g_szEntitiesKey, lambda::_1==g_strOrder);
	return f==end(g_szEntitiesKey) ? TYPE_NOTSUPPORT : (enENTITIESTYPE)(f-g_szEntitiesKey);
}

static inline int _EntitiesLayerCheck(void)
{
	if ( g_nGroup != g_nGroupCode[GROUP8] )
		return -2;

	// 切削ﾚｲﾔ
	if ( g_pOpt->IsCutterLayer(g_strOrder) )
		return DXFCAMLAYER;		// 1
	// 移動指示ﾚｲﾔ
	if ( g_pOpt->IsMoveLayer(g_strOrder) )
		return DXFMOVLAYER;		// 3
	// 加工開始位置指示ﾚｲﾔ
	if ( g_pOpt->IsStartLayer(g_strOrder) )
		return DXFSTRLAYER;		// 2
	// ｺﾒﾝﾄﾚｲﾔ
	if ( g_pOpt->IsCommentLayer(g_strOrder) )
		return DXFCOMLAYER;		// 4
	// 原点ﾚｲﾔ
	if ( g_pOpt->IsOriginLayer(g_strOrder) )
		return DXFORGLAYER;		// 0
	// ↑ﾃﾞｰﾀ数の多い順にﾁｪｯｸ

	return -1;
}

void SetEntitiesFromBlock(CDXFDoc* pDoc, CDXFBlockData* pBlock)
{
#ifdef _DEBUG
	printf("SetEntitiesFromBlock()\n");
#endif
	CLayerData*	pLayer;
	CDXFdata*	pData;
	CDXFdata*	pDataBlock;
	CPointF		pt;
	DXFEARGV	dxfEllipse;
	DXFBLOCK	argvBlock;

	if ( !_SetBlockArgv(&argvBlock) ) {
#ifdef _DEBUG
		printf("InsertOrg error x|y\n");
#endif
		return;
	}

#ifdef _DEBUG
	printf("InsertOrg x=%f y=%f\n", argvBlock.ptOrg.x, argvBlock.ptOrg.y);
	if ( argvBlock.dwBlockFlg & DXFBLFLG_X )
		printf(" Xmagni=%f\n", argvBlock.dMagni[NCA_X]);
	if ( argvBlock.dwBlockFlg & DXFBLFLG_Y )
		printf(" Ymagni=%f\n", argvBlock.dMagni[NCA_Y]);
	if ( argvBlock.dwBlockFlg & DXFBLFLG_R )
		printf(" Round =%f\n", argvBlock.dRound);
	printf("InsertCnt  =%d\n", pBlock->GetSize());
#endif

	// 例外ｽﾛｰは上位でｷｬｯﾁ
	for ( int i=0; i<pBlock->GetSize(); i++ ) {
		pDataBlock = pBlock->GetBlockData(i);
		pData = NULL;
		switch ( pDataBlock->GetType() ) {
		case DXFPOINTDATA:
			if ( g_nLayer == DXFCAMLAYER ) {
				pLayer = pDoc->AddLayerMap(g_strLayer);
				pData = new CDXFpoint(pLayer, static_cast<CDXFpoint*>(pDataBlock), &argvBlock);
			}
			break;

		case DXFLINEDATA:
			if ( g_nLayer == DXFORGLAYER ) {
				// 旋盤用原点ﾗｲﾝ
				pDoc->CreateLatheLine(static_cast<CDXFline*>(pDataBlock), &argvBlock);
			}
			else if ( DXFCAMLAYER<=g_nLayer && g_nLayer<=DXFMOVLAYER ) {
				pLayer = pDoc->AddLayerMap(g_strLayer, g_nLayer);
				pData = new CDXFline(pLayer, static_cast<CDXFline*>(pDataBlock), &argvBlock);
			}
			break;

		case DXFCIRCLEDATA:
			switch ( g_nLayer ) {
			case DXFORGLAYER:
				pt = static_cast<CDXFcircle*>(pDataBlock)->GetCenter();
#ifdef _DEBUG
				printf("Org x=%f y=%f\n", pt.x, pt.y);
#endif
				pDoc->CreateCutterOrigin(pt, g_dValue[VALUE40]);
				break;
			case DXFCAMLAYER:
				pLayer = pDoc->AddLayerMap(g_strLayer);
				// 各軸独自の拡大率は CDXFcircle -> CDXFellipse
				if ( argvBlock.dMagni[NCA_X] != argvBlock.dMagni[NCA_Y] ) {
					static_cast<CDXFcircle*>(pDataBlock)->SetEllipseArgv(&argvBlock, &dxfEllipse);
					dxfEllipse.pLayer = pLayer;
					pData = new CDXFellipse(&dxfEllipse); 
				}
				else
					pData = new CDXFcircle(pLayer, static_cast<CDXFcircle*>(pDataBlock), &argvBlock);
				break;
			case DXFSTRLAYER:
				pLayer = pDoc->AddLayerMap(g_strLayer, DXFSTRLAYER);
				pt = static_cast<CDXFcircle*>(pDataBlock)->GetCenter();
#ifdef _DEBUG
				printf("StartOrg x=%f y=%f\n", pt.x, pt.y);
#endif
				pData = new CDXFcircleEx(DXFSTADATA, pLayer, pt, g_dValue[VALUE40]);
				break;
			}
			break;

		case DXFARCDATA:
			if ( g_nLayer == DXFCAMLAYER ) {
				pLayer = pDoc->AddLayerMap(g_strLayer);
				// 各軸独自の拡大率は CDXFarc -> CDXFellipse
				if ( argvBlock.dMagni[NCA_X] != argvBlock.dMagni[NCA_Y] ) {
					static_cast<CDXFarc*>(pDataBlock)->SetEllipseArgv(&argvBlock, &dxfEllipse);
					dxfEllipse.pLayer = pLayer;
					pData = new CDXFellipse(&dxfEllipse); 
				}
				else
					pData = new CDXFarc(pLayer, static_cast<CDXFarc*>(pDataBlock), &argvBlock);
			}
			break;

		case DXFELLIPSEDATA:
			if ( g_nLayer == DXFCAMLAYER ) {
				pLayer = pDoc->AddLayerMap(g_strLayer);
				pData = new CDXFellipse(pLayer, static_cast<CDXFellipse*>(pDataBlock), &argvBlock);
			}
			break;

		case DXFPOLYDATA:
			if ( DXFCAMLAYER<=g_nLayer && g_nLayer<=DXFMOVLAYER ) {
				pLayer = pDoc->AddLayerMap(g_strLayer, g_nLayer);
				pData = new CDXFpolyline(pLayer, static_cast<CDXFpolyline*>(pDataBlock), &argvBlock);
			}
			break;

		case DXFTEXTDATA:
			if ( DXFCAMLAYER<=g_nLayer && g_nLayer<=DXFCOMLAYER ) {
				pLayer = pDoc->AddLayerMap(g_strLayer, g_nLayer);
				pData = new CDXFtext(pLayer, static_cast<CDXFtext*>(pDataBlock), &argvBlock);
			}
			break;
		}
		if ( pData )
			pDoc->DataOperation(pData);
	}
}

void SetEntitiesInfo(CDXFDoc* pDoc)
{
#ifdef _DEBUG
	printf("SetEntitiesInfo() g_nType=%d\n", g_nType);
#endif
	CDXFdata*	pData = NULL;
	DXFPARGV	dxfPoint;
	DXFLARGV	dxfLine;
	DXFCARGV	dxfCircle;
	DXFAARGV	dxfArc;
	DXFEARGV	dxfEllipse;
	DXFTARGV	dxfText;
	CDXFBlockData*	pBlock;
	CPointF		pt;

	switch ( g_nType ) {
	case TYPE_POINT:
		if ( g_nLayer == DXFCAMLAYER ) {
			dxfPoint.pLayer = pDoc->AddLayerMap(g_strLayer);
			if ( _SetDxfArgv(&dxfPoint) )
				pData = new CDXFpoint(&dxfPoint);
		}
		break;

	case TYPE_LINE:
		if ( g_nLayer == DXFORGLAYER ) {
			// 旋盤用原点ﾗｲﾝ
			dxfLine.pLayer = NULL;
			if ( _SetDxfArgv(&dxfLine) )
				pDoc->CreateLatheLine(dxfLine.s, dxfLine.e);
		}
		else if ( DXFCAMLAYER<=g_nLayer && g_nLayer<=DXFMOVLAYER ) {
			dxfLine.pLayer = pDoc->AddLayerMap(g_strLayer, g_nLayer);
			if ( _SetDxfArgv(&dxfLine) )
				pData = new CDXFline(&dxfLine);
		}
		break;

	case TYPE_CIRCLE:
		switch ( g_nLayer ) {
		case DXFORGLAYER:
			if ( g_dwValueFlg & VALFLG_CIRCLE ) {
				pt.SetPoint(g_dValue[VALUE10], g_dValue[VALUE20]);
				if ( g_dwValueFlg & VALFLG_PLANE )
					_ArbitraryAxis(pt);		// OCS -> WCS 座標変換
#ifdef _DEBUG
				printf("Org x=%f y=%f\n", pt.x, pt.y);
#endif
				pDoc->CreateCutterOrigin(pt, g_dValue[VALUE40]);
			}
			else {
#ifdef _DEBUG
				printf("Org error x|y|r\n");
#endif
			}
			break;
		case DXFCAMLAYER:
			dxfCircle.pLayer = pDoc->AddLayerMap(g_strLayer);
			if ( _SetDxfArgv(&dxfCircle) )
				pData = new CDXFcircle(&dxfCircle);
			break;
		case DXFSTRLAYER:
			if ( g_dwValueFlg & VALFLG_CIRCLE ) {
				pt.SetPoint(g_dValue[VALUE10], g_dValue[VALUE20]);
				if ( g_dwValueFlg & VALFLG_PLANE )
					_ArbitraryAxis(pt);		// OCS -> WCS 座標変換
#ifdef _DEBUG
				printf("StartOrg x=%f y=%f\n", pt.x, pt.y);
#endif
				pData = new CDXFcircleEx(DXFSTADATA, pDoc->AddLayerMap(g_strLayer, DXFSTRLAYER),
					pt, g_dValue[VALUE40]);
			}
			else {
#ifdef _DEBUG
				printf("StartOrg error x|y|r\n");
#endif
			}
			break;
		}
		break;

	case TYPE_ARC:
		if ( g_nLayer == DXFCAMLAYER ) {
			dxfArc.pLayer = pDoc->AddLayerMap(g_strLayer);
			if ( _SetDxfArgv(&dxfArc) )
				pData = new CDXFarc(&dxfArc);
		}
		break;

	case TYPE_ELLIPSE:
		if ( g_nLayer == DXFCAMLAYER ) {
			dxfEllipse.pLayer = pDoc->AddLayerMap(g_strLayer);
			if ( _SetDxfArgv(&dxfEllipse) )
				pData = new CDXFellipse(&dxfEllipse);
		}
		break;

	case TYPE_POLYLINE:
		if ( g_pPolyline )
			_DeletePolyline();	// SEQENDで登録するはずなので消去
		break;

	case TYPE_TEXT:
		if ( DXFCAMLAYER<=g_nLayer && g_nLayer<=DXFCOMLAYER ) {
			dxfText.pLayer = pDoc->AddLayerMap(g_strLayer, g_nLayer);
			if ( _SetDxfArgv(&dxfText) )
				pData = new CDXFtext(&dxfText);
		}
		break;

	case TYPE_INSERT:
		if ( g_strBlockMap.Lookup(g_strBlock, pBlock) )
			SetEntitiesFromBlock(pDoc, pBlock);
		else
			g_strMissBlckMap.SetAt(g_strBlock, NULL);
		break;

	case TYPE_LWPOLYLINE:
		// 最後の点を処理
		LWPolylineProcedure(pDoc, TRUE);
		// LWPOLYLINE終了処理
		if ( DXFCAMLAYER<=g_nLayer && g_nLayer<=DXFMOVLAYER ) {
			g_pPolyline->SetParentLayer(pDoc->AddLayerMap(g_strLayer, g_nLayer));
			PolylineEndProcedure(pDoc);
			g_pPolyline = NULL;
		}
		else
			_DeletePolyline();
		break;
	}

	if ( pData )
		pDoc->DataOperation(pData);
}

BOOL EntitiesProcedure(CDXFDoc* pDoc)
{
	enENTITIESTYPE	enResultType = _EntitiesKeywordCheck();
	int		nResultLayer;
	BOOL	bResult = TRUE;
	
	switch ( enResultType ) {
	case -2:	// ﾃﾞｰﾀ開始ｺｰﾄﾞ " 0" でない
		if ( g_nGroup == g_nGroupCode[GROUP2] ) {
			g_strBlock = g_strOrder;	// ﾌﾞﾛｯｸ名ｾｯﾄ
			break;
		}
		else if ( g_pPolyline && g_nGroup==g_nGroupCode[GROUP70] ) {
			if ( atoi(g_strOrder) & 1 )	// Polylineの閉ﾙｰﾌﾟか？
				g_pPolyline->SetPolyFlag(DXFPOLY_CLOSED);
			break;
		}
		// ﾚｲﾔﾁｪｯｸ
		nResultLayer = _EntitiesLayerCheck();
		switch ( nResultLayer ) {
		case -2:	// ﾚｲﾔｺｰﾄﾞでもない
			if ( g_nType > TYPE_NOTSUPPORT ) {
				if ( g_nType == TYPE_LWPOLYLINE )
					LWPolylineProcedure(pDoc, FALSE);
				_SetValue();				// 値ｾｯﾄ
			}
			break;
		case -1:	// 違うﾚｲﾔ
			if ( !g_pPolyline ) {
				g_nType = TYPE_NOTSUPPORT;
				g_nLayer = -1;
				g_strLayer.Empty();
				g_strBlock.Empty();
				g_strMissOrder.Empty();
				_ClearValue();
			}
			break;
		default:	// Hit Layer!
			if ( g_nType <= TYPE_NOTSUPPORT ) {
				if ( !g_strMissOrder.IsEmpty() )
					_NotsupportList();	// 未ｻﾎﾟｰﾄｷｰﾜｰﾄﾞの登録
			}
			else {
				g_nLayer = nResultLayer;
				g_strLayer = g_strOrder;
				if ( g_nType==TYPE_POLYLINE || g_nType==TYPE_LWPOLYLINE )
					_CreatePolyline();		// Polylineﾃﾞｰﾀの先行生成
			}
			break;
		}
		break;
	case -1:	// 認識する命令ｷｰﾜｰﾄﾞでない
		if ( g_nType==TYPE_POLYLINE && g_pPolyline ) {
			bResult = PolylineProcedure(pDoc);
			break;
		}
		g_strMissOrder = g_strOrder;
		// through
	default:	// 認識した
		// すでに処理中の時は登録処理
		if ( g_nType>TYPE_NOTSUPPORT && g_nLayer>=0 ) {
			SetEntitiesInfo(pDoc);
			_ClearValue();
		}
		if ( enResultType >= 0 )		// nResult==-1 以外
			g_strMissOrder.Empty();
		g_nType = enResultType;
		g_nLayer = -1;
		g_strLayer.Empty();
		g_strBlock.Empty();
		break;
	}

	return bResult;
}

/////////////////////////////////////////////////////////////////////////////
//	Blocksｾｸｼｮﾝ 補助関数

static inline int _BlocksKeywordCheck(void)
{
	if ( g_nGroup != g_nGroupCode[GROUP0] )
		return -2;
	auto f = find_if(g_szBlocks, lambda::_1==g_strOrder);
	return f==end(g_szBlocks) ? -1 : (int)(f-g_szBlocks);
}

BOOL SetBlockData(void)
{
#ifdef _DEBUG
	printf("SetBlockData() g_nType=%d\n", g_nType);
#endif
	DXFPARGV	dxfPoint;
	DXFLARGV	dxfLine;
	DXFCARGV	dxfCircle;
	DXFAARGV	dxfArc;
	DXFEARGV	dxfEllipse;
	DXFTARGV	dxfText;
	CDXFBlockData*	pBlock;

	// BLOCKﾃﾞｰﾀｵﾌﾞｼﾞｪｸﾄ生成(ﾚｲﾔ情報は無視)
	switch ( g_nType ) {
	case TYPE_POINT:
		dxfPoint.pLayer = NULL;
		if ( _SetDxfArgv(&dxfPoint) )
			g_pBkData->AddData(&dxfPoint);
		break;

	case TYPE_LINE:
		dxfLine.pLayer = NULL;
		if ( _SetDxfArgv(&dxfLine) )
			g_pBkData->AddData(&dxfLine);
		break;

	case TYPE_CIRCLE:
		dxfCircle.pLayer = NULL;
		if ( _SetDxfArgv(&dxfCircle) )
			g_pBkData->AddData(&dxfCircle);
		break;

	case TYPE_ARC:
		dxfArc.pLayer = NULL;
		if ( _SetDxfArgv(&dxfArc) )
			g_pBkData->AddData(&dxfArc);
		break;

	case TYPE_ELLIPSE:
		dxfEllipse.pLayer = NULL;
		if ( _SetDxfArgv(&dxfEllipse) )
			g_pBkData->AddData(&dxfEllipse);
		break;

	case TYPE_POLYLINE:
		g_pBkData->AddData(g_pPolyline);
		g_pPolyline = NULL;
		break;

	case TYPE_TEXT:
		dxfText.pLayer = NULL;
		if ( _SetDxfArgv(&dxfText) )
			g_pBkData->AddData(&dxfText);
		break;

	case TYPE_INSERT:	// Blockのﾈｽﾄ
		if ( g_strBlockMap.Lookup(g_strBlock, pBlock) ) {
			DXFBLOCK	argvBlock;
			if ( _SetBlockArgv(&argvBlock) )
				g_pBkData->CopyBlock(pBlock, &argvBlock);
		}
		else
			g_strMissBlckMap.SetAt(g_strBlock, NULL);
		break;

	case TYPE_LWPOLYLINE:
		LWPolylineProcedure(NULL, TRUE);
		PolylineEndProcedure(NULL);
		g_pBkData->AddData(g_pPolyline);
		g_pPolyline = NULL;
		break;

	case TYPE_VIEWPORT:
		break;	// 何もしない

	default:
		return FALSE;
	}

	return TRUE;
}

static inline void _SetBlockMap(void)
{
	CDXFBlockData*	pBlock;
	// 重複ﾌﾞﾛｯｸ名があると，SetAt()で置換されるので先に検索して消去
	if ( g_strBlockMap.Lookup(g_pBkData->GetBlockName(), pBlock) )
		delete	pBlock;
	g_strBlockMap.SetAt(g_pBkData->GetBlockName(), g_pBkData);
	g_pBkData = NULL;
}

BOOL BlocksProcedure(CDXFDoc* pDoc)
{
#ifdef _DEBUG
	printf("BlocksProcedure()\n");
#endif
	int		nResultBlock = _BlocksKeywordCheck();

	if ( nResultBlock < 0 && g_nBlock < 0 )	// BLOCK処理中でなければ
		return TRUE;

	enENTITIESTYPE	nResultEntities;
	BOOL	bResult = TRUE;

	switch ( nResultBlock ) {
	case -2:	// ﾌﾞﾛｯｸ開始ｺｰﾄﾞ " 0" でない
		// ﾌﾞﾛｯｸ名の検査
		if ( g_nGroup == g_nGroupCode[GROUP2] ) {
			if ( g_strOrder.GetLength()<=0 || g_strOrder[0]=='*' || g_strOrder[0]=='$' )	// 疑似ﾌﾞﾛｯｸ
				g_nBlock = -1;
			else {
				if ( g_nType == TYPE_INSERT ) {
					g_strBlock = g_strOrder;	// ﾈｽﾄﾌﾞﾛｯｸ
#ifdef _DEBUG
					printf("Nst BlockName=%s\n", LPCTSTR(g_strOrder));
#endif
				}
				else {
					g_pBkData = new CDXFBlockData(g_strOrder);
#ifdef _DEBUG
					printf("New BlockName=%s\n", LPCTSTR(g_strOrder));
#endif
				}
			}
		}
		else if ( g_pPolyline && g_nGroup==g_nGroupCode[GROUP70] ) {
			if ( atoi(g_strOrder) & 1 )
				g_pPolyline->SetPolyFlag(DXFPOLY_CLOSED);
		}
		else {
			if ( g_nType>TYPE_NOTSUPPORT || g_nBlock==0 ) {
				if ( g_nType == TYPE_LWPOLYLINE )
					LWPolylineProcedure(pDoc, FALSE);
				_SetValue();
			}
		}
		break;
	case -1:	// 認識できないｷｰﾜｰﾄﾞ(BLOCK, ENDBLK以外)
		if ( g_nBlock==0 && g_pBkData ) {	// Block基点待ち
			CPointF		pt(g_dValue[VALUE10], g_dValue[VALUE20]);
			g_pBkData->SetBlockOrigin(pt);
#ifdef _DEBUG
			printf("BlockOrigin x=%f y=%f\n", pt.x, pt.y);
#endif
			g_nBlock = 1;
		}
		if ( g_nType==TYPE_POLYLINE && g_pPolyline ) {	// VERTEX, SEQEND 処理
			bResult = PolylineProcedure(pDoc);
			break;
		}
		if ( g_nType>TYPE_NOTSUPPORT && g_pBkData ) {
			// 処理中のﾌﾞﾛｯｸ要素登録
			bResult = SetBlockData();
			_ClearValue();
		}
		// ENTITIESｷｰﾜｰﾄﾞﾁｪｯｸ
		nResultEntities = _EntitiesKeywordCheck();
		if ( nResultEntities < 0 ) {
			g_strMissOrder = g_strOrder;
			_NotsupportList();
			g_nType = TYPE_NOTSUPPORT;
		}
		else {
			g_nType = nResultEntities;
			if ( g_nType==TYPE_POLYLINE || g_nType==TYPE_LWPOLYLINE )
				_CreatePolyline();	// Polylineﾃﾞｰﾀの先行生成(ﾚｲﾔ情報無視)
		}
		break;
	case 0:		// BLOCKｷｰﾜｰﾄﾞ
		if ( g_pBkData ) {	// 既に処理中なら
			// 本来はｴﾗｰ処理やけど寛大に
			delete	g_pBkData;
			g_pBkData = NULL;
		}
		if ( g_pPolyline )
			_DeletePolyline();
		g_nBlock = 0;
		break;
	case 1:		// ENDBLKｷｰﾜｰﾄﾞ
		if ( g_pBkData ) {
			if ( g_nType > TYPE_NOTSUPPORT ) {
				// 処理中のﾌﾞﾛｯｸ要素登録
				bResult = SetBlockData();
			}
			if ( bResult )
				_SetBlockMap();	// BLOCKSﾏｯﾌﾟに登録
		}
		_ClearValue();
		g_nType = TYPE_NOTSUPPORT;
		g_nBlock = -1;
		break;
	default:	// ???
		bResult = FALSE;
		break;
	}

	return bResult;
}

/////////////////////////////////////////////////////////////////////////////
//	POLYLINE 補助関数

static inline int _PolylineKeywordCheck(void)
{
	// ｸﾞﾙｰﾌﾟｺｰﾄﾞ" 0" はﾁｪｯｸ済み
//	if ( g_nGroup != g_nGroupCode[GROUP0] )
//		return -2;
	auto f = find_if(g_szPolyline, lambda::_1==g_strOrder);
	return f==end(g_szPolyline) ? -1 : (int)(f-g_szPolyline);
}

BOOL PolylineProcedure(CDXFDoc* pDoc)
{
#ifdef _DEBUG
	printf("PolylineProcedure()\n");
#endif

	if ( g_bVertex ) {
		DXFPARGV	dxfPoint;
		dxfPoint.pLayer = g_strLayer.IsEmpty() ? NULL : pDoc->AddLayerMap(g_strLayer, g_nLayer);
		if ( _SetDxfArgv(&dxfPoint) ) {
			if ( g_nBlock >= 0 )	// Block処理中
				dxfPoint.c -= g_pBkData->GetBlockOrigin();	// 原点補正
			if ( !(g_bPuff ?
					g_pPolyline->SetVertex(&dxfPoint, g_dPuff, g_ptPuff) :	// CDXFarcとして登録
					g_pPolyline->SetVertex(&dxfPoint)) ) {					// CDXFpointとして登録
				AfxMessageBox(IDS_ERR_DXFPOLYLINE, MB_OK|MB_ICONEXCLAMATION);
				return FALSE;
			}
		}
		else {
			return FALSE;
		}
		g_ptPuff = dxfPoint.c;
		g_bVertex = FALSE;
	}

	switch ( _PolylineKeywordCheck() ) {
	case 0:		// VERTEX
		g_bVertex = TRUE;
		g_dPuff = g_dValue[VALUE42];
		g_bPuff = g_dValue[VALUE42] == 0.0f ? FALSE : TRUE;
		_ClearValue();
		break;

	case 1:		// SEQEND
#ifdef _DEBUG
		printf("SEQEND\n");
#endif
		if ( !PolylineEndProcedure(pDoc) )
			return FALSE;
		g_pPolyline = NULL;
		g_nType = TYPE_NOTSUPPORT;
		_ClearValue();
		g_bPuff = FALSE;
		break;

	default:
		g_strMissOrder = g_strOrder;
		_NotsupportList();
		break;
	}

	return TRUE;
}

BOOL PolylineEndProcedure(CDXFDoc* pDoc)
{
	// 頂点の数が１以下ならｴﾗｰ
	if ( g_pPolyline->GetVertexCount() <= 1 ) {
		AfxMessageBox(IDS_ERR_DXFPOLYLINE, MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	// 閉じたﾎﾟﾘﾗｲﾝの後処理
	if ( g_pPolyline->IsStartEqEnd() ) {
		DXFPARGV	dxfPoint;
		dxfPoint.pLayer = g_strLayer.IsEmpty() ? NULL : pDoc->AddLayerMap(g_strLayer, g_nLayer);
		dxfPoint.c = g_pPolyline->GetFirstPoint();	// 最初の座標が終点
		// 最後にふくらみ情報がない場合はCDXFpoint登録
		// ある場合はCDXFarc登録
		if ( !(g_dValue[VALUE42]==0.0f ?
				g_pPolyline->SetVertex(&dxfPoint) :
				g_pPolyline->SetVertex(&dxfPoint, g_dValue[VALUE42], g_ptPuff)) ) {
			AfxMessageBox(IDS_ERR_DXFPOLYLINE, MB_OK|MB_ICONEXCLAMATION);
			return FALSE;
		}
	}
	g_pPolyline->EndSeq();
	if ( !pDoc || g_nBlock>=0 )	// Block処理中
		SetBlockData();
	else {
		if ( DXFCAMLAYER<=g_nLayer && g_nLayer<=DXFMOVLAYER ) {
			g_pPolyline->SetParentLayer(pDoc->AddLayerMap(g_strLayer, g_nLayer));
			pDoc->DataOperation(g_pPolyline);
		}
	}

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//	LWPOLYLINE 補助関数

BOOL LWPolylineProcedure(CDXFDoc* pDoc, BOOL bEnd)
{
#ifdef _DEBUG
	printf("LWPolylineProcedure()\n");
#endif

	if ( !bEnd ) {
		// ｸﾞﾙｰﾌﾟｺｰﾄﾞ"10"で既に値が読み込まれている場合に処理する
		if ( g_nGroup!=g_nValueGroupCode[0] || !(g_dwValueFlg&VALFLG10) )
			return TRUE;
	}

	DXFPARGV	dxfPoint;
	dxfPoint.pLayer = !pDoc || g_strLayer.IsEmpty() ? NULL : pDoc->AddLayerMap(g_strLayer, g_nLayer);
	if ( _SetDxfArgv(&dxfPoint) ) {
		if ( g_nBlock >= 0 )	// Block処理中
			dxfPoint.c -= g_pBkData->GetBlockOrigin();	// 原点補正
		if ( !(g_bPuff ?
				g_pPolyline->SetVertex(&dxfPoint, g_dPuff, g_ptPuff) :	// CDXFarcとして登録
				g_pPolyline->SetVertex(&dxfPoint)) ) {					// CDXFpointとして登録
			AfxMessageBox(IDS_ERR_DXFPOLYLINE, MB_OK|MB_ICONEXCLAMATION);
			return FALSE;
		}
	}
	else {
		return FALSE;
	}

	// 次のﾃﾞｰﾀに備える
	g_ptPuff = dxfPoint.c;
	g_dPuff = g_dValue[VALUE42];
	g_bPuff = g_dValue[VALUE42] == 0.0f ? FALSE : TRUE;
	g_dValue[VALUE42] = 0.0f;

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//	旧 CDXFDoc::Serialize_Read
//		ｱﾄﾞｲﾝ的呼び出しにてDXFﾃﾞｰﾀを読み込む

BOOL ReadDXF(CDXFDoc* pDoc, LPCTSTR lpszPathName)
{
#ifdef _DEBUG
	printf("Serialize_Read(ReadDXF) Start\n");
#endif
	extern	LPCTSTR	gg_szCat;	// ", "

	ASSERT( SIZEOF(g_nGroupCode) == GROUP_NUM );
	ASSERT( SIZEOF(g_nValueGroupCode) == DXFMAXVALUESIZE );
	ASSERT( SIZEOF(g_dwValSet) == DXFMAXVALUESIZE );
	ASSERT( SIZEOF(g_szSection) == SEC_SECTION_NUM );
	ASSERT( SIZEOF(g_szSectionName) == SEC_SECNAME_NUM );
	ASSERT( SIZEOF(g_szHeader) == HEAD_NUM );
	ASSERT( SIZEOF(g_szEntitiesKey) == TYPE_ENTITIES_NUM );

	CStdioFile	fp;
	// ﾌｧｲﾙｵｰﾌﾟﾝ
	if ( !fp.Open(lpszPathName, CFile::modeRead | CFile::shareDenyWrite | CFile::osSequentialScan) ) {
		AfxMessageBox(IDS_ERR_FILEREAD, MB_OK|MB_ICONSTOP);
		return FALSE;
	}

	CProgressCtrl*	pProgress = pDoc->IsDocFlag(DXFDOC_BIND) ? NULL : AfxGetNCVCMainWnd()->GetProgressCtrl();
	ULONGLONG	dwFileSize = fp.GetLength(),	// ﾌｧｲﾙｻｲｽﾞ
				dwPosition = 0,					// ﾌｧｲﾙ現在位置
				dwReadSize;
	UINT	nReadCnt = 0;						// ﾚｺｰﾄﾞ件数
	BOOL	bSection = FALSE,	// ｾｸｼｮﾝ処理中
			bResult = TRUE;
	int		n;
	enSECNAME	nSectionName = SEC_NOSECNAME;	// -1: ENDSECまで読み飛ばし

	// ﾒｲﾝﾌﾚｰﾑのﾌﾟﾛｸﾞﾚｽﾊﾞｰ準備
	if ( pProgress ) {
		pProgress->SetRange32(0, 100);		// 100%表記
		pProgress->SetPos(0);
	}

	// ｽﾚｯﾄﾞ側でﾛｯｸ解除するまで待つ
	g_csRemoveBlockMap.Lock();
	g_csRemoveBlockMap.Unlock();
#ifdef _DEBUG
	printf("g_csRemoveBlockMap Unlock OK\n");
	g_dbgReadLine = 0;
#endif

	// 変数初期化
	g_pOpt = AfxGetNCVCApp()->GetDXFOption();
	g_strMissEntiMap.RemoveAll();
	g_strMissBlckMap.RemoveAll();
	g_pBkData = NULL;
	g_pPolyline = NULL;
	_InitialVariable();

	// ﾒｲﾝﾙｰﾌﾟ
	try {
		while ( (dwReadSize=_DubleRead(fp))>0 && bResult ) {
			if ( pProgress ) {
				// ﾌﾟﾛｸﾞﾚｽﾊﾞｰの更新
				dwPosition += dwReadSize;
				nReadCnt += 2;	// ２行づつ
				if ( (nReadCnt & 0x0100) == 0 ) {	// 128(256/2)行おき
					n = (int)(dwPosition*100/dwFileSize);
					pProgress->SetPos(min(100, n));
				}
			}
			// ｾｸｼｮﾝのﾁｪｯｸ == DXFﾌｧｲﾙのﾌｫｰﾏｯﾄﾁｪｯｸ
			switch ( _SectionCheck() ) {
			case SEC_SECTION:
				if ( !bSection ) {
					bSection = TRUE;
#ifdef _DEBUG
					printf("SECTION keyword\n");
#endif
				}
				continue;

			case SEC_ENDSEC:		// ｾｸｼｮﾝ終了処理
				bSection = FALSE;
#ifdef _DEBUG
				printf("ENDSEC keyword\n");
#endif
				// 途中ﾃﾞｰﾀの後処理
				if ( g_nType>TYPE_NOTSUPPORT && g_nLayer>=0 )
					SetEntitiesInfo(pDoc);
				if ( g_pBkData  ) {		// BLOCKﾃﾞｰﾀは必ずENDBLKで終了のため
					delete	g_pBkData;		// ここでは消去
					g_pBkData = NULL;
				}
				if ( g_pPolyline )		// SEQENDで登録するため
					_DeletePolyline();		// ここでは消去
				// 初期化
				nSectionName = SEC_NOSECNAME;
				_InitialVariable();
				continue;
			}

			// 各ｾｸｼｮﾝの処理
			switch ( nSectionName ) {
			case SEC_HEADER:
				bResult = HeaderProcedure();
				break;
			case SEC_BLOCKS:
				bResult = BlocksProcedure(pDoc);
				break;
			case SEC_ENTITIES:
				bResult = EntitiesProcedure(pDoc);
				break;
			default:
				if ( bSection ) {
					nSectionName = _SectionNameCheck();
#ifdef _DEBUG
					if ( nSectionName >= 0 )
						printf("SectionName=%s\n", LPCTSTR(g_strOrder));
#endif
				}
			}
		}	// End of MainLoop
	}
	catch ( CMemoryException* e ) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		// 未ｻﾎﾟｰﾄのｴﾗｰがでないように消去
		g_strMissEntiMap.RemoveAll();
		g_strMissBlckMap.RemoveAll();
		bResult = FALSE;
	}
	catch (	CFileException* e ) {
		AfxMessageBox(IDS_ERR_FILEREAD, MB_OK|MB_ICONSTOP);
		e->Delete();
		g_strMissEntiMap.RemoveAll();
		g_strMissBlckMap.RemoveAll();
		bResult = FALSE;
	}

	if ( pProgress )
		pProgress->SetPos(100);

#ifdef _DEBUG
	printf("dwFileSize=%lld dwPosition=%lld\n", dwFileSize, dwPosition);
#endif

	// BLOCKSﾘｽﾄのｶﾞｰﾍﾞｰｼﾞｺﾚｸｼｮﾝ
	AfxBeginThread(RemoveBlockMapThread, NULL,
		/*THREAD_PRIORITY_LOWEST*/
		THREAD_PRIORITY_IDLE
		/*THREAD_PRIORITY_BELOW_NORMAL*/);

	// 安全ｺｰﾄﾞ
	if ( g_pBkData )
		delete	g_pBkData;
	if ( g_pPolyline )
		delete	g_pPolyline;

	// 未ｻﾎﾟｰﾄのｷｰﾜｰﾄﾞを案内「ﾃﾞｰﾀ欠落の可能性」
	CString		strMiss, strMsg, strAdd;
	LPVOID		pDummy;
	VERIFY(strAdd.LoadString(IDS_ERR_DXFMISSING));
	if ( !g_strMissEntiMap.IsEmpty() ) {
		PMAP_FOREACH(strMsg, pDummy, &g_strMissEntiMap)
			if ( !strMiss.IsEmpty() )
				strMiss += gg_szCat;
			strMiss += strMsg;
		END_FOREACH
		strMsg.Format(IDS_ERR_DXFKEYWORD, strMiss);
		AfxMessageBox(strMsg+strAdd, MB_OK|MB_ICONINFORMATION);
	}
	if ( !g_strMissBlckMap.IsEmpty() ) {
		strMiss.Empty();
		PMAP_FOREACH(strMsg, pDummy, &g_strMissBlckMap)
			if ( !strMiss.IsEmpty() )
				strMiss += gg_szCat;
			strMiss += strMsg;
		END_FOREACH
		strMsg.Format(IDS_ERR_DXFBLOCK, strMiss);
		AfxMessageBox(strMsg+strAdd, MB_OK|MB_ICONINFORMATION);
	}

	return bResult;
}

UINT RemoveBlockMapThread(LPVOID)
{
	g_csRemoveBlockMap.Lock();		// ｽﾚｯﾄﾞ終了までﾛｯｸ
#ifdef _DEBUG
	printf("RemoveBlockMapThread() Start Cnt=%d\n", g_strBlockMap.GetCount());
#endif
	CDXFBlockData*	pBlock;
	CString			strBlock;
	PMAP_FOREACH(strBlock, pBlock, &g_strBlockMap)
		delete	pBlock;
	END_FOREACH
	g_strBlockMap.RemoveAll();
	g_csRemoveBlockMap.Unlock();

	return 0;
}
