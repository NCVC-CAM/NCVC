// MCOption.cpp: CMCOption クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "MCOption.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

using namespace boost;
using std::string;

extern	LPCTSTR	g_szNdelimiter;		// "XYZUVWIJKRPLDH" from NCDoc.cpp
extern	LPCTSTR	gg_szComma;			// ","
//
extern	LPCTSTR	gg_szRegKey;
// 機械情報履歴サイズ
#define	NCMAXMCFILE		10

// ﾚｼﾞｽﾄﾘ・INIﾌｧｲﾙの移行ｽｷｰﾏ
static	const	UINT	g_nConvert = 2;
//
static	LPCTSTR	g_szGformat = "G%d";

// int型命令
static	LPCTSTR	g_szNOrder[] = {
	"Modal%d",
	"G0Speed%c",
	"FDot", "CorrectType", "ForceViewMode"
};
static	const	int		g_dfNOrder[] = {
	0, 0, 0, 0, 0,
	0, 0, 0,
	0, 0, 0
};

// float型命令
static	LPCTSTR	g_szDOrder[] = {
	"Initial%c",
	"DefaultFeed",
	"BlockTime",
	"DefWireDepth"
};
static	const	float	g_dfDOrder[] = {
	0.0, 0.0, 10.0,
	30.0,
	0.0,
	20.0,
	// G54～G59
	0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
	0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0
};

// BOOL型命令
static	LPCTSTR	g_szBOrder[] = {
	"L0Cycle"
};
static	const	BOOL	g_dfBOrder[] = {
	FALSE
};

// CString型命令
static	LPCTSTR	g_szSOrder[] = {
	"Name",
	"AutoBreak",
	"Tool%d"
};
static	LPCTSTR	g_szSOrderMacro[] = {
	"MacroCode", "MacroFolder", "MacroIF", "MacroArgv",
	"MacroResult", "MachineFile", "CurrentFolder"	// 置換引数のみに使用
};
extern	const	int		g_nDefaultMacroID[] = {
	MCMACHINEFILE, MCMACROCODE, MCMACROFOLDER, MCCURRENTFOLDER, MCMACRORESULT
};

// 旧ｷｰﾜｰﾄﾞ
static	LPCTSTR	g_szOldOrder[] = {
	"Lathe"		// -> ForceViewMode(int)
};

/////////////////////////////////////////////////////////////////////////////
// CMCOption クラスの構築/消滅

