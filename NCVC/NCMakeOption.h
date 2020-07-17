// NCMakeOption.h: NC¶¬µÌß¼®Ý‚ÌŠÇ—
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCVCdefine.h"

#define	MKNC_NUM_SPINDLE			0
#define	MKNC_NUM_LINEADD			1
#define	MKNC_NUM_G90				2
#define	MKNC_NUM_ZRETURN			3
#define	MKNC_NUM_DOT				4
#define	MKNC_NUM_FDOT				5
#define	MKNC_NUM_CIRCLECODE			6
#define	MKNC_NUM_IJ					7
#define	MKNC_NUM_MAKEEND			8
#define	MKNC_NUM_DEEPSPINDLE		9
#define	MKNC_NUM_DEEPZPROCESS		10
#define	MKNC_NUM_DEEPAPROCESS		11
#define	MKNC_NUM_DEEPCPROCESS		12
#define	MKNC_NUM_DRILLSPINDLE		13
#define	MKNC_NUM_DWELL				14
#define	MKNC_NUM_DWELLFORMAT		15
#define	MKNC_NUM_DRILLZPROCESS		16
#define	MKNC_NUM_DRILLPROCESS		17
#define	MKNC_NUM_DRILLSORT			18
#define	MKNC_NUM_DRILLCIRCLEPROCESS	19
#define	MKNC_NUM_MOVEZ				20
#define	MKNC_NUM_TOLERANCE			21
#define	MKNC_NUM_OPTIMAIZEDRILL		22

#define	MKNC_DBL_FEED			0
#define	MKNC_DBL_ZFEED			1
#define	MKNC_DBL_ZG0STOP		2
#define	MKNC_DBL_ZCUT			3
#define	MKNC_DBL_G92X			4
#define	MKNC_DBL_G92Y			5
#define	MKNC_DBL_G92Z			6
#define	MKNC_DBL_ELLIPSE		7
#define	MKNC_DBL_MAKEEND		8
#define	MKNC_DBL_MAKEENDFEED	9
#define	MKNC_DBL_DEEP			10
#define	MKNC_DBL_ZSTEP			11
#define	MKNC_DBL_DEEPFEED		12
#define	MKNC_DBL_DRILLFEED		13
#define	MKNC_DBL_DRILLR			14
#define	MKNC_DBL_DRILLZ			15
#define	MKNC_DBL_DRILLCIRCLE	16
#define	MKNC_DBL_TOLERANCE		17
#define	MKNC_DBL_DRILLMARGIN	18

#define	MKNC_FLG_XREV			0
#define	MKNC_FLG_YREV			1
#define	MKNC_FLG_LINEADD		2
#define	MKNC_FLG_ZEROCUT		3
#define	MKNC_FLG_GCLIP			4
#define	MKNC_FLG_DISABLESPINDLE	5
#define	MKNC_FLG_CIRCLEHALF		6
#define	MKNC_FLG_ELLIPSE		7
#define	MKNC_FLG_DEEP			8
#define	MKNC_FLG_HELICAL		9
#define	MKNC_FLG_DEEPFINISH		10
#define	MKNC_FLG_DRILLMATCH		11
#define	MKNC_FLG_DRILLCIRCLE	12
#define	MKNC_FLG_DRILLBREAK		13
#define	MKNC_FLG_LAYERCOMMENT	14
#define	MKNC_FLG_L0CYCLE		15

#define	MKNC_STR_LINEFORM		0
#define	MKNC_STR_EOB			1
#define	MKNC_STR_CUSTMOVE_B		2
#define	MKNC_STR_CUSTMOVE_A		3
#define	MKNC_STR_HEADER			4
#define	MKNC_STR_FOOTER			5

class CNCMakeOption
{
	CString	m_strInitFile;		// ðŒÌ§²Ù–¼
	int		m_nOrderLength;		// Å‘å–½—ß’·

// ØíÊß×Ò°ÀÝ’è‚ÌÀÞ²±Û¸Þ‚Í‚¨—F’B
friend class CMKNCSetup;
friend class CMKNCSetup1;
friend class CMKNCSetup2;
friend class CMKNCSetup3;
friend class CMKNCSetup4;
friend class CMKNCSetup5;
friend class CMKNCSetup6;
friend class CMKNCSetup8;

