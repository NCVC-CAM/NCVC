// NCMakeMillOpt.cpp: CNCMakeMillOpt クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "NCMakeMillOpt.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// int型命令
static	LPCTSTR	g_szMilNOrder[] = {
	"ProgNo", "LineAddType", "G90", "Dot", "FDot", "CircleCode",
	"Spindle",
		"ZReturn", "CircleIJ",
	"MakeEnd", "FinishSpindle",
		"DeepZProcess", "DeepAProcess", "DeepCProcess",
	"DrillSpindle", "DwellFormat", "DrillZProcess",
		"DrillProcess", "DrillSort", "DrillCircleProcess",
	"MoveZ",
	"ToleranceProcess", "DrillOptimaize"
};
static	const	int		g_dfMilNOrder[] = {
	1, 1, 0, 0, 2, 0,
	3000,
		1, 1,
	0, 8000,
		0, 0, 0,
	3000, 1, 0,
		0, 0, 0,
	1,
	0, 0
};

// float型命令
static	LPCTSTR	g_szMilDOrder[] = {
	"Feed", "ZFeed",
	"ZG0Stop", "ZCut", "G92X", "G92Y", "G92Z",
	"Ellipse",
	"MakeEndValue", "MakeEndFeed", "DeepFinal", "ZStep", "FinishFeed",
	"DrillFeed", "DrillR", "DrillZ", "DrillQ", "Dwell", "DrillCircleR",
	"Tolerance", "DrillMargin"
};
static	const	float	g_dfMilDOrder[] = {
	300.0f, 100.0f,
	1.0f, -12.0f, 0.0f, 0.0f, 10.0f,
	0.5f,
	0.0f, 1000.0f, -20.0f, -2.0f, 100.0f,
	60.0f, -9.0f, -12.0f, 1.0f, 1.0f, 10.0f,
	NCMIN, 1.0f
};

// BOOL型命令
static	LPCTSTR	g_szMilBOrder[] = {
	"ProgSet", "ProgAuto", "LineAdd", "ZeroCut", "GClip", "EllipseFlg",
	// --
	"XRev", "YRev", "DisableSpindle",
	"CircleHalf", "ZeroCutIJ",
	"Deep", "Helical", "DeepFinishSet",
	"DrillMatch", "DrillCircle", "DrillBreak",
	"LayerComment", "L0Cycle"
};
static	const	BOOL	g_dfMilBOrder[] = {
	TRUE, FALSE, FALSE, TRUE, TRUE, TRUE,
	FALSE, FALSE, FALSE,
	FALSE, TRUE,
	FALSE, TRUE, FALSE,
	TRUE, FALSE, TRUE,
	TRUE, FALSE
};

// CString型命令
static	LPCTSTR	g_szMilSOrder[] = {
	"LineForm", "EOB", "Header", "Footer",
	"CustomMoveB", "CustomMoveA",
	"PerlScript"
};
static	LPCTSTR	g_dfMilSOrder[] = {
	"N%04d", "", "Header.txt", "Footer.txt",
	"", "",
	""
};