CMCOption::CMCOption()
{
//	ASSERT( MC_INT_NUMS == SIZEOF(g_szNOrder) );	// SIZEOF(g_szNOrder)+MODALGROUP+NCXYZ-2
	ASSERT( MC_INT_NUMS == SIZEOF(g_dfNOrder) );
//	ASSERT( MC_DBL_NUMS == SIZEOF(g_szDOrder) );	// SIZEOF(g_szDOrder)+NCXYZ-1
	ASSERT( MC_DBL_NUMS == SIZEOF(g_dfDOrder) );
	ASSERT( MC_FLG_NUMS == SIZEOF(g_szBOrder) );
	ASSERT( MC_FLG_NUMS == SIZEOF(g_dfBOrder) );

	CString	strRegKey, strEntry, strResult, strFmt;
	int		i;

	// ﾒﾝﾊﾞ変数の初期化
	for ( i=0; i<SIZEOF(g_dfNOrder); i++ )
		m_unNums[i] = g_dfNOrder[i];
	for ( i=0; i<SIZEOF(g_dfDOrder); i++ )
		m_udNums[i] = g_dfDOrder[i];
	for ( i=0; i<SIZEOF(g_dfBOrder); i++ )
		m_ubFlgs[i] = g_dfBOrder[i];

	VERIFY(strRegKey.LoadString(IDS_REGKEY_NC));

	// 機械情報履歴(逆順で読み込み)
	VERIFY(strEntry.LoadString(IDS_REG_NCV_MCFILE));
	for ( i=NCMAXMCFILE-1; i>=0; i-- ) {
		strFmt.Format(IDS_COMMON_FORMAT, strEntry, i);
		strResult = AfxGetApp()->GetProfileString(strRegKey, strFmt);
		// ﾌｧｲﾙが存在するときだけ履歴に登録
		if ( !strResult.IsEmpty() && ::IsFileExist(strResult, TRUE, FALSE) )
			AddMCListHistory(strResult);
	}

	// 下位互換移行完了ﾁｪｯｸ
	VERIFY(strEntry.LoadString(IDS_REG_CONVERT));
	UINT nResult = AfxGetApp()->GetProfileInt(strRegKey, strEntry, 0);
	if ( nResult < g_nConvert ) {
		// 0:Convertｷｰ無し(新規ﾕｰｻﾞ)，Fﾊﾟﾗﾒｰﾀがint型
		// 1:機械情報がﾚｼﾞｽﾄﾘにある
		VERIFY(strEntry.LoadString(IDS_REG_NCV_INPUT));
		m_nModal[MODALGROUP3] = AfxGetApp()->GetProfileInt(strRegKey, strEntry, 0);
		VERIFY(strEntry.LoadString(IDS_REG_NCV_MOVE));
		for ( i=0; i<NCXYZ; i++ )
			m_nG0Speed[i] = AfxGetApp()->GetProfileInt(strRegKey, g_szNdelimiter[i]+strEntry, 0);
		VERIFY(strEntry.LoadString(IDS_REG_NCV_FDOT));
		m_nFDot = AfxGetApp()->GetProfileInt(strRegKey, strEntry, 0);
		for ( i=0; i<WORKOFFSET; i++ ) {
			strEntry.Format(g_szGformat, i+54);
			strResult = AfxGetApp()->GetProfileString(strRegKey, strEntry);
			if ( !strResult.IsEmpty() )
				ConvertWorkOffset(i, strResult);
		}
		VERIFY(strEntry.LoadString(IDS_REG_NCV_DEFFEED));
		if ( nResult == 1 ) {
			strResult = AfxGetApp()->GetProfileString(strRegKey, strEntry);
			m_dFeed = strResult.IsEmpty() ? g_dfDOrder[0] : (float)atof((LPCTSTR)strResult.Trim());
		}
		else
			m_dFeed = (float)AfxGetApp()->GetProfileInt(strRegKey, strEntry, (int)g_dfDOrder[0]);
		// ﾌｧｲﾙへの移行確認
		Convert();
	}

	// 機械情報をﾌｧｲﾙから読み込み(ﾚｼﾞｽﾄﾘ情報を上書き)
	if ( !m_strMCList.IsEmpty() )
		ReadMCoption(m_strMCList.GetHead());
}

CMCOption::~CMCOption()
{
	int			i;
	POSITION	pos = m_strMCList.GetHeadPosition();
	CString	strRegKey, strEntry, strFmt;
	VERIFY(strRegKey.LoadString(IDS_REGKEY_NC));
	VERIFY(strEntry.LoadString(IDS_REG_NCV_MCFILE));

	// 履歴の保存
	for ( i=0; i<NCMAXMCFILE; i++ ) {
		strFmt.Format(IDS_COMMON_FORMAT, strEntry, i);
		AfxGetApp()->WriteProfileString(strRegKey, strFmt, NULL);	// 先に履歴削除
	}
	for ( i=0; pos && i<NCMAXMCFILE; i++ ) {
		strFmt.Format(IDS_COMMON_FORMAT, strEntry, i);
		if ( !AfxGetApp()->WriteProfileString(strRegKey, strFmt, m_strMCList.GetNext(pos)) )
			break;
	}

	// 移行完了ｷｰの書き込み
	VERIFY(strEntry.LoadString(IDS_REG_CONVERT));
	AfxGetApp()->WriteProfileInt(strRegKey, strEntry, g_nConvert);

	// 工具情報の削除
	for ( pos=m_ltTool.GetHeadPosition(); pos; )
		delete	m_ltTool.GetNext(pos);
}

/////////////////////////////////////////////////////////////////////////////
// ﾒﾝﾊﾞ関数