	// intŒ^µÌß¼®Ý
	union {
		struct {
			int		m_nSpindle,			// ŽåŽ²‰ñ“]‘¬“x
			// -----
					m_nLineAdd,			// s”Ô†‘‰Á
					m_nG90,				// ˆÊ’uŽw’è(G90 or G91)
					m_nZReturn,			// ZŽ²‚Ì•œ‹A(Initial or R)
					m_nDot,				// ”’l•\‹L(¬”“_ or 1/1000)
					m_nFDot,			// ‚eÊß×Ò°À‚Ì”’l•\‹L
					m_nCircleCode,		// ‰~Øí(G2 or G3)
					m_nIJ,				// ‰~ŒÊ•âŠÔ‚ÉR‚©I/J/K
			// -----
					m_nMakeEnd,			// ‰ÁHÏ‚Ý[‚³‚ÌŽwŽ¦
					m_nDeepSpindle,		// [’¤Ždã‚°‰ñ“]”
					m_nDeepZProcess,	// [’¤Ždã‚°‚©‚ç‚ÌZŽ²•œ‹A
					m_nDeepAProcess,	// [’¤‚ÌØíŽè‡
					m_nDeepCProcess,	// [’¤‚ÌØí•ûŒü
			// -----
					m_nDrillSpindle,	// ŒŠ‚ ‚¯‰ñ“]”
					m_nDwell,			// ÄÞ³ªÙŽžŠÔ
					m_nDwellFormat,		// ÄÞ³ªÙŽžŠÔ‚Ì•\‹L
					m_nDrillZProcess,	// ŒŠ‚ ‚¯‚ÌZŽ²•œ‹A
					m_nDrillProcess,	// ŒŠ‚ ‚¯‚ÌŽd•û
					m_nDrillSort,		// ŒŠ‚ ‚¯‚Ì¸ÞÙ°ËßÝ¸Þ‡˜
					m_nDrillCircleProcess,	// ‰~ÃÞ°À‚ðŠÜ‚Þ‚Æ‚«‚ÌŒŠ‚ ‚¯‡˜
			// -----
					m_nMoveZ,			// ˆÚ“®Ú²Ô‚ÌZŽ²
			// -----
					m_nTolerance,		// ‹–—e·‚ð’´‚¦‚½‚Æ‚«‚Ì“®ì
					m_nOptimaizeDrill;	// ŒŠ‰ÁH‚ÌŠî€Ž²
		};
		int			m_unNums[23];
	};
	// doubleŒ^µÌß¼®Ý
	union {
		struct {
			double	m_dFeed,			// Øí‘—‚è‘¬“x
					m_dZFeed,			// ZŽ²‚ð‰º‚°‚é‚Æ‚«‚Ì‘—‚è‘¬“x
					m_dZG0Stop,			// G0‚Å“®‚©‚·ZˆÊ’u(R“_)
					m_dZCut,			// ZŽ²‚Ì‰º‚°ˆÊ’u(Ø‚èž‚Ý)
					m_dG92[NCXYZ],		// G92‚ÌX/Y/Z
			// -----
					m_dEllipse,			// ‘È‰~Œö·
			// -----
					m_dMakeValue,		// ‰ÁHÏ‚Ý[‚³‚ÌµÌ¾¯ÄorŒÅ’èZ’l
					m_dMakeFeed,		// ‰ÁHÏ‚ÝZ‘—‚è‘¬“x
					m_dDeep,			// [’¤‚ÌÅIØ‚èž‚Ý
					m_dZStep,			// [’¤Øí‚Ì‚½‚ß‚Ì½Ã¯Ìß
					m_dDeepFeed,		// [’¤Ždã‚°‘—‚è
			// -----
					m_dDrillFeed,		// ŒŠ‚ ‚¯‘—‚è
					m_dDrillR,			// ŒŠ‰ÁHR“_
					m_dDrillZ,			// ŒŠ‰ÁHØ‚èž‚Ý
					m_dDrillCircle,		// ŒŠ‰ÁH‚ÉŒ©—§‚Ä‚é‰~ÃÞ°À‚Ì”¼Œa
			// -----
					m_dTolerance,		// “¯ˆêÀ•W‚ÆŒ©‚È‚·‹–—e·
					m_dDrillMargin;		// Šî€Ž²‚É‘Î‚·‚é‹–—e·
		};
		double		m_udNums[19];
	};
	// BOOLŒ^µÌß¼®Ý
	union {
		struct {
			BOOL	m_bXrev,			// XŽ²”½“]
					m_bYrev,			// YŽ²”½“]
					m_bLineAdd,			// s”Ô†
					m_bZeroCut,			// ¬”“_ˆÈ‰º‚Ì¾ÞÛ¶¯Ä
					m_bGclip,			// Gº°ÄÞÈ—ªŒ`
					m_bDisableSpindle,	// SÊß×Ò°À‚ð¶¬‚µ‚È‚¢
					m_bCircleHalf,		// ‘S‰~‚Í•ªŠ„
			// -----
					m_bEllipse,			// ’·Œa‚Æ’ZŒa‚ª“™‚µ‚¢‘È‰~‚Í‰~‚Æ‚Ý‚È‚·
			// -----
					m_bDeep,			// [’¤Øí‚ðs‚¤
					m_bHelical,			// ‰~ÃÞ°À‚ðÍØ¶ÙØí
					m_bDeepFinish,		// Ždã‚°µÌß¼®Ý“K—p‚©
			// -----
					m_bDrillMatch,		// ŒŠ‰ÁH“¯ˆêÀ•W‚Í–³Ž‹
					m_bDrillCircle,		// ‰~ÃÞ°À‚àŒŠ‰ÁHÃÞ°À‚ÆŒ©‚È‚·
					m_bDrillBreak,		// ‘å‚«‚³‚²‚Æ‚ÉºÒÝÄ‚ð–„‚ßž‚Þ
			// -----
					m_bLayerComment,	// Ú²Ô‚²‚Æ‚ÉºÒÝÄ‚ð–„‚ßž‚Þ
					m_bL0Cycle;			// ŒÅ’è»²¸Ù’†‚ÍL0o—Í
		};
		BOOL		m_ubFlags[16];
	};
	// CStringŒ^µÌß¼®Ý
	CString		m_strOption[6];			// s”Ô†Ì«°Ï¯Ä, EOB, ¶½ÀÑˆÚ“®º°ÄÞ(‘OŒã)
										// ¶½ÀÑÍ¯ÀÞ°C¶½ÀÑÌ¯À°

