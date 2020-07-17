// NCMakeMillOpt.h: NC生成ｵﾌﾟｼｮﾝの管理
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCVCdefine.h"
#include "NCMakeOption.h"

#define	MKNC_NUM_SPINDLE			0
#define	MKNC_NUM_PROG				1
#define	MKNC_NUM_LINEADD			2
#define	MKNC_NUM_G90				3
#define	MKNC_NUM_ZRETURN			4
#define	MKNC_NUM_DOT				5
#define	MKNC_NUM_FDOT				6
#define	MKNC_NUM_CIRCLECODE			7
#define	MKNC_NUM_IJ					8
#define	MKNC_NUM_MAKEEND			9
#define	MKNC_NUM_DEEPSPINDLE		10
#define	MKNC_NUM_DEEPRETURN			11
#define	MKNC_NUM_DEEPALL			12
#define	MKNC_NUM_DEEPROUND			13
#define	MKNC_NUM_DRILLSPINDLE		14
#define	MKNC_NUM_DWELL				15
#define	MKNC_NUM_DWELLFORMAT		16
#define	MKNC_NUM_DRILLRETURN		17
#define	MKNC_NUM_DRILLPROCESS		18
#define	MKNC_NUM_DRILLSORT			19
#define	MKNC_NUM_DRILLCIRCLEPROCESS	20
#define	MKNC_NUM_MOVEZ				21
#define	MKNC_NUM_TOLERANCE			22
#define	MKNC_NUM_OPTIMAIZEDRILL		23

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
#define	MKNC_FLG_PROG			2
#define	MKNC_FLG_PROGAUTO		3
#define	MKNC_FLG_LINEADD		4
#define	MKNC_FLG_ZEROCUT		5
#define	MKNC_FLG_GCLIP			6
#define	MKNC_FLG_DISABLESPINDLE	7
#define	MKNC_FLG_CIRCLEHALF		8
#define	MKNC_FLG_ELLIPSE		9
#define	MKNC_FLG_DEEP			10
#define	MKNC_FLG_HELICAL		11
#define	MKNC_FLG_DEEPFINISH		12
#define	MKNC_FLG_DRILLMATCH		13
#define	MKNC_FLG_DRILLCIRCLE	14
#define	MKNC_FLG_DRILLBREAK		15
#define	MKNC_FLG_LAYERCOMMENT	16
#define	MKNC_FLG_L0CYCLE		17

#define	MKNC_STR_LINEFORM		0
#define	MKNC_STR_EOB			1
#define	MKNC_STR_HEADER			2
#define	MKNC_STR_FOOTER			3
#define	MKNC_STR_CUSTMOVE_B		4
#define	MKNC_STR_CUSTMOVE_A		5

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
			int		m_nSpindle,			// 主軸回転速度
			// -----
					m_nProg,			// ﾌﾟﾛｸﾞﾗﾑ番号
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
		int			m_unNums[24];
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
					m_bProg,			// O番号付与
					m_bProgAuto,		// ﾗﾝﾀﾞﾑ割り当て
					m_bLineAdd,			// 行番号
					m_bZeroCut,			// 小数点以下のｾﾞﾛｶｯﾄ
					m_bGclip,			// Gｺｰﾄﾞ省略形
					m_bDisableSpindle,	// Sﾊﾟﾗﾒｰﾀを生成しない
					m_bCircleHalf,		// 全円は分割
			// -----
					m_bEllipse,			// 長径と短径が等しい楕円は円とみなす
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
		BOOL		m_ubFlags[18];
	};
	// CString型ｵﾌﾟｼｮﾝ -> 実体はﾍﾞｰｽｸﾗｽへ
		// 行番号ﾌｫｰﾏｯﾄ, EOB, ｶｽﾀﾑﾍｯﾀﾞｰ，ｶｽﾀﾑﾌｯﾀｰ
		// ｶｽﾀﾑ移動ｺｰﾄﾞ(前後)

	BOOL	Convert();						// 旧ﾊﾞｰｼﾞｮﾝ互換用

public:
	CNCMakeMillOpt(LPCTSTR);

	BOOL	ReadMakeOption(LPCTSTR);
	BOOL	SaveMakeOption(LPCTSTR = NULL);	// 条件ﾌｧｲﾙの書き出し

#ifdef _DEBUGOLD
	void	DbgDump(void) const;		// ｵﾌﾟｼｮﾝ変数のﾀﾞﾝﾌﾟ
#endif
};
