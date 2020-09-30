// stdafx.h : 標準のシステム インクルード ファイルのインクルード ファイル、または
// 参照回数が多く、かつあまり変更されない、プロジェクト専用のインクルード ファイル
// を記述します。

#pragma once

#define	NO_WARN_MBCS_MFC_DEPRECATION		// VS2015～
#define	_BIND_TO_CURRENT_VCLIBS_VERSION
#define _SECURE_ATL

/*
#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// Windows ヘッダーから使用されていない部分を除外します。
#endif
// 一部の CString コンストラクタは明示的です。
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS
// 一般的で無視しても安全な MFC の警告メッセージの一部の非表示を解除します。
#define _AFX_ALL_WARNINGS
*/
#include "targetver.h"
#include <afxwin.h>         // MFC のコアおよび標準コンポーネント
#include <afxext.h>         // MFC の拡張部分
#include <afxdisp.h>        // MFC オートメーション クラス
#include <afxtempl.h>		// MFC TemplateClass
#include <atlbase.h>		// CRegKey
#include <afxadv.h>			// NCVC.cpp にて CRecentFileList を扱う
#include <afxmt.h>			// ｽﾚｯﾄﾞ関連
#include <afxcview.h>		// CListView, CTreeView
#include <afxdlgs.h>
#include <gdiplus.h>

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>		// MFC の Internet Explorer 4 コモン コントロール サポート
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC の Windows コモン コントロール サポート
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxcontrolbars.h>	// MFC におけるリボンとコントロール バーのサポート

// OpenGL
#define	GLEW_STATIC			// GLEW static link
#include <gl/glew.h>		// OpenGL Extention
#include <gl/wglew.h>
#include <gl/gl.h>			// OpenGL

// Kodatuno
#define	USE_KODATUNO
#ifdef USE_KODATUNO
#ifdef _DEBUG
#pragma comment(lib, "Kodatuno_R3.4d.lib")
#elif defined(_WIN64)
#pragma comment(lib, "Kodatuno_R3.4_x64.lib")
#else
#pragma comment(lib, "Kodatuno_R3.4.lib")
#endif
#endif

// STL
#define	_SCL_SECURE_NO_WARNINGS			// 対Boost1.44用
#include <string>
#include <vector>
#include <bitset>
#include <algorithm>

#pragma	warning( disable : 4800 )		// BOOL setting

// BOOST Libraries
#pragma	warning( disable : 4819 )		// codepage
#pragma	warning( disable : 4348 )		// boost 1.60～ ???
#include "boost/regex.hpp"
#include "boost/tokenizer.hpp"
#include "boost/tuple/tuple.hpp"
#include "boost/optional.hpp"
#include "boost/variant.hpp"
#include "boost/bind/bind.hpp"
#include "boost/function.hpp"
#include "boost/utility.hpp"
#include "boost/foreach.hpp"
#include "boost/range/algorithm.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/algorithm/minmax.hpp"
#include "boost/algorithm/string.hpp"

// VS2015 不具合
void MySplitPath(LPCTSTR lpszFullPath,
	LPTSTR pszDrive, size_t nDrive, LPTSTR pszPath, size_t nPath,
	LPTSTR pszName,  size_t nName,  LPTSTR pszExt,  size_t nExt);
#ifdef _tsplitpath_s
#undef _tsplitpath_s
#define _tsplitpath_s MySplitPath
#endif

#define	NCVCSERIALVERSION_1503	1503	// v1.00RC～
#define	NCVCSERIALVERSION_1505	1505	// v1.10～
#define	NCVCSERIALVERSION_1507	1507	// v1.10a～
#define	NCVCSERIALVERSION_1600	1600	// v1.60～ (CDXFworkingOutlineのﾃﾞｰﾀ変更)
#define	NCVCSERIALVERSION_1700	1700	// v1.70～ (CDXFworkingOutlineのｵﾌｾｯﾄ値追加)
#define	NCVCSERIALVERSION_2300	2300	// v2.30～ (旋盤原点ﾃﾞｰﾀ)
#define	NCVCSERIALVERSION_3600	3600	// v3.60～ (CADﾃﾞｰﾀの統合)
#define	NCVCSERIALVERSION_3602	3602	// v3.60b～ (閉じたPOLYLINEの方向判定ﾃﾞｰﾀ)
#define	NCVCSERIALVERSION_3620	3620	// v3.62～ (double->float)
#define	NCVCSERIALVERSION		NCVCSERIALVERSION_3620

