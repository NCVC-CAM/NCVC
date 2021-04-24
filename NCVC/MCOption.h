// MCOption.h: çHçÏã@äBµÃﬂºÆ›ÇÃä«óù
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCVCdefine.h"

// ModalGroup
enum {
	MODALGROUP0 = 0,	// G00Å`G03
	MODALGROUP1,		// G17Å`G19
	MODALGROUP2,		// G54Å`G59
	MODALGROUP3,		// G90ÅCG91
	MODALGROUP4,		// G98ÅCG99
		MODALGROUP		// [5]
};
enum {		// intå^
	MC_INT_FDOT = MODALGROUP+NCXYZ,
	MC_INT_CORRECTTYPE,
	MC_INT_FORCEVIEWMODE,
		MC_INT_NUMS		// [11]
};
enum {		// floatå^
	MC_DBL_FEED = NCXYZ,
	MC_DBL_BLOCKWAIT,
	MC_DBL_DEFWIREDEPTH,
		MC_DBL_AAA		// [6](dummy)
};
#define	WORKOFFSET		6	// G54Å`G59
const	size_t	MC_DBL_NUMS = MC_DBL_AAA+WORKOFFSET*NCXYZ;	// 24
enum {		// BOOLå^
	MC_FLG_OBS0 = 0,	// µÃﬂºÆ≈ŸÃﬁ€Ø∏Ω∑ØÃﬂ
	MC_FLG_OBS1,
	MC_FLG_OBS2,
	MC_FLG_OBS3,
	MC_FLG_OBS4,
	MC_FLG_OBS5,
	MC_FLG_OBS6,
	MC_FLG_OBS7,
	MC_FLG_OBS8,
	MC_FLG_OBS9,
	MC_FLG_L0CYCLE,		// å≈íËª≤∏ŸÇÃL0éwíË
		MC_FLG_NUMS		// [11]
};
enum {		// œ∏€ä÷òA(CStringå^)
	MCMACROCODE = 0,	// åƒÇ—èoÇµ∫∞ƒﬁ
	MCMACROFOLDER,		// Ã´Ÿ¿ﬁ
	MCMACROIF,			// I/F
	MCMACROARGV,		// à¯êî
		MCMACROSTRING	// [4]
};
enum {		// íuä∑óp
	MCMACRORESULT = MCMACROSTRING,	// èoóÕåãâ 
	MCMACHINEFILE,					// åªç›ÇÃã@äBèÓïÒÃß≤Ÿñº
	MCCURRENTFOLDER,				// åªç›ÇÃNCÃß≤ŸÃ´Ÿ¿ﬁ
};
// çHãÔï‚ê≥¿≤Ãﬂ
enum {
	MC_TYPE_A = 0,
	MC_TYPE_B
};
// ã≠êßï\é¶”∞ƒﬁ(MC_INT_FORCEVIEWMODE)
enum {
	MC_VIEWMODE_MILL = 0,
	MC_VIEWMODE_LATHE,
	MC_VIEWMODE_WIRE
};

// çHãÔèÓïÒ
enum {
	MCTOOL_T = 0,
	MCTOOL_NAME,
	MCTOOL_D,
	MCTOOL_H,
	MCTOOL_TYPE,
		MCTOOL_NUMS		// [5]
};
class CMCTOOLINFO
{
friend	class	CMCOption;
friend	class	CMCSetup3;

	BOOL	m_bDlgAdd, m_bDlgDel;
	int		m_nTool, m_nType;
	CString	m_strName;
	float	m_dToolD, m_dToolH;

public:
	CMCTOOLINFO(void) {
		ClearOption();
	}
	CMCTOOLINFO(int nTool, const CString& strName, float dToolD, float dToolH, int nType, 
			BOOL bDlgAdd = FALSE) {
		m_bDlgAdd	= bDlgAdd;
		m_bDlgDel	= FALSE;
		m_nTool		= nTool;
		m_strName	= strName;
		m_dToolD	= dToolD;
		m_dToolH	= dToolH;
		m_nType		= nType;
	}

	void	ClearOption(void) {
		m_bDlgAdd = m_bDlgDel = FALSE;
		m_nTool = m_nType = 0;
		m_strName.Empty();
		m_dToolD = m_dToolH = 0;
	}
};
typedef	CTypedPtrList<CPtrList, CMCTOOLINFO*>	CMCTOOLLIST;

class CMCOption
{
friend	class	CMCSetup1;
friend	class	CMCSetup2;
friend	class	CMCSetup3;
friend	class	CMCSetup4;
friend	class	CMCSetup5;
friend	class	COBSdlg;

	CStringList	m_strMCList;	// ã@äBèÓïÒÃß≤Ÿóöó

