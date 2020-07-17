// DXFDoc2.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "DXFOption.h"
#include "DXFdata.h"
#include "DXFDoc.h"
#include "DXFBlock.h"
#include "DXFkeyword.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

/*
	DXF の読み込みは
	CString strBuf の比較演算子だけで検査できるので
	STRING_TESTER クラスは使わない

	--------------------------------------
	以下のDXFｷｰﾜｰﾄﾞは，DXFMakeClass.cpp でも使用
*/

// ｸﾞﾙｰﾌﾟｺｰﾄﾞ
extern	LPCTSTR	g_szGroupCode[] = {
	"0", "1", "2", "3", "6", "8", "9", "70"
};
extern	LPCTSTR	g_szValueGroupCode[] = {
	"10", "20", "11", "21",
	"40", "41", "42", "50", "51"
};
extern	const	DWORD	g_dwValSet[] = {
	VALFLG10, VALFLG20, VALFLG11, VALFLG21,
	VALFLG40, VALFLG41, VALFLG42,
	VALFLG50, VALFLG51
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
	"TABLE", "ENDTAB"
};
// ﾌﾞﾛｯｸｻﾌﾞｷｰ
extern	LPCTSTR	g_szBlocks[] = {
	"BLOCK", "ENDBLK"
};
// ﾃｰﾌﾞﾙｷｰﾜｰﾄﾞ
extern	LPCTSTR	g_szTableKey[] = {
	"LTYPE", "LAYER", "VPORT"
};
// ｴﾝﾃｨﾃｨｷｰﾜｰﾄﾞ
extern	LPCTSTR	g_szEntitiesKey[] = {
	"POINT", "LINE", "CIRCLE", "ARC", "ELLIPSE", "POLYLINE", "TEXT",
	"INSERT"
};
// Polylineｷｰﾜｰﾄﾞ
extern	LPCTSTR	g_szPolyline[] = {
	"VERTEX", "SEQEND"
};

//
static	CString			g_strGroup,		// ﾌｧｲﾙ内容
						g_strOrder,
						g_strMissOrder;	// 未ｻﾎﾟｰﾄのｷｰﾜｰﾄﾞ一時保管
static	CMapStringToPtr	g_strMissEntiMap,	// ｻﾎﾟｰﾄしていないENTITIESｷｰﾜｰﾄﾞﾘｽﾄ
						g_strMissBlckMap;	// 見つからなかったﾌﾞﾛｯｸ名
static	CTypedPtrMap<CMapStringToPtr, CString, CDXFBlockData*>
						g_strBlockMap;	// BLOCKSｾｸｼｮﾝﾃﾞｰﾀ一時領域
										// ｷｰ検索高速化のためCMapｸﾗｽを使用
static	const	CDXFOption*		g_pOpt;	// DXFｵﾌﾟｼｮﾝ
static	double			g_dValue[DXFMAXVALUESIZE];
static	DWORD			g_dwValueFlg;
static	CString			g_strValue;

static	CDXFBlockData*	g_pBkData;		// BLOCK要素
static	CDXFpolyline*	g_pPolyline;	// Polylineﾃﾞｰﾀの先行生成
										// VERTEX溜まってからﾄﾞｷｭﾒﾝﾄﾃﾞｰﾀとして登録
static	BOOL	g_bVertex,		// Polylineの各頂点処理中
				g_bPuff;		// ふくらみ情報処理中
static	int		g_nType,		// TYPE_XXX
				g_nBlock,		// (-1:未処理, 0:Block基点待ち, 1:Block処理中)
				g_nLayer;		// ﾚｲﾔ情報
static	CString	g_strLayer,		// ﾚｲﾔ名
				g_strBlock;		// ﾌﾞﾛｯｸ名

// g_strBlockMapをﾊﾞｯｸｸﾞﾗｳﾝﾄﾞで消去するｽﾚｯﾄﾞ
static	CCriticalSection	g_csRemoveBlockMap;
static	UINT		RemoveBlockMapThread(LPVOID);

//
static	BOOL	HeaderProcedure(void);
static	BOOL	EntitiesProcedure(CDXFDoc*);
static	void	SetEntitiesFromBlock(CDXFDoc*, CDXFBlockData*);
static	void	SetEntitiesInfo(CDXFDoc*);
static	BOOL	BlocksProcedure(CDXFDoc*);
static	BOOL	PolylineProcedure(CDXFDoc*);


/////////////////////////////////////////////////////////////////////////////
//	ReadDXF() 補助関数
//		inlineは static宣言効果も含まれる

inline BOOL DubleRead(CStdioFile& fp)
{
	CString	strBuf;
	BOOL	bResult = fp.ReadString(strBuf);
	if ( bResult ) {
		g_strGroup = strBuf.Trim();		// 左詰ﾁｪｯｸ
		// 命令に続く値を読み込み
		bResult = fp.ReadString(strBuf);
		if ( bResult )
			g_strOrder = strBuf.Trim();
		else
			g_strOrder.Empty();
	}
	else {
		g_strGroup.Empty();
		g_strOrder.Empty();
	}
	return bResult;
}

