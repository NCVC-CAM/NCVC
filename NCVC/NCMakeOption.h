// NCMakeOption.h: NC¶¬µÌß¼®Ý‚ÌÍÞ°½¸×½
//
//////////////////////////////////////////////////////////////////////

#pragma once

// •Û‘¶‚ÉŠÖ‚·‚éî•ñ
enum	ENORDERTYPE {
	NC_PAGE = -1,
	NC_NUM = 0, NC_DBL, NC_FLG, NC_STR,
		NC_MAXOD	// 4
};
struct	SAVEORDER
{
	ENORDERTYPE		enType;
	int				nID;
	LPCTSTR			lpszComment;
};
typedef	SAVEORDER*	LPSAVEORDER;
// µÌß¼®Ý“‡ŠÇ—
struct	NCMAKEOPTION
{
	int				nOrderCnt;
	LPCTSTR*		pszOrder;
};

//////////////////////////////////////////////////////////////////////

class CNCMakeOption
{
	CString	m_strInitFile;		// ðŒÌ§²Ù–¼
	int		m_nOrderLength;		// Å‘å–½—ß’·

	// µÌß¼®ÝŠÇ—
	NCMAKEOPTION	m_MakeOpt[NC_MAXOD];	// int, float, BOOL, CStringŒ^

	// SaveMakeOption() ºÒÝÄî•ñ‘¼
	int				m_nComment;
	LPCTSTR*		m_szComment;
	int				m_nSaveOrder;
	LPSAVEORDER		m_pSaveOrder;

protected:
	int*			m_pIntOpt;
	float*			m_pDblOpt;
	BOOL*			m_pFlgOpt;
	CStringArray	m_strOption;

	CNCMakeOption(NCMAKEOPTION[],
		int, LPCTSTR*, int, LPSAVEORDER);	// SaveMakeOption()î•ñ
	virtual	void	InitialDefault(void) = 0;	// ÃÞÌ«ÙÄÝ’è(Še”h¶¸×½‚É‚Ä)
	virtual	BOOL	IsPathID(int) = 0;			// Ì§²ÙÊß½ID‚©”Û‚©

	CString	GetInsertSpace(int nLen) {
		CString	strResult(' ', m_nOrderLength - nLen);
		return strResult;
	}

public:
	virtual	~CNCMakeOption();
	CString	GetInitFile(void) const {
		return m_strInitFile;
	}
	BOOL	ReadMakeOption(LPCTSTR);
	BOOL	SaveMakeOption(LPCTSTR = NULL);

	int		GetNum(int n) const {		// ®”’lµÌß¼®Ý
		ASSERT( n>=0 && n<m_MakeOpt[NC_NUM].nOrderCnt );
		return m_pIntOpt[n];
	}
	float	GetDbl(int n) const {		// ŽÀ”’lµÌß¼®Ý
		ASSERT( n>=0 && n<m_MakeOpt[NC_DBL].nOrderCnt );
		return m_pDblOpt[n];
	}
	BOOL	GetFlag(int n) const {		// Ì×¸ÞµÌß¼®Ý
		ASSERT( n>=0 && n<m_MakeOpt[NC_FLG].nOrderCnt );
		return m_pFlgOpt[n];
	}
	CString	GetStr(int n) const {		// •¶Žš—ñµÌß¼®Ý
		ASSERT( n>=0 && n<m_MakeOpt[NC_STR].nOrderCnt );
		ASSERT( n>=0 && n<m_strOption.GetCount() );
		return m_strOption[n];
	}
	virtual	CString	GetLineNoForm(void)const = 0;	// s”Ô†‘Ž®

#ifdef _DEBUG
	virtual	void DbgDump(void) const = 0;	// µÌß¼®Ý•Ï”‚ÌÀÞÝÌß(”h¶—p)
#endif
};