	// intå^µÃﬂºÆ›
	union {
		struct {
			int		m_nModal[MODALGROUP],	// ”∞¿ﬁŸê›íË
					m_nG0Speed[NCXYZ],		// à íuåàÇﬂ(G0)à⁄ìÆë¨ìx
					m_nFDot,				// îFéØ 0:sec 1:msec
					m_nCorrectType,			// ï‚ê≥¿≤Ãﬂ
					m_nForceViewMode;		// ã≠êßï\é¶”∞ƒﬁ
		};
		int			m_unNums[MC_INT_NUMS];
	};
	// floatå^µÃﬂºÆ›
	union {
		struct {
			float	m_dInitialXYZ[NCXYZ],	// XYZèâä˙íl
					m_dFeed,				// è»ó™éûÇÃêÿçÌë¨ìx
					m_dBlock,				// 1Ãﬁ€Ø∏èàóùéûä‘
					m_dDefWireDepth,		// ‹≤‘â¡çHã@ÇÃ√ﬁÃ´Ÿƒå˙Ç≥
					m_dWorkOffset[WORKOFFSET][NCXYZ];	// G54Å`G59
		};
		float		m_udNums[MC_DBL_NUMS];
	};
	// BOOLå^µÃﬂºÆ›
	union {
		struct {
			BOOL	m_bOBS[10],		// µÃﬂºÆ≈ŸÃﬁ€Ø∏Ω∑ØÃﬂ
					m_bL0Cycle;		// å≈íËª≤∏ŸíÜÇÃL0ìÆçÏ
		};
		BOOL		m_ubFlgs[MC_FLG_NUMS];
	};
	// CStringå^µÃﬂºÆ›
	CString		m_strMCname,	// ã@äBñº
				m_strAutoBreak,	// é©ìÆÃﬁ⁄≤∏ê›íË∫∞ƒﬁ
				m_strMacroOpt[MCMACROSTRING];	// œ∏€ä÷åW
	// çHãÔèÓïÒµÃﬂºÆ›
	CMCTOOLLIST		m_ltTool;	// CMCTOOLINFOå^Çäiî[

	void	Convert(void);			// ⁄ºﬁΩƒÿÇ©ÇÁÃß≤ŸÇ÷ & ãå ﬁ∞ºﬁÆ›ÇÃ⁄ºﬁΩƒÿÇè¡ãé
	void	ConvertWorkOffset(size_t, LPCTSTR);
	BOOL	AddMCListHistory(LPCTSTR);	// óöóçXêV

public:
	CMCOption();
	~CMCOption();
	BOOL	ReadMCoption(LPCTSTR, BOOL = TRUE);
	BOOL	SaveMCoption(LPCTSTR);

	BOOL	GetFlag(size_t n) const {
		ASSERT( n>=0 && n<SIZEOF(m_ubFlgs) );
		return m_ubFlgs[n];
	}
	int		GetInt(size_t n) const {
		ASSERT( n>=0 && n<SIZEOF(m_unNums) );
		return m_unNums[n];
	}
	float	GetDbl(size_t n) const {
		ASSERT( n>=0 && n<SIZEOF(m_udNums) );
		return m_udNums[n];
	}

	const	CStringList*	GetMCList(void) {
		return &m_strMCList;
	}
	CString	GetMCHeadFileName(void) const {
		CString	strResult;
		if ( !m_strMCList.IsEmpty() )
			strResult = m_strMCList.GetHead();
		return strResult;
	}
	int		GetModalSetting(size_t n) const {
		ASSERT( n>=0 && n<MODALGROUP );
		return m_nModal[n];
	}
	int		GetG0Speed(size_t n) const {
		ASSERT( n>=0 && n<NCXYZ );
		return m_nG0Speed[n];
	}
	BOOL	IsZeroG0Speed(void) const {
		if ( GetG0Speed(NCA_X)==0 || GetG0Speed(NCA_Y)==0 || GetG0Speed(NCA_Z)==0 )
			return TRUE;
		return FALSE;
	}
	float	GetInitialXYZ(size_t n) const {
		ASSERT( n>=0 && n<NCXYZ );
		return m_dInitialXYZ[n];
	}
	CPoint3F	GetWorkOffset(size_t n) const {
		ASSERT( n>=0 && n<WORKOFFSET );
		return	CPoint3F(m_dWorkOffset[n][NCA_X], m_dWorkOffset[n][NCA_Y], m_dWorkOffset[n][NCA_Z]);
	}
	CString	GetMacroStr(int n) const {
		ASSERT( n>=0 && n<SIZEOF(m_strMacroOpt) );
		return m_strMacroOpt[n];
	}
	CString	MakeMacroCommand(int) const;
	CString	GetDefaultOption(void) const;	// from MCSetup4.cpp
	CString	GetAutoBreakStr(void) const {
		return m_strAutoBreak;
	}
	boost::optional<float>	GetToolD(int) const;
	boost::optional<float>	GetToolH(int) const;
	int						GetMillType(int) const;
	BOOL	AddTool(int, float, BOOL);	// from NCDoc.cpp(G10)
	void	ReductionTools(BOOL);

	void	AddMCHistory_ComboBox(CComboBox&);
};