// ｵﾌﾟｼｮﾝ統合
static	NCMAKEOPTION	MilOption[] = {
	{MKNC_NUM_NUMS, g_szMilNOrder},
	{MKNC_DBL_NUMS, g_szMilDOrder},
	{MKNC_FLG_NUMS, g_szMilBOrder},
	{MKNC_STR_NUMS, g_szMilSOrder}
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
	{NC_NUM,	MKNC_NUM_DOT,			"座標表記(0:小数3位,1:小数4位,2:1/1000)"},
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
	{NC_DBL,	MKNC_DBL_DWELL,			"ﾄﾞｳｪﾙ時間"},
	{NC_NUM,	MKNC_NUM_DWELLFORMAT,	"ﾄﾞｳｪﾙ時間表記(0:小数点,1:整数)"},
	{NC_NUM,	MKNC_NUM_DRILLPROCESS,	"穴加工手順(0:先,1:後,2:のみ)"},
	{NC_NUM,	MKNC_NUM_DRILLRETURN,	"加工ﾀｲﾌﾟ(0:G81|G82,1:G85|G89, 2:G83)"},
	{NC_DBL,	MKNC_DBL_DRILLQ,		"G83深穴Q値"},
	{NC_FLG,	MKNC_FLG_DRILLCIRCLE,	"円ﾃﾞｰﾀも穴加工"},
	{NC_DBL,	MKNC_DBL_DRILLCIRCLE,	"対象半径"},
	{NC_NUM,	MKNC_NUM_DRILLSORT,		"ｸﾞﾙｰﾋﾟﾝｸﾞ(0:昇順,1:降順)"},
	{NC_FLG,	MKNC_FLG_DRILLBREAK,	"大きさごとにｺﾒﾝﾄを埋め込む"},
	{NC_NUM,	MKNC_NUM_DRILLCIRCLEPROCESS,	"実点と円ﾃﾞｰﾀとの手順(0:無視,1:先,2:後)"},
	{NC_PAGE,	6},		// Page6(ﾚｲﾔ:Dialog8)
	{NC_FLG,	MKNC_FLG_LAYERCOMMENT,	"ﾚｲﾔ情報をｺﾒﾝﾄで挿入"},
	{NC_NUM,	MKNC_NUM_MOVEZ,			"移動ﾚｲﾔのZ軸(0:そのまま,1:R点,2:ｲﾆｼｬﾙ点)"},
	{NC_STR,	MKNC_STR_CUSTMOVE_B,	"ｶｽﾀﾑ移動ｺｰﾄﾞ(前)"},
	{NC_STR,	MKNC_STR_CUSTMOVE_A,	"ｶｽﾀﾑ移動ｺｰﾄﾞ(後)"},
	{NC_FLG,	MKNC_FLG_L0CYCLE,		"固定ｻｲｸﾙ中はL0出力"},
	{NC_PAGE,	7},		// Page7(最適化:Dialog5)
	{NC_DBL,	MKNC_DBL_TOLERANCE,		"同一座標と見なす許容差"},
	{NC_NUM,	MKNC_NUM_TOLERANCE,		"超えた時の動作(0:Z上昇G00移動,1:G01補間)"},
	{NC_NUM,	MKNC_NUM_OPTIMAIZEDRILL,"穴加工基準軸(0:なし,1:X,2:Y)"},
	{NC_DBL,	MKNC_DBL_DRILLMARGIN,	"穴加工同一軸上と見なす許容差"},
	{NC_STR,	MKNC_STR_PERLSCRIPT,	"生成後に実行されるPerlｽｸﾘﾌﾟﾄ"},
};

/////////////////////////////////////////////////////////////////////////////
// CNCMakeMillOpt クラスの構築/消滅

