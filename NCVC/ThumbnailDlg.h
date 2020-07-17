// ThumbnailDlg.h : ヘッダー ファイル
//

#pragma once

#include <afxshelltreeCtrl.h>

class CNCDoc;
struct THUMBNAILINFO {
	CFileStatus	fStatus;
	CNCDoc*		pDoc;
	CView*		pView;	// CNCView, CNCViewXY, CNCViewXZ, CNCViewYZ
};
#define	LPTHUMBNAILINFO	THUMBNAILINFO *

class CThumbnailStatic : public CStatic
{
protected:
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg LRESULT OnUserDblClk(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CThumbnailDlg ダイアログ

class CThumbnailDlg : public CDialog
{
	BOOL		m_bInitialize,
				m_bEnumDoc;
	CWinThread*	m_pEnumDocThread;
	CMFCShellTreeCtrl	m_ctFolderTree;		// VC++2008SP1～
	CStatic				m_ctParentView;
	CScrollBar			m_ctScroll;

	CStringArray		m_aExt;
	CThumbnailStatic	m_ctChild[9];		// ﾋﾞｭｰを納めるｺﾝﾄﾛｰﾙ
	CSortArray<CPtrArray, LPTHUMBNAILINFO>	m_aInfo;

	void	ResizeControl(int, int);
	void	ChangeFolder(const CString&);
	void	SetAllFileFromFolder(const CString&, const CString&);
	void	SortThumbnailInfo(void);
	void	SetThumbnailDocument(void);
	CView*	CreatePlaneView(void);

	static	UINT	CreateEnumDoc_Thread(LPVOID);
	void	WaitEnumDocThread(BOOL = TRUE);	// ｽﾚｯﾄﾞの終了

public:
	CThumbnailDlg(int, ENNCVPLANE, CWnd* pParent = NULL);	// 標準コンストラクタ
	virtual ~CThumbnailDlg();

// ダイアログ データ
	enum { IDD = IDD_THUMBNAIL };
	int			m_nSort;
	int			m_nPlane;
	ENNCVPLANE	m_enPlane;
	CString		m_strFile;	// ﾀﾞﾌﾞﾙｸﾘｯｸされたﾌｧｲﾙ名

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSelchanged(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnDestroy();
	afx_msg void OnSelchangeSort();
	afx_msg void OnSelchangePlane();
	afx_msg LRESULT OnUserDblClk(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};
