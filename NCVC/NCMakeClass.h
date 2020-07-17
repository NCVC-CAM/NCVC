// NCMakeClass.h: CNCMake クラスのインターフェイス
//		生成の高速化のため，動的関数呼び出しとｲﾝﾗｲﾝ関数を多用
//////////////////////////////////////////////////////////////////////

#pragma once

#include "DXFdata.h"
#include "NCMakeOption.h"
#include "NCVCdefine.h"

typedef struct	tagMAKECIRCLE {	// 円弧生成の引数
	int		g;
	CPointD	pt;
	double	r, i, j;
} MAKECIRCLE,  *LPMAKECIRCLE;

typedef	CString	(*PFNGETSPINDLE)(int);
typedef	CString	(*PFNGETFEED)(double);
typedef CString (*PFNGETVALSTRING)(double);
typedef CString (*PFNGETLINENO)(void);
typedef CString (*PFNGETGSTRING)(int);
typedef CString (*PFNGETCYCLESTRING)(void);
typedef	CString	(*PFNMAKECIRCLESUB)(int, const CPointD&, const CPointD&, double);
typedef	CString	(*PFNMAKECIRCLE)(const CDXFcircle*, double);
typedef	CString	(*PFNMAKEARC)(const CDXFarc*, double, const CPointD*);

// 固定ｻｲｸﾙ認識ｺｰﾄﾞ
#define	NCMAKECYCLECODE		81

class CNCMake
{
	CString			m_strGcode;		// 生成されたGｺｰﾄﾞ(1ﾌﾞﾛｯｸ)
	CStringArray	m_strGarray;	// POLYLINE, Ellipse等複数のGｺｰﾄﾞ生成

	// 生成中は変化のないｵﾌﾟｼｮﾝに対する動作
	static	PFNGETSPINDLE		ms_pfnGetSpindle;		// Sｺｰﾄﾞの生成
	static	PFNGETFEED			ms_pfnGetFeed;			// Fｺｰﾄﾞの生成
	static	PFNGETLINENO		ms_pfnGetLineNo;		// 行番号付加
	static	PFNGETGSTRING		ms_pfnGetGString;		// Gｺｰﾄﾞﾓｰﾀﾞﾙ
	static	PFNGETCYCLESTRING	ms_pfnGetCycleString;	// Gｺｰﾄﾞﾓｰﾀﾞﾙ(固定ｻｲｸﾙ)
	static	PFNGETVALSTRING		ms_pfnGetValString;		// 座標値設定
	static	PFNMAKECIRCLESUB	ms_pfnMakeCircleSub;	// 円・円弧ﾃﾞｰﾀの生成補助
	static	PFNMAKECIRCLE		ms_pfnMakeCircle;		// 円ﾃﾞｰﾀの生成
	static	PFNMAKEARC			ms_pfnMakeArc;			// 円弧ﾃﾞｰﾀの生成

	static	int		ms_nGcode;		// 前回のGｺｰﾄﾞ
	static	int		ms_nSpindle;	// 前回の回転数
	static	double	ms_dFeed;		// 前回の切削送り速度
	static	int		ms_nCnt;		// ﾌｧｲﾙ出力時の行ﾅﾝﾊﾞｰ
	static	int		ms_nMagni;		// 行番号倍率
	static	int		ms_nCircleCode;	// 円ﾃﾞｰﾀの切削指示(2 or 3)
	static	int		ms_nCycleCode;	// 固定ｻｲｸﾙの切削指示(81,82,85,89)
	static	int		ms_nCycleReturn;// 固定ｻｲｸﾙの復帰指示(88,89)
	static	CString	ms_strEOB;		// EOB