CNCMakeMillOpt::CNCMakeMillOpt(LPCTSTR lpszInit) :
	CNCMakeOption(MilOption,
		SIZEOF(g_szInitComment), g_szInitComment,
		SIZEOF(g_stSaveOrder),   g_stSaveOrder)
{
	ASSERT(	MKNC_NUM_NUMS == SIZEOF(g_szMilNOrder) );
	ASSERT( MKNC_NUM_NUMS == SIZEOF(g_dfMilNOrder) );
	ASSERT( MKNC_DBL_NUMS == SIZEOF(g_szMilDOrder) );
	ASSERT( MKNC_DBL_NUMS == SIZEOF(g_dfMilDOrder) );
	ASSERT( MKNC_FLG_NUMS == SIZEOF(g_szMilBOrder) );
	ASSERT( MKNC_FLG_NUMS == SIZEOF(g_dfMilBOrder) );
	ASSERT( MKNC_STR_NUMS == SIZEOF(g_szMilSOrder) );
	ASSERT( MKNC_STR_NUMS == SIZEOF(g_dfMilSOrder) );

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

void CNCMakeMillOpt::InitialDefault(void)
{
	extern	LPTSTR	g_pszExecDir;	// 実行ﾃﾞｨﾚｸﾄﾘ(NCVC.cpp)
	int		i;

	for ( i=0; i<MKNC_NUM_NUMS; i++ )
		m_pIntOpt[i] = g_dfMilNOrder[i];
	for ( i=0; i<MKNC_DBL_NUMS; i++ )
		m_pDblOpt[i] = g_dfMilDOrder[i];
	for ( i=0; i<MKNC_FLG_NUMS; i++ )
		m_pFlgOpt[i] = g_dfMilBOrder[i];
	m_strOption.RemoveAll();
	for ( i=0; i<MKNC_STR_NUMS; i++ )
		m_strOption.Add(g_dfMilSOrder[i]);

	for ( i=MKNC_STR_HEADER; i<=MKNC_STR_FOOTER; i++ )
		m_strOption[i] = g_pszExecDir + m_strOption[i];
}

BOOL CNCMakeMillOpt::IsPathID(int n)
{
	return ( n==MKNC_STR_HEADER || n==MKNC_STR_FOOTER || n==MKNC_STR_PERLSCRIPT );
}

CString CNCMakeMillOpt::GetLineNoForm(void) const
{
	return MIL_S_LINEFORM;
}

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
		m_pIntOpt[nID] = AfxGetApp()->GetProfileInt(strRegKey, strEntry, nRegDxfNumsDef[i]);
	}
	for ( i=0; i<SIZEOF(nRegDxfFlags); i++ ) {
		VERIFY(strEntry.LoadString(nRegDxfFlags[i]));
		nID = nRegDxfFlagsID[i];
		m_pFlgOpt[nID] = (BOOL)(AfxGetApp()->GetProfileInt(strRegKey, strEntry, nRegDxfFlagsDef[i]));
	}
	VERIFY(strEntry.LoadString(IDS_REG_DXF_CIRCLEIJ));
	MIL_I_IJ = AfxGetApp()->GetProfileInt(strRegKey, strEntry, 1);
	VERIFY(strEntry.LoadString(IDS_REG_DXF_LINEFORM));
	MIL_S_LINEFORM = AfxGetApp()->GetProfileString(strRegKey, strEntry);

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

