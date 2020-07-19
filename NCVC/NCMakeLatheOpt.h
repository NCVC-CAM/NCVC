// NCMakeLatheOpt.h: 旋盤用NC生成ｵﾌﾟｼｮﾝの管理
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCMakeOption.h"

enum {
	MKLA_NUM_PROG = 0,			// ﾌﾟﾛｸﾞﾗﾑ番号
	MKLA_NUM_LINEADD,			// 行番号増加
	MKLA_NUM_G90,				// 位置指定(G90 or G91)
	MKLA_NUM_DOT,				// 数値表記(小数点 or 1/1000)
	MKLA_NUM_FDOT,				// Ｆﾊﾟﾗﾒｰﾀの数値表記
	MKLA_NUM_CIRCLECODE,		// 円切削(G2 or G3)
	MKLA_NUM_IJ,				// 円弧補間にRかI/J/K
	MKLA_NUM_E_SPINDLE,			// 端面主軸回転数
	MKLA_NUM_I_SPINDLE,			// 内径主軸回転数
	MKLA_NUM_O_SPINDLE,			// 外径主軸回転数
	MKLA_NUM_I_MARGIN,			// 内径仕上げ回数
	MKLA_NUM_O_MARGIN,			// 外径仕上げ回数
		MKLA_NUM_NUMS		// [12]
};
enum {
	MKLA_DBL_O_FEED = 0,		// 外径切削送り(Z)
	MKLA_DBL_O_FEEDX,			// 外径切削送り(X)
	MKLA_DBL_O_CUT,				// 外径切り込み(半径値)
	MKLA_DBL_O_PULLZ,			// 外径引き代Z
	MKLA_DBL_O_PULLX,			// 外径引き代X(半径値)
	MKLA_DBL_O_MARGIN,			// 外径仕上げ代(半径値)
	MKLA_DBL_ELLIPSE,			// 楕円公差
	MKLA_DBL_E_FEED,			// 端面切削送り
	MKLA_DBL_E_CUT,				// 端面切り込み
	MKLA_DBL_E_STEP,			// 端面切り込みｽﾃｯﾌﾟ
	MKLA_DBL_E_PULLZ,			// 端面引き代Z
	MKLA_DBL_E_PULLX,			// 端面引き代X
	MKLA_DBL_DRILLZ,			// 下穴切り込み
	MKLA_DBL_DRILLR,			// R点
	MKLA_DBL_DRILLQ,			// Q値
	MKLA_DBL_DRILLD,			// 戻り値
	MKLA_DBL_DWELL,				// ﾄﾞｳｪﾙ時間(Makeの関係でfloat)
	MKLA_DBL_HOLE,				// 既存下穴ｻｲｽﾞ
	MKLA_DBL_I_FEED,			// 内径切削送り(Z)
	MKLA_DBL_I_FEEDX,			// 内径切削送り(X)
	MKLA_DBL_I_CUT,				// 内径切り込み(半径値)
	MKLA_DBL_I_PULLZ,			// 内径引き代Z
	MKLA_DBL_I_PULLX,			// 内径引き代X(半径値)
	MKLA_DBL_I_MARGIN,			// 内径仕上げ代(半径値)
		MKLA_DBL_NUMS		// [24]
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
	MKLA_FLG_ENDFACE,			// 端面処理を行う
	MKLA_FLG_CYCLE,				// 下穴を固定サイクルで
		MKLA_FLG_NUMS		// [11]
};
enum {
	MKLA_STR_LINEFORM = 0,		// 行番号ﾌｫｰﾏｯﾄ
	MKLA_STR_EOB,				// EOB
	MKLA_STR_HEADER,			// ｶｽﾀﾑﾍｯﾀﾞｰ
	MKLA_STR_FOOTER,			// ｶｽﾀﾑﾌｯﾀｰ
	MKLA_STR_DRILL,				// 下穴ﾄﾞﾘﾙ
	MKLA_STR_DRILLSPINDLE,		// 下穴主軸回転速度
	MKLA_STR_DRILLFEED,			// 下穴送り速度
	MKLA_STR_D_CUSTOM,			// 下穴ｶｽﾀﾑｺｰﾄﾞ
	MKLA_STR_E_CUSTOM,			// 端面ｶｽﾀﾑｺｰﾄﾞ
	MKLA_STR_I_CUSTOM,			// 内径ｶｽﾀﾑｺｰﾄﾞ
	MKLA_STR_O_CUSTOM,			// 外径ｶｽﾀﾑｺｰﾄﾞ
		MKLA_STR_NUMS		// [11]
};
//
#define	LTH_I_PROG				m_pIntOpt[MKLA_NUM_PROG]
#define	LTH_I_LINEADD			m_pIntOpt[MKLA_NUM_LINEADD]
#define	LTH_I_G90				m_pIntOpt[MKLA_NUM_G90]
#define	LTH_I_DOT				m_pIntOpt[MKLA_NUM_DOT]
#define	LTH_I_FDOT				m_pIntOpt[MKLA_NUM_FDOT]
#define	LTH_I_CIRCLECODE		m_pIntOpt[MKLA_NUM_CIRCLECODE]
#define	LTH_I_IJ				m_pIntOpt[MKLA_NUM_IJ]
#define	LTH_I_E_SPINDLE			m_pIntOpt[MKLA_NUM_E_SPINDLE]
#define	LTH_I_I_SPINDLE			m_pIntOpt[MKLA_NUM_I_SPINDLE]
#define	LTH_I_O_SPINDLE			m_pIntOpt[MKLA_NUM_O_SPINDLE]
#define	LTH_I_I_MARGIN			m_pIntOpt[MKLA_NUM_I_MARGIN]
#define	LTH_I_O_MARGIN			m_pIntOpt[MKLA_NUM_O_MARGIN]
//
#define	LTH_D_O_FEED			m_pDblOpt[MKLA_DBL_O_FEED]
#define	LTH_D_O_FEEDX			m_pDblOpt[MKLA_DBL_O_FEEDX]
#define	LTH_D_O_CUT				m_pDblOpt[MKLA_DBL_O_CUT]
#define	LTH_D_O_PULLZ			m_pDblOpt[MKLA_DBL_O_PULLZ]
#define	LTH_D_O_PULLX			m_pDblOpt[MKLA_DBL_O_PULLX]
#define	LTH_D_O_MARGIN			m_pDblOpt[MKLA_DBL_O_MARGIN]
#define	LTH_D_ELLIPSE			m_pDblOpt[MKLA_DBL_ELLIPSE]
#define	LTH_D_E_FEED			m_pDblOpt[MKLA_DBL_E_FEED]
#define	LTH_D_E_CUT				m_pDblOpt[MKLA_DBL_E_CUT]
#define	LTH_D_E_STEP			m_pDblOpt[MKLA_DBL_E_STEP]
#define	LTH_D_E_PULLZ			m_pDblOpt[MKLA_DBL_E_PULLZ]
#define	LTH_D_E_PULLX			m_pDblOpt[MKLA_DBL_E_PULLX]
#define	LTH_D_DRILLZ			m_pDblOpt[MKLA_DBL_DRILLZ]
#define	LTH_D_DRILLR			m_pDblOpt[MKLA_DBL_DRILLR]
#define	LTH_D_DRILLQ			m_pDblOpt[MKLA_DBL_DRILLQ]
#define	LTH_D_DRILLD			m_pDblOpt[MKLA_DBL_DRILLD]
#define	LTH_D_DWELL				m_pDblOpt[MKLA_DBL_DWELL]
#define	LTH_D_HOLE				m_pDblOpt[MKLA_DBL_HOLE]
#define	LTH_D_I_FEED			m_pDblOpt[MKLA_DBL_I_FEED]
#define	LTH_D_I_FEEDX			m_pDblOpt[MKLA_DBL_I_FEEDX]
#define	LTH_D_I_CUT				m_pDblOpt[MKLA_DBL_I_CUT]
#define	LTH_D_I_PULLZ			m_pDblOpt[MKLA_DBL_I_PULLZ]
#define	LTH_D_I_PULLX			m_pDblOpt[MKLA_DBL_I_PULLX]
#define	LTH_D_I_MARGIN			m_pDblOpt[MKLA_DBL_I_MARGIN]
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
#define	LTH_F_ENDFACE			m_pFlgOpt[MKLA_FLG_ENDFACE]
#define	LTH_F_CYCLE				m_pFlgOpt[MKLA_FLG_CYCLE]
//
#define	LTH_S_LINEFORM			m_strOption[MKLA_STR_LINEFORM]
#define	LTH_S_EOB				m_strOption[MKLA_STR_EOB]
#define	LTH_S_HEADER			m_strOption[MKLA_STR_HEADER]
#define	LTH_S_FOOTER			m_strOption[MKLA_STR_FOOTER]
#define	LTH_S_DRILL				m_strOption[MKLA_STR_DRILL]
#define	LTH_S_DRILLSPINDLE		m_strOption[MKLA_STR_DRILLSPINDLE]
#define	LTH_S_DRILLFEED			m_strOption[MKLA_STR_DRILLFEED]
#define	LTH_S_D_CUSTOM			m_strOption[MKLA_STR_D_CUSTOM]
#define	LTH_S_E_CUSTOM			m_strOption[MKLA_STR_E_CUSTOM]
#define	LTH_S_I_CUSTOM			m_strOption[MKLA_STR_I_CUSTOM]
#define	LTH_S_O_CUSTOM			m_strOption[MKLA_STR_O_CUSTOM]
//
struct LATHEDRILLINFO
{
	float	d;	// 径
	int		s;	// 回転数
	float	f;	// 送り速度
};
typedef	std::vector<LATHEDRILLINFO>		VLATHEDRILLINFO;
//
class CNCMakeLatheOpt : public CNCMakeOption
{
	friend class CMKLASetup0;
	friend class CMKLASetup1;
	friend class CMKLASetup2;
	friend class CMKLASetup3;
	friend class CMKLASetup4;
	friend class CMKNCSetup2;
	friend class CMKNCSetup6;

	// 親ｸﾗｽにﾃﾞｰﾀを持たせるので、
	// union/struct技は使えない
	// むしろC++らしくｺｰﾃﾞｨﾝｸﾞ

protected:
	virtual	void	InitialDefault(void);
	virtual	BOOL	IsPathID(int);

public:
	CNCMakeLatheOpt(LPCTSTR);

	BOOL	GetDrillInfo(VLATHEDRILLINFO&) const;
	virtual	CString	GetLineNoForm(void) const;

#ifdef _DEBUG
	virtual	void	DbgDump(void) const;	// ｵﾌﾟｼｮﾝ変数のﾀﾞﾝﾌﾟ
#endif
};
