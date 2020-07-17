// NCMakeMillOpt.h: NC生成ｵﾌﾟｼｮﾝの管理
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCVCdefine.h"
#include "NCMakeOption.h"

enum {
	MKNC_NUM_PROG = 0,
	MKNC_NUM_LINEADD,
	MKNC_NUM_G90,
	MKNC_NUM_DOT,
	MKNC_NUM_FDOT,
	MKNC_NUM_CIRCLECODE,
	// -- ここまで派生WireOption共通
	MKNC_NUM_SPINDLE,
	MKNC_NUM_ZRETURN,
	MKNC_NUM_IJ,
	MKNC_NUM_MAKEEND,
	MKNC_NUM_DEEPSPINDLE,
	MKNC_NUM_DEEPRETURN,
	MKNC_NUM_DEEPALL,
	MKNC_NUM_DEEPROUND,
	MKNC_NUM_DRILLSPINDLE,
	MKNC_NUM_DWELL,
	MKNC_NUM_DWELLFORMAT,
	MKNC_NUM_DRILLRETURN,
	MKNC_NUM_DRILLPROCESS,
	MKNC_NUM_DRILLSORT,
	MKNC_NUM_DRILLCIRCLEPROCESS,
	MKNC_NUM_MOVEZ,
	MKNC_NUM_TOLERANCE,
	MKNC_NUM_OPTIMAIZEDRILL,
		MKNC_NUM_NUMS		// [24]
};
enum {
	MKNC_DBL_FEED = 0,
	MKNC_DBL_ZFEED,
	MKNC_DBL_ZG0STOP,
	MKNC_DBL_ZCUT,
	MKNC_DBL_G92X,
	MKNC_DBL_G92Y,
	MKNC_DBL_G92Z,
	MKNC_DBL_ELLIPSE,
	MKNC_DBL_MAKEEND,
	MKNC_DBL_MAKEENDFEED,
	MKNC_DBL_DEEP,
	MKNC_DBL_ZSTEP,
	MKNC_DBL_DEEPFEED,
	MKNC_DBL_DRILLFEED,
	MKNC_DBL_DRILLR,
	MKNC_DBL_DRILLZ,
	MKNC_DBL_DRILLCIRCLE,
	MKNC_DBL_TOLERANCE,
	MKNC_DBL_DRILLMARGIN,
		MKNC_DBL_NUMS		// [19]
};
enum {
	MKNC_FLG_PROG = 0,
	MKNC_FLG_PROGAUTO,
	MKNC_FLG_LINEADD,
	MKNC_FLG_ZEROCUT,
	MKNC_FLG_GCLIP,
	MKNC_FLG_ELLIPSE,
	// -- ここまで派生WireOption共通
	MKNC_FLG_XREV,
	MKNC_FLG_YREV,
	MKNC_FLG_DISABLESPINDLE,
	MKNC_FLG_CIRCLEHALF,
	MKNC_FLG_ZEROCUT_IJ,
	MKNC_FLG_DEEP,
	MKNC_FLG_HELICAL,
	MKNC_FLG_DEEPFINISH,
	MKNC_FLG_DRILLMATCH,
	MKNC_FLG_DRILLCIRCLE,
	MKNC_FLG_DRILLBREAK,
	MKNC_FLG_LAYERCOMMENT,
	MKNC_FLG_L0CYCLE,
		MKNC_FLG_NUMS		// [19]
};
enum {
	MKNC_STR_LINEFORM = 0,
	MKNC_STR_EOB,
	MKNC_STR_HEADER,
	MKNC_STR_FOOTER,
	MKNC_STR_CUSTMOVE_B,
	MKNC_STR_CUSTMOVE_A,
		MKNC_STR_NUMS		// [6]
};

