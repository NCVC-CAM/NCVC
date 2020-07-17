// DXFOption.cpp: CDXFOption クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "NCVCdefine.h"
#include "DXFOption.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

static	const	int		g_nDxfID[] = {
	IDS_REG_DXF_VIEWER, IDS_REG_DXF_ORGTYPE
};
static	const	int		g_nDxfOldID[] = {
	IDS_REG_DXF_REGEX, IDS_REG_DXF_MATCH, IDS_REG_DXF_ACCEPT
};
static	const	int		g_nDxfDef[] = {
	1, 0
};
extern	LPCTSTR	g_szDefaultLayer[] = {
	"ORIGIN", "CAM",
	"MOVE", "CORRECT"		// DXF出力におけるﾃﾞﾌｫﾙﾄﾚｲﾔ名にのみ使用
};

/////////////////////////////////////////////////////////////////////////////
// CDXFOption クラスの構築/消滅

CDXFOption::CDXFOption()
{
	extern	LPTSTR	g_pszExecDir;	// 実行ﾃﾞｨﾚｸﾄﾘ(NCVC.cpp)
	extern	LPCTSTR	gg_szRegKey;
	int		i;
	CString	strRegKey, strEntry, strTmp, strResult;

	VERIFY(strRegKey.LoadString(IDS_REGKEY_DXF));
	for ( i=0; i<DXFLAYERSIZE; i++ ) {	// DXF[ORG|CAM|STR|MOV|COM]LAYER
		VERIFY(strEntry.LoadString(IDS_REG_DXF_ORGLAYER+i));
		m_strReadLayer[i] = AfxGetApp()->GetProfileString(strRegKey, strEntry,
			i<=DXFCAMLAYER ? g_szDefaultLayer[i] : strTmp );	// ﾃﾞﾌｫﾙﾄﾊﾟﾗﾒｰﾀ は，原点と切削ﾚｲﾔのみ
	}
	m_regCutter = m_strReadLayer[DXFCAMLAYER];

	// ｵﾌﾟｼｮﾝ
	for ( i=0; i<SIZEOF(m_nDXF); i++ ) {
		VERIFY(strEntry.LoadString(g_nDxfID[i]));
		m_nDXF[i] = AfxGetApp()->GetProfileInt(strRegKey, strEntry, g_nDxfDef[i]);
	}
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
		for ( i=m_strInitList.GetCount(); i>DXFMAXINITFILE; i-- )
			m_strInitList.RemoveTail();		// DXFMAXINITFILE を超える分を削除
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
	for ( i=0; i<DXFMAXINITFILE; i++ ) {
		strFmt.Format(IDS_COMMON_FORMAT, strEntry, i);
		AfxGetApp()->WriteProfileString(strRegKey, strFmt, NULL);
	}
	for ( i=0, pos=m_strInitList.GetHeadPosition();
				pos && i<DXFMAXINITFILE; i++ ) {
		strFmt.Format(IDS_COMMON_FORMAT, strEntry, i);
		if ( !AfxGetApp()->WriteProfileString(strRegKey, strFmt, m_strInitList.GetNext(pos)) )
			return FALSE;
	}

	VERIFY(strEntry.LoadString(IDS_REG_DXF_LAYERTOINIT));
	for ( i=0; i<DXFMAXINITFILE; i++ ) {
		strFmt.Format(IDS_COMMON_FORMAT, strEntry, i);
		AfxGetApp()->WriteProfileString(strRegKey, strFmt, NULL);
	}
	for ( i=0, pos=m_strLayerToInitList.GetHeadPosition();
				pos && i<DXFMAXINITFILE; i++ ) {
		strFmt.Format(IDS_COMMON_FORMAT, strEntry, i);
		if ( !AfxGetApp()->WriteProfileString(strRegKey, strFmt, m_strLayerToInitList.GetNext(pos)) )
			return FALSE;
	}

	return TRUE;
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

void CDXFOption::DelListHistory(CStringList& strList, LPCTSTR lpszSearch)
{
	POSITION pos1, pos2;
	for ( pos1=strList.GetHeadPosition(); (pos2 = pos1); ) {
		if ( strList.GetNext(pos1).CompareNoCase(lpszSearch) == 0 ) {
			strList.RemoveAt(pos2);
			break;
		}
	}
}
