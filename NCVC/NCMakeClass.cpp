// NCMakeClass.cpp: CNCMake クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "DXFdata.h"
#include "NCMakeOption.h"
#include "NCMakeClass.h"

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
const	CNCMakeOption*	CNCMake::ms_pMakeOpt = NULL;
double	CNCMake::ms_xyz[] = {HUGE_VAL, HUGE_VAL, HUGE_VAL};
int		CNCMake::ms_nGcode = -1;
int		CNCMake::ms_nSpindle = -1;
double	CNCMake::ms_dFeed = -1.0;
double	CNCMake::ms_dCycleZ[] = {HUGE_VAL, HUGE_VAL};
double	CNCMake::ms_dCycleR[] = {HUGE_VAL, HUGE_VAL};
double	CNCMake::ms_dCycleP[] = {HUGE_VAL, HUGE_VAL};
int		CNCMake::ms_nCnt = 1;
int		CNCMake::ms_nMagni = 1;
int		CNCMake::ms_nCircleCode = 2;
int		CNCMake::ms_nCycleCode = 81;
int		CNCMake::ms_nCycleReturn = 88;
CString	CNCMake::ms_strEOB;
PFNGETSPINDLE		CNCMake::ms_pfnGetSpindle = &CNCMake::GetSpindleString;
PFNGETFEED			CNCMake::ms_pfnGetFeed = &CNCMake::GetFeedString_Integer;
PFNGETLINENO		CNCMake::ms_pfnGetLineNo = &CNCMake::GetLineNoString;
PFNGETGSTRING		CNCMake::ms_pfnGetGString = &CNCMake::GetGString;
PFNGETCYCLESTRING	CNCMake::ms_pfnGetCycleString = &CNCMake::GetCycleString;
PFNGETVALSTRING		CNCMake::ms_pfnGetValString = &CNCMake::GetValString_Normal;
PFNMAKECIRCLESUB	CNCMake::ms_pfnMakeCircleSub = &CNCMake::MakeCircleSub_IJ;
PFNMAKECIRCLE		CNCMake::ms_pfnMakeCircle = &CNCMake::MakeCircle_IJ;
PFNMAKEHELICAL		CNCMake::ms_pfnMakeHelical = &CNCMake::MakeCircle_IJ_Helical;
PFNMAKEARC			CNCMake::ms_pfnMakeArc = &CNCMake::MakeArc_IJ;

