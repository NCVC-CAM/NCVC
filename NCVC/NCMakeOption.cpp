// NCMakeOption.cpp: CNCMakeOption �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "NCMakeOption.h"
#include "NCMakeMillOpt.h"
#include "NCMakeLatheOpt.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif
//#define	_DEBUGOLD
#undef	_DEBUGOLD

/////////////////////////////////////////////////////////////////////////////
// CNCMakeOption �N���X�̍\�z/����

CNCMakeOption::CNCMakeOption(
	int nCnt0, LPCTSTR* pszNOrder, const int*    pdfNOrder, int*     punNums,
	int nCnt1, LPCTSTR* pszDOrder, const double* pdfDOrder, double*  pudNums,
	int nCnt2, LPCTSTR* pszBOrder, const BOOL*   pdfBOrder, BOOL*    pubNums,
	int nCnt3, LPCTSTR* pszSOrder, LPCTSTR*      pdfSOrder,
	int nComment, LPCTSTR* pszComment, int nSaveOrder, LPSAVEORDER pSaveOrder)
{
	int		i, nLen;

	// int�^��߼��
	m_nOrderCnt[0] = nCnt0;
	m_szNOrder	= pszNOrder;
	m_dfNOrder	= pdfNOrder;
	m_pnNums	= punNums;
	// double�^��߼��
	m_nOrderCnt[1] = nCnt1;
	m_szDOrder	= pszDOrder;
	m_dfDOrder	= pdfDOrder;
	m_pdNums	= pudNums;
	// BOOL�^��߼��
	m_nOrderCnt[2] = nCnt2;
	m_szBOrder	= pszBOrder;
	m_dfBOrder	= pdfBOrder;
	m_pbFlags	= pubNums;
	// CString�^��߼��
	m_nOrderCnt[3] = nCnt3;
	m_szSOrder	= pszSOrder;
	m_dfSOrder	= pdfSOrder;
	// SaveMakeOption()���
	m_nComment  = nComment;
	m_szComment = pszComment;
	m_nSaveOrder= nSaveOrder;
	m_pSaveOrder= pSaveOrder;

	// ���ߒ��̌v�Z(GetInsertSpace()�Ŏg�p)
	m_nOrderLength = 0;
	for ( i=0; i<m_nOrderCnt[0]; i++ ) {
		if ( m_nOrderLength < (nLen=lstrlen(m_szNOrder[i])) )
			m_nOrderLength = nLen;
	}
	for ( i=0; i<m_nOrderCnt[1]; i++ ) {
		if ( m_nOrderLength < (nLen=lstrlen(m_szDOrder[i])) )
			m_nOrderLength = nLen;
	}
	for ( i=0; i<m_nOrderCnt[2]; i++ ) {
		if ( m_nOrderLength < (nLen=lstrlen(m_szBOrder[i])) )
			m_nOrderLength = nLen;
	}
	for ( i=0; i<m_nOrderCnt[3]; i++ ) {
		if ( m_nOrderLength < (nLen=lstrlen(m_szSOrder[i])) )
			m_nOrderLength = nLen;
	}
	m_nOrderLength += 2;	// 2��������߰�
}

/////////////////////////////////////////////////////////////////////////////
// հ�����ފ֐�

void CNCMakeOption::InitialDefault(void)
{
	extern	LPTSTR	g_pszExecDir;	// ���s�ިڸ��(NCVC.cpp)
	int		i;

	for ( i=0; i<m_nOrderCnt[0]; i++ )
		m_pnNums[i] = m_dfNOrder[i];
	for ( i=0; i<m_nOrderCnt[1]; i++ )
		m_pdNums[i] = m_dfDOrder[i];
	for ( i=0; i<m_nOrderCnt[2]; i++ )
		m_pbFlags[i] = m_dfBOrder[i];
	m_strOption.RemoveAll();
	for ( i=0; i<m_nOrderCnt[3]; i++ )
		m_strOption.Add(m_dfSOrder[i]);

	CString	strTmp;
	ASSERT( MKNC_STR_HEADER == MKLA_STR_HEADER );
	for ( i=MKNC_STR_HEADER; i<=MKNC_STR_FOOTER; i++ ) {
		strTmp = m_strOption[i];
		m_strOption[i] = g_pszExecDir + strTmp;
	}
}

