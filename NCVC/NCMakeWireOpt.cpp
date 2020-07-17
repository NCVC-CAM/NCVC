// NCMakeWireOpt.cpp: CNCMakeWireOpt �N���X�̃C���v�������e�[�V����
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
//#define	_DEBUGOLD
#undef	_DEBUGOLD


// int�^����
static	LPCTSTR	g_szNOrder[] = {
	"ProgNo", "LineAddType", "G90", "Dot", "FDot", "CircleCode"
};
static	const	int		g_dfNOrder[] = {
	1, 1, 0, 0, 2, 0
};

// double�^����
static	LPCTSTR	g_szDOrder[] = {
	"Depth", "Taper", "Feed",
	"G92X", "G92Y",
	"AWFcircleLo", "AWFcircleHi",
	"Ellipse"
};
static	const	double	g_dfDOrder[] = {
	10.0, 0.0, 0.0,
	0.0, 0.0,
	1.0, 1.0,
	0.5,
};

// BOOL�^����
static	LPCTSTR	g_szBOrder[] = {
	"ProgSet", "ProgAuto", "LineAdd", "ZeroCut", "GClip", "EllipseFlg",
	// --
	"AWFstart", "AWFend"
};
static	const	BOOL	g_dfBOrder[] = {
	TRUE, FALSE, FALSE, TRUE, TRUE, TRUE,
	TRUE, TRUE
};

// CString�^����
static	LPCTSTR	g_szSOrder[] = {
	"LineForm", "EOB", "Header", "Footer",
	"TaperMode", "AWFconnect", "AWFcut"
};
static	LPCTSTR	g_dfSOrder[] = {
	"N%04d", "", "HeaderWire.txt", "FooterWire.txt",
	"M15P0", "M60", "M50"
};

// �ۑ��Ɋւ�����
static	LPCTSTR	g_szInitComment[] = {
	"##\n",
	"## NCVC:ܲԕ��d���H�@�p�؍����̧��\n",
};
static	SAVEORDER	g_stSaveOrder[] = {
	{NC_PAGE,	1},		// Page1(��{:Dialog1)
	{NC_DBL,	MKWI_DBL_DEPTH,			"ܰ�����"},
	{NC_DBL,	MKWI_DBL_TAPER,			"ð�ߊp�x[deg]"},
	{NC_STR,	MKWI_STR_TAPERMODE,		"ð��Ӱ��"},
	{NC_DBL,	MKWI_DBL_FEED,			"�؍푗��(��ۂͻ��ޑ���)"},
	{NC_DBL,	MKWI_DBL_G92X,			"�؍팴�_(G92X)"},
	{NC_DBL,	MKWI_DBL_G92Y,			"�؍팴�_(G92Y)"},
	{NC_STR,	MKWI_STR_HEADER,		"����ͯ�ް"},
	{NC_STR,	MKWI_STR_FOOTER,		"����̯�ް"},
	{NC_PAGE,	2},		// Page2(AWF:Dialog2)
	{NC_STR,	MKWI_STR_AWFCNT,		"AWF��������"},
	{NC_STR,	MKWI_STR_AWFCUT,		"AWF�ؒf����"},
	{NC_DBL,	MKWI_DBL_AWFCIRCLE_LO,	"AWF�����߲�� �Ώ۔��a"},
	{NC_DBL,	MKWI_DBL_AWFCIRCLE_HI,	"AWF�����߲�� �Ώ۔��a"},
	{NC_FLG,	MKWI_FLG_AWFSTART,		"���H�O����"},
	{NC_FLG,	MKWI_FLG_AWFEND,		"���H��ؒf"},
	{NC_PAGE,	3},		// Page2(����:Dialog2)
	{NC_FLG,	MKWI_FLG_PROG,			"�n�ԍ�����"},
	{NC_NUM,	MKWI_NUM_PROG,			"��۸��єԍ�"},
	{NC_FLG,	MKWI_FLG_PROGAUTO,		"���܂����ԍ�"},
	{NC_FLG,	MKWI_FLG_LINEADD,		"�s�ԍ��ǉ�"},
	{NC_STR,	MKWI_STR_LINEFORM,		"�s�ԍ�����"},
	{NC_NUM,	MKWI_NUM_LINEADD,		"�s�ԍ��{��"},
	{NC_STR,	MKWI_STR_EOB,			"EOB"},
	{NC_NUM,	MKWI_NUM_G90,			"�ʒu�w��(0:G90,1:G91)"},
	{NC_FLG,	MKWI_FLG_GCLIP,			"Ӱ���"},
	{NC_PAGE,	4},		// Page3(�\�L:Dialog6)
	{NC_NUM,	MKWI_NUM_DOT,			"���W�\�L(0:�����_,1:1/1000)"},
	{NC_NUM,	MKWI_NUM_FDOT,			"F���Ұ��\�L(0:�����_,1:1/1000,2:����)"},
	{NC_FLG,	MKWI_FLG_ZEROCUT,		"�����_�ȉ��̾�۶��"},
	{NC_NUM,	MKWI_NUM_CIRCLECODE,	"�~�ް��̐؍�(0:G02,1:G03)"},
	{NC_DBL,	MKWI_DBL_ELLIPSE,		"�ȉ~����"},
	{NC_FLG,	MKWI_FLG_ELLIPSE,		"���a�ƒZ�a���������ȉ~�͉~�Ƃ݂Ȃ�"}
};

