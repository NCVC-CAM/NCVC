// NCMakeWireOpt.cpp: CNCMakeWireOpt �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "NCMakeWireOpt.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// int�^����
static	LPCTSTR	g_szWirNOrder[] = {
	"ProgNo", "LineAddType", "G90", "Dot", "FDot", "CircleCode"
};
static	const	int		g_dfWirNOrder[] = {
	1, 1, 0, 0, 2, 0
};

// float�^����
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

// BOOL�^����
static	LPCTSTR	g_szWirBOrder[] = {
	"ProgSet", "ProgAuto", "LineAdd", "ZeroCut", "GClip", "EllipseFlg",
	// --
	"AWFstart", "AWFend"
};
static	const	BOOL	g_dfWirBOrder[] = {
	TRUE, FALSE, FALSE, TRUE, TRUE, TRUE,
	TRUE, TRUE
};

// CString�^����
static	LPCTSTR	g_szWirSOrder[] = {
	"LineForm", "EOB", "Header", "Footer",
	"TaperMode", "AWFconnect", "AWFcut"
};
static	LPCTSTR	g_dfWirSOrder[] = {
	"N%04d", "", "HeaderWire.txt", "FooterWire.txt",
	"M15P0", "M60", "M50"
};

// ��߼�ݓ���
static	NCMAKEOPTION	WirOption[] = {
	{MKWI_NUM_NUMS, g_szWirNOrder},
	{MKWI_DBL_NUMS, g_szWirDOrder},
	{MKWI_FLG_NUMS, g_szWirBOrder},
	{MKWI_STR_NUMS, g_szWirSOrder}
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
// հ�����ފ֐�

void CNCMakeWireOpt::InitialDefault(void)
{
	extern	LPTSTR	g_pszExecDir;	// ���s�ިڸ��(NCVC.cpp)
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