#ifdef _DEBUG
void CNCMakeMillOpt::DbgDump(void) const
{
	printf("CNCMakeMillOpt InitFile=%s\n", LPCTSTR(GetInitFile()));
	printf("----------\n");
	printf("  Spindle      =%d\n", MIL_I_SPINDLE);
	printf("  Feed         =%f\n", MIL_D_FEED);
	printf("  ZFeed        =%f\n", MIL_D_ZFEED);
	printf("  ZG0Stop      =%f\n", MIL_D_ZG0STOP);
	printf("  ZCut         =%f\n", MIL_D_ZCUT);
	printf("  G92[X]       =%f\n", MIL_D_G92X);
	printf("  G92[Y]       =%f\n", MIL_D_G92Y);
	printf("  G92[Z]       =%f\n", MIL_D_G92Z);
	printf("  Header       =%s\n", LPCTSTR(MIL_S_HEADER));
	printf("  Footer       =%s\n", LPCTSTR(MIL_S_FOOTER));
	printf("----------\n");
	printf("  Xrev         =%d\n", MIL_F_XREV);
	printf("  Yrev         =%d\n", MIL_F_YREV);
	printf("  bLineAdd     =%d\n", MIL_F_LINEADD);
	printf("  LineForm     =%s\n", LPCTSTR(MIL_S_LINEFORM));
	printf("  nLineAdd     =%d\n", MIL_I_LINEADD);
	printf("  EOB          =%s\n", LPCTSTR(MIL_S_EOB));
	printf("  G90          =%d\n", MIL_I_G90);
	printf("  ZReturn      =%d\n", MIL_I_ZRETURN);
	printf("  Gclip        =%d\n", MIL_F_GCLIP);
	printf("  DisSpindle   =%d\n", MIL_F_DISABLESPINDLE);
	printf("----------\n");
	printf("  Dot          =%d\n", MIL_I_DOT);
	printf("  FDot         =%d\n", MIL_I_FDOT);
	printf("  ZeroCut      =%d\n", MIL_F_ZEROCUT);
	printf("  CircleCode   =%d\n", MIL_I_CIRCLECODE);
	printf("  IJ           =%d\n", MIL_I_IJ);
	printf("  CircleHalf   =%d\n", MIL_F_CIRCLEHALF);
	printf("  ZeroCutIJ    =%d\n", MIL_F_ZEROCUT_IJ);
	printf("----------\n");
	printf("  Ellipse      =%f\n", MIL_D_ELLIPSE);
	printf("  EllipseFlg   =%d\n", MIL_F_ELLIPSE);
	printf("----------\n");
	printf("  MakeEnd      =%d\n", MIL_I_MAKEEND);
	printf("  MakeValue    =%f\n", MIL_D_MAKEEND);
	printf("  MakeFeed     =%f\n", MIL_D_MAKEENDFEED);
	printf("  Deep         =%d\n", MIL_F_DEEP);
	printf("  DeepFinal    =%f\n", MIL_D_DEEP);
	printf("  ZStep        =%f\n", MIL_D_ZSTEP);
	printf("  DeepZProcess =%d\n", MIL_I_DEEPRETURN);
	printf("  DeepAProcess =%d\n", MIL_I_DEEPALL);
	printf("  DeepCProcess =%d\n", MIL_I_DEEPROUND);
	printf("  Helical      =%d\n", MIL_F_HELICAL);
	printf("  DeepFinish   =%d\n", MIL_F_DEEPFINISH);
	printf("  DeepSpindle  =%d\n", MIL_I_DEEPSPINDLE);
	printf("  DeepFeed     =%f\n", MIL_D_DEEPFEED);
	printf("----------\n");
	printf("  DrillSpindle =%d\n", MIL_I_DRILLSPINDLE);
	printf("  DrillFeed    =%f\n", MIL_D_DRILLFEED);
	printf("  DrillR       =%f\n", MIL_D_DRILLR);
	printf("  DrillZ       =%f\n", MIL_D_DRILLZ);
	printf("  DrillQ       =%f\n", MIL_D_DRILLQ);
	printf("  Dwell        =%f\n", MIL_D_DWELL);
	printf("  DwellFormat  =%d\n", MIL_I_DWELLFORMAT);
	printf("  DrillMatch   =%d\n", MIL_F_DRILLMATCH);
	printf("  DrillProcess =%d\n", MIL_I_DRILLPROCESS);
	printf("  DrillZProcess=%d\n", MIL_I_DRILLRETURN);
	printf("----------\n");
	printf("  DrillCircle  =%d\n", MIL_F_DRILLCIRCLE);
	printf("  DrillCircleR =%f\n", MIL_D_DRILLCIRCLE);
	printf("  DrillSort    =%d\n", MIL_I_DRILLSORT);
	printf("  DrillCProcess=%d\n", MIL_I_DRILLCIRCLEPROCESS);
	printf("  DrillBreak   =%d\n", MIL_F_DRILLBREAK);
	printf("----------\n");
	printf("  LayerComment =%d\n", MIL_F_LAYERCOMMENT);
	printf("  MoveZ        =%d\n", MIL_I_MOVEZ);
	printf("  CustMoveB    =%s\n", LPCTSTR(MIL_S_CUSTMOVE_B));
	printf("  CustMoveA    =%s\n", LPCTSTR(MIL_S_CUSTMOVE_A));
	printf("  L0Cycle      =%d\n", MIL_F_L0CYCLE);
	printf("----------\n");
	printf("  Tolerance    =%f\n", MIL_D_TOLERANCE);
	printf("  TolerancePro =%d\n", MIL_I_TOLERANCE);
	printf("  DrillOptimaiz=%d\n", MIL_I_OPTIMAIZEDRILL);
	printf("  DrillMargin  =%f\n", MIL_D_DRILLMARGIN);
	printf("  PerlScript   =%s\n", LPCTSTR(MIL_S_PERLSCRIPT));
}
#endif