void CMCOption::Convert(void)
{
	extern	LPTSTR	g_pszExecDir;
	static	const	int		nRegNCkey[] = {
		IDS_REG_NCV_INPUT, IDS_REG_NCV_DEFFEED, IDS_REG_NCV_FDOT,
		IDS_REG_NCV_FPROCESS
	};

	int		i;
	CString	strRegKey, strEntry, strFile("Init.mnc");
	CRegKey	reg;

	// ﾚｼﾞｽﾄﾘ情報の確認
	VERIFY(strRegKey.LoadString(IDS_REGKEY_NC));
	if ( reg.Open(HKEY_CURRENT_USER, gg_szRegKey+strRegKey) != ERROR_SUCCESS )
		return;
	VERIFY(strEntry.LoadString(IDS_REG_NCV_INPUT));
	if ( reg.QueryValue(strEntry, NULL, NULL, NULL) != ERROR_SUCCESS )
		return;

	// ﾌｧｲﾙへの移行確認
	if ( AfxMessageBox(IDS_ANA_MCFILESHIFT, MB_YESNO|MB_ICONQUESTION) == IDYES ) {
		if ( ::NCVC_FileDlgCommon(IDS_OPTION_MCSAVE, IDS_MC_FILTER, FALSE, 
					strFile, g_pszExecDir,
					FALSE, OFN_OVERWRITEPROMPT|OFN_HIDEREADONLY) == IDOK ) {
			if ( !SaveMCoption(strFile) )
				AfxMessageBox(IDS_ANA_NOMCFILE, MB_OK|MB_ICONINFORMATION);
		}
		else
			AfxMessageBox(IDS_ANA_NOMCFILE, MB_OK|MB_ICONINFORMATION);
	}
	else
		AfxMessageBox(IDS_ANA_NOMCFILE, MB_OK|MB_ICONINFORMATION);

	// 旧形式のﾚｼﾞｽﾄﾘ値削除
	for ( i=0; i<SIZEOF(nRegNCkey); i++ ) {
		VERIFY(strEntry.LoadString(nRegNCkey[i]));
		reg.DeleteValue(strEntry);
	}
	VERIFY(strEntry.LoadString(IDS_REG_NCV_MOVE));
	for ( i=0; i<NCXYZ; i++ )
		reg.DeleteValue(g_szNdelimiter[i]+strEntry);
	for ( i=0; i<WORKOFFSET; i++ ) {
		strEntry.Format(g_szGformat, i+54);
		reg.DeleteValue(strEntry);
	}
	reg.Close();
}

