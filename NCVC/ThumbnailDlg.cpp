// ThumbnailDlg.cpp : インプリメンテーション ファイル
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "NCDoc.h"
#include "NCView.h"
#include "NCViewXY.h"
#include "NCViewXZ.h"
#include "NCViewYZ.h"
#include "ThumbnailDlg.h"
#include <afxshellmanager.h>

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CThumbnailDlg, CDialog)
	ON_WM_SIZE()
	ON_WM_VSCROLL()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEWHEEL()
	ON_WM_DESTROY()
	ON_NOTIFY(TVN_SELCHANGED, IDC_THUMBNAIL_TREE, &CThumbnailDlg::OnSelchanged)
	ON_CBN_SELCHANGE(IDC_THUMBNAIL_SORTNAME, &CThumbnailDlg::OnSelchangeSort)
	ON_CBN_SELCHANGE(IDC_THUMBNAIL_PLANE, &CThumbnailDlg::OnSelchangePlane)
	ON_MESSAGE(WM_USERFINISH, &CThumbnailDlg::OnUserDblClk)
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(CThumbnailStatic, CStatic)
	ON_WM_LBUTTONDOWN()
	ON_MESSAGE(WM_USERFINISH, &CThumbnailStatic::OnUserDblClk)
END_MESSAGE_MAP()

// 並べ替え補助関数
typedef	int	(*PFNSORTFUNC)(LPTHUMBNAILINFO, LPTHUMBNAILINFO);
static int	FileNameCompareFunc1(LPTHUMBNAILINFO, LPTHUMBNAILINFO);		// ﾌｧｲﾙ名昇順ｿｰﾄ
static int	FileNameCompareFunc2(LPTHUMBNAILINFO, LPTHUMBNAILINFO);		// ﾌｧｲﾙ名降順ｿｰﾄ
static int	UpdateTimeCompareFunc1(LPTHUMBNAILINFO, LPTHUMBNAILINFO);	// 更新日付昇順ｿｰﾄ
static int	UpdateTimeCompareFunc2(LPTHUMBNAILINFO, LPTHUMBNAILINFO);	// 更新日付降順ｿｰﾄ
static int	FileSizeCompareFunc1(LPTHUMBNAILINFO, LPTHUMBNAILINFO);		// ﾌｧｲﾙｻｲｽﾞ昇順ｿｰﾄ
static int	FileSizeCompareFunc2(LPTHUMBNAILINFO, LPTHUMBNAILINFO);		// ﾌｧｲﾙｻｲｽﾞ降順ｿｰﾄ

/////////////////////////////////////////////////////////////////////////////
// CThumbnailDlg ダイアログ

CThumbnailDlg::CThumbnailDlg(int nSort, ENNCVPLANE nPlane, CWnd* pParent /*=NULL*/)
	: CDialog(CThumbnailDlg::IDD, pParent)
{
	m_bInitialize = m_bEnumDoc = FALSE;
	m_pEnumDocThread = NULL;
	m_nSort  = nSort;
	m_nPlane = nPlane;
	m_enPlane = nPlane;
}

CThumbnailDlg::~CThumbnailDlg()
{
	// ﾋﾞｭｰは親が破棄されると同時に DestroyWindow() される
	for ( int i=0; i<m_thumbInfo.GetSize(); i++ ) {
		delete	m_thumbInfo[i]->pDoc;
		delete	m_thumbInfo[i];
	}
	m_thumbInfo.RemoveAll();
}

void CThumbnailDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_THUMBNAIL_TREE, m_ctFolderTree);
	DDX_Control(pDX, IDC_THUMBNAIL_VIEWPARENT, m_ctParentView);
	DDX_Control(pDX, IDC_THUMBNAIL_SCROLL, m_ctScroll);
	DDX_CBIndex(pDX, IDC_THUMBNAIL_SORTNAME, m_nSort);
	DDX_CBIndex(pDX, IDC_THUMBNAIL_PLANE, m_nPlane);
}

