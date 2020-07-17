// NCMakeMillOpt.cpp: CNCMakeMillOpt クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "NCMakeMillOpt.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif
//#define	_DEBUGOLD
#undef	_DEBUGOLD

// int型命令
static	LPCTSTR	g_szNOrder[] = {
	"ProgNo", "LineAddType", "G90", "Dot", "FDot", "CircleCode",
	"Spindle",
		"ZReturn", "CircleIJ",
	"MakeEnd", "FinishSpindle",
		"DeepZProcess", "DeepAProcess", "DeepCProcess",
	"DrillSpindle", "Dwell", "DwellFormat", "DrillZProcess",
		"DrillProcess", "DrillSort", "DrillCircleProcess",
	"MoveZ",
	"ToleranceProcess", "DrillOptimaize"
};
static	const	int		g_dfNOrder[] = {
	1, 1, 0, 0, 2, 0,
	3000,
		1, 1,
	0, 8000,
		0, 0, 0,
	3000, 10, 0, 0,
		0, 0, 0,
	0,
	0, 0
};

// double型命令
static	LPCTSTR	g_szDOrder[] = {
	"Feed", "ZFeed",
	"ZG0Stop", "ZCut", "G92X", "G92Y", "G92Z",
	"Ellipse",
	"MakeEndValue", "MakeEndFeed", "DeepFinal", "ZStep", "FinishFeed",
	"DrillFeed", "DrillR", "DrillZ", "DrillCircleR",
	"Tolerance", "DrillMargin"
};
static	const	double	g_dfDOrder[] = {
	300.0, 100.0,
	1.0, -12.0, 0.0, 0.0, 10.0,
	0.5,
	0.0, 1000.0, -20.0, -2.0, 100.0,
	60.0, -9.0, -12.0, 10.0,
	NCMIN, 1.0
};

// BOOL型命令
static	LPCTSTR	g_szBOrder[] = {
	"ProgSet", "ProgAuto", "LineAdd", "ZeroCut", "GClip", "EllipseFlg",
	// --
	"XRev", "YRev", "DisableSpindle",
	"CircleHalf", "ZeroCutIJ",
	"Deep", "Helical", "DeepFinishSet",
	"DrillMatch", "DrillCircle", "DrillBreak",
	"LayerComment", "L0Cycle"
};
static	const	BOOL	g_dfBOrder[] = {
	TRUE, FALSE, FALSE, TRUE, TRUE, TRUE,
	FALSE, FALSE, FALSE,
	FALSE, TRUE,
	FALSE, TRUE, FALSE,
	TRUE, FALSE, TRUE,
	TRUE, FALSE
};

// CString型命令
static	LPCTSTR	g_szSOrder[] = {
	"LineForm", "EOB", "Header", "Footer",
	"CustomMoveB", "CustomMoveA"
};
static	LPCTSTR	g_dfSOrder[] = {
	"N%04d", "", "Header.txt", "Footer.txt",
	"", ""
};

