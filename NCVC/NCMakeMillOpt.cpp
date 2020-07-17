// NCMakeMillOpt.cpp: CNCMakeMillOpt �N���X�̃C���v�������e�[�V����
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

// int�^����
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

// double�^����
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

// BOOL�^����
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

// CString�^����
static	LPCTSTR	g_szSOrder[] = {
	"LineForm", "EOB", "Header", "Footer",
	"CustomMoveB", "CustomMoveA"
};
static	LPCTSTR	g_dfSOrder[] = {
	"N%04d", "", "Header.txt", "Footer.txt",
	"", ""
};

// �ۑ��Ɋւ�����
static	LPCTSTR	g_szInitComment[] = {
	"##\n",
	"## NCVC:�؍����̧��\n"
};
static	SAVEORDER	g_stSaveOrder[] = {
	{NC_PAGE,	1},		// Page1(��{:Dialog1)
	{NC_NUM,	MKNC_NUM_SPINDLE,		"�厲��]��"},
	{NC_DBL,	MKNC_DBL_FEED,			"�؍푗��"},
	{NC_DBL,	MKNC_DBL_ZFEED,			"Z������"},
	{NC_DBL,	MKNC_DBL_ZG0STOP,		"R�_"},
	{NC_DBL,	MKNC_DBL_ZCUT,			"Z���؂荞��"},
	{NC_DBL,	MKNC_DBL_G92X,			"�؍팴�_(G92X)"},
	{NC_DBL,	MKNC_DBL_G92Y,			"�؍팴�_(G92Y)"},
	{NC_DBL,	MKNC_DBL_G92Z,			"�؍팴�_(G92Z)"},
	{NC_FLG,	MKNC_FLG_XREV,			"X�����]"},
	{NC_FLG,	MKNC_FLG_YREV,			"Y�����]"},
	{NC_STR,	MKNC_STR_HEADER,		"����ͯ�ް"},
	{NC_STR,	MKNC_STR_FOOTER,		"����̯�ް"},
	{NC_PAGE,	2},		// Page2(����:Dialog2)
	{NC_FLG,	MKNC_FLG_PROG,			"�n�ԍ�����"},
	{NC_NUM,	MKNC_NUM_PROG,			"��۸��єԍ�"},
	{NC_FLG,	MKNC_FLG_PROGAUTO,		"���܂����ԍ�"},
	{NC_FLG,	MKNC_FLG_LINEADD,		"�s�ԍ��ǉ�"},
	{NC_STR,	MKNC_STR_LINEFORM,		"�s�ԍ�����"},
	{NC_NUM,	MKNC_NUM_LINEADD,		"�s�ԍ��{��"},
	{NC_STR,	MKNC_STR_EOB,			"EOB"},
	{NC_NUM,	MKNC_NUM_G90,			"�ʒu�w��(0:G90,1:G91)"},
	{NC_NUM,	MKNC_NUM_ZRETURN,		"Z�����A(0:�Ƽ��,1:R�_)"},
	{NC_FLG,	MKNC_FLG_GCLIP,			"Ӱ���"},
	{NC_FLG,	MKNC_FLG_DISABLESPINDLE,"S���Ұ��𐶐����Ȃ�"},
	{NC_PAGE,	3},		// Page3(�\�L:Dialog6)
	{NC_NUM,	MKNC_NUM_DOT,			"���W�\�L(0:�����_,1:1/1000)"},
	{NC_NUM,	MKNC_NUM_FDOT,			"F���Ұ��\�L(0:�����_,1:1/1000,2:����)"},
	{NC_FLG,	MKNC_FLG_ZEROCUT,		"�����_�ȉ��̾�۶��"},
	{NC_NUM,	MKNC_NUM_CIRCLECODE,	"�~�ް��̐؍�(0:G02,1:G03)"},
	{NC_NUM,	MKNC_NUM_IJ,			"�~�ʎw��(0:R,1:I/J)"},
	{NC_FLG,	MKNC_FLG_CIRCLEHALF,	"�S�~��2����"},
	{NC_FLG,	MKNC_FLG_ZEROCUT_IJ,	"[I|J]0�͏ȗ�"},
	{NC_DBL,	MKNC_DBL_ELLIPSE,		"�ȉ~����"},
	{NC_FLG,	MKNC_FLG_ELLIPSE,		"���a�ƒZ�a���������ȉ~�͉~�Ƃ݂Ȃ�"},
	{NC_PAGE,	4},		// Page4(�[��:Dialog3)
	{NC_NUM,	MKNC_NUM_MAKEEND,		"���H�ςݐ[���̎w��(0:�Ȃ�,1:�̾��,2:�Œ�Z)"},
	{NC_DBL,	MKNC_DBL_MAKEEND,		"�̾�Ēl or �Œ�Z�l"},
	{NC_DBL,	MKNC_DBL_MAKEENDFEED,	"���H�ςݐ[����Z���葬�x"},
	{NC_FLG,	MKNC_FLG_DEEP,			"�[�����s��"},
	{NC_DBL,	MKNC_DBL_DEEP,			"�ŏI�؂荞��"},
	{NC_DBL,	MKNC_DBL_ZSTEP,			"Z���؂荞�ݽï��"},
	{NC_NUM,	MKNC_NUM_DEEPALL,		"�[���؍�菇(0:�S��,1:��M)"},
	{NC_NUM,	MKNC_NUM_DEEPROUND,		"�[���؍����(0:����,1:���)"},
	{NC_NUM,	MKNC_NUM_DEEPRETURN,	"R�_�ւ�Z�����A(0:������,1:�؍푗��)"},
	{NC_FLG,	MKNC_FLG_HELICAL,		"�~�ް����ضِ؍�"},
	{NC_FLG,	MKNC_FLG_DEEPFINISH,	"�ŏIZ�l�d�グ�K�p"},
	{NC_NUM,	MKNC_NUM_DEEPSPINDLE,	"�d�グ��]��"},
	{NC_DBL,	MKNC_DBL_DEEPFEED,		"�d�グ����"},
	{NC_PAGE,	5},		// Page5(�����H:Dialog4)
	{NC_NUM,	MKNC_NUM_DRILLSPINDLE,	"�����H��]��"},
	{NC_DBL,	MKNC_DBL_DRILLFEED,		"�����H����"},
	{NC_DBL,	MKNC_DBL_DRILLR,		"�����HR�_"},
	{NC_DBL,	MKNC_DBL_DRILLZ,		"�����H�؂荞��"},
	{NC_FLG,	MKNC_FLG_DRILLMATCH,	"�����H������W����"},
	{NC_NUM,	MKNC_NUM_DWELL,			"�޳�َ���"},
	{NC_NUM,	MKNC_NUM_DWELLFORMAT,	"�޳�َ��ԕ\�L(0:�����_,1:����)"},
	{NC_NUM,	MKNC_NUM_DRILLPROCESS,	"�����H�菇(0:��,1:��,2:�̂�)"},
	{NC_NUM,	MKNC_NUM_DRILLRETURN,	"Z�����A(0:G81|G82,1:G85|G89)"},
	{NC_FLG,	MKNC_FLG_DRILLCIRCLE,	"�~�ް��������H"},
	{NC_DBL,	MKNC_DBL_DRILLCIRCLE,	"�Ώ۔��a"},
	{NC_NUM,	MKNC_NUM_DRILLSORT,		"��ٰ��ݸ�(0:����,1:�~��)"},
	{NC_FLG,	MKNC_FLG_DRILLBREAK,	"�傫�����Ƃɺ��Ă𖄂ߍ���"},
	{NC_NUM,	MKNC_NUM_DRILLCIRCLEPROCESS,	"���_�Ɖ~�ް��Ƃ̎菇(0:����,1:��,2:��)"},
	{NC_PAGE,	6},		// Page6(ڲ�:Dialog8)
	{NC_FLG,	MKNC_FLG_LAYERCOMMENT,	"ڲԂ��Ƃɺ��Ă𖄂ߍ���"},
	{NC_NUM,	MKNC_NUM_MOVEZ,			"�ړ�ڲԂ�Z��(0:���̂܂�,1:R�_,2:�Ƽ�ٓ_)"},
	{NC_STR,	MKNC_STR_CUSTMOVE_B,	"���шړ�����(�O)"},
	{NC_STR,	MKNC_STR_CUSTMOVE_A,	"���шړ�����(��)"},
	{NC_FLG,	MKNC_FLG_L0CYCLE,		"�Œ軲�ْ���L0�o��"},
	{NC_PAGE,	7},		// Page7(�œK��:Dialog5)
	{NC_DBL,	MKNC_DBL_TOLERANCE,		"������W�ƌ��Ȃ����e��"},
	{NC_NUM,	MKNC_NUM_TOLERANCE,		"���������̓���(0:Z�㏸G00�ړ�,1:G01���)"},
	{NC_NUM,	MKNC_NUM_OPTIMAIZEDRILL,"�����H���(0:�Ȃ�,1:X,2:Y)"},
	{NC_DBL,	MKNC_DBL_DRILLMARGIN,	"�����H���ꎲ��ƌ��Ȃ����e��"}
};

/////////////////////////////////////////////////////////////////////////////
// CNCMakeMillOpt �N���X�̍\�z/����

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

	// ���ʌ݊��̂��߂̏���
	BOOL bResult = Convert();
	// ���ʌ݊���߼�݂ɏ㏑��
	ReadMakeOption(lpszInit);
	// �ڍs����
	if ( bResult && lpszInit )
		SaveMakeOption();
}

/////////////////////////////////////////////////////////////////////////////
// հ�����ފ֐�

// ڼ޽�؂���̈ڍs
BOOL CNCMakeMillOpt::Convert()
{
	extern	LPCTSTR	gg_szRegKey;

	// NC������߼��(���ް�ޮ݌݊��p)
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
	// �ڍs��������
	VERIFY(strRegKey.LoadString(IDS_REGKEY_DXF));
	VERIFY(strEntry.LoadString(IDS_REG_CONVERT));
	if ( AfxGetApp()->GetProfileInt(strRegKey, strEntry, 0) > 0 )
		return FALSE;

	// �ް��̈ڍs
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

	// ��ڼ޽�؂̏���
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

	// Init����̧�ٖ��̍폜
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