//////////////////////////////////////////////////////////////////////
// CNCMake 構築/消滅
//////////////////////////////////////////////////////////////////////
CNCMake::CNCMake(const CDXFdata* pData, double dFeed, const double* pdHelical/*=NULL*/)
{
	MAKECIRCLE	mc;
	CString		strGcode;
	CPointD		pt;

	// 本ﾃﾞｰﾀ
	switch ( pData->GetMakeType() ) {
	case DXFPOINTDATA:
		strGcode = (*ms_pfnGetCycleString)() +
			GetValString(NCA_X, pData->GetEndMakePoint().x) +
			GetValString(NCA_Y, pData->GetEndMakePoint().y);
		if ( !GetFlg(MKNC_FLG_GCLIP) || ms_dCycleZ[0]!=ms_dCycleZ[1] ) {
			strGcode += GetValString(NCA_Z, ms_dCycleZ[0]);
			ms_dCycleZ[1] = ms_dCycleZ[0];
		}
		if ( !GetFlg(MKNC_FLG_GCLIP) || ms_dCycleR[0]!=ms_dCycleR[1] ) {
			strGcode += GetValString(NCA_R, ms_dCycleR[0], TRUE);
			ms_dCycleR[1] = ms_dCycleR[0];
		}
		if ( ms_dCycleP[0] > 0 ) {
			if ( !GetFlg(MKNC_FLG_GCLIP) || ms_dCycleP[0]!=ms_dCycleP[1] ) {
				strGcode += GetValString(NCA_P, ms_dCycleP[0], TRUE);
				ms_dCycleP[1] = ms_dCycleP[0];
			}
		}
		if ( !strGcode.IsEmpty() )
			m_strGcode += (*ms_pfnGetLineNo)() + strGcode + GetFeedString(dFeed) + ms_strEOB;
		// Z軸の復帰点を静的変数へ
		ms_xyz[NCA_Z] = GetNum(MKNC_NUM_ZRETURN) == 0 ?
			::RoundUp(GetDbl(MKNC_DBL_G92Z)) : ::RoundUp(GetDbl(MKNC_DBL_ZG0STOP));
		break;

	case DXFLINEDATA:
		strGcode = (*ms_pfnGetGString)(1) +
			GetValString(NCA_X, pData->GetEndMakePoint().x) +
			GetValString(NCA_Y, pData->GetEndMakePoint().y);
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

CNCMake::CNCMake(const CDXFdata* pData, BOOL bL0)
{
	CPointD	pt;

	switch ( pData->GetMakeType() ) {
	case DXFLINEDATA:
		pt = pData->GetMakePoint(0);
		// そのｵﾌﾞｼﾞｪｸﾄと現在位置が違うなら、そこまで移動(bL0除く)
		if ( bL0 && (pt.x!=ms_xyz[NCA_X] || pt.y!=ms_xyz[NCA_Y]) ) {
			m_strGcode = (*ms_pfnGetLineNo)() + (*ms_pfnGetGString)(0) +
				GetValString(NCA_X, pt.x) + GetValString(NCA_Y, pt.y) +
				ms_strEOB;
		}
		// through
	case DXFCIRCLEDATA:
		// ｵﾌﾞｼﾞｪｸﾄの移動ﾃﾞｰﾀ生成
		pt = pData->GetEndMakePoint();
		if ( pt.x!=ms_xyz[NCA_X] || pt.y!=ms_xyz[NCA_Y] ) {
			if ( bL0 ) {
				m_strGcode = (*ms_pfnGetLineNo)() + (*ms_pfnGetCycleString)() +
					GetValString(NCA_X, pt.x) + GetValString(NCA_Y, pt.y) +
					GetValString(NCA_L, 0) + ms_strEOB;
			}
			else {
				m_strGcode += (*ms_pfnGetLineNo)() + (*ms_pfnGetGString)(0) +
					GetValString(NCA_X, pt.x) + GetValString(NCA_Y, pt.y) + ms_strEOB;
			}
		}
		break;

	case DXFPOLYDATA:
		m_strGarray.SetSize(0, 1024);
		MakePolylineMov(static_cast<const CDXFpolyline*>(pData), bL0);
		break;
	}
}

//////////////////////////////////////////////////////////////////////
// ﾒﾝﾊﾞ関数

void CNCMake::MakeEllipse(const CDXFellipse* pEllipse, double dFeed)
{
	CString	strGcode;
	BOOL	bFeed = TRUE;
	CPointD	pt, ptMake;
	double	sq, eq,
			// 角度のｽﾃｯﾌﾟ数を求める -> (sq-eq) / (r*(sq-eq) / STEP)
			dStep = 1.0 / (pEllipse->GetR() / GetDbl(MKNC_DBL_ELLIPSE));

	// 楕円時の開始終了角度を再計算
	// -> XY反転など傾きを考慮しないと正しい開始位置が保持できないため
	if ( pEllipse->IsArc() ) {
		sq = pEllipse->GetStartAngle();
		eq = pEllipse->GetEndAngle();
	}
	else {
		pt = pEllipse->GetStartCutterPoint() - pEllipse->GetMakeCenter();
		sq = atan2(pt.y, pt.x) - pEllipse->GetMakeLean();	// 傾きを吸収
		if ( pEllipse->GetRound() )
			eq = sq + 360.0*RAD;
		else
			eq = sq - 360.0*RAD;
	}

	// 生成開始
	if ( pEllipse->GetRound() ) {
		for ( sq+=dStep; sq<eq; sq+=dStep ) {
			strGcode = MakeEllipse_Tolerance(pEllipse, sq);
			if ( bFeed && !strGcode.IsEmpty() ) {
				strGcode += GetFeedString(dFeed);
				bFeed = FALSE;
			}
			if ( !strGcode.IsEmpty() )
				m_strGarray.Add((*ms_pfnGetLineNo)() + strGcode + ms_strEOB);
		}
	}
	else {
		for ( sq-=dStep; sq>eq; sq-=dStep ) {
			strGcode = MakeEllipse_Tolerance(pEllipse, sq);
			if ( bFeed && !strGcode.IsEmpty() ) {
				strGcode += GetFeedString(dFeed);
				bFeed = FALSE;
			}
			if ( !strGcode.IsEmpty() )
				m_strGarray.Add((*ms_pfnGetLineNo)() + strGcode + ms_strEOB);
		}
	}
	strGcode = MakeEllipse_Tolerance(pEllipse, eq);
	if ( bFeed && !strGcode.IsEmpty() )
		strGcode += GetFeedString(dFeed);
	if ( !strGcode.IsEmpty() )
		m_strGarray.Add((*ms_pfnGetLineNo)() + strGcode + ms_strEOB);
}

void CNCMake::MakePolylineCut(const CDXFpolyline* pPoly, double dFeed)
{
	if ( pPoly->GetVertexCount() <= 1 )
		return;

	BOOL	bFeed = TRUE;
	CString	strGcode;
	CDXFdata*	pData;

	// SwapPt()で順序が入れ替わっても端点は必ず CDXFpoint
	POSITION pos = pPoly->GetFirstVertex();
	pPoly->GetNextVertex(pos);
	// ２点目からﾙｰﾌﾟ
	while ( pos ) {
		pData = pPoly->GetNextVertex(pos);
		switch ( pData->GetMakeType() ) {
		case DXFPOINTDATA:
			strGcode = (*ms_pfnGetGString)(1) +
					GetValString(NCA_X, pData->GetEndMakePoint().x) +
					GetValString(NCA_Y, pData->GetEndMakePoint().y);
			// 最初だけ送り速度追加
			if ( bFeed && !strGcode.IsEmpty() ) {
				strGcode += GetFeedString(dFeed);
				bFeed = FALSE;
			}
			if ( !strGcode.IsEmpty() )
				m_strGarray.Add((*ms_pfnGetLineNo)() + strGcode + ms_strEOB);
			break;

		case DXFARCDATA:
			strGcode = (*ms_pfnMakeArc)(static_cast<CDXFarc*>(pData), dFeed);
			if ( !strGcode.IsEmpty() ) {
				m_strGarray.Add((*ms_pfnGetLineNo)() + strGcode);
				bFeed = FALSE;
			}
			// 終点分を飛ばす
			pPoly->GetNextVertex(pos);
			break;

		case DXFELLIPSEDATA:
			MakeEllipse(static_cast<CDXFellipse*>(pData), dFeed);
			bFeed = FALSE;
			// 終点分を飛ばす
			pPoly->GetNextVertex(pos);
			break;
		}
	}	// End of loop
}

void CNCMake::MakePolylineMov(const CDXFpolyline* pPoly, BOOL bL0)
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
					GetValString(NCA_X, pt.x) + GetValString(NCA_Y, pt.y) +
					GetValString(NCA_L, 0);
			else
				strGcode = (*ms_pfnGetGString)(0) +
					GetValString(NCA_X, pt.x) + GetValString(NCA_Y, pt.y);
			if ( !strGcode.IsEmpty() )
				m_strGarray.Add((*ms_pfnGetLineNo)() + strGcode + ms_strEOB);
		}
	}
}

