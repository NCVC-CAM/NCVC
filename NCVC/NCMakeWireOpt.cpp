// NCMakeWireOpt.cpp: CNCMakeWireOpt クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "NCMakeWireOpt.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif
//#define	_DEBUGOLD
#undef	_DEBUGOLD


// int型命令
static	LPCTSTR	g_szNOrder[] = {
	"ProgNo", "LineAddType", "G90", "Dot", "FDot", "CircleCode"
};
static	const	int		g_dfNOrder[] = {
	1, 1, 0, 0, 2, 0
};

// double型命令
static	LPCTSTR	g_szDOrder[] = {
	"Depth", "Taper", "Feed",
	"G92X", "G92Y",
	"AWFcircleLo", "AWFcircleHi",
	"Ellipse"
};
static	const	double	g_dfDOrder[] = {
	10.0, 0.0, 0.0,
	0.0, 0.0,
	1.0, 1.0,
	0.5,
};

// BOOL型命令
static	LPCTSTR	g_szBOrder[] = {
	"ProgSet", "ProgAuto", "LineAdd", "ZeroCut", "GClip", "EllipseFlg",
	// --
	"AWFstart", "AWFend"
};
static	const	BOOL	g_dfBOrder[] = {
	TRUE, FALSE, FALSE, TRUE, TRUE, TRUE,
	TRUE, TRUE
};

// CString型命令
static	LPCTSTR	g_szSOrder[] = {
	"LineForm", "EOB", "Header", "Footer",
	"TaperMode", "AWFconnect", "AWFcut"
};
static	LPCTSTR	g_dfSOrder[] = {
	"N%04d", "", "HeaderWire.txt", "FooterWire.txt",
	"M15P0", "M60", "M50"
};

// 保存に関する情報
static	LPCTSTR	g_szInitComment[] = {
	"##\n",
	"## NCVC:ﾜｲﾔ放電加工機用切削条件ﾌｧｲﾙ\n",
};
static	SAVEORDER	g_stSaveOrder[] = {
	{NC_PAGE,	1},		// Page1(基本:Dialog1)
	{NC_DBL,	MKWI_DBL_DEPTH,			"ﾜｰｸ厚み"},
	{NC_DBL,	MKWI_DBL_TAPER,			"ﾃｰﾊﾟ角度[deg]"},
	{NC_STR,	MKWI_STR_TAPERMODE,		"ﾃｰﾊﾟﾓｰﾄﾞ"},
	{NC_DBL,	MKWI_DBL_FEED,			"切削送り(ｾﾞﾛはｻｰﾎﾞ送り)"},
	{NC_DBL,	MKWI_DBL_G92X,			"切削原点(G92X)"},
	{NC_DBL,	MKWI_DBL_G92Y,			"切削原点(G92Y)"},
	{NC_STR,	MKWI_STR_HEADER,		"ｶｽﾀﾑﾍｯﾀﾞｰ"},
	{NC_STR,	MKWI_STR_FOOTER,		"ｶｽﾀﾑﾌｯﾀﾞｰ"},
	{NC_PAGE,	2},		// Page2(AWF:Dialog2)
	{NC_STR,	MKWI_STR_AWFCNT,		"AWF結線ｺｰﾄﾞ"},
	{NC_STR,	MKWI_STR_AWFCUT,		"AWF切断ｺｰﾄﾞ"},
	{NC_DBL,	MKWI_DBL_AWFCIRCLE_LO,	"AWF結線ﾎﾟｲﾝﾄ 対象半径"},
	{NC_DBL,	MKWI_DBL_AWFCIRCLE_HI,	"AWF結線ﾎﾟｲﾝﾄ 対象半径"},
	{NC_FLG,	MKWI_FLG_AWFSTART,		"加工前結線"},
	{NC_FLG,	MKWI_FLG_AWFEND,		"加工後切断"},
	{NC_PAGE,	3},		// Page2(生成:Dialog2)
	{NC_FLG,	MKWI_FLG_PROG,			"Ｏ番号生成"},
	{NC_NUM,	MKWI_NUM_PROG,			"ﾌﾟﾛｸﾞﾗﾑ番号"},
	{NC_FLG,	MKWI_FLG_PROGAUTO,		"おまかせ番号"},
	{NC_FLG,	MKWI_FLG_LINEADD,		"行番号追加"},
	{NC_STR,	MKWI_STR_LINEFORM,		"行番号書式"},
	{NC_NUM,	MKWI_NUM_LINEADD,		"行番号倍率"},
	{NC_STR,	MKWI_STR_EOB,			"EOB"},
	{NC_NUM,	MKWI_NUM_G90,			"位置指令(0:G90,1:G91)"},
	{NC_FLG,	MKWI_FLG_GCLIP,			"ﾓｰﾀﾞﾙ"},
	{NC_PAGE,	4},		// Page3(表記:Dialog6)
	{NC_NUM,	MKWI_NUM_DOT,			"座標表記(0:小数点,1:1/1000)"},
	{NC_NUM,	MKWI_NUM_FDOT,			"Fﾊﾟﾗﾒｰﾀ表記(0:小数点,1:1/1000,2:整数)"},
	{NC_FLG,	MKWI_FLG_ZEROCUT,		"小数点以下のｾﾞﾛｶｯﾄ"},
	{NC_NUM,	MKWI_NUM_CIRCLECODE,	"円ﾃﾞｰﾀの切削(0:G02,1:G03)"},
	{NC_DBL,	MKWI_DBL_ELLIPSE,		"楕円公差"},
	{NC_FLG,	MKWI_FLG_ELLIPSE,		"長径と短径が等しい楕円は円とみなす"}
};

