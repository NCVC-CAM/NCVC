// NCVCaddinIF.cpp : CNCVCaddinIF クラスの動作の定義を行います。
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "NCVCdefine.h"
#include "NCVCaddin.h"
#include "NCVCaddinIF.h"
#include "NCDoc.h"
#include "DXFDoc.h"
#include "MCOption.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

extern	LPCTSTR	gg_szRegKey;
extern	const	DWORD	g_dwAddinType[] = {
	NCVCADIN_FLG_APPFILE, NCVCADIN_FLG_APPOPTION,
	NCVCADIN_FLG_NCDFILE, NCVCADIN_FLG_NCDEDIT, NCVCADIN_FLG_NCDVIEW, NCVCADIN_FLG_NCDOPTION,
	NCVCADIN_FLG_DXFFILE, NCVCADIN_FLG_DXFEDIT, NCVCADIN_FLG_DXFVIEW, NCVCADIN_FLG_DXFOPTION
};

/////////////////////////////////////////////////////////////////////////////
// CNCVCaddinIF クラスの構築/消滅

CNCVCaddinIF::CNCVCaddinIF
	(HMODULE hAddin, LPNCVCINITIALIZE lpnci, LPCTSTR lpstrFileName)
{
	int		i;
	ASSERT( NCVCADIN_TYPESIZE == SIZEOF(g_dwAddinType) );

	m_hAddin	= hAddin;
	m_dwType	= lpnci->dwType;
	m_nToolBar	= lpnci->nToolBar;
	for ( i=0; i<NCVCADIN_TYPESIZE; i++ ) {
		m_nMenuID[i] = -1;
		if ( m_dwType & g_dwAddinType[i] &&
				lpnci->lpszMenuName[i] && lstrlen(lpnci->lpszMenuName[i]) > 0 &&
				lpnci->lpszFuncName[i] && lstrlen(lpnci->lpszFuncName[i]) > 0 ) {
			m_strMenuName[i] = lpnci->lpszMenuName[i];
			m_strFuncName[i] = lpnci->lpszFuncName[i];
			m_pFunc[i] = (PFNNCVCADDINFUNC)::GetProcAddress(m_hAddin, m_strFuncName[i]);
		}
		else
			m_pFunc[i] = NULL;
	}
	m_strFileName	= lpstrFileName;
	if ( lpnci->lpszAddinName && lstrlen(lpnci->lpszAddinName) > 0 )
		m_strName = lpnci->lpszAddinName;
	if ( lpnci->lpszCopyright && lstrlen(lpnci->lpszCopyright) > 0 )
		m_strCopyright = lpnci->lpszCopyright;
	if ( lpnci->lpszSupport && lstrlen(lpnci->lpszSupport) > 0 )
		m_strSupport = lpnci->lpszSupport;
	if ( lpnci->lpszComment && lstrlen(lpnci->lpszComment) > 0 )
		m_strComment = lpnci->lpszComment;

	m_nImage = -1;
}

CNCVCaddinIF::~CNCVCaddinIF()
{
	if ( m_hAddin )
		::FreeLibrary(m_hAddin);
}

int CNCVCaddinIF::GetToolBarID(void) const
{
	// ﾂｰﾙﾊﾞｰへの登録情報が正しいなら，そのIDを返す
	if ( m_nToolBar>=0 && m_nToolBar<NCVCADIN_TYPESIZE && m_nMenuID[m_nToolBar]>0 )
		return m_nMenuID[m_nToolBar];
	else {
		// でなければ一番若いIDを返す
		for ( int i=0; i<NCVCADIN_TYPESIZE; i++ ) {
			if ( m_nMenuID[i] > 0 )	// 符号なしではアウト
				return m_nMenuID[i];
		}
	}
	return -1;
}

/////////////////////////////////////////////////////////////////////////////
// ｴｸｽﾎﾟｰﾄ関数のｻﾌﾞ

