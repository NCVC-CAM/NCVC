// NCMakeOption.h: NC生成ｵﾌﾟｼｮﾝの管理
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCVCdefine.h"

#define	MKNC_NUM_SPINDLE			0
#define	MKNC_NUM_LINEADD			1
#define	MKNC_NUM_G90				2
#define	MKNC_NUM_ZRETURN			3
#define	MKNC_NUM_DOT				4
#define	MKNC_NUM_FDOT				5
#define	MKNC_NUM_CIRCLECODE			6
#define	MKNC_NUM_IJ					7
#define	MKNC_NUM_MAKEEND			8
#define	MKNC_NUM_DEEPSPINDLE		9
#define	MKNC_NUM_DEEPZPROCESS		10
#define	MKNC_NUM_DEEPAPROCESS		11
#define	MKNC_NUM_DEEPCPROCESS		12
#define	MKNC_NUM_DRILLSPINDLE		13
#define	MKNC_NUM_DWELL				14
#define	MKNC_NUM_DWELLFORMAT		15
#define	MKNC_NUM_DRILLZPROCESS		16
#define	MKNC_NUM_DRILLPROCESS		17
#define	MKNC_NUM_DRILLSORT			18
#define	MKNC_NUM_DRILLCIRCLEPROCESS	19
#define	MKNC_NUM_MOVEZ				20
#define	MKNC_NUM_TOLERANCE			21
#define	MKNC_NUM_OPTIMAIZEDRILL		22

#define	MKNC_DBL_FEED			0
#define	MKNC_DBL_ZFEED			1
#define	MKNC_DBL_ZG0STOP		2
#define	MKNC_DBL_ZCUT			3
#define	MKNC_DBL_G92X			4
#define	MKNC_DBL_G92Y			5
#define	MKNC_DBL_G92Z			6
#define	MKNC_DBL_ELLIPSE		7
#define	MKNC_DBL_MAKEEND		8
#define	MKNC_DBL_MAKEENDFEED	9
#define	MKNC_DBL_DEEP			10
#define	MKNC_DBL_ZSTEP			11
#define	MKNC_DBL_DEEPFEED		12
#define	MKNC_DBL_DRILLFEED		13
#define	MKNC_DBL_DRILLR			14
#define	MKNC_DBL_DRILLZ			15
#define	MKNC_DBL_DRILLCIRCLE	16
#define	MKNC_DBL_TOLERANCE		17
#define	MKNC_DBL_DRILLMARGIN	18

#define	MKNC_FLG_XREV			0
#define	MKNC_FLG_YREV			1
#define	MKNC_FLG_LINEADD		2
#define	MKNC_FLG_ZEROCUT		3
#define	MKNC_FLG_GCLIP			4
#define	MKNC_FLG_DISABLESPINDLE	5
#define	MKNC_FLG_CIRCLEHALF		6
#define	MKNC_FLG_ELLIPSE		7
#define	MKNC_FLG_DEEP			8
#define	MKNC_FLG_DEEPFINISH		9
#define	MKNC_FLG_DRILLMATCH		10
#define	MKNC_FLG_DRILLCIRCLE	11
#define	MKNC_FLG_DRILLBREAK		12
#define	MKNC_FLG_LAYERCOMMENT	13

#define	MKNC_STR_LINEFORM		0
#define	MKNC_STR_EOB			1
#define	MKNC_STR_CUSTMOVE_B		2
#define	MKNC_STR_CUSTMOVE_A		3
#define	MKNC_STR_HEADER			4
#define	MKNC_STR_FOOTER			5

class CNCMakeOption
{
	CString	m_strInitFile;		// 条件ﾌｧｲﾙ名
	int		m_nOrderLength;		// 最大命令長

// 切削ﾊﾟﾗﾒｰﾀ設定のﾀﾞｲｱﾛｸﾞはお友達
friend class CMKNCSetup;
friend class CMKNCSetup1;
friend class CMKNCSetup2;
friend class CMKNCSetup3;
friend class CMKNCSetup4;
friend class CMKNCSetup5;
friend class CMKNCSetup6;
friend class CMKNCSetup8;