void CThumbnailDlg::ResizeControl(int cx, int cy)
{
	int		i, j, nWidth, nHeight;
	CRect	rcBase, rc;

	// 各ｺﾝﾄﾛｰﾙの再配置
	m_ctFolderTree.GetWindowRect(&rcBase);	// 基準
	ScreenToClient(&rcBase);
	rcBase.bottom = cy - rcBase.left;
	m_ctFolderTree.MoveWindow(&rcBase);

	m_ctScroll.GetWindowRect(&rc);
	ScreenToClient(&rc);
	nWidth = rc.Width();
	rc.left   = cx - rcBase.left - nWidth;
	rc.right  = rc.left + nWidth;
	rc.bottom = cy - rcBase.left;
	m_ctScroll.MoveWindow(&rc);

	m_ctParentView.MoveWindow(rcBase.left + rcBase.Width() + 1, rcBase.top,
		cx - rcBase.left*2 - rcBase.Width() - nWidth - 2, rcBase.Height() );

	// ｻﾑﾈｲﾙ表示ｴﾘｱ
	m_ctParentView.GetClientRect(&rcBase);
	nWidth  = (rcBase.Width()  - 1) / 3;
	nHeight = (rcBase.Height() - 1) / 3;
	for ( i=0; i<3; i++ ) {			// ３行
		for ( j=0; j<3; j++ ) {		// ３列
			m_ctChild[i*3+j].MoveWindow(nWidth*j+1, nHeight*i+1, nWidth, nHeight);
		}
	}
	// ﾋﾞｭｰの調整
	for ( i=m_ctScroll.GetScrollPos(), j=0; i<m_thumbInfo.GetSize() && j<SIZEOF(m_ctChild); i++, j++ ) {
		if ( m_thumbInfo[i]->pView ) {
			m_ctChild[j].GetClientRect(&rc);
			m_thumbInfo[i]->pView->MoveWindow(0, 0, rc.Width(), rc.Height());
			m_thumbInfo[i]->pView->PostMessage(WM_USERVIEWFITMSG, 0, 1);
		}
	}
}

void CThumbnailDlg::ChangeFolder(const CString& strPath)
{
	int		i;
	CWaitCursor		wait;
	// 前回のｽﾚｯﾄﾞ待ち
	WaitEnumDocThread();

	// ﾊﾟｽ表示の最適化(shlwapi.h)
	::PathSetDlgItemPath(m_hWnd, IDC_THUMBNAIL_PATH, strPath);

	// ｺﾝﾄﾛｰﾙのﾘｾｯﾄ
	for ( i=0; i<m_thumbInfo.GetSize(); i++ ) {
		if ( m_thumbInfo[i]->pView ) {
			m_thumbInfo[i]->pDoc->RemoveView(m_thumbInfo[i]->pView);
			m_thumbInfo[i]->pView->DestroyWindow();
		}
		delete	m_thumbInfo[i]->pDoc;
		delete	m_thumbInfo[i];
	}
	m_thumbInfo.RemoveAll();

	// ﾌｧｲﾙ一覧の取得
	for ( i=0; i<m_aExt.GetSize(); i++ )
		SetAllFileFromFolder(strPath, m_aExt[i]);

	if ( m_thumbInfo.GetSize() <= 9 )
		m_ctScroll.EnableScrollBar(ESB_DISABLE_BOTH);
	else {
		int	nRange;
		if ( m_thumbInfo.GetSize() % 3 == 0 )
			nRange = m_thumbInfo.GetSize() - 3;
		else
			nRange = m_thumbInfo.GetSize() / 3 * 3;
		m_ctScroll.SetScrollRange(0, nRange, FALSE);
		m_ctScroll.EnableScrollBar(ESB_ENABLE_BOTH);
		m_ctScroll.SetScrollPos(0);
	}

	// ｻﾑﾈｲﾙ情報の並び替え
	SortThumbnailInfo();

	// ﾌｧｲﾙ列挙の数だけﾄﾞｷｭﾒﾝﾄとﾋﾞｭｰを生成
	SetThumbnailDocument();
}

void CThumbnailDlg::SetAllFileFromFolder
	(const CString& strFolder, const CString& strExtension)
{
	BOOL			bContinue;
	CFileFind		cFind;
	CFileStatus		rStatus;
	LPTHUMBNAILINFO	pInfo;
	CString			strPath(strFolder);

	if ( strPath.Right(1) != _T("\\") )
		strPath += _T("\\");
	strPath += strExtension;

	try {
		bContinue = cFind.FindFile(strPath);
		while ( bContinue ) {
			bContinue = cFind.FindNextFile();
			if ( !cFind.IsDirectory() && CFile::GetStatus(cFind.GetFilePath(), rStatus) ) {
				pInfo = new THUMBNAILINFO;
				pInfo->fStatus = rStatus;
				pInfo->pDoc  = NULL;
				pInfo->pView = NULL;
				m_thumbInfo.Add(pInfo);
			}
		}
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
	}
}

