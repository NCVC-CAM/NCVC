/*
	±ƒﬁ≤›I/F
*/
#pragma once

#include "NCVCaddin.h"

//////////////////////////////////////////////////////////////////////
//
class CNCVCaddinIF
{
private:
	HMODULE				m_hAddin;	// ±ƒﬁ≤› ›ƒﬁŸ
	PFNNCVCADDINFUNC	m_pFunc[NCVCADIN_TYPESIZE];	// “∆≠∞≤Õﬁ›ƒä÷êî

	DWORD		m_dwType;				// ±ƒﬁ≤›¿≤Ãﬂ(Ç«ÇÃ“∆≠∞≤Õﬁ›ƒÇ…ëÆÇ∑ÇÈÇ©)
	CString		m_strMenuName[NCVCADIN_TYPESIZE],	// “∆≠∞Ç…ï\é¶Ç≥ÇÍÇÈï∂éöóÒ
				m_strFuncName[NCVCADIN_TYPESIZE],	// ¥›ƒÿä÷êîñº
				m_strFileName,			// DLLÃß≤Ÿñº
				m_strName,				// ±ƒﬁ≤›ñº
				m_strCopyright,			// ∫Àﬂ∞◊≤ƒ
				m_strSupport,			// “∞Ÿ±ƒﬁ⁄ΩÇ»Ç«
				m_strComment;			// ∫“›ƒëº

	int			m_nToolBar;				// ¬∞Ÿ ﬁ∞Ç…ìoò^Ç∑ÇÈ≤Õﬁ›ƒ
	WORD		m_nMenuID[NCVCADIN_TYPESIZE];	// “∆≠∞ID (from CNCVCApp)
	WORD		m_nImage;				// ≤“∞ºﬁÿΩƒáÇ (from CMainFrame)

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

typedef	CTypedPtrArrayEx<CPtrArray, CNCVCaddinIF*>	CNCVCaddinArray;

//////////////////////////////////////////////////////////////////////
// CNCVCApp::m_mpAddin ìoò^óp
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