	// int型ｵﾌﾟｼｮﾝ
	union {
		struct {
			int		m_nSpindle,			// 主軸回転速度
			// -----
					m_nLineAdd,			// 行番号増加
					m_nG90,				// 位置指定(G90 or G91)
					m_nZReturn,			// Z軸の復帰(Initial or R)
					m_nDot,				// 数値表記(小数点 or 1/1000)
					m_nFDot,			// Ｆﾊﾟﾗﾒｰﾀの数値表記
					m_nCircleCode,		// 円切削(G2 or G3)
					m_nIJ,				// 円弧補間にRかI/J/K
			// -----
					m_nMakeEnd,			// 加工済み深さの指示
					m_nDeepSpindle,		// 深彫仕上げ回転数
					m_nDeepZProcess,	// 深彫仕上げからのZ軸復帰
					m_nDeepAProcess,	// 深彫の切削手順
					m_nDeepCProcess,	// 深彫の切削方向
			// -----
					m_nDrillSpindle,	// 穴あけ回転数
					m_nDwell,			// ﾄﾞｳｪﾙ時間
					m_nDwellFormat,		// ﾄﾞｳｪﾙ時間の表記
					m_nDrillZProcess,	// 穴あけのZ軸復帰
					m_nDrillProcess,	// 穴あけの仕方
					m_nDrillSort,		// 穴あけのｸﾞﾙｰﾋﾟﾝｸﾞ順序
					m_nDrillCircleProcess,	// 円ﾃﾞｰﾀを含むときの穴あけ順序
			// -----
					m_nMoveZ,			// 移動ﾚｲﾔのZ軸
			// -----
					m_nTolerance,		// 許容差を超えたときの動作
					m_nOptimaizeDrill;	// 穴加工の基準軸
		};
		int			m_unNums[23];
	};
	// double型ｵﾌﾟｼｮﾝ
	union {
		struct {
			double	m_dFeed,			// 切削送り速度
					m_dZFeed,			// Z軸を下げるときの送り速度
					m_dZG0Stop,			// G0で動かすZ位置(R点)
					m_dZCut,			// Z軸の下げ位置(切り込み)
					m_dG92[NCXYZ],		// G92のX/Y/Z
			// -----
					m_dEllipse,			// 楕円公差
			// -----
					m_dMakeValue,		// 加工済み深さのｵﾌｾｯﾄor固定Z値
					m_dMakeFeed,		// 加工済みZ送り速度
					m_dDeep,			// 深彫の最終切り込み
					m_dZStep,			// 深彫切削のためのｽﾃｯﾌﾟ
					m_dDeepFeed,		// 深彫仕上げ送り
			// -----
					m_dDrillFeed,		// 穴あけ送り
					m_dDrillR,			// 穴加工R点
					m_dDrillZ,			// 穴加工切り込み
					m_dDrillCircle,		// 穴加工に見立てる円ﾃﾞｰﾀの半径
			// -----
					m_dTolerance,		// 同一座標と見なす許容差
					m_dDrillMargin;		// 基準軸に対する許容差
		};
		double		m_udNums[19];
	};
	// BOOL型ｵﾌﾟｼｮﾝ
	union {
		struct {
			BOOL	m_bXrev,			// X軸反転
					m_bYrev,			// Y軸反転
					m_bLineAdd,			// 行番号
					m_bZeroCut,			// 小数点以下のｾﾞﾛｶｯﾄ
					m_bGclip,			// Gｺｰﾄﾞ省略形
					m_bDisableSpindle,	// Sﾊﾟﾗﾒｰﾀを生成しない
					m_bCircleHalf,		// 全円は分割
			// -----
					m_bEllipse,			// 長径と短径が等しい楕円は円とみなす
			// -----
					m_bDeep,			// 深彫切削を行う
					m_bDeepFinish,		// 仕上げｵﾌﾟｼｮﾝ適用か
			// -----
					m_bDrillMatch,		// 穴加工同一座標は無視
					m_bDrillCircle,		// 円ﾃﾞｰﾀも穴加工ﾃﾞｰﾀと見なす
					m_bDrillBreak,		// 大きさごとにｺﾒﾝﾄを埋め込む
			// -----
					m_bLayerComment;	// ﾚｲﾔごとにｺﾒﾝﾄを埋め込む
		};
		BOOL		m_ubFlags[14];
	};
	// CString型ｵﾌﾟｼｮﾝ
	CString		m_strOption[6];			// 行番号ﾌｫｰﾏｯﾄ, EOB, ｶｽﾀﾑ移動ｺｰﾄﾞ(前後)
										// ｶｽﾀﾑﾍｯﾀﾞｰ，ｶｽﾀﾑﾌｯﾀｰ

	BOOL	Convert();				// 旧ﾊﾞｰｼﾞｮﾝ互換用
	void	InitialDefault(void);	// ﾃﾞﾌｫﾙﾄ設定
	CString	GetInsertSpace(int nLen) {
		CString	strResult(' ', m_nOrderLength - nLen);
		return strResult;
	}

public:
	CNCMakeOption(LPCTSTR);

	BOOL	ReadMakeOption(LPCTSTR);	// 条件ﾌｧｲﾙの読み込み
	BOOL	SaveMakeOption(LPCTSTR = NULL);	// 条件ﾌｧｲﾙの書き出し

	CString	GetInitFile(void) const {
		return m_strInitFile;
	}
	int		GetNum(size_t n) const {		// 数字ｵﾌﾟｼｮﾝ
		ASSERT( n>=0 && n<SIZEOF(m_unNums) );
		return m_unNums[n];
	}
	double	GetDbl(size_t n) const {		// 数字ｵﾌﾟｼｮﾝ
		ASSERT( n>=0 && n<SIZEOF(m_udNums) );
		return m_udNums[n];
	}
	BOOL	GetFlag(size_t n) const {		// ﾌﾗｸﾞｵﾌﾟｼｮﾝ
		ASSERT( n>=0 && n<SIZEOF(m_ubFlags) );
		return m_ubFlags[n];
	}
	CString	GetStr(size_t n) const {		// 文字列ｵﾌﾟｼｮﾝ
		ASSERT( n>=0 && n<SIZEOF(m_strOption) );
		return m_strOption[n];
	}

#ifdef _DEBUGOLD
	void	DbgDump(void) const;		// ｵﾌﾟｼｮﾝ変数のﾀﾞﾝﾌﾟ
#endif
};
