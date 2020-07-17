// NCMakeBase.h: CNCMake クラスのインターフェイス
//////////////////////////////////////////////////////////////////////

#pragma once

typedef	CString	(*PFNGETARGINT)(int);
typedef	CString	(*PFNGETARGDOUBLE)(double);
typedef CString (*PFNGETARGVOID)(void);
typedef	CString	(*PFNGETVALSTRING)(int, double, BOOL);
typedef	CString	(*PFNMAKECIRCLESUB)(int, const CPointD&, const CPointD&, double);
typedef	CString	(*PFNMAKECIRCLE)(const CDXFcircle*, double);
typedef	CString	(*PFNMAKEHELICAL)(const CDXFcircle*, double, double);
typedef	CString	(*PFNMAKEARC)(const CDXFarc*, double);

class CNCMakeBase
{
protected:
	CNCMakeBase();		// 派生ｸﾗｽ用
	// ｺﾝｽﾄﾗｸﾀ -- 任意の文字列ｺｰﾄﾞ
	CNCMakeBase(const CString& strGcode);

	static	const	CNCMakeOption*	ms_pMakeOpt;

	CString			m_strGcode;		// 生成されたGｺｰﾄﾞ(1ﾌﾞﾛｯｸ)
	CStringArray	m_strGarray;	// POLYLINE, Ellipse等複数のGｺｰﾄﾞ生成

	// 生成中は変化のないｵﾌﾟｼｮﾝに対する動作
	static	int		NCAX, NCAY,		// ﾌﾗｲｽと旋盤の軸変換
					NCAI, NCAJ;
	static	int		ms_nGcode;		// 前回のGｺｰﾄﾞ
	static	int		ms_nSpindle;	// 前回の回転数
	static	double	ms_dFeed;		// 前回の切削送り速度
	static	int		ms_nCnt;		// ﾌｧｲﾙ出力時の行ﾅﾝﾊﾞｰ
	static	int		ms_nMagni;		// 行番号倍率
	static	int		ms_nCircleCode;	// 円ﾃﾞｰﾀの切削指示(2 or 3)
	static	BOOL	ms_bIJValue;	// [I|J]0を生成するかどうか
	static	double	ms_dEllipse;	// 楕円公差
	static	CString	ms_strEOB;		// EOB

	static	PFNGETARGINT		ms_pfnGetSpindle;		// Sｺｰﾄﾞの生成
	static	PFNGETARGDOUBLE		ms_pfnGetFeed;			// Fｺｰﾄﾞの生成
	static	PFNGETARGVOID		ms_pfnGetLineNo;		// 行番号付加
	static	PFNGETARGINT		ms_pfnGetGString;		// Gｺｰﾄﾞﾓｰﾀﾞﾙ
	static	PFNGETVALSTRING		ms_pfnGetValString;		// 座標値設定
	static	PFNGETARGDOUBLE		ms_pfnGetValDetail;		// 　の詳細
	static	PFNMAKECIRCLESUB	ms_pfnMakeCircleSub;	// 円・円弧ﾃﾞｰﾀの生成補助
	static	PFNMAKECIRCLE		ms_pfnMakeCircle;		// 円ﾃﾞｰﾀの生成
	static	PFNMAKEHELICAL		ms_pfnMakeHelical;		// 円ﾃﾞｰﾀのﾍﾘｶﾙ切削
	static	PFNMAKEARC			ms_pfnMakeArc;			// 円弧ﾃﾞｰﾀの生成

	// 回転指示
	static	CString	GetSpindleString(int);
	static	CString	GetSpindleString_Clip(int);
	// 送り速度
	static	CString	GetFeedString(double);
	static	CString	GetFeedString_Integer(double);
	// 行番号付加
	static	CString	GetLineNoString(void);
	static	CString	GetLineNoString_Clip(void);
	// Gｺｰﾄﾞﾓｰﾀﾞﾙ
	static	CString	GetGString(int);
	static	CString	GetGString_Clip(int);
	// 座標値設定
	static	CString	GetValString_Normal(double);
	static	CString	GetValString_UZeroCut(double);
	static	CString	GetValString_Multi1000(double);
	// 円・円弧の生成補助
	static	CString	MakeCircleSub_R(int, const CPointD&, const CPointD&, double);
	static	CString	MakeCircleSub_IJ(int, const CPointD&, const CPointD&, double);
	static	CString	MakeCircleSub_Helical(int, const CPoint3D&);
	// 円ﾃﾞｰﾀの生成
	static	CString	MakeCircle_R(const CDXFcircle*, double);
	static	CString	MakeCircle_IJ(const CDXFcircle*, double);
	static	CString	MakeCircle_IJHALF(const CDXFcircle*, double);
	static	CString	MakeCircle_R_Helical(const CDXFcircle*, double, double);
	static	CString	MakeCircle_IJ_Helical(const CDXFcircle*, double, double);
	static	CString	MakeCircle_IJHALF_Helical(const CDXFcircle*, double, double);
	// 円弧ﾃﾞｰﾀの生成
	static	CString	MakeArc_R(const CDXFarc*, double);
	static	CString	MakeArc_IJ(const CDXFarc*, double);
	// 生成ｵﾌﾟｼｮﾝによる静的変数の初期化
	static	void	SetStaticOption(const CNCMakeOption*);

	// ---

	// 楕円ﾃﾞｰﾀの生成(直線補間)
	void	MakeEllipse(const CDXFellipse *, double);
	CString	MakeEllipse_Tolerance(const CDXFellipse*, double);
	// Polyline の生成
	void	MakePolylineCut(const CDXFpolyline*, double);

public:
	// TH_MakeXXX.cpp で初期化
	static	double	ms_xyz[NCXYZ];	// 前回の位置
	static	void	InitialVariable(void);
	// TH_MakeXXX.cpp でも使用
	static	CString	MakeCustomString(int, DWORD = 0, double* = NULL, BOOL = TRUE);
	static	CString	MakeCustomString(int, int[], double[]);

	// Gｺｰﾄﾞ出力
	void	WriteGcode(CStdioFile&);
};
