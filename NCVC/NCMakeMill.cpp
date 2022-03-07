// NCMakeMill.cpp: CNCMakeMill クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "DXFdata.h"
#include "NCMakeMillOpt.h"
#include "NCMakeMill.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using std::string;
using namespace boost;

// よく使う変数や呼び出しの簡略置換
#define	GetFlg		ms_pMakeOpt->GetFlag
#define	GetNum		ms_pMakeOpt->GetNum
#define	GetDbl		ms_pMakeOpt->GetDbl
#define	GetStr		ms_pMakeOpt->GetStr

/////////////////////////////////////////////////////////////////////////////
// 静的変数の初期化
float	CNCMakeMill::ms_dCycleZ[] = {HUGE_VALF, HUGE_VALF};
float	CNCMakeMill::ms_dCycleR[] = {HUGE_VALF, HUGE_VALF};
float	CNCMakeMill::ms_dCycleP[] = {HUGE_VALF, HUGE_VALF};
float	CNCMakeMill::ms_dCycleQ[] = {HUGE_VALF, HUGE_VALF};
int		CNCMakeMill::ms_nCycleCode = NCMAKECYCLECODE;	// 81
int		CNCMakeMill::ms_nCycleReturn = 98;
PFNGETCYCLESTRING	CNCMakeMill::ms_pfnGetCycleString = &CNCMakeMill::GetCycleString;

//////////////////////////////////////////////////////////////////////
// CNCMakeMill 構築/消滅
//////////////////////////////////////////////////////////////////////

CNCMakeMill::CNCMakeMill()
{
}

