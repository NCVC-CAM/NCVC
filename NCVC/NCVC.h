// NCVC.h : NCVC アプリケーションのメイン ヘッダー ファイル
//
#pragma once

#ifndef __AFXWIN_H__
	#error "PCH に対してこのファイルをインクルードする前に 'stdafx.h' をインクルードしてください"
#endif

#include <afxwinappex.h>
#include "resource.h"       // メイン シンボル

#ifdef _DEBUG
#define	_DEBUG_FILEOPEN		// ﾌｧｲﾙを開くときの追跡
#endif

// ｱﾄﾞｲﾝ定義(ncvcaddin.h)
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
// CNCVCDocTemplate: NCVC拡張ﾄﾞｷｭﾒﾝﾄﾃﾝﾌﾟﾚｰﾄ

enum	eEXTTYPE	{EXT_ADN=0, EXT_DLG=1};

class CNCVCDocTemplate : public CMultiDocTemplate  
{
	friend	class	CExtensionDlg;

	CMapStringToPtr		m_mpExt[2];	// 登録拡張子とｼﾘｱﾙ関数のﾏｯﾌﾟ

public:
	CNCVCDocTemplate(UINT nIDResource, CRuntimeClass* pDocClass,
		CRuntimeClass* pFrameClass, CRuntimeClass* pViewClass);

	// 拡張子の登録(ｱﾄﾞｲﾝ用)
	BOOL	AddExtensionFunc(LPCTSTR, LPVOID);
	// 登録拡張子の保存(ﾀﾞｲｱﾛｸﾞ用)
	BOOL	SaveExt(void);

	// 拡張子ﾏｯﾌﾟ参照
	const	CMapStringToPtr*	GetExtMap(eEXTTYPE n) {
		return &m_mpExt[n];
	}
	// ﾌｨﾙﾀ文字列の生成
	CString	GetFilterString(void);
	// 登録拡張子か否か
	BOOL	IsExtension(LPCTSTR, LPVOID* = NULL);

	// 厳格な拡張子検査
	virtual	Confidence	MatchDocType(LPCTSTR lpszPathName, CDocument*& rpDocMatch);
#ifdef _DEBUG_FILEOPEN
	CDocument* OpenDocumentFile(LPCTSTR lpszPathName, BOOL bMakeVisible = TRUE);
#endif

	DECLARE_DYNAMIC(CNCVCDocTemplate)
};

/////////////////////////////////////////////////////////////////////////////
// CRecentViewInfo: ﾌｧｲﾙごとの描画情報
//

class CRecentViewInfo
{
	friend	class	CNCVCApp;

	BOOL		m_bInfo;

	CString		m_strFile;
	struct VINFO {		// ﾚｼﾞｽﾄﾘからﾊﾞｲﾅﾘで読み書きする単位
		GLdouble	objectXform[4][4];	// OpenGL回転ﾏﾄﾘｸｽ
		CRect3D		rcView;				// ﾓﾃﾞﾙ空間
		CPointD		ptCenter;			// 中心座標
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
	// NCViewTab管理情報
	UINT	m_nTraceSpeed;		// ﾄﾚｰｽ実行の速度
	int		m_nNCTabPage;		// ｱｸﾃｨﾌﾞﾍﾟｰｼﾞ情報

	// ｼｽﾃﾑで唯一のｵﾌﾟｼｮﾝ
	CMCOption*		m_pOptMC;		// MCｵﾌﾟｼｮﾝ
	CDXFOption*		m_pOptDXF;		// DXFｵﾌﾟｼｮﾝ
	CViewOption*	m_pOptView;		// Viewｵﾌﾟｼｮﾝ
	//
	CRecentViewList		m_liRecentView;	// 描画情報格納配列
	CRecentViewInfo*	m_pDefViewInfo;	// ﾃﾞﾌｫﾙﾄ描画情報

