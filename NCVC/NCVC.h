// NCVC.h : NCVC �A�v���P�[�V�����̃��C�� �w�b�_�[ �t�@�C��
//
#pragma once

#ifndef __AFXWIN_H__
	#error "PCH �ɑ΂��Ă��̃t�@�C�����C���N���[�h����O�� 'stdafx.h' ���C���N���[�h���Ă�������"
#endif

#include <afxwinappex.h>
#include "resource.h"       // ���C�� �V���{��

#ifdef _DEBUG
#define	_DEBUG_FILEOPEN		// ̧�ق��J���Ƃ��̒ǐ�
#endif

// ��޲ݒ�`(ncvcaddin.h)
#define	___NCVC___

#include "MCOption.h"
#include "DXFOption.h"
#include "ViewOption.h"
#include "ExecOption.h"
#include "NCVCaddinIF.h"

class	CNCVCApp;
class	CNCDoc;
class	CDXFDoc;

/////////////////////////////////////////////////////////////////////////////
// CNCVCDocTemplate: NCVC�g���޷��������ڰ�

enum	eEXTTYPE	{EXT_ADN=0, EXT_DLG=1};

class CNCVCDocTemplate : public CMultiDocTemplate  
{
	friend	class	CExtensionDlg;

	CMapStringToPtr		m_mpExt[2];	// �o�^�g���q�Ƽري֐���ϯ��

public:
	CNCVCDocTemplate(UINT nIDResource, CRuntimeClass* pDocClass,
		CRuntimeClass* pFrameClass, CRuntimeClass* pViewClass);

	// �g���q�̓o�^(��޲ݗp)
	BOOL	AddExtensionFunc(LPCTSTR, LPVOID);
	// �o�^�g���q�̕ۑ�(�޲�۸ޗp)
	BOOL	SaveExt(void);

	// �g���qϯ�ߎQ��
	const	CMapStringToPtr*	GetExtMap(eEXTTYPE n) {
		return &m_mpExt[n];
	}
	// ̨��������̐���
	CString	GetFilterString(void);
	// �o�^�g���q���ۂ�
	BOOL	IsExtension(LPCTSTR, LPVOID* = NULL);

	// ���i�Ȋg���q����
	virtual	Confidence	MatchDocType(LPCTSTR lpszPathName, CDocument*& rpDocMatch);
#ifdef _DEBUG_FILEOPEN
	CDocument* OpenDocumentFile(LPCTSTR lpszPathName, BOOL bMakeVisible = TRUE);
#endif

	DECLARE_DYNAMIC(CNCVCDocTemplate)
};

/////////////////////////////////////////////////////////////////////////////
// CRecentViewInfo: ̧�ق��Ƃ̕`����
//

class CRecentViewInfo
{
	friend	class	CNCVCApp;

	BOOL		m_bInfo;

	CString		m_strFile;
	struct VINFO {		// ڼ޽�؂����޲�؂œǂݏ�������P��
		GLdouble	objectXform[4][4];	// OpenGL��]��ظ�
		CRect3D		rcView;				// ���ً��
		CPointD		ptCenter;			// ���S���W
	} m;

public:
	CRecentViewInfo(LPCTSTR);

	void	SetViewInfo(const GLdouble[4][4], const CRect3D&, const CPointD&);
	BOOL	GetViewInfo(GLdouble[4][4], CRect3D&, CPointD&) const;
};
typedef	CTypedPtrList<CPtrList, CRecentViewInfo*>	CRecentViewList;

/////////////////////////////////////////////////////////////////////////////
// CNCVCApp:
//

class CNCVCApp : public CWinAppEx
{
	// NCViewTab�Ǘ����
	UINT	m_nTraceSpeed;		// �ڰ����s�̑��x
	int		m_nNCTabPage;		// ��è���߰�ޏ��

	// ���тŗB��̵�߼��
	CMCOption*		m_pOptMC;		// MC��߼��
	CDXFOption*		m_pOptDXF;		// DXF��߼��
	CViewOption*	m_pOptView;		// View��߼��
	//
	CRecentViewList		m_liRecentView;	// �`����i�[�z��
	CRecentViewInfo*	m_pDefViewInfo;	// ��̫�ĕ`����

	// �޷��������ڰ�
	CNCVCDocTemplate*	m_pDocTemplate[2];	// NC,DXF�޷����

	// ��޲ݏ��
	CNCVCaddinIF*	m_pActiveAddin;	// ���ݱ�è�ނȱ�޲�
	WORD			m_wAddin;		// ��޲ݺ����ID
	CNCVCaddinArray	m_obAddin;
	CNCVCaddinWordMap	m_mpAddin;	// �ƭ�ID�𷰂Ƃ�����؊֐�ϯ��
	BOOL	NCVCAddinInit(int);		// ��޲ݏ��ǂݍ���
	BOOL	NCVCAddinMenu(void);	// ��޲݂��ƭ��o�^
	PFNNCVCSERIALIZEFUNC	m_pfnSerialFunc;	// ��޲ݼري֐��̈ꎞ�ێ�

	// �O�����ع���ݏ��
	WORD		m_wExec;	// �O�����ع���ݺ����ID
	CExecList	m_liExec;	// �O�����ع����ؽ�(����) CTypedPtrList<CPtrList, CExecOption*>
	CTypedPtrMap<CMapWordToPtr,	WORD, CExecOption*>	m_mpExec;	// �@�V�@(CommandID�Ή�)
	void	SaveExecData(void);
	BOOL	CreateExecMap(void);

	// ڼ޽�ؑ���
	BOOL	NCVCRegInit(void);		// ڼ޽�؏��ǂݍ���
	BOOL	NCVCRegInit_OldExec(CString&);
	BOOL	NCVCRegInit_NewExec(CString&, int);
	void	NCVCRegOld(void);		// ��ڼ޽���ް��̍폜
	//
	void	InitialRecentViewList(void);
	void	WriteRecentViewList(void);
	void	AddToRecentViewList(LPCTSTR);

	// NCVC����
	BOOL	DoPromptFileNameEx(CString&);	// ����̧�ٵ����

public:
	CNCVCApp();
	virtual ~CNCVCApp();
	UINT	GetTraceSpeed(void) {
		return m_nTraceSpeed;
	}
	void	SetTraceSpeed(UINT nSpeed) {
		m_nTraceSpeed = nSpeed;
	}
	int		GetNCTabPage(void) {
		return m_nNCTabPage;
	}
	void	SetNCTabPage(int nPage) {
		m_nNCTabPage = nPage;
	}

	CMCOption*		GetMCOption(void) {
		return m_pOptMC;
	}
	CDXFOption*		GetDXFOption(void) {
		return m_pOptDXF;
	}
	CViewOption*	GetViewOption(void) {
		return m_pOptView;
	}
	const	CNCVCaddinArray*	GetAddinArray(void) {
		return &m_obAddin;
	}
	const	CNCVCaddinWordMap*	GetAddinMap(void) {
		return &m_mpAddin;
	}
	WORD	GetMaxAddinID(void) {
		return m_wAddin;
	}
	WORD	GetMaxExecID(void) {
		return m_wExec;
	}
	CExecList*	GetExecList(void) {
		return &m_liExec;
	}
	void	CallAddinFunc(WORD);
	const	CNCVCaddinIF*	GetLookupAddinID(WORD);
	void	SetSerializeFunc(LPCTSTR pszSerialFunc) {
		m_pfnSerialFunc = pszSerialFunc && m_pActiveAddin ? 
			(PFNNCVCSERIALIZEFUNC)::GetProcAddress(
				m_pActiveAddin->GetAddinHandle(), pszSerialFunc) : NULL;
	}
	void	SetSerializeFunc(PFNNCVCSERIALIZEFUNC pfnSerialFunc) {
		m_pfnSerialFunc = pfnSerialFunc;
	}
	PFNNCVCSERIALIZEFUNC	GetSerializeFunc(void) {
		return m_pfnSerialFunc;
	}
	BOOL	AddExtensionFunc(DOCTYPE, LPCTSTR, LPCTSTR, LPCTSTR);
	const	CExecOption*	GetLookupExecID(WORD);