void CThumbnailDlg::SortThumbnailInfo(void)
{
	PFNSORTFUNC		pfnSort = NULL;

	switch ( m_nSort ) {
	case 0:		// ﾌｧｲﾙ名(昇順)
		pfnSort = FileNameCompareFunc1;
		break;
	case 1:		// ﾌｧｲﾙ名(降順)
		pfnSort = FileNameCompareFunc2;
		break;
	case 2:		// 更新日付(昇順)
		pfnSort = UpdateTimeCompareFunc1;
		break;
	case 3:		// 更新日付(降順)
		pfnSort = UpdateTimeCompareFunc2;
		break;
	case 4:		// ｻｲｽﾞ(昇順)
		pfnSort = FileSizeCompareFunc1;
		break;
	case 5:		// ｻｲｽﾞ(降順)
		pfnSort = FileSizeCompareFunc2;
		break;
	}

	if ( pfnSort )
		m_thumbInfo.Sort(pfnSort);
}

void CThumbnailDlg::SetThumbnailDocument(void)
{
	int		i;
	DWORD	dwStyle;
	CRect	rc;
	CNCDoc*	pDoc;
	CView*	pView;
	CWnd*	pParent;

	// ﾌｧｲﾙ列挙の数だけﾄﾞｷｭﾒﾝﾄとﾋﾞｭｰを生成
	for ( i=0; i<m_thumbInfo.GetSize(); i++ ) {
		pDoc  = static_cast<CNCDoc*>(RUNTIME_CLASS(CNCDoc)->CreateObject());
		if ( !pDoc ) {
			m_thumbInfo[i]->pDoc  = NULL;
			m_thumbInfo[i]->pView = NULL;
			continue;
		}
		pView = CreatePlaneView();
		if ( pView ) {
			if ( i < SIZEOF(m_ctChild) ) {
				m_ctChild[i].GetClientRect(&rc);
				dwStyle = AFX_WS_DEFAULT_VIEW;
				pParent = &m_ctChild[i];
			}
			else {
				rc.SetRectEmpty();
				dwStyle = AFX_WS_DEFAULT_VIEW & ~WS_VISIBLE;
				pParent = &m_ctParentView;
			}
			if ( pView->Create(NULL, NULL, dwStyle, rc, pParent, AFX_IDW_PANE_FIRST) ) {
				pDoc->AddView(pView);
				// この時点ではﾃﾞｰﾀを読み込めていないので
				// 本来は pView->OnInitialUpdate() は無効であるが
				// 呼ばないと不明なﾌﾞﾚｲｸが発生する
				pView->OnInitialUpdate();
			}
			else {
				pView->DestroyWindow();
				pView = NULL;
			}
		}	// pView==NULLはCreateEnumDoc_Thread()で弾く
		pDoc->SetDocFlag(NCDOC_THUMBNAIL);	// ｻﾑﾈｲﾙ表示ﾓｰﾄﾞに設定
		m_thumbInfo[i]->pDoc  = pDoc;
		m_thumbInfo[i]->pView = pView;
	}

	// ﾌｧｲﾙ読み込み処理は別ｽﾚｯﾄﾞで
	if ( !m_thumbInfo.IsEmpty() ) {
		m_pEnumDocThread = AfxBeginThread(CreateEnumDoc_Thread, this,
				THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
		if ( m_pEnumDocThread ) {
			m_bEnumDoc = TRUE;
			m_pEnumDocThread->m_bAutoDelete = FALSE;
			m_pEnumDocThread->ResumeThread();
		}
		else
			m_bEnumDoc = FALSE;
	}
}

CView* CThumbnailDlg::CreatePlaneView(void)
{
	CRuntimeClass*	pClass;
	CView*			pView = NULL;
	switch ( m_enPlane ) {
	case NC_XYZ_PLANE:
		pClass = RUNTIME_CLASS(CNCView);
		break;
	case NC_XY_PLANE:
		pClass = RUNTIME_CLASS(CNCViewXY);
		break;
	case NC_XZ_PLANE:
		pClass = RUNTIME_CLASS(CNCViewXZ);
		break;
	case NC_YZ_PLANE:
		pClass = RUNTIME_CLASS(CNCViewYZ);
		break;
	default:
		pClass = NULL;
	}
	if ( pClass )
		pView = static_cast<CView*>(pClass->CreateObject());
	return pView;
}

void CThumbnailDlg::WaitEnumDocThread(BOOL bStop/*=TRUE*/)
{
	if ( bStop )
		m_bEnumDoc = FALSE;
	if ( m_pEnumDocThread ) {
#ifdef _DEBUG
		CMagaDbg	dbg("CThumbnailDlg::WaitEnumDocThread()", DBG_BLUE);
		if ( ::WaitForSingleObject(m_pEnumDocThread->m_hThread, INFINITE) == WAIT_FAILED ) {
			dbg.printf("WaitForSingleObject() Fail!");
			::NC_FormatMessage();
		}
		else
			dbg.printf("WaitForSingleObject() OK");
#else
		::WaitForSingleObject(m_pEnumDocThread->m_hThread, INFINITE);
#endif
		delete	m_pEnumDocThread;
		m_pEnumDocThread = NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CThumbnailDlg メッセージ ハンドラ

BOOL CThumbnailDlg::OnInitDialog()
{
	extern	LPCTSTR	gg_szWild;	// "*.";
	int		i;
	CRect	rc;

	__super::OnInitDialog();

	// ﾂﾘｰﾋﾞｭｰのﾃﾞｽｸﾄｯﾌﾟを展開
	HTREEITEM hTree = m_ctFolderTree.GetRootItem();
	if ( hTree )
		m_ctFolderTree.Expand(hTree, TVE_EXPAND);
	// 現在のｶﾚﾝﾄﾊﾟｽを選択
	CString	strPath, strFile;
	::Path_Name_From_FullPath(AfxGetNCVCApp()->GetRecentFileName(), strPath, strFile);
	LPITEMIDLIST	pidl;
	CShellManager*	pShell = AfxGetNCVCApp()->GetShellManager();
	if ( pShell->ItemFromPath(strPath, pidl) == NOERROR )
		m_ctFolderTree.SelectPath(pidl);
	else
		m_ctFolderTree.SelectPath(strPath);

	// CStatic生成
	rc.SetRectEmpty();
	for ( i=0; i<SIZEOF(m_ctChild); i++ ) {
		if ( !m_ctChild[i].Create(NULL, WS_CHILD|WS_VISIBLE|SS_SUNKEN|SS_ENHMETAFILE|SS_NOTIFY,
					rc, &m_ctParentView, i+1) ) {
			NCVC_ControlErrorMsg(__FILE__, __LINE__);
			EndDialog(IDCANCEL);
		}
	}

	// 現在登録されている拡張子を取得
	LPVOID		pFunc;		// dummy
	CString		strExt;
	m_aExt.Add( gg_szWild + AfxGetNCVCApp()->GetDocExtString(TYPE_NCD).Right(3) );	// "." 除く「ncd」
	for ( i=0; i<2/*EXT_ADN,EXT_DLG*/; i++ ) {	// 登録拡張子でのﾌｫﾙﾀﾞ検索
		const CMapStringToPtr* pMap = AfxGetNCVCApp()->GetDocTemplate(TYPE_NCD)->GetExtMap((eEXTTYPE)i);
		for ( POSITION pos=pMap->GetStartPosition(); pos; ) {
			pMap->GetNextAssoc(pos, strExt, pFunc);
			m_aExt.Add( gg_szWild + strExt );
		}
	}
	// 初期ﾌｫﾙﾀﾞに含まれるﾌｧｲﾙを列挙
	ChangeFolder(strPath);

	// 以降のOnSize()を許可する
	m_bInitialize = TRUE;

	// ｳｨﾝﾄﾞｳ情報のﾘｽﾄｱ
	WINDOWPLACEMENT		wp;
	wp.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(&wp);
	CString	strRegKey(GetSubTreeRegKey(IDS_REGKEY_WINDOW, IDS_REGKEY_WINDOW_THUMBNAILDLG));
	if ( AfxGetNCVCApp()->GetWindowState(strRegKey, &wp) )
		SetWindowPlacement(&wp);

	// ｺﾝﾄﾛｰﾙの配置
	GetClientRect(&rc);
	ResizeControl(rc.Width(), rc.Height());

	return TRUE;  // return TRUE unless you set the focus to a control
	// 例外 : OCX プロパティ ページは必ず FALSE を返します。
}

void CThumbnailDlg::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);

	if ( m_bInitialize )	// OnInitDialog()前のOnSize()は無視
		ResizeControl(cx, cy);
}

void CThumbnailDlg::OnSelchanged(NMHDR *pNMHDR, LRESULT *pResult)
{
	if ( m_bInitialize ) {
		LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
		CString	strPath;
		if ( m_ctFolderTree.GetItemPath(strPath, pNMTreeView->itemNew.hItem) )
			ChangeFolder(strPath);
	}
	*pResult = 0;
}

void CThumbnailDlg::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if ( !pScrollBar ) {
		__super::OnVScroll(nSBCode, nPos, pScrollBar);
		return;
	}

	int		i, j, nCnt, nPosNow = pScrollBar->GetScrollPos();
	CRect	rc;

	switch ( nSBCode ) {
	case SB_LINEUP:
		if ( nPosNow <= 0 )
			break;
		for ( i=nPosNow+6; i<nPosNow+9 && i<m_thumbInfo.GetSize(); i++ ) {
			if ( m_thumbInfo[i]->pView ) {
				m_thumbInfo[i]->pView->ShowWindow(SW_HIDE);
				m_thumbInfo[i]->pView->SetParent(&m_ctParentView);
			}
		}
		for ( i=nPosNow-3, j=0; i<m_thumbInfo.GetSize() && j<SIZEOF(m_ctChild); i++, j++ ) {
			if ( m_thumbInfo[i]->pView ) {
				m_ctChild[j].GetClientRect(&rc);
				m_thumbInfo[i]->pView->SetParent(&m_ctChild[j]);
				m_thumbInfo[i]->pView->MoveWindow(0, 0, rc.Width(), rc.Height());
				m_thumbInfo[i]->pView->ShowWindow(SW_SHOWNA);
				m_thumbInfo[i]->pView->PostMessage(WM_USERVIEWFITMSG, 0, 1);
			}
		}
		pScrollBar->SetScrollPos(nPosNow-3);
		break;
	case SB_PAGEUP:
		if ( nPosNow <= 0 )
			break;
		nCnt = nPosNow - 9;
		if ( nCnt < 0 )
			nCnt = 0;
		for ( i=nPosNow; i<nPosNow+9 && i<m_thumbInfo.GetSize(); i++ ) {
			if ( m_thumbInfo[i]->pView ) {
				m_thumbInfo[i]->pView->ShowWindow(SW_HIDE);
				m_thumbInfo[i]->pView->SetParent(&m_ctParentView);
			}
		}
		for ( i=nCnt, j=0; i<m_thumbInfo.GetSize() && j<SIZEOF(m_ctChild); i++, j++ ) {
			if ( m_thumbInfo[i]->pView ) {
				m_ctChild[j].GetClientRect(&rc);
				m_thumbInfo[i]->pView->SetParent(&m_ctChild[j]);
				m_thumbInfo[i]->pView->MoveWindow(0, 0, rc.Width(), rc.Height());
				m_thumbInfo[i]->pView->ShowWindow(SW_SHOWNA);
				m_thumbInfo[i]->pView->PostMessage(WM_USERVIEWFITMSG, 0, 1);
			}
		}
		pScrollBar->SetScrollPos(nCnt);
		break;
	case SB_LINEDOWN:
	case SB_PAGEDOWN:
		nCnt = nSBCode==SB_LINEDOWN ? 3 : 9;
		if ( nPosNow+3 >= m_thumbInfo.GetSize() )
			break;
		if ( nSBCode==SB_PAGEDOWN && nPosNow+nCnt>=m_thumbInfo.GetSize() ) {
			if ( m_thumbInfo.GetSize() % 3 == 0 )
				nCnt = m_thumbInfo.GetSize() - 3 - nPosNow;
			else
				nCnt = m_thumbInfo.GetSize()/3*3 - nPosNow;
		}
		for ( i=nPosNow; i<nPosNow+nCnt && i<m_thumbInfo.GetSize(); i++ ) {
			if ( m_thumbInfo[i]->pView ) {
				m_thumbInfo[i]->pView->ShowWindow(SW_HIDE);
				m_thumbInfo[i]->pView->SetParent(&m_ctParentView);
			}
		}
		nPosNow = i;
		for ( j=0; i<m_thumbInfo.GetSize() && j<SIZEOF(m_ctChild); i++, j++ ) {
			if ( m_thumbInfo[i]->pView ) {
				m_ctChild[j].GetClientRect(&rc);
				m_thumbInfo[i]->pView->SetParent(&m_ctChild[j]);
				m_thumbInfo[i]->pView->MoveWindow(0, 0, rc.Width(), rc.Height());
				m_thumbInfo[i]->pView->ShowWindow(SW_SHOWNA);
				m_thumbInfo[i]->pView->PostMessage(WM_USERVIEWFITMSG, 0, 1);
			}
		}
		pScrollBar->SetScrollPos(nPosNow);
		break;
	case SB_THUMBTRACK:
	case SB_THUMBPOSITION:
		nCnt = (int)nPos/3*3;
		if ( nPosNow == nCnt )
			break;
		for ( i=nPosNow; i<nPosNow+9 && i<m_thumbInfo.GetSize(); i++ ) {
			if ( m_thumbInfo[i]->pView ) {
				m_thumbInfo[i]->pView->ShowWindow(SW_HIDE);
				m_thumbInfo[i]->pView->SetParent(&m_ctParentView);
			}
		}
		for ( i=nCnt, j=0; i<m_thumbInfo.GetSize() && j<SIZEOF(m_ctChild); i++, j++ ) {
			if ( m_thumbInfo[i]->pView ) {
				m_ctChild[j].GetClientRect(&rc);
				m_thumbInfo[i]->pView->SetParent(&m_ctChild[j]);
				m_thumbInfo[i]->pView->MoveWindow(0, 0, rc.Width(), rc.Height());
				m_thumbInfo[i]->pView->ShowWindow(SW_SHOWNA);
				m_thumbInfo[i]->pView->PostMessage(WM_USERVIEWFITMSG, 0, 1);
			}
		}
		pScrollBar->SetScrollPos(nCnt);
		break;
	case SB_ENDSCROLL:
		m_ctChild[0].SetFocus();
		break;
	}
}

void CThumbnailDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	m_ctChild[0].SetFocus();
	__super::OnLButtonUp(nFlags, point);
}

BOOL CThumbnailDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if ( zDelta <= -WHEEL_DELTA )
		OnVScroll(SB_LINEDOWN, -1, &m_ctScroll);
	else if ( zDelta >= WHEEL_DELTA )
		OnVScroll(SB_LINEUP, -1, &m_ctScroll);

	return TRUE;
}

void CThumbnailDlg::OnDestroy()
{
	CWaitCursor		wait;
	WaitEnumDocThread();

	// ｳｨﾝﾄﾞｳ状態をﾚｼﾞｽﾄﾘに保存
	WINDOWPLACEMENT		wp;
	wp.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(&wp);	// ｳｨﾝﾄﾞｳの状態Get
	CString	strRegKey(GetSubTreeRegKey(IDS_REGKEY_WINDOW, IDS_REGKEY_WINDOW_THUMBNAILDLG));
	AfxGetNCVCApp()->SaveWindowState(strRegKey, wp);

	__super::OnDestroy();
	// 親が消えれば子(m_ctChild[])も消える（はず）
}

void CThumbnailDlg::OnSelchangeSort()
{
	UpdateData();
	int	i, nPosNow = m_ctScroll.GetScrollPos();
	CRect	rc;

	// 現在のｻﾑﾈｲﾙ表示を非表示に
	for ( i=nPosNow; i<nPosNow+9 && i<m_thumbInfo.GetSize(); i++ ) {
		if ( m_thumbInfo[i]->pView ) {
			m_thumbInfo[i]->pView->ShowWindow(SW_HIDE);
			m_thumbInfo[i]->pView->SetParent(&m_ctParentView);
		}
	}

	// ｻﾑﾈｲﾙ情報の並び替え
	SortThumbnailInfo();

	// 再表示
	for ( i=0; i<SIZEOF(m_ctChild) && i<m_thumbInfo.GetSize(); i++ ) {
		if ( m_thumbInfo[i]->pView ) {
			m_ctChild[i].GetClientRect(&rc);
			m_thumbInfo[i]->pView->SetParent(&m_ctChild[i]);
			m_thumbInfo[i]->pView->MoveWindow(0, 0, rc.Width(), rc.Height());
			m_thumbInfo[i]->pView->ShowWindow(SW_SHOWNA);
			m_thumbInfo[i]->pView->PostMessage(WM_USERVIEWFITMSG, 0, 1);
		}
	}

	m_ctScroll.SetScrollPos(0);
	m_ctChild[0].SetFocus();
}

