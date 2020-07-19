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

// int型命令
static	LPCTSTR	g_szLthNOrder[] = {
	"ProgNo", "LineAddType",
	"G90", "Dot", "FDot", "CircleCode", "CircleIJ",
	"EndFaceSpindle", "InsideSpindle", "OutsideSpindle",
	"InsideMarginNum", "MarginNum"
};
static	const	int		g_dfLthNOrder[] = {
	1, 1,
	0, 0, 0, 0, 0,
	200, 200, 200,
	1, 1
};

// float型命令
static	LPCTSTR	g_szLthDOrder[] = {
	"Feed", "XFeed", "Cut", "PullZ", "PullX", "Margin",
	"Ellipse",
	"EndFaceFeed", "EndFaceCut", "EndFaceStep", "EndFacePullZ", "EndFacePullX",
	"DrillZ", "DrillR", "DrillQ", "DrillD", "Dwell", "PilotHole",
	"InsideFeed", "InsideFeedX", "InsideCut",
		"InsidePullZ", "InsidePullX", "InsideMargin"
};
static	const	float	g_dfLthDOrder[] = {
	300.0, 150.0, 1.0, 2.0, 2.0, 1.0,
	0.5,
	150.0, -5.0, -1.0, 2.0, 2.0,
	-50.0, 10.0, 15.0, 10.0, 0.0, 0.0,
	300.0, 150, 0.5, 2.0, 2.0, 1.0
};

// BOOL型命令
static	LPCTSTR	g_szLthBOrder[] = {
	"ProgSet", "ProgAuto",
	"LineAdd", "ZeroCut", "GClip", "DisableSpindle",
	"CircleHalf", "ZeroCutIJ", "EllipseFlg",
	"EndFace", "DrillCycle"
};
static	const	BOOL	g_dfLthBOrder[] = {
	TRUE, FALSE,
	FALSE, TRUE, TRUE, FALSE,
	FALSE, TRUE, TRUE,
	FALSE, FALSE
};

// CString型命令
static	LPCTSTR	g_szLthSOrder[] = {
	"LineForm", "EOB",
	"Header", "Footer",
	"UseDrill", "DrillSpindle", "DrillFeed", "DrillCustom",
	"EndFaceCustom", "InsideCustom", "OutsideCustom"
};
static	LPCTSTR	g_dfLthSOrder[] = {
	"N%04d", "",
	"HeaderLathe.txt", "FooterLathe.txt",
	"", "", "", "(Enter drill-tool change code etc)",
	"\\n(TANMEN START)\\n(Enter TANMEN-tool change code etc)",
	"\\n(NAIKEI START)\\n(Enter NAIKEI-tool change code etc)",
	"\\n(GAIKEI START)\\n(Enter GAIKEI-tool change code etc)"
};

// ｵﾌﾟｼｮﾝ統合
static	NCMAKEOPTION	LthOption[] = {
	{MKLA_NUM_NUMS, g_szLthNOrder},
	{MKLA_DBL_NUMS, g_szLthDOrder},
	{MKLA_FLG_NUMS, g_szLthBOrder},
	{MKLA_STR_NUMS, g_szLthSOrder}
};

