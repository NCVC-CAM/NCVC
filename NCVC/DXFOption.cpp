// DXFOption.cpp: CDXFOption クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "NCVCdefine.h"
#include "DXFOption.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace boost;

static	const	int		g_bDxfID[] = {
	IDS_REG_DXF_VIEWER, IDS_REG_DXF_BINDFILECOMMENT
};
static	const	int		g_nDxfID[] = {
	IDS_REG_DXF_ORGTYPE, IDS_REG_DXF_SPLINENUM,
	IDS_REG_DXF_BINDORGTYPE, IDS_REG_DXF_BINDSORT
};
static	const	int		g_nDxfOldID[] = {
	IDS_REG_DXF_REGEX, IDS_REG_DXF_MATCH, IDS_REG_DXF_ACCEPT
};
static	const	int		g_bDxfDef[] = {
	1, 1
};
static	const	int		g_nDxfDef[] = {
	0, 1000,
	0, 0
};
static	const	float	g_dDxfDef[] = {
	300.0, 300.0,
	1.0
};
extern	LPCTSTR	g_szDefaultLayer[] = {
	"ORIGIN", "CAM",
	"MOVE", "CORRECT"			// DXF出力におけるﾃﾞﾌｫﾙﾄﾚｲﾔ名にのみ使用
};
extern	LPCTSTR	g_szNdelimiter;	// "XYZUVWIJKRPLDH" from NCDoc.cpp

/////////////////////////////////////////////////////////////////////////////
// CDXFOption クラスの構築/消滅

CDXFOption::CDXFOption()
{
	extern	LPCTSTR	gg_szRegKey;
	int		i;
	CString	strRegKey, strEntry, strResult;

	VERIFY(strRegKey.LoadString(IDS_REGKEY_DXF));
	for ( i=0; i<DXFLAYERSIZE; i++ ) {	// DXF[ORG|CAM|STR|MOV|COM]LAYER
		VERIFY(strEntry.LoadString(IDS_REG_DXF_ORGLAYER+i));
		m_strReadLayer[i] = AfxGetApp()->GetProfileString(strRegKey, strEntry,
			i<=DXFCAMLAYER ? g_szDefaultLayer[i] : strResult );	// ﾃﾞﾌｫﾙﾄﾊﾟﾗﾒｰﾀ は，原点と切削ﾚｲﾔのみ
	}
	try {
		m_regCutter = m_strReadLayer[DXFCAMLAYER];
	}
	catch (boost::regex_error&) {
		m_regCutter = g_szDefaultLayer[DXFCAMLAYER];
	}

	// ｵﾌﾟｼｮﾝ
	for ( i=0; i<SIZEOF(m_ubNums); i++ ) {
		VERIFY(strEntry.LoadString(g_bDxfID[i]));
		m_ubNums[i] = AfxGetApp()->GetProfileInt(strRegKey, strEntry, g_bDxfDef[i]);
	}
	for ( i=0; i<SIZEOF(m_unNums); i++ ) {
		VERIFY(strEntry.LoadString(g_nDxfID[i]));
		m_unNums[i] = AfxGetApp()->GetProfileInt(strRegKey, strEntry, g_nDxfDef[i]);
	}
	for ( i=0; i<SIZEOF(m_dBindWork); i++ ) {
		VERIFY(strEntry.LoadString(IDS_REG_DXF_BINDSIZE));
		strResult = AfxGetApp()->GetProfileString(strRegKey, strEntry+g_szNdelimiter[i]);
		m_dBindWork[i] = strResult.IsEmpty() ? g_dDxfDef[i] : (float)atof(LPCTSTR(strResult.Trim()));
	}
	VERIFY(strEntry.LoadString(IDS_REG_DXF_BINDMARGIN));
	strResult = AfxGetApp()->GetProfileString(strRegKey, strEntry);
	m_dBindMargin = strResult.IsEmpty() ? g_dDxfDef[i] : (float)atof(LPCTSTR(strResult.Trim()));
	VERIFY(strEntry.LoadString(IDS_REG_DXF_IGNORE));
	strResult = AfxGetApp()->GetProfileString(strRegKey, strEntry);
	SetIgnoreArray(strResult);

	// 旧式ｵﾌﾟｼｮﾝの削除
	CRegKey	reg;
	// --- "Software\MNCT-S\NCVC\Settings"
	if ( reg.Open(HKEY_CURRENT_USER, gg_szRegKey+strRegKey) == ERROR_SUCCESS ) {
		for ( i=0; i<SIZEOF(g_nDxfOldID); i++ ) {
			VERIFY(strEntry.LoadString(g_nDxfOldID[i]));
			reg.DeleteValue(strEntry);
		}
		reg.Close();
	}

	m_enMakeType = NCMAKEMILL;
	try {
		// 切削条件ﾌｧｲﾙの履歴
		ReadInitHistory(NCMAKEMILL);
		ReadInitHistory(NCMAKELATHE);
		ReadInitHistory(NCMAKEWIRE);
		// ﾚｲﾔ名と条件ﾌｧｲﾙ関係の履歴
		ReadInitHistory(NCMAKELAYER);
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		return;
	}
}

/////////////////////////////////////////////////////////////////////////////
// ﾒﾝﾊﾞ関数

BOOL CDXFOption::ReadInitHistory(NCMAKETYPE enType)
{
	extern	LPTSTR	g_pszExecDir;	// 実行ﾃﾞｨﾚｸﾄﾘ(NCVC.cpp)
	int		i;
	CString	strRegKey, strEntry, strDefExt('.'),
			strTmp, strResult;
	CStringList&	strList = m_strInitList[enType];

	VERIFY(strRegKey.LoadString(IDS_REGKEY_DXF));
	VERIFY(strEntry.LoadString(IDS_REG_DXF_INIT+enType));
	VERIFY(strTmp.LoadString(IDS_NCIM_FILTER+enType));
	strDefExt += strTmp.Left(3);

	for ( i=0; i<DXFMAXINITFILE; i++ ) {
		strTmp.Format(IDS_COMMON_FORMAT, strEntry, i);
		strResult = AfxGetApp()->GetProfileString(strRegKey, strTmp);
		if ( strResult.IsEmpty() )
			break;
		if ( ::IsFileExist(strResult, TRUE, FALSE) )
			strList.AddTail(strResult); 
	}
	if ( strList.IsEmpty() && enType!=NCMAKELAYER ) {
		strResult  = g_pszExecDir;
		strResult += "INIT" + strDefExt;
		strList.AddTail(strResult);
	}

	return TRUE;
}

BOOL CDXFOption::SaveInitHistory(NCMAKETYPE enType)
{
	int			i;
	POSITION	pos;
	CString		strRegKey, strEntry, strFmt;
	CStringList&	strList = m_strInitList[enType];

	VERIFY(strRegKey.LoadString(IDS_REGKEY_DXF));
	VERIFY(strEntry.LoadString(IDS_REG_DXF_INIT+enType));

	for ( i=0; i<DXFMAXINITFILE; i++ ) {
		strFmt.Format(IDS_COMMON_FORMAT, strEntry, i);
		AfxGetApp()->WriteProfileString(strRegKey, strFmt, NULL);
	}
	for ( i=0, pos=strList.GetHeadPosition(); pos && i<DXFMAXINITFILE; i++ ) {
		strFmt.Format(IDS_COMMON_FORMAT, strEntry, i);
		if ( !AfxGetApp()->WriteProfileString(strRegKey, strFmt, strList.GetNext(pos)) )
			return FALSE;
	}
	if ( enType != NCMAKELAYER )
		m_enMakeType = enType;

	return TRUE;
}

BOOL CDXFOption::AddListHistory(NCMAKETYPE enType, LPCTSTR lpszSearch)
{
	CStringList& strList = m_strInitList[enType];

	if ( !lpszSearch || lstrlen(lpszSearch) <= 0 )
		return TRUE;
	if ( !strList.IsEmpty() &&
			strList.GetHead().CompareNoCase(lpszSearch) == 0 )
		return TRUE;

	// 文字列の検索(Findは大文字小文字を区別するので使えない)
	try {
		POSITION pos1, pos2;
		for ( pos1=strList.GetHeadPosition(); (pos2=pos1); ) {
			if ( strList.GetNext(pos1).CompareNoCase(lpszSearch) == 0 ) {
				// 文字列があれば，それを消して先頭へ
				strList.RemoveAt(pos2);
				strList.AddHead(lpszSearch);
				return TRUE;
			}
		}
		// なければ先頭に追加
		strList.AddHead(lpszSearch);
		// DXFMAXINITFILE を越えれば最後を消去
		if ( strList.GetCount() > DXFMAXINITFILE )
			strList.RemoveTail();
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		return FALSE;
	}

	return TRUE;
}