BOOL CNCMakeOption::ReadMakeOption(LPCTSTR lpszInitFile)
{
	// ���߂�"="��";"(����)�ŕ���
	typedef boost::tokenizer< boost::escaped_list_separator<TCHAR> > tokenizer;
	static	boost::escaped_list_separator<TCHAR> sep("", "=;", "\"");	// �����ߖ���
	// �؍�����̖��ߌ���(�啶���������͖������邪���߂͊��S��v)
	CStringKeyIndex		stNOrder(m_nOrderCnt[0], m_szNOrder);
	CStringKeyIndex		stDOrder(m_nOrderCnt[1], m_szDOrder);
	CStringKeyIndex		stBOrder(m_nOrderCnt[2], m_szBOrder);
	CStringKeyIndex		stSOrder(m_nOrderCnt[3], m_szSOrder);

#ifdef _DEBUGOLD
	CMagaDbg	dbg("CNCMakeOption::ReadMakeOption()\nStart", DBG_GREEN);
#endif

	// �܂���̫�Ăŏ�����
	InitialDefault();
	if ( !lpszInitFile || lstrlen(lpszInitFile)<=0 ) {
		m_strInitFile.Empty();
		return TRUE;
	}
/*
	NCVC�N�����ɊO����ި��ɂĕύX����Ă���\��������̂ŁC
	���ɕێ����Ă���̧�ٖ��Ɠ������Ă��C���̊֐����Ă΂ꂽ�Ƃ��͕K���ǂݍ���
*/
	m_strInitFile = lpszInitFile;

	tokenizer::iterator it;
	std::string	str, strOrder, strResult;
	tokenizer	tok( str, sep );

	CString	strTmp, strBuf;
	TCHAR	szCurrent[_MAX_PATH], szFile[_MAX_PATH];
	INT_PTR	n;
	BOOL	bResult = TRUE;

	// �����ިڸ�؂� lpszFile �� -> PathSearchAndQualify()
	::GetCurrentDirectory(_MAX_PATH, szCurrent);
	::Path_Name_From_FullPath(m_strInitFile, strTmp/*strPath*/, strBuf/*dummy*/);
	::SetCurrentDirectory(strTmp);

	try {
		CStdioFile	fp(m_strInitFile,
			CFile::modeRead | CFile::shareDenyWrite | CFile::typeText);
		while ( fp.ReadString(strTmp) ) {
			strBuf = strTmp.Trim();
			if ( ::NC_IsNullLine(strBuf) )
				continue;
			// �����������
			str = strBuf;
			tok.assign(str);
#ifdef _DEBUGOLD
			dbg.printf("strBuf   = %s", strBuf);
#endif
			// ���߂ƒl�ɕ���
			it = tok.begin();
			strOrder  = ::Trim(*it);	++it;
			strResult = ::Trim(*it);
			if ( strOrder.empty() )
				continue;
#ifdef _DEBUGOLD
			dbg.printf("strOrder  =%s", strOrder.c_str());
			dbg.printf("strResult =%s", strResult.c_str());
#endif
			// ���ߌ���(int�^)
			n = stNOrder.GetIndex(strOrder.c_str());
			if ( n>=0 && n<m_nOrderCnt[0] ) {
				m_pnNums[n] = strResult.empty() ? 0 : atoi(strResult.c_str());
				continue;
			}
			// ���ߌ���(double�^)
			n = stDOrder.GetIndex(strOrder.c_str());
			if ( n>=0 && n<m_nOrderCnt[1] ) {
				m_pdNums[n] = strResult.empty() ? 0 : atof(strResult.c_str());
				continue;
			}
			// ���ߌ���(BOOL�^)
			n = stBOrder.GetIndex(strOrder.c_str());
			if ( n>=0 && n<m_nOrderCnt[2] ) {
				m_pbFlags[n] = strResult.empty() ? FALSE :
						atoi(strResult.c_str())!=0 ? TRUE : FALSE;
				continue;
			}
			// ���ߌ���(CString�^)
			n = stSOrder.GetIndex(strOrder.c_str());
			if ( n>=0 && n<m_nOrderCnt[3] ) {
				if ( n==MKNC_STR_HEADER || n==MKNC_STR_FOOTER ) {
					// �����߽�Ȃ����߽�ɕϊ�
					if ( !strResult.empty() &&
								::PathIsRelative(strResult.c_str()) &&		// Shlwapi.h
								::PathSearchAndQualify(strResult.c_str(), szFile, _MAX_PATH) )
						strResult = szFile;
				}
//				m_strOption.SetAtGrow(n, strResult.c_str());	// InitialDefault()������̂�
				m_strOption[n] = strResult.c_str();				// �����OK
				continue;
			}
		}	// End of while
	}
	catch (CFileException* e) {
		strBuf.Format(IDS_ERR_DXF2NCDINIT, m_strInitFile);
		AfxMessageBox(strBuf, MB_OK|MB_ICONSTOP);
		e->Delete();
		bResult = FALSE;
	}

	// �����ިڸ�؂����ɖ߂�
	::SetCurrentDirectory(szCurrent);

	return bResult;
}

