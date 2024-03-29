// NCMakeOption.h: NC生成ｵﾌﾟｼｮﾝのﾍﾞｰｽｸﾗｽ
//
//////////////////////////////////////////////////////////////////////

#pragma once

// 保存に関する情報
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
// ｵﾌﾟｼｮﾝ統合管理
struct	NCMAKEOPTION
{
	int				nOrderCnt;
	LPCTSTR*		pszOrder;
};

//////////////////////////////////////////////////////////////////////

class CNCMakeOption
{
	CString	m_strInitFile;		// 条件ﾌｧｲﾙ名
	int		m_nOrderLength;		// 最大命令長

	// ｵﾌﾟｼｮﾝ管理
	NCMAKEOPTION	m_MakeOpt[NC_MAXOD];	// int, float, BOOL, CString型

	// SaveMakeOption() ｺﾒﾝﾄ情報他
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
		int, LPCTSTR*, int, LPSAVEORDER);	// SaveMakeOption()情報
	virtual	void	InitialDefault(void) = 0;	// ﾃﾞﾌｫﾙﾄ設定(各派生ｸﾗｽにて)
	virtual	BOOL	IsPathID(int) = 0;			// ﾌｧｲﾙﾊﾟｽIDか否か

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

	int		GetNum(int n) const {		// 整数値ｵﾌﾟｼｮﾝ
		ASSERT( n>=0 && n<m_MakeOpt[NC_NUM].nOrderCnt );
		return m_pIntOpt[n];
	}
	float	GetDbl(int n) const {		// 実数値ｵﾌﾟｼｮﾝ
		ASSERT( n>=0 && n<m_MakeOpt[NC_DBL].nOrderCnt );
		return m_pDblOpt[n];
	}
	BOOL	GetFlag(int n) const {		// ﾌﾗｸﾞｵﾌﾟｼｮﾝ
		ASSERT( n>=0 && n<m_MakeOpt[NC_FLG].nOrderCnt );
		return m_pFlgOpt[n];
	}
	CString	GetStr(int n) const {		// 文字列ｵﾌﾟｼｮﾝ
		ASSERT( n>=0 && n<m_MakeOpt[NC_STR].nOrderCnt );
		ASSERT( n>=0 && n<m_strOption.GetCount() );
		return m_strOption[n];
	}
	virtual	CString	GetLineNoForm(void)const = 0;	// 行番号書式

#ifdef _DEBUG
	virtual	void DbgDump(void) const = 0;	// ｵﾌﾟｼｮﾝ変数のﾀﾞﾝﾌﾟ(派生用)
#endif
};