// 保存に関する情報
static	LPCTSTR	g_szInitComment[] = {
	"##\n",
	"## NCVC:切削条件ﾌｧｲﾙ\n"
};
static	SAVEORDER	g_stSaveOrder[] = {
	{NC_PAGE,	1},		// Page1(基本:Dialog1)
	{NC_NUM,	MKNC_NUM_SPINDLE,		"主軸回転数"},
	{NC_DBL,	MKNC_DBL_FEED,			"切削送り"},
	{NC_DBL,	MKNC_DBL_ZFEED,			"Z軸送り"},
	{NC_DBL,	MKNC_DBL_ZG0STOP,		"R点"},
	{NC_DBL,	MKNC_DBL_ZCUT,			"Z軸切り込み"},
	{NC_DBL,	MKNC_DBL_G92X,			"切削原点(G92X)"},
	{NC_DBL,	MKNC_DBL_G92Y,			"切削原点(G92Y)"},
	{NC_DBL,	MKNC_DBL_G92Z,			"切削原点(G92Z)"},
	{NC_FLG,	MKNC_FLG_XREV,			"X軸反転"},
	{NC_FLG,	MKNC_FLG_YREV,			"Y軸反転"},
	{NC_STR,	MKNC_STR_HEADER,		"ｶｽﾀﾑﾍｯﾀﾞｰ"},
	{NC_STR,	MKNC_STR_FOOTER,		"ｶｽﾀﾑﾌｯﾀﾞｰ"},
	{NC_PAGE,	2},		// Page2(生成:Dialog2)
	{NC_FLG,	MKNC_FLG_PROG,			"Ｏ番号生成"},
	{NC_NUM,	MKNC_NUM_PROG,			"ﾌﾟﾛｸﾞﾗﾑ番号"},
	{NC_FLG,	MKNC_FLG_PROGAUTO,		"おまかせ番号"},
	{NC_FLG,	MKNC_FLG_LINEADD,		"行番号追加"},
	{NC_STR,	MKNC_STR_LINEFORM,		"行番号書式"},
	{NC_NUM,	MKNC_NUM_LINEADD,		"行番号倍率"},
	{NC_STR,	MKNC_STR_EOB,			"EOB"},
	{NC_NUM,	MKNC_NUM_G90,			"位置指令(0:G90,1:G91)"},
	{NC_NUM,	MKNC_NUM_ZRETURN,		"Z軸復帰(0:ｲﾆｼｬﾙ,1:R点)"},
	{NC_FLG,	MKNC_FLG_GCLIP,			"ﾓｰﾀﾞﾙ"},
	{NC_FLG,	MKNC_FLG_DISABLESPINDLE,"Sﾊﾟﾗﾒｰﾀを生成しない"},
	{NC_PAGE,	3},		// Page3(表記:Dialog6)
	{NC_NUM,	MKNC_NUM_DOT,			"座標表記(0:小数点,1:1/1000)"},
	{NC_NUM,	MKNC_NUM_FDOT,			"Fﾊﾟﾗﾒｰﾀ表記(0:小数点,1:1/1000,2:整数)"},
	{NC_FLG,	MKNC_FLG_ZEROCUT,		"小数点以下のｾﾞﾛｶｯﾄ"},
	{NC_NUM,	MKNC_NUM_CIRCLECODE,	"円ﾃﾞｰﾀの切削(0:G02,1:G03)"},
	{NC_NUM,	MKNC_NUM_IJ,			"円弧指示(0:R,1:I/J)"},
	{NC_FLG,	MKNC_FLG_CIRCLEHALF,	"全円は2分割"},
	{NC_FLG,	MKNC_FLG_ZEROCUT_IJ,	"[I|J]0は省略"},
	{NC_DBL,	MKNC_DBL_ELLIPSE,		"楕円公差"},
	{NC_FLG,	MKNC_FLG_ELLIPSE,		"長径と短径が等しい楕円は円とみなす"},
	{NC_PAGE,	4},		// Page4(深彫:Dialog3)
	{NC_NUM,	MKNC_NUM_MAKEEND,		"加工済み深さの指示(0:なし,1:ｵﾌｾｯﾄ,2:固定Z)"},
	{NC_DBL,	MKNC_DBL_MAKEEND,		"ｵﾌｾｯﾄ値 or 固定Z値"},
	{NC_DBL,	MKNC_DBL_MAKEENDFEED,	"加工済み深さのZ送り速度"},
	{NC_FLG,	MKNC_FLG_DEEP,			"深彫を行う"},
	{NC_DBL,	MKNC_DBL_DEEP,			"最終切り込み"},
	{NC_DBL,	MKNC_DBL_ZSTEP,			"Z軸切り込みｽﾃｯﾌﾟ"},
	{NC_NUM,	MKNC_NUM_DEEPALL,		"深彫切削手順(0:全体,1:一筆)"},
	{NC_NUM,	MKNC_NUM_DEEPROUND,		"深彫切削方向(0:往復,1:一方)"},
	{NC_NUM,	MKNC_NUM_DEEPRETURN,	"R点へのZ軸復帰(0:早送り,1:切削送り)"},
	{NC_FLG,	MKNC_FLG_HELICAL,		"円ﾃﾞｰﾀをﾍﾘｶﾙ切削"},
	{NC_FLG,	MKNC_FLG_DEEPFINISH,	"最終Z値仕上げ適用"},
	{NC_NUM,	MKNC_NUM_DEEPSPINDLE,	"仕上げ回転数"},
	{NC_DBL,	MKNC_DBL_DEEPFEED,		"仕上げ送り"},
	{NC_PAGE,	5},		// Page5(穴加工:Dialog4)
	{NC_NUM,	MKNC_NUM_DRILLSPINDLE,	"穴加工回転数"},
	{NC_DBL,	MKNC_DBL_DRILLFEED,		"穴加工送り"},
	{NC_DBL,	MKNC_DBL_DRILLR,		"穴加工R点"},
	{NC_DBL,	MKNC_DBL_DRILLZ,		"穴加工切り込み"},
	{NC_FLG,	MKNC_FLG_DRILLMATCH,	"穴加工同一座標無視"},
	{NC_NUM,	MKNC_NUM_DWELL,			"ﾄﾞｳｪﾙ時間"},
	{NC_NUM,	MKNC_NUM_DWELLFORMAT,	"ﾄﾞｳｪﾙ時間表記(0:小数点,1:整数)"},
	{NC_NUM,	MKNC_NUM_DRILLPROCESS,	"穴加工手順(0:先,1:後,2:のみ)"},
	{NC_NUM,	MKNC_NUM_DRILLRETURN,	"Z軸復帰(0:G81|G82,1:G85|G89)"},
	{NC_FLG,	MKNC_FLG_DRILLCIRCLE,	"円ﾃﾞｰﾀも穴加工"},
	{NC_DBL,	MKNC_DBL_DRILLCIRCLE,	"対象半径"},
	{NC_NUM,	MKNC_NUM_DRILLSORT,		"ｸﾞﾙｰﾋﾟﾝｸﾞ(0:昇順,1:降順)"},
	{NC_FLG,	MKNC_FLG_DRILLBREAK,	"大きさごとにｺﾒﾝﾄを埋め込む"},
	{NC_NUM,	MKNC_NUM_DRILLCIRCLEPROCESS,	"実点と円ﾃﾞｰﾀとの手順(0:無視,1:先,2:後)"},
	{NC_PAGE,	6},		// Page6(ﾚｲﾔ:Dialog8)
	{NC_FLG,	MKNC_FLG_LAYERCOMMENT,	"ﾚｲﾔごとにｺﾒﾝﾄを埋め込む"},
	{NC_NUM,	MKNC_NUM_MOVEZ,			"移動ﾚｲﾔのZ軸(0:そのまま,1:R点,2:ｲﾆｼｬﾙ点)"},
	{NC_STR,	MKNC_STR_CUSTMOVE_B,	"ｶｽﾀﾑ移動ｺｰﾄﾞ(前)"},
	{NC_STR,	MKNC_STR_CUSTMOVE_A,	"ｶｽﾀﾑ移動ｺｰﾄﾞ(後)"},
	{NC_FLG,	MKNC_FLG_L0CYCLE,		"固定ｻｲｸﾙ中はL0出力"},
	{NC_PAGE,	7},		// Page7(最適化:Dialog5)
	{NC_DBL,	MKNC_DBL_TOLERANCE,		"同一座標と見なす許容差"},
	{NC_NUM,	MKNC_NUM_TOLERANCE,		"超えた時の動作(0:Z上昇G00移動,1:G01補間)"},
	{NC_NUM,	MKNC_NUM_OPTIMAIZEDRILL,"穴加工基準軸(0:なし,1:X,2:Y)"},
	{NC_DBL,	MKNC_DBL_DRILLMARGIN,	"穴加工同一軸上と見なす許容差"}
};