inline void ClearValue(void)
{
	for ( int i=0; i<SIZEOF(g_dValue); i++ )
		g_dValue[i] = 0.0;
	g_dwValueFlg = 0;
}

inline int SetValue(void)
{
	int		i;
	for ( i=0; i<SIZEOF(g_dValue); i++ ) {
		if ( g_strGroup == g_szValueGroupCode[i] ) {
			g_dValue[i] = atof(g_strOrder);
			g_dwValueFlg |= g_dwValSet[i];
			return i;
		}
	}
	if ( g_strGroup == g_szGroupCode[GROUP1] ) {
		g_strValue = g_strOrder;
		return 0;
	}
	return -1;
}

inline int SectionCheck(void)
{
	if ( g_strGroup != g_szGroupCode[GROUP0] )
		return SEC_NOSECTION;
	// EOF除くﾙｰﾌﾟ
	for ( int i=0; i<SIZEOF(g_szSection)-1; i++ ) {
		if ( g_strOrder == g_szSection[i] )
			return i;
	}
	return SEC_NOSECTION;
}

inline int SectionNameCheck(void)
{
	if ( g_strGroup != g_szGroupCode[GROUP2] )
		return SEC_NOSECTION;
	for ( int i=0; i<SIZEOF(g_szSectionName); i++ ) {
		if ( g_strOrder == g_szSectionName[i] )
			return i;
	}
	return SEC_NOSECTION;	// -1
}

inline void SetDxfArgv(LPDXFPARGV lpPoint)
{
#ifdef _DEBUG
	CMagaDbg	dbg("SetDxfArgv()", DBG_MAGENTA);
#endif
	lpPoint->c.x = g_dValue[VALUE10];
	lpPoint->c.y = g_dValue[VALUE20];
#ifdef _DEBUG
	dbg.printf("Point Layer=%s", lpPoint->pLayer ? lpPoint->pLayer->GetStrLayer() : "?");
	dbg.printf("      cx=%f cy=%f", lpPoint->c.x, lpPoint->c.y);
#endif
}

inline void SetDxfArgv(LPDXFLARGV lpLine)
{
#ifdef _DEBUG
	CMagaDbg	dbg("SetDxfArgv()", DBG_MAGENTA);
#endif
	lpLine->s.x = g_dValue[VALUE10];
	lpLine->s.y = g_dValue[VALUE20];
	lpLine->e.x = g_dValue[VALUE11];
	lpLine->e.y = g_dValue[VALUE21];
#ifdef _DEBUG
	dbg.printf("Line Layer=%s", lpLine->pLayer ? lpLine->pLayer->GetStrLayer() : "?");
	dbg.printf("     sx=%f sy=%f ex=%f ey=%f", 
		lpLine->s.x, lpLine->s.y, lpLine->e.x, lpLine->e.y);
#endif
}

inline void SetDxfArgv(LPDXFCARGV lpCircle)
{
#ifdef _DEBUG
	CMagaDbg	dbg("SetDxfArgv()", DBG_MAGENTA);
#endif
	lpCircle->c.x = g_dValue[VALUE10];
	lpCircle->c.y = g_dValue[VALUE20];
	lpCircle->r   = g_dValue[VALUE40];
#ifdef _DEBUG
	dbg.printf("Circle Layer=%s", lpCircle->pLayer ? lpCircle->pLayer->GetStrLayer() : "?");
	dbg.printf("       cx=%f cy=%f r=%f", 
		lpCircle->c.x, lpCircle->c.y, lpCircle->r);
#endif
}

inline void SetDxfArgv(LPDXFAARGV lpArc)
{
#ifdef _DEBUG
	CMagaDbg	dbg("SetDxfArgv()", DBG_MAGENTA);
#endif
	lpArc->c.x = g_dValue[VALUE10];
	lpArc->c.y = g_dValue[VALUE20];
	lpArc->r   = g_dValue[VALUE40];
	lpArc->sq  = g_dValue[VALUE50];
	lpArc->eq  = g_dValue[VALUE51];
#ifdef _DEBUG
	dbg.printf("Arc Layer=%s", lpArc->pLayer ? lpArc->pLayer->GetStrLayer() : "?");
	dbg.printf("    cx=%f cy=%f r=%f sp=%f ep=%f", 
		lpArc->c.x, lpArc->c.y, lpArc->r, lpArc->sq*DEG, lpArc->eq*DEG);
#endif
}