void CThumbnailDlg::OnSelchangePlane()
{
	CWaitCursor		wait;
	WaitEnumDocThread(FALSE);	// 読み込みｽﾚｯﾄﾞは停止させない

	UpdateData();
	m_enPlane = (ENNCVPLANE)m_nPlane;

	int		i, j, nPosNow = m_ctScroll.GetScrollPos();
	DWORD	dwStyle;
	CRect	rc;
	CView*	pView;
	CWnd*	pParent;

	// 現在のﾋﾞｭｰをﾄﾞｷｭﾒﾝﾄから切り離して破棄
	for ( i=0; i<m_thumbInfo.GetSize(); i++ ) {
		if ( m_thumbInfo[i]->pView ) {
			m_thumbInfo[i]->pDoc->RemoveView(m_thumbInfo[i]->pView);
			m_thumbInfo[i]->pView->DestroyWindow();
		}
	}

	// 選択されたﾋﾞｭｰを再構築

	// 0 -> nPosNow-1
	rc.SetRectEmpty();
	dwStyle = AFX_WS_DEFAULT_VIEW & ~WS_VISIBLE;
	pParent = &m_ctParentView;
	for ( i=0; i<nPosNow && i<m_thumbInfo.GetSize(); i++ ) {
		pView = CreatePlaneView();
		if ( pView ) {
			if ( pView->Create(NULL, NULL, dwStyle, rc, pParent, AFX_IDW_PANE_FIRST) ) {
				m_thumbInfo[i]->pDoc->AddView(pView);
				pView->OnInitialUpdate();
			}
			else {
				pView->DestroyWindow();
				pView = NULL;
			}
		}
		m_thumbInfo[i]->pView = pView;
	}
	// nPosNow -> SIZEOF(m_ctChild)-1
	dwStyle = AFX_WS_DEFAULT_VIEW;
	for ( j=0; i<m_thumbInfo.GetSize() && j<SIZEOF(m_ctChild); i++, j++ ) {
		pView = CreatePlaneView();
		if ( pView ) {
			m_ctChild[j].GetClientRect(&rc);
			pParent = &m_ctChild[j];
			if ( pView->Create(NULL, NULL, dwStyle, rc, pParent, AFX_IDW_PANE_FIRST) ) {
				m_thumbInfo[i]->pDoc->AddView(pView);
				pView->OnInitialUpdate();
			}
			else {
				pView->DestroyWindow();
				pView = NULL;
			}
		}
		pView->PostMessage(WM_USERVIEWFITMSG, 0, 1);
		m_thumbInfo[i]->pView = pView;
	}
	// 残り
	rc.SetRectEmpty();
	dwStyle = AFX_WS_DEFAULT_VIEW & ~WS_VISIBLE;
	pParent = &m_ctParentView;
	for ( ; i<m_thumbInfo.GetSize(); i++ ) {
		pView = CreatePlaneView();
		if ( pView ) {
			if ( pView->Create(NULL, NULL, dwStyle, rc, pParent, AFX_IDW_PANE_FIRST) ) {
				m_thumbInfo[i]->pDoc->AddView(pView);
				pView->OnInitialUpdate();
			}
			else {
				pView->DestroyWindow();
				pView = NULL;
			}
		}
		m_thumbInfo[i]->pView = pView;
	}

	m_ctChild[0].SetFocus();
}