	// 回転指示
	static	CString	GetSpindleString(int nSpindle) {
		CString	strResult;
		if ( ms_nSpindle != nSpindle ) {
			strResult.Format("S%d", nSpindle);
			ms_nSpindle = nSpindle;
		}
		return strResult;
	}
	static	CString	GetSpindleString_Clip(int) {
		return CString();
	}
	// 送り速度
	static	CString	GetFeedString(double dFeed) {
		CString		strResult;
		if ( ms_dFeed != dFeed ) {
			ms_dFeed = dFeed;
			strResult = "F" + (*ms_pfnGetFeed)(dFeed);
		}
		return strResult;
	}
	static	CString	GetFeedString_Integer(double dFeed) {
		CString	strResult;
		strResult.Format("%d", (int)dFeed);
		return strResult;
	}
	// 行番号付加
	static	CString	GetLineNoString(void) {
		CString	strResult;
		strResult.Format(ms_pMakeOpt->GetStr(MKNC_STR_LINEFORM), ms_nCnt++ * ms_nMagni);
		return strResult;
	}
	static	CString	GetLineNoString_Clip(void) {
		return CString();
	}
	// Gｺｰﾄﾞﾓｰﾀﾞﾙ
	static	CString	GetGString(int nCode) {
		CString		strResult;
		strResult.Format("G%02d", nCode);
		return strResult;
	}
	static	CString	GetGString_Clip(int nCode) {
		CString		strResult;
		if ( ms_nGcode != nCode ) {
			strResult = GetGString(nCode);
			ms_nGcode = nCode;
		}
		return strResult;
	}
	// Gｺｰﾄﾞﾓｰﾀﾞﾙ(固定ｻｲｸﾙ)
	static	CString	GetCycleString(void) {
		return GetGString(ms_nCycleReturn) + GetGString(ms_nCycleCode);
	}
	static	CString	GetCycleString_Clip(void) {
		CString		strResult;
		if ( ms_nGcode != NCMAKECYCLECODE ) {
			strResult = GetCycleString();
			ms_nGcode = NCMAKECYCLECODE;
		}
		return strResult;
	}
	// 座標値設定
	static	CString	GetValString(int, double, BOOL = FALSE);
	static	CString	GetValString_Normal(double dVal) {
		CString		strResult;
		strResult.Format(IDS_MAKENCD_FORMAT, dVal);
		return strResult;
	}
	static	CString	GetValString_UZeroCut(double);
	static	CString	GetValString_Multi1000(double dVal) {
		CString		strResult;
/*		--- RoundUp() にて処理済みのため不要
		dVal *= 1000.0;
		dVal  = (dVal<0 ? floor(dVal) : ceil(dVal));
		strResult.Format("%ld", (long)dVal);
*/
		// 単純な1000倍では，丸め誤差が発生(??)
		strResult.Format("%d", (int)(dVal*1000.0+_copysign(0.0001, dVal)));
		return strResult;
	}
	// 円・円弧の生成補助
	static	CString	MakeCircleSub_R(int nCode, const CPointD& pt, const CPointD&, double r) {
		return CString( (*ms_pfnGetGString)(nCode) +
			GetValString(NCA_X, pt.x) + GetValString(NCA_Y, pt.y) + GetValString(NCA_R, r) );
	}
	static	CString	MakeCircleSub_IJ(int nCode, const CPointD& pt, const CPointD& ptij, double) {
		return CString( (*ms_pfnGetGString)(nCode) +
			GetValString(NCA_X, pt.x) + GetValString(NCA_Y, pt.y) +
			GetValString(NCA_I, ptij.x) + GetValString(NCA_J, ptij.y) );
	}
	// 円ﾃﾞｰﾀの生成
	static	CString	MakeCircle_R(const CDXFcircle* pCircle, double dFeed) {
		int		a = pCircle->GetBaseAxis(), b = a & 0x01 ? (a-1) : (a+1);
		double	r = pCircle->GetMakeR();
		int		nCode = pCircle->IsRoundFixed() ? pCircle->GetG() : ms_nCircleCode;
		CPointD	pt;		// dummy
		// R指定，180゜ずつ分けて生成
		CString	strGcode;
		CString	strBuf1( MakeCircleSub_R(nCode, pCircle->GetMakePoint(b), pt, r) );
		CString	strBuf2( MakeCircleSub_R(nCode, pCircle->GetMakePoint(a), pt, r) );
		if ( !strBuf1.IsEmpty() )
			strGcode = (*ms_pfnGetLineNo)() + strBuf1 + GetFeedString(dFeed) + ms_strEOB +
							(*ms_pfnGetLineNo)() + strBuf2 + ms_strEOB;
		return strGcode;
	}
	static	CString	MakeCircle_IJ(const CDXFcircle* pCircle, double dFeed) {
		CString	strGcode, strBuf( pCircle->GetBaseAxis() > 1 ?	// X軸かY軸か
							GetValString(NCA_J, pCircle->GetIJK(NCA_J)) :
							GetValString(NCA_I, pCircle->GetIJK(NCA_I)) ); 
		int		nCode = pCircle->IsRoundFixed() ? pCircle->GetG() : ms_nCircleCode;
		if ( !strBuf.IsEmpty() )
			strGcode = (*ms_pfnGetLineNo)() + (*ms_pfnGetGString)(nCode) +
						strBuf + GetFeedString(dFeed) + ms_strEOB;
		return strGcode;
	}
	static	CString	MakeCircle_IJ_HALF(const CDXFcircle* pCircle, double dFeed) {
		int		a = pCircle->GetBaseAxis(), b = a & 0x01 ? (a-1) : (a+1);
		int		nCode = pCircle->IsRoundFixed() ? pCircle->GetG() : ms_nCircleCode;
		CPointD	ij;
		// 基準軸の決定
		if ( a > 1 ) 	// Y軸ﾍﾞｰｽ
			ij.y = pCircle->GetIJK(NCA_J);
		else 			// X軸ﾍﾞｰｽ
			ij.x = pCircle->GetIJK(NCA_I);
		CString	strGcode;
		CString	strBuf1( MakeCircleSub_IJ(nCode, pCircle->GetMakePoint(b), ij, 0.0) );
		ij *= -1.0;		// ij = -ij;
		CString	strBuf2( MakeCircleSub_IJ(nCode, pCircle->GetMakePoint(a), ij, 0.0) );
		if ( !strBuf1.IsEmpty() )
			strGcode = (*ms_pfnGetLineNo)() + strBuf1 + GetFeedString(dFeed) + ms_strEOB +
							(*ms_pfnGetLineNo)() + strBuf2 + ms_strEOB;
		return strGcode;
	}
	// 円弧ﾃﾞｰﾀの生成
	static	CString	MakeArc_R(const CDXFarc* pArc, double dFeed, const CPointD* lpt) {
		CPointD	ij, pt;
		if ( lpt ) {
			pt.x = lpt->x;
			pt.y = lpt->y;
		}
		else
			pt = pArc->GetEndMakePoint();
		CString	strGcode,
				strBuf( MakeCircleSub_R(pArc->GetG(), pt, ij, pArc->GetMakeR()) );
		if ( !strBuf.IsEmpty() )
			strGcode = (*ms_pfnGetLineNo)() + strBuf + GetFeedString(dFeed) + ms_strEOB;
		return strGcode;
	}
	static	CString	MakeArc_IJ(const CDXFarc* pArc, double dFeed, const CPointD* lpt) {
		CPointD	ij(pArc->GetIJK(NCA_I), pArc->GetIJK(NCA_J)), pt;
		if ( lpt ) {
			pt.x = lpt->x;
			pt.y = lpt->y;
		}
		else
			pt = pArc->GetEndMakePoint();
		CString	strGcode,
				strBuf( MakeCircleSub_IJ(pArc->GetG(), pt, ij, 0.0) );
		if ( !strBuf.IsEmpty() )
			strGcode = (*ms_pfnGetLineNo)() + strBuf + GetFeedString(dFeed) + ms_strEOB;
		return strGcode;
	}
	// 楕円ﾃﾞｰﾀの生成(直線補間)
	void	MakeEllipse(const CDXFellipse *, double);
	static	CString	MakeEllipse_Tolerance(const CDXFellipse* pEllipse, double q) {
		CPointD	pt( pEllipse->GetLongLength() * cos(q),
						pEllipse->GetShortLength() * sin(q) );
		CPointD	ptMake( pt.x * pEllipse->GetLeanCos() - pt.y * pEllipse->GetLeanSin(),
						pt.x * pEllipse->GetLeanSin() + pt.y * pEllipse->GetLeanCos() );
		ptMake += pEllipse->GetMakeCenter();
		pt = ptMake.RoundUp();
		return CString( (*ms_pfnGetGString)(1) + GetValString(NCA_X, pt.x) + GetValString(NCA_Y, pt.y) );
	}
	// Polyline の生成
	void	MakePolylineCut(const CDXFpolyline*, double);
	void	MakePolylineMov(const CDXFpolyline*, BOOL);

public:
	// 切削ﾃﾞｰﾀ
	CNCMake(const CDXFdata*, double, const CPointD* = NULL);
	// 加工開始位置指示ﾃﾞｰﾀのXY移動
	CNCMake(const CDXFdata*, BOOL = FALSE);
	// Z軸の変化(上昇・下降)
	CNCMake(int nCode, double ZVal, double dFeed) {
		CString	strGcode;
		CString	strValue(GetValString(NCA_Z, ZVal));
		if ( !strValue.IsEmpty() ) {
			strGcode = (*ms_pfnGetGString)(nCode) + strValue;
			if ( dFeed > 0 )
				strGcode += GetFeedString(dFeed);
		}
		if ( !strGcode.IsEmpty() )
			m_strGcode = (*ms_pfnGetLineNo)() + strGcode + ms_strEOB;
	}
	// XYのG[0|1]移動
	CNCMake(int nCode, const CPointD& pt) {
		CString	strGcode(GetValString(NCA_X, pt.x) + GetValString(NCA_Y, pt.y));
		if ( !strGcode.IsEmpty() ) {
			if ( nCode != 0 )	// G0以外
				strGcode += GetFeedString(ms_pMakeOpt->GetDbl(MKNC_DBL_FEED));
			m_strGcode = (*ms_pfnGetLineNo)() + (*ms_pfnGetGString)(nCode) + strGcode + ms_strEOB;
		}
	}
	// 座標指示による円弧の生成
	CNCMake(int nCode, const CPointD& pts, const CPointD& pte, const CPointD& pto, double r) {
		CString	strGcode( (*ms_pfnMakeCircleSub)(nCode, pte, pto-pts, r) );
		if ( !strGcode.IsEmpty() )
			m_strGcode = (*ms_pfnGetLineNo)() + strGcode + GetFeedString(ms_pMakeOpt->GetDbl(MKNC_DBL_FEED)) + ms_strEOB;
	}
	// 任意の文字列ｺｰﾄﾞ
	CNCMake(const CString& strGcode) {
		extern	LPCTSTR	gg_szReturn;
		if ( strGcode.IsEmpty() )
			m_strGcode = gg_szReturn;
		else {
			if ( strGcode[0] == '%' )
				m_strGcode = strGcode + gg_szReturn;
			else
				m_strGcode = (*ms_pfnGetLineNo)() + strGcode + ms_strEOB;
		}
	}

