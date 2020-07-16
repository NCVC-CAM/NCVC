// DXFOption.cpp: CDXFOption クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "NCVCdefine.h"
#include "DXFOption.h"

#include <stdlib.h>
#include <string.h>

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
extern	CMagaDbg	g_dbg;
#endif

static	const	int		g_nDxfID[] = {
	IDS_REG_DXF_REGEX, IDS_REG_DXF_MATCH, IDS_REG_DXF_ACCEPT,
	IDS_REG_DXF_VIEWER, IDS_REG_DXF_ORGTYPE
};
static	const	int		g_nDxfDef[] = {
	0, 0, 0,
	1, 0
};

/////////////////////////////////////////////////////////////////////////////
// CDXFOption クラスの構築/消滅

CDXFOption::CDXFOption()
{
	extern	LPTSTR	g_pszExecDir;	// 実行ﾃﾞｨﾚｸﾄﾘ(NCVC.cpp)
	int		i;
	CString	strRegKey, strEntry, strTmp, strResult;

	VERIFY(strRegKey.LoadString(IDS_REGKEY_DXF));
	for ( i=0; i<DXFLAYERSIZE; i++ ) {
		VERIFY(strEntry.LoadString(IDS_REG_DXF_ORGLAYER+i));
		m_strReadLayer[i] = AfxGetApp()->GetProfileString(strRegKey, strEntry,
			(i<=DXFCAMLAYER ? strEntry : strTmp) );	// ﾃﾞﾌｫﾙﾄﾊﾟﾗﾒｰﾀ は，原点と切削ﾚｲﾔのみ
	}
	// 複数ﾚｲﾔの管理ﾘｽﾄ作成
	SetCutterList();

	// ｵﾌﾟｼｮﾝ
	for ( i=0; i<SIZEOF(m_nDXF); i++ ) {
		VERIFY(strEntry.LoadString(g_nDxfID[i]));
		m_nDXF[i] = AfxGetApp()->GetProfileInt(strRegKey, strEntry, g_nDxfDef[i]);
	}

	// 切削ﾚｲﾔ名の認識ﾊﾟﾀｰﾝ関数ｾｯﾄ
	SetCallingLayerFunction();

	try {
		// 切削条件ﾌｧｲﾙの履歴
		VERIFY(strEntry.LoadString(IDS_REG_DXF_INIT));
		for ( i=0; i<DXFMAXINITFILE; i++ ) {
			strTmp.Format(IDS_COMMON_FORMAT, strEntry, i);
			strResult = AfxGetApp()->GetProfileString(strRegKey, strTmp);
			if ( !strResult.IsEmpty() && ::IsFileExist(strResult, TRUE, FALSE) )
				m_strInitList.AddTail(strResult); 
		}
		if ( m_strInitList.IsEmpty() ) {	// 下位互換
			strResult = AfxGetApp()->GetProfileString(strRegKey, strEntry);
			if ( !strResult.IsEmpty() && ::IsFileExist(strResult, TRUE, FALSE) )
				m_strInitList.AddTail(strResult); 
		}
		if ( m_strInitList.IsEmpty() ) {
			strResult  = g_pszExecDir;
			strResult += "INIT.nci";
			m_strInitList.AddTail(strResult);
		}
		// ﾚｲﾔ名と条件ﾌｧｲﾙ関係の履歴
		VERIFY(strEntry.LoadString(IDS_REG_DXF_LAYERTOINIT));
		for ( i=0; i<DXFMAXINITFILE; i++ ) {
			strTmp.Format(IDS_COMMON_FORMAT, strEntry, i);
			strResult = AfxGetApp()->GetProfileString(strRegKey, strTmp);
			if ( !strResult.IsEmpty() && ::IsFileExist(strResult, TRUE, FALSE) )
				m_strLayerToInitList.AddTail(strResult); 
		}
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		return;
	}
}

/////////////////////////////////////////////////////////////////////////////
// ﾒﾝﾊﾞ関数

BOOL CDXFOption::SaveDXFoption(void)
{
	int			i;
	CString		strRegKey, strEntry;
	VERIFY(strRegKey.LoadString(IDS_REGKEY_DXF));
	for ( i=0; i<DXFLAYERSIZE; i++ ) {
		VERIFY(strEntry.LoadString(IDS_REG_DXF_ORGLAYER+i));
		if ( !AfxGetApp()->WriteProfileString(strRegKey, strEntry, m_strReadLayer[i]) )
			return FALSE;
	}

	for ( i=0; i<SIZEOF(m_nDXF); i++ ) {
		VERIFY(strEntry.LoadString(g_nDxfID[i]));
		if ( !AfxGetApp()->WriteProfileInt(strRegKey, strEntry, m_nDXF[i]) )
			return FALSE;
	}

	return SaveInitHistory();
}

BOOL CDXFOption::SaveInitHistory(void)
{
	int			i;
	POSITION	pos;
	CString		strRegKey, strEntry, strFmt;
	VERIFY(strRegKey.LoadString(IDS_REGKEY_DXF));

	VERIFY(strEntry.LoadString(IDS_REG_DXF_INIT));
	for ( i=0, pos=m_strInitList.GetHeadPosition();
				pos && i<DXFMAXINITFILE; i++ ) {
		strFmt.Format(IDS_COMMON_FORMAT, strEntry, i);
		if ( !AfxGetApp()->WriteProfileString(strRegKey, strFmt, m_strInitList.GetNext(pos)) )
			return FALSE;
	}

	VERIFY(strEntry.LoadString(IDS_REG_DXF_LAYERTOINIT));
	for ( i=0, pos = m_strLayerToInitList.GetHeadPosition();
				pos && i<DXFMAXINITFILE; i++ ) {
		strFmt.Format(IDS_COMMON_FORMAT, strEntry, i);
		if ( !AfxGetApp()->WriteProfileString(strRegKey, strFmt, m_strLayerToInitList.GetNext(pos)) )
			return FALSE;
	}

	return TRUE;
}