CString CNCMake::MakeCustomString
	(int nCode, DWORD dwValFlags/*=0*/, double* dValue/*=NULL*/, BOOL bReflect/*=TRUE*/)
{
	extern	LPCTSTR	g_szNdelimiter;		// "XYZRIJKPLDH" from NCDoc.cpp
	extern	const	DWORD	g_dwSetValFlags[];

	// 座標値を前回値に反映させない場合(G92など)は直接 ms_pfnGetValString を呼び出す
	CString	strResult;
	if ( nCode >= 0 )
		strResult = bReflect ? (*ms_pfnGetGString)(nCode) : GetGString(nCode);
	if ( dValue ) {
		for ( int i=0; i<VALUESIZE; i++ ) {
			if ( dwValFlags & g_dwSetValFlags[i] )
				strResult += bReflect || i>=NCA_P ? GetValString(i, dValue[i]) :
								(g_szNdelimiter[i]+(*ms_pfnGetValString)(dValue[i]));
		}
	}
	return strResult;
}

CString CNCMake::MakeCustomString(int nCode, int nValFlag[], double dValue[])
{
	int		n;
	// nValFlagに指定された順に値を追加
	CString	strResult( (*ms_pfnGetGString)(nCode) );
	for ( int i=0; i<VALUESIZE && nValFlag[i]>0; i++ ) {
		n = nValFlag[i];
		strResult += GetValString(n, dValue[n]);
	}
	return strResult;
}

