// NCMakeWireOpt.cpp: CNCMakeWireOpt クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "NCMakeWireOpt.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// int型命令
static	LPCTSTR	g_szWirNOrder[] = {
	"ProgNo", "LineAddType", "G90", "Dot", "FDot", "CircleCode"
};
static	const	int		g_dfWirNOrder[] = {
	1, 1, 0, 0, 2, 0
};

// float型命令
static	LPCTSTR	g_szWirDOrder[] = {
	"Depth", "Taper", "Feed",
	"G92X", "G92Y",
	"AWFcircleLo", "AWFcircleHi",
	"Ellipse"
};
static	const	float	g_dfWirDOrder[] = {
	10.0f, 0.0f, 0.0f,
	0.0f, 0.0f,
	1.0f, 1.0f,
	0.5f,
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

CString CNCMakeWireOpt::GetLineNoForm(void) const
{
	return WIR_S_LINEFORM;
}

#ifdef _DEBUG
void CNCMakeWireOpt::DbgDump(void) const
{
	printf("CNCMakeWireOpt InitFile=%s\n", LPCTSTR(GetInitFile()));
	printf("----------\n");
	printf("  Depth        =%f\n", WIR_D_DEPTH);
	printf("  Taper        =%f\n", WIR_D_TAPER);
	printf("  TaperMode    =%s\n", LPCTSTR(WIR_S_TAPERMODE));
	printf("  Feed         =%f\n", WIR_D_FEED);
	printf("  Header       =%s\n", LPCTSTR(WIR_S_HEADER));
	printf("  Footer       =%s\n", LPCTSTR(WIR_S_FOOTER));
	printf("----------\n");
	printf("  AWFconnect   =%s\n", LPCTSTR(WIR_S_AWFCNT));
	printf("  AWFcut       =%s\n", LPCTSTR(WIR_S_AWFCUT));
	printf("  AWFcircleLo  =%f\n", WIR_D_AWFCIRCLE_LO);
	printf("  AWFcircleHi  =%f\n", WIR_D_AWFCIRCLE_HI);
	printf("  AWFstart     =%d\n", WIR_F_AWFSTART);
	printf("  AWFend       =%d\n", WIR_F_AWFEND);
	printf("----------\n");
	printf("  bProgNo?     =%d\n", WIR_F_PROG);
	printf("  nProgNo      =%d\n", WIR_I_PROG);
	printf("  bProgNoAuto  =%d\n", WIR_F_PROGAUTO);
	printf("  bLineAdd     =%d\n", WIR_F_LINEADD);
	printf("  LineForm     =%s\n", LPCTSTR(WIR_S_LINEFORM));
	printf("  nLineAdd     =%d\n", WIR_I_LINEADD);
	printf("  EOB          =%s\n", LPCTSTR(WIR_S_EOB));
	printf("  G90          =%d\n", WIR_I_G90);
	printf("  Gclip        =%d\n", WIR_F_GCLIP);
	printf("----------\n");
	printf("  Dot          =%d\n", WIR_I_DOT);
	printf("  FDot         =%d\n", WIR_I_FDOT);
	printf("  ZeroCut      =%d\n", WIR_F_ZEROCUT);
	printf("  CircleCode   =%d\n", WIR_I_CIRCLECODE);
	printf("  Ellipse      =%f\n", WIR_D_ELLIPSE);
	printf("  EllipseFlg   =%d\n", WIR_F_ELLIPSE);
}
#endif