BOOL IsNCDocument(NCVCHANDLE hDoc)
{
	if ( !hDoc )
		return FALSE;
	return reinterpret_cast<CNCDoc *>(hDoc)->IsKindOf(RUNTIME_CLASS(CNCDoc));
}

BOOL IsDXFDocument(NCVCHANDLE hDoc)
{
	if ( !hDoc )
		return FALSE;
	return reinterpret_cast<CDXFDoc *>(hDoc)->IsKindOf(RUNTIME_CLASS(CDXFDoc));
}

/////////////////////////////////////////////////////////////////////////////
// ｴｸｽﾎﾟｰﾄ関数(全般)

NCEXPORT HWND WINAPI NCVC_GetMainWnd(void)
{
	return AfxGetMainWnd()->m_hWnd;
}

NCEXPORT HINSTANCE WINAPI NCVC_GetMainInstance(void)
{
	return AfxGetInstanceHandle();
}

NCEXPORT LONG WINAPI NCVC_CreateRegKey(LPCTSTR lpszAddinName, PHKEY phKey)
{
	CString	strKey(gg_szRegKey), strTmp("Addin\\");
	strKey += strTmp + lpszAddinName;	// "Software\MNCT-S\NCVC\Addin\ﾌﾟﾛｸﾞﾗﾑ名"

	DWORD	dwDisposition;

	return (RegCreateKeyEx(HKEY_CURRENT_USER, strKey, NULL, "",
				REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, phKey, &dwDisposition) );
}

NCEXPORT NCVCHANDLE WINAPI NCVC_GetDocument(LPCTSTR lpszPathName)
{
	// 引数NULLの場合は現在ｱｸﾃｨﾌﾞなﾄﾞｷｭﾒﾝﾄ
	if ( !lpszPathName ) {
		CMDIChildWnd*	pChild = AfxGetNCVCMainWnd()->MDIGetActive();
		return pChild ? pChild->GetActiveDocument() : NULL;
	}
	// 登録外拡張子の場合もあるので，２つのﾄﾞｷｭﾒﾝﾄﾃﾝﾌﾟﾚｰﾄを検索
	CDocument*	pDoc = AfxGetNCVCApp()->GetAlreadyDocument(TYPE_NCD, lpszPathName);
	if ( pDoc )
		return pDoc;
	return AfxGetNCVCApp()->GetAlreadyDocument(TYPE_DXF, lpszPathName);
}

NCEXPORT int WINAPI NCVC_GetDocumentFileName(NCVCHANDLE hDoc, LPTSTR lpszPathName, int nSize)
{
	int	nLength = -1;
	if ( IsNCDocument(hDoc) || IsDXFDocument(hDoc) ) {
		CString	strFullPath( reinterpret_cast<CDocument *>(hDoc)->GetPathName() );
		nLength = strFullPath.GetLength();
		if ( lpszPathName ) {
			if ( nLength+1 <= nSize )
				lstrcpy(lpszPathName, strFullPath);
			else
				nLength = -1;
		}
	}
	return nLength;
}

NCEXPORT void WINAPI NCVC_LockDocument(NCVCHANDLE hDoc, HANDLE hThread)
{
	if ( IsNCDocument(hDoc) )
		reinterpret_cast<CNCDoc *>(hDoc)->LockDocument(hThread);
	if ( IsDXFDocument(hDoc) )
		reinterpret_cast<CDXFDoc *>(hDoc)->LockDocument(hThread);
}

NCEXPORT void WINAPI NCVC_UnlockDocument(NCVCHANDLE hDoc)
{
	if ( IsNCDocument(hDoc) )
		reinterpret_cast<CNCDoc *>(hDoc)->UnlockDocument();
	if ( IsDXFDocument(hDoc) )
		reinterpret_cast<CDXFDoc *>(hDoc)->UnlockDocument();
}

NCEXPORT void WINAPI NCVC_MainfrmProgressRange(int nMin, int nMax)
{
	AfxGetNCVCMainWnd()->GetProgressCtrl()->SetRange32(nMin, nMax);
	AfxGetNCVCMainWnd()->GetProgressCtrl()->SetPos(0);
}