	CString	GetDocExtString(DOCTYPE enType) {	// ��̫���޷��������[.ncd|.cam]��Ԃ�
		CString	strFilter;
		m_pDocTemplate[enType]->GetDocString(strFilter, CDocTemplate::filterExt);
		return strFilter;
	}
	CString	GetFilterString(DOCTYPE enType) {	// �o�^�g���q�ꗗ��̨���`���ŕԂ�
		CString	strFilter, strResult(m_pDocTemplate[enType]->GetFilterString());
		strFilter.Format(enType == TYPE_NCD ? IDS_NCD_FILTER : IDS_CAM_FILTER,
			strResult, strResult);
		return strFilter;
	}
	CNCVCDocTemplate* GetDocTemplate(DOCTYPE enType) {
		return m_pDocTemplate[enType];
	}
	CString		GetRecentFileName(void) {
		return m_pRecentFileList->operator[](0);
	}
	CRecentViewInfo*	GetRecentViewInfo(void) {
		ASSERT( !m_liRecentView.IsEmpty() );
		return m_liRecentView.GetHead();
	}
	CRecentViewInfo*	GetDefaultViewInfo(void) {
		return m_pDefViewInfo;
	}
	void		SetDefaultViewInfo(const GLdouble[4][4]);
	CNCDoc*		GetAlreadyNCDocument(LPCTSTR = NULL);
	CDXFDoc*	GetAlreadyDXFDocument(LPCTSTR = NULL);
	int			GetDXFOpenDocumentCount(void);
	void		ReloadDXFDocument(void);
	void		ChangeViewOption(void);
	BOOL		ChangeMachine(int);
	BOOL		ChangeMachine(LPCTSTR);

	void		SaveWindowState(const CString&, const WINDOWPLACEMENT&);
	BOOL		GetWindowState(const CString&, WINDOWPLACEMENT*);
	void		SaveDlgWindow(int, const CWnd*);
	BOOL		GetDlgWindow(int, CPoint*);

// �I�[�o�[���C�h
public:
	virtual BOOL InitInstance();
	virtual int	 ExitInstance();
	virtual void AddToRecentFileList(LPCTSTR);
#ifdef _DEBUG_FILEOPEN
	virtual CDocument* OpenDocumentFile(LPCTSTR lpszFileName);
#endif

// ����
	afx_msg void OnAppAbout();
	afx_msg void OnFileOpen();
	afx_msg void OnFileThumbnail();
	afx_msg void OnFileCloseAndOpen();
	afx_msg void OnHelp();
	afx_msg void OnHelpAddin();
	afx_msg void OnOptionMC();
	afx_msg void OnOptionEditMC();
	afx_msg void OnOptionDXF();
	afx_msg void OnOptionMakeNC();
	afx_msg void OnOptionEditNC();
	afx_msg void OnOptionExec();
	afx_msg void OnOptionExt();
	afx_msg void OnViewSetup();
	afx_msg void OnViewSetupInport();
	afx_msg void OnViewSetupExport();
	afx_msg void OnWindowAllClose();
	afx_msg void OnUpdateOptionEdit(CCmdUI* pCmdUI);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// ��ۼު�čL��֐�(ؿ�����܂�)

// ��è�ٴװ��ү�����ޯ��
void	NCVC_CriticalErrorMsg(LPCTSTR, int);
// ̧�ق̑�������
BOOL	IsFileExist(LPCTSTR lpszFile, BOOL bExist = TRUE, BOOL bMsg = TRUE);

// CFileDialog�̌Ăяo��
int		NCVC_FileDlgCommon(int nTitle, const CString& strFilter, BOOL bAll,
			CString& strFileName, LPCTSTR lpszInitialDir = NULL,
			BOOL bRead = TRUE,
			DWORD dwFlags = OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_PATHMUSTEXIST);
inline
int		NCVC_FileDlgCommon(int nTitle, UINT nIDfilter, BOOL bAll,
			CString& strFileName, LPCTSTR lpszInitialDir = NULL,
			BOOL bRead = TRUE,
			DWORD dwFlags = OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_PATHMUSTEXIST)
{
	CString	strFilter;
	VERIFY(strFilter.LoadString(nIDfilter));
	return NCVC_FileDlgCommon(nTitle, strFilter, bAll, strFileName, lpszInitialDir, bRead, dwFlags);
}

/////////////////////////////////////////////////////////////////////////////

extern CNCVCApp theApp;