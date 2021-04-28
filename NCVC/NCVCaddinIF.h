/*
	ｱﾄﾞｲﾝI/F
*/
#pragma once

#include "NCVCaddin.h"

//////////////////////////////////////////////////////////////////////
//	Export関数で AfxGetNCVCApp() を使うと、DLL側の CWinApp* を見ることになる
//	theApp を使用すること

//////////////////////////////////////////////////////////////////////
//
class CNCVCaddinIF
{
private:
	HMODULE				m_hAddin;	// ｱﾄﾞｲﾝﾊﾝﾄﾞﾙ
	PFNNCVCADDINFUNC	m_pFunc[NCVCADIN_TYPESIZE];	// ﾒﾆｭｰｲﾍﾞﾝﾄ関数

	DWORD		m_dwType;				// ｱﾄﾞｲﾝﾀｲﾌﾟ(どのﾒﾆｭｰｲﾍﾞﾝﾄに属するか)
	CString		m_strMenuName[NCVCADIN_TYPESIZE],	// ﾒﾆｭｰに表示される文字列
				m_strFuncName[NCVCADIN_TYPESIZE],	// ｴﾝﾄﾘ関数名
				m_strFileName,			// DLLﾌｧｲﾙ名
				m_strName,				// ｱﾄﾞｲﾝ名
				m_strCopyright,			// ｺﾋﾟｰﾗｲﾄ
				m_strSupport,			// ﾒｰﾙｱﾄﾞﾚｽなど
				m_strComment;			// ｺﾒﾝﾄ他

	int			m_nToolBar;				// ﾂｰﾙﾊﾞｰに登録するｲﾍﾞﾝﾄ
	int			m_nMenuID[NCVCADIN_TYPESIZE];	// ﾒﾆｭｰID (from CNCVCApp)符号なしはアウト
	WORD		m_nImage;				// ｲﾒｰｼﾞﾘｽﾄ№ (from CMainFrame)

public:
	CNCVCaddinIF(HMODULE, LPNCVCINITIALIZE, LPCTSTR);
	virtual	~CNCVCaddinIF();

	HMODULE	GetAddinHandle(void) const {
		return m_hAddin;
	}
	PFNNCVCADDINFUNC	GetAddinFunc(size_t n) const {
		return m_pFunc[n];
	}
	DWORD	GetAddinType(void) const {
		return m_dwType;
	}
	CString		GetMenuName(size_t n) const {
		return m_strMenuName[n];
	}
	CString		GetFileName(void) const {
		return m_strFileName;
	}
	CString		GetDLLName(void) const {
		return m_strName;
	}
	CString		GetCopyright(void) const {
		return m_strCopyright;
	}
	CString		GetSupport(void) const {
		return m_strSupport;
	}
	CString		GetComment(void) const {
		return m_strComment;
	}
	void		SetMenuID(size_t n, WORD nID) {
		m_nMenuID[n] = nID;
	}
	WORD		GetImageNo(void) const {
		return m_nImage;
	}
	void		SetImageNo(WORD nImage) {
		m_nImage = nImage;
	}
	int			GetToolBarID(void) const;
};

typedef	CTypedPtrArray<CPtrArray, CNCVCaddinIF*>	CNCVCaddinArray;

//////////////////////////////////////////////////////////////////////
// CNCVCApp::m_mpAddin 登録用
class CNCVCaddinMap
{
	CNCVCaddinIF*		m_pAddin;
	PFNNCVCADDINFUNC	m_pFunc;
public:
	CNCVCaddinMap(CNCVCaddinIF* pAddin, PFNNCVCADDINFUNC pFunc) {
		m_pAddin = pAddin;
		m_pFunc  = pFunc;
	}
	CNCVCaddinIF*		GetAddinIF(void) const {
		return m_pAddin;
	}
	PFNNCVCADDINFUNC	GetAddinFunc(void) const {
		return m_pFunc;
	}
};

typedef	CTypedPtrMap<CMapWordToPtr, WORD, CNCVCaddinMap*>	CNCVCaddinWordMap;

BOOL	IsNCDocument(NCVCHANDLE pDoc);
BOOL	IsDXFDocument(NCVCHANDLE pDoc);