CNCMakeMill::CNCMakeMill(const CDXFdata* pData, float dFeed, const float* pdHelical/*=NULL*/)
{
	CString	strGcode;
	CPointF	pt;

	// 本ﾃﾞｰﾀ
	switch ( pData->GetMakeType() ) {
	case DXFPOINTDATA:
		pt = pData->GetEndMakePoint();
		strGcode = GetValString(NCA_X, pt.x) + GetValString(NCA_Y, pt.y);
		if ( !GetFlg(MKNC_FLG_GCLIP) || ms_dCycleZ[0]!=ms_dCycleZ[1] ) {
			strGcode += GetValString(NCA_Z, ms_dCycleZ[0]);
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
		if ( GetNum(MKNC_NUM_DRILLRETURN)==2 &&		// G83
				(!GetFlg(MKNC_FLG_GCLIP) || ms_dCycleQ[0]!=ms_dCycleQ[1]) ) {
			strGcode += "Q" + (*ms_pfnGetValDetail)(ms_dCycleQ[0]);
			ms_dCycleQ[1] = ms_dCycleQ[0];
		}
		if ( !strGcode.IsEmpty() ) {
			m_strGcode = MakeStrBlock((*ms_pfnGetCycleString)() + strGcode + GetFeedString(dFeed));
		}
		// Z軸の復帰点を静的変数へ
		ms_xyz[NCA_Z] = GetNum(MKNC_NUM_ZRETURN) == 0 ?
			::RoundUp(GetDbl(MKNC_DBL_G92Z)) : ::RoundUp(GetDbl(MKNC_DBL_ZG0STOP));
		break;

	case DXFLINEDATA:
		pt = pData->GetEndMakePoint();
		strGcode = GetValString(NCA_X, pt.x) + GetValString(NCA_Y, pt.y);
		if ( !strGcode.IsEmpty() ) {
			m_strGcode = MakeStrBlock((*ms_pfnGetGString)(1) + strGcode + GetFeedString(dFeed));
		}
		break;

	case DXFCIRCLEDATA:
		m_strGcode = pdHelical ?
			(*ms_pfnMakeHelical)(static_cast<const CDXFcircle*>(pData), dFeed, *pdHelical) :
			(*ms_pfnMakeCircle) (static_cast<const CDXFcircle*>(pData), dFeed);
		break;

	case DXFARCDATA:
		m_strGcode = (*ms_pfnMakeArc)(static_cast<const CDXFarc*>(pData), dFeed);
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
	CPointF	pt;

	switch ( pData->GetMakeType() ) {
	case DXFLINEDATA:
		pt = pData->GetStartMakePoint();
		// そのｵﾌﾞｼﾞｪｸﾄと現在位置が違うなら、そこまで移動(bL0除く)
		if ( bL0 && (pt.x!=ms_xyz[NCA_X] || pt.y!=ms_xyz[NCA_Y]) ) {
			m_strGcode = MakeStrBlock((*ms_pfnGetGString)(0) +
				GetValString(NCA_X, pt.x) + GetValString(NCA_Y, pt.y));
		}
		// through
	case DXFCIRCLEDATA:
		// ｵﾌﾞｼﾞｪｸﾄの移動ﾃﾞｰﾀ生成
		pt = pData->GetEndMakePoint();
		if ( pt.x!=ms_xyz[NCA_X] || pt.y!=ms_xyz[NCA_Y] ) {
			if ( bL0 ) {
				m_strGcode = MakeStrBlock((*ms_pfnGetCycleString)() +
					GetValString(NCA_X, pt.x) + GetValString(NCA_Y, pt.y) +
					GetValString(NCA_L, 0));
			}
			else {
				m_strGcode = MakeStrBlock((*ms_pfnGetGString)(0) +
					GetValString(NCA_X, pt.x) + GetValString(NCA_Y, pt.y));
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
CNCMakeMill::CNCMakeMill(int nCode, float ZVal, float dFeed)
{
	CString	strGcode;
	CString	strValue(GetValString(NCA_Z, ZVal));
	if ( !strValue.IsEmpty() ) {
		strGcode = (*ms_pfnGetGString)(nCode) + strValue;
		if ( dFeed > 0 )
			strGcode += GetFeedString(dFeed);
	}
	if ( !strGcode.IsEmpty() ) {
		m_strGcode = MakeStrBlock(strGcode);
	}
}

// XYのG[0|1]移動
CNCMakeMill::CNCMakeMill(int nCode, const CPointF& pt, float dFeed)
{
	CString	strGcode(GetValString(NCA_X, pt.x) +
					 GetValString(NCA_Y, pt.y) );
	if ( !strGcode.IsEmpty() ) {
		if ( nCode != 0 )	// G00以外
			strGcode += GetFeedString(dFeed);
		m_strGcode = MakeStrBlock((*ms_pfnGetGString)(nCode) + strGcode);
	}
}

// XYZのG01
CNCMakeMill::CNCMakeMill(const CPoint3F& pt, float dFeed)
{
	CString	strGcode(GetValString(NCA_X, pt.x) +
					 GetValString(NCA_Y, pt.y) +
					 GetValString(NCA_Z, pt.z) );
	if ( !strGcode.IsEmpty() ) {
		m_strGcode = MakeStrBlock((*ms_pfnGetGString)(1) + strGcode + GetFeedString(dFeed));
	}
}

// 座標指示による円弧の生成
CNCMakeMill::CNCMakeMill
	(int nCode, const CPointF& pts, const CPointF& pte, const CPointF& pto, float r)
{
	CString	strGcode( (*ms_pfnMakeCircleSub)(nCode, pte, pto-pts, r) );
	if ( !strGcode.IsEmpty() ) {
		m_strGcode = MakeStrBlock(strGcode + GetFeedString(GetDbl(MKNC_DBL_FEED)));
	}
}

// ドウェル時間（G04）
CNCMakeMill::CNCMakeMill(float t)
{
	m_strGcode = MakeStrBlock((*ms_pfnGetGString)(4) + GetValString(NCA_P, t));
}

// 任意の文字列ｺｰﾄﾞ
CNCMakeMill::CNCMakeMill(const CString& strGcode) : CNCMakeBase(strGcode)
{
}

// Kodatuno座標
CNCMakeMill::CNCMakeMill(const Coord& xyz) :
	CNCMakeMill( CPoint3D(xyz), GetDbl(MKNC_DBL_FEED) )	// 委譲コンストラクタ(XYZのG01へ)
{
	// double -> float のキャストが入るので改良の余地あり
}

//////////////////////////////////////////////////////////////////////
// ﾒﾝﾊﾞ関数

void CNCMakeMill::MakePolylineMov(const CDXFpolyline* pPoly, BOOL bL0)
{
	if ( pPoly->GetVertexCount() <= 1 )
		return;

	CPointF	pt;
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
					GetValString(NCA_X, pt.x) +
					GetValString(NCA_Y, pt.y) +
					GetValString(NCA_L, 0);
			else
				strGcode = (*ms_pfnGetGString)(0) +
					GetValString(NCA_X, pt.x) +
					GetValString(NCA_Y, pt.y);
			if ( !strGcode.IsEmpty() )
				AddGcodeArray(strGcode);
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

CString CNCMakeMill::GetValString(int xyz, float dVal, BOOL bSpecial)
{
	// *** CNCMakeWire と共用 ***
	// WireMode でKとLの生成には、bSpecialにTRUEを指示

	extern	LPCTSTR	g_szNdelimiter;		// "XYZUVWIJKRPLDH" from NCDoc.cpp
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
				float	d = dVal;
				dVal -= ms_xyz[xyz];
				ms_xyz[xyz] = d;
			}
		}
		break;
	case NCA_U:		// WireMode
	case NCA_V:
		break;			// ｾﾞﾛも省略せずに出力
	case NCA_I:		// [I|J]0も出力 -> bSpecial==TRUE
	case NCA_J:
	case NCA_K:		// WireMode -> bSpecial==TRUE
		if ( !bSpecial && fabs(dVal)<NCMIN )	// NCの桁落ち誤差未満なら無視
			return strResult;
		break;
	case NCA_L:
		if ( !bSpecial ) {
			// 小数点指定なし
			strResult = (g_szNdelimiter[xyz] + lexical_cast<string>((int)dVal)).c_str();
			return strResult;
		}
		break;		// -> WireMode
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
	default:
		// 小数点指定なし
		strResult = (g_szNdelimiter[xyz] + lexical_cast<string>((int)dVal)).c_str();
		return strResult;
	}

	// 数値書式はｵﾌﾟｼｮﾝによって動的に関数を呼び出す
	strResult = g_szNdelimiter[xyz] + (*ms_pfnGetValDetail)(dVal);

	return strResult;
}

void CNCMakeMill::SetStaticOption(const CNCMakeMillOpt* pNCMake)
{
	extern	LPCTSTR	gg_szReturn;	// "\n"
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
	if ( GetNum(MKNC_NUM_DOT) < 2 ) {
		ms_pfnGetValDetail = GetFlg(MKNC_FLG_ZEROCUT) ?
				&GetValString_UZeroCut : &GetValString_Normal;
		if ( GetNum(MKNC_NUM_DOT) == 0 )
			_dp.SetDecimal3();		// 小数第3位
		else
			_dp.SetDecimal4();		// 小数第4位
	}
	else {
		ms_pfnGetValDetail = &GetValString_Multi1000;
	}
	// --- 行番号増加分
	ms_nMagni = GetNum(MKNC_NUM_LINEADD)<0 ||
					 GetNum(MKNC_NUM_LINEADD)>=SIZEOF(nLineMulti) ?
		nLineMulti[0] : nLineMulti[GetNum(MKNC_NUM_LINEADD)];
	// --- EOB
	ms_strEOB = GetStr(MKNC_STR_EOB) + gg_szReturn;
	// --- 固定ｻｲｸﾙ指示
	ms_nCycleReturn = GetNum(MKNC_NUM_ZRETURN) == 0 ? 98 : 99;
	if ( GetNum(MKNC_NUM_DRILLRETURN) == 2 ) {
		ms_nCycleCode = 83;
	}
	else {
		if ( GetDbl(MKNC_DBL_DWELL) > 0 )
			ms_nCycleCode = GetNum(MKNC_NUM_DRILLRETURN) == 0 ? 82 : 89;
		else
			ms_nCycleCode = GetNum(MKNC_NUM_DRILLRETURN) == 0 ? 81 : 85;
	}
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
	ms_bIJValue = !GetFlg(MKNC_FLG_ZEROCUT_IJ);
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