/////////////////////////////////////////////////////////////////////////////
// CNCMakeMillOpt クラスの構築/消滅

CNCMakeMillOpt::CNCMakeMillOpt(LPCTSTR lpszInit) :
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

	// 下位互換のための処理
	BOOL bResult = Convert();
	// 下位互換ｵﾌﾟｼｮﾝに上書き
	ReadMakeOption(lpszInit);
	// 移行完了
	if ( bResult && lpszInit )
		SaveMakeOption();
}

/////////////////////////////////////////////////////////////////////////////
// ﾕｰｻﾞﾒﾝﾊﾞ関数

// ﾚｼﾞｽﾄﾘからの移行
BOOL CNCMakeMillOpt::Convert()
{
	extern	LPCTSTR	gg_szRegKey;

	// NC生成ｵﾌﾟｼｮﾝ(旧ﾊﾞｰｼﾞｮﾝ互換用)
	static	const	int		nRegDxfNums[] = {
		IDS_REG_DXF_LINEADD, IDS_REG_DXF_G90, IDS_REG_DXF_DOTP
	};
	static	const	int		nRegDxfNumsDef[] = {
		1, 0, 0
	};
	static	const	int		nRegDxfNumsID[] = {
		MKNC_NUM_LINEADD, MKNC_NUM_G90, MKNC_NUM_DOT
	};

	static	const	int		nRegDxfFlags[] = {
		IDS_REG_DXF_XREV,
		IDS_REG_DXF_LINE, IDS_REG_DXF_ZERO, IDS_REG_DXF_CUT
	};
	static	const	int		nRegDxfFlagsDef[] = {
		FALSE,
		FALSE, TRUE, TRUE
	};
	static	const	int		nRegDxfFlagsID[] = {
		MKNC_FLG_XREV,
		MKNC_FLG_LINEADD, MKNC_FLG_ZEROCUT, MKNC_FLG_GCLIP
	};

	CString	strRegKey, strEntry;
	// 移行完了ﾁｪｯｸ
	VERIFY(strRegKey.LoadString(IDS_REGKEY_DXF));
	VERIFY(strEntry.LoadString(IDS_REG_CONVERT));
	if ( AfxGetApp()->GetProfileInt(strRegKey, strEntry, 0) > 0 )
		return FALSE;

	// ﾃﾞｰﾀの移行
	AfxGetApp()->WriteProfileInt(strRegKey, strEntry, 1);
	int	i, nID;
	for ( i=0; i<SIZEOF(nRegDxfNums); i++ ) {
		VERIFY(strEntry.LoadString(nRegDxfNums[i]));
		nID = nRegDxfNumsID[i];
		m_unNums[nID] = AfxGetApp()->GetProfileInt(strRegKey, strEntry, nRegDxfNumsDef[i]);
	}
	for ( i=0; i<SIZEOF(nRegDxfFlags); i++ ) {
		VERIFY(strEntry.LoadString(nRegDxfFlags[i]));
		nID = nRegDxfFlagsID[i];
		m_ubFlags[nID] = (BOOL)(AfxGetApp()->GetProfileInt(strRegKey, strEntry, nRegDxfFlagsDef[i]));
	}
	VERIFY(strEntry.LoadString(IDS_REG_DXF_CIRCLEIJ));
	m_unNums[MKNC_NUM_IJ] = AfxGetApp()->GetProfileInt(strRegKey, strEntry, 1);
	VERIFY(strEntry.LoadString(IDS_REG_DXF_LINEFORM));
	m_strOption[MKNC_STR_LINEFORM] = AfxGetApp()->GetProfileString(strRegKey, strEntry);

	// 旧ﾚｼﾞｽﾄﾘの消去
	CRegKey	reg;

	// --- "Software\MNCT-S\NCVC\DXF"
	if ( reg.Open(HKEY_CURRENT_USER, gg_szRegKey+strRegKey) != ERROR_SUCCESS )
		return FALSE;

	for ( i=0; i<SIZEOF(nRegDxfNums); i++ ) {
		VERIFY(strEntry.LoadString(nRegDxfNums[i]));
		reg.DeleteValue(strEntry);
	}
	for ( i=0; i<SIZEOF(nRegDxfFlags); i++ ) {
		VERIFY(strEntry.LoadString(nRegDxfFlags[i]));
		reg.DeleteValue(strEntry);
	}
	VERIFY(strEntry.LoadString(IDS_REG_DXF_CIRCLEIJ));
	reg.DeleteValue(strEntry);
	VERIFY(strEntry.LoadString(IDS_REG_DXF_LINEFORM));
	reg.DeleteValue(strEntry);

	// Init条件ﾌｧｲﾙ名の削除
	VERIFY(strEntry.LoadString(IDS_REG_DXF_INIT));
	reg.DeleteValue(strEntry);

	return TRUE;
}