// 保存に関する情報
static	LPCTSTR	g_szInitComment[] = {
	"##\n",
	"## NCVC:旋盤用切削条件ﾌｧｲﾙ\n",
	"##\tX軸の値は半径値です。ﾀﾞｲｱﾛｸﾞ設定では２倍されます。\n"
};
static	SAVEORDER	g_stSaveOrder[] = {
	{NC_PAGE,	1},		// Page1(基本)
	{NC_STR,	MKLA_STR_HEADER,		"ｶｽﾀﾑﾍｯﾀﾞｰ"},
	{NC_STR,	MKLA_STR_FOOTER,		"ｶｽﾀﾑﾌｯﾀﾞｰ"},
	{NC_PAGE,	2},		// Page2(生成)
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
	{NC_PAGE,	3},		// Page3(表記)
	{NC_NUM,	MKLA_NUM_DOT,			"座標表記(0:小数点,1:1/1000)"},
	{NC_NUM,	MKLA_NUM_FDOT,			"Fﾊﾟﾗﾒｰﾀ表記(0:小数点,1:1/1000,2:整数)"},
	{NC_FLG,	MKLA_FLG_ZEROCUT,		"小数点以下のｾﾞﾛｶｯﾄ"},
	{NC_NUM,	MKLA_NUM_CIRCLECODE,	"円ﾃﾞｰﾀの切削(0:G02,1:G03)"},
	{NC_NUM,	MKLA_NUM_IJ,			"円弧指示(0:R,1:I/J)"},
	{NC_FLG,	MKLA_FLG_CIRCLEHALF,	"全円は2分割"},
	{NC_FLG,	MKLA_FLG_ZEROCUT_IJ,	"[I|J]0は省略"},
	{NC_DBL,	MKLA_DBL_ELLIPSE,		"楕円公差"},
	{NC_FLG,	MKLA_FLG_ELLIPSE,		"長径と短径が等しい楕円は円とみなす"},
	{NC_PAGE,	4},		// Page4(端面)
	{NC_FLG,	MKLA_FLG_ENDFACE,		"端面処理を行う"},
	{NC_STR,	MKLA_STR_E_CUSTOM,		"端面ｶｽﾀﾑｺｰﾄﾞ"},
	{NC_NUM,	MKLA_NUM_E_SPINDLE,		"端面主軸回転数"},
	{NC_DBL,	MKLA_DBL_E_FEED,		"端面切削送り"},
	{NC_DBL,	MKLA_DBL_E_CUT,			"端面最終切り込み"},
	{NC_DBL,	MKLA_DBL_E_STEP,		"端面切り込みｽﾃｯﾌﾟ"},
	{NC_DBL,	MKLA_DBL_E_PULLZ,		"端面引き代(Z)"},
	{NC_DBL,	MKLA_DBL_E_PULLX,		"端面引き代(X)"},
	{NC_PAGE,	5},		// Page5(下穴)
	{NC_STR,	MKLA_STR_DRILL,			"使用ドリル(複数の場合はｺﾝﾏで区切る)"},
	{NC_STR,	MKLA_STR_DRILLSPINDLE,	"下穴主軸回転数"},
	{NC_STR,	MKLA_STR_DRILLFEED,		"下穴切削送り"},
	{NC_STR,	MKLA_STR_D_CUSTOM,		"下穴ｶｽﾀﾑｺｰﾄﾞ"},
	{NC_DBL,	MKLA_DBL_HOLE,			"既存下穴ｻｲｽﾞ"},
	{NC_FLG,	MKLA_FLG_CYCLE,			"固定ｻｲｸﾙで生成"},
	{NC_DBL,	MKLA_DBL_DRILLZ,		"切り込み"},
	{NC_DBL,	MKLA_DBL_DRILLQ,		"R点"},
	{NC_DBL,	MKLA_DBL_DRILLQ,		"Q値"},
	{NC_DBL,	MKLA_DBL_DRILLD,		"戻り量"},
	{NC_DBL,	MKLA_DBL_DWELL,			"ﾄﾞｳｪﾙ時間[msec]"},
	{NC_PAGE,	6},		// Page6(内径)
	{NC_STR,	MKLA_STR_I_CUSTOM,		"内径ｶｽﾀﾑｺｰﾄﾞ"},
	{NC_NUM,	MKLA_NUM_I_SPINDLE,		"内径主軸回転数"},
	{NC_DBL,	MKLA_DBL_I_FEED,		"内径切削送り(Z)"},
	{NC_DBL,	MKLA_DBL_I_FEEDX,		"内径切削送り(X)"},
	{NC_DBL,	MKLA_DBL_I_CUT,			"内径切り込み(半径値)"},
	{NC_DBL,	MKLA_DBL_I_PULLZ,		"内径引き代(Z)"},
	{NC_DBL,	MKLA_DBL_I_PULLX,		"内径引き代(X半径値)"},
	{NC_DBL,	MKLA_DBL_I_MARGIN,		"内径仕上げ代(半径値)"},
	{NC_NUM,	MKLA_NUM_I_MARGIN,		"内径仕上げ回数"},
	{NC_PAGE,	7},		// Page7(外径)
	{NC_STR,	MKLA_STR_O_CUSTOM,		"外径ｶｽﾀﾑｺｰﾄﾞ"},
	{NC_NUM,	MKLA_NUM_O_SPINDLE,		"外径主軸回転数"},
	{NC_DBL,	MKLA_DBL_O_FEED,		"外径切削送り(Z)"},
	{NC_DBL,	MKLA_DBL_O_FEEDX,		"外径切削送り(X)"},
	{NC_DBL,	MKLA_DBL_O_CUT,			"外径切り込み(半径値)"},
	{NC_DBL,	MKLA_DBL_O_PULLZ,		"外径引き代(Z)"},
	{NC_DBL,	MKLA_DBL_O_PULLX,		"外径引き代(X半径値)"},
	{NC_DBL,	MKLA_DBL_O_MARGIN,		"外径仕上げ代(半径値)"},
	{NC_NUM,	MKLA_NUM_O_MARGIN,		"外径仕上げ回数"},
};