LRESULT CThumbnailDlg::OnUserDblClk(WPARAM wParam, LPARAM)
{
	CWaitCursor		wait;
	WaitEnumDocThread();
	m_strFile = reinterpret_cast<CView*>(wParam)->GetDocument()->GetPathName();
	EndDialog(IDOK);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CThumbnailStatic メッセージ ハンドラ

void CThumbnailStatic::OnLButtonDown(UINT nFlags, CPoint point) 
{
	// 自力でﾌｫｰｶｽを取得することでﾏｳｽｲﾍﾞﾝﾄが親ﾀﾞｲｱﾛｸﾞに通知
	SetFocus();
}

LRESULT CThumbnailStatic::OnUserDblClk(WPARAM wParam, LPARAM lParam)
{
	// CStatic(m_ctParentView) -> CThumbnailDlg
	GetParent()->GetParent()->SendMessage(WM_USERFINISH, wParam, lParam);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CThumbnailDlg サブスレッド

UINT CThumbnailDlg::CreateEnumDoc_Thread(LPVOID pVoid)
{
	CThumbnailDlg*	pParent = reinterpret_cast<CThumbnailDlg*>(pVoid);
	LPTHUMBNAILINFO pInfo;
	CDocument*		dummy;

	// 無理にﾏﾙﾁｽﾚｯﾄﾞで処理せず、ｼｰｹﾝｼｬﾙに1件ずつ
	// → TH_NCRead.cpp で __declspec(thread) の外部変数が必要なくなる
	for ( int i=0; i<pParent->m_thumbInfo.GetSize() && pParent->m_bEnumDoc; i++ ) {
		pInfo = pParent->m_thumbInfo[i];
		if ( pInfo->pView ) {
			// ﾌｧｲﾙを開くから一連の動作へ
			AfxGetNCVCApp()->GetDocTemplate(TYPE_NCD)->MatchDocType(pInfo->fStatus.m_szFullName, dummy);	// Serialize関数の決定
			pInfo->pDoc->ReadThumbnail(pInfo->fStatus.m_szFullName);
			// ﾋﾞｭｰへの通知
			pInfo->pView->OnInitialUpdate();
			pInfo->pView->PostMessage(WM_USERVIEWFITMSG, 0, 1);
		}
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// 並べ替え補助関数

int FileNameCompareFunc1(LPTHUMBNAILINFO pFirst, LPTHUMBNAILINFO pSecond)
{
	CString	strFirst(pFirst->fStatus.m_szFullName);
	return strFirst.CompareNoCase(pSecond->fStatus.m_szFullName);
}

int FileNameCompareFunc2(LPTHUMBNAILINFO pFirst, LPTHUMBNAILINFO pSecond)
{
	CString	strSecond(pSecond->fStatus.m_szFullName);
	return strSecond.CompareNoCase(pFirst->fStatus.m_szFullName);
}

int UpdateTimeCompareFunc1(LPTHUMBNAILINFO pFirst, LPTHUMBNAILINFO pSecond)
{
	if ( pFirst->fStatus.m_mtime < pSecond->fStatus.m_mtime )
		return 1;
	else if ( pFirst->fStatus.m_mtime > pSecond->fStatus.m_mtime )
		return -1;
	else
		return 0;
}

int UpdateTimeCompareFunc2(LPTHUMBNAILINFO pFirst, LPTHUMBNAILINFO pSecond)
{
	if ( pFirst->fStatus.m_mtime > pSecond->fStatus.m_mtime )
		return 1;
	else if ( pFirst->fStatus.m_mtime < pSecond->fStatus.m_mtime )
		return -1;
	else
		return 0;
}

int FileSizeCompareFunc1(LPTHUMBNAILINFO pFirst, LPTHUMBNAILINFO pSecond)
{
	if ( pFirst->fStatus.m_size < pSecond->fStatus.m_size )
		return 1;
	else if ( pFirst->fStatus.m_size > pSecond->fStatus.m_size )
		return -1;
	else
		return 0;
}

int FileSizeCompareFunc2(LPTHUMBNAILINFO pFirst, LPTHUMBNAILINFO pSecond)
{
	if ( pFirst->fStatus.m_size > pSecond->fStatus.m_size )
		return 1;
	else if ( pFirst->fStatus.m_size < pSecond->fStatus.m_size )
		return -1;
	else
		return 0;
}