void CDXFOption::DelInitHistory(NCMAKETYPE enType, LPCTSTR lpszSearch)
{
	CStringList& strList = m_strInitList[enType];

	POSITION pos1, pos2;
	for ( pos1=strList.GetHeadPosition(); (pos2 = pos1); ) {
		if ( strList.GetNext(pos1).CompareNoCase(lpszSearch) == 0 ) {
			strList.RemoveAt(pos2);
			break;
		}
	}
}

CString	CDXFOption::GetIgnoreStr(void) const
{
	extern	LPCTSTR	gg_szCRLF;	// "\r\n"
	CString	strResult;
	for ( int i=0; i<m_strIgnoreArray.GetSize(); i++ )
		strResult += m_strIgnoreArray[i] + gg_szCRLF;		// エディットコントロールの改行
	return strResult;
}

void CDXFOption::SetIgnoreArray(const CString& strIgnore)
{
	extern	LPCTSTR	gg_szCRLF;
	m_strIgnoreArray.RemoveAll();
	std::string	str(strIgnore), strTok;
	STDSEPA		sep(gg_szCRLF);
	STDTOKEN	tok(str, sep);
	BOOST_FOREACH(strTok, tok) {
		boost::algorithm::trim(strTok);
		if ( !strTok.empty() )
			m_strIgnoreArray.Add(strTok.c_str());
	}
}

BOOL CDXFOption::IsIgnore(const CString& strIgnore) const
{
	for ( int i=0; i<m_strIgnoreArray.GetSize(); i++ ) {
		if ( strIgnore.CompareNoCase(m_strIgnoreArray[i]) == 0 )
			return TRUE;
	}
	return FALSE;
}

BOOL CDXFOption::SaveDXFoption(void)
{
	int			i;
	CString		strRegKey, strEntry, strResult;

	VERIFY(strRegKey.LoadString(IDS_REGKEY_DXF));
	for ( i=0; i<DXFLAYERSIZE; i++ ) {
		VERIFY(strEntry.LoadString(IDS_REG_DXF_ORGLAYER+i));
		if ( !AfxGetApp()->WriteProfileString(strRegKey, strEntry, m_strReadLayer[i]) )
			return FALSE;
	}
	// BIND関係除く
	VERIFY(strEntry.LoadString(IDS_REG_DXF_VIEWER));
	if ( !AfxGetApp()->WriteProfileInt(strRegKey, strEntry, m_bView) )
		return FALSE;
	VERIFY(strEntry.LoadString(IDS_REG_DXF_ORGTYPE));
	if ( !AfxGetApp()->WriteProfileInt(strRegKey, strEntry, m_nOrgType) )
		return FALSE;
	VERIFY(strEntry.LoadString(IDS_REG_DXF_SPLINENUM));
	if ( !AfxGetApp()->WriteProfileInt(strRegKey, strEntry, m_nSplineNum) )
		return FALSE;
	VERIFY(strEntry.LoadString(IDS_REG_DXF_IGNORE));
	strResult = GetIgnoreStr();
	if ( !AfxGetApp()->WriteProfileString(strRegKey, strEntry, strResult) )
		return FALSE;

	return TRUE;
}

BOOL CDXFOption::SaveBindOption(void)
{
	int			i;
	CString		strRegKey, strEntry, strResult;

	VERIFY(strRegKey.LoadString(IDS_REGKEY_DXF));
	VERIFY(strEntry.LoadString(IDS_REG_DXF_BINDFILECOMMENT));
	if ( !AfxGetApp()->WriteProfileInt(strRegKey, strEntry, m_bFileComment) )
		return FALSE;
	for ( i=DXFOPT_BINDORG; i<SIZEOF(m_unNums); i++ ) {
		VERIFY(strEntry.LoadString(g_nDxfID[i]));
		if ( !AfxGetApp()->WriteProfileInt(strRegKey, strEntry, m_unNums[i]) )
			return FALSE;
	}
	VERIFY(strEntry.LoadString(IDS_REG_DXF_BINDSIZE));
	for ( i=0; i<SIZEOF(m_dBindWork); i++ ) {
		strResult.Format(IDS_MAKENCD_FORMAT, m_dBindWork[i]);
		if ( !AfxGetApp()->WriteProfileString(strRegKey, strEntry+g_szNdelimiter[i], strResult) )
			return FALSE;
	}
	VERIFY(strEntry.LoadString(IDS_REG_DXF_BINDMARGIN));
	strResult.Format(IDS_MAKENCD_FORMAT, m_dBindMargin);
	if ( !AfxGetApp()->WriteProfileString(strRegKey, strEntry, strResult) )
		return FALSE;

	return TRUE;
}
