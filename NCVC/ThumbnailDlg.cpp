// ThumbnailDlg.cpp : �C���v�������e�[�V���� �t�@�C��
//

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
	ON_NOTIFY(TVN_SELCHANGED, IDC_THUMBNAIL_TREE, &CThumbnailDlg::OnSelchanged)
	ON_WM_VSCROLL()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEWHEEL()
	ON_WM_DESTROY()
	ON_CBN_SELCHANGE(IDC_THUMBNAIL_SORTNAME, &CThumbnailDlg::OnSelchangeSort)
	ON_MESSAGE(WM_USERFINISH, OnUserDblClk)
	ON_CBN_SELCHANGE(IDC_THUMBNAIL_PLANE, &CThumbnailDlg::OnSelchangePlane)
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(CThumbnailStatic, CStatic)
	ON_WM_LBUTTONDOWN()
	ON_MESSAGE(WM_USERFINISH, OnUserDblClk)
END_MESSAGE_MAP()

struct	ENUMDOCTHREADPARAM {
	CNCDoc*		pDoc;
	CView*		pView;
	CString		strFile;
};
#define	LPENUMDOCTHREADPARAM	ENUMDOCTHREADPARAM *
static UINT	EnumDocument_Thread(LPVOID);

// ���בւ��⏕�֐�
static int	FileNameCompareFunc1(LPTHUMBNAILINFO, LPTHUMBNAILINFO);		// ̧�ٖ��������
static int	FileNameCompareFunc2(LPTHUMBNAILINFO, LPTHUMBNAILINFO);		// ̧�ٖ��~�����
static int	UpdateTimeCompareFunc1(LPTHUMBNAILINFO, LPTHUMBNAILINFO);	// �X�V���t�������
static int	UpdateTimeCompareFunc2(LPTHUMBNAILINFO, LPTHUMBNAILINFO);	// �X�V���t�~�����
static int	FileSizeCompareFunc1(LPTHUMBNAILINFO, LPTHUMBNAILINFO);		// ̧�ٻ��ޏ������
static int	FileSizeCompareFunc2(LPTHUMBNAILINFO, LPTHUMBNAILINFO);		// ̧�ٻ��ލ~�����

/////////////////////////////////////////////////////////////////////////////
// CThumbnailDlg �_�C�A���O

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
	// �ޭ��͐e���j�������Ɠ����� DestroyWindow() �����
	for ( int i=0; i<m_aInfo.GetSize(); i++ ) {
		delete	m_aInfo[i]->pDoc;
		delete	m_aInfo[i];
	}
	m_aInfo.RemoveAll();
}

void CThumbnailDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
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

	// �e���۰ق̍Ĕz�u
	m_ctFolderTree.GetWindowRect(&rcBase);	// �
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

	// ��Ȳٕ\���ر
	m_ctParentView.GetClientRect(&rcBase);
	nWidth  = (rcBase.Width()  - 1) / 3;
	nHeight = (rcBase.Height() - 1) / 3;
	for ( i=0; i<3; i++ ) {			// �R�s
		for ( j=0; j<3; j++ ) {		// �R��
			m_ctChild[i*3+j].MoveWindow(nWidth*j+1, nHeight*i+1, nWidth, nHeight);
		}
	}
	// �ޭ��̒���
	for ( i=m_ctScroll.GetScrollPos(), j=0; i<m_aInfo.GetSize() && j<SIZEOF(m_ctChild); i++, j++ ) {
		if ( m_aInfo[i]->pView ) {
			m_ctChild[j].GetClientRect(&rc);
			m_aInfo[i]->pView->MoveWindow(0, 0, rc.Width(), rc.Height());
			m_aInfo[i]->pView->PostMessage(WM_USERVIEWFITMSG, 0, 1);
		}
	}
}

void CThumbnailDlg::ChangeFolder(const CString& strPath)
{
	int		i;
	CWaitCursor		wait;
	// �O��̽گ�ޑ҂�
	WaitEnumDocThread();

	// �߽�\���̍œK��(shlwapi.h)
	::PathSetDlgItemPath(m_hWnd, IDC_THUMBNAIL_PATH, strPath);

	// ���۰ق�ؾ��
	for ( i=0; i<m_aInfo.GetSize(); i++ ) {
		if ( m_aInfo[i]->pView ) {
			m_aInfo[i]->pDoc->RemoveView(m_aInfo[i]->pView);
			m_aInfo[i]->pView->DestroyWindow();
		}
		delete	m_aInfo[i]->pDoc;
		delete	m_aInfo[i];
	}
	m_aInfo.RemoveAll();

	// ̧�وꗗ�̎擾
	for ( i=0; i<m_aExt.GetSize(); i++ )
		SetAllFileFromFolder(strPath, m_aExt[i]);

	if ( m_aInfo.GetSize() <= 9 )
		m_ctScroll.EnableScrollBar(ESB_DISABLE_BOTH);
	else {
		int	nRange;
		if ( m_aInfo.GetSize() % 3 == 0 )
			nRange = m_aInfo.GetSize() - 3;
		else
			nRange = m_aInfo.GetSize() / 3 * 3;
		m_ctScroll.SetScrollRange(0, nRange, FALSE);
		m_ctScroll.EnableScrollBar(ESB_ENABLE_BOTH);
		m_ctScroll.SetScrollPos(0);
	}

	// ��Ȳُ��̕��ёւ�
	SortThumbnailInfo();

	// ̧�ٗ񋓂̐������޷���Ă��ޭ��𐶐�
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
				m_aInfo.Add(pInfo);
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
	switch ( m_nSort ) {
	case 0:		// ̧�ٖ�(����)
		m_aInfo.Sort(FileNameCompareFunc1);
		break;
	case 1:		// ̧�ٖ�(�~��)
		m_aInfo.Sort(FileNameCompareFunc2);
		break;
	case 2:		// �X�V���t(����)
		m_aInfo.Sort(UpdateTimeCompareFunc1);
		break;
	case 3:		// �X�V���t(�~��)
		m_aInfo.Sort(UpdateTimeCompareFunc2);
		break;
	case 4:		// ����(����)
		m_aInfo.Sort(FileSizeCompareFunc1);
		break;
	case 5:		// ����(�~��)
		m_aInfo.Sort(FileSizeCompareFunc2);
		break;
	}
}