#ifdef _DEBUGOLD
void CNCMakeMillOpt::DbgDump(void) const
{
	CMagaDbg	dbg("CNCMakeMillOpt", DBG_RED);

	dbg.printf("InitFile=%s", GetInitFile());
	dbg.printf("----------");
	dbg.printf("  Spindle      =%d", m_nSpindle);
	dbg.printf("  Feed         =%f", m_dFeed);
	dbg.printf("  ZFeed        =%f", m_dZFeed);
	dbg.printf("  ZG0Stop      =%f", m_dZG0Stop);
	dbg.printf("  ZCut         =%f", m_dZCut);
	dbg.printf("  G92[X]       =%f", m_dG92[NCA_X]);
	dbg.printf("  G92[Y]       =%f", m_dG92[NCA_Y]);
	dbg.printf("  G92[Z]       =%f", m_dG92[NCA_Z]);
	dbg.printf("  Header       =%s", m_strOption[MKNC_STR_HEADER]);
	dbg.printf("  Footer       =%s", m_strOption[MKNC_STR_FOOTER]);
	dbg.printf("----------");
	dbg.printf("  Xrev         =%d", m_bXrev);
	dbg.printf("  Yrev         =%d", m_bYrev);
	dbg.printf("  bLineAdd     =%d", m_bLineAdd);
	dbg.printf("  LineForm     =%s", m_strOption[MKNC_STR_LINEFORM]);
	dbg.printf("  nLineAdd     =%d", m_nLineAdd);
	dbg.printf("  EOB          =%s", m_strOption[MKNC_STR_EOB]);
	dbg.printf("  G90          =%d", m_nG90);
	dbg.printf("  ZReturn      =%d", m_nZReturn);
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
	dbg.printf("----------");
	dbg.printf("  MakeEnd      =%d", m_nMakeEnd);
	dbg.printf("  MakeValue    =%f", m_dMakeValue);
	dbg.printf("  MakeFeed     =%f", m_dMakeFeed);
	dbg.printf("  Deep         =%d", m_bDeep);
	dbg.printf("  DeepFinal    =%f", m_dDeep);
	dbg.printf("  ZStep        =%f", m_dZStep);
	dbg.printf("  DeepZProcess =%d", m_nDeepReturn);
	dbg.printf("  DeepAProcess =%d", m_nDeepAll);
	dbg.printf("  DeepCProcess =%d", m_nDeepRound);
	dbg.printf("  Helical      =%d", m_bHelical);
	dbg.printf("  DeepFinish   =%d", m_bDeepFinish);
	dbg.printf("  DeepSpindle  =%d", m_nDeepSpindle);
	dbg.printf("  DeepFeed     =%f", m_dDeepFeed);
	dbg.printf("----------");
	dbg.printf("  DrillSpindle =%d", m_nDrillSpindle);
	dbg.printf("  DrillFeed    =%f", m_dDrillFeed);
	dbg.printf("  DrillR       =%f", m_dDrillR);
	dbg.printf("  DrillZ       =%f", m_dDrillZ);
	dbg.printf("  DrillMatch   =%d", m_bDrillMatch);
	dbg.printf("  Dwell        =%d", m_nDwell);
	dbg.printf("  DwellFormat  =%d", m_nDwellFormat);
	dbg.printf("  DrillProcess =%d", m_nDrillProcess);
	dbg.printf("  DrillZProcess=%d", m_nDrillReturn);
	dbg.printf("----------");
	dbg.printf("  DrillCircle  =%d", m_bDrillCircle);
	dbg.printf("  DrillCircleR =%f", m_dDrillCircle);
	dbg.printf("  DrillSort    =%d", m_nDrillSort);
	dbg.printf("  DrillCProcess=%d", m_nDrillCircleProcess);
	dbg.printf("  DrillBreak   =%d", m_bDrillBreak);
	dbg.printf("----------");
	dbg.printf("  LayerComment =%d", m_bLayerComment);
	dbg.printf("  MoveZ        =%d", m_nMoveZ);
	dbg.printf("  CustMoveB    =%s", m_strOption[MKNC_STR_CUSTMOVE_B]);
	dbg.printf("  CustMoveA    =%s", m_strOption[MKNC_STR_CUSTMOVE_A]);
	dbg.printf("  L0Cycle      =%d", m_bL0Cycle);
	dbg.printf("----------");
	dbg.printf("  Tolerance    =%f", m_dTolerance);
	dbg.printf("  TolerancePro =%d", m_nTolerance);
	dbg.printf("  DrillOptimaiz=%d", m_nOptimaizeDrill);
	dbg.printf("  DrillMargin  =%f", m_dDrillMargin);
}
#endif