/////////////////////////////////////////////////////////////////////////////
// CNCMakeWireOpt �N���X�̍\�z/����

CNCMakeWireOpt::CNCMakeWireOpt(LPCTSTR lpszInit) :
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
void CNCMakeWireOpt::DbgDump(void) const
{
	CMagaDbg	dbg("CNCMakeWireOpt", DBG_RED);

	dbg.printf("InitFile=%s", GetInitFile());
	dbg.printf("----------");
	dbg.printf("  Depth        =%f", m_dDepth);
	dbg.printf("  Taper        =%f", m_dTaper);
	dbg.printf("  TaperMode    =%s", m_strOption[MKWI_STR_TAPERMODE]);
	dbg.printf("  Feed         =%f", m_dFeed);
	dbg.printf("  Header       =%s", m_strOption[MKWI_STR_HEADER]);
	dbg.printf("  Footer       =%s", m_strOption[MKWI_STR_FOOTER]);
	dbg.printf("----------");
	dbg.printf("  AWFconnect   =%s", m_strOption[MKWI_STR_AWFCNT]);
	dbg.printf("  AWFcut       =%s", m_strOption[MKWI_STR_AWFCUT]);
	dbg.printf("  AWFcircleLo  =%f", m_dAWFcircleLo);
	dbg.printf("  AWFcircleHi  =%f", m_dAWFcircleHi);
	dbg.printf("  AWFstart     =%d", m_bAWFstart);
	dbg.printf("  AWFend       =%d", m_bAWFend);
	dbg.printf("----------");
	dbg.printf("  bProgNo?     =%d", m_bProg);
	dbg.printf("  bProgNo      =%d", m_nProg);
	dbg.printf("  bProgNoAuto  =%d", m_bProgAuto);
	dbg.printf("  bLineAdd     =%d", m_bLineAdd);
	dbg.printf("  LineForm     =%s", m_strOption[MKWI_STR_LINEFORM]);
	dbg.printf("  nLineAdd     =%d", m_nLineAdd);
	dbg.printf("  EOB          =%s", m_strOption[MKWI_STR_EOB]);
	dbg.printf("  G90          =%d", m_nG90);
	dbg.printf("  Gclip        =%d", m_bGclip);
	dbg.printf("----------");
	dbg.printf("  Dot          =%d", m_nDot);
	dbg.printf("  FDot         =%d", m_nFDot);
	dbg.printf("  ZeroCut      =%d", m_bZeroCut);
	dbg.printf("  CircleCode   =%d", m_nCircleCode);
	dbg.printf("  Ellipse      =%f", m_dEllipse);
	dbg.printf("  EllipseFlg   =%d", m_bEllipse);
}
#endif