	// 生成ｵﾌﾟｼｮﾝによる静的変数の初期化(TH_MakeNCD.cpp)
	static	void	SetStaticOption(void);
	static	void	InitialVariable(void) {
		ms_nGcode = -1;
		ms_nSpindle = -1;
		ms_dFeed = -1.0;
		ms_nCnt = 1;
	}

	// TH_MakeNCD.cpp で初期化
	static	const	CNCMakeOption*	ms_pMakeOpt;
	static	double	ms_xyz[NCXYZ];	// 前回の位置
	static	double	ms_dCycleZ[2];	// 固定ｻｲｸﾙの切り込みZ座標
	static	double	ms_dCycleR[2];	// 固定ｻｲｸﾙのR点
	static	double	ms_dCycleP[2];	// 固定ｻｲｸﾙのﾄﾞｳｪﾙ時間

	// TH_MakeNCD.cpp からも参照
	static	CString	MakeSpindle(ENDXFTYPE enType, BOOL bDeep = FALSE) {
		CString	strResult;
		if ( enType != DXFPOINTDATA ) {
			if ( bDeep )
				strResult = (*ms_pfnGetSpindle)(ms_pMakeOpt->GetNum(MKNC_NUM_DEEPSPINDLE));
			else
				strResult = (*ms_pfnGetSpindle)(ms_pMakeOpt->GetNum(MKNC_NUM_SPINDLE));
		}
		else
			strResult = (*ms_pfnGetSpindle)(ms_pMakeOpt->GetNum(MKNC_NUM_DRILLSPINDLE));
		return strResult;
	}
	static	CString	MakeCustomString(int, DWORD = 0, double* = NULL, BOOL = TRUE);
	static	CString	MakeCustomString(int, int[], double[]);

	// Gｺｰﾄﾞ出力
	void	WriteGcode(CStdioFile& fp) {
		if ( !m_strGcode.IsEmpty() )
			fp.WriteString( m_strGcode );
		for ( int i=0; i<m_strGarray.GetSize(); i++ )
			fp.WriteString( m_strGarray[i] );
	}
};