inline void SetDxfArgv(LPDXFEARGV lpEllipse)
{
#ifdef _DEBUG
	CMagaDbg	dbg("SetDxfArgv()", DBG_MAGENTA);
#endif
	lpEllipse->c.x		= g_dValue[VALUE10];
	lpEllipse->c.y		= g_dValue[VALUE20];
	lpEllipse->l.x		= g_dValue[VALUE11];
	lpEllipse->l.y		= g_dValue[VALUE21];
	lpEllipse->s		= g_dValue[VALUE40];
	lpEllipse->sq		= g_dValue[VALUE41];
	lpEllipse->eq		= g_dValue[VALUE42];
	lpEllipse->bRound	= TRUE;		// Default
#ifdef _DEBUG
	dbg.printf("Ellipse Layer=%s", lpEllipse->pLayer ? lpEllipse->pLayer->GetStrLayer() : "?");
	dbg.printf("        cx=%f cy=%f lx=%f ly=%f s=%f", 
		lpEllipse->c.x, lpEllipse->c.y, lpEllipse->l.x, lpEllipse->l.y, lpEllipse->s);
	dbg.printf("        sp=%f ep=%f",
		lpEllipse->sq*DEG, lpEllipse->eq*DEG);
#endif
}

inline void SetDxfArgv(LPDXFTARGV lpText)
{
#ifdef _DEBUG
	CMagaDbg	dbg("SetDxfArgv()", DBG_MAGENTA);
#endif
	lpText->strValue = g_strValue;
	lpText->c.x = g_dValue[VALUE10];
	lpText->c.y = g_dValue[VALUE20];
#ifdef _DEBUG
	dbg.printf("Text Layer=%s", lpText->pLayer ? lpText->pLayer->GetStrLayer() : "?");
	dbg.printf("      cx=%f cy=%f", lpText->c.x, lpText->c.y);
	dbg.printf("      Value=%s", lpText->strValue);
#endif
}

inline void SetBlockArgv(LPDXFBLOCK lpBlock)
{
	lpBlock->ptOrg.x = g_dValue[VALUE10];
	lpBlock->ptOrg.y = g_dValue[VALUE20];
	lpBlock->dwBlockFlg = 0;
	if ( g_dwValueFlg & VALFLG41 ) {
		lpBlock->dMagni[NCA_X] = g_dValue[VALUE41];
		lpBlock->dwBlockFlg |= DXFBLFLG_X;
	}
	else {
		lpBlock->dMagni[NCA_X] = 1.0;
	}
	if ( g_dwValueFlg & VALFLG42 ) {
		lpBlock->dMagni[NCA_Y] = g_dValue[VALUE42];
		lpBlock->dwBlockFlg |= DXFBLFLG_Y;
	}
	else {
		lpBlock->dMagni[NCA_Y] = 1.0;
	}
	if ( g_dwValueFlg & VALFLG50 ) {
		lpBlock->dRound = g_dValue[VALUE50];
		lpBlock->dwBlockFlg |= DXFBLFLG_R;
	}
	else {
		lpBlock->dRound = 0.0;
	}
}

inline void CreatePolyline(void)
{
	// 例外処理はﾒｲﾝﾙｰﾌﾟで
	g_pPolyline = new CDXFpolyline();
	ASSERT( g_pPolyline );
#ifdef _DEBUG
	g_dbg.printf("CreatePolyline()");
#endif
}

inline void DeletePolyline(void)
{
#ifdef _DEBUG
	g_dbg.printf("DeletePolyline()");
#endif
	delete	g_pPolyline;
	g_pPolyline = NULL;
	g_bVertex = g_bPuff = FALSE;
}

inline void NotsupportList(void)
{
	// 例外処理はﾒｲﾝﾙｰﾌﾟで
	g_strMissEntiMap.SetAt(g_strMissOrder, NULL);
	g_strMissOrder.Empty();
}

inline void InitialVariable(void)
{
	g_bVertex = g_bPuff = FALSE;
	g_nType = g_nBlock = g_nLayer = -1;
	g_strLayer.Empty();
	g_strBlock.Empty();
	g_strMissOrder.Empty();
	ClearValue();
}

/////////////////////////////////////////////////////////////////////////////
//	Headerｾｸｼｮﾝ 補助関数

