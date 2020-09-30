// stdafx.h : �W���̃V�X�e�� �C���N���[�h �t�@�C���̃C���N���[�h �t�@�C���A�܂���
// �Q�Ɖ񐔂������A�����܂�ύX����Ȃ��A�v���W�F�N�g��p�̃C���N���[�h �t�@�C��
// ���L�q���܂��B

#pragma once

#define	NO_WARN_MBCS_MFC_DEPRECATION		// VS2015�`
#define	_BIND_TO_CURRENT_VCLIBS_VERSION
#define _SECURE_ATL

/*
#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// Windows �w�b�_�[����g�p����Ă��Ȃ����������O���܂��B
#endif
// �ꕔ�� CString �R���X�g���N�^�͖����I�ł��B
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS
// ��ʓI�Ŗ������Ă����S�� MFC �̌x�����b�Z�[�W�̈ꕔ�̔�\�����������܂��B
#define _AFX_ALL_WARNINGS
*/
#include "targetver.h"
#include <afxwin.h>         // MFC �̃R�A����ѕW���R���|�[�l���g
#include <afxext.h>         // MFC �̊g������
#include <afxdisp.h>        // MFC �I�[�g���[�V���� �N���X
#include <afxtempl.h>		// MFC TemplateClass
#include <atlbase.h>		// CRegKey
#include <afxadv.h>			// NCVC.cpp �ɂ� CRecentFileList ������
#include <afxmt.h>			// �گ�ފ֘A
#include <afxcview.h>		// CListView, CTreeView
#include <afxdlgs.h>
#include <gdiplus.h>

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>		// MFC �� Internet Explorer 4 �R���� �R���g���[�� �T�|�[�g
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC �� Windows �R���� �R���g���[�� �T�|�[�g
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxcontrolbars.h>	// MFC �ɂ����郊�{���ƃR���g���[�� �o�[�̃T�|�[�g

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
#define	_SCL_SECURE_NO_WARNINGS			// ��Boost1.44�p
#include <string>
#include <vector>
#include <bitset>
#include <algorithm>

#pragma	warning( disable : 4800 )		// BOOL setting

// BOOST Libraries
#pragma	warning( disable : 4819 )		// codepage
#pragma	warning( disable : 4348 )		// boost 1.60�` ???
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

// VS2015 �s�
void MySplitPath(LPCTSTR lpszFullPath,
	LPTSTR pszDrive, size_t nDrive, LPTSTR pszPath, size_t nPath,
	LPTSTR pszName,  size_t nName,  LPTSTR pszExt,  size_t nExt);
#ifdef _tsplitpath_s
#undef _tsplitpath_s
#define _tsplitpath_s MySplitPath
#endif

#define	NCVCSERIALVERSION_1503	1503	// v1.00RC�`
#define	NCVCSERIALVERSION_1505	1505	// v1.10�`
#define	NCVCSERIALVERSION_1507	1507	// v1.10a�`
#define	NCVCSERIALVERSION_1600	1600	// v1.60�` (CDXFworkingOutline���ް��ύX)
#define	NCVCSERIALVERSION_1700	1700	// v1.70�` (CDXFworkingOutline�̵̾�Ēl�ǉ�)
#define	NCVCSERIALVERSION_2300	2300	// v2.30�` (���Ռ��_�ް�)
#define	NCVCSERIALVERSION_3600	3600	// v3.60�` (CAD�ް��̓���)
#define	NCVCSERIALVERSION_3602	3602	// v3.60b�` (����POLYLINE�̕��������ް�)
#define	NCVCSERIALVERSION_3620	3620	// v3.62�` (double->float)
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

// �ƭ�ّ���̳���޳ID
#define	ID_NC_STATUSBAR			30100
#define	ID_DXF_STATUSBAR		30101
#define	ID_MAIN_PROGRESS		30900

// ؿ����`�̂Ȃ�°��ް
#define	IDR_ADDINBAR			120
#define	IDR_EXECBAR				121

// ��޲݁C�O�����ع���ݗp�ƭ�ID
#define	EXECSTARTID				40000
#define	ADDINSTARTID			40100
#define	EXECADDIN_ENDID			40200

// UserMessage
	// �گ�ޏI�� ThreadDlg.cpp ��
#define	WM_USERFINISH			WM_USER+100
	// �ڰт�̧�ٕύX�ʒm
#define	WM_USERFILECHANGENOTIFY	WM_USER+101
	// CViewBase::OnInitialUpdate() �Ő��m�ȳ���޳���ނ𓾂邽�߂�ү����
#define	WM_USERINITIALUPDATE	WM_USER+105
	// �گ�ނ���Ҳݽð���ް����۸�ڽ���۰قւ��߼޼�ݾ��
#define	WM_USERPROGRESSPOS		WM_USER+106
	// Ӱ��ڽ�޲�۸ނւ��޷���Đؑ֒ʒm
#define	WM_USERSWITCHDOCUMENT	WM_USER+107
	// CTabViewBase ���߰�ސֲؑ����
#define	WM_USERACTIVATEPAGE		WM_USER+108
	// MDI�q����޳�̼��Ѻ����(�ő剻,���ɖ߂�)�ɑ΂���̨��ү���� ��
	// �e�ޭ��ւ�̨��ү����
#define	WM_USERVIEWFITMSG		WM_USER+109
	// �ڰ����s�گ�ނ���̒ʒm(CTraceThread -> CNCListView)
#define WM_USERTRACESELECT		WM_USER+110
	// G���ނ̽ð���ް�X�V(CNCListView -> CNCChild)
#define WM_USERSTATUSLINENO		WM_USER+111
	// CAD�ް��̓���(CNCVCApp::OnFileCADbind() -> CDXFView )
#define	WM_USERBINDINIT			WM_USER+112
	// CAD�ް��̓���(CDXFView[child] -> CDXFView[parent] )
#define	WM_USERBIND_LDOWN		WM_USER+113
#define	WM_USERBIND_ROUND		WM_USER+114
#define	WM_USERBIND_CANCEL		WM_USER+115

/////////////////////////////////////////////////////////////////////////////
// NCVC ���ʊ֐��E�}�N��

// ÷��̧�ق̍s����
inline	BOOL	NC_IsNullLine(const CString& str)	// EOF ���̍s
{
	return ( str.IsEmpty() || str[0]=='#' || str[0]=='\x1a' );
}
/*	-- boost::algorithm::trim()
//�O��̋󔒕������폜
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
// ����
int		GetRandom(int min, int max);

// ���߽�����߽����̧�ٖ��ɕ���
void	Path_Name_From_FullPath(LPCTSTR, CString&, CString&, BOOL = TRUE);

// ���΃p�X��Ԃ�
CString	RelativePath(LPCTSTR, LPCTSTR);

// ڼ޽�؂̊K�w������
inline	CString	GetSubTreeRegKey(int nID1, int nID2)
{
	CString	strID1, strID2;
	VERIFY(strID1.LoadString(nID1));
	VERIFY(strID2.LoadString(nID2));
	return (strID1 + "\\" + strID2);
}

// �޲�۸����ق�̧�ٖ���t��
inline	CString	AddDialogTitle2File(const CString& strBase, const CString& strFullPath)
{
	CString	strPath, strFile;
	::Path_Name_From_FullPath(strFullPath, strPath, strFile);
	return strBase + " --- " + strFile;
}

// XpStyle�̉���
// Xp�ȊO�ɂ��Ή����邽��LoadLibrary()+GetProcAddress()�Ŋ֐����ڽ�擾
inline void DisableXpStyle(HWND hWnd)
{
	typedef	HRESULT	(WINAPI *pFuncSetWindowTheme)(HWND, LPCWSTR, LPCWSTR);
	HINSTANCE			hLib;
	pFuncSetWindowTheme	pProc;
	if ( (hLib=::LoadLibrary(_T("UxTheme.dll"))) != NULL ) {
		if ( (pProc=(pFuncSetWindowTheme)::GetProcAddress(hLib, _T("SetWindowTheme"))) != NULL ) {
			// SetWindowTheme() ����̕�����ŌĂяo�����޼ޭ�ٽ��ى���
			(pProc)(hWnd, L"", L"");
		}
		::FreeLibrary(hLib);
	}
}

// ̧�ق��ް�ޮ�ؿ���̎擾
LPVOID	GetVersionResource(LPCTSTR lpszFileName, LPDWORD* dwTrans);
// �ް�ޮ�ؿ�������ް�ޮݏ��̺�߰���擾
//	Keyword:
//		"Comments"         // �R�����g
//		"CompanyName"      // ��Ж�
//		"FileDescription"  // ����
//		"FileVersion"      // �t�@�C���o�[�W����
//		"InternalName"     // ������
//		"LegalCopyright"   // ���쌠
//		"LegalTrademarks"  // ���W
//		"OriginalFilename" // �����t�@�C����
//		"PrivateBuild"     // �v���C�x�[�g�r���h���
//		"ProductName"      // ���i��
//		"ProductVersion"   // ���i�o�[�W����
//		"SpecialBuild"     // �X�y�V�����r���h���
BOOL	GetVersionValue(CString& strBuffer, LPVOID pVersionInfo, DWORD dwTrans, LPCTSTR strKeyWord);

#ifdef _DEBUG
// MagaDebug����AllocConsole()�g�p�Ƀ`�F���W
#include <stdio.h>
#include <iostream>
#include "DbgConsole.h"
// GetLastMessage() ��ү���ސ��`
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
