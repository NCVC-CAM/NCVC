// stdafx.cpp : 標準インクルード NCVC.pch のみを
// 含むソース ファイルは、プリコンパイル済みヘッダーになります。
// stdafx.obj にはプリコンパイルされた型情報が含まれます。

#include "stdafx.h"

// NCVCﾚｼﾞｽﾄﾘｷｰ
extern	LPCTSTR	gg_szRegKey = "Software\\MNCT-S\\NCVC\\";

// 改行文字
extern	LPCTSTR	gg_szReturn = "\n";
// CR+LF
extern	LPCTSTR	gg_szCRLF = "\r\n";
// スペース
extern	LPCTSTR	gg_szSpace = " ";
// 分解文字
extern	LPCTSTR	gg_szDelimiter = ":";
// 分解文字（拡張子フィルタ）
extern	LPCTSTR	gg_szSemicolon = ";";
// 文字連結
extern	LPCTSTR	gg_szCat = ", ";
// カンマ分割
extern	LPCTSTR	gg_szComma = ",";
// ﾜｲﾙﾄﾞｶｰﾄﾞ
extern	LPCTSTR	gg_szWild = "*.";
// 円記号
extern	LPCTSTR	gg_szEn = "\\";
// スクリプト拡張子
extern	LPCTSTR	gg_szScriptExt[] = {
	".pl", ".py"
};
// スクリプト実行
extern	LPCTSTR	gg_szScriptRun[] = {
	"perl.exe", "python.exe"
};

// ｱｲｺﾝｻｲｽﾞ
extern	const	int		gg_nIconX = 16;
extern	const	int		gg_nIconY = 15;

/////////////////////////////////////////////////////////////////////////////
// NCVC 共通関数

// 自作_tsplitpath_s(VS2015の不具合)
void MySplitPath(LPCTSTR lpszFullPath,
	LPTSTR pszDrive, size_t nDrive, LPTSTR pszPath, size_t nPath,
	LPTSTR pszName,  size_t nName,  LPTSTR pszExt,  size_t nExt)
{
	setlocale(LC_ALL, "japanese");
	size_t	nResult;
	WCHAR	wszFullPath[_MAX_PATH],
			wszDrive[_MAX_DRIVE],
			wszPath[_MAX_DIR],
			wszName[_MAX_FNAME],
			wszExt[_MAX_EXT];
	mbstowcs_s(&nResult, wszFullPath, lpszFullPath, lstrlen(lpszFullPath)+1);
	_wsplitpath_s(wszFullPath,
		wszDrive, SIZEOF(wszDrive), wszPath, SIZEOF(wszPath),
		wszName, SIZEOF(wszName), wszExt, SIZEOF(wszExt));
	if ( pszDrive )
		wcstombs_s(&nResult, pszDrive, _MAX_DRIVE, wszDrive, _MAX_DRIVE);
	if ( pszPath )
		wcstombs_s(&nResult, pszPath,  _MAX_DIR,   wszPath,  _MAX_DIR);
	if ( pszName )
		wcstombs_s(&nResult, pszName,  _MAX_FNAME, wszName,  _MAX_FNAME);
	if ( pszExt )
		wcstombs_s(&nResult, pszExt,   _MAX_EXT,   wszExt,   _MAX_EXT);
}

// 乱数
int GetRandom(int min, int max)
{
	static	int		nFirst = 0;

	if ( nFirst == 0 ) {
		srand((unsigned int)time(NULL));
		nFirst = 1;
	}

	return min + (int)(rand()*(max-min+1.0)/(1.0+RAND_MAX));
}

// ﾌﾙﾊﾟｽ名をﾊﾟｽ名とﾌｧｲﾙ名に分割
void Path_Name_From_FullPath
	(LPCTSTR lpszFullPath, CString& strPath, CString& strName, BOOL bExt/*=TRUE*/)
{
	if ( !lpszFullPath || lstrlen(lpszFullPath) <= 0 ) {
		strPath.Empty();
		strName.Empty();
		return;
	}
	TCHAR	szDrive[_MAX_DRIVE],
			szDir[_MAX_DIR],
			szFileName[_MAX_FNAME],
			szExt[_MAX_EXT];
	_tsplitpath_s(lpszFullPath,
		szDrive, SIZEOF(szDrive), szDir, SIZEOF(szDir),
		szFileName, SIZEOF(szFileName), szExt, SIZEOF(szExt));
	strPath  = szDrive;
	strPath += szDir;
	strName  = szFileName;
	if ( bExt )
		strName += szExt;
}

// 相対パスを返す
CString	RelativePath(LPCTSTR lpszBase, LPCTSTR lpszSrc)
{
	CString	strResult,
			strPath1, strFile1,
			strPath2, strFile2;
	TCHAR	szRelativePath[_MAX_PATH];

	// なぜか Vista で PathRelativePathTo() が正常なパスを返さない
	// 同じパスに対して ..\ を返す
//	strResult = ::PathRelativePathTo(szRelativePath, lpszBase, FILE_ATTRIBUTE_NORMAL,
//					lpszSrc, FILE_ATTRIBUTE_NORMAL) ? szRelativePath : lpszSrc;

	// ディレクトリだけにして判断させる
	Path_Name_From_FullPath(lpszBase, strPath1, strFile1);
	Path_Name_From_FullPath(lpszSrc,  strPath2, strFile2);
	if ( ::PathRelativePathTo(szRelativePath, strPath1, FILE_ATTRIBUTE_DIRECTORY, strPath2, FILE_ATTRIBUTE_DIRECTORY) ) {
		strResult = szRelativePath;
		if ( strResult.Right(1) != gg_szEn )
			strResult += gg_szEn;	// 同じパスのとき「.」だけ返されるための対策
		strResult += strFile2;
	}
	else
		strResult = lpszSrc;

	return strResult;
}

