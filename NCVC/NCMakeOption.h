// NCMakeOption.h: NC������߼�݂��ް��׽
//
//////////////////////////////////////////////////////////////////////

#pragma once

// �ۑ��Ɋւ�����
enum	ENORDERTYPE {	// ���߂̌^
	NC_PAGE,	// �߰�ދ�؂�
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
	CString	m_strInitFile;		// ����̧�ٖ�
	int		m_nOrderLength;		// �ő喽�ߒ�

	int		m_nOrderCnt[4];		// int, double, BOOL, CString�^�̖��ߐ�
	// int�^��߼��
	LPCTSTR*		m_szNOrder;		// ���ߌ�
	const int*		m_dfNOrder;		// ��̫�Ēl
	int*			m_pnNums;		// �i�[�ϐ�
	// double�^��߼��
	LPCTSTR*		m_szDOrder;
	const double*	m_dfDOrder;
	double*			m_pdNums;
	// BOOL�^��߼��
	LPCTSTR*		m_szBOrder;
	const BOOL*		m_dfBOrder;
	BOOL*			m_pbFlags;
	// CString�^��߼��
	LPCTSTR*		m_szSOrder;
	LPCTSTR*		m_dfSOrder;

	// SaveMakeOption() ���ď��
	int				m_nComment;
	LPCTSTR*		m_szComment;
	int				m_nSaveOrder;
	LPSAVEORDER		m_pSaveOrder;

protected:
	CStringArray	m_strOption;	// CString�^�������ް��׽�Ŏ��̂�����

	CNCMakeOption(
		int, LPCTSTR*, const int*,    int*,		// int�^
		int, LPCTSTR*, const double*, double*,	// double�^
		int, LPCTSTR*, const BOOL*,   BOOL*,	// BOOL�^
		int, LPCTSTR*, LPCTSTR*,				// CString�^
		int, LPCTSTR*, int, LPSAVEORDER);		// SaveMakeOption()���

	CString	GetInsertSpace(int nLen) {
		CString	strResult(' ', m_nOrderLength - nLen);
		return strResult;
	}
	void	InitialDefault(void);	// ��̫�Đݒ�

public:
	CString	GetInitFile(void) const {
		return m_strInitFile;
	}
	BOOL	ReadMakeOption(LPCTSTR);
	BOOL	SaveMakeOption(LPCTSTR = NULL);

	int		GetNum(int n) const {		// �����l��߼��
		ASSERT( n>=0 && n<m_nOrderCnt[0] );
		return m_pnNums[n];
	}
	double	GetDbl(int n) const {		// �����l��߼��
		ASSERT( n>=0 && n<m_nOrderCnt[1] );
		return m_pdNums[n];
	}
	BOOL	GetFlag(int n) const {		// �׸޵�߼��
		ASSERT( n>=0 && n<m_nOrderCnt[2] );
		return m_pbFlags[n];
	}
	CString	GetStr(int n) const {		// �������߼��
		ASSERT( n>=0 && n<m_strOption.GetCount() );
		return m_strOption[n];
	}

#ifdef _DEBUGOLD
	virtual	void DbgDump(void) const = 0;	// ��߼�ݕϐ��������(�h���p)
#endif
};
