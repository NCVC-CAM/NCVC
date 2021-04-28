/*
	��޲�I/F
*/
#pragma once

#include "NCVCaddin.h"

//////////////////////////////////////////////////////////////////////
//	Export�֐��� AfxGetNCVCApp() ���g���ƁADLL���� CWinApp* �����邱�ƂɂȂ�
//	theApp ���g�p���邱��

//////////////////////////////////////////////////////////////////////
//
class CNCVCaddinIF
{
private:
	HMODULE				m_hAddin;	// ��޲������
	PFNNCVCADDINFUNC	m_pFunc[NCVCADIN_TYPESIZE];	// �ƭ�����Ċ֐�

	DWORD		m_dwType;				// ��޲�����(�ǂ��ƭ�����Ăɑ����邩)
	CString		m_strMenuName[NCVCADIN_TYPESIZE],	// �ƭ��ɕ\������镶����
				m_strFuncName[NCVCADIN_TYPESIZE],	// ���؊֐���
				m_strFileName,			// DLĻ�ٖ�
				m_strName,				// ��޲ݖ�
				m_strCopyright,			// ��߰ײ�
				m_strSupport,			// Ұٱ��ڽ�Ȃ�
				m_strComment;			// ���đ�

	int			m_nToolBar;				// °��ް�ɓo�^��������
	int			m_nMenuID[NCVCADIN_TYPESIZE];	// �ƭ�ID (from CNCVCApp)�����Ȃ��̓A�E�g
	WORD		m_nImage;				// �Ұ��ؽć� (from CMainFrame)

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
// CNCVCApp::m_mpAddin �o�^�p
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
