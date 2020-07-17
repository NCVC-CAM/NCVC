// NCMakeOption.cpp: CNCMakeOption クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "NCMakeOption.h"
#include "NCMakeMillOpt.h"
#include "NCMakeLatheOpt.h"

using namespace boost;

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

/////////////////////////////////////////////////////////////////////////////
//	boost::proterty_tree 使えそうやけど、値の横にコメント入れられないので却下
//	他、spiritの構文木とか使えそうやけど、面倒なので tokenizer

/////////////////////////////////////////////////////////////////////////////
// CNCMakeOption クラスの構築/消滅

CNCMakeOption::CNCMakeOption(NCMAKEOPTION makeopt[],
	int nComment, LPCTSTR* pszComment, int nSaveOrder, LPSAVEORDER pSaveOrder)
{
	int		i, j, nLen;

	// int型ｵﾌﾟｼｮﾝ
	m_MakeOpt[0] = makeopt[0];
	m_pIntOpt = new int[m_MakeOpt[0].nOrderCnt];
	// double型ｵﾌﾟｼｮﾝ
	m_MakeOpt[1] = makeopt[1];
	m_pDblOpt = new double[m_MakeOpt[1].nOrderCnt];
	// BOOL型ｵﾌﾟｼｮﾝ
	m_MakeOpt[2] = makeopt[2];
	m_pFlgOpt = new BOOL[m_MakeOpt[2].nOrderCnt];
	// CString型ｵﾌﾟｼｮﾝ
	m_MakeOpt[3] = makeopt[3];
	// SaveMakeOption()情報
	m_nComment  = nComment;
	m_szComment = pszComment;
	m_nSaveOrder= nSaveOrder;
	m_pSaveOrder= pSaveOrder;

	// 命令長の計算(GetInsertSpace()で使用)
	m_nOrderLength = 0;
	for ( j=0; j<NC_MAXOD; j++ ) {
		for ( i=0; i<m_MakeOpt[j].nOrderCnt; i++ ) {
			if ( m_nOrderLength < (nLen=lstrlen(m_MakeOpt[j].pszOrder[i])) )
				m_nOrderLength = nLen;
		}
	}
	m_nOrderLength += 2;	// 2文字分ｽﾍﾟｰｽ
}

CNCMakeOption::~CNCMakeOption()
{
	delete[]	m_pIntOpt;
	delete[]	m_pDblOpt;
	delete[]	m_pFlgOpt;
}

/////////////////////////////////////////////////////////////////////////////
// ﾕｰｻﾞﾒﾝﾊﾞ関数

