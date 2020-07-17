// NCMakeLatheOpt.cpp: CNCMakeLatheOpt クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "NCMakeLatheOpt.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif
//#define	_DEBUGOLD
#undef	_DEBUGOLD

// int型命令
static	LPCTSTR	g_szNOrder[] = {
	"Spindle", "MarginNum",
	"ProgNo", "LineAddType",
	"G90", "Dot", "FDot", "CircleCode", "CircleIJ"
};
static	const	int		g_dfNOrder[] = {
	200, 1,
	1, 1,
	0, 0, 0, 0, 0
};

// double型命令
static	LPCTSTR	g_szDOrder[] = {
	"Feed", "XFeed", "Cut", "PullZ", "PullX",
	"Margin", "Ellipse"
};
static	const	double	g_dfDOrder[] = {
	300.0, 150.0, 1.0, 2.0, 2.0,
	1.0, 0.5,
};

// BOOL型命令
static	LPCTSTR	g_szBOrder[] = {
	"ProgSet", "ProgAuto",
	"LineAdd", "ZeroCut", "GClip", "DisableSpindle",
	"CircleHalf", "ZeroCutIJ",
	"EllipseFlg"
};
static	const	BOOL	g_dfBOrder[] = {
	TRUE, FALSE,
	FALSE, TRUE, TRUE, FALSE,
	FALSE, TRUE,
	TRUE,
};

// CString型命令
static	LPCTSTR	g_szSOrder[] = {
	"LineForm", "EOB",
	"Header", "Footer"
};
static	LPCTSTR	g_dfSOrder[] = {
	"N%04d", "",
	"HeaderLathe.txt", "FooterLathe.txt"
};

// 保存に関する情報
static	LPCTSTR	g_szInitComment[] = {
	"##\n",
	"## NCVC:旋盤用切削条件ﾌｧｲﾙ\n",
	"##\tX軸の値は半径値です。ﾀﾞｲｱﾛｸﾞ設定では倍されます。\n"
};
static	SAVEORDER	g_stSaveOrder[] = {
	{NC_PAGE,	1},		// Page1(基本:Dialog1)
	{NC_NUM,	MKLA_NUM_SPINDLE,		"主軸回転数"},
	{NC_DBL,	MKLA_DBL_FEED,			"切削送り(Z)"},
	{NC_DBL,	MKLA_DBL_XFEED,			"切削送り(X)"},
	{NC_DBL,	MKLA_DBL_CUT,			"切り込み(半径値)"},
	{NC_DBL,	MKLA_DBL_PULL_Z,		"引き代(Z)"},
	{NC_DBL,	MKLA_DBL_PULL_X,		"引き代(X半径値)"},
	{NC_DBL,	MKLA_DBL_MARGIN,		"仕上げ代(半径値)"},
	{NC_NUM,	MKLA_NUM_MARGIN,		"仕上げ回数"},
	{NC_STR,	MKLA_STR_HEADER,		"ｶｽﾀﾑﾍｯﾀﾞｰ"},
	{NC_STR,	MKLA_STR_FOOTER,		"ｶｽﾀﾑﾌｯﾀﾞｰ"},
	{NC_PAGE,	2},		// Page2(生成:Dialog2)
	{NC_FLG,	MKLA_FLG_PROG,			"Ｏ番号生成"},
	{NC_NUM,	MKLA_NUM_PROG,			"ﾌﾟﾛｸﾞﾗﾑ番号"},
	{NC_FLG,	MKLA_FLG_PROGAUTO,		"おまかせ番号"},
	{NC_FLG,	MKLA_FLG_LINEADD,		"行番号追加"},
	{NC_STR,	MKLA_STR_LINEFORM,		"行番号書式"},
	{NC_NUM,	MKLA_NUM_LINEADD,		"行番号倍率"},
	{NC_STR,	MKLA_STR_EOB,			"EOB"},
	{NC_NUM,	MKLA_NUM_G90,			"位置指令(0:G90,1:G91)"},
	{NC_FLG,	MKLA_FLG_GCLIP,			"ﾓｰﾀﾞﾙ"},
	{NC_FLG,	MKLA_FLG_DISABLESPINDLE,"Sﾊﾟﾗﾒｰﾀを生成しない"},
	{NC_PAGE,	3},		// Page3(表記:Dialog6)
	{NC_NUM,	MKLA_NUM_DOT,			"座標表記(0:小数点,1:1/1000)"},
	{NC_NUM,	MKLA_NUM_FDOT,			"Fﾊﾟﾗﾒｰﾀ表記(0:小数点,1:1/1000,2:整数)"},
	{NC_FLG,	MKLA_FLG_ZEROCUT,		"小数点以下のｾﾞﾛｶｯﾄ"},
	{NC_NUM,	MKLA_NUM_CIRCLECODE,	"円ﾃﾞｰﾀの切削(0:G02,1:G03)"},
	{NC_NUM,	MKLA_NUM_IJ,			"円弧指示(0:R,1:I/J)"},
	{NC_FLG,	MKLA_FLG_CIRCLEHALF,	"全円は2分割"},
	{NC_FLG,	MKLA_FLG_ZEROCUT_IJ,	"[I|J]0は省略"},
	{NC_DBL,	MKLA_DBL_ELLIPSE,		"楕円公差"},
	{NC_FLG,	MKLA_FLG_ELLIPSE,		"長径と短径が等しい楕円は円とみなす"}
};