	// ﾄﾞｷｭﾒﾝﾄﾃﾝﾌﾟﾚｰﾄ
	CNCVCDocTemplate*	m_pDocTemplate[2];	// NC,DXFﾄﾞｷｭﾒﾝﾄ

	// ｱﾄﾞｲﾝ情報
	CNCVCaddinIF*	m_pActiveAddin;	// 現在ｱｸﾃｨﾌﾞなｱﾄﾞｲﾝ
	WORD			m_wAddin;		// ｱﾄﾞｲﾝｺﾏﾝﾄﾞID
	CNCVCaddinArray	m_obAddin;
	CNCVCaddinWordMap	m_mpAddin;	// ﾒﾆｭｰIDをｷｰとするｴﾝﾄﾘ関数ﾏｯﾌﾟ
	BOOL	NCVCAddinInit(int);		// ｱﾄﾞｲﾝ情報読み込み
	BOOL	NCVCAddinMenu(void);	// ｱﾄﾞｲﾝのﾒﾆｭｰ登録
	PFNNCVCSERIALIZEFUNC	m_pfnSerialFunc;	// ｱﾄﾞｲﾝｼﾘｱﾙ関数の一時保持

	// 外部ｱﾌﾟﾘｹｰｼｮﾝ情報
	WORD		m_wExec;	// 外部ｱﾌﾟﾘｹｰｼｮﾝｺﾏﾝﾄﾞID
	CExecList	m_liExec;	// 外部ｱﾌﾟﾘｹｰｼｮﾝﾘｽﾄ(順序) CTypedPtrList<CPtrList, CExecOption*>
	CTypedPtrMap<CMapWordToPtr,	WORD, CExecOption*>	m_mpExec;	// 　〃　(CommandID対応)
	void	SaveExecData(void);
	BOOL	CreateExecMap(void);

	// ﾚｼﾞｽﾄﾘ操作
	BOOL	NCVCRegInit(void);		// ﾚｼﾞｽﾄﾘ情報読み込み
	BOOL	NCVCRegInit_OldExec(CString&);
	BOOL	NCVCRegInit_NewExec(CString&, int);
	void	NCVCRegOld(void);		// 旧ﾚｼﾞｽﾄﾘﾃﾞｰﾀの削除
	//
	void	InitialRecentViewList(void);
	void	WriteRecentViewList(void);
	void	AddToRecentViewList(LPCTSTR);

	// NCVCｶｽﾀﾑ
	BOOL	DoPromptFileNameEx(CString&);	// ｶｽﾀﾑﾌｧｲﾙｵｰﾌﾟﾝ

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

	CString	GetDocExtString(DOCTYPE enType) {	// ﾃﾞﾌｫﾙﾄﾄﾞｷｭﾒﾝﾄﾀｲﾌﾟ[.ncd|.cam]を返す
		CString	strFilter;
		m_pDocTemplate[enType]->GetDocString(strFilter, CDocTemplate::filterExt);
		return strFilter;
	}
	CString	GetFilterString(DOCTYPE enType) {	// 登録拡張子一覧をﾌｨﾙﾀ形式で返す
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

// オーバーライド
public:
	virtual BOOL InitInstance();
	virtual int	 ExitInstance();
	virtual void AddToRecentFileList(LPCTSTR);
#ifdef _DEBUG_FILEOPEN
	virtual CDocument* OpenDocumentFile(LPCTSTR lpszFileName);
#endif

// 実装
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
// ﾌﾟﾛｼﾞｪｸﾄ広域関数(ﾘｿｰｽ情報含む)

// ｸﾘﾃｨｶﾙｴﾗｰのﾒｯｾｰｼﾞﾎﾞｯｸｽ
void	NCVC_CriticalErrorMsg(LPCTSTR, int);
// ﾌｧｲﾙの存在ﾁｪｯｸ
BOOL	IsFileExist(LPCTSTR lpszFile, BOOL bExist = TRUE, BOOL bMsg = TRUE);

// CFileDialogの呼び出し
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