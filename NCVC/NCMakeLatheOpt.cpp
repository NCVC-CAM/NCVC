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
//#define	_DEBUGOLD
#undef	_DEBUGOLD

// int�^����
static	LPCTSTR	g_szNOrder[] = {
	"Spindle", "MarginNum",
	"ProgNo", "LineAddType",
	"G90", "Dot", "FDot", "CircleCode", "CircleIJ"
};
static	const	int		g_dfNOrder[] = {
	200, 1,
	1, 1,
	0, 0, 0, 0, 0
};

// double�^����
static	LPCTSTR	g_szDOrder[] = {
	"Feed", "XFeed", "Cut", "PullZ", "PullX",
	"Margin", "Ellipse"
};
static	const	double	g_dfDOrder[] = {
	300.0, 150.0, 1.0, 2.0, 2.0,
	1.0, 0.5,
};

// BOOL�^����
static	LPCTSTR	g_szBOrder[] = {
	"ProgSet", "ProgAuto",
	"LineAdd", "ZeroCut", "GClip", "DisableSpindle",
	"CircleHalf", "ZeroCutIJ",
	"EllipseFlg"
};
static	const	BOOL	g_dfBOrder[] = {
	TRUE, FALSE,
	FALSE, TRUE, TRUE, FALSE,
	FALSE, TRUE,
	TRUE,
};

// CString�^����
static	LPCTSTR	g_szSOrder[] = {
	"LineForm", "EOB",
	"Header", "Footer"
};
static	LPCTSTR	g_dfSOrder[] = {
	"N%04d", "",
	"HeaderLathe.txt", "FooterLathe.txt"
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
// հ�����ފ֐�

#ifdef _DEBUGOLD
void CNCMakeLatheOpt::DbgDump(void) const
{
	CMagaDbg	dbg("CNCMakeLatheOpt", DBG_RED);

	dbg.printf("InitFile=%s", GetInitFile());
	dbg.printf("----------");
	dbg.printf("  Spindle      =%d", m_nSpindle);
	dbg.printf("  Feed         =%f", m_dFeed);
	dbg.printf("  Cut          =%f", m_dCut);
	dbg.printf("  PullZ        =%f", m_dPullZ);
	dbg.printf("  PullX        =%f", m_dPullX);
	dbg.printf("  Margin       =%f", m_dMargin);
	dbg.printf("  Header       =%s", m_strOption[MKLA_STR_HEADER]);
	dbg.printf("  Footer       =%s", m_strOption[MKLA_STR_FOOTER]);
	dbg.printf("----------");
	dbg.printf("  bLineAdd     =%d", m_bLineAdd);
	dbg.printf("  LineForm     =%s", m_strOption[MKLA_STR_LINEFORM]);
	dbg.printf("  nLineAdd     =%d", m_nLineAdd);
	dbg.printf("  EOB          =%s", m_strOption[MKLA_STR_EOB]);
	dbg.printf("  G90          =%d", m_nG90);
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
}
#endif
