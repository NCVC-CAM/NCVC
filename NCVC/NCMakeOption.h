// NCMakeOption.h: NC¶¬µÌß¼®Ý‚ÌÍÞ°½¸×½
//
//////////////////////////////////////////////////////////////////////

#pragma once

// •Û‘¶‚ÉŠÖ‚·‚éî•ñ
enum	ENORDERTYPE {	// –½—ß‚ÌŒ^
	NC_PAGE,	// Íß°¼Þ‹æØ‚è
	NC_NUM, NC_DBL, NC_FLG, NC_STR
};
typedef	struct	tagSAVEORDER {
	ENORDERTYPE		enType;
	int				nID;
	LPCTSTR			lpszComment;
} SAVEORDER, *LPSAVEORDER;

//////////////////////////////////////////////////////////////////////

class CNCMakeOption
{
	CString	m_strInitFile;		// ðŒÌ§²Ù–¼
	int		m_nOrderLength;		// Å‘å–½—ß’·

	int		m_nOrderCnt[4];		// int, double, BOOL, CStringŒ^‚Ì–½—ß”
	// intŒ^µÌß¼®Ý
	LPCTSTR*		m_szNOrder;		// –½—ßŒê
	const int*		m_dfNOrder;		// ÃÞÌ«ÙÄ’l
	int*			m_pnNums;		// Ši”[•Ï”
	// doubleŒ^µÌß¼®Ý
	LPCTSTR*		m_szDOrder;
	const double*	m_dfDOrder;
	double*			m_pdNums;
	// BOOLŒ^µÌß¼®Ý
	LPCTSTR*		m_szBOrder;
	const BOOL*		m_dfBOrder;
	BOOL*			m_pbFlags;
	// CStringŒ^µÌß¼®Ý
	LPCTSTR*		m_szSOrder;
	LPCTSTR*		m_dfSOrder;

	// SaveMakeOption() ºÒÝÄî•ñ‘¼
	int				m_nComment;
	LPCTSTR*		m_szComment;
	int				m_nSaveOrder;
	LPSAVEORDER		m_pSaveOrder;

protected:
	CStringArray	m_strOption;	// CStringŒ^‚¾‚¯‚ÍÍÞ°½¸×½‚ÅŽÀ‘Ì‚ðŽ‚Â

	CNCMakeOption(
		int, LPCTSTR*, const int*,    int*,		// intŒ^
		int, LPCTSTR*, const double*, double*,	// doubleŒ^
		int, LPCTSTR*, const BOOL*,   BOOL*,	// BOOLŒ^
		int, LPCTSTR*, LPCTSTR*,				// CStringŒ^
		int, LPCTSTR*, int, LPSAVEORDER);		// SaveMakeOption()î•ñ

	CString	GetInsertSpace(int nLen) {
		CString	strResult(' ', m_nOrderLength - nLen);
		return strResult;
	}
	void	InitialDefault(void);	// ÃÞÌ«ÙÄÝ’è

public:
	CString	GetInitFile(void) const {
		return m_strInitFile;
	}
	BOOL	ReadMakeOption(LPCTSTR);
	BOOL	SaveMakeOption(LPCTSTR = NULL);

	int		GetNum(int n) const {		// ®”’lµÌß¼®Ý
		ASSERT( n>=0 && n<m_nOrderCnt[0] );
		return m_pnNums[n];
	}
	double	GetDbl(int n) const {		// ŽÀ”’lµÌß¼®Ý
		ASSERT( n>=0 && n<m_nOrderCnt[1] );
		return m_pdNums[n];
	}
	BOOL	GetFlag(int n) const {		// Ì×¸ÞµÌß¼®Ý
		ASSERT( n>=0 && n<m_nOrderCnt[2] );
		return m_pbFlags[n];
	}
	CString	GetStr(int n) const {		// •¶Žš—ñµÌß¼®Ý
		ASSERT( n>=0 && n<m_strOption.GetCount() );
		return m_strOption[n];
	}

#ifdef _DEBUGOLD
	virtual	void DbgDump(void) const = 0;	// µÌß¼®Ý•Ï”‚ÌÀÞÝÌß(”h¶—p)
#endif
};
