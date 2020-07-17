// DXFMakeClass.h: CDXFMake クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCdata.h"
#include "DXFMakeOption.h"
#include "DXFkeyword.h"

typedef	CString	(*PFNMAKEVALUE)(const CNCdata*);
typedef	CString	(*PFNMAKEVALUECYCLE)(const CNCcycle*, int);

class CDXFMake
{
	CStringArray	m_strDXFarray;	// 各NCｵﾌﾞｼﾞｪｸﾄのDXFｺｰﾄﾞ

	static	CString	GROUPCODE(LPCTSTR lpszGroup) {
		// ｸﾞﾙｰﾌﾟｺｰﾄﾞを3桁右詰にする
		extern	LPCTSTR	gg_szReturn;
		return CString(' ', 3-lstrlen(lpszGroup))+lpszGroup+gg_szReturn;
	}

	// ｾｸｼｮﾝ生成(共通)
	static	CString		MakeSection(enSECNAME enType) {
		extern	LPCTSTR	gg_szReturn;
		extern	LPCTSTR	g_szGroupCode[];
		extern	LPCTSTR	g_szSection[];
		extern	LPCTSTR	g_szSectionName[];
		ASSERT(enType > SEC_NOSECNAME);
		// ﾊﾟﾀｰﾝ組み立て
		return GROUPCODE(g_szGroupCode[GROUP0])+
					g_szSection[SEC_SECTION]+gg_szReturn+
					GROUPCODE(g_szGroupCode[GROUP2])+
					g_szSectionName[enType]+gg_szReturn;
	}
	static	CString		MakeEndSec(void) {
		extern	LPCTSTR	gg_szReturn;
		extern	LPCTSTR	g_szGroupCode[];
		extern	LPCTSTR	g_szSection[];
		// ﾊﾟﾀｰﾝ組み立て
		return GROUPCODE(g_szGroupCode[GROUP0])+g_szSection[SEC_ENDSEC]+gg_szReturn;
	}
	// 値生成
	static	CString		MakeFloatValue(DWORD dwFlags, double dVal[]) {
		extern	LPCTSTR	gg_szReturn;
		extern	const	DWORD	g_dwValSet[];
		extern	LPCTSTR	g_szValueGroupCode[];
		CString	strResult, strFormat;
		for ( int i=0; i<DXFMAXVALUESIZE; i++ ) {
			if ( dwFlags & g_dwValSet[i] ) {
				strResult += GROUPCODE(g_szValueGroupCode[i]);
				strFormat.Format(IDS_MAKENCD_FORMAT, dVal[i]);
				strResult += strFormat+gg_szReturn;
			}
		}
		return strResult;
	}
	static	CString		MakeFloatValue(int nCode, double dVal) {
		extern	LPCTSTR	gg_szReturn;
		CString	strResult, strFormat;
		strFormat.Format("%d", nCode);
		strResult = GROUPCODE(strFormat);
		strFormat.Format(IDS_MAKENCD_FORMAT, dVal);
		strResult += strFormat + gg_szReturn;
		return strResult;
	}
	static	CString		MakeIntValue(int nCode, int nVal) {
		CString	strResult, strFormat;
		strFormat.Format("%d", nCode);
		strResult = GROUPCODE(strFormat);
		strFormat.Format("%6d\n", nVal);
		strResult += strFormat;
		return strResult;
	}

	static	CString		MakeDxfInfo(int nType, int nLayer) {
		extern	LPCTSTR	gg_szReturn;
		extern	LPCTSTR	g_szGroupCode[];
		extern	LPCTSTR	g_szEntitiesKey[];
		extern	const	PENSTYLE	g_penStyle[];
		CString	strResult;
		// ｵﾌﾞｼﾞｪｸﾄﾀｲﾌﾟとﾚｲﾔ名, 線種
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
		strResult = GROUPCODE(g_szGroupCode[GROUP0])+
						g_szEntitiesKey[nType]+gg_szReturn+
						GROUPCODE(g_szGroupCode[GROUP8])+
						ms_pMakeOpt->GetStr(nLayer)+gg_szReturn;
		if ( nType != TYPE_POINT )
			strResult += GROUPCODE(g_szGroupCode[GROUP6])+
				g_penStyle[ms_pMakeOpt->GetNum(nLType)].lpszDXFname+gg_szReturn;
		return strResult+MakeIntValue(62, ms_pMakeOpt->GetNum(nLCol)+1);
	}

	// 各ｾｸｼｮﾝ生成
	void	MakeSection_Header(const CNCDoc*);
	void	MakeSection_Tables(const CNCDoc*);
	void	MakeSection_Blocks(void);
	void	MakeSection_Entities(void);
	void	MakeSection_EOF(void);
	// ｵﾌﾞｼﾞｪｸﾄ生成
	void	MakeDXF_Line(const CNCline*, BOOL);
	void	MakeDXF_Arc(const CNCcircle*, BOOL);
	void	MakeDXF_Cycle(const CNCcycle*);

	// 静的変数
	static	const	CDXFMakeOption*		ms_pMakeOpt;
	static	PFNMAKEVALUE				ms_pfnMakeValueLine;
	static	PFNMAKEVALUE				ms_pfnMakeValueCircle;
	static	PFNMAKEVALUE				ms_pfnMakeValueCircleToLine;
	static	PFNMAKEVALUECYCLE			ms_pfnMakeValueCycle;
	//
	static	CString	MakeValueLine_XY(const CNCdata*);
	static	CString	MakeValueLine_XZ(const CNCdata*);
	static	CString	MakeValueLine_YZ(const CNCdata*);
	static	CString	MakeValueCircle_XY(const CNCdata*);
	static	CString	MakeValueCircle_XZ(const CNCdata*);
	static	CString	MakeValueCircle_YZ(const CNCdata*);
	static	CString	MakeValueCircleToLine_XY(const CNCdata*);
	static	CString	MakeValueCircleToLine_XZ(const CNCdata*);
	static	CString	MakeValueCircleToLine_YZ(const CNCdata*);
	static	CString	MakeValueCycle_XY_Circle(const CNCcycle*, int);
	static	CString	MakeValueCycle_XZ_Circle(const CNCcycle*, int);
	static	CString	MakeValueCycle_YZ_Circle(const CNCcycle*, int);
	static	CString	MakeValueCycle_XY_Point(const CNCcycle*, int);
	static	CString	MakeValueCycle_XZ_Point(const CNCcycle*, int);
	static	CString	MakeValueCycle_YZ_Point(const CNCcycle*, int);
	static	CString	MakeValueCycle(const CNCcycle*, int, int, ENPLANE);

public:
	// 各ｾｸｼｮﾝ情報
	CDXFMake(enSECNAME, const CNCDoc* = NULL);
	// ENTITIESﾃﾞｰﾀ
	CDXFMake(const CNCdata*, BOOL = FALSE);
	// 原点情報
	CDXFMake(const CPoint3D&);

	// 静的変数初期化
	static	void	SetStaticOption(const CDXFMakeOption*);

	// DXF出力
	void	WriteDXF(CStdioFile& fp) {
		for ( int i=0; i<m_strDXFarray.GetSize(); i++ )
			fp.WriteString( m_strDXFarray[i] );
	}
};
