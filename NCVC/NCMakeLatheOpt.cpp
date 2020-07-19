// NCMakeLatheOpt.cpp: CNCMakeLatheOpt �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "NCMakeLatheOpt.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// int�^����
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

// float�^����
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

// BOOL�^����
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

// CString�^����
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
	"##\tX���̒l�͔��a�l�ł��B�޲�۸ސݒ�ł͂Q�{����܂��B\n"
};
static	SAVEORDER	g_stSaveOrder[] = {
	{NC_PAGE,	1},		// Page1(��{)
	{NC_STR,	MKLA_STR_HEADER,		"����ͯ�ް"},
	{NC_STR,	MKLA_STR_FOOTER,		"����̯�ް"},
	{NC_PAGE,	2},		// Page2(����)
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
	{NC_PAGE,	3},		// Page3(�\�L)
	{NC_NUM,	MKLA_NUM_DOT,			"���W�\�L(0:�����_,1:1/1000)"},
	{NC_NUM,	MKLA_NUM_FDOT,			"F���Ұ��\�L(0:�����_,1:1/1000,2:����)"},
	{NC_FLG,	MKLA_FLG_ZEROCUT,		"�����_�ȉ��̾�۶��"},
	{NC_NUM,	MKLA_NUM_CIRCLECODE,	"�~�ް��̐؍�(0:G02,1:G03)"},
	{NC_NUM,	MKLA_NUM_IJ,			"�~�ʎw��(0:R,1:I/J)"},
	{NC_FLG,	MKLA_FLG_CIRCLEHALF,	"�S�~��2����"},
	{NC_FLG,	MKLA_FLG_ZEROCUT_IJ,	"[I|J]0�͏ȗ�"},
	{NC_DBL,	MKLA_DBL_ELLIPSE,		"�ȉ~����"},
	{NC_FLG,	MKLA_FLG_ELLIPSE,		"���a�ƒZ�a���������ȉ~�͉~�Ƃ݂Ȃ�"},
	{NC_PAGE,	4},		// Page4(�[��)
	{NC_FLG,	MKLA_FLG_ENDFACE,		"�[�ʏ������s��"},
	{NC_STR,	MKLA_STR_E_CUSTOM,		"�[�ʶ��Ѻ���"},
	{NC_NUM,	MKLA_NUM_E_SPINDLE,		"�[�ʎ厲��]��"},
	{NC_DBL,	MKLA_DBL_E_FEED,		"�[�ʐ؍푗��"},
	{NC_DBL,	MKLA_DBL_E_CUT,			"�[�ʍŏI�؂荞��"},
	{NC_DBL,	MKLA_DBL_E_STEP,		"�[�ʐ؂荞�ݽï��"},
	{NC_DBL,	MKLA_DBL_E_PULLZ,		"�[�ʈ�����(Z)"},
	{NC_DBL,	MKLA_DBL_E_PULLX,		"�[�ʈ�����(X)"},
	{NC_PAGE,	5},		// Page5(����)
	{NC_STR,	MKLA_STR_DRILL,			"�g�p�h����(�����̏ꍇ�ͺ�ςŋ�؂�)"},
	{NC_STR,	MKLA_STR_DRILLSPINDLE,	"�����厲��]��"},
	{NC_STR,	MKLA_STR_DRILLFEED,		"�����؍푗��"},
	{NC_STR,	MKLA_STR_D_CUSTOM,		"�������Ѻ���"},
	{NC_DBL,	MKLA_DBL_HOLE,			"������������"},
	{NC_FLG,	MKLA_FLG_CYCLE,			"�Œ軲�قŐ���"},
	{NC_DBL,	MKLA_DBL_DRILLZ,		"�؂荞��"},
	{NC_DBL,	MKLA_DBL_DRILLQ,		"R�_"},
	{NC_DBL,	MKLA_DBL_DRILLQ,		"Q�l"},
	{NC_DBL,	MKLA_DBL_DRILLD,		"�߂��"},
	{NC_DBL,	MKLA_DBL_D_DWELL,		"�����޳�َ���[msec]"},
	{NC_PAGE,	6},		// Page6(���a)
	{NC_STR,	MKLA_STR_I_CUSTOM,		"���a���Ѻ���"},
	{NC_NUM,	MKLA_NUM_I_SPINDLE,		"���a�厲��]��"},
	{NC_DBL,	MKLA_DBL_I_FEED,		"���a�؍푗��(Z)"},
	{NC_DBL,	MKLA_DBL_I_FEEDX,		"���a�؍푗��(X)"},
	{NC_DBL,	MKLA_DBL_I_CUT,			"���a�؂荞��(���a�l)"},
	{NC_DBL,	MKLA_DBL_I_PULLZ,		"���a������(Z)"},
	{NC_DBL,	MKLA_DBL_I_PULLX,		"���a������(X���a�l)"},
	{NC_DBL,	MKLA_DBL_I_MARGIN,		"���a�d�グ��(���a�l)"},
	{NC_NUM,	MKLA_NUM_I_MARGIN,		"���a�d�グ��"},
	{NC_PAGE,	7},		// Page7(�O�a)
	{NC_STR,	MKLA_STR_O_CUSTOM,		"�O�a���Ѻ���"},
	{NC_NUM,	MKLA_NUM_O_SPINDLE,		"�O�a�厲��]��"},
	{NC_DBL,	MKLA_DBL_O_FEED,		"�O�a�؍푗��(Z)"},
	{NC_DBL,	MKLA_DBL_O_FEEDX,		"�O�a�؍푗��(X)"},
	{NC_DBL,	MKLA_DBL_O_CUT,			"�O�a�؂荞��(���a�l)"},
	{NC_DBL,	MKLA_DBL_O_PULLZ,		"�O�a������(Z)"},
	{NC_DBL,	MKLA_DBL_O_PULLX,		"�O�a������(X���a�l)"},
	{NC_DBL,	MKLA_DBL_O_MARGIN,		"�O�a�d�グ��(���a�l)"},
	{NC_NUM,	MKLA_NUM_O_MARGIN,		"�O�a�d�グ��"},
	{NC_PAGE,	8},		// Page8(�˂��؂�)
	{NC_STR,	MKLA_STR_G_CUSTOM,		"�˂��؂趽�Ѻ���"},
	{NC_NUM,	MKLA_NUM_G_SPINDLE,		"�˂��؂�厲��]��"},
	{NC_DBL,	MKLA_DBL_G_FEED,		"�˂��؂�؍푗��(Z)"},
	{NC_DBL,	MKLA_DBL_G_FEEDX,		"�˂��؂�؍푗��(X)"},
	{NC_DBL,	MKLA_DBL_G_PULLX,		"�˂��؂������(X���a�l)"},
	{NC_DBL,	MKLA_DBL_G_DWELL,		"�˂��؂��޳�َ���[msec]"},
	{NC_DBL,	MKLA_DBL_GROOVEWIDTH,	"�˂��؂�n��"},
	{NC_NUM,	MKLA_NUM_GROOVETOOL,	"�˂��؂�H���_(0:��,1:����,2:�E)"},
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
				v.push_back(info);	// ���ق��L���ȂƂ������o�^
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
