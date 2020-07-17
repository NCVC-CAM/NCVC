// NCMakeLatheOpt.h: 旋盤用NC生成ｵﾌﾟｼｮﾝの管理
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCMakeOption.h"

enum {
	MKLA_NUM_SPINDLE = 0,		// 主軸回転速度
	MKLA_NUM_MARGIN,			// 仕上げ回数
	MKLA_NUM_PROG,				// ﾌﾟﾛｸﾞﾗﾑ番号
	MKLA_NUM_LINEADD,			// 行番号増加
	MKLA_NUM_G90,				// 位置指定(G90 or G91)
	MKLA_NUM_DOT,				// 数値表記(小数点 or 1/1000)
	MKLA_NUM_FDOT,				// Ｆﾊﾟﾗﾒｰﾀの数値表記
	MKLA_NUM_CIRCLECODE,		// 円切削(G2 or G3)
	MKLA_NUM_IJ,				// 円弧補間にRかI/J/K
		MKLA_NUM_NUMS		// [9]
};
enum {
	MKLA_DBL_FEED = 0,			// 切削送り(Z)
	MKLA_DBL_XFEED,				// 切削送り(X)
	MKLA_DBL_CUT,				// 切り込み(半径値)
	MKLA_DBL_PULL_Z,			// 引き代Z
	MKLA_DBL_PULL_X,			// 引き代X(半径値)
	MKLA_DBL_MARGIN,			// 仕上げ代(半径値)
	MKLA_DBL_ELLIPSE,			// 楕円公差
		MKLA_DBL_NUMS		// [7]
};
enum {
	MKLA_FLG_PROG = 0,			// O番号付与
	MKLA_FLG_PROGAUTO,			// ﾗﾝﾀﾞﾑ割り当て
	MKLA_FLG_LINEADD,			// 行番号
	MKLA_FLG_ZEROCUT,			// 小数点以下のｾﾞﾛｶｯﾄ
	MKLA_FLG_GCLIP,				// Gｺｰﾄﾞ省略形
	MKLA_FLG_DISABLESPINDLE,	// Sﾊﾟﾗﾒｰﾀを生成しない
	MKLA_FLG_CIRCLEHALF,		// 全円は分割
	MKLA_FLG_ZEROCUT_IJ,		// [I|J]0は省略
	MKLA_FLG_ELLIPSE,			// 長径と短径が等しい楕円は円とみなす
		MKLA_FLG_NUMS		// [9]
};
enum {
	MKLA_STR_LINEFORM = 0,		// 行番号ﾌｫｰﾏｯﾄ
	MKLA_STR_EOB,				// EOB
	MKLA_STR_HEADER,			// ｶｽﾀﾑﾍｯﾀﾞｰ
	MKLA_STR_FOOTER,			// ｶｽﾀﾑﾌｯﾀｰ
		MKLA_STR_NUMS		// [4]
};
//
#define	LTH_I_SPINDLE			m_pIntOpt[MKLA_NUM_SPINDLE]
#define	LTH_I_MARGIN			m_pIntOpt[MKLA_NUM_MARGIN]
#define	LTH_I_PROG				m_pIntOpt[MKLA_NUM_PROG]
#define	LTH_I_LINEADD			m_pIntOpt[MKLA_NUM_LINEADD]
#define	LTH_I_G90				m_pIntOpt[MKLA_NUM_G90]
#define	LTH_I_DOT				m_pIntOpt[MKLA_NUM_DOT]
#define	LTH_I_FDOT				m_pIntOpt[MKLA_NUM_FDOT]
#define	LTH_I_CIRCLECODE		m_pIntOpt[MKLA_NUM_CIRCLECODE]
#define	LTH_I_IJ				m_pIntOpt[MKLA_NUM_IJ]
//
#define	LTH_D_FEED				m_pDblOpt[MKLA_DBL_FEED]
#define	LTH_D_XFEED				m_pDblOpt[MKLA_DBL_XFEED]
#define	LTH_D_CUT				m_pDblOpt[MKLA_DBL_CUT]
#define	LTH_D_PULL_Z			m_pDblOpt[MKLA_DBL_PULL_Z]
#define	LTH_D_PULL_X			m_pDblOpt[MKLA_DBL_PULL_X]
#define	LTH_D_MARGIN			m_pDblOpt[MKLA_DBL_MARGIN]
#define	LTH_D_ELLIPSE			m_pDblOpt[MKLA_DBL_ELLIPSE]
//
#define	LTH_F_PROG				m_pFlgOpt[MKLA_FLG_PROG]
#define	LTH_F_PROGAUTO			m_pFlgOpt[MKLA_FLG_PROGAUTO]
#define	LTH_F_LINEADD			m_pFlgOpt[MKLA_FLG_LINEADD]
#define	LTH_F_ZEROCUT			m_pFlgOpt[MKLA_FLG_ZEROCUT]
#define	LTH_F_GCLIP				m_pFlgOpt[MKLA_FLG_GCLIP]
#define	LTH_F_DISABLESPINDLE	m_pFlgOpt[MKLA_FLG_DISABLESPINDLE]
#define	LTH_F_CIRCLEHALF		m_pFlgOpt[MKLA_FLG_CIRCLEHALF]
#define	LTH_F_ZEROCUT_IJ		m_pFlgOpt[MKLA_FLG_ZEROCUT_IJ]
#define	LTH_F_ELLIPSE			m_pFlgOpt[MKLA_FLG_ELLIPSE]
//
class CNCMakeLatheOpt : public CNCMakeOption
{
	friend class CMKLASetup1;
	friend class CMKNCSetup2;
	friend class CMKNCSetup6;

	// 親ｸﾗｽにﾃﾞｰﾀを持たせるので、
	// union/struct技は使えない
	// むしろC++らしくｺｰﾃﾞｨﾝｸﾞ

protected:
	virtual	void	InitialDefault(void);

public:
	CNCMakeLatheOpt(LPCTSTR);

#ifdef _DEBUG
	virtual	void	DbgDump(void) const;	// ｵﾌﾟｼｮﾝ変数のﾀﾞﾝﾌﾟ
#endif
};