/////////////////////////////////////////////////////////////////////////////
// CNCMakeLatheOpt クラスの構築/消滅

CNCMakeLatheOpt::CNCMakeLatheOpt(LPCTSTR lpszInit) :
	CNCMakeOption(
		SIZEOF(g_szNOrder), g_szNOrder, g_dfNOrder, m_unNums,
		SIZEOF(g_szDOrder), g_szDOrder, g_dfDOrder, m_udNums,
		SIZEOF(g_szBOrder), g_szBOrder, g_dfBOrder, m_ubFlags,
		SIZEOF(g_szSOrder), g_szSOrder, g_dfSOrder,
		SIZEOF(g_szInitComment), g_szInitComment,
		SIZEOF(g_stSaveOrder),   g_stSaveOrder)
{
	ASSERT( SIZEOF(g_szNOrder) == SIZEOF(g_dfNOrder) );
	ASSERT( SIZEOF(g_szNOrder) == SIZEOF(m_unNums) );
	ASSERT( SIZEOF(g_szDOrder) == SIZEOF(g_dfDOrder) );
	ASSERT( SIZEOF(g_szDOrder) == SIZEOF(m_udNums) );
	ASSERT( SIZEOF(g_szBOrder) == SIZEOF(g_dfBOrder) );
	ASSERT( SIZEOF(g_szBOrder) == SIZEOF(m_ubFlags) );
	ASSERT( SIZEOF(g_szSOrder) == SIZEOF(g_dfSOrder) );

	ReadMakeOption(lpszInit);
}

/////////////////////////////////////////////////////////////////////////////
// ﾕｰｻﾞﾒﾝﾊﾞ関数

#ifdef _DEBUGOLD
void CNCMakeLatheOpt::DbgDump(void) const
{
	CMagaDbg	dbg("CNCMakeLatheOpt", DBG_RED);

	dbg.printf("InitFile=%s", GetInitFile());
	dbg.printf("----------");
	dbg.printf("  Spindle      =%d", m_nSpindle);
	dbg.printf("  Feed         =%f", m_dFeed);
	dbg.printf("  Cut          =%f", m_dCut);
	dbg.printf("  PullZ        =%f", m_dPullZ);
	dbg.printf("  PullX        =%f", m_dPullX);
	dbg.printf("  Margin       =%f", m_dMargin);
	dbg.printf("  Header       =%s", m_strOption[MKLA_STR_HEADER]);
	dbg.printf("  Footer       =%s", m_strOption[MKLA_STR_FOOTER]);
	dbg.printf("----------");
	dbg.printf("  bLineAdd     =%d", m_bLineAdd);
	dbg.printf("  LineForm     =%s", m_strOption[MKLA_STR_LINEFORM]);
	dbg.printf("  nLineAdd     =%d", m_nLineAdd);
	dbg.printf("  EOB          =%s", m_strOption[MKLA_STR_EOB]);
	dbg.printf("  G90          =%d", m_nG90);
	dbg.printf("  Gclip        =%d", m_bGclip);
	dbg.printf("  DisSpindle   =%d", m_bDisableSpindle);
	dbg.printf("----------");
	dbg.printf("  Dot          =%d", m_nDot);
	dbg.printf("  FDot         =%d", m_nFDot);
	dbg.printf("  ZeroCut      =%d", m_bZeroCut);
	dbg.printf("  CircleCode   =%d", m_nCircleCode);
	dbg.printf("  IJ           =%d", m_nIJ);
	dbg.printf("  CircleHalf   =%d", m_bCircleHalf);
	dbg.printf("  ZeroCutIJ    =%d", m_bZeroCutIJ);
	dbg.printf("----------");
	dbg.printf("  Ellipse      =%f", m_dEllipse);
	dbg.printf("  EllipseFlg   =%d", m_bEllipse);
}
#endif
