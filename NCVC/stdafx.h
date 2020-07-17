// stdafx.h : �W���̃V�X�e�� �C���N���[�h �t�@�C���̃C���N���[�h �t�@�C���A�܂���
// �Q�Ɖ񐔂������A�����܂�ύX����Ȃ��A�v���W�F�N�g��p�̃C���N���[�h �t�@�C��
// ���L�q���܂��B

#pragma once

#define	_BIND_TO_CURRENT_VCLIBS_VERSION	1

#ifndef _SECURE_ATL
#define _SECURE_ATL 1
#endif
/*
#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// Windows �w�b�_�[����g�p����Ă��Ȃ����������O���܂��B
#endif
*/
#include "targetver.h"
/*
// �ꕔ�� CString �R���X�g���N�^�͖����I�ł��B
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS
// ��ʓI�Ŗ������Ă����S�� MFC �̌x�����b�Z�[�W�̈ꕔ�̔�\�����������܂��B
#define _AFX_ALL_WARNINGS
*/
#include <afxwin.h>         // MFC �̃R�A����ѕW���R���|�[�l���g
#include <afxext.h>         // MFC �̊g������
#include <afxdisp.h>        // MFC �I�[�g���[�V���� �N���X
#include <afxtempl.h>		// MFC TemplateClass
#include <atlbase.h>		// CRegKey
#include <afxadv.h>			// NCVC.cpp �ɂ� CRecentFileList ������
#include <afxmt.h>			// �گ�ފ֘A
#include <afxcview.h>		// CListView, CTreeView
#include <afxdlgs.h>

#define	GLEW_STATIC			// GLEW static link
#include <gl/glew.h>		// OpenGL Extention
#include <gl/wglew.h>
#include <gl/gl.h>			// OpenGL
#include <gl/glu.h>
//#include <gl/glaux.h>		// VC++2008 �ŕs�v�H
#ifdef _DEBUG
#include <gl/glut.h>		// Use glut32.dll
#endif

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>		// MFC �� Internet Explorer 4 �R���� �R���g���[�� �T�|�[�g
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC �� Windows �R���� �R���g���[�� �T�|�[�g
#endif // _AFX_NO_AFXCMN_SUPPORT

// STL
#include <string>
#include <vector>
#include <algorithm>

// BOOST Libraries
#pragma	warning( disable : 4819 )
#define	BOOST_SPIRIT_THREADSAFE
#include "boost/regex.hpp"				// ���K�\��
#include "boost/tokenizer.hpp"			// �����񕪊�
#include "boost/tuple/tuple.hpp"		// �g���ް�
#include "boost/optional.hpp"
#include "boost/variant.hpp"
#include "boost/spirit.hpp"				// �\�����
#include "boost/utility.hpp"			// հè�è
#include "boost/algorithm/minmax.hpp"

#define	NCVCSERIALVERSION_1503	1503	// v1.00RC�`
#define	NCVCSERIALVERSION_1505	1505	// v1.10�`
#define	NCVCSERIALVERSION_1507	1507	// v1.10a�`
#define	NCVCSERIALVERSION_1600	1600	// v1.60�` (CDXFworkingOutline���ް��ύX)
#define	NCVCSERIALVERSION		1700	// v1.70�` (CDXFworkingOutline�̵̾�Ēl�ǉ�)
#define	SIZEOF(array)			( sizeof(array)/sizeof(array[0]) )

// NCVC original
#include "3Dto2D.h"
#include "CustomClass.h"
#include "CustomControl.h"

enum	DOCTYPE		{TYPE_NCD = 0, TYPE_DXF = 1};

// Common Define
#define	AfxGetNCVCApp()			( static_cast<CNCVCApp *>(AfxGetApp()) )
#define	AfxGetNCVCMainWnd()		( static_cast<CMainFrame *>(AfxGetMainWnd()) )
#define	LOMETRICFACTOR			10.0
// �~��64(2��/64��5.6�x)�����ŕ`�� from NCdata.cpp, DXFdata.cpp
#define	ARCCOUNT				64
#define	ARCSTEP					(PI/32)

// Timer Event
#define	IDC_SPLASH_TIMER		100
#define	IDC_STATUSBAR_EVENT		101

// �ƭ�ّ���̳���޳ID
#define	ID_NC_STATUSBAR			30100
#define	ID_DXF_STATUSBAR		30101
#define	ID_MAIN_PROGRESS		30900

// ؿ����`�̂Ȃ�°��ް
#define	IDR_ADDINBAR			120
#define	IDR_EXECBAR				121

// ��޲݁C�O�����ع���ݗp�ƭ�ID
#define	EXECSTARTID		40000
#define	ADDINSTARTID	40100
#define	EXECADDIN_ENDID	40200

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
	// CTabView ���߰�ސֲؑ����
#define	WM_USERACTIVATEPAGE		WM_USER+108
	// MDI�q����޳�̼��Ѻ����(�ő剻,���ɖ߂�)�ɑ΂���̨��ү���� ��
	// �e�ޭ��ւ�̨��ү����
#define	WM_USERVIEWFITMSG		WM_USER+109
	// �ڰ����s�گ�ނ���̒ʒm(CTraceThread->CNCListView)
#define WM_USERTRACESELECT		WM_USER+110
	// G���ނ̽ð���ް�X�V(CNCListView->CNCChild)
#define WM_USERSTATUSLINENO		WM_USER+111

/////////////////////////////////////////////////////////////////////////////
// NCVC ���ʊ֐�

// ÷��̧�ق̍s����
inline	BOOL	NC_IsNullLine(const CString& str)	// EOF ���̍s
{
	return ( str.IsEmpty() || str[0]=='#' || str[0]=='\x1a' ) ? TRUE : FALSE;
}

// ���߽�����߽����̧�ٖ��ɕ���
void	Path_Name_From_FullPath(LPCTSTR, CString&, CString&, BOOL = TRUE);

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
			(pProc)(hWnd, (L" "), (L" "));
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
// GetLastMessage() ��ү���ސ��`
void	NC_FormatMessage(void);
#endif

#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif
