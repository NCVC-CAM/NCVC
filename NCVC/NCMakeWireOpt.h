// NCMakeWireOpt.h: ﾜｲﾔ放電加工機用NC生成ｵﾌﾟｼｮﾝの管理
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCMakeOption.h"

// -- 基本MillOption共通
#define	MKWI_NUM_PROG			0
#define	MKWI_NUM_LINEADD		1
#define	MKWI_NUM_G90			2
#define	MKWI_NUM_DOT			3
#define	MKWI_NUM_FDOT			4
#define	MKWI_NUM_CIRCLECODE		5
// --

#define	MKWI_DBL_DEPTH			0
#define	MKWI_DBL_TAPER			1
#define	MKWI_DBL_FEED			2
#define	MKWI_DBL_G92X			3
#define	MKWI_DBL_G92Y			4
#define	MKWI_DBL_AWFCIRCLE_LO	5
#define	MKWI_DBL_AWFCIRCLE_HI	6
#define	MKWI_DBL_ELLIPSE		7

// -- 基本MillOption共通
#define	MKWI_FLG_PROG			0
#define	MKWI_FLG_PROGAUTO		1
#define	MKWI_FLG_LINEADD		2
#define	MKWI_FLG_ZEROCUT		3
#define	MKWI_FLG_GCLIP			4
#define	MKWI_FLG_ELLIPSE		5
// --
#define	MKWI_FLG_AWFSTART		6
#define	MKWI_FLG_AWFEND			7

#define	MKWI_STR_LINEFORM		0
#define	MKWI_STR_EOB			1
#define	MKWI_STR_HEADER			2
#define	MKWI_STR_FOOTER			3
#define	MKWI_STR_TAPERMODE		4
#define	MKWI_STR_AWFCNT			5
#define	MKWI_STR_AWFCUT			6

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
		int			m_unNums[6];
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
		double		m_udNums[8];
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
		BOOL		m_ubFlags[8];
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