inline int HeaderVariableCheck(void)
{
/*	--- ﾍｯﾀﾞｰ読み込みは廃止
	if ( g_strGroup != g_szGroupCode[GROUP9] )
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
	CMagaDbg	dbg("HeaderProcedure()", DBG_GREEN);
#endif
	int nResult = HeaderVariableCheck();
	
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

inline int EntitiesKeywordCheck(void)
{
	if ( g_strGroup != g_szGroupCode[GROUP0] )
		return -2;
	for ( int i=0; i<SIZEOF(g_szEntitiesKey); i++ ) {
		if ( g_strOrder == g_szEntitiesKey[i] )
			return i;
	}
	return -1;
}

inline int EntitiesLayerCheck(void)
{
	if ( g_strGroup != g_szGroupCode[GROUP8] )
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
	CMagaDbg	dbg("SetEntitiesFromBlock()", DBG_MAGENTA);
#endif
	CLayerData*		pLayer;
	CDXFdata*		pData;
	CDXFdata*		pDataBlock;
	CPointD		pt;
	DXFEARGV	dxfEllipse;
	DXFBLOCK	argvBlock;
	SetBlockArgv(&argvBlock);
#ifdef _DEBUG
	dbg.printf("InsertOrg x=%f y=%f", argvBlock.ptOrg.x, argvBlock.ptOrg.y);
	if ( argvBlock.dwBlockFlg & DXFBLFLG_X )
		dbg.printf(" Xmagni=%f", argvBlock.dMagni[NCA_X]);
	if ( argvBlock.dwBlockFlg & DXFBLFLG_Y )
		dbg.printf(" Ymagni=%f", argvBlock.dMagni[NCA_Y]);
	if ( argvBlock.dwBlockFlg & DXFBLFLG_R )
		dbg.printf(" Round =%f", argvBlock.dRound);
	dbg.printf("InsertCnt  =%d", pBlock->GetSize());
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
			if ( DXFCAMLAYER<=g_nLayer && g_nLayer<=DXFMOVLAYER ) {
				pLayer = pDoc->AddLayerMap(g_strLayer, g_nLayer);
				pData = new CDXFline(pLayer, static_cast<CDXFline*>(pDataBlock), &argvBlock);
			}
			break;

		case DXFCIRCLEDATA:
			switch ( g_nLayer ) {
			case DXFORGLAYER:
				pt.SetPoint(g_dValue[VALUE10], g_dValue[VALUE20]);
#ifdef _DEBUG
				dbg.printf("Org x=%f y=%f", pt.x, pt.y);
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
				pt.SetPoint(g_dValue[VALUE10], g_dValue[VALUE20]);
#ifdef _DEBUG
				dbg.printf("StartOrg x=%f y=%f", pt.x, pt.y);
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
	CMagaDbg	dbg("SetEntitiesInfo()", DBG_MAGENTA);
	dbg.printf("g_nType=%d", g_nType);
#endif
	CDXFdata*	pData = NULL;
	DXFPARGV	dxfPoint;
	DXFLARGV	dxfLine;
	DXFCARGV	dxfCircle;
	DXFAARGV	dxfArc;
	DXFEARGV	dxfEllipse;
	DXFTARGV	dxfText;
	CDXFBlockData*	pBlock;
	CPointD		pt;

	switch ( g_nType ) {
	case TYPE_POINT:
		if ( g_nLayer == DXFCAMLAYER ) {
			dxfPoint.pLayer = pDoc->AddLayerMap(g_strLayer);
			SetDxfArgv(&dxfPoint);
			pData = new CDXFpoint(&dxfPoint);
		}
		break;

	case TYPE_LINE:
		if ( DXFCAMLAYER<=g_nLayer && g_nLayer<=DXFMOVLAYER ) {
			dxfLine.pLayer = pDoc->AddLayerMap(g_strLayer, g_nLayer);
			SetDxfArgv(&dxfLine);
			pData = new CDXFline(&dxfLine);
		}
		break;

	case TYPE_CIRCLE:
		switch ( g_nLayer ) {
		case DXFORGLAYER:
			pt.SetPoint(g_dValue[VALUE10], g_dValue[VALUE20]);
#ifdef _DEBUG
			dbg.printf("Org x=%f y=%f", pt.x, pt.y);
#endif
			pDoc->CreateCutterOrigin(pt, g_dValue[VALUE40]);
			break;
		case DXFCAMLAYER:
			dxfCircle.pLayer = pDoc->AddLayerMap(g_strLayer);
			SetDxfArgv(&dxfCircle);
			pData = new CDXFcircle(&dxfCircle);
			break;
		case DXFSTRLAYER:
			pt.SetPoint(g_dValue[VALUE10], g_dValue[VALUE20]);
#ifdef _DEBUG
			dbg.printf("StartOrg x=%f y=%f", pt.x, pt.y);
#endif
			pData = new CDXFcircleEx(DXFSTADATA, pDoc->AddLayerMap(g_strLayer, DXFSTRLAYER),
				pt, g_dValue[VALUE40]);
			break;
		}
		break;

	case TYPE_ARC:
		if ( g_nLayer == DXFCAMLAYER ) {
			dxfArc.pLayer = pDoc->AddLayerMap(g_strLayer);
			SetDxfArgv(&dxfArc);
			pData = new CDXFarc(&dxfArc);
		}
		break;

	case TYPE_ELLIPSE:
		if ( g_nLayer == DXFCAMLAYER ) {
			dxfEllipse.pLayer = pDoc->AddLayerMap(g_strLayer);
			SetDxfArgv(&dxfEllipse);
			pData = new CDXFellipse(&dxfEllipse);
		}
		break;

	case TYPE_POLYLINE:
		if ( g_pPolyline )
			DeletePolyline();	// SEQENDで登録するはずなので消去
		break;

	case TYPE_TEXT:
		if ( DXFCAMLAYER<=g_nLayer && g_nLayer<=DXFCOMLAYER ) {
			dxfText.pLayer = pDoc->AddLayerMap(g_strLayer, g_nLayer);
			SetDxfArgv(&dxfText);
			pData = new CDXFtext(&dxfText);
		}
		break;

	case TYPE_INSERT:
		if ( g_strBlockMap.Lookup(g_strBlock, pBlock) )
			SetEntitiesFromBlock(pDoc, pBlock);
		else
			g_strMissBlckMap.SetAt(g_strBlock, NULL);
		break;
	}
	if ( pData )
		pDoc->DataOperation(pData);
}

BOOL EntitiesProcedure(CDXFDoc* pDoc)
{
#ifdef _DEBUG
	CMagaDbg	dbg("EntitiesProcedure()", DBG_GREEN);
#endif
	int		nResultType = EntitiesKeywordCheck();
	int		nResultLayer;
	BOOL	bResult = TRUE;
	
	switch ( nResultType ) {
	case -2:	// ﾃﾞｰﾀ開始ｺｰﾄﾞ " 0" でない
		if ( g_strGroup == g_szGroupCode[GROUP2] ) {
			g_strBlock = g_strOrder;	// ﾌﾞﾛｯｸ名ｾｯﾄ
			break;
		}
		else if ( g_pPolyline && g_strGroup==g_szGroupCode[GROUP70] ) {
			if ( atoi(g_strOrder) & 1 )	// Polylineの閉ﾙｰﾌﾟか？
				g_pPolyline->SetPolyFlag(DXFPOLY_CLOSED);
			break;
		}
		// ﾚｲﾔﾁｪｯｸ
		nResultLayer = EntitiesLayerCheck();
		switch ( nResultLayer ) {
		case -2:	// ﾚｲﾔｺｰﾄﾞでもない
			if ( g_nType >= 0 )
				SetValue();				// 値ｾｯﾄ
			break;
		case -1:	// 違うﾚｲﾔ
			if ( !g_pPolyline ) {
				g_nType = g_nLayer = -1;
				g_strLayer.Empty();
				g_strBlock.Empty();
				g_strMissOrder.Empty();
				ClearValue();
			}
			break;
		default:	// Hit Layer!
			if ( g_nType < 0 ) {
				if ( !g_strMissOrder.IsEmpty() )
					NotsupportList();	// 未ｻﾎﾟｰﾄｷｰﾜｰﾄﾞの登録
			}
			else {
				g_nLayer = nResultLayer;
				g_strLayer = g_strOrder;
				if ( g_nType==TYPE_POLYLINE && !g_pPolyline )
					CreatePolyline();		// Polylineﾃﾞｰﾀの先行生成
			}
			break;
		}
		break;
	case -1:	// 認識する命令ｷｰﾜｰﾄﾞでない
		if ( g_pPolyline ) {
			bResult = PolylineProcedure(pDoc);
			break;
		}
		g_strMissOrder = g_strOrder;
		// through
	default:	// 認識した
		// すでに処理中の時は登録処理
		if ( g_nType>=0 && g_nLayer>=0 ) {
			SetEntitiesInfo(pDoc);
			ClearValue();
		}
		if ( nResultType >= 0 )		// nResult==-1 以外
			g_strMissOrder.Empty();
		g_nType = nResultType;
		g_nLayer = -1;
		g_strLayer.Empty();
		g_strBlock.Empty();
		break;
	}

	return bResult;
}

/////////////////////////////////////////////////////////////////////////////
//	Blocksｾｸｼｮﾝ 補助関数

inline int BlocksKeywordCheck(void)
{
	if ( g_strGroup != g_szGroupCode[GROUP0] )
		return -2;
	for ( int i=0; i<SIZEOF(g_szBlocks); i++ ) {
		if ( g_strOrder == g_szBlocks[i] )
			return i;
	}
	return -1;
}

inline BOOL SetBlockData(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("SetBlockData()", DBG_MAGENTA);
	dbg.printf("g_nType=%d", g_nType);
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
		SetDxfArgv(&dxfPoint);
		g_pBkData->AddData(&dxfPoint);
		break;

	case TYPE_LINE:
		dxfLine.pLayer = NULL;
		SetDxfArgv(&dxfLine);
		g_pBkData->AddData(&dxfLine);
		break;

	case TYPE_CIRCLE:
		dxfCircle.pLayer = NULL;
		SetDxfArgv(&dxfCircle);
		g_pBkData->AddData(&dxfCircle);
		break;

	case TYPE_ARC:
		dxfArc.pLayer = NULL;
		SetDxfArgv(&dxfArc);
		g_pBkData->AddData(&dxfArc);
		break;

	case TYPE_ELLIPSE:
		dxfEllipse.pLayer = NULL;
		SetDxfArgv(&dxfEllipse);
		g_pBkData->AddData(&dxfEllipse);
		break;

	case TYPE_POLYLINE:
		g_pBkData->AddData(g_pPolyline);
		break;

	case TYPE_TEXT:
		dxfText.pLayer = NULL;
		SetDxfArgv(&dxfText);
		g_pBkData->AddData(&dxfText);
		break;

	case TYPE_INSERT:	// Blockのﾈｽﾄ
		if ( g_strBlockMap.Lookup(g_strBlock, pBlock) ) {
			DXFBLOCK	argvBlock;
			SetBlockArgv(&argvBlock);
			g_pBkData->CopyBlock(pBlock, &argvBlock);
		}
		else
			g_strMissBlckMap.SetAt(g_strBlock, NULL);
		break;

	default:
		return FALSE;
	}

	return TRUE;
}

inline void SetBlockMap(void)
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
	CMagaDbg	dbg("BlocksProcedure()", DBG_GREEN);
#endif
	int		nResultBlock = BlocksKeywordCheck();

	if ( nResultBlock < 0 && g_nBlock < 0 )	// BLOCK処理中でなければ
		return TRUE;

	int		nResultEntities;
	BOOL	bResult = TRUE;

	switch ( nResultBlock ) {
	case -2:	// ﾌﾞﾛｯｸ開始ｺｰﾄﾞ " 0" でない
		// ﾌﾞﾛｯｸ名の検査
		if ( g_strGroup == g_szGroupCode[GROUP2] ) {
			if ( g_strOrder.GetLength()<=0 || g_strOrder[0]=='*' || g_strOrder[0]=='$' )	// 疑似ﾌﾞﾛｯｸ
				g_nBlock = -1;
			else {
				if ( g_nType == TYPE_INSERT ) {
					g_strBlock = g_strOrder;	// ﾈｽﾄﾌﾞﾛｯｸ
#ifdef _DEBUG
					dbg.printf("Nst BlockName=%s", g_strOrder);
#endif
				}
				else {
					g_pBkData = new CDXFBlockData(g_strOrder);
#ifdef _DEBUG
					dbg.printf("New BlockName=%s", g_strOrder);
#endif
				}
			}
		}
		else if ( g_pPolyline && g_strGroup==g_szGroupCode[GROUP70] ) {
			if ( atoi(g_strOrder) & 1 )
				g_pPolyline->SetPolyFlag(DXFPOLY_CLOSED);
		}
		else {
			if ( g_nType>=0 || g_nBlock==0 )
				SetValue();
		}
		break;
	case -1:	// 認識できないｷｰﾜｰﾄﾞ(BLOCK, ENDBLK以外)
		if ( g_nBlock==0 && g_pBkData ) {	// Block基点待ち
			CPointD		pt(g_dValue[VALUE10], g_dValue[VALUE20]);
			g_pBkData->SetBlockOrigin(pt);
#ifdef _DEBUG
			dbg.printf("BlockOrigin x=%f y=%f", pt.x, pt.y);
#endif
			g_nBlock = 1;
		}
		if ( g_pPolyline ) {	// VERTEX, SEQEND 処理
			bResult = PolylineProcedure(pDoc);
			break;
		}
		if ( g_nType>=0 && g_pBkData ) {
			// 処理中のﾌﾞﾛｯｸ要素登録
			bResult = SetBlockData();
			ClearValue();
		}
		// ENTITIESｷｰﾜｰﾄﾞﾁｪｯｸ
		nResultEntities = EntitiesKeywordCheck();
		if ( nResultEntities < 0 ) {
			g_strMissOrder = g_strOrder;
			NotsupportList();
			g_nType = -1;
		}
		else {
			g_nType = nResultEntities;
			if ( g_nType==TYPE_POLYLINE && !g_pPolyline )
				CreatePolyline();	// Polylineﾃﾞｰﾀの先行生成(ﾚｲﾔ情報無視)
		}
		break;
	case 0:		// BLOCKｷｰﾜｰﾄﾞ
		if ( g_pBkData ) {	// 既に処理中なら
			// 本来はｴﾗｰ処理やけど寛大に
			delete	g_pBkData;
			g_pBkData = NULL;
		}
		if ( g_pPolyline )
			DeletePolyline();
		g_nBlock = 0;
		break;
	case 1:		// ENDBLKｷｰﾜｰﾄﾞ
		if ( g_pBkData ) {
			if ( g_nType >= 0 ) {
				// 処理中のﾌﾞﾛｯｸ要素登録
				bResult = SetBlockData();
			}
			if ( bResult )
				SetBlockMap();	// BLOCKSﾏｯﾌﾟに登録
		}
		ClearValue();
		g_nType = g_nBlock = -1;
		break;
	default:	// ???
		bResult = FALSE;
		break;
	}

	return bResult;
}

/////////////////////////////////////////////////////////////////////////////
//	Polyline 補助関数

inline int PolylineKeywordCheck(void)
{
	// ｸﾞﾙｰﾌﾟｺｰﾄﾞ" 0" はﾁｪｯｸ済み
//	if ( g_strGroup != g_szGroupCode[GROUP0] )
//		return -2;
	for ( int i=0; i<SIZEOF(g_szPolyline); i++ ) {
		if ( g_strOrder == g_szPolyline[i] )
			return i;
	}
	return -1;
}

BOOL PolylineProcedure(CDXFDoc* pDoc)
{
	// ふくらみ情報
	static	double	ss_dPuff = 0.0;	// 前のVERTEXのふくらみ値
	static	CPointD	ss_pts;			// ふくらみ情報を計算するための前回位置
#ifdef _DEBUG
	CMagaDbg	dbg("PolylineProcedure()", DBG_CYAN);
#endif
	DXFPARGV	dxfPoint;

	if ( g_bVertex ) {
		dxfPoint.pLayer = g_strLayer.IsEmpty() ? NULL : pDoc->AddLayerMap(g_strLayer);
		SetDxfArgv(&dxfPoint);
		if ( g_nBlock >= 0 )	// Block処理中
			dxfPoint.c -= g_pBkData->GetBlockOrigin();	// 原点補正
		if ( !(g_bPuff ?
					g_pPolyline->SetVertex(&dxfPoint, ss_dPuff, ss_pts) :	// CDXFarcとして登録
					g_pPolyline->SetVertex(&dxfPoint)) ) {					// CDXFpointとして登録
			AfxMessageBox(IDS_ERR_DXFPOLYLINE, MB_OK|MB_ICONEXCLAMATION);
			return FALSE;
		}
		ss_pts = dxfPoint.c;
		g_bVertex = FALSE;
	}

	switch ( PolylineKeywordCheck() ) {
	case 0:		// VERTEX
		g_bVertex = TRUE;
		ss_dPuff = g_dValue[VALUE42];
		g_bPuff = g_dValue[VALUE42] == 0.0 ? FALSE : TRUE;
		ClearValue();
		break;

	case 1:		// SEQEND
#ifdef _DEBUG
		dbg.printf("SEQEND");
#endif
		// 頂点の数が１以下ならｴﾗｰ
		if ( g_pPolyline->GetVertexCount() <= 1 ) {
			AfxMessageBox(IDS_ERR_DXFPOLYLINE, MB_OK|MB_ICONEXCLAMATION);
			return FALSE;
		}
		// 閉じたﾎﾟﾘﾗｲﾝの後処理
		if ( g_pPolyline->IsStartEqEnd() ) {
			dxfPoint.pLayer = g_strLayer.IsEmpty() ? NULL : pDoc->AddLayerMap(g_strLayer);
			dxfPoint.c = g_pPolyline->GetFirstPoint();	// 最初の座標が終点
			// 最後にふくらみ情報がない場合はCDXFpoint登録
			// ある場合はCDXFarc登録
			if ( !(g_dValue[VALUE42]==0.0 ?
					g_pPolyline->SetVertex(&dxfPoint) :
					g_pPolyline->SetVertex(&dxfPoint, g_dValue[VALUE42], ss_pts)) ) {
				AfxMessageBox(IDS_ERR_DXFPOLYLINE, MB_OK|MB_ICONEXCLAMATION);
				return FALSE;
			}
		}
		g_pPolyline->EndSeq();
		if ( g_nBlock >= 0 )	// Block処理中
			SetBlockData();
		else {
			if ( DXFCAMLAYER<=g_nLayer && g_nLayer<=DXFMOVLAYER ) {
				g_pPolyline->SetParentLayer(pDoc->AddLayerMap(g_strLayer, g_nLayer));
				pDoc->DataOperation(g_pPolyline);
			}
		}
		g_pPolyline = NULL;
		g_nType = -1;
		ClearValue();
		g_bPuff = FALSE;
		break;

	default:
		g_strMissOrder = g_strOrder;
		NotsupportList();
		break;
	}

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//	旧 CDXFDoc::Serialize_Read
//		ｱﾄﾞｲﾝ的呼び出しにてDXFﾃﾞｰﾀを読み込む

BOOL ReadDXF(CDXFDoc* pDoc, LPCTSTR lpszPathName)
{
#ifdef _DEBUG
	CMagaDbg	dbg("Serialize_Read()\nStart", DBG_GREEN);
#endif
	extern	LPCTSTR	gg_szCat;

	CStdioFile	fp;
	// ﾌｧｲﾙｵｰﾌﾟﾝ
	if ( !fp.Open(lpszPathName, CFile::modeRead | CFile::shareDenyWrite) ) {
		AfxMessageBox(IDS_ERR_FILEREAD, MB_OK|MB_ICONSTOP);
		return FALSE;
	}

	CProgressCtrl*	pProgress = AfxGetNCVCMainWnd()->GetProgressCtrl();
	ULONGLONG	dwSize = fp.GetLength();	// ﾌｧｲﾙｻｲｽﾞ
	DWORD		dwPosition = 0;				// ﾌｧｲﾙ現在位置
	UINT	nReadCnt = 0;				// ﾚｺｰﾄﾞ件数
	BOOL	bSection = FALSE;			// ｾｸｼｮﾝ処理中
	int		n, nSectionName = SEC_NOSECTION;	// -1: ENDSECまで読み飛ばし
	BOOL	bResult = TRUE;

	// ﾒｲﾝﾌﾚｰﾑのﾌﾟﾛｸﾞﾚｽﾊﾞｰ準備
	pProgress->SetRange32(0, 100);		// 100%表記
	pProgress->SetPos(0);

	// ｽﾚｯﾄﾞ側でﾛｯｸ解除するまで待つ
	g_csRemoveBlockMap.Lock();
	g_csRemoveBlockMap.Unlock();
#ifdef _DEBUG
	dbg.printf("g_csRemoveBlockMap Unlock OK");
#endif

	// 変数初期化
	g_pOpt = AfxGetNCVCApp()->GetDXFOption();
	g_strMissEntiMap.RemoveAll();
	g_strMissBlckMap.RemoveAll();
	g_pBkData = NULL;
	g_pPolyline = NULL;
	InitialVariable();

	// ﾒｲﾝﾙｰﾌﾟ
	try {
		while ( DubleRead(fp) && bResult ) {
			// ﾌﾟﾛｸﾞﾚｽﾊﾞｰの更新
			dwPosition += g_strGroup.GetLength() + 2;	// +2 = 改行ｺｰﾄﾞ分
			dwPosition += g_strOrder.GetLength() + 2;
			nReadCnt += 2;	// ２行づつ
			if ( (nReadCnt & 0x0100) == 0 )	// 128(256/2)行おき
				n = (int)(dwPosition*100/dwSize);
				pProgress->SetPos(min(100, n));

			// ｾｸｼｮﾝのﾁｪｯｸ == DXFﾌｧｲﾙのﾌｫｰﾏｯﾄﾁｪｯｸ
			switch ( SectionCheck() ) {
			case 0:
				if ( !bSection ) {
					bSection = TRUE;
#ifdef _DEBUG
					dbg.printf("SECTION keyword");
#endif
				}
				continue;

			case 1:		// ｾｸｼｮﾝ終了処理
				bSection = FALSE;
#ifdef _DEBUG
				dbg.printf("ENDSEC keyword");
#endif
				// 途中ﾃﾞｰﾀの後処理
				if ( g_nType>=0 && g_nLayer>=0 )
					SetEntitiesInfo(pDoc);
				if ( g_pBkData  ) {		// BLOCKﾃﾞｰﾀは必ずENDBLKで終了のため
					delete	g_pBkData;		// ここでは消去
					g_pBkData = NULL;
				}
				if ( g_pPolyline )		// SEQENDで登録するため
					DeletePolyline();		// ここでは消去
				// 初期化
				nSectionName = SEC_NOSECTION;
				InitialVariable();
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
					nSectionName = SectionNameCheck();
#ifdef _DEBUG
					if ( nSectionName >= 0 )
						dbg.printf("SectionName=%s", g_strOrder);
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

#ifdef _DEBUG
	dbg.printf("dwSize=%d dwPosition=%d", dwSize, dwPosition);
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
	POSITION	pos;
	VERIFY(strAdd.LoadString(IDS_ERR_DXFMISSING));
	if ( !g_strMissEntiMap.IsEmpty() ) {
		for ( pos=g_strMissEntiMap.GetStartPosition(); pos; ) {
			g_strMissEntiMap.GetNextAssoc(pos, strMsg, pDummy);
			if ( !strMiss.IsEmpty() )
				strMiss += gg_szCat;
			strMiss += strMsg;
		}
		strMsg.Format(IDS_ERR_DXFKEYWORD, strMiss);
		AfxMessageBox(strMsg+strAdd, MB_OK|MB_ICONINFORMATION);
	}
	if ( !g_strMissBlckMap.IsEmpty() ) {
		strMiss.Empty();
		for ( pos=g_strMissBlckMap.GetStartPosition(); pos; ) {
			g_strMissBlckMap.GetNextAssoc(pos, strMsg, pDummy);
			if ( !strMiss.IsEmpty() )
				strMiss += gg_szCat;
			strMiss += strMsg;
		}
		strMsg.Format(IDS_ERR_DXFBLOCK, strMiss);
		AfxMessageBox(strMsg+strAdd, MB_OK|MB_ICONINFORMATION);
	}

	return bResult;
}

UINT RemoveBlockMapThread(LPVOID)
{
	g_csRemoveBlockMap.Lock();		// ｽﾚｯﾄﾞ終了までﾛｯｸ
#ifdef _DEBUG
	CMagaDbg	dbg("RemoveBlockMapThread()", TRUE, DBG_RED);
	dbg.printf("Start Cnt=%d", g_strBlockMap.GetCount());
#endif
	CDXFBlockData*	pBlock;
	CString			strBlock;
	for ( POSITION pos=g_strBlockMap.GetStartPosition(); pos; ) {
		g_strBlockMap.GetNextAssoc(pos, strBlock, pBlock);
		delete	pBlock;
	}
	g_strBlockMap.RemoveAll();
	g_csRemoveBlockMap.Unlock();

	return 0;
}
