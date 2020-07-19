// NCMakeOption.h: NC������߼�݂��ް��׽
//
//////////////////////////////////////////////////////////////////////

#pragma once

// �ۑ��Ɋւ�����
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
// ��߼�ݓ����Ǘ�
struct	NCMAKEOPTION
{
	int				nOrderCnt;
	LPCTSTR*		pszOrder;
};

//////////////////////////////////////////////////////////////////////

class CNCMakeOption
{
	CString	m_strInitFile;		// ����̧�ٖ�
	int		m_nOrderLength;		// �ő喽�ߒ�

	// ��߼�݊Ǘ�
	NCMAKEOPTION	m_MakeOpt[NC_MAXOD];	// int, float, BOOL, CString�^

	// SaveMakeOption() ���ď��
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
		int, LPCTSTR*, int, LPSAVEORDER);	// SaveMakeOption()���
	virtual	void	InitialDefault(void) = 0;	// ��̫�Đݒ�(�e�h���׽�ɂ�)
	virtual	BOOL	IsPathID(int) = 0;			// ̧���߽ID���ۂ�

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

	int		GetNum(int n) const {		// �����l��߼��
		ASSERT( n>=0 && n<m_MakeOpt[NC_NUM].nOrderCnt );
		return m_pIntOpt[n];
	}
	float	GetDbl(int n) const {		// �����l��߼��
		ASSERT( n>=0 && n<m_MakeOpt[NC_DBL].nOrderCnt );
		return m_pDblOpt[n];
	}
	BOOL	GetFlag(int n) const {		// �׸޵�߼��
		ASSERT( n>=0 && n<m_MakeOpt[NC_FLG].nOrderCnt );
		return m_pFlgOpt[n];
	}
	CString	GetStr(int n) const {		// �������߼��
		ASSERT( n>=0 && n<m_MakeOpt[NC_STR].nOrderCnt );
		ASSERT( n>=0 && n<m_strOption.GetCount() );
		return m_strOption[n];
	}
	virtual	CString	GetLineNoForm(void)const = 0;	// �s�ԍ�����

#ifdef _DEBUG
	virtual	void DbgDump(void) const = 0;	// ��߼�ݕϐ��������(�h���p)
#endif
};
