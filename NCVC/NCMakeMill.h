// NCMakeMill.h: CNCMakeMill クラスのインターフェイス
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCMakeBase.h"

typedef CString (*PFNGETCYCLESTRING)(void);

// 固定ｻｲｸﾙ認識ｺｰﾄﾞ
#define	NCMAKECYCLECODE		81

class CNCMakeMill : public CNCMakeBase
{
	// 生成中は変化のないｵﾌﾟｼｮﾝに対する動作
	static	PFNGETCYCLESTRING	ms_pfnGetCycleString;	// Gｺｰﾄﾞﾓｰﾀﾞﾙ(固定ｻｲｸﾙ)
	static	int		ms_nCycleCode;	// 固定ｻｲｸﾙの切削指示(81,82,85,89)
	static	int		ms_nCycleReturn;// 固定ｻｲｸﾙの復帰指示(88,89)

	// 座標値設定
	static	CString	GetValString(int, double, BOOL);
	// Gｺｰﾄﾞﾓｰﾀﾞﾙ(固定ｻｲｸﾙ)
	static	CString	GetCycleString(void);
	static	CString	GetCycleString_Clip(void);

	// ---
	void	MakePolylineMov(const CDXFpolyline*, BOOL);

public:
	// 切削ﾃﾞｰﾀ
	CNCMakeMill(const CDXFdata*, double, const double* = NULL);
	// 加工開始位置指示ﾃﾞｰﾀのXY移動
	CNCMakeMill(const CDXFdata*, BOOL);
	// Z軸の変化(上昇・下降)
	CNCMakeMill(int, double, double);
	// XYのG[0|1]移動
	CNCMakeMill(int, const CPointD& pt);
	// 座標指示による円弧の生成
	CNCMakeMill(int, const CPointD&, const CPointD&, const CPointD&, double);
	// 任意の文字列ｺｰﾄﾞ
	CNCMakeMill(const CString&);

	// 生成ｵﾌﾟｼｮﾝによる静的変数の初期化(TH_MakeNCD.cpp)
	static	void	SetStaticOption(const CNCMakeMillOpt*);

	// TH_MakeNCD.cpp で初期化
	static	double	ms_dCycleZ[2];	// 固定ｻｲｸﾙの切り込みZ座標
	static	double	ms_dCycleR[2];	// 固定ｻｲｸﾙのR点
	static	double	ms_dCycleP[2];	// 固定ｻｲｸﾙのﾄﾞｳｪﾙ時間

	// TH_MakeNCD.cpp から参照
	static	CString	MakeSpindle(ENDXFTYPE enType, BOOL bDeep = FALSE);
};
