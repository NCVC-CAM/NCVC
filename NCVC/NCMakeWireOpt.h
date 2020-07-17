// NCMakeWireOpt.h: ﾜｲﾔ放電加工機用NC生成ｵﾌﾟｼｮﾝの管理
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCMakeOption.h"

enum {		// -- 基本MillOption共通
	MKWI_NUM_PROG = 0,
	MKWI_NUM_LINEADD,
	MKWI_NUM_G90,
	MKWI_NUM_DOT,
	MKWI_NUM_FDOT,
	MKWI_NUM_CIRCLECODE,
		MKWI_NUM_NUMS		// [6]
};
enum {
	MKWI_DBL_DEPTH = 0,
	MKWI_DBL_TAPER,
	MKWI_DBL_FEED,
	MKWI_DBL_G92X,
	MKWI_DBL_G92Y,
	MKWI_DBL_AWFCIRCLE_LO,
	MKWI_DBL_AWFCIRCLE_HI,
	MKWI_DBL_ELLIPSE,
		MKWI_DBL_NUMS		// [8]
};
enum {		// -- 基本MillOption共通
	MKWI_FLG_PROG = 0,
	MKWI_FLG_PROGAUTO,
	MKWI_FLG_LINEADD,
	MKWI_FLG_ZEROCUT,
	MKWI_FLG_GCLIP,
	MKWI_FLG_ELLIPSE,
	// --
	MKWI_FLG_AWFSTART,
	MKWI_FLG_AWFEND,
		MKWI_FLG_NUMS		// [8]
};
enum {
	MKWI_STR_LINEFORM = 0,
	MKWI_STR_EOB,
	MKWI_STR_HEADER,
	MKWI_STR_FOOTER,
	MKWI_STR_TAPERMODE,
	MKWI_STR_AWFCNT,
	MKWI_STR_AWFCUT,
		MKWI_STR_NUMS		// [7]
};

class CNCMakeWireOpt : public CNCMakeOption
{
// 切削ﾊﾟﾗﾒｰﾀ設定のﾀﾞｲｱﾛｸﾞはお友達
friend class CMKWISetup1;
friend class CMKWISetup2;
friend class CMKNCSetup2;
friend class CMKNCSetup6;

	// int型ｵﾌﾟｼｮﾝ
	union {
		struct {
			int		m_nProg,			// ﾌﾟﾛｸﾞﾗﾑ番号
					m_nLineAdd,			// 行番号増加
					m_nG90,				// 位置指定(G90 or G91)
					m_nDot,				// 数値表記(小数点 or 1/1000)
					m_nFDot,			// Ｆﾊﾟﾗﾒｰﾀの数値表記
					m_nCircleCode;		// 円切削(G2 or G3)
		};
		int			m_unNums[MKWI_NUM_NUMS];
	};
	// double型ｵﾌﾟｼｮﾝ
	union {
		struct {
			double	m_dDepth,			// ﾜｰｸ厚み
					m_dTaper,			// ﾃｰﾊﾟ角度[deg]
					m_dFeed,			// 切削送り
					m_dG92X,			// G92
					m_dG92Y,
					m_dAWFcircleLo,		// AWF結線対象円
					m_dAWFcircleHi,		// AWF結線対象円
			// -----
					m_dEllipse;			// 楕円公差
		};
		double		m_udNums[MKWI_DBL_NUMS];
	};
	// BOOL型ｵﾌﾟｼｮﾝ
	union {
		struct {
			BOOL	m_bProg,			// O番号付与
					m_bProgAuto,		// ﾗﾝﾀﾞﾑ割り当て
					m_bLineAdd,			// 行番号
					m_bZeroCut,			// 小数点以下のｾﾞﾛｶｯﾄ
					m_bGclip,			// Gｺｰﾄﾞ省略形
			// -----
					m_bEllipse,			// 長径と短径が等しい楕円は円とみなす
			//
					m_bAWFstart,		// 加工前結線
					m_bAWFend;			// 加工後切断
		};
		BOOL		m_ubFlags[MKWI_FLG_NUMS];
	};
	// CString型ｵﾌﾟｼｮﾝ -> 実体はﾍﾞｰｽｸﾗｽへ
		// 行番号ﾌｫｰﾏｯﾄ, EOB, ｶｽﾀﾑﾍｯﾀﾞｰ，ｶｽﾀﾑﾌｯﾀｰ,
		// TaperMode, AWF結線ｺｰﾄﾞ, AWF切断ｺｰﾄﾞ

public:
	CNCMakeWireOpt(LPCTSTR);

	BOOL	IsAWFcircle(double r) {
		return m_dAWFcircleLo<=r && r<=m_dAWFcircleHi;
	}

#ifdef _DEBUGOLD
	virtual	void	DbgDump(void) const;	// ｵﾌﾟｼｮﾝ変数のﾀﾞﾝﾌﾟ
#endif
};
