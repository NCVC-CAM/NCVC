// 3dOption.cpp: C3dOption クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "3dOption.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace boost;
using std::string;

// int型命令
static	LPCTSTR	g_szNOrder[] = {
	"LineSplit"
};
static	const	int		g_dfNOrder[] = {
	50
};
// float型命令
static	LPCTSTR	g_szDOrder[] = {
	"BallEndmill",
	"WorkHeight",
	"ZCut"
};
static	const	float	g_dfDOrder[] = {
	3.0f, 50.0f, 5.0f
};
// BOOL型命令
static	LPCTSTR	g_szFOrder[] = {
	"ZOrigin"
};
static	const	BOOL	g_dfFOrder[] = {
	TRUE
};

/////////////////////////////////////////////////////////////////////////////
// C3dOption クラスの構築/消滅

C3dOption::C3dOption()
{
	ASSERT( D3_INT_NUMS == SIZEOF(g_dfNOrder) );
	ASSERT( D3_DBL_NUMS == SIZEOF(g_dfDOrder) );
	ASSERT( D3_FLG_NUMS == SIZEOF(g_dfFOrder) );

	int		i;

	// メンバ変数の初期化
	for ( i=0; i<SIZEOF(g_dfNOrder); i++ )
		m_unNums[i] = g_dfNOrder[i];
	for ( i=0; i<SIZEOF(g_dfDOrder); i++ )
		m_udNums[i] = g_dfDOrder[i];
	for ( i=0; i<SIZEOF(g_dfFOrder); i++ )
		m_ubFlgs[i] = g_dfFOrder[i];
}

C3dOption::~C3dOption()
{
}

/////////////////////////////////////////////////////////////////////////////
// メンバ関数

BOOL C3dOption::Read3dOption(LPCTSTR lpszFile)
{
	int		i;
	CString	strRegKey;
	TCHAR	szResult[_MAX_PATH];

	// ドキュメントファイル名から拡張子を .ncvc に変更
	// 存在しなくてもファイル名だけは先にセット
	VERIFY(strRegKey.LoadString(IDR_MAINFRAME));
	TCHAR	szDrive[_MAX_DRIVE],
			szDir[_MAX_DIR],
			szFileName[_MAX_FNAME],
			szExt[_MAX_EXT];
	_tsplitpath_s(lpszFile,
		szDrive, SIZEOF(szDrive), szDir, SIZEOF(szDir),
		szFileName, SIZEOF(szFileName), szExt, SIZEOF(szExt));
	m_str3dOptionFile  = szDrive;
	m_str3dOptionFile += szDir;
	m_str3dOptionFile += szFileName;
	m_str3dOptionFile += '.' + strRegKey.MakeLower();

	if ( !::IsFileExist(lpszFile, TRUE, FALSE) )
		return FALSE;

	// 独自の入力は面倒なので
	// Win32API の GetPrivateProfile[Int|String]() 関数を使う
	VERIFY(strRegKey.LoadString(IDS_REGKEY_SETTINGS));

	// int型命令
	for ( i=0; i<SIZEOF(m_unNums); i++ ) {
		m_unNums[i] = ::GetPrivateProfileInt(strRegKey, g_szNOrder[i], g_dfNOrder[i], m_str3dOptionFile);
	}
	// float型命令
	for ( i=0; i<SIZEOF(m_udNums); i++ ) {
		m_udNums[i] = ::GetPrivateProfileString(strRegKey, g_szDOrder[i], "", szResult, _MAX_PATH, m_str3dOptionFile) > 0 ?
			(float)atof(boost::algorithm::trim_copy(string(szResult)).c_str()) : g_dfDOrder[i];
	}
	// BOOL型命令
	for ( i=0; i<SIZEOF(m_ubFlgs); i++ ) {
		m_ubFlgs[i] = ::GetPrivateProfileInt(strRegKey, g_szFOrder[i], g_dfFOrder[i], m_str3dOptionFile);
	}

	return TRUE;
}

BOOL C3dOption::Save3dOption(void)
{
	int		i;
	CString	strRegKey, strResult;

	// 独自の出力は面倒なので
	// Win32API の WritePrivateProfileString() 関数を使う
	VERIFY(strRegKey.LoadString(IDS_REGKEY_SETTINGS));

	// int型命令
	for ( i=0; i<SIZEOF(m_unNums); i++ ) {
		strResult = lexical_cast<string>(m_unNums[i]).c_str();
		if ( !::WritePrivateProfileString(strRegKey, g_szNOrder[i], strResult, m_str3dOptionFile) )
			return FALSE;
	}
	// float型命令
	for ( i=0; i<SIZEOF(m_udNums); i++ ) {
		strResult.Format(IDS_MAKENCD_FORMAT, m_udNums[i]);
		if ( !::WritePrivateProfileString(strRegKey, g_szDOrder[i], strResult, m_str3dOptionFile) )
			return FALSE;
	}
	// BOOL型命令
	for ( i=0; i<SIZEOF(m_ubFlgs); i++ ) {
		strResult = lexical_cast<string>(m_ubFlgs[i] ? 1 : 0).c_str();
		if ( !::WritePrivateProfileString(strRegKey, g_szFOrder[i], strResult, m_str3dOptionFile) )
			return FALSE;
	}

	return TRUE;
}
