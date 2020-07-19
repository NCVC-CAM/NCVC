// NCMakeLathe.cpp: CNCMakeLathe クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "DXFdata.h"
#include "NCDoc.h"		// g_szNCcomment[]
#include "NCMakeLatheOpt.h"
#include "NCMakeLathe.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

using std::string;
using namespace boost;

// よく使う変数や呼び出しの簡略置換
#define	GetFlg		ms_pMakeOpt->GetFlag
#define	GetNum		ms_pMakeOpt->GetNum
#define	GetDbl		ms_pMakeOpt->GetDbl
#define	GetStr		ms_pMakeOpt->GetStr

//////////////////////////////////////////////////////////////////////
// CNCMakeMill 構築/消滅
//////////////////////////////////////////////////////////////////////

CNCMakeLathe::CNCMakeLathe(void)
{
}

CNCMakeLathe::CNCMakeLathe(const CDXFdata* pData, float dFeed)
{
	switch ( pData->GetMakeType() ) {
	case DXFLINEDATA:
	{
		CPointF	pt(pData->GetEndMakePoint());
		CString	strGcode(GetValString(NCA_Z, pt.x)+GetValString(NCA_X, pt.y));
		if ( !strGcode.IsEmpty() )
			m_strGcode = (*ms_pfnGetLineNo)() + (*ms_pfnGetGString)(1) +
						strGcode + GetFeedString(dFeed) + ms_strEOB;
	}
		break;

	case DXFCIRCLEDATA:
		m_strGcode = (*ms_pfnMakeCircle)(static_cast<const CDXFcircle*>(pData), dFeed);
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

// 指定位置に直線移動
CNCMakeLathe::CNCMakeLathe(const CPointF& pt)
{
	CString	strGcode( (*ms_pfnGetGString)(0) +
		GetValString(NCA_Z, pt.x) + GetValString(NCA_X, pt.y) );
	if ( !strGcode.IsEmpty() )
		m_strGcode = (*ms_pfnGetLineNo)() + strGcode + ms_strEOB;
}

// 指定位置に２軸移動
CNCMakeLathe::CNCMakeLathe(TWOMOVEMODE enMode, const CPointF& pt, float dFeed)
{
	CString	strGcode1, strGcode2, strGcode;

	if ( enMode == ZXMOVE ) {
		// Z軸移動後、X軸移動
		strGcode1 = (*ms_pfnGetGString)(0) + GetValString(NCA_Z, pt.x);
		strGcode  = (*ms_pfnGetGString)(1) + GetValString(NCA_X, pt.y);
		if ( !strGcode.IsEmpty() )
			strGcode2 = strGcode + GetFeedString(dFeed);
	}
	else {
		// X軸移動後、Z軸移動
		strGcode  = (*ms_pfnGetGString)(1) + GetValString(NCA_X, pt.y);
		if ( !strGcode.IsEmpty() )
			strGcode1 = strGcode + GetFeedString(dFeed);
		strGcode2 = (*ms_pfnGetGString)(0) + GetValString(NCA_Z, pt.x);
	}
	if ( !strGcode1.IsEmpty() )
		AddGcode(strGcode1);
	if ( !strGcode2.IsEmpty() )
		AddGcode(strGcode2);
}

// X|Z軸の変化
CNCMakeLathe::CNCMakeLathe(int nCode, int xz, float dVal, float dFeed)
{
	CString	strGcode;
	CString	strValue(GetValString(xz, dVal));
	if ( !strValue.IsEmpty() ) {
		strGcode = (*ms_pfnGetGString)(nCode) + strValue;
		if ( nCode > 0 )	// G00以外
			strGcode += GetFeedString(dFeed);
	}
	if ( !strGcode.IsEmpty() )
		m_strGcode = (*ms_pfnGetLineNo)() + strGcode + ms_strEOB;
}

// 任意の文字列ｺｰﾄﾞ
CNCMakeLathe::CNCMakeLathe(const CString& strGcode) : CNCMakeBase(strGcode)
{
}

//////////////////////////////////////////////////////////////////////
// ﾒﾝﾊﾞ関数

void CNCMakeLathe::CreateEndFace(const CPointF& pts)
{
	CString		strGcode;

	// 回転数設定（ｶｽﾀﾑﾍｯﾀﾞｰで処理済み）
	strGcode = MakeSpindle(GetNum(MKLA_NUM_E_SPINDLE));
	if ( !strGcode.IsEmpty() ) {
		AddGcode(strGcode);
	}

	// ｶｽﾀﾑｺｰﾄﾞ挿入 改行ｺｰﾄﾞの置換, 行番号付与 etc.
	if ( !GetStr(MKLA_STR_E_CUSTOM).IsEmpty() ) {
		m_strGarray.Add( GetChangeEnter(GetStr(MKLA_STR_E_CUSTOM)) );
	}

	// 外径座標から端面切削開始位置を設定
	CPointF	pt;
	pt.x = max(GetDbl(MKLA_DBL_E_CUT), pts.x+GetDbl(MKLA_DBL_E_STEP));	// MKLA_DBL_ENDSTEPはﾏｲﾅｽ値
	pt.y = pts.y + GetDbl(MKLA_DBL_E_PULLX);
	// Z軸の移動
	strGcode = (*ms_pfnGetGString)(0) + GetValString(NCA_Z, pt.x);
	if ( !strGcode.IsEmpty() )
		AddGcode(strGcode);
	// X軸の切削移動
	strGcode = (*ms_pfnGetGString)(0) + GetValString(NCA_X, pt.y);
	if ( !strGcode.IsEmpty() )
		AddGcode(strGcode);

	// 最終切り込みまで繰り返し
	while (TRUE) {
		// 中心まで切削送り
		strGcode = (*ms_pfnGetGString)(1) + GetValString(NCA_X, 0) + GetFeedString(GetDbl(MKLA_DBL_E_FEED));;
		AddGcode(strGcode);
		// 引き代分移動(Z軸移動がG00だとOpenGL描画に反映されない)
		strGcode = (*ms_pfnGetGString)(1) + GetValString(NCA_Z, pt.x+GetDbl(MKLA_DBL_E_PULLZ));
		AddGcode(strGcode);
		strGcode = (*ms_pfnGetGString)(0) + GetValString(NCA_X, pt.y);
		AddGcode(strGcode);
		// 次の端面座標
		if ( fabs(GetDbl(MKLA_DBL_E_CUT)-pt.x) < NCMIN )
			break;
		pt.x = max(GetDbl(MKLA_DBL_E_CUT), pt.x+GetDbl(MKLA_DBL_E_STEP));
		strGcode = (*ms_pfnGetGString)(0) + GetValString(NCA_Z, pt.x);
		AddGcode(strGcode);
	}
}

void CNCMakeLathe::CreatePilotHole(void)
{
	extern	LPCTSTR	g_szNCcomment[];
	CString		strGcode;
	VLATHEDRILLINFO	v;

	// 既存下穴ｻｲｽﾞ
	if ( GetDbl(MKLA_DBL_HOLE) > 0.0f ) {
		CString	strFmt;
		strFmt.Format(IDS_MAKENCD_FORMAT, GetDbl(MKLA_DBL_HOLE));
		strGcode = LATHEHOLE_S;		// g_szNCcomment[LATHEHOLE]
		strGcode += '=' + strFmt;
		AddGcode( '(' + strGcode + ')' );
	}

	// ドリル情報
	if ( !static_cast<const CNCMakeLatheOpt*>(ms_pMakeOpt)->GetDrillInfo(v) )
		return;

	for ( const auto& info : v ) {
		// 回転数設定
		strGcode = MakeSpindle(info.s);
		if ( !strGcode.IsEmpty() ) {
			AddGcode(strGcode);
		}
		// ドリル情報
		strGcode.Format(IDS_MAKENCD_LATHEDRILL, info.d);
		AddGcode(strGcode);
		// ｶｽﾀﾑｺｰﾄﾞ挿入 改行ｺｰﾄﾞの置換, 行番号付与 etc.
		if ( !GetStr(MKLA_STR_D_CUSTOM).IsEmpty() ) {
			m_strGarray.Add( GetChangeEnter(GetStr(MKLA_STR_D_CUSTOM)) );
		}
		// 切削開始位置(R点)へ移動
		if ( ms_xyz[NCA_Z] != GetDbl(MKLA_DBL_DRILLR) ) {
			strGcode = (*ms_pfnGetGString)(0) + GetValString(NCA_Z, GetDbl(MKLA_DBL_DRILLR));
			AddGcode(strGcode);
		}
		if ( ms_xyz[NCA_X] != 0 ) {
			strGcode = (*ms_pfnGetGString)(0) + GetValString(NCA_X, 0);
			AddGcode(strGcode);
		}
		if ( GetFlg(MKLA_FLG_CYCLE) ) {
			// 固定ｻｲｸﾙで生成
			strGcode = (*ms_pfnGetGString)(83) +
				GetValString(NCA_Z, GetDbl(MKLA_DBL_DRILLZ)) + GetValString(NCA_X, 0);
//				GetValString(NCA_R, GetDbl(MKLA_DBL_DRILLR), TRUE);	// 上で移動しているので不要やけど...
			if ( GetDbl(MKLA_DBL_DRILLQ) > 0 )
				strGcode += "Q" + (*ms_pfnGetValDetail)(GetDbl(MKLA_DBL_DRILLQ));
			if ( GetDbl(MKLA_DBL_DWELL) > 0 )
				strGcode += GetValString(NCA_P, GetDbl(MKLA_DBL_DWELL));
			strGcode += GetFeedString(info.f);
			AddGcode(strGcode);
			// Z値を元に戻す
			ms_xyz[NCA_Z] = GetDbl(MKLA_DBL_DRILLR);
		}
		else {
			// 直線補間で生成
			float z = ms_xyz[NCA_Z];	// GetDbl(MKLA_DBL_DRILLR)
			while (TRUE) {
				z -= GetDbl(MKLA_DBL_DRILLQ);
				if ( z <= GetDbl(MKLA_DBL_DRILLZ) ) {
					strGcode = (*ms_pfnGetGString)(1) + GetValString(NCA_Z, GetDbl(MKLA_DBL_DRILLZ)) + GetFeedString(info.f);
					AddGcode(strGcode);
					if ( GetDbl(MKLA_DBL_DWELL) > 0 ) {
						strGcode = (*ms_pfnGetGString)(4) + GetValString(NCA_P, GetDbl(MKLA_DBL_DWELL));
						AddGcode(strGcode);
					}
					strGcode = (*ms_pfnGetGString)(0) + GetValString(NCA_Z, GetDbl(MKLA_DBL_DRILLR));
					AddGcode(strGcode);
					break;
				}
				strGcode = (*ms_pfnGetGString)(1) + GetValString(NCA_Z, z) + GetFeedString(info.f);
				AddGcode(strGcode);
				strGcode = (*ms_pfnGetGString)(0) + GetValString(NCA_Z, z+GetDbl(MKLA_DBL_DRILLD));
				AddGcode(strGcode);
			}
		}
	}

	if ( GetFlg(MKLA_FLG_CYCLE) ) {
		// 固定ｻｲｸﾙｷｬﾝｾﾙ
		AddGcode( (*ms_pfnGetGString)(80) );
	}
	// ﾄﾞﾘﾙ工程終了ｺﾒﾝﾄ
	strGcode = ENDDRILL_S;	// g_szNCcomment[ENDDRILL]
	AddGcode( '(' + strGcode + ')' );
}

CString	CNCMakeLathe::MakeSpindle(int s)
{
	return (*ms_pfnGetSpindle)(s);
}

CString CNCMakeLathe::GetValString(int xyz, float dVal, BOOL bSpecial)
{
	extern	LPCTSTR	g_szNdelimiter;		// "XYZUVWIJKRPLDH" from NCDoc.cpp
	CString	strResult;

	// 小数点以下３桁(以下切り上げ)での値合致ﾁｪｯｸ
	switch ( xyz ) {
	case NCA_X:
	case NCA_Y:
	case NCA_Z:
		if ( GetFlg(MKLA_FLG_GCLIP) && fabs(ms_xyz[xyz]-dVal)<NCMIN )
			return strResult;
		else {
			if ( GetNum(MKLA_NUM_G90) == 0 ) {	// ｱﾌﾞｿﾘｭｰﾄ
				ms_xyz[xyz] = dVal;
			}
			else {								// ｲﾝｸﾘﾒﾝﾀﾙ
				float	d = dVal;
				dVal -= ms_xyz[xyz];
				ms_xyz[xyz] = d;
			}
		}
		break;
	case NCA_R:
		// 半径Rではなく固定ｻｲｸﾙR点の場合
		if ( bSpecial ) {
			if ( GetNum(MKLA_NUM_G90) != 0 ) {	// ｲﾝｸﾘﾒﾝﾀﾙなら
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
	case NCA_P:		// 固定ｻｲｸﾙﾄﾞｳｪﾙ時間
		strResult = (g_szNdelimiter[xyz] + lexical_cast<string>((int)dVal)).c_str();
		return strResult;
	}

	// 数値書式はｵﾌﾟｼｮﾝによって動的に関数を呼び出す
	if ( xyz == NCA_X )
		dVal *= 2.0f;	// X軸直径指示
	strResult = g_szNdelimiter[xyz] + (*ms_pfnGetValDetail)(dVal);

	return strResult;
}

void CNCMakeLathe::SetStaticOption(const CNCMakeLatheOpt* pNCMake)
{
	extern	LPCTSTR	gg_szReturn;	// "\n"
	static	const	int		nLineMulti[] = {	// 行番号増加分
		1, 5, 10, 100
	};

	CNCMakeBase::SetStaticOption(pNCMake);

	// --- 軸指示
	NCAX = NCA_Z;	NCAY = NCA_X;
	NCAI = NCA_K;	NCAJ = NCA_I;
	// --- 回転指示
	ms_pfnGetSpindle = GetFlg(MKLA_FLG_DISABLESPINDLE) ?
		&GetSpindleString_Clip : &GetSpindleString;
	// --- 送り指示
	switch ( GetNum(MKLA_NUM_FDOT) ) {
	case 0:		// 小数点表記
		ms_pfnGetFeed = GetFlg(MKLA_FLG_ZEROCUT) ?
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
	ms_pfnGetLineNo = GetFlg(MKLA_FLG_LINEADD) && !(GetStr(MKLA_STR_LINEFORM).IsEmpty()) ?
		&GetLineNoString : &GetLineNoString_Clip;
	// --- Gｺｰﾄﾞﾓｰﾀﾞﾙ
	ms_pfnGetGString = GetFlg(MKLA_FLG_GCLIP) ?
		&GetGString_Clip : &GetGString;
	// --- 座標表記
	ms_pfnGetValString = &GetValString;	// ﾍﾞｰｽｸﾗｽからの呼出用
	ms_pfnGetValDetail = GetNum(MKLA_NUM_DOT) == 0 ?
		(GetFlg(MKLA_FLG_ZEROCUT) ?
			&GetValString_UZeroCut : &GetValString_Normal) : &GetValString_Multi1000;
	// --- 行番号増加分
	ms_nMagni = GetNum(MKLA_NUM_LINEADD)<0 ||
					 GetNum(MKLA_NUM_LINEADD)>=SIZEOF(nLineMulti) ?
		nLineMulti[0] : nLineMulti[GetNum(MKLA_NUM_LINEADD)];
	// --- EOB
	ms_strEOB = GetStr(MKLA_STR_EOB).IsEmpty() ? 
		gg_szReturn : (GetStr(MKLA_STR_EOB) + gg_szReturn);
	// --- 円ﾃﾞｰﾀの切削指示
	ms_nCircleCode = GetNum(MKLA_NUM_CIRCLECODE) == 0 ? 2 : 3;
	// --- 円,円弧ﾃﾞｰﾀの生成
	if ( GetNum(MKLA_NUM_IJ) == 0 ) {
		ms_pfnMakeCircle	= &MakeCircle_R;
		ms_pfnMakeCircleSub	= &MakeCircleSub_R;
		ms_pfnMakeHelical	= &MakeCircle_R_Helical;
		ms_pfnMakeArc		= &MakeArc_R;
	}
	else {
		if ( GetFlg(MKLA_FLG_CIRCLEHALF) ) {
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
	ms_bIJValue = !GetFlg(MKLA_FLG_ZEROCUT_IJ);
	// --- 楕円公差
	ms_dEllipse = GetDbl(MKLA_DBL_ELLIPSE);
}