	BOOL	Convert();				// ‹ŒÊÞ°¼Þ®ÝŒÝŠ·—p
	void	InitialDefault(void);	// ÃÞÌ«ÙÄÝ’è
	CString	GetInsertSpace(int nLen) {
		CString	strResult(' ', m_nOrderLength - nLen);
		return strResult;
	}

public:
	CNCMakeOption(LPCTSTR);

	BOOL	ReadMakeOption(LPCTSTR);	// ðŒÌ§²Ù‚Ì“Ç‚Ýž‚Ý
	BOOL	SaveMakeOption(LPCTSTR = NULL);	// ðŒÌ§²Ù‚Ì‘‚«o‚µ

	CString	GetInitFile(void) const {
		return m_strInitFile;
	}
	int		GetNum(size_t n) const {		// ”ŽšµÌß¼®Ý
		ASSERT( n>=0 && n<SIZEOF(m_unNums) );
		return m_unNums[n];
	}
	double	GetDbl(size_t n) const {		// ”ŽšµÌß¼®Ý
		ASSERT( n>=0 && n<SIZEOF(m_udNums) );
		return m_udNums[n];
	}
	BOOL	GetFlag(size_t n) const {		// Ì×¸ÞµÌß¼®Ý
		ASSERT( n>=0 && n<SIZEOF(m_ubFlags) );
		return m_ubFlags[n];
	}
	CString	GetStr(size_t n) const {		// •¶Žš—ñµÌß¼®Ý
		ASSERT( n>=0 && n<SIZEOF(m_strOption) );
		return m_strOption[n];
	}

#ifdef _DEBUGOLD
	void	DbgDump(void) const;		// µÌß¼®Ý•Ï”‚ÌÀÞÝÌß
#endif
};