/////////////////////////////////////////////////////////////////////////////
// CNCMakeWireOpt クラスの構築/消滅

CNCMakeWireOpt::CNCMakeWireOpt(LPCTSTR lpszInit) :
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
void CNCMakeWireOpt::DbgDump(void) const
{
	CMagaDbg	dbg("CNCMakeWireOpt", DBG_RED);

	dbg.printf("InitFile=%s", GetInitFile());
	dbg.printf("----------");
	dbg.printf("  Depth        =%f", m_dDepth);
	dbg.printf("  Taper        =%f", m_dTaper);
	dbg.printf("  TaperMode    =%s", m_strOption[MKWI_STR_TAPERMODE]);
	dbg.printf("  Feed         =%f", m_dFeed);
	dbg.printf("  Header       =%s", m_strOption[MKWI_STR_HEADER]);
	dbg.printf("  Footer       =%s", m_strOption[MKWI_STR_FOOTER]);
	dbg.printf("----------");
	dbg.printf("  AWFconnect   =%s", m_strOption[MKWI_STR_AWFCNT]);
	dbg.printf("  AWFcut       =%s", m_strOption[MKWI_STR_AWFCUT]);
	dbg.printf("  AWFcircleLo  =%f", m_dAWFcircleLo);
	dbg.printf("  AWFcircleHi  =%f", m_dAWFcircleHi);
	dbg.printf("  AWFstart     =%d", m_bAWFstart);
	dbg.printf("  AWFend       =%d", m_bAWFend);
	dbg.printf("----------");
	dbg.printf("  bProgNo?     =%d", m_bProg);
	dbg.printf("  bProgNo      =%d", m_nProg);
	dbg.printf("  bProgNoAuto  =%d", m_bProgAuto);
	dbg.printf("  bLineAdd     =%d", m_bLineAdd);
	dbg.printf("  LineForm     =%s", m_strOption[MKWI_STR_LINEFORM]);
	dbg.printf("  nLineAdd     =%d", m_nLineAdd);
	dbg.printf("  EOB          =%s", m_strOption[MKWI_STR_EOB]);
	dbg.printf("  G90          =%d", m_nG90);
	dbg.printf("  Gclip        =%d", m_bGclip);
	dbg.printf("----------");
	dbg.printf("  Dot          =%d", m_nDot);
	dbg.printf("  FDot         =%d", m_nFDot);
	dbg.printf("  ZeroCut      =%d", m_bZeroCut);
	dbg.printf("  CircleCode   =%d", m_nCircleCode);
	dbg.printf("  Ellipse      =%f", m_dEllipse);
	dbg.printf("  EllipseFlg   =%d", m_bEllipse);
}
#endif