enum	DOCTYPE		{TYPE_NCD = 0, TYPE_DXF = 1};

// Common Define
#define	SIZEOF(array)			( sizeof(array)/sizeof(array[0]) )
#define	AfxGetNCVCApp()			( static_cast<CNCVCApp *>(AfxGetApp()) )
#define	AfxGetNCVCMainWnd()		( static_cast<CMainFrame *>(AfxGetMainWnd()) )
#define	LOMETRICFACTOR			10.0f

// Timer Event
#define	IDC_SPLASH_TIMER		100
#define	IDC_STATUSBAR_EVENT		101
#define	IDC_OPENGL_DRAGROUND	110

// ﾏﾆｭｱﾙ操作のｳｨﾝﾄﾞｳID
#define	ID_NC_STATUSBAR			30100
#define	ID_DXF_STATUSBAR		30101
#define	ID_MAIN_PROGRESS		30900

// ﾘｿｰｽ定義のないﾂｰﾙﾊﾞｰ
#define	IDR_ADDINBAR			120
#define	IDR_EXECBAR				121

// ｱﾄﾞｲﾝ，外部ｱﾌﾟﾘｹｰｼｮﾝ用ﾒﾆｭｰID
#define	EXECSTARTID				40000
#define	ADDINSTARTID			40100
#define	EXECADDIN_ENDID			40200

// UserMessage
	// ｽﾚｯﾄﾞ終了 ThreadDlg.cpp 他
#define	WM_USERFINISH			WM_USER+100
	// ﾌﾚｰﾑへﾌｧｲﾙ変更通知
#define	WM_USERFILECHANGENOTIFY	WM_USER+101
	// CViewBase::OnInitialUpdate() で正確なｳｨﾝﾄﾞｳｻｲｽﾞを得るためのﾒｯｾｰｼﾞ
#define	WM_USERINITIALUPDATE	WM_USER+105
	// ｽﾚｯﾄﾞからﾒｲﾝｽﾃｰﾀｽﾊﾞｰのﾌﾟﾛｸﾞﾚｽｺﾝﾄﾛｰﾙへのﾎﾟｼﾞｼｮﾝｾｯﾄ
#define	WM_USERPROGRESSPOS		WM_USER+106
	// ﾓｰﾄﾞﾚｽﾀﾞｲｱﾛｸﾞへのﾄﾞｷｭﾒﾝﾄ切替通知
#define	WM_USERSWITCHDOCUMENT	WM_USER+107
	// CTabViewBase のﾍﾟｰｼﾞ切替ｲﾍﾞﾝﾄ
#define	WM_USERACTIVATEPAGE		WM_USER+108
	// MDI子ｳｨﾝﾄﾞｳのｼｽﾃﾑｺﾏﾝﾄﾞ(最大化,元に戻す)に対するﾌｨｯﾄﾒｯｾｰｼﾞ 兼
	// 各ﾋﾞｭｰへのﾌｨｯﾄﾒｯｾｰｼﾞ
#define	WM_USERVIEWFITMSG		WM_USER+109
	// ﾄﾚｰｽ実行ｽﾚｯﾄﾞからの通知(CTraceThread -> CNCListView)
#define WM_USERTRACESELECT		WM_USER+110
	// Gｺｰﾄﾞのｽﾃｰﾀｽﾊﾞｰ更新(CNCListView -> CNCChild)
#define WM_USERSTATUSLINENO		WM_USER+111
	// CADﾃﾞｰﾀの統合(CNCVCApp::OnFileCADbind() -> CDXFView )
#define	WM_USERBINDINIT			WM_USER+112
	// CADﾃﾞｰﾀの統合(CDXFView[child] -> CDXFView[parent] )
#define	WM_USERBIND_LDOWN		WM_USER+113
#define	WM_USERBIND_ROUND		WM_USER+114
#define	WM_USERBIND_CANCEL		WM_USER+115

/////////////////////////////////////////////////////////////////////////////
// NCVC 共通関数・マクロ

