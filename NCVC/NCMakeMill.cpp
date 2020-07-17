// NCMakeMill.cpp: CNCMakeMill クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "DXFdata.h"
#include "NCMakeMillOpt.h"
#include "NCMakeMill.h"

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

/////////////////////////////////////////////////////////////////////////////
// 静的変数の初期化
double	CNCMakeMill::ms_dCycleZ[] = {HUGE_VAL, HUGE_VAL};
double	CNCMakeMill::ms_dCycleR[] = {HUGE_VAL, HUGE_VAL};
double	CNCMakeMill::ms_dCycleP[] = {HUGE_VAL, HUGE_VAL};
int		CNCMakeMill::ms_nCycleCode = 81;
int		CNCMakeMill::ms_nCycleReturn = 88;
PFNGETCYCLESTRING	CNCMakeMill::ms_pfnGetCycleString = &CNCMakeMill::GetCycleString;

//////////////////////////////////////////////////////////////////////
// CNCMakeMill 構築/消滅
//////////////////////////////////////////////////////////////////////
CNCMakeMill::CNCMakeMill(const CDXFdata* pData, double dFeed, const double* pdHelical/*=NULL*/)
{
	CString	strGcode;
	CPointD	pt;

	// 本ﾃﾞｰﾀ
	switch ( pData->GetMakeType() ) {
	case DXFPOINTDATA:
		pt = pData->GetEndMakePoint();
		strGcode = (*ms_pfnGetCycleString)() +
			GetValString(NCA_X, pt.x, FALSE) +
			GetValString(NCA_Y, pt.y, FALSE);
		if ( !GetFlg(MKNC_FLG_GCLIP) || ms_dCycleZ[0]!=ms_dCycleZ[1] ) {
			strGcode += GetValString(NCA_Z, ms_dCycleZ[0], FALSE);
			ms_dCycleZ[1] = ms_dCycleZ[0];
		}
		if ( !GetFlg(MKNC_FLG_GCLIP) || ms_dCycleR[0]!=ms_dCycleR[1] ) {
			strGcode += GetValString(NCA_R, ms_dCycleR[0], TRUE);
			ms_dCycleR[1] = ms_dCycleR[0];
		}
		if ( ms_dCycleP[0]>0 &&
				(!GetFlg(MKNC_FLG_GCLIP) || ms_dCycleP[0]!=ms_dCycleP[1]) ) {
			strGcode += GetValString(NCA_P, ms_dCycleP[0], TRUE);
			ms_dCycleP[1] = ms_dCycleP[0];
		}
		if ( !strGcode.IsEmpty() )
			m_strGcode += (*ms_pfnGetLineNo)() + strGcode +
								GetFeedString(dFeed) + ms_strEOB;
		// Z軸の復帰点を静的変数へ
		ms_xyz[NCA_Z] = GetNum(MKNC_NUM_ZRETURN) == 0 ?
			::RoundUp(GetDbl(MKNC_DBL_G92Z)) : ::RoundUp(GetDbl(MKNC_DBL_ZG0STOP));
		break;

	case DXFLINEDATA:
		pt = pData->GetEndMakePoint();
		strGcode = (*ms_pfnGetGString)(1) +
			GetValString(NCA_X, pt.x, FALSE) +
			GetValString(NCA_Y, pt.y, FALSE);
		if ( !strGcode.IsEmpty() )
			m_strGcode += (*ms_pfnGetLineNo)() + strGcode + GetFeedString(dFeed) + ms_strEOB;
		break;

	case DXFCIRCLEDATA:
		m_strGcode += pdHelical ?
			(*ms_pfnMakeHelical)(static_cast<const CDXFcircle*>(pData), dFeed, *pdHelical) :
			(*ms_pfnMakeCircle) (static_cast<const CDXFcircle*>(pData), dFeed);
		break;

	case DXFARCDATA:
		m_strGcode += (*ms_pfnMakeArc)(static_cast<const CDXFarc*>(pData), dFeed);
		break;

	case DXFELLIPSEDATA:
		m_strGarray.SetSize(0, 1024);
		MakeEllipse(static_cast<const CDXFellipse*>(pData), dFeed);
		break;

	case DXFPOLYDATA:
		m_strGarray.SetSize(0, 1024);
		MakePolylineCut(static_cast<const CDXFpolyline*>(pData), dFeed);
		break;
	}
}