BOOL CNCMakeOption::SaveMakeOption(LPCTSTR lpszInitFile)
{
	int		i, n, nLen;
	CString	strBuf, strResult;

#ifdef _DEBUG
	CMagaDbg	dbg("CNCMakeOption::SaveMakeOption()\nStart", DBG_GREEN);
#endif

	// �V�K�ۑ��̏ꍇ
	if ( lpszInitFile )
		m_strInitFile = lpszInitFile;
	ASSERT( !m_strInitFile.IsEmpty() );

	try {
		CStdioFile	fp(m_strInitFile,
			CFile::modeCreate | CFile::modeWrite | CFile::shareDenyNone | CFile::typeText);
		// ����ͯ�ް
		for ( i=0; i<m_nComment; i++ )
			fp.WriteString(m_szComment[i]);
		fp.WriteString(m_szComment[0]);
		// ���߂̏����o��
		for ( i=0; i<m_nSaveOrder; i++ ) {
			n = m_pSaveOrder[i].nID;
			switch ( m_pSaveOrder[i].enType ) {
			case NC_PAGE:	// �߰��ͯ�ް
				strBuf.Format("#--> Page:%d\n", n);
				break;
			case NC_NUM:	// int�^
				strBuf.Format("%s%s= %8d     ; %s\n",
					m_szNOrder[n], GetInsertSpace(lstrlen(m_szNOrder[n])),
					m_pnNums[n],
					m_pSaveOrder[i].lpszComment);
				break;
			case NC_DBL:	// double�^
				strBuf.Format("%s%s= %12.3f ; %s\n",
					m_szDOrder[n], GetInsertSpace(lstrlen(m_szDOrder[n])),
					m_pdNums[n],
					m_pSaveOrder[i].lpszComment);
				break;
			case NC_FLG:	// BOOL�^
				strBuf.Format("%s%s= %8d     ; %s\n",
					m_szBOrder[n], GetInsertSpace(lstrlen(m_szBOrder[n])),
					m_pbFlags[n] ? 1 : 0,
					m_pSaveOrder[i].lpszComment);
				break;
			case NC_STR:	// CString�^
				if ( n==MKNC_STR_HEADER || n==MKNC_STR_FOOTER ) {
					// ����ٰ��߽�Ȃ瑊���߽�ɕϊ�
					if ( !m_strOption[n].IsEmpty() && ::PathIsSameRoot(m_strInitFile, m_strOption[n]) )
						strResult = ::RelativePath(m_strInitFile, m_strOption[n]);
					else
						strResult = m_strOption[n];
				}
				else
					strResult = m_strOption[n];
				// �����𐮂��邽�߂Ɏc��߰��̌v�Z
				nLen = 11 - strResult.GetLength();
				strBuf.Format("%s%s= \"%s\"%s; %s\n",
					m_szSOrder[n], GetInsertSpace(lstrlen(m_szSOrder[n])),
					strResult, CString(' ', max(1, nLen)),
					m_pSaveOrder[i].lpszComment);
				break;
			default:
				strBuf.Empty();
				break;
			}
			if ( !strBuf.IsEmpty() )
				fp.WriteString(strBuf);
		}
	}
	catch (CFileException* e) {
		strBuf.Format(IDS_ERR_DXF2NCDINIT, m_strInitFile);
		AfxMessageBox(strBuf, MB_OK|MB_ICONSTOP);
		e->Delete();
		return FALSE;
	}

	return TRUE;
}
