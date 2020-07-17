// NCMakeLatheOpt.cpp: CNCMakeLatheOpt �N���X�̃C���v�������e�[�V����
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

// int�^����
static	LPCTSTR	g_szLthNOrder[] = {
	"Spindle", "MarginNum",
	"ProgNo", "LineAddType",
	"G90", "Dot", "FDot", "CircleCode", "CircleIJ"
};
static	const	int		g_dfLthNOrder[] = {
	200, 1,
	1, 1,
	0, 0, 0, 0, 0
};

// float�^����
static	LPCTSTR	g_szLthDOrder[] = {
	"Feed", "XFeed", "Cut", "PullZ", "PullX",
	"Margin", "Ellipse"
};
static	const	float	g_dfLthDOrder[] = {
	300.0, 150.0, 1.0, 2.0, 2.0,
	1.0, 0.5,
};

// BOOL�^����
static	LPCTSTR	g_szLthBOrder[] = {
	"ProgSet", "ProgAuto",
	"LineAdd", "ZeroCut", "GClip", "DisableSpindle",
	"CircleHalf", "ZeroCutIJ",
	"EllipseFlg"
};
static	const	BOOL	g_dfLthBOrder[] = {
	TRUE, FALSE,
	FALSE, TRUE, TRUE, FALSE,
	FALSE, TRUE,
	TRUE,
};

// CString�^����
static	LPCTSTR	g_szLthSOrder[] = {
	"LineForm", "EOB",
	"Header", "Footer"
};
static	LPCTSTR	g_dfLthSOrder[] = {
	"N%04d", "",
	"HeaderLathe.txt", "FooterLathe.txt"
};

// ��߼�ݓ���
static	NCMAKEOPTION	LthOption[] = {
	{MKLA_NUM_NUMS, g_szLthNOrder},
	{MKLA_DBL_NUMS, g_szLthDOrder},
	{MKLA_FLG_NUMS, g_szLthBOrder},
	{MKLA_STR_NUMS, g_szLthSOrder}
};

// �ۑ��Ɋւ�����
static	LPCTSTR	g_szInitComment[] = {
	"##\n",
	"## NCVC:���՗p�؍����̧��\n",
	"##\tX���̒l�͔��a�l�ł��B�޲�۸ސݒ�ł͔{����܂��B\n"
};
static	SAVEORDER	g_stSaveOrder[] = {
	{NC_PAGE,	1},		// Page1(��{:Dialog1)
	{NC_NUM,	MKLA_NUM_SPINDLE,		"�厲��]��"},
	{NC_DBL,	MKLA_DBL_FEED,			"�؍푗��(Z)"},
	{NC_DBL,	MKLA_DBL_XFEED,			"�؍푗��(X)"},
	{NC_DBL,	MKLA_DBL_CUT,			"�؂荞��(���a�l)"},
	{NC_DBL,	MKLA_DBL_PULL_Z,		"������(Z)"},
	{NC_DBL,	MKLA_DBL_PULL_X,		"������(X���a�l)"},
	{NC_DBL,	MKLA_DBL_MARGIN,		"�d�グ��(���a�l)"},
	{NC_NUM,	MKLA_NUM_MARGIN,		"�d�グ��"},
	{NC_STR,	MKLA_STR_HEADER,		"����ͯ�ް"},
	{NC_STR,	MKLA_STR_FOOTER,		"����̯�ް"},
	{NC_PAGE,	2},		// Page2(����:Dialog2)
	{NC_FLG,	MKLA_FLG_PROG,			"�n�ԍ�����"},
	{NC_NUM,	MKLA_NUM_PROG,			"��۸��єԍ�"},
	{NC_FLG,	MKLA_FLG_PROGAUTO,		"���܂����ԍ�"},
	{NC_FLG,	MKLA_FLG_LINEADD,		"�s�ԍ��ǉ�"},
	{NC_STR,	MKLA_STR_LINEFORM,		"�s�ԍ�����"},
	{NC_NUM,	MKLA_NUM_LINEADD,		"�s�ԍ��{��"},
	{NC_STR,	MKLA_STR_EOB,			"EOB"},
	{NC_NUM,	MKLA_NUM_G90,			"�ʒu�w��(0:G90,1:G91)"},
	{NC_FLG,	MKLA_FLG_GCLIP,			"Ӱ���"},
	{NC_FLG,	MKLA_FLG_DISABLESPINDLE,"S���Ұ��𐶐����Ȃ�"},
	{NC_PAGE,	3},		// Page3(�\�L:Dialog6)
	{NC_NUM,	MKLA_NUM_DOT,			"���W�\�L(0:�����_,1:1/1000)"},
	{NC_NUM,	MKLA_NUM_FDOT,			"F���Ұ��\�L(0:�����_,1:1/1000,2:����)"},
	{NC_FLG,	MKLA_FLG_ZEROCUT,		"�����_�ȉ��̾�۶��"},
	{NC_NUM,	MKLA_NUM_CIRCLECODE,	"�~�ް��̐؍�(0:G02,1:G03)"},
	{NC_NUM,	MKLA_NUM_IJ,			"�~�ʎw��(0:R,1:I/J)"},
	{NC_FLG,	MKLA_FLG_CIRCLEHALF,	"�S�~��2����"},
	{NC_FLG,	MKLA_FLG_ZEROCUT_IJ,	"[I|J]0�͏ȗ�"},
	{NC_DBL,	MKLA_DBL_ELLIPSE,		"�ȉ~����"},
	{NC_FLG,	MKLA_FLG_ELLIPSE,		"���a�ƒZ�a���������ȉ~�͉~�Ƃ݂Ȃ�"}
};

/////////////////////////////////////////////////////////////////////////////
// CNCMakeLatheOpt �N���X�̍\�z/����

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
// հ�����ފ֐�

void CNCMakeLatheOpt::InitialDefault(void)
{
	extern	LPTSTR	g_pszExecDir;	// ���s�ިڸ��(NCVC.cpp)
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

#ifdef _DEBUG
void CNCMakeLatheOpt::DbgDump(void) const
{
	CMagaDbg	dbg("CNCMakeLatheOpt", DBG_RED);

	dbg.printf("InitFile=%s", GetInitFile());
	dbg.printf("----------");
	dbg.printf("  Spindle      =%d", LTH_I_SPINDLE);
	dbg.printf("  Feed         =%f", LTH_D_FEED);
	dbg.printf("  XFeed        =%f", LTH_D_XFEED);
	dbg.printf("  Cut          =%f", LTH_D_CUT);
	dbg.printf("  PullZ        =%f", LTH_D_PULL_Z);
	dbg.printf("  PullX        =%f", LTH_D_PULL_X);
	dbg.printf("  Margin       =%f", LTH_D_MARGIN);
	dbg.printf("  Header       =%s", m_strOption[MKLA_STR_HEADER]);
	dbg.printf("  Footer       =%s", m_strOption[MKLA_STR_FOOTER]);
	dbg.printf("----------");
	dbg.printf("  bLineAdd     =%d", LTH_F_LINEADD);
	dbg.printf("  LineForm     =%s", m_strOption[MKLA_STR_LINEFORM]);
	dbg.printf("  nLineAdd     =%d", LTH_I_LINEADD);
	dbg.printf("  EOB          =%s", m_strOption[MKLA_STR_EOB]);
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
	dbg.printf("----------");
	dbg.printf("  Ellipse      =%f", LTH_D_ELLIPSE);
	dbg.printf("  EllipseFlg   =%d", LTH_F_ELLIPSE);
}
#endif