BOOL CMCOption::ReadMCoption(LPCTSTR lpszFile, BOOL bHistory/*=TRUE*/)
{
	int		i, j, k;
	CString	strRegKey, strEntry;
	TCHAR	szResult[_MAX_PATH];
/* 
	独自の入力は面倒なので
	Win32API の GetPrivateProfile[Int|String]() 関数を使う
*/
	VERIFY(strRegKey.LoadString(IDS_REGKEY_SETTINGS));

	// 旧ｷｰﾜｰﾄﾞの先読み
	m_nForceViewMode = ::GetPrivateProfileInt(strRegKey, g_szOldOrder[0], 0, lpszFile);

	// int型命令
	for ( i=j=k=0; i<MODALGROUP; i++, j++ ) {
		strEntry.Format(g_szNOrder[k], i);
		m_nModal[i] = ::GetPrivateProfileInt(strRegKey, strEntry, g_dfNOrder[j], lpszFile);
	}
	for ( i=0, k++; i<NCXYZ; i++, j++ ) {
		strEntry.Format(g_szNOrder[k], g_szNdelimiter[i]);
		m_nG0Speed[i] = ::GetPrivateProfileInt(strRegKey, strEntry, g_dfNOrder[j], lpszFile);
	}
	for ( k++; j<SIZEOF(m_unNums); j++, k++ ) {
		m_unNums[j] = ::GetPrivateProfileInt(strRegKey, g_szNOrder[k], g_dfNOrder[j], lpszFile);
	}

	// float型命令
	for ( i=j=k=0; i<NCXYZ; i++, j++ ) {
		strEntry.Format(g_szDOrder[k], g_szNdelimiter[i]);
		m_udNums[j] =	//	m_dInitialXYZ[i]
			::GetPrivateProfileString(strRegKey, strEntry, "", szResult, _MAX_PATH, lpszFile) > 0 ?
				(float)atof(::Trim(szResult).c_str()) : g_dfDOrder[j];
	}
	for ( k++; k<SIZEOF(g_szDOrder); j++, k++ ) {
		m_udNums[j] = ::GetPrivateProfileString(strRegKey, g_szDOrder[k], "", szResult, _MAX_PATH, lpszFile) > 0 ?
			(float)atof(::Trim(szResult).c_str()) : g_dfDOrder[j];
	}
	// ----------
		// ～Ver1.72までのﾊﾞｸﾞﾁｪｯｸ
		::GetPrivateProfileString(strRegKey, g_szDOrder[0], "", szResult, _MAX_PATH, lpszFile);
		if ( lstrlen(szResult) > 0 ) {
			// "BlockTime" が "Initial%c" に書き込んでいたﾊﾞｸﾞ
			m_dBlock = (float)atof(::Trim(szResult).c_str());
			// 正しいｷｰで出力
			// 　→単なる切り替えでは SaveMCoption() が呼ばれないので
			// 　　ここで処理する
			strEntry.Format(IDS_MAKENCD_FORMAT, m_dBlock);
			::WritePrivateProfileString(strRegKey, g_szDOrder[2], strEntry, lpszFile);
			// "Initial%c" のｴﾝﾄﾘを削除
			::WritePrivateProfileString(strRegKey, g_szDOrder[0], NULL, lpszFile);
		}
	// ----------
	for ( i=0; i<WORKOFFSET; i++ ) {
		strEntry.Format(g_szGformat, i+54);
		if ( ::GetPrivateProfileString(strRegKey, strEntry, "", szResult, _MAX_PATH, lpszFile) > 0 ) {
			ConvertWorkOffset(i, szResult);
			j += NCXYZ;
		}
		else {
			for ( k=0; k<NCXYZ; k++, j++ )
				m_dWorkOffset[i][k] = g_dfDOrder[j];
		}
	}

	// BOOL型命令
	for ( i=0; i<SIZEOF(m_ubFlgs); i++ ) {
		strEntry = g_szBOrder[i];
		m_ubFlgs[i] = ::GetPrivateProfileInt(strRegKey, strEntry, g_dfBOrder[i], lpszFile);
	}

	// CString型命令
	for ( i=0; i<MCMACROSTRING/*SIZEOF(g_szSOrderMacro)-1*/; i++ ) {
		::GetPrivateProfileString(strRegKey, g_szSOrderMacro[i], "", szResult, _MAX_PATH, lpszFile);
		m_strMacroOpt[i] = szResult;
	}
	k = 0;
	::GetPrivateProfileString(strRegKey, g_szSOrder[k++], "", szResult, _MAX_PATH, lpszFile);
	m_strMCname = szResult;
	::GetPrivateProfileString(strRegKey, g_szSOrder[k++], "", szResult, _MAX_PATH, lpszFile);
	m_strAutoBreak = szResult;

	// 工具情報
	CMCTOOLINFO*	pToolInfo;
	CMCTOOLINFO		tool;

	// 工具情報を一旦削除
	PLIST_FOREACH(pToolInfo, &m_ltTool)
		delete	pToolInfo;
	END_FOREACH
	m_ltTool.RemoveAll();

	// 命令を分割
	typedef tokenizer< char_separator<TCHAR> > tokenizer;
	static	char_separator<TCHAR> sep(gg_szComma, "", keep_empty_tokens);
	string	str, strTok;
	tokenizer	tok(str, sep);

	try {
		for ( i=0; TRUE; i++ ) {	// Toolｴﾝﾄﾘが読めなくなるまで
			strEntry.Format(g_szSOrder[k], i);
			if ( ::GetPrivateProfileString(strRegKey, strEntry, "", szResult, _MAX_PATH, lpszFile) <= 0 )
				break;
			str = szResult;
			tok.assign(str);
			tool.ClearOption();	j = 0;
			BOOST_FOREACH(strTok, tok) {
				strTok = ::Trim(strTok);	// stdafx.h
				switch ( j++ ) {
				case MCTOOL_T:		// Ｔ番号
					tool.m_nTool = atoi(strTok.c_str());
					break;
				case MCTOOL_NAME:	// 工具名
					tool.m_strName = strTok.c_str();
					break;
				case MCTOOL_D:		// 径補正
					tool.m_dToolD = (float)atof(strTok.c_str());
					break;
				case MCTOOL_H:		// 長補正
					tool.m_dToolH = (float)atof(strTok.c_str());
					break;
				}
			}
			pToolInfo = new CMCTOOLINFO(tool);
			m_ltTool.AddTail(pToolInfo);
		}
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		return FALSE;
	}

	// 履歴の更新
	if ( bHistory )
		return AddMCListHistory(lpszFile);

	return TRUE;
}