class CNCMakeMillOpt : public CNCMakeOption
{
// 切削ﾊﾟﾗﾒｰﾀ設定のﾀﾞｲｱﾛｸﾞはお友達
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
			// -----
			int		m_nProg,			// ﾌﾟﾛｸﾞﾗﾑ番号
					m_nLineAdd,			// 行番号増加
					m_nG90,				// 位置指定(G90 or G91)
					m_nDot,				// 数値表記(小数点 or 1/1000)
					m_nFDot,			// Ｆﾊﾟﾗﾒｰﾀの数値表記
					m_nCircleCode,		// 円切削(G2 or G3)
			//
					m_nSpindle,			// 主軸回転速度
					m_nZReturn,			// Z軸の復帰(Initial or R)
					m_nIJ,				// 円弧補間にRかI/J/K
			// -----
					m_nMakeEnd,			// 加工済み深さの指示
					m_nDeepSpindle,		// 深彫仕上げ回転数
					m_nDeepReturn,		// 深彫仕上げからのZ軸復帰
					m_nDeepAll,			// 深彫の切削手順
					m_nDeepRound,		// 深彫の切削方向
			// -----
					m_nDrillSpindle,	// 穴あけ回転数
					m_nDwell,			// ﾄﾞｳｪﾙ時間
					m_nDwellFormat,		// ﾄﾞｳｪﾙ時間の表記
					m_nDrillReturn,		// 穴あけのZ軸復帰
					m_nDrillProcess,	// 穴あけの仕方
					m_nDrillSort,		// 穴あけのｸﾞﾙｰﾋﾟﾝｸﾞ順序
					m_nDrillCircleProcess,	// 円ﾃﾞｰﾀを含むときの穴あけ順序
			// -----
					m_nMoveZ,			// 移動ﾚｲﾔのZ軸
			// -----
					m_nTolerance,		// 許容差を超えたときの動作
					m_nOptimaizeDrill;	// 穴加工の基準軸
		};
		int			m_unNums[MKNC_NUM_NUMS];
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
		double		m_udNums[MKNC_DBL_NUMS];
	};
	// BOOL型ｵﾌﾟｼｮﾝ
	union {
		struct {
			BOOL	m_bProg,			// O番号付与
					m_bProgAuto,		// ﾗﾝﾀﾞﾑ割り当て
					m_bLineAdd,			// 行番号
					m_bZeroCut,			// 小数点以下のｾﾞﾛｶｯﾄ
					m_bGclip,			// Gｺｰﾄﾞ省略形
					m_bEllipse,			// 長径と短径が等しい楕円は円とみなす
			//
					m_bXrev,			// X軸反転
					m_bYrev,			// Y軸反転
					m_bDisableSpindle,	// Sﾊﾟﾗﾒｰﾀを生成しない
					m_bCircleHalf,		// 全円は分割
					m_bZeroCutIJ,		// [I|J]0は省略
			// -----
					m_bDeep,			// 深彫切削を行う
					m_bHelical,			// 円ﾃﾞｰﾀをﾍﾘｶﾙ切削
					m_bDeepFinish,		// 仕上げｵﾌﾟｼｮﾝ適用か
			// -----
					m_bDrillMatch,		// 穴加工同一座標は無視
					m_bDrillCircle,		// 円ﾃﾞｰﾀも穴加工ﾃﾞｰﾀと見なす
					m_bDrillBreak,		// 大きさごとにｺﾒﾝﾄを埋め込む
			// -----
					m_bLayerComment,	// ﾚｲﾔごとにｺﾒﾝﾄを埋め込む
					m_bL0Cycle;			// 固定ｻｲｸﾙ中はL0出力
		};
		BOOL		m_ubFlags[MKNC_FLG_NUMS];
	};
	// CString型ｵﾌﾟｼｮﾝ -> 実体はﾍﾞｰｽｸﾗｽへ
		// 行番号ﾌｫｰﾏｯﾄ, EOB, ｶｽﾀﾑﾍｯﾀﾞｰ，ｶｽﾀﾑﾌｯﾀｰ
		// ｶｽﾀﾑ移動ｺｰﾄﾞ(前後)

	BOOL	Convert();						// 旧ﾊﾞｰｼﾞｮﾝ互換用

public:
	CNCMakeMillOpt(LPCTSTR);

#ifdef _DEBUGOLD
	virtual	void	DbgDump(void) const;	// ｵﾌﾟｼｮﾝ変数のﾀﾞﾝﾌﾟ
#endif
};
