// NCMakeLatheOpt.cpp: CNCMakeLatheOpt クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "NCMakeLatheOpt.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// int型命令
static	LPCTSTR	g_szLthNOrder[] = {
	"ProgNo", "LineAddType",
	"G90", "Dot", "FDot", "CircleCode", "CircleIJ",
	"EndFaceSpindle", "InsideSpindle", "OutsideSpindle", "GrooveSpindle",
	"InsideMarginNum", "MarginNum",
	"GrooveTool"
};
static	const	int		g_dfLthNOrder[] = {
	1, 1,
	0, 0, 0, 0, 0,
	200, 200, 200, 50,
	1, 1,
	0
};

// float型命令
static	LPCTSTR	g_szLthDOrder[] = {
	"Feed", "XFeed", "Cut", "PullZ", "PullX", "Margin",
	"Ellipse",
	"EndFaceFeed", "EndFaceCut", "EndFaceStep", "EndFacePullZ", "EndFacePullX",
	"DrillZ", "DrillR", "DrillQ", "DrillD", "Dwell", "PilotHole",
	"InsideFeed", "InsideFeedX", "InsideCut",
		"InsidePullZ", "InsidePullX", "InsideMargin",
	"GrooveFeed", "GrooveFeedX", "GroovePullX", "GrooveDwell", "GrooveWidth"
};
static	const	float	g_dfLthDOrder[] = {
	300.0f, 150.0f, 1.0f, 2.0f, 2.0f, 1.0f,
	0.5f,
	150.0f, -5.0f, -1.0f, 2.0f, 2.0f,
	-50.0f, 10.0f, 15.0f, 10.0f, 0.0f, 0.0f,
	300.0f, 150.0f, 0.5f, 2.0f, 2.0f, 1.0f,
	0.1f, 0.1f, 2.0f, 1000.0f, 3.0f
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
	"EndFaceCustom", "InsideCustom", "OutsideCustom", "GrooveCustom"
};
static	LPCTSTR	g_dfLthSOrder[] = {
	"N%04d", "",
	"HeaderLathe.txt", "FooterLathe.txt",
	"", "", "", "(Enter drill-tool change code etc)",
	"\\n(TANMEN START)\\n(Enter TANMEN-tool change code etc)",
	"\\n(NAIKEI START)\\n(Enter NAIKEI-tool change code etc)",
	"\\n(GAIKEI START)\\n(Enter GAIKEI-tool change code etc)",
	"\\n(Grooving START)\\n(Enter Groove-tool change code etc)"
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
	{NC_DBL,	MKLA_DBL_D_DWELL,		"下穴ﾄﾞｳｪﾙ時間[msec]"},
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
	{NC_PAGE,	8},		// Page8(突っ切り)
	{NC_STR,	MKLA_STR_G_CUSTOM,		"突っ切りｶｽﾀﾑｺｰﾄﾞ"},
	{NC_NUM,	MKLA_NUM_G_SPINDLE,		"突っ切り主軸回転数"},
	{NC_DBL,	MKLA_DBL_G_FEED,		"突っ切り切削送り(Z)"},
	{NC_DBL,	MKLA_DBL_G_FEEDX,		"突っ切り切削送り(X)"},
	{NC_DBL,	MKLA_DBL_G_PULLX,		"突っ切り引き代(X半径値)"},
	{NC_DBL,	MKLA_DBL_G_DWELL,		"突っ切りﾄﾞｳｪﾙ時間[msec]"},
	{NC_DBL,	MKLA_DBL_GROOVEWIDTH,	"突っ切り刃幅"},
	{NC_NUM,	MKLA_NUM_GROOVETOOL,	"突っ切り工具基準点(0:左,1:中央,2:右)"},
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
	printf("CNCMakeLatheOpt InitFile=%s\n", LPCTSTR(GetInitFile()));
	printf("----------\n");
	printf("  Spindle      =%d\n", LTH_I_O_SPINDLE);
	printf("  Feed         =%f\n", LTH_D_O_FEED);
	printf("  XFeed        =%f\n", LTH_D_O_FEEDX);
	printf("  Cut          =%f\n", LTH_D_O_CUT);
	printf("  PullZ        =%f\n", LTH_D_O_PULLZ);
	printf("  PullX        =%f\n", LTH_D_O_PULLX);
	printf("  Margin       =%f\n", LTH_D_O_MARGIN);
	printf("  Header       =%s\n", LPCTSTR(LTH_S_HEADER));
	printf("  Footer       =%s\n", LPCTSTR(LTH_S_FOOTER));
	printf("----------\n");
	printf("  bLineAdd     =%d\n", LTH_F_LINEADD);
	printf("  LineForm     =%s\n", LPCTSTR(LTH_S_LINEFORM));
	printf("  nLineAdd     =%d\n", LTH_I_LINEADD);
	printf("  EOB          =%s\n", LPCTSTR(LTH_S_EOB));
	printf("  G90          =%d\n", LTH_I_G90);
	printf("  Gclip        =%d\n", LTH_F_GCLIP);
	printf("  DisSpindle   =%d\n", LTH_F_DISABLESPINDLE);
	printf("----------\n");
	printf("  Dot          =%d\n", LTH_I_DOT);
	printf("  FDot         =%d\n", LTH_I_FDOT);
	printf("  ZeroCut      =%d\n", LTH_F_ZEROCUT);
	printf("  CircleCode   =%d\n", LTH_I_CIRCLECODE);
	printf("  IJ           =%d\n", LTH_I_IJ);
	printf("  CircleHalf   =%d\n", LTH_F_CIRCLEHALF);
	printf("  ZeroCutIJ    =%d\n", LTH_F_ZEROCUT_IJ);
	printf("  Ellipse      =%f\n", LTH_D_ELLIPSE);
	printf("  EllipseFlg   =%d\n", LTH_F_ELLIPSE);
	printf("----------\n");
	printf("  Drill        =%s\n", LPCTSTR(LTH_S_DRILL));
	printf("  DrillSpindle =%s\n", LPCTSTR(LTH_S_DRILLSPINDLE));
	printf("  DrillFeed    =%s\n", LPCTSTR(LTH_S_DRILLFEED));
	printf("  DrillZ       =%f\n", LTH_D_DRILLZ);
	printf("  DrillQ       =%f\n", LTH_D_DRILLQ);
	printf("  Dwell        =%f\n", LTH_D_DWELL);
	printf("  Hole         =%f\n", LTH_D_HOLE);
	printf("  Cycle        =%d\n", LTH_F_CYCLE);
	printf("----------\n");
	printf("  InSpindle    =%d\n", LTH_I_I_SPINDLE);
	printf("  InFeed       =%f\n", LTH_D_I_FEED);
	printf("  InFeedX      =%f\n", LTH_D_I_FEEDX);
	printf("  InCut        =%f\n", LTH_D_I_CUT);
	printf("  InPullZ      =%f\n", LTH_D_I_PULLZ);
	printf("  InPullX      =%f\n", LTH_D_I_PULLX);
	printf("  InMargin     =%f\n", LTH_D_I_MARGIN);
	printf("----------\n");
	printf("  GrooveSpindle=%d\n", LTH_I_G_SPINDLE);
	printf("  GrooveWidth  =%f\n", LTH_D_GROOVEWIDTH);
	printf("  GrooveTool   =%d\n", LTH_I_GROOVETOOL);
}
#endif
