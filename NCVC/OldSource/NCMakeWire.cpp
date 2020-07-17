// NCMakeWire.cpp: CNCMakeWire クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "DXFdata.h"
#include "NCMakeWireOpt.h"
#include "NCMakeWire.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

// よく使う変数や呼び出しの簡略置換
#define	GetFlg		ms_pMakeOpt->GetFlag
#define	GetNum		ms_pMakeOpt->GetNum
#define	GetDbl		ms_pMakeOpt->GetDbl
#define	GetStr		ms_pMakeOpt->GetStr

//////////////////////////////////////////////////////////////////////
// CNCMakeWire 構築/消滅
//////////////////////////////////////////////////////////////////////

CNCMakeWire::CNCMakeWire(const CDXFdata* pData, double dFeed) : CNCMakeMill(pData, dFeed)
{
}

// XYのG[0|1]移動 -> 要 CNCMakeMill ﾃﾞﾌｫﾙﾄｺﾝｽﾄﾗｸﾀ
CNCMakeWire::CNCMakeWire(int nCode, const CPointD& pt, double dFeed, double dTaper)
{
	// ﾃｰﾊﾟ指示ｺｰﾄﾞ
	CString	strTaper;
	if ( dTaper != 0.0 ) {
		if ( nCode == 0 )
			strTaper = GetGString(50) + "T0";
		else {
			if ( dTaper > 0 )
				strTaper = GetGString(51) + "T" + (*ms_pfnGetValDetail)(dTaper);
			else
				strTaper = GetGString(52) + "T" + (*ms_pfnGetValDetail)(fabs(dTaper));
		}
	}
	// 移動ｺｰﾄﾞ
	CString	strGcode(
		GetValString(NCA_X, pt.x, FALSE) +
		GetValString(NCA_Y, pt.y, FALSE) );
	if ( !strGcode.IsEmpty() ) {
		if ( nCode != 0 )	// G0以外
			strGcode += GetFeedString(dFeed);
	}
	// 最終整形
	if ( !strTaper.IsEmpty() || !strGcode.IsEmpty() ) {
		m_strGcode = (*ms_pfnGetLineNo)() + strTaper;
		if ( !strGcode.IsEmpty() )
			m_strGcode += (*ms_pfnGetGString)(nCode) + strGcode;
		m_strGcode += ms_strEOB;
	}
}

// 任意の文字列ｺｰﾄﾞ
CNCMakeWire::CNCMakeWire(const CString& strGcode) : CNCMakeMill(strGcode)
{
}

//////////////////////////////////////////////////////////////////////
// ﾒﾝﾊﾞ関数

void CNCMakeWire::SetStaticOption(const CNCMakeWireOpt* pNCMake)
{
	extern	LPCTSTR	gg_szReturn;
	static	const	int		nLineMulti[] = {	// 行番号増加分
		1, 5, 10, 100
	};

	CNCMakeBase::SetStaticOption(pNCMake);

	// --- 軸指示
	NCAX = NCA_X;	NCAY = NCA_Y;
	NCAI = NCA_I;	NCAJ = NCA_J;
	// --- 送り指示
	switch ( GetNum(MKWI_NUM_FDOT) ) {
	case 0:		// 小数点表記
		ms_pfnGetFeed = GetFlg(MKWI_FLG_ZEROCUT) ?
			&GetValString_UZeroCut : &GetValString_Normal;
		break;
	case 1:		// 1/1000表記
		ms_pfnGetFeed = &GetValString_Multi1000;
		break;
	default:	// 整数表記
		ms_pfnGetFeed = &GetFeedString_Integer;
		break;
	}
	// --- 行番号付加
	ms_pfnGetLineNo = GetFlg(MKWI_FLG_LINEADD) && !(GetStr(MKWI_STR_LINEFORM).IsEmpty()) ?
		&GetLineNoString : &GetLineNoString_Clip;
	// --- Gｺｰﾄﾞﾓｰﾀﾞﾙ
	ms_pfnGetGString =  GetFlg(MKWI_FLG_GCLIP) ? &GetGString_Clip : &GetGString;
	// --- 座標表記
//	ms_pfnGetValString = &GetValString;	// ﾍﾞｰｽｸﾗｽからの呼出用
	ms_pfnGetValDetail = GetNum(MKWI_NUM_DOT) == 0 ?
		(GetFlg(MKWI_FLG_ZEROCUT) ?
			&GetValString_UZeroCut : &GetValString_Normal) : &GetValString_Multi1000;
	// --- 行番号増加分
	ms_nMagni = GetNum(MKWI_NUM_LINEADD)<0 ||
					 GetNum(MKWI_NUM_LINEADD)>=SIZEOF(nLineMulti) ?
		nLineMulti[0] : nLineMulti[GetNum(MKWI_NUM_LINEADD)];
	// --- EOB
	ms_strEOB = GetStr(MKWI_STR_EOB).IsEmpty() ? 
		gg_szReturn : GetStr(MKWI_STR_EOB) + gg_szReturn;
	// --- 円ﾃﾞｰﾀの切削指示
	ms_nCircleCode = GetNum(MKWI_NUM_CIRCLECODE) == 0 ? 2 : 3;
	// --- 円,円弧ﾃﾞｰﾀの生成
	ms_pfnMakeCircle	= &MakeCircle_IJ;
	ms_pfnMakeCircleSub	= &MakeCircleSub_IJ;
	ms_pfnMakeHelical	= &MakeCircle_IJ_Helical;	// 保険(ﾜｲﾔ加工には無い)
	ms_pfnMakeArc		= &MakeArc_IJ;
	// --- 楕円公差
	ms_dEllipse = GetDbl(MKWI_DBL_ELLIPSE);
}