/////////////////////////////////////////////////////////////////////////////
// CNCMakeLatheOpt クラスの構築/消滅

CNCMakeLatheOpt::CNCMakeLatheOpt(LPCTSTR lpszInit) :
	CNCMakeOption(LthOption,
		SIZEOF(g_szInitComment), g_szInitComment,
		SIZEOF(g_stSaveOrder),   g_stSaveOrder)
{
	ASSERT( MKLA_NUM_NUMS == SIZEOF(g_szLthNOrder) );
	ASSERT( MKLA_NUM_NUMS == SIZEOF(g_dfLthNOrder) );
	ASSERT( MKLA_DBL_NUMS == SIZEOF(g_szLthDOrder) );
	ASSERT( MKLA_DBL_NUMS == SIZEOF(g_dfLthDOrder) );
	ASSERT( MKLA_FLG_NUMS == SIZEOF(g_szLthBOrder) );
	ASSERT( MKLA_FLG_NUMS == SIZEOF(g_dfLthBOrder) );
	ASSERT( MKLA_STR_NUMS == SIZEOF(g_szLthSOrder) );
	ASSERT( MKLA_STR_NUMS == SIZEOF(g_dfLthSOrder) );

	ReadMakeOption(lpszInit);
}

/////////////////////////////////////////////////////////////////////////////
// ﾕｰｻﾞﾒﾝﾊﾞ関数

void CNCMakeLatheOpt::InitialDefault(void)
{
	extern	LPTSTR	g_pszExecDir;	// 実行ﾃﾞｨﾚｸﾄﾘ(NCVC.cpp)
	int		i;

	for ( i=0; i<MKLA_NUM_NUMS; i++ )
		m_pIntOpt[i] = g_dfLthNOrder[i];
	for ( i=0; i<MKLA_DBL_NUMS; i++ )
		m_pDblOpt[i] = g_dfLthDOrder[i];
	for ( i=0; i<MKLA_FLG_NUMS; i++ )
		m_pFlgOpt[i] = g_dfLthBOrder[i];
	m_strOption.RemoveAll();
	for ( i=0; i<MKLA_STR_NUMS; i++ )
		m_strOption.Add(g_dfLthSOrder[i]);

	for ( i=MKLA_STR_HEADER; i<=MKLA_STR_FOOTER; i++ )
		m_strOption[i] = g_pszExecDir + m_strOption[i];
}

BOOL CNCMakeLatheOpt::IsPathID(int n)
{
	return ( n==MKLA_STR_HEADER || n==MKLA_STR_FOOTER );
}

CString CNCMakeLatheOpt::GetLineNoForm(void) const
{
	return LTH_S_LINEFORM;
}

BOOL CNCMakeLatheOpt::GetDrillInfo(VLATHEDRILLINFO& v) const
{
	if ( LTH_S_DRILL.IsEmpty() )
		return FALSE;

	v.clear();
	extern	LPCTSTR	gg_szComma;
	std::string		strDrill(LTH_S_DRILL), strDrillTok,
					strSpindle(LTH_S_DRILLSPINDLE), strResult,
					strFeed(LTH_S_DRILLFEED);
	boost::char_separator<TCHAR>	sep(gg_szComma);
	typedef	boost::tokenizer< boost::char_separator<TCHAR> >	TOKEN;
	TOKEN		tok1(strDrill, sep),
				tok2(strSpindle, sep),
				tok3(strFeed, sep);
	TOKEN::iterator	it2 = tok2.begin(), it3 = tok3.begin();
	LATHEDRILLINFO	info;

	BOOST_FOREACH(strDrillTok, tok1) {
		info.s = it2 != tok2.end() ? atoi((it2++)->c_str()) : 0;
		info.f = it3 != tok3.end() ? (float)atof((it3++)->c_str()) : 0;
		boost::trim(strDrillTok);
		if ( !strDrillTok.empty() ) {
			float f = (float)atof(strDrillTok.c_str());
			if ( f > 0 ) {
				info.d = f;
				v.push_back(info);	// ﾄﾞﾘﾙが有効なときだけ登録
			}
		}
	}

	return !v.empty();
}

