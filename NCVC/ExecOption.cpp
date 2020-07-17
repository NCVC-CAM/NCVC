// ExecOption.cpp: CExecOption クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "ExecOption.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

using std::string;
using namespace boost;

extern	LPCTSTR	gg_szReturn;	// "\n"

//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////

CExecOption::CExecOption(const CString& strExec)
{
	m_strFileName.Empty();
	m_strCommand.Empty();
	m_strToolTip.Empty();
	m_bNCType = m_bDXFType = m_bShort = FALSE;
	m_nImage = m_nMenuID = -1;

	typedef tokenizer< char_separator<TCHAR> > tokenizer;
	static	char_separator<TCHAR> sep(gg_szReturn, "", keep_empty_tokens);
	std::string	str(strExec), strTok;
	tokenizer	tok(str, sep);
	int		i = 0;

	// ｺﾝｽﾄﾗｸﾀ内の例外は上位で処理
	BOOST_FOREACH(strTok, tok) {
		switch ( i++ ) {
		case 0:
			m_strFileName = strTok.c_str();
			break;
		case 1:
			m_strCommand = strTok.c_str();
			break;
		case 2:
			m_strToolTip = strTok.c_str();
			break;
		case 3:
			m_bNCType = atoi(strTok.c_str()) ? TRUE : FALSE;
			break;
		case 4:
			m_bDXFType = atoi(strTok.c_str()) ? TRUE : FALSE;
			break;
		case 5:
			m_bShort = atoi(strTok.c_str()) ? TRUE : FALSE;
			break;
		}
	}

	m_bDlgAdd = m_bDlgDel = FALSE;
}

CExecOption::CExecOption
	(const CString& strFileName, const CString& strCommand, const CString& strToolTip,
		BOOL bNCType, BOOL bDXFType, BOOL bShort)
{
	m_strFileName	= strFileName;
	m_strCommand	= strCommand;
	m_strToolTip	= strToolTip;
	m_bNCType		= bNCType;
	m_bDXFType		= bDXFType;
	m_bShort		= bShort;
	m_nImage = m_nMenuID = -1;

	m_bDlgAdd = TRUE;
	m_bDlgDel = FALSE;
}

CExecOption::CExecOption(const CString& strApp, DOCTYPE emDoc)
{
	CString	strPath, strFile;
	::Path_Name_From_FullPath(strApp, strPath, strFile, FALSE);	// 拡張子無視

	m_strFileName = strApp;
	m_strCommand  = "\"" + AfxGetNCVCMainWnd()->MakeCommand(0) + "\"";	// "${FileFullPath}"
	m_strToolTip  = strFile;
	if ( emDoc == TYPE_NCD ) {
		m_bNCType  = TRUE;
		m_bDXFType = FALSE;
	}
	else /*if ( emDoc == TYPE_DXF )*/ {
		m_bNCType  = FALSE;
		m_bDXFType = TRUE;
	}
	m_bShort = FALSE;
	m_nImage = m_nMenuID = -1;

	m_bDlgAdd = m_bDlgDel = FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CExecOption クラスのメンバ関数

CString CExecOption::GetStringData(void) const
{
	CString	strResult;	// .Format("%s\n%s\n%s\n%d\n%d\n%d", ...)
	strResult = m_strFileName + gg_szReturn +
				m_strCommand  + gg_szReturn +
				m_strToolTip  + gg_szReturn +
				lexical_cast<string>(m_bNCType  ? 1 : 0).c_str() + gg_szReturn + 
				lexical_cast<string>(m_bDXFType ? 1 : 0).c_str() + gg_szReturn + 
				lexical_cast<string>(m_bShort   ? 1 : 0).c_str();
	return strResult;
}
