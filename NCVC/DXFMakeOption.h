// DXFMakeOption.h: DXFo—ÍµÌß¼®Ý‚ÌŠÇ—
//
//////////////////////////////////////////////////////////////////////

#pragma once

enum {
	MKDX_NUM_LTYPE_O = 0,
	MKDX_NUM_LTYPE_C,
	MKDX_NUM_LTYPE_M,
	MKDX_NUM_LTYPE_H,
	MKDX_NUM_LCOL_O,
	MKDX_NUM_LCOL_C,
	MKDX_NUM_LCOL_M,
	MKDX_NUM_LCOL_H,
	MKDX_NUM_PLANE,
	MKDX_NUM_CYCLE,
		MKDX_NUM_NUMS		// [10]
};
enum {
	MKDX_DBL_ORGLENGTH = 0,
	MKDX_DBL_CYCLER,
		MKDX_DBL_NUMS		// [2]
};
enum {
	MKDX_FLG_OUT_O = 0,
	MKDX_FLG_OUT_C,
	MKDX_FLG_OUT_M,
	MKDX_FLG_OUT_H,
	MKDX_FLG_ORGCIRCLE,
	MKDX_FLG_ORGCROSS,
		MKDX_FLG_NUMS		// [6]
};
enum {
	MKDX_STR_ORIGIN = 0,
	MKDX_STR_CAMLINE,
	MKDX_STR_MOVE,
	MKDX_STR_CORRECT,
		MKDX_STR_NUMS		// [4]
};

class CDXFMakeOption  
{
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
		int			m_unNums[MKDX_NUM_NUMS];
	};
	// floatŒ^µÌß¼®Ý
	union {
		struct {
			float	m_dOrgLength,	// Œ´“_’·‚³(Œa)
					m_dCycleR;		// ŒÅ’è»²¸Ù‰~o—Í‚ÌŒa
		};
		float		m_udNums[MKDX_DBL_NUMS];
	};
	// BOOLŒ^µÌß¼®Ý
	union {
		struct {
			BOOL	m_bOut[4],		// ŠeÚ²Ô‚Ìo—ÍÌ×¸Þ
					m_bOrgCircle,	// Œ´“_‰~o—Í
					m_bOrgCross;	// Œ´“_\Žšo—Í
		};
		BOOL		m_ubFlags[MKDX_FLG_NUMS];
	};
	// CStringŒ^µÌß¼®Ý
	CString		m_strOption[MKDX_STR_NUMS];	// ŠeŽíÚ²Ô

	//
	void	Initialize_Registry(void);
	void	Initialize_Default(void);

public:
	CDXFMakeOption(BOOL bRegist = TRUE);
	BOOL	SaveDXFMakeOption(void);		// Ú¼Þ½ÄØ‚Ö‚Ì•Û‘¶

	int		GetNum(size_t n) const {		// ”ŽšµÌß¼®Ý
		ASSERT( n>=0 && n<SIZEOF(m_unNums) );
		return m_unNums[n];
	}
	float	GetDbl(size_t n) const {		// ”ŽšµÌß¼®Ý
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