BOOL CMCOption::SaveMCoption(LPCTSTR lpszFile)
{
	int		i, j, k;
	CString	strRegKey, strEntry, strResult, strFormat;
	TCHAR	szResult[_MAX_PATH];

	if ( !lpszFile || lstrlen(lpszFile)<=0 )
		return FALSE;
/* 
	独自の出力は面倒なので
	Win32API の WritePrivateProfileString() 関数を使う
*/
	VERIFY(strRegKey.LoadString(IDS_REGKEY_SETTINGS));

	// int型命令
	for ( i=0, j=0, k=0; i<MODALGROUP; i++, j++ ) {
		strEntry.Format(g_szNOrder[k], i);
		strResult = lexical_cast<string>(m_nModal[i]).c_str();
		if ( !::WritePrivateProfileString(strRegKey, strEntry, strResult, lpszFile) )
			return FALSE;
	}
	for ( i=0, k++; i<NCXYZ; i++, j++ ) {
		strEntry.Format(g_szNOrder[k], g_szNdelimiter[i]);
		strResult = lexical_cast<string>(m_nG0Speed[i]).c_str();
		if ( !::WritePrivateProfileString(strRegKey, strEntry, strResult, lpszFile) )
			return FALSE;
	}
	for ( k++; j<SIZEOF(m_unNums); j++, k++ ) {
		strResult = lexical_cast<string>(m_unNums[j]).c_str();
		if ( !::WritePrivateProfileString(strRegKey, g_szNOrder[k], strResult, lpszFile) )
			return FALSE;
	}

	// float型命令
	for ( i=j=k=0; i<NCXYZ; i++, j++ ) {
		strEntry.Format(g_szDOrder[k], g_szNdelimiter[i]);
		strResult.Format(IDS_MAKENCD_FORMAT, m_udNums[j]);	// m_dInitialXYZ[i]
		if ( !::WritePrivateProfileString(strRegKey, strEntry, strResult, lpszFile) )
			return FALSE;
	}
	for ( k++; k<SIZEOF(g_szDOrder); j++, k++ ) {
		strResult.Format(IDS_MAKENCD_FORMAT, m_udNums[j]);
		if ( !::WritePrivateProfileString(strRegKey, g_szDOrder[k], strResult, lpszFile) )
			return FALSE;
	}
	for ( i=0; i<WORKOFFSET; i++ ) {
		strResult.Empty();
		for ( j=0; j<NCXYZ; j++ ) {
			if ( !strResult.IsEmpty() )
				strResult += gg_szComma;
			strFormat.Format(IDS_MAKENCD_FORMAT, m_dWorkOffset[i][j]);
			strResult += strFormat;
		}
		strEntry.Format(g_szGformat, i+54);
		if ( !::WritePrivateProfileString(strRegKey, strEntry, strResult, lpszFile) )
			return FALSE;
	}

	// BOOL型命令
	for ( i=0; i<SIZEOF(m_ubFlgs); i++ ) {
		strEntry = g_szBOrder[i];
		strResult = lexical_cast<string>(m_ubFlgs[i] ? 1 : 0).c_str();
		if ( !::WritePrivateProfileString(strRegKey, strEntry, strResult, lpszFile) )
			return FALSE;
	}

	// CString型命令
	for ( i=0; i<MCMACROSTRING/*SIZEOF(g_szSOrderMacro)-1*/; i++ ) {
		if ( !::WritePrivateProfileString(strRegKey, g_szSOrderMacro[i], m_strMacroOpt[i], lpszFile) )
			return FALSE;
	}
	k = 0;
	if ( !::WritePrivateProfileString(strRegKey, g_szSOrder[k++], m_strMCname, lpszFile) )
		return FALSE;
	if ( !::WritePrivateProfileString(strRegKey, g_szSOrder[k++], m_strAutoBreak, lpszFile) )
		return FALSE;

	// 工具情報を一旦削除(読み込み件数確認後)
	for ( j=0; TRUE; j++ ) {	// Toolｴﾝﾄﾘが読めなくなるまで
		strEntry.Format(g_szSOrder[k], j);
		if ( ::GetPrivateProfileString(strRegKey, strEntry, "", szResult, _MAX_PATH, lpszFile) <= 0 )
			break;
	}
	for ( i=0; i<j; i++ ) {
		strEntry.Format(g_szSOrder[k], i);
		::WritePrivateProfileString(strRegKey, strEntry, NULL, lpszFile);
	}
	// 工具情報
	POSITION	pos = m_ltTool.GetHeadPosition();
	CMCTOOLINFO*	pToolInfo;
	for ( i=0; pos; i++ ) {
		pToolInfo = m_ltTool.GetNext(pos);
		strEntry.Format(g_szSOrder[k], i);
		strResult  = lexical_cast<string>(pToolInfo->m_nTool).c_str();
		strResult += gg_szComma + pToolInfo->m_strName;
		strFormat.Format(IDS_MAKENCD_FORMAT, pToolInfo->m_dToolD);
		strResult += gg_szComma + strFormat;
		strFormat.Format(IDS_MAKENCD_FORMAT, pToolInfo->m_dToolH);
		strResult += gg_szComma + strFormat;
		if ( !::WritePrivateProfileString(strRegKey, strEntry, strResult, lpszFile) )
			return FALSE;
		// 仮登録ﾌﾗｸﾞ初期化
		pToolInfo->m_bDlgAdd = pToolInfo->m_bDlgDel = FALSE;
	}

	// 旧ﾃﾞｰﾀの削除
	for ( i=0; i<SIZEOF(g_szOldOrder); i++ )
		::WritePrivateProfileString(strRegKey, g_szOldOrder[i], NULL, lpszFile);

	// 履歴の更新
	return AddMCListHistory(lpszFile);
}