// ﾃｷｽﾄﾌｧｲﾙの行ﾁｪｯｸ
inline	BOOL	NC_IsNullLine(const CString& str)	// EOF 等の行
{
	return ( str.IsEmpty() || str[0]=='#' || str[0]=='\x1a' );
}
/*	-- boost::algorithm::trim()
//前後の空白文字を削除
inline std::string Trim(const std::string &str)
{
	std::string	strResult;
    size_t sPos = str.find_first_not_of(' ');
    size_t ePos = str.find_last_not_of(' ');
    if ( sPos != -1 )
		strResult = str.substr(sPos, ePos - sPos + 1);
    return strResult;
}
inline std::string Trim(LPCTSTR str)
{
	return Trim(std::string(str));
}
*/
// 乱数
int		GetRandom(int min, int max);

// ﾌﾙﾊﾟｽ名をﾊﾟｽ名とﾌｧｲﾙ名に分割
void	Path_Name_From_FullPath(LPCTSTR, CString&, CString&, BOOL = TRUE);

// 相対パスを返す
CString	RelativePath(LPCTSTR, LPCTSTR);

// ﾚｼﾞｽﾄﾘの階層文字列
inline	CString	GetSubTreeRegKey(int nID1, int nID2)
{
	CString	strID1, strID2;
	VERIFY(strID1.LoadString(nID1));
	VERIFY(strID2.LoadString(nID2));
	return (strID1 + "\\" + strID2);
}

// ﾀﾞｲｱﾛｸﾞﾀｲﾄﾙにﾌｧｲﾙ名を付加
inline	CString	AddDialogTitle2File(const CString& strBase, const CString& strFullPath)
{
	CString	strPath, strFile;
	::Path_Name_From_FullPath(strFullPath, strPath, strFile);
	return strBase + " --- " + strFile;
}

// XpStyleの解除
// Xp以外にも対応するためLoadLibrary()+GetProcAddress()で関数ｱﾄﾞﾚｽ取得
inline void DisableXpStyle(HWND hWnd)
{
	typedef	HRESULT	(WINAPI *pFuncSetWindowTheme)(HWND, LPCWSTR, LPCWSTR);
	HINSTANCE			hLib;
	pFuncSetWindowTheme	pProc;
	if ( (hLib=::LoadLibrary(_T("UxTheme.dll"))) != NULL ) {
		if ( (pProc=(pFuncSetWindowTheme)::GetProcAddress(hLib, _T("SetWindowTheme"))) != NULL ) {
			// SetWindowTheme() を空の文字列で呼び出すとﾋﾞｼﾞｭｱﾙｽﾀｲﾙ解除
			(pProc)(hWnd, L"", L"");
		}
		::FreeLibrary(hLib);
	}
}

// ﾌｧｲﾙのﾊﾞｰｼﾞｮﾝﾘｿｰｽの取得
LPVOID	GetVersionResource(LPCTSTR lpszFileName, LPDWORD* dwTrans);
// ﾊﾞｰｼﾞｮﾝﾘｿｰｽからﾊﾞｰｼﾞｮﾝ情報のｺﾋﾟｰを取得
//	Keyword:
//		"Comments"         // コメント
//		"CompanyName"      // 会社名
//		"FileDescription"  // 説明
//		"FileVersion"      // ファイルバージョン
//		"InternalName"     // 内部名
//		"LegalCopyright"   // 著作権
//		"LegalTrademarks"  // 商標
//		"OriginalFilename" // 正式ファイル名
//		"PrivateBuild"     // プライベートビルド情報
//		"ProductName"      // 製品名
//		"ProductVersion"   // 製品バージョン
//		"SpecialBuild"     // スペシャルビルド情報
BOOL	GetVersionValue(CString& strBuffer, LPVOID pVersionInfo, DWORD dwTrans, LPCTSTR strKeyWord);

#ifdef _DEBUG
// MagaDebugからAllocConsole()使用にチェンジ
#include <stdio.h>
#include <iostream>
#include "DbgConsole.h"
// GetLastMessage() のﾒｯｾｰｼﾞ整形
void	NC_FormatMessage(void);
#endif

// NCVC original
#include "MyTemplate.h"
#include "CustomClass.h"
#include "CustomControl.h"
/*
// XpStyle Manifest
//#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
//#endif
*/