CNCMakeMill::CNCMakeMill(const CDXFdata* pData, BOOL bL0)
{
	CPointD	pt;

	switch ( pData->GetMakeType() ) {
	case DXFLINEDATA:
		pt = pData->GetStartMakePoint();
		// そのｵﾌﾞｼﾞｪｸﾄと現在位置が違うなら、そこまで移動(bL0除く)
		if ( bL0 && (pt.x!=ms_xyz[NCA_X] || pt.y!=ms_xyz[NCA_Y]) ) {
			m_strGcode = (*ms_pfnGetLineNo)() + (*ms_pfnGetGString)(0) +
				GetValString(NCA_X, pt.x, FALSE) +
				GetValString(NCA_Y, pt.y, FALSE) +
				ms_strEOB;
		}
		// through
	case DXFCIRCLEDATA:
		// ｵﾌﾞｼﾞｪｸﾄの移動ﾃﾞｰﾀ生成
		pt = pData->GetEndMakePoint();
		if ( pt.x!=ms_xyz[NCA_X] || pt.y!=ms_xyz[NCA_Y] ) {
			if ( bL0 ) {
				m_strGcode = (*ms_pfnGetLineNo)() + (*ms_pfnGetCycleString)() +
					GetValString(NCA_X, pt.x, FALSE) +
					GetValString(NCA_Y, pt.y, FALSE) +
					GetValString(NCA_L, 0,    FALSE) +
					ms_strEOB;
			}
			else {
				m_strGcode += (*ms_pfnGetLineNo)() + (*ms_pfnGetGString)(0) +
					GetValString(NCA_X, pt.x, FALSE) +
					GetValString(NCA_Y, pt.y, FALSE) +
					ms_strEOB;
			}
		}
		break;

	case DXFPOLYDATA:
		m_strGarray.SetSize(0, 1024);
		MakePolylineMov(static_cast<const CDXFpolyline*>(pData), bL0);
		break;
	}
}

// Z軸の変化(上昇・下降)
CNCMakeMill::CNCMakeMill(int nCode, double ZVal, double dFeed)
{
	CString	strGcode;
	CString	strValue(GetValString(NCA_Z, ZVal, FALSE));
	if ( !strValue.IsEmpty() ) {
		strGcode = (*ms_pfnGetGString)(nCode) + strValue;
		if ( dFeed > 0 )
			strGcode += GetFeedString(dFeed);
	}
	if ( !strGcode.IsEmpty() )
		m_strGcode = (*ms_pfnGetLineNo)() + strGcode + ms_strEOB;
}

// XYのG[0|1]移動
CNCMakeMill::CNCMakeMill(int nCode, const CPointD& pt)
{
	CString	strGcode(
		GetValString(NCA_X, pt.x, FALSE) +
		GetValString(NCA_Y, pt.y, FALSE) );
	if ( !strGcode.IsEmpty() ) {
		if ( nCode != 0 )	// G0以外
			strGcode += GetFeedString(GetDbl(MKNC_DBL_FEED));
		m_strGcode = (*ms_pfnGetLineNo)() + (*ms_pfnGetGString)(nCode) +
			strGcode + ms_strEOB;
	}
}

// 座標指示による円弧の生成
CNCMakeMill::CNCMakeMill
	(int nCode, const CPointD& pts, const CPointD& pte, const CPointD& pto, double r)
{
	CString	strGcode( (*ms_pfnMakeCircleSub)(nCode, pte, pto-pts, r) );
	if ( !strGcode.IsEmpty() )
		m_strGcode = (*ms_pfnGetLineNo)() + strGcode +
			GetFeedString(GetDbl(MKNC_DBL_FEED)) + ms_strEOB;
}

// 任意の文字列ｺｰﾄﾞ
CNCMakeMill::CNCMakeMill(const CString& strGcode) : CNCMakeBase(strGcode)
{
}

//////////////////////////////////////////////////////////////////////
// ﾒﾝﾊﾞ関数