// ﾌｧｲﾙのﾊﾞｰｼﾞｮﾝﾘｿｰｽの取得
LPVOID GetVersionResource(LPCTSTR lpszFileName, LPDWORD* pdwTrans)
{
	LPVOID	pVersionInfo;	// ﾊﾞｰｼﾞｮﾝﾘｿｰｽを指すﾎﾟｲﾝﾀ
	UINT	uDumy;
	DWORD	dwDumy;
	DWORD	dwSize;			// ﾊﾞｰｼﾞｮﾝﾘｿｰｽのｻｲｽﾞ

	// ﾊﾞｰｼﾞｮﾝﾘｿｰｽのｻｲｽﾞを取得
	dwSize = GetFileVersionInfoSize( const_cast<LPTSTR>(lpszFileName), &dwDumy );
	if ( dwSize <= 0 )
		return NULL;

	pVersionInfo = new byte [dwSize];
	if ( !pVersionInfo )
		return NULL;

	// ﾊﾞｰｼﾞｮﾝﾘｿｰｽを取得
	if ( GetFileVersionInfo( const_cast<LPTSTR>(lpszFileName), 0, dwSize, pVersionInfo ) ) {
		// ﾊﾞｰｼﾞｮﾝﾘｿｰｽの言語情報を取得
		if ( VerQueryValue( pVersionInfo, "\\VarFileInfo\\Translation", (LPVOID*)pdwTrans, &uDumy ) )
			return pVersionInfo;
	}
	delete[]	pVersionInfo;

	return	NULL;
}

// ﾊﾞｰｼﾞｮﾝﾘｿｰｽからﾊﾞｰｼﾞｮﾝ情報のｺﾋﾟｰを取得
BOOL GetVersionValue(CString& strBuffer, LPVOID pVersionInfo, DWORD dwTrans, LPCTSTR strKeyWord)
{
	static	WORD  wCodePageID[] = { 0, 932, 949, 950, 1200, 1250, 1251, 1252, 1253, 1254, 1255, 1256 };
	static	WORD  wLanguageID[] = {
		0x0401, 0x0402, 0x0403, 0x0404, 0x0405, 0x0406, 0x0407, 0x0408,
		0x0409, 0x040A, 0x040B, 0x040C, 0x040D, 0x040E, 0x040F, 0x0410,
		0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417, 0x0418,
		0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F, 0x0420,
		0x0421, 0x0804, 0x0807, 0x0809, 0x080A, 0x080C, 0x0810, 0x0813,
		0x0814, 0x0816, 0x081A, 0x0C0C, 0x100C
	};
	LPTSTR	lpszValue;	// ﾊﾞｰｼﾞｮﾝﾘｿｰｽ中のﾊﾞｰｼﾞｮﾝ情報を指すﾎﾟｲﾝﾀ
	CString	strPath;
	UINT	uDumy;

	strPath.Format("\\StringFileInfo\\%04x%04x\\%s", LOWORD(dwTrans), HIWORD(dwTrans), strKeyWord);
	if ( VerQueryValue( pVersionInfo, const_cast<LPTSTR>((LPCTSTR)strPath), (LPVOID*)&lpszValue, &uDumy ) ) {
		strBuffer = lpszValue;
		return TRUE;
	}

	// バージョンリソースに言語情報が記録されていない場合
	for( int i=0; i<SIZEOF(wCodePageID); i++ ) {
		for( int j=0; j<SIZEOF(wLanguageID); j++ ) {
			strPath.Format("\\StringFileInfo\\%04x%04x\\%s", wLanguageID[j], wCodePageID[i], strKeyWord);
			if ( VerQueryValue( pVersionInfo, const_cast<LPTSTR>((LPCTSTR)strPath), (LPVOID*)&lpszValue, &uDumy ) ) {
				strBuffer = lpszValue;
				return TRUE;
			}
		}
	}

	strBuffer.Empty();
	return FALSE;
}

// スクリプト実行ファイルを取得
CString	GetScriptExec(const CString& strFile)
{
	CString	strExt( strFile.Right(3) );
	for ( int i=0; i<SIZEOF(gg_szScriptExt); i++ ) {
		if ( strExt.CompareNoCase(gg_szScriptExt[i]) == 0 )
			return CString( gg_szScriptRun[i] );
	}
	return CString();
}

#ifdef _DEBUG
// GetLastMessage() のﾒｯｾｰｼﾞ整形
void NC_FormatMessage(void)
{
	LPVOID	lpMsgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("ErrorMsg = %s\n", (char*)lpMsgBuf);
	LocalFree( lpMsgBuf );
}
#endif
