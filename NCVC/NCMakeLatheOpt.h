// NCMakeLatheOpt.h: 旋盤用NC生成ｵﾌﾟｼｮﾝの管理
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCMakeOption.h"

enum {
	MKLA_NUM_SPINDLE = 0,
	MKLA_NUM_MARGIN,
	MKLA_NUM_PROG,
	MKLA_NUM_LINEADD,
	MKLA_NUM_G90,
	MKLA_NUM_DOT,
	MKLA_NUM_FDOT,
	MKLA_NUM_CIRCLECODE,
	MKLA_NUM_IJ,
		MKLA_NUM_NUMS		// [9]
};
enum {
	MKLA_DBL_FEED = 0,
	MKLA_DBL_XFEED,
	MKLA_DBL_CUT,
	MKLA_DBL_PULL_Z,
	MKLA_DBL_PULL_X,
	MKLA_DBL_MARGIN,
	MKLA_DBL_ELLIPSE,
		MKLA_DBL_NUMS		// [7]
};
enum {
	MKLA_FLG_PROG = 0,
	MKLA_FLG_PROGAUTO,
	MKLA_FLG_LINEADD,
	MKLA_FLG_ZEROCUT,
	MKLA_FLG_GCLIP,
	MKLA_FLG_DISABLESPINDLE,
	MKLA_FLG_CIRCLEHALF,
	MKLA_FLG_ZEROCUT_IJ,
	MKLA_FLG_ELLIPSE,
		MKLA_FLG_NUMS		// [9]
};
enum {
	MKLA_STR_LINEFORM = 0,
	MKLA_STR_EOB,
	MKLA_STR_HEADER,
	MKLA_STR_FOOTER,
		MKLA_STR_NUMS		// [4]
};

class CNCMakeLatheOpt : public CNCMakeOption
{
// 切削ﾊﾟﾗﾒｰﾀ設定のﾀﾞｲｱﾛｸﾞはお友達
friend class CMKLASetup1;
friend class CMKNCSetup2;
friend class CMKNCSetup6;

	// int型ｵﾌﾟｼｮﾝ
	union {
		struct {
			int		m_nSpindle,			// 主軸回転速度
					m_nMargin,			// 仕上げ回数
			// -----
					m_nProg,			// ﾌﾟﾛｸﾞﾗﾑ番号
					m_nLineAdd,			// 行番号増加
					m_nG90,				// 位置指定(G90 or G91)
					m_nDot,				// 数値表記(小数点 or 1/1000)
					m_nFDot,			// Ｆﾊﾟﾗﾒｰﾀの数値表記
					m_nCircleCode,		// 円切削(G2 or G3)
					m_nIJ;				// 円弧補間にRかI/J/K
		};
		int			m_unNums[MKLA_NUM_NUMS];
	};
	// double型ｵﾌﾟｼｮﾝ
	union {
		struct {
			double	m_dFeed,			// 切削送り(Z)
					m_dXFeed,			// 切削送り(X)
					m_dCut,				// 切り込み(半径値)
					m_dPullZ,			// 引き代Z
					m_dPullX,			// 引き代X(半径値)
					m_dMargin,			// 仕上げ代(半径値)
			// -----
					m_dEllipse;			// 楕円公差
		};
		double		m_udNums[MKLA_DBL_NUMS];
	};
	// BOOL型ｵﾌﾟｼｮﾝ
	union {
		struct {
			BOOL	m_bProg,			// O番号付与
					m_bProgAuto,		// ﾗﾝﾀﾞﾑ割り当て
					m_bLineAdd,			// 行番号
					m_bZeroCut,			// 小数点以下のｾﾞﾛｶｯﾄ
					m_bGclip,			// Gｺｰﾄﾞ省略形
					m_bDisableSpindle,	// Sﾊﾟﾗﾒｰﾀを生成しない
					m_bCircleHalf,		// 全円は分割
					m_bZeroCutIJ,		// [I|J]0は省略
			// -----
					m_bEllipse;			// 長径と短径が等しい楕円は円とみなす
		};
		BOOL		m_ubFlags[MKLA_FLG_NUMS];
	};
	// CString型ｵﾌﾟｼｮﾝ -> 実体はﾍﾞｰｽｸﾗｽへ
		// 行番号ﾌｫｰﾏｯﾄ, EOB, ｶｽﾀﾑﾍｯﾀﾞｰ，ｶｽﾀﾑﾌｯﾀｰ

public:
	CNCMakeLatheOpt(LPCTSTR);

#ifdef _DEBUGOLD
	virtual	void	DbgDump(void) const;	// ｵﾌﾟｼｮﾝ変数のﾀﾞﾝﾌﾟ
#endif
};
