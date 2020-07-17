// MCOption.h: Hì‹@ŠBµÌß¼®İ‚ÌŠÇ—
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "NCVCdefine.h"
// ModalGroup
#define	MODALGROUP	5
#define	MODALGROUP0	0	// G00`G03
#define	MODALGROUP1	1	// G17`G19
#define	MODALGROUP2	2	// G54`G59
#define	MODALGROUP3	3	// G90CG91
#define	MODALGROUP4	4	// G98CG99
// G54`G59
#define	WORKOFFSET		6
// Ï¸ÛŠÖ˜A
#define	MCMACROSTRING		4
#define	MCMACROCODE			0	// ŒÄ‚Ño‚µº°ÄŞ
#define	MCMACROFOLDER		1	// Ì«ÙÀŞ
#define	MCMACROIF			2	// I/F
#define	MCMACROARGV			3	// ˆø”
#define	MCMACRORESULT		4	// o—ÍŒ‹‰Ê
#define	MCMACHINEFILE		5	// Œ»İ‚Ì‹@ŠBî•ñÌ§²Ù–¼
#define	MCCURRENTFOLDER		6	// Œ»İ‚ÌNCÌ§²ÙÌ«ÙÀŞ
// BOOLŒ^
#define	MC_FLG_L0CYCLE		0
// H‹ï•â³À²Ìß
#define	MC_TYPE_A			0
#define	MC_TYPE_B			1

// H‹ïî•ñ
#define	MCTOOLINFOOPT	4	// MCTOOLINFO ‚ÌµÌß¼®İ‹æØ‚è”
class CMCTOOLINFO
{
friend	class	CMCOption;
friend	class	CMCSetup3;

	BOOL	m_bDlgAdd, m_bDlgDel;
	int		m_nTool;
	CString	m_strName;
	double	m_dToolD, m_dToolH;

public:
	CMCTOOLINFO(void) {
		ClearOption();
	}
	CMCTOOLINFO(int nTool, const CString& strName, double dToolD, double dToolH,
			BOOL bDlgAdd = FALSE) {
		m_bDlgAdd	= bDlgAdd;
		m_bDlgDel	= FALSE;
		m_nTool		= nTool;
		m_strName	= strName;
		m_dToolD	= dToolD;
		m_dToolH	= dToolH;
	}

	void	ClearOption(void) {
		m_bDlgAdd = m_bDlgDel = FALSE;
		m_nTool = 0;
		m_strName.Empty();
		m_dToolD = m_dToolH = 0;
	}
};

class CMCOption
{
friend	class	CMCSetup1;
friend	class	CMCSetup2;
friend	class	CMCSetup3;
friend	class	CMCSetup4;
friend	class	CMCSetup5;

	CStringList	m_strMCList;	// ‹@ŠBî•ñÌ§²Ù—š—ğ

	// intŒ^µÌß¼®İ
	union {
		struct {
			int		m_nModal[MODALGROUP],	// Ó°ÀŞÙİ’è
					m_nG0Speed[NCXYZ],		// ˆÊ’uŒˆ‚ß(G0)ˆÚ“®‘¬“x
					m_nFDot,				// ”F¯ 0:sec 1:msec
					m_nCorrectType;			// •â³À²Ìß
		};
		int			m_unNums[10];
	};
	// doubleŒ^µÌß¼®İ
	union {
		struct {
			double	m_dFeed,				// È—ª‚ÌØí‘¬“x
					m_dInitialXYZ[NCXYZ],	// XYZ‰Šú’l
					m_dBlock,				// 1ÌŞÛ¯¸ˆ—ŠÔ
					m_dWorkOffset[WORKOFFSET][NCXYZ];	// G54`G59
		};
		double		m_udNums[23];
	};
	// BOOLŒ^µÌß¼®İ
	union {
		struct {
			BOOL	m_bL0Cycle;		// ŒÅ’è»²¸Ù’†‚ÌL0“®ì
		};
		BOOL		m_ubFlgs[1];
	};
	// CStringŒ^µÌß¼®İ
	CString		m_strMCname,	// ‹@ŠB–¼
				m_strMacroOpt[MCMACROSTRING];	// Ï¸ÛŠÖŒW
	// H‹ïî•ñµÌß¼®İ
	CTypedPtrList<CPtrList, CMCTOOLINFO*>	m_ltTool;	// CMCTOOLINFOŒ^‚ğŠi”[

	void	Convert(void);			// Ú¼Ş½ÄØ‚©‚çÌ§²Ù‚Ö & ‹ŒÊŞ°¼Ş®İ‚ÌÚ¼Ş½ÄØ‚ğÁ‹
	void	ConvertWorkOffset(size_t, LPCTSTR);
	BOOL	AddMCListHistory(LPCTSTR);	// —š—ğXV

public:
	CMCOption();
	~CMCOption();
	BOOL	ReadMCoption(LPCTSTR, BOOL = TRUE);
	BOOL	SaveMCoption(LPCTSTR);

	BOOL	GetFlag(size_t n) const {		// Ì×¸ŞµÌß¼®İ
		ASSERT( n>=0 && n<SIZEOF(m_ubFlgs) );
		return m_ubFlgs[n];
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
	double	GetBlockTime(void) const {
		return m_dBlock;
	}
	int		GetFDot(void) const {
		return m_nFDot;
	}
	double	GetFeed(void) const {
		return m_dFeed;
	}
	double	GetInitialXYZ(size_t n) const {
		ASSERT( n>=0 && n<NCXYZ );
		return m_dInitialXYZ[n];
	}
	CPoint3D	GetWorkOffset(size_t n) const {
		ASSERT( n>=0 && n<WORKOFFSET );
		return	CPoint3D(m_dWorkOffset[n][NCA_X], m_dWorkOffset[n][NCA_Y], m_dWorkOffset[n][NCA_Z]);
	}
	CString	GetMacroStr(int n) const {
		ASSERT( n>=0 && n<SIZEOF(m_strMacroOpt) );
		return m_strMacroOpt[n];
	}
	BOOL	IsMacroSearch(void) const {
		return !(m_strMacroOpt[MCMACROCODE].IsEmpty() | m_strMacroOpt[MCMACROIF].IsEmpty());
	}
	CString	MakeMacroCommand(int) const;
	CString	GetDefaultOption(void) const;	// from MCSetup4.cpp

	int		GetCorrectType(void) const {
		return m_nCorrectType;
	}
	boost::optional<double>	GetToolD(int) const;
	boost::optional<double>	GetToolH(int) const;
	BOOL	AddTool(int, double, BOOL);	// from NCDoc.cpp(G10)
	void	ReductionTools(BOOL);

	void	AddMCHistory_ComboBox(CComboBox&);
};