NCEXPORT void WINAPI NCVC_MainfrmProgressPos(int nPos)
{
	AfxGetNCVCMainWnd()->GetProgressCtrl()->SetPos(nPos);
}

NCEXPORT int WINAPI NCVC_GetSelectMachineFileName(LPTSTR lpszPathName, int nSize)
{
	int		nLength;
	CString	strFile( AfxGetNCVCApp()->GetMCOption()->GetMCHeadFileName() );

	if ( strFile.IsEmpty() )
		nLength = 0;
	else {
		nLength = strFile.GetLength();
		if ( lpszPathName ) {
			if ( nLength+1 <= nSize )
				lstrcpy(lpszPathName, strFile);
			else
				nLength = -1;
		}
	}

	return nLength;
}

NCEXPORT BOOL WINAPI NCVC_SetMachineFile(LPCTSTR lpszPathName)
{
	return AfxGetNCVCApp()->ChangeMachine(lpszPathName);
}

NCEXPORT NCVCHANDLE WINAPI NCVC_CreateNCDocument
	(LPCTSTR lpszPathName, LPCTSTR lpszSerialFunc)
{
	CNCVCApp*	pApp = AfxGetNCVCApp();
	pApp->SetSerializeFunc(lpszSerialFunc);
	CDocument* pDoc = pApp->GetDocTemplate(TYPE_NCD)->OpenDocumentFile(lpszPathName);
	return pDoc;
}

NCEXPORT NCVCHANDLE WINAPI NCVC_CreateDXFDocument
	(LPCTSTR lpszPathName, LPCTSTR lpszSerialFunc)
{
	CNCVCApp*	pApp = AfxGetNCVCApp();
	pApp->SetSerializeFunc(lpszSerialFunc);
	CDocument* pDoc = pApp->GetDocTemplate(TYPE_DXF)->OpenDocumentFile(lpszPathName);
	return pDoc;
}

NCEXPORT BOOL WINAPI NCVC_AddNCDExtensionFunc
	(LPCTSTR lpszExt, LPCTSTR lpszModuleName, LPCTSTR lpszSerialFunc)
{
	return AfxGetNCVCApp()->AddExtensionFunc(TYPE_NCD, lpszExt, lpszModuleName, lpszSerialFunc);
}

NCEXPORT BOOL WINAPI NCVC_AddDXFExtensionFunc
	(LPCTSTR lpszExt, LPCTSTR lpszModuleName, LPCTSTR lpszSerialFunc)
{
	return AfxGetNCVCApp()->AddExtensionFunc(TYPE_DXF, lpszExt, lpszModuleName, lpszSerialFunc);
}

NCEXPORT void WINAPI NCVC_CloseDocument(NCVCHANDLE hDoc)
{
	if ( IsNCDocument(hDoc) || IsDXFDocument(hDoc) )
		reinterpret_cast<CDocument *>(hDoc)->OnCloseDocument();
}

NCEXPORT void WINAPI NCVC_ReDraw(NCVCHANDLE hDoc)
{
	if ( IsNCDocument(hDoc) || IsDXFDocument(hDoc) )
		reinterpret_cast<CDocument *>(hDoc)->UpdateAllViews(NULL, UAV_ADDINREDRAW);
}

NCEXPORT void WINAPI NCVC_GetObjectSize(NCVCHANDLE hDoc, LPRECT3D lprc)
{
	CRect3F		rc;
	if ( IsNCDocument(hDoc) )
		rc = reinterpret_cast<CNCDoc *>(hDoc)->GetMaxRect();
	else if ( IsDXFDocument(hDoc) )
		rc = reinterpret_cast<CDXFDoc *>(hDoc)->GetMaxRect();
	lprc->left		= rc.left;
	lprc->top		= rc.top;
	lprc->right		= rc.right;
	lprc->bottom	= rc.bottom;
	lprc->high		= rc.high;
	lprc->low		= rc.low;
}
