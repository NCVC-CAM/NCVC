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

// int�^����
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
	0,
	0, 0
};

// float�^����
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

// BOOL�^����
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

// CString�^����
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

// ��߼�ݓ���
static	NCMAKEOPTION	MilOption[] = {
	{MKNC_NUM_NUMS, g_szMilNOrder},
	{MKNC_DBL_NUMS, g_szMilDOrder},
	{MKNC_FLG_NUMS, g_szMilBOrder},
	{MKNC_STR_NUMS, g_szMilSOrder}
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
	{NC_DBL,	MKNC_DBL_DWELL,			"�޳�َ���"},
	{NC_NUM,	MKNC_NUM_DWELLFORMAT,	"�޳�َ��ԕ\�L(0:�����_,1:����)"},
	{NC_NUM,	MKNC_NUM_DRILLPROCESS,	"�����H�菇(0:��,1:��,2:�̂�)"},
	{NC_NUM,	MKNC_NUM_DRILLRETURN,	"���H����(0:G81|G82,1:G85|G89, 2:G83)"},
	{NC_DBL,	MKNC_DBL_DRILLQ,		"G83�[��Q�l"},
	{NC_FLG,	MKNC_FLG_DRILLCIRCLE,	"�~�ް��������H"},
	{NC_DBL,	MKNC_DBL_DRILLCIRCLE,	"�Ώ۔��a"},
	{NC_NUM,	MKNC_NUM_DRILLSORT,		"��ٰ��ݸ�(0:����,1:�~��)"},
	{NC_FLG,	MKNC_FLG_DRILLBREAK,	"�傫�����Ƃɺ��Ă𖄂ߍ���"},
	{NC_NUM,	MKNC_NUM_DRILLCIRCLEPROCESS,	"���_�Ɖ~�ް��Ƃ̎菇(0:����,1:��,2:��)"},
	{NC_PAGE,	6},		// Page6(ڲ�:Dialog8)
	{NC_FLG,	MKNC_FLG_LAYERCOMMENT,	"ڲԏ�����Ăő}��"},
	{NC_NUM,	MKNC_NUM_MOVEZ,			"�ړ�ڲԂ�Z��(0:���̂܂�,1:R�_,2:�Ƽ�ٓ_)"},
	{NC_STR,	MKNC_STR_CUSTMOVE_B,	"���шړ�����(�O)"},
	{NC_STR,	MKNC_STR_CUSTMOVE_A,	"���шړ�����(��)"},
	{NC_FLG,	MKNC_FLG_L0CYCLE,		"�Œ軲�ْ���L0�o��"},
	{NC_PAGE,	7},		// Page7(�œK��:Dialog5)
	{NC_DBL,	MKNC_DBL_TOLERANCE,		"������W�ƌ��Ȃ����e��"},
	{NC_NUM,	MKNC_NUM_TOLERANCE,		"���������̓���(0:Z�㏸G00�ړ�,1:G01���)"},
	{NC_NUM,	MKNC_NUM_OPTIMAIZEDRILL,"�����H���(0:�Ȃ�,1:X,2:Y)"},
	{NC_DBL,	MKNC_DBL_DRILLMARGIN,	"�����H���ꎲ��ƌ��Ȃ����e��"},
	{NC_STR,	MKNC_STR_PERLSCRIPT,	"������Ɏ��s�����Perl������"},
};

/////////////////////////////////////////////////////////////////////////////
// CNCMakeMillOpt �N���X�̍\�z/����

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

void CNCMakeMillOpt::InitialDefault(void)
{
	extern	LPTSTR	g_pszExecDir;	// ���s�ިڸ��(NCVC.cpp)
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

#ifdef _DEBUG
void CNCMakeMillOpt::DbgDump(void) const
{
	CMagaDbg	dbg("CNCMakeMillOpt", DBG_RED);

	dbg.printf("InitFile=%s", GetInitFile());
	dbg.printf("----------");
	dbg.printf("  Spindle      =%d", MIL_I_SPINDLE);
	dbg.printf("  Feed         =%f", MIL_D_FEED);
	dbg.printf("  ZFeed        =%f", MIL_D_ZFEED);
	dbg.printf("  ZG0Stop      =%f", MIL_D_ZG0STOP);
	dbg.printf("  ZCut         =%f", MIL_D_ZCUT);
	dbg.printf("  G92[X]       =%f", MIL_D_G92X);
	dbg.printf("  G92[Y]       =%f", MIL_D_G92Y);
	dbg.printf("  G92[Z]       =%f", MIL_D_G92Z);
	dbg.printf("  Header       =%s", MIL_S_HEADER);
	dbg.printf("  Footer       =%s", MIL_S_FOOTER);
	dbg.printf("----------");
	dbg.printf("  Xrev         =%d", MIL_F_XREV);
	dbg.printf("  Yrev         =%d", MIL_F_YREV);
	dbg.printf("  bLineAdd     =%d", MIL_F_LINEADD);
	dbg.printf("  LineForm     =%s", MIL_S_LINEFORM);
	dbg.printf("  nLineAdd     =%d", MIL_I_LINEADD);
	dbg.printf("  EOB          =%s", MIL_S_EOB);
	dbg.printf("  G90          =%d", MIL_I_G90);
	dbg.printf("  ZReturn      =%d", MIL_I_ZRETURN);
	dbg.printf("  Gclip        =%d", MIL_F_GCLIP);
	dbg.printf("  DisSpindle   =%d", MIL_F_DISABLESPINDLE);
	dbg.printf("----------");
	dbg.printf("  Dot          =%d", MIL_I_DOT);
	dbg.printf("  FDot         =%d", MIL_I_FDOT);
	dbg.printf("  ZeroCut      =%d", MIL_F_ZEROCUT);
	dbg.printf("  CircleCode   =%d", MIL_I_CIRCLECODE);
	dbg.printf("  IJ           =%d", MIL_I_IJ);
	dbg.printf("  CircleHalf   =%d", MIL_F_CIRCLEHALF);
	dbg.printf("  ZeroCutIJ    =%d", MIL_F_ZEROCUT_IJ);
	dbg.printf("----------");
	dbg.printf("  Ellipse      =%f", MIL_D_ELLIPSE);
	dbg.printf("  EllipseFlg   =%d", MIL_F_ELLIPSE);
	dbg.printf("----------");
	dbg.printf("  MakeEnd      =%d", MIL_I_MAKEEND);
	dbg.printf("  MakeValue    =%f", MIL_D_MAKEEND);
	dbg.printf("  MakeFeed     =%f", MIL_D_MAKEENDFEED);
	dbg.printf("  Deep         =%d", MIL_F_DEEP);
	dbg.printf("  DeepFinal    =%f", MIL_D_DEEP);
	dbg.printf("  ZStep        =%f", MIL_D_ZSTEP);
	dbg.printf("  DeepZProcess =%d", MIL_I_DEEPRETURN);
	dbg.printf("  DeepAProcess =%d", MIL_I_DEEPALL);
	dbg.printf("  DeepCProcess =%d", MIL_I_DEEPROUND);
	dbg.printf("  Helical      =%d", MIL_F_HELICAL);
	dbg.printf("  DeepFinish   =%d", MIL_F_DEEPFINISH);
	dbg.printf("  DeepSpindle  =%d", MIL_I_DEEPSPINDLE);
	dbg.printf("  DeepFeed     =%f", MIL_D_DEEPFEED);
	dbg.printf("----------");
	dbg.printf("  DrillSpindle =%d", MIL_I_DRILLSPINDLE);
	dbg.printf("  DrillFeed    =%f", MIL_D_DRILLFEED);
	dbg.printf("  DrillR       =%f", MIL_D_DRILLR);
	dbg.printf("  DrillZ       =%f", MIL_D_DRILLZ);
	dbg.printf("  DrillQ       =%f", MIL_D_DRILLQ);
	dbg.printf("  Dwell        =%f", MIL_D_DWELL);
	dbg.printf("  DwellFormat  =%d", MIL_I_DWELLFORMAT);
	dbg.printf("  DrillMatch   =%d", MIL_F_DRILLMATCH);
	dbg.printf("  DrillProcess =%d", MIL_I_DRILLPROCESS);
	dbg.printf("  DrillZProcess=%d", MIL_I_DRILLRETURN);
	dbg.printf("----------");
	dbg.printf("  DrillCircle  =%d", MIL_F_DRILLCIRCLE);
	dbg.printf("  DrillCircleR =%f", MIL_D_DRILLCIRCLE);
	dbg.printf("  DrillSort    =%d", MIL_I_DRILLSORT);
	dbg.printf("  DrillCProcess=%d", MIL_I_DRILLCIRCLEPROCESS);
	dbg.printf("  DrillBreak   =%d", MIL_F_DRILLBREAK);
	dbg.printf("----------");
	dbg.printf("  LayerComment =%d", MIL_F_LAYERCOMMENT);
	dbg.printf("  MoveZ        =%d", MIL_I_MOVEZ);
	dbg.printf("  CustMoveB    =%s", MIL_S_CUSTMOVE_B);
	dbg.printf("  CustMoveA    =%s", MIL_S_CUSTMOVE_A);
	dbg.printf("  L0Cycle      =%d", MIL_F_L0CYCLE);
	dbg.printf("----------");
	dbg.printf("  Tolerance    =%f", MIL_D_TOLERANCE);
	dbg.printf("  TolerancePro =%d", MIL_I_TOLERANCE);
	dbg.printf("  DrillOptimaiz=%d", MIL_I_OPTIMAIZEDRILL);
	dbg.printf("  DrillMargin  =%f", MIL_D_DRILLMARGIN);
	dbg.printf("  PerlScript   =%s", MIL_S_PERLSCRIPT);
}
#endif