void CNCMakeMill::MakePolylineMov(const CDXFpolyline* pPoly, BOOL bL0)
{
	if ( pPoly->GetVertexCount() <= 1 )
		return;

	CPointD	pt;
	CString	strGcode;
	const	CDXFdata*	pData;

	POSITION pos = pPoly->GetFirstVertex();
	pPoly->GetNextVertex(pos);
	while ( pos ) {
		pData = pPoly->GetNextVertex(pos);
		// 円弧(ふくらみ情報)は無視
		if ( pData->GetMakeType() == DXFPOINTDATA ) {
			pt = pData->GetEndMakePoint();
			if ( bL0 )
				strGcode = (*ms_pfnGetCycleString)() +
					GetValString(NCA_X, pt.x, FALSE) +
					GetValString(NCA_Y, pt.y, FALSE) +
					GetValString(NCA_L, 0,    FALSE);
			else
				strGcode = (*ms_pfnGetGString)(0) +
					GetValString(NCA_X, pt.x, FALSE) +
					GetValString(NCA_Y, pt.y, FALSE);
			if ( !strGcode.IsEmpty() )
				m_strGarray.Add((*ms_pfnGetLineNo)() + strGcode + ms_strEOB);
		}
	}
}

CString	CNCMakeMill::MakeSpindle(ENDXFTYPE enType, BOOL bDeep)
{
	CString	strResult;
	if ( enType != DXFPOINTDATA )
		strResult = bDeep ?
			(*ms_pfnGetSpindle)(GetNum(MKNC_NUM_DEEPSPINDLE)) :
			(*ms_pfnGetSpindle)(GetNum(MKNC_NUM_SPINDLE));
	else
		strResult = (*ms_pfnGetSpindle)(GetNum(MKNC_NUM_DRILLSPINDLE));
	return strResult;
}

CString CNCMakeMill::GetValString(int xyz, double dVal, BOOL bSpecial)
{
	extern	LPCTSTR	g_szNdelimiter;		// "XYZRIJKPLDH" from NCDoc.cpp
	CString	strResult;

	// 小数点以下３桁(以下切り上げ)での値合致ﾁｪｯｸ
	switch ( xyz ) {
	case NCA_X:
	case NCA_Y:
	case NCA_Z:
		if ( GetFlg(MKNC_FLG_GCLIP) && fabs(ms_xyz[xyz]-dVal)<NCMIN )
			return strResult;
		else {
			if ( GetNum(MKNC_NUM_G90) == 0 ) {	// ｱﾌﾞｿﾘｭｰﾄ
				ms_xyz[xyz] = dVal;
			}
			else {								// ｲﾝｸﾘﾒﾝﾀﾙ
				double	d = dVal;
				dVal -= ms_xyz[xyz];
				ms_xyz[xyz] = d;
			}
		}
		break;
	case NCA_R:
		// 半径Rではなく固定ｻｲｸﾙR点の場合
		if ( bSpecial ) {
			if ( GetNum(MKNC_NUM_G90) != 0 ) {	// ｲﾝｸﾘﾒﾝﾀﾙなら
				dVal -= ms_xyz[NCA_Z];			// 現在Zからの差分
				if ( dVal == 0 )
					return strResult;
			}
		}
		break;
	case NCA_I:
	case NCA_J:
	case NCA_K:
		if ( fabs(dVal) < NCMIN )	// NCの桁落ち誤差未満なら無視
			return strResult;
		break;
	case NCA_P:
		if ( bSpecial )	{	// G8x 固定ｻｲｸﾙ ﾄﾞｳｪﾙ時間
			if ( GetNum(MKNC_NUM_DWELLFORMAT) == 0 ) {	// 小数点表記
				strResult = g_szNdelimiter[NCA_P] +
					( GetFlg(MKNC_FLG_ZEROCUT) ?
						GetValString_UZeroCut(dVal) : GetValString_Normal(dVal) );
				return strResult;
			}	// 整数表記は default で処理
		}
		// through
	default:	// L(小数点指定なし)
		strResult.Format("%c%d", g_szNdelimiter[xyz], (int)dVal);
		return strResult;
	}

	// 数値書式はｵﾌﾟｼｮﾝによって動的に関数を呼び出す
	strResult = g_szNdelimiter[xyz] + (*ms_pfnGetValDetail)(dVal);

	return strResult;
}

