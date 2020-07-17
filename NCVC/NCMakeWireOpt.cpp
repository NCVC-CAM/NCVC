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

// int型命令
static	LPCTSTR	g_szWirNOrder[] = {
	"ProgNo", "LineAddType", "G90", "Dot", "FDot", "CircleCode"
};
static	const	int		g_dfWirNOrder[] = {
	1, 1, 0, 0, 2, 0
};

// double型命令
static	LPCTSTR	g_szWirDOrder[] = {
	"Depth", "Taper", "Feed",
	"G92X", "G92Y",
	"AWFcircleLo", "AWFcircleHi",
	"Ellipse"
};
static	const	double	g_dfWirDOrder[] = {
	10.0, 0.0, 0.0,
	0.0, 0.0,
	1.0, 1.0,
	0.5,
};

// BOOL型命令
static	LPCTSTR	g_szWirBOrder[] = {
	"ProgSet", "ProgAuto", "LineAdd", "ZeroCut", "GClip", "EllipseFlg",
	// --
	"AWFstart", "AWFend"
};
static	const	BOOL	g_dfWirBOrder[] = {
	TRUE, FALSE, FALSE, TRUE, TRUE, TRUE,
	TRUE, TRUE
};

// CString型命令
static	LPCTSTR	g_szWirSOrder[] = {
	"LineForm", "EOB", "Header", "Footer",
	"TaperMode", "AWFconnect", "AWFcut"
};
static	LPCTSTR	g_dfWirSOrder[] = {
	"N%04d", "", "HeaderWire.txt", "FooterWire.txt",
	"M15P0", "M60", "M50"
};

// ｵﾌﾟｼｮﾝ統合
static	NCMAKEOPTION	WirOption[] = {
	{MKWI_NUM_NUMS, g_szWirNOrder},
	{MKWI_DBL_NUMS, g_szWirDOrder},
	{MKWI_FLG_NUMS, g_szWirBOrder},
	{MKWI_STR_NUMS, g_szWirSOrder}
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
	CNCMakeOption(WirOption,
		SIZEOF(g_szInitComment), g_szInitComment,
		SIZEOF(g_stSaveOrder),   g_stSaveOrder)
{
	ASSERT( MKWI_NUM_NUMS == SIZEOF(g_szWirNOrder) );
	ASSERT( MKWI_NUM_NUMS == SIZEOF(g_dfWirNOrder) );
	ASSERT( MKWI_DBL_NUMS == SIZEOF(g_szWirDOrder) );
	ASSERT( MKWI_DBL_NUMS == SIZEOF(g_dfWirDOrder) );
	ASSERT( MKWI_FLG_NUMS == SIZEOF(g_szWirBOrder) );
	ASSERT( MKWI_FLG_NUMS == SIZEOF(g_dfWirBOrder) );
	ASSERT( MKWI_STR_NUMS == SIZEOF(g_szWirSOrder) );
	ASSERT( MKWI_STR_NUMS == SIZEOF(g_dfWirSOrder) );

	ReadMakeOption(lpszInit);
}

/////////////////////////////////////////////////////////////////////////////
// ﾕｰｻﾞﾒﾝﾊﾞ関数

void CNCMakeWireOpt::InitialDefault(void)
{
	extern	LPTSTR	g_pszExecDir;	// 実行ﾃﾞｨﾚｸﾄﾘ(NCVC.cpp)
	int		i;

	for ( i=0; i<MKWI_NUM_NUMS; i++ )
		m_pIntOpt[i] = g_dfWirNOrder[i];
	for ( i=0; i<MKWI_DBL_NUMS; i++ )
		m_pDblOpt[i] = g_dfWirDOrder[i];
	for ( i=0; i<MKWI_FLG_NUMS; i++ )
		m_pFlgOpt[i] = g_dfWirBOrder[i];
	m_strOption.RemoveAll();
	for ( i=0; i<MKWI_STR_NUMS; i++ )
		m_strOption.Add(g_dfWirSOrder[i]);

	for ( i=MKWI_STR_HEADER; i<=MKWI_STR_FOOTER; i++ )
		m_strOption[i] = g_pszExecDir + m_strOption[i];
}

BOOL CNCMakeWireOpt::IsPathID(int n)
{
	return ( n==MKWI_STR_HEADER || n==MKWI_STR_FOOTER );
}

#ifdef _DEBUG
void CNCMakeWireOpt::DbgDump(void) const
{
	CMagaDbg	dbg("CNCMakeWireOpt", DBG_RED);

	dbg.printf("InitFile=%s", GetInitFile());
	dbg.printf("----------");
	dbg.printf("  Depth        =%f", WIR_D_DEPTH);
	dbg.printf("  Taper        =%f", WIR_D_TAPER);
	dbg.printf("  TaperMode    =%s", m_strOption[MKWI_STR_TAPERMODE]);
	dbg.printf("  Feed         =%f", WIR_D_FEED);
	dbg.printf("  Header       =%s", m_strOption[MKWI_STR_HEADER]);
	dbg.printf("  Footer       =%s", m_strOption[MKWI_STR_FOOTER]);
	dbg.printf("----------");
	dbg.printf("  AWFconnect   =%s", m_strOption[MKWI_STR_AWFCNT]);
	dbg.printf("  AWFcut       =%s", m_strOption[MKWI_STR_AWFCUT]);
	dbg.printf("  AWFcircleLo  =%f", WIR_D_AWFCIRCLE_LO);
	dbg.printf("  AWFcircleHi  =%f", WIR_D_AWFCIRCLE_HI);
	dbg.printf("  AWFstart     =%d", WIR_F_AWFSTART);
	dbg.printf("  AWFend       =%d", WIR_F_AWFEND);
	dbg.printf("----------");
	dbg.printf("  bProgNo?     =%d", WIR_F_PROG);
	dbg.printf("  nProgNo      =%d", WIR_I_PROG);
	dbg.printf("  bProgNoAuto  =%d", WIR_F_PROGAUTO);
	dbg.printf("  bLineAdd     =%d", WIR_F_LINEADD);
	dbg.printf("  LineForm     =%s", m_strOption[MKWI_STR_LINEFORM]);
	dbg.printf("  nLineAdd     =%d", WIR_I_LINEADD);
	dbg.printf("  EOB          =%s", m_strOption[MKWI_STR_EOB]);
	dbg.printf("  G90          =%d", WIR_I_G90);
	dbg.printf("  Gclip        =%d", WIR_F_GCLIP);
	dbg.printf("----------");
	dbg.printf("  Dot          =%d", WIR_I_DOT);
	dbg.printf("  FDot         =%d", WIR_I_FDOT);
	dbg.printf("  ZeroCut      =%d", WIR_F_ZEROCUT);
	dbg.printf("  CircleCode   =%d", WIR_I_CIRCLECODE);
	dbg.printf("  Ellipse      =%f", WIR_D_ELLIPSE);
	dbg.printf("  EllipseFlg   =%d", WIR_F_ELLIPSE);
}
#endif