void CThumbnailDlg::SetThumbnailDocument(void)
{
	int		i;
	DWORD	dwStyle;
	CRect	rc;
	CNCDoc*	pDoc;
	CView*	pView;
	CWnd*	pParent;

	// ̧�ٗ񋓂̐������޷���Ă��ޭ��𐶐�
	for ( i=0; i<m_aInfo.GetSize(); i++ ) {
		pDoc  = static_cast<CNCDoc*>(RUNTIME_CLASS(CNCDoc)->CreateObject());
		if ( !pDoc ) {
			m_aInfo[i]->pDoc  = NULL;
			m_aInfo[i]->pView = NULL;
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
				// ���̎��_�ł��ް���ǂݍ��߂Ă��Ȃ��̂�
				// �{���� pView->OnInitialUpdate() �͖����ł��邪
				// �Ă΂Ȃ��ƕs������ڲ�����������
				pView->OnInitialUpdate();
			}
			else {
				pView->DestroyWindow();
				pView = NULL;
			}
		}
		pDoc->SetThumbnailMode();		// ��Ȳٕ\��Ӱ�ނɐݒ�
		m_aInfo[i]->pDoc  = pDoc;
		m_aInfo[i]->pView = pView;
	}

	// ̧�ٓǂݍ��ݏ���������گ�ނŁI
	if ( !m_aInfo.IsEmpty() ) {
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
	CView*	pView;
	switch ( m_enPlane ) {
	case NC_XYZ_PLANE:
		pView = static_cast<CView*>(RUNTIME_CLASS(CNCView)->CreateObject());
		break;
	case NC_XY_PLANE:
		pView = static_cast<CView*>(RUNTIME_CLASS(CNCViewXY)->CreateObject());
		break;
	case NC_XZ_PLANE:
		pView = static_cast<CView*>(RUNTIME_CLASS(CNCViewXZ)->CreateObject());
		break;
	case NC_YZ_PLANE:
		pView = static_cast<CView*>(RUNTIME_CLASS(CNCViewYZ)->CreateObject());
		break;
	default:
		pView = NULL;
	}
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
// CThumbnailDlg ���b�Z�[�W �n���h��

BOOL CThumbnailDlg::OnInitDialog()
{
	extern	LPCTSTR	gg_szWild;	// "*.";
	int		i;
	CRect	rc;

	CDialog::OnInitDialog();

	// �ذ�ޭ����޽�į�߂�W�J
	HTREEITEM hTree = m_ctFolderTree.GetRootItem();
	if ( hTree )
		m_ctFolderTree.Expand(hTree, TVE_EXPAND);
	// ���̶݂����߽��I��
	CString	strPath, strFile;
	::Path_Name_From_FullPath(AfxGetNCVCApp()->GetRecentFileName(), strPath, strFile);
	LPITEMIDLIST	pidl;
	CShellManager*	pShell = AfxGetNCVCApp()->GetShellManager();
	if ( pShell->ItemFromPath(strPath, pidl) == NOERROR )
		m_ctFolderTree.SelectPath(pidl);
	else
		m_ctFolderTree.SelectPath(strPath);

	// CStatic����
	rc.SetRectEmpty();
	for ( i=0; i<SIZEOF(m_ctChild); i++ )
		m_ctChild[i].Create(NULL, WS_CHILD|WS_VISIBLE|SS_SUNKEN|SS_ENHMETAFILE|SS_NOTIFY,
			rc, &m_ctParentView, i+1);

	// ���ݓo�^����Ă���g���q���擾
	LPVOID		pFunc;		// dummy
	CString		strExt;
	m_aExt.Add( gg_szWild + AfxGetNCVCApp()->GetDocExtString(TYPE_NCD).Right(3) );	// "." �����uncd�v
	for ( i=0; i<2/*EXT_ADN,EXT_DLG*/; i++ ) {	// �o�^�g���q�ł�̫��ތ���
		const CMapStringToPtr* pMap = AfxGetNCVCApp()->GetDocTemplate(TYPE_NCD)->GetExtMap((eEXTTYPE)i);
		for ( POSITION pos=pMap->GetStartPosition(); pos; ) {
			pMap->GetNextAssoc(pos, strExt, pFunc);
			m_aExt.Add( gg_szWild + strExt );
		}
	}
	// ����̫��ނɊ܂܂��̧�ق��
	ChangeFolder(strPath);

	// �ȍ~��OnSize()��������
	m_bInitialize = TRUE;

	// ����޳����ؽı
	WINDOWPLACEMENT		wp;
	wp.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(&wp);
	CString	strRegKey(GetSubTreeRegKey(IDS_REGKEY_WINDOW, IDS_REGKEY_WINDOW_THUMBNAILDLG));
	if ( AfxGetNCVCApp()->GetWindowState(strRegKey, &wp) )
		SetWindowPlacement(&wp);

	// ���۰ق̔z�u
	GetClientRect(&rc);
	ResizeControl(rc.Width(), rc.Height());

	return TRUE;  // return TRUE unless you set the focus to a control
	// ��O : OCX �v���p�e�B �y�[�W�͕K�� FALSE ��Ԃ��܂��B
}

void CThumbnailDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	if ( m_bInitialize )	// OnInitDialog()�O��OnSize()�͖���
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
		CDialog::OnVScroll(nSBCode, nPos, pScrollBar);
		return;
	}

	int		i, j, nCnt, nPosNow = pScrollBar->GetScrollPos();
	CRect	rc;

	switch ( nSBCode ) {
	case SB_LINEUP:
		if ( nPosNow <= 0 )
			break;
		for ( i=nPosNow+6; i<nPosNow+9 && i<m_aInfo.GetSize(); i++ ) {
			if ( m_aInfo[i]->pView ) {
				m_aInfo[i]->pView->ShowWindow(SW_HIDE);
				m_aInfo[i]->pView->SetParent(&m_ctParentView);
			}
		}
		for ( i=nPosNow-3, j=0; i<m_aInfo.GetSize() && j<SIZEOF(m_ctChild); i++, j++ ) {
			if ( m_aInfo[i]->pView ) {
				m_ctChild[j].GetClientRect(&rc);
				m_aInfo[i]->pView->SetParent(&m_ctChild[j]);
				m_aInfo[i]->pView->MoveWindow(0, 0, rc.Width(), rc.Height());
				m_aInfo[i]->pView->ShowWindow(SW_SHOWNA);
				m_aInfo[i]->pView->PostMessage(WM_USERVIEWFITMSG, 0, 1);
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
		for ( i=nPosNow; i<nPosNow+9 && i<m_aInfo.GetSize(); i++ ) {
			if ( m_aInfo[i]->pView ) {
				m_aInfo[i]->pView->ShowWindow(SW_HIDE);
				m_aInfo[i]->pView->SetParent(&m_ctParentView);
			}
		}
		for ( i=nCnt, j=0; i<m_aInfo.GetSize() && j<SIZEOF(m_ctChild); i++, j++ ) {
			if ( m_aInfo[i]->pView ) {
				m_ctChild[j].GetClientRect(&rc);
				m_aInfo[i]->pView->SetParent(&m_ctChild[j]);
				m_aInfo[i]->pView->MoveWindow(0, 0, rc.Width(), rc.Height());
				m_aInfo[i]->pView->ShowWindow(SW_SHOWNA);
				m_aInfo[i]->pView->PostMessage(WM_USERVIEWFITMSG, 0, 1);
			}
		}
		pScrollBar->SetScrollPos(nCnt);
		break;
	case SB_LINEDOWN:
	case SB_PAGEDOWN:
		nCnt = nSBCode==SB_LINEDOWN ? 3 : 9;
		if ( nPosNow+3 >= m_aInfo.GetSize() )
			break;
		if ( nSBCode==SB_PAGEDOWN && nPosNow+nCnt>=m_aInfo.GetSize() ) {
			if ( m_aInfo.GetSize() % 3 == 0 )
				nCnt = m_aInfo.GetSize() - 3 - nPosNow;
			else
				nCnt = m_aInfo.GetSize()/3*3 - nPosNow;
		}
		for ( i=nPosNow; i<nPosNow+nCnt && i<m_aInfo.GetSize(); i++ ) {
			if ( m_aInfo[i]->pView ) {
				m_aInfo[i]->pView->ShowWindow(SW_HIDE);
				m_aInfo[i]->pView->SetParent(&m_ctParentView);
			}
		}
		nPosNow = i;
		for ( j=0; i<m_aInfo.GetSize() && j<SIZEOF(m_ctChild); i++, j++ ) {
			if ( m_aInfo[i]->pView ) {
				m_ctChild[j].GetClientRect(&rc);
				m_aInfo[i]->pView->SetParent(&m_ctChild[j]);
				m_aInfo[i]->pView->MoveWindow(0, 0, rc.Width(), rc.Height());
				m_aInfo[i]->pView->ShowWindow(SW_SHOWNA);
				m_aInfo[i]->pView->PostMessage(WM_USERVIEWFITMSG, 0, 1);
			}
		}
		pScrollBar->SetScrollPos(nPosNow);
		break;
	case SB_THUMBTRACK:
	case SB_THUMBPOSITION:
		nCnt = (int)nPos/3*3;
		if ( nPosNow == nCnt )
			break;
		for ( i=nPosNow; i<nPosNow+9 && i<m_aInfo.GetSize(); i++ ) {
			if ( m_aInfo[i]->pView ) {
				m_aInfo[i]->pView->ShowWindow(SW_HIDE);
				m_aInfo[i]->pView->SetParent(&m_ctParentView);
			}
		}
		for ( i=nCnt, j=0; i<m_aInfo.GetSize() && j<SIZEOF(m_ctChild); i++, j++ ) {
			if ( m_aInfo[i]->pView ) {
				m_ctChild[j].GetClientRect(&rc);
				m_aInfo[i]->pView->SetParent(&m_ctChild[j]);
				m_aInfo[i]->pView->MoveWindow(0, 0, rc.Width(), rc.Height());
				m_aInfo[i]->pView->ShowWindow(SW_SHOWNA);
				m_aInfo[i]->pView->PostMessage(WM_USERVIEWFITMSG, 0, 1);
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
	CDialog::OnLButtonUp(nFlags, point);
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

	// ����޳��Ԃ�ڼ޽�؂ɕۑ�
	WINDOWPLACEMENT		wp;
	wp.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(&wp);	// ����޳�̏��Get
	CString	strRegKey(GetSubTreeRegKey(IDS_REGKEY_WINDOW, IDS_REGKEY_WINDOW_THUMBNAILDLG));
	AfxGetNCVCApp()->SaveWindowState(strRegKey, wp);

	CDialog::OnDestroy();
}

void CThumbnailDlg::OnSelchangeSort()
{
	UpdateData();
	int	i, nPosNow = m_ctScroll.GetScrollPos();
	CRect	rc;

	// ���݂̻�Ȳٕ\�����\����
	for ( i=nPosNow; i<nPosNow+9 && i<m_aInfo.GetSize(); i++ ) {
		if ( m_aInfo[i]->pView ) {
			m_aInfo[i]->pView->ShowWindow(SW_HIDE);
			m_aInfo[i]->pView->SetParent(&m_ctParentView);
		}
	}

	// ��Ȳُ��̕��ёւ�
	SortThumbnailInfo();

	// �ĕ\��
	for ( i=0; i<SIZEOF(m_ctChild) && i<m_aInfo.GetSize(); i++ ) {
		if ( m_aInfo[i]->pView ) {
			m_ctChild[i].GetClientRect(&rc);
			m_aInfo[i]->pView->SetParent(&m_ctChild[i]);
			m_aInfo[i]->pView->MoveWindow(0, 0, rc.Width(), rc.Height());
			m_aInfo[i]->pView->ShowWindow(SW_SHOWNA);
			m_aInfo[i]->pView->PostMessage(WM_USERVIEWFITMSG, 0, 1);
		}
	}

	m_ctScroll.SetScrollPos(0);
	m_ctChild[0].SetFocus();
}

void CThumbnailDlg::OnSelchangePlane()
{
	CWaitCursor		wait;
	WaitEnumDocThread(FALSE);	// �ǂݍ��ݽگ�ނ͒�~�����Ȃ�

	UpdateData();
	m_enPlane = (ENNCVPLANE)m_nPlane;

	int		i, j, nPosNow = m_ctScroll.GetScrollPos();
	DWORD	dwStyle;
	CRect	rc;
	CView*	pView;
	CWnd*	pParent;

	// ���݂��ޭ����޷���Ă���؂藣���Ĕj��
	for ( i=0; i<m_aInfo.GetSize(); i++ ) {
		if ( m_aInfo[i]->pView ) {
			m_aInfo[i]->pDoc->RemoveView(m_aInfo[i]->pView);
			m_aInfo[i]->pView->DestroyWindow();
		}
	}

	// �I�����ꂽ�ޭ����č\�z

	// 0 -> nPosNow-1
	rc.SetRectEmpty();
	dwStyle = AFX_WS_DEFAULT_VIEW & ~WS_VISIBLE;
	pParent = &m_ctParentView;
	for ( i=0; i<nPosNow && i<m_aInfo.GetSize(); i++ ) {
		pView = CreatePlaneView();
		if ( pView ) {
			if ( pView->Create(NULL, NULL, dwStyle, rc, pParent, AFX_IDW_PANE_FIRST) ) {
				m_aInfo[i]->pDoc->AddView(pView);
				pView->OnInitialUpdate();
			}
			else {
				pView->DestroyWindow();
				pView = NULL;
			}
		}
		m_aInfo[i]->pView = pView;
	}
	// nPosNow -> SIZEOF(m_ctChild)-1
	dwStyle = AFX_WS_DEFAULT_VIEW;
	for ( j=0; i<m_aInfo.GetSize() && j<SIZEOF(m_ctChild); i++, j++ ) {
		pView = CreatePlaneView();
		if ( pView ) {
			m_ctChild[j].GetClientRect(&rc);
			pParent = &m_ctChild[j];
			if ( pView->Create(NULL, NULL, dwStyle, rc, pParent, AFX_IDW_PANE_FIRST) ) {
				m_aInfo[i]->pDoc->AddView(pView);
				pView->OnInitialUpdate();
			}
			else {
				pView->DestroyWindow();
				pView = NULL;
			}
		}
		pView->PostMessage(WM_USERVIEWFITMSG, 0, 1);
		m_aInfo[i]->pView = pView;
	}
	// �c��
	rc.SetRectEmpty();
	dwStyle = AFX_WS_DEFAULT_VIEW & ~WS_VISIBLE;
	pParent = &m_ctParentView;
	for ( ; i<m_aInfo.GetSize(); i++ ) {
		pView = CreatePlaneView();
		if ( pView ) {
			if ( pView->Create(NULL, NULL, dwStyle, rc, pParent, AFX_IDW_PANE_FIRST) ) {
				m_aInfo[i]->pDoc->AddView(pView);
				pView->OnInitialUpdate();
			}
			else {
				pView->DestroyWindow();
				pView = NULL;
			}
		}
		m_aInfo[i]->pView = pView;
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
// CThumbnailStatic ���b�Z�[�W �n���h��

void CThumbnailStatic::OnLButtonDown(UINT nFlags, CPoint point) 
{
	// ���͂�̫������擾���邱�Ƃ�ϳ�����Ă��e�޲�۸ނɒʒm
	SetFocus();
}

LRESULT CThumbnailStatic::OnUserDblClk(WPARAM wParam, LPARAM lParam)
{
	// CStatic(m_ctParentView) -> CThumbnailDlg
	GetParent()->GetParent()->SendMessage(WM_USERFINISH, wParam, lParam);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CThumbnailDlg �T�u�X���b�h

UINT CThumbnailDlg::CreateEnumDoc_Thread(LPVOID pVoid)
{
	extern	int		g_nProcesser;		// ��۾����(->�����گ�ސ�)
	CThumbnailDlg*	pParent = reinterpret_cast<CThumbnailDlg*>(pVoid);
	int		i, n, nCnt;
	DWORD	dwResult;
	CWinThread**	pThread = new CWinThread*[g_nProcesser];
	HANDLE*			pHandle = new HANDLE[g_nProcesser];
	LPENUMDOCTHREADPARAM	pParam;

	// CPU�̐������گ�ދN��
	for ( i=0; i<g_nProcesser && i<pParent->m_aInfo.GetSize() && pParent->m_bEnumDoc; i++ ) {
		pParam = new ENUMDOCTHREADPARAM;
		pParam->pDoc	= pParent->m_aInfo[i]->pDoc;
		pParam->pView	= pParent->m_aInfo[i]->pView;
		pParam->strFile	= pParent->m_aInfo[i]->fStatus.m_szFullName;
		pThread[i] = AfxBeginThread(EnumDocument_Thread, pParam,
						THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
		if ( pThread[i] ) {
			pThread[i]->m_bAutoDelete = FALSE;
			pHandle[i] = pThread[i]->m_hThread;
			pThread[i]->ResumeThread();
		}
	}
	nCnt = i;	// �L���Ƚگ������ٶ���

	// �I���҂����Ďc���̧�ق𒀈ꏈ��
	for ( ; i<pParent->m_aInfo.GetSize() && pParent->m_bEnumDoc; i++ ) {
		// �ǂꂩ�P�ł�گ�ނ��I��
		dwResult = ::WaitForMultipleObjects(nCnt, pHandle, FALSE, INFINITE);
		// �Y���گ�ނ�CWinThread*���폜
		n = dwResult - WAIT_OBJECT_0;
		if ( n < 0 || nCnt <= n ) {
#ifdef _DEBUG
			NC_FormatMessage();
#endif
			break;
		}
		delete	pThread[n];
		// �V���Ƚگ�ނ��N��
		pParam = new ENUMDOCTHREADPARAM;
		pParam->pDoc	= pParent->m_aInfo[i]->pDoc;
		pParam->pView	= pParent->m_aInfo[i]->pView;
		pParam->strFile	= pParent->m_aInfo[i]->fStatus.m_szFullName;
		pThread[n] = AfxBeginThread(EnumDocument_Thread, pParam,
						THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
		if ( pThread[n] ) {
			pThread[n]->m_bAutoDelete = FALSE;
			pHandle[n] = pThread[n]->m_hThread;
			pThread[n]->ResumeThread();
		}
	}

	// �S�Ă̽گ�ޏI���҂�
	::WaitForMultipleObjects(nCnt, pHandle, TRUE, INFINITE);
	for ( i=0; i<nCnt; i++ )
		delete	pThread[i];

	delete	pThread;
	delete	pHandle;

	return 0;
}

UINT EnumDocument_Thread(LPVOID lpVoid)
{
#ifdef _DEBUG
	CMagaDbg	dbg("EnumDocument_Thread()\nStart", TRUE, DBG_RED);
#endif
	LPENUMDOCTHREADPARAM pParam = reinterpret_cast<LPENUMDOCTHREADPARAM>(lpVoid);
	CNCDoc*		pDoc  = pParam->pDoc;
	CView*		pView = pParam->pView;
	CString	strFile(pParam->strFile);
	delete	pParam;

	// ̧�ق��J�������A�̓����
	pDoc->ReadThumbnail(strFile);
	// �ޭ��ւ̒ʒm
	if ( pView ) {
		pView->OnInitialUpdate();
		pView->PostMessage(WM_USERVIEWFITMSG, 0, 1);
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// ���בւ��⏕�֐�

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