void CNCMakeMill::SetStaticOption(const CNCMakeMillOpt* pNCMake)
{
	extern	LPCTSTR	gg_szReturn;
	static	const	int		nLineMulti[] = {	// 行番号増加分
		1, 5, 10, 100
	};

	CNCMakeBase::SetStaticOption(pNCMake);

	// --- 軸指示
	NCAX = NCA_X;	NCAY = NCA_Y;
	NCAI = NCA_I;	NCAJ = NCA_J;
	// --- 回転指示
	ms_pfnGetSpindle = GetFlg(MKNC_FLG_DISABLESPINDLE) ?
		&GetSpindleString_Clip : &GetSpindleString;
	// --- 送り指示
	switch ( GetNum(MKNC_NUM_FDOT) ) {
	case 0:		// 小数点表記
		ms_pfnGetFeed = GetFlg(MKNC_FLG_ZEROCUT) ?
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
	ms_pfnGetLineNo = GetFlg(MKNC_FLG_LINEADD) && !(GetStr(MKNC_STR_LINEFORM).IsEmpty()) ?
		&GetLineNoString : &GetLineNoString_Clip;
	// --- Gｺｰﾄﾞﾓｰﾀﾞﾙ
	if ( GetFlg(MKNC_FLG_GCLIP) ) {
		ms_pfnGetGString =  &GetGString_Clip;
		ms_pfnGetCycleString = &GetCycleString_Clip;
	}
	else {
		ms_pfnGetGString = &GetGString;
		ms_pfnGetCycleString = &GetCycleString;
	}
	// --- 座標表記
	ms_pfnGetValString = &GetValString;	// ﾍﾞｰｽｸﾗｽからの呼出用
	ms_pfnGetValDetail = GetNum(MKNC_NUM_DOT) == 0 ?
		(GetFlg(MKNC_FLG_ZEROCUT) ?
			&GetValString_UZeroCut : &GetValString_Normal) : &GetValString_Multi1000;
	// --- 行番号増加分
	ms_nMagni = GetNum(MKNC_NUM_LINEADD)<0 ||
					 GetNum(MKNC_NUM_LINEADD)>=SIZEOF(nLineMulti) ?
		nLineMulti[0] : nLineMulti[GetNum(MKNC_NUM_LINEADD)];
	// --- EOB
	ms_strEOB = GetStr(MKNC_STR_EOB).IsEmpty() ? 
		gg_szReturn : GetStr(MKNC_STR_EOB) + gg_szReturn;
	// --- 固定ｻｲｸﾙ指示
	ms_nCycleReturn = GetNum(MKNC_NUM_ZRETURN) == 0 ? 98 : 99;
	if ( GetNum(MKNC_NUM_DWELL) > 0 )
		ms_nCycleCode = GetNum(MKNC_NUM_DRILLRETURN) == 0 ? 82 : 89;
	else
		ms_nCycleCode = GetNum(MKNC_NUM_DRILLRETURN) == 0 ? 81 : 85;
	// --- 円ﾃﾞｰﾀの切削指示
	ms_nCircleCode = GetNum(MKNC_NUM_CIRCLECODE) == 0 ? 2 : 3;
	// --- 円,円弧ﾃﾞｰﾀの生成
	if ( GetNum(MKNC_NUM_IJ) == 0 ) {
		ms_pfnMakeCircle	= &MakeCircle_R;
		ms_pfnMakeCircleSub	= &MakeCircleSub_R;
		ms_pfnMakeHelical	= &MakeCircle_R_Helical;
		ms_pfnMakeArc		= &MakeArc_R;
	}
	else {
		if ( GetFlg(MKNC_FLG_CIRCLEHALF) ) {
			ms_pfnMakeCircle  = &MakeCircle_IJHALF;
			ms_pfnMakeHelical = &MakeCircle_IJHALF_Helical;
		}
		else {
			ms_pfnMakeCircle  = &MakeCircle_IJ;
			ms_pfnMakeHelical = &MakeCircle_IJ_Helical;
		}
		ms_pfnMakeCircleSub	= &MakeCircleSub_IJ;
		ms_pfnMakeArc		= &MakeArc_IJ;
	}
	// --- 楕円公差
	ms_dEllipse = GetDbl(MKNC_DBL_ELLIPSE);
}

//////////////////////////////////////////////////////////////////////

// Gｺｰﾄﾞﾓｰﾀﾞﾙ(固定ｻｲｸﾙ)
CString	CNCMakeMill::GetCycleString(void)
{
	return GetGString(ms_nCycleReturn) + GetGString(ms_nCycleCode);
}

CString	CNCMakeMill::GetCycleString_Clip(void)
{
	CString		strResult;
	if ( ms_nGcode != NCMAKECYCLECODE ) {
		strResult = GetCycleString();
		ms_nGcode = NCMAKECYCLECODE;
	}
	return strResult;
}