void CMCOption::ConvertWorkOffset(size_t n, LPCTSTR lpszResult)
{
	ASSERT( n>=0 && n<WORKOFFSET );
	LPTSTR	lpsztok, lpszcontext, lpszBuf = NULL;
	int		i;

	for ( i=0; i<NCXYZ; m_dWorkOffset[n][i++]=0 );

	try {
		lpszBuf = new TCHAR[lstrlen(lpszResult)+1];
		lpsztok = strtok_s(lstrcpy(lpszBuf, lpszResult), gg_szComma, &lpszcontext);
		for ( i=0; i<NCXYZ && lpsztok; i++ ) {
			m_dWorkOffset[n][i] = (float)atof(::Trim(lpsztok).c_str());
			lpsztok = strtok_s(NULL, gg_szComma, &lpszcontext);
		}
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
	}
	if ( lpszBuf )
		delete[]	lpszBuf;
}

BOOL CMCOption::AddMCListHistory(LPCTSTR lpszSearch)
{
	if ( !lpszSearch || lstrlen(lpszSearch) <= 0 )
		return TRUE;
	if ( !m_strMCList.IsEmpty() &&
			m_strMCList.GetHead().CompareNoCase(lpszSearch) == 0 )
		return TRUE;

	// 文字列の検索(Findは大文字小文字を区別するので使えない)
	try {
		POSITION	pos1, pos2;
		for ( pos1=m_strMCList.GetHeadPosition(); (pos2 = pos1); ) {
			if ( m_strMCList.GetNext(pos1).CompareNoCase(lpszSearch) == 0 ) {
				// 文字列があれば，それを消して先頭へ
				m_strMCList.RemoveAt(pos2);
				m_strMCList.AddHead(lpszSearch);
				return TRUE;
			}
		}
		// なければ先頭に追加
		m_strMCList.AddHead(lpszSearch);
		// 10個越えれば最後を消去
		if ( m_strMCList.GetCount() > NCMAXMCFILE )
			m_strMCList.RemoveTail();
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		return FALSE;
	}

	return TRUE;
}