#ifdef _DEBUG
void CNCMakeLatheOpt::DbgDump(void) const
{
	CMagaDbg	dbg("CNCMakeLatheOpt", DBG_RED);

	dbg.printf("InitFile=%s", GetInitFile());
	dbg.printf("----------");
	dbg.printf("  Spindle      =%d", LTH_I_O_SPINDLE);
	dbg.printf("  Feed         =%f", LTH_D_O_FEED);
	dbg.printf("  XFeed        =%f", LTH_D_O_FEEDX);
	dbg.printf("  Cut          =%f", LTH_D_O_CUT);
	dbg.printf("  PullZ        =%f", LTH_D_O_PULLZ);
	dbg.printf("  PullX        =%f", LTH_D_O_PULLX);
	dbg.printf("  Margin       =%f", LTH_D_O_MARGIN);
	dbg.printf("  Header       =%s", LTH_S_HEADER);
	dbg.printf("  Footer       =%s", LTH_S_FOOTER);
	dbg.printf("----------");
	dbg.printf("  bLineAdd     =%d", LTH_F_LINEADD);
	dbg.printf("  LineForm     =%s", LTH_S_LINEFORM);
	dbg.printf("  nLineAdd     =%d", LTH_I_LINEADD);
	dbg.printf("  EOB          =%s", LTH_S_EOB);
	dbg.printf("  G90          =%d", LTH_I_G90);
	dbg.printf("  Gclip        =%d", LTH_F_GCLIP);
	dbg.printf("  DisSpindle   =%d", LTH_F_DISABLESPINDLE);
	dbg.printf("----------");
	dbg.printf("  Dot          =%d", LTH_I_DOT);
	dbg.printf("  FDot         =%d", LTH_I_FDOT);
	dbg.printf("  ZeroCut      =%d", LTH_F_ZEROCUT);
	dbg.printf("  CircleCode   =%d", LTH_I_CIRCLECODE);
	dbg.printf("  IJ           =%d", LTH_I_IJ);
	dbg.printf("  CircleHalf   =%d", LTH_F_CIRCLEHALF);
	dbg.printf("  ZeroCutIJ    =%d", LTH_F_ZEROCUT_IJ);
	dbg.printf("  Ellipse      =%f", LTH_D_ELLIPSE);
	dbg.printf("  EllipseFlg   =%d", LTH_F_ELLIPSE);
	dbg.printf("----------");
	dbg.printf("  Drill        =%s", LTH_S_DRILL);
	dbg.printf("  DrillSpindle =%s", LTH_S_DRILLSPINDLE);
	dbg.printf("  DrillFeed    =%s", LTH_S_DRILLFEED);
	dbg.printf("  DrillZ       =%f", LTH_D_DRILLZ);
	dbg.printf("  DrillQ       =%f", LTH_D_DRILLQ);
	dbg.printf("  Dwell        =%f", LTH_D_DWELL);
	dbg.printf("  Hole         =%f", LTH_D_HOLE);
	dbg.printf("  Cycle        =%d", LTH_F_CYCLE);
	dbg.printf("----------");
	dbg.printf("  InSpindle    =%d", LTH_I_I_SPINDLE);
	dbg.printf("  InFeed       =%f", LTH_D_I_FEED);
	dbg.printf("  InFeedX      =%f", LTH_D_I_FEEDX);
	dbg.printf("  InCut        =%f", LTH_D_I_CUT);
	dbg.printf("  InPullZ      =%f", LTH_D_I_PULLZ);
	dbg.printf("  InPullX      =%f", LTH_D_I_PULLX);
	dbg.printf("  InMargin     =%f", LTH_D_I_MARGIN);
}
#endif