CString CNCMake::GetValString(int xyz, double dVal, BOOL bSpecial/*=FALSE*/)
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
					(GetFlg(MKNC_FLG_ZEROCUT) ?
						GetValString_UZeroCut(dVal) : GetValString_Normal(dVal));
				return strResult;
			}	// 整数表記は default で処理
		}
		// through
	default:	// L(小数点指定なし)
		strResult.Format("%c%d", g_szNdelimiter[xyz], (int)dVal);
		return strResult;
	}

	// 数値書式はｵﾌﾟｼｮﾝによって動的に関数を呼び出す
	strResult = g_szNdelimiter[xyz] + (*ms_pfnGetValString)(dVal);

	return strResult;
}

CString	CNCMake::GetValString_UZeroCut(double dVal)
{
	CString		strResult;
	if ( dVal == 0.0 ) {
		strResult = "0";
		return strResult;
	}
	int			nCnt;
	strResult = GetValString_Normal(dVal);
	for ( nCnt=strResult.GetLength(); nCnt>0 ; nCnt-- ) {
		if ( strResult[nCnt-1]=='.' || strResult[nCnt-1]!='0' )
			break;
	}
	return	strResult.Left(nCnt);
}

void CNCMake::SetStaticOption(void)
{
	extern	LPCTSTR	gg_szReturn;
	static	const	int		nLineMulti[] = {	// 行番号増加分
		1, 5, 10, 100
	};

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
	// --- 座標表記
	ms_pfnGetValString = GetNum(MKNC_NUM_DOT) == 0 ?
		(GetFlg(MKNC_FLG_ZEROCUT) ?
			&GetValString_UZeroCut : &GetValString_Normal) : &GetValString_Multi1000;
	// --- 行番号付加
	ms_pfnGetLineNo = GetFlg(MKNC_FLG_LINEADD) && !(GetStr(MKNC_STR_LINEFORM).IsEmpty()) ?
		&GetLineNoString : &GetLineNoString_Clip;
	// --- 行番号増加分
	ms_nMagni = GetNum(MKNC_NUM_LINEADD)<0 ||
					 GetNum(MKNC_NUM_LINEADD)>=SIZEOF(nLineMulti) ?
		nLineMulti[0] : nLineMulti[GetNum(MKNC_NUM_LINEADD)];
	// --- EOB
	ms_strEOB = GetStr(MKNC_STR_EOB).IsEmpty() ? 
		gg_szReturn : GetStr(MKNC_STR_EOB) + gg_szReturn;
	// --- Gｺｰﾄﾞﾓｰﾀﾞﾙ
	if ( GetFlg(MKNC_FLG_GCLIP) ) {
		ms_pfnGetGString =  &GetGString_Clip;
		ms_pfnGetCycleString = &GetCycleString_Clip;
	}
	else {
		ms_pfnGetGString = &GetGString;
		ms_pfnGetCycleString = &GetCycleString;
	}
	// --- 固定ｻｲｸﾙ指示
	ms_nCycleReturn = GetNum(MKNC_NUM_ZRETURN) == 0 ? 98 : 99;
	if ( GetNum(MKNC_NUM_DWELL) > 0 )
		ms_nCycleCode = GetNum(MKNC_NUM_DRILLZPROCESS) == 0 ? 82 : 89;
	else
		ms_nCycleCode = GetNum(MKNC_NUM_DRILLZPROCESS) == 0 ? 81 : 85;
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
}