optional<float> CMCOption::GetToolD(int nTool) const
{
	PLIST_FOREACH(CMCTOOLINFO* pTool, &m_ltTool)
		if ( pTool->m_nTool == nTool )
			return pTool->m_dToolD;
	END_FOREACH
	return optional<float>();
}

optional<float> CMCOption::GetToolH(int nTool) const
{
	PLIST_FOREACH(CMCTOOLINFO* pTool, &m_ltTool)
		if ( pTool->m_nTool == nTool )
			return pTool->m_dToolH;
	END_FOREACH
	return optional<float>();
}

BOOL CMCOption::AddTool(int nTool, float d, BOOL bAbs)
{
	CMCTOOLINFO*	pTool;
	BOOL			bMatch = FALSE, bResult = TRUE;

	// 存在ﾁｪｯｸ
	PLIST_FOREACH(pTool, &m_ltTool)
		if ( pTool->m_nTool == nTool ) {
			bMatch = TRUE;
			break;
		}
	END_FOREACH
	if ( bMatch && !bAbs )
		d += pTool->m_dToolD;	// ｲﾝｸﾘﾒﾝﾀﾙ

	// 補正値仮登録
	CMCTOOLINFO		tool;
	tool.ClearOption();
	tool.m_nTool   = nTool;
	tool.m_dToolD  = d;
	tool.m_bDlgAdd = TRUE;		// 仮登録ﾏｰｸ
	try {
		pTool = new CMCTOOLINFO(tool);
		m_ltTool.AddHead(pTool);	// 先に検索されるよう先頭に登録
	}
	catch (CMemoryException* e) {
		e->Delete();
		bResult = FALSE;
	}

	return bResult;
}

void CMCOption::ReductionTools(BOOL bAdd)
{
	CMCTOOLINFO*	pToolInfo;
	POSITION		pos1, pos2;

	// ﾀﾞｲｱﾛｸﾞ等で仮登録された工具情報を整理(削除)
	for ( pos1=m_ltTool.GetHeadPosition(); (pos2 = pos1); ) {
		pToolInfo = m_ltTool.GetNext(pos1);
		BOOL&	bFlg = bAdd ? pToolInfo->m_bDlgAdd : pToolInfo->m_bDlgDel;
		if ( bFlg ) {
			delete	pToolInfo;
			m_ltTool.RemoveAt(pos2);
		}
	}
}

void CMCOption::AddMCHistory_ComboBox(CComboBox& combo)
{
	static	LPCTSTR	ss_lpszRefer = "参照...";

	combo.ResetContent();

	// ｺﾝﾎﾞﾎﾞｯｸｽに機械情報の履歴を追加
	CString	strBuf, strPath, strFile;
	PLIST_FOREACH(strBuf, &m_strMCList)
		::Path_Name_From_FullPath(strBuf, strPath, strFile);
		combo.AddString(strFile);
	END_FOREACH
	combo.AddString( ss_lpszRefer );

	combo.SetCurSel( combo.GetCount() > 1 ? 0 : -1 );
}
/*
	--- 以下 MainFrm.cpp を参考
*/
CString	CMCOption::MakeMacroCommand(int a) const
{
	CString	strResult;
	ASSERT( a>=0 && a<SIZEOF(g_szSOrderMacro) );
	if ( a>=0 && a<SIZEOF(g_szSOrderMacro) ) {
		strResult  = "${";
		strResult += g_szSOrderMacro[a];
		strResult += "}";
	}
	return strResult;
}

CString CMCOption::GetDefaultOption(void) const
{
	CString	strResult;
	BOOST_FOREACH( auto ref, g_nDefaultMacroID ) {
		if ( !strResult.IsEmpty() )
			strResult += " ";
		strResult += MakeMacroCommand(ref);
	}
	return strResult;
}