BOOL CNCMakeOption::ReadMakeOption(LPCTSTR lpszInitFile)
{
	// 命令を"="と";"(ｺﾒﾝﾄ)で分割
	typedef tokenizer< escaped_list_separator<TCHAR> > tokenizer;
	static	escaped_list_separator<TCHAR> sep("", "=;", "\"");	// ｴｽｹｰﾌﾟ無視
	// 切削条件の命令検査(大文字小文字は無視するが命令は完全一致)
	CStringKeyIndex	stNOrder(m_MakeOpt[0].nOrderCnt, m_MakeOpt[0].pszOrder);
	CStringKeyIndex	stDOrder(m_MakeOpt[1].nOrderCnt, m_MakeOpt[1].pszOrder);
	CStringKeyIndex	stBOrder(m_MakeOpt[2].nOrderCnt, m_MakeOpt[2].pszOrder);
	CStringKeyIndex	stSOrder(m_MakeOpt[3].nOrderCnt, m_MakeOpt[3].pszOrder);

#ifdef _DEBUGOLD
	CMagaDbg	dbg("CNCMakeOption::ReadMakeOption()\nStart", DBG_GREEN);
#endif

	// まずﾃﾞﾌｫﾙﾄで初期化
	InitialDefault();		// 各派生ｸﾗｽ
	if ( !lpszInitFile || lstrlen(lpszInitFile)<=0 ) {
		m_strInitFile.Empty();
		return TRUE;
	}
/*
	NCVC起動中に外部ｴﾃﾞｨﾀにて変更されている可能性があるので，
	既に保持しているﾌｧｲﾙ名と等しくても，この関数が呼ばれたときは必ず読み込み
*/
	m_strInitFile = lpszInitFile;

	tokenizer::iterator it;
	std::string	str, strOrder, strResult;
	tokenizer	tok(str, sep);

	CString	strTmp, strBuf;
	TCHAR	szCurrent[_MAX_PATH], szFile[_MAX_PATH];
	INT_PTR	n;
	BOOL	bResult = TRUE;

	// ｶﾚﾝﾄﾃﾞｨﾚｸﾄﾘを lpszFile に -> PathSearchAndQualify()
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
			// 分割文字列ｾｯﾄ
			str = strBuf;
			tok.assign(str);
#ifdef _DEBUGOLD
			dbg.printf("strBuf   = %s", strBuf);
#endif
			// 命令と値に分割
			it = tok.begin();
			strOrder  = ::Trim(*it);	++it;
			strResult = ::Trim(*it);
			if ( strOrder.empty() )
				continue;
#ifdef _DEBUGOLD
			dbg.printf("strOrder  =%s", strOrder.c_str());
			dbg.printf("strResult =%s", strResult.c_str());
#endif
			// 命令検査(int型)
			n = stNOrder.GetIndex(strOrder.c_str());
			if ( 0<=n && n<m_MakeOpt[0].nOrderCnt ) {
				m_pIntOpt[n] = strResult.empty() ? 0 : lexical_cast<int>(strResult);
				continue;
			}
			// 命令検査(double型)
			n = stDOrder.GetIndex(strOrder.c_str());
			if ( 0<=n && n<m_MakeOpt[1].nOrderCnt ) {
				m_pDblOpt[n] = strResult.empty() ? 0 : lexical_cast<double>(strResult);
				continue;
			}
			// 命令検査(BOOL型)
			n = stBOrder.GetIndex(strOrder.c_str());
			if ( 0<=n && n<m_MakeOpt[2].nOrderCnt ) {
				m_pFlgOpt[n] = strResult.empty() ? FALSE :
						(lexical_cast<int>(strResult) ? TRUE : FALSE);
				continue;
			}
			// 命令検査(CString型)
			n = stSOrder.GetIndex(strOrder.c_str());
			if ( 0<=n && n<m_MakeOpt[3].nOrderCnt ) {
				if ( IsPathID(n) ) {
					// 相対ﾊﾟｽなら絶対ﾊﾟｽに変換
					if ( !strResult.empty() &&
								::PathIsRelative(strResult.c_str()) &&		// Shlwapi.h
								::PathSearchAndQualify(strResult.c_str(), szFile, _MAX_PATH) )
						strResult = szFile;
				}
//				m_strOption.SetAtGrow(n, strResult.c_str());	// InitialDefault()があるので
				m_strOption[n] = strResult.c_str();				// これでOK
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
	catch ( const bad_lexical_cast& ) {
		strBuf.Format(IDS_ERR_DXF2NCDINIT, m_strInitFile);
		AfxMessageBox(strBuf, MB_OK|MB_ICONSTOP);
		bResult = FALSE;
	}

	// ｶﾚﾝﾄﾃﾞｨﾚｸﾄﾘを元に戻す
	::SetCurrentDirectory(szCurrent);

	return bResult;
}

BOOL CNCMakeOption::SaveMakeOption(LPCTSTR lpszInitFile)
{
	int		i, n, m, nLen;
	CString	strBuf, strResult;

#ifdef _DEBUG
	CMagaDbg	dbg("CNCMakeOption::SaveMakeOption()\nStart", DBG_GREEN);
#endif

	// 新規保存の場合
	if ( lpszInitFile )
		m_strInitFile = lpszInitFile;
	ASSERT( !m_strInitFile.IsEmpty() );

	try {
		CStdioFile	fp(m_strInitFile,
			CFile::modeCreate | CFile::modeWrite | CFile::shareDenyNone | CFile::typeText);
		// ｺﾒﾝﾄﾍｯﾀﾞｰ
		for ( i=0; i<m_nComment; i++ )
			fp.WriteString(m_szComment[i]);
		fp.WriteString(m_szComment[0]);
		// 命令の書き出し
		for ( i=0; i<m_nSaveOrder; i++ ) {
			n = m_pSaveOrder[i].nID;
			m = m_pSaveOrder[i].enType;
			switch ( m ) {
			case NC_PAGE:	// ﾍﾟｰｼﾞﾍｯﾀﾞｰ
				strBuf.Format("#--> Page:%d\n", n);
				break;
			case NC_NUM:	// int型
				strBuf.Format("%s%s= %8d     ; %s\n",
					m_MakeOpt[m].pszOrder[n], GetInsertSpace(lstrlen(m_MakeOpt[m].pszOrder[n])),
					m_pIntOpt[n],
					m_pSaveOrder[i].lpszComment);
				break;
			case NC_DBL:	// double型
				strBuf.Format("%s%s= %12.3f ; %s\n",
					m_MakeOpt[m].pszOrder[n], GetInsertSpace(lstrlen(m_MakeOpt[m].pszOrder[n])),
					m_pDblOpt[n],
					m_pSaveOrder[i].lpszComment);
				break;
			case NC_FLG:	// BOOL型
				strBuf.Format("%s%s= %8d     ; %s\n",
					m_MakeOpt[m].pszOrder[n], GetInsertSpace(lstrlen(m_MakeOpt[m].pszOrder[n])),
					m_pFlgOpt[n] ? 1 : 0,
					m_pSaveOrder[i].lpszComment);
				break;
			case NC_STR:	// CString型
				if ( IsPathID(n) ) {
					// 同じﾙｰﾄﾊﾟｽなら相対ﾊﾟｽに変換
					if ( !m_strOption[n].IsEmpty() && ::PathIsSameRoot(m_strInitFile, m_strOption[n]) ) {
						strResult = ::RelativePath(m_strInitFile, m_strOption[n]);
						if ( strResult.GetLength() >= m_strOption[n].GetLength() )
							strResult = m_strOption[n];	// 相対ﾊﾟｽの方が長ければそのまま
					}
					else
						strResult = m_strOption[n];
				}
				else
					strResult = m_strOption[n];
				// 書式を整えるために残ｽﾍﾟｰｽの計算
				nLen = 11 - strResult.GetLength();
				strBuf.Format("%s%s= \"%s\"%s; %s\n",
					m_MakeOpt[m].pszOrder[n], GetInsertSpace(lstrlen(m_MakeOpt[m].pszOrder[n])),
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
