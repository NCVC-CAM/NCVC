// DXFMakeOption.h: DXFo—ÍµÌß¼®Ý‚ÌŠÇ—
//
//////////////////////////////////////////////////////////////////////

#pragma once

#define	MKDX_NUM_LTYPE_O		0
#define	MKDX_NUM_LTYPE_C		1
#define	MKDX_NUM_LTYPE_M		2
#define	MKDX_NUM_LTYPE_H		3
#define	MKDX_NUM_LCOL_O			4
#define	MKDX_NUM_LCOL_C			5
#define	MKDX_NUM_LCOL_M			6
#define	MKDX_NUM_LCOL_H			7
#define	MKDX_NUM_PLANE			8
#define	MKDX_NUM_CYCLE			9

#define	MKDX_DBL_ORGLENGTH		0
#define	MKDX_DBL_CYCLER			1

#define	MKDX_FLG_ORGCIRCLE		0
#define	MKDX_FLG_ORGCROSS		1

#define	MKDX_STR_ORIGIN			0
#define	MKDX_STR_CAMLINE		1
#define	MKDX_STR_MOVE			2
#define	MKDX_STR_CORRECT		3

class CDXFMakeOption  
{
// Êß×Ò°ÀÝ’è‚ÌÀÞ²±Û¸Þ‚Í‚¨—F’B
friend class CMakeDXFDlg1;
friend class CMakeDXFDlg2;

	// intŒ^µÌß¼®Ý
	union {
		struct {
			int		m_nLType[4],	// ŠeÚ²Ô‚ÌüŽí
					m_nLColor[4],	// @V ‚ÌF
					m_nPlane,		// •½–ÊŽw’è
					m_nCycle;		// ŒÅ’è»²¸Ùo—ÍÀ²Ìß
		};
		int			m_unNums[10];
	};
	// doubleŒ^µÌß¼®Ý
	union {
		struct {
			double	m_dOrgLength,	// Œ´“_’·‚³(Œa)
					m_dCycleR;		// ŒÅ’è»²¸Ù‰~o—Í‚ÌŒa
		};
		double		m_udNums[2];
	};
	// BOOLŒ^µÌß¼®Ý
	union {
		struct {
			BOOL	m_bOrgCircle,	// Œ´“_‰~o—Í
					m_bOrgCross;	// Œ´“_\Žšo—Í
		};
		BOOL		m_ubFlags[2];
	};
	// CStringŒ^µÌß¼®Ý
	CString		m_strOption[4];		// ŠeŽíÚ²Ô

public:
	CDXFMakeOption();
	BOOL	SaveDXFMakeOption(void);		// Ú¼Þ½ÄØ‚Ö‚Ì•Û‘¶

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
};