void CDXFOption::SetCallingLayerFunction(void)
{
	if ( m_nDXF[DXFOPT_REGEX] == 1 )
		m_pfnIsCutterLayer = &IsRegex;
	else {
		if ( m_nDXF[DXFOPT_ACCEPT] == 0 ) {
			if ( m_strCutterMap.IsEmpty() )
				m_pfnIsCutterLayer = &IsAllMatch;
			else
				m_pfnIsCutterLayer = m_nDXF[DXFOPT_MATCH] == 0 ? &IsPerfectMatch : &IsPartMatch;
		}
		else
			m_pfnIsCutterLayer = m_nDXF[DXFOPT_MATCH] == 0 ? &IsPerfectNotMatch : &IsPartNotMatch;
	}
}

void CDXFOption::SetCutterList(void)
{
	extern	LPCTSTR	gg_szCat;

	m_strCutterMap.RemoveAll();
	m_strCutterFirst.Empty();

	// 正規表現の時はﾏｯﾌﾟ作成の必要なし
	if ( m_nDXF[DXFOPT_REGEX] == 1 )
		return;

	LPTSTR	lpszBuf = NULL;
	LPTSTR	lpsztok;

	try {
		lpszBuf = new TCHAR[m_strReadLayer[DXFCAMLAYER].GetLength()+1];
		for ( lpsztok = strtok(lstrcpy(lpszBuf, m_strReadLayer[DXFCAMLAYER]), gg_szCat); lpsztok; ) {
#ifdef _DEBUG
			g_dbg.printf("CutterList=%s", lpsztok);
#endif
			PathRemoveBlanks(lpsztok);	// shlwapi.h
			if ( lstrlen(lpsztok) > 0 ) {
				m_strCutterMap.SetAt(lpsztok, NULL);
				if ( m_strCutterFirst.IsEmpty() )
					m_strCutterFirst = lpsztok;
			}
			lpsztok = strtok(NULL, gg_szCat);
		}
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
	}

	if ( lpszBuf )
		delete[]	lpszBuf;
}

BOOL CDXFOption::AddListHistory(CStringList& strList, LPCTSTR lpszSearch)
{
	if ( !lpszSearch || lstrlen(lpszSearch) <= 0 )
		return TRUE;
	if ( !strList.IsEmpty() &&
			strList.GetHead().CompareNoCase(lpszSearch) == 0 )
		return TRUE;

	// 文字列の検索(Findは大文字小文字を区別するので使えない)
	try {
		POSITION pos1, pos2;
		for ( pos1 = strList.GetHeadPosition(); (pos2 = pos1); ) {
			if ( strList.GetNext(pos1).CompareNoCase(lpszSearch) == 0 ) {
				// 文字列があれば，それを消して先頭へ
				strList.RemoveAt(pos2);
				strList.AddHead(lpszSearch);
				return TRUE;
			}
		}
		// なければ先頭に追加
		strList.AddHead(lpszSearch);
		// 10個越えれば最後を消去
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

void CDXFOption::DelListHistory(CStringList& strList, LPCTSTR lpszSearch)
{
	POSITION pos1, pos2;
	for ( pos1 = strList.GetHeadPosition(); (pos2 = pos1); ) {
		if ( strList.GetNext(pos1).CompareNoCase(lpszSearch) == 0 ) {
			strList.RemoveAt(pos2);
			break;
		}
	}
}

// 切削ﾚｲﾔの認識
BOOL CDXFOption::IsAllMatch(const CDXFOption* pOpt, LPCTSTR lpszLayer)
{
	return lstrlen(lpszLayer) > 0 ? TRUE : FALSE;
}

BOOL CDXFOption::IsPerfectMatch(const CDXFOption* pOpt, LPCTSTR lpszLayer)
{
	CObject*	pDummy;
	return pOpt->m_strCutterMap.Lookup(lpszLayer, pDummy);
}

BOOL CDXFOption::IsPartMatch(const CDXFOption* pOpt, LPCTSTR lpszLayer)
{
	CObject*	pDummy;
	CString		strLayer(lpszLayer), strKey;
	for ( POSITION pos = pOpt->m_strCutterMap.GetStartPosition(); pos; ) {
		pOpt->m_strCutterMap.GetNextAssoc(pos, strKey, pDummy);
		if ( strLayer.Find((LPCTSTR)strKey) != -1 )
			return TRUE;
	}
	return FALSE;
}

BOOL CDXFOption::IsPerfectNotMatch(const CDXFOption* pOpt, LPCTSTR lpszLayer)
{
	return !IsPerfectMatch(pOpt, lpszLayer);
}

BOOL CDXFOption::IsPartNotMatch(const CDXFOption* pOpt, LPCTSTR lpszLayer)
{
	return !IsPartMatch(pOpt, lpszLayer);
}

BOOL CDXFOption::IsRegex(const CDXFOption* pOpt, LPCTSTR lpszLayer)
{
	boost::regex	r(pOpt->m_strReadLayer[DXFCAMLAYER]);
	return boost::regex_search(lpszLayer, r);
}
