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

//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////

CExecOption::CExecOption(const CString& strExec)
{
	extern	LPCTSTR	gg_szReturn;

	m_strFileName.Empty();
	m_strCommand.Empty();
	m_strToolTip.Empty();
	m_bNCType = m_bDXFType = m_bShort = FALSE;
	m_nImage = m_nMenuID = -1;

	typedef boost::tokenizer< boost::char_separator<TCHAR> > tokenizer;
	static	boost::char_separator<TCHAR> sep(gg_szReturn, "", boost::keep_empty_tokens);
	std::string	str( strExec );
	tokenizer	tok( str, sep );
	tokenizer::iterator it;
	int		i;

	// ｺﾝｽﾄﾗｸﾀ内の例外は上位で処理
	for ( i=0, it=tok.begin(); it!=tok.end(); i++, ++it ) {
		switch ( i ) {
		case 0:
			m_strFileName = it->c_str();
			break;
		case 1:
			m_strCommand = it->c_str();
			break;
		case 2:
			m_strToolTip = it->c_str();
			break;
		case 3:
			m_bNCType = atoi(it->c_str()) ? TRUE : FALSE;
			break;
		case 4:
			m_bDXFType = atoi(it->c_str()) ? TRUE : FALSE;
			break;
		case 5:
			m_bShort = atoi(it->c_str()) ? TRUE : FALSE;
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

CExecOption::~CExecOption()
{
}

/////////////////////////////////////////////////////////////////////////////
// CExecOption クラスのメンバ関数

CString CExecOption::GetStringData(void) const
{
	CString	str;
	int		nNCType  = m_bNCType  ? 1 : 0;
	int		nDXFType = m_bDXFType ? 1 : 0;
	int		nShort   = m_bShort   ? 1 : 0;

	str.Format("%s\n%s\n%s\n%d\n%d\n%d", m_strFileName, m_strCommand, m_strToolTip,
					nNCType, nDXFType, nShort);
	return str;
}
