// DXFDoc.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "DocBase.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#ifdef USE_KODATUNO
#include "Kodatuno/BODY.h"
#include "Kodatuno/IGES_Parser.h"
#include "Kodatuno/STL_Parser.h"
#undef PI	// Use NCVC (MyTemplate.h)
#endif

/////////////////////////////////////////////////////////////////////////////

void CDocBase::ReportSaveLoadException
	(LPCTSTR lpszPathName, CException* e, BOOL bSaving, UINT nIDPDefault)
{
	if ( e->IsKindOf(RUNTIME_CLASS(CUserException)) ) {
		AfxGetNCVCMainWnd()->GetProgressCtrl()->SetPos(0);
		return;	// �W���װү���ނ��o���Ȃ�
	}
	__super::ReportSaveLoadException(lpszPathName, e, bSaving, nIDPDefault);
}

void CDocBase::SetModifiedFlag(BOOL bModified)
{
	CString	strTitle( GetTitle() );
	if ( UpdateModifiedTitle(bModified, strTitle) )
		SetTitle(strTitle);
	__super::SetModifiedFlag(bModified);
}

/////////////////////////////////////////////////////////////////////////////

BOOL CDocBase::OnOpenDocumentBase(LPCTSTR lpstrFileName)
{
	POSITION	pos = GetFirstViewPosition();
	ASSERT( pos );
	CFrameWnd*	pWnd = GetNextView(pos)->GetParentFrame();
	ASSERT( pWnd );
	m_pFileChangeThread = NULL;

	try {
		// ̧�يĎ��گ�ދN��
		LPFNCNGTHREADPARAM pParam = new FNCNGTHREADPARAM;
		pParam->lpstrFileName = lpstrFileName;
		pParam->hWndFrame	= pWnd->GetSafeHwnd();
		pParam->hFinish		= m_evFinish.m_hObject;
		m_pFileChangeThread = AfxBeginThread(FileChangeNotificationThread, pParam,
									THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
		if ( m_pFileChangeThread ) {
			m_pFileChangeThread->m_bAutoDelete = FALSE;
			m_pFileChangeThread->ResumeThread();
		}
		else
			::NCVC_CriticalErrorMsg(__FILE__, __LINE__);	// ExitProcess()
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		return FALSE;
	}

	return TRUE;
}

void CDocBase::OnCloseDocumentBase(void)
{
	if ( !m_pFileChangeThread )
		return;

	m_evFinish.SetEvent();
#ifdef _DEBUG
	if ( ::WaitForSingleObject(m_pFileChangeThread->m_hThread, INFINITE) == WAIT_FAILED ) {
		printf("OnCloseDocumentBase()�@WaitForSingleObject() Fail!\n");
		::NC_FormatMessage();
	}
	else
		printf("OnCloseDocumentBase() WaitForSingleObject() OK\n");
#else
	::WaitForSingleObject(m_pFileChangeThread->m_hThread, INFINITE);
#endif
	delete	m_pFileChangeThread;
}

BOOL CDocBase::UpdateModifiedTitle(BOOL bModified, CString& strTitle)
{
	int		nLength = strTitle.GetLength();
	if ( nLength <= 0 )
		return FALSE;

	TCHAR	szModified = strTitle.GetAt(nLength-1);
	BOOL	bUpdate = FALSE;
	
	// �X�Vϰ��t�^
	if ( bModified ) {
		if ( szModified != '*' ) {
			strTitle += '*';
			bUpdate = TRUE;
		}
	}
	else {
		if ( szModified == '*' ) {
			strTitle = strTitle.Left(nLength-1);
			bUpdate = TRUE;
		}
	}

	return bUpdate;
}

BOOL CDocBase::IsLockThread(void)
{
	if ( m_hAddinThread ) {
		// �ꉞ3�b�����҂�
		DWORD	dwResult = ::WaitForSingleObject(m_hAddinThread, 3000);
		switch ( dwResult ) {
		case WAIT_TIMEOUT:
			if ( AfxMessageBox(IDS_ANA_LOCKADDIN, MB_YESNO|MB_ICONQUESTION) != IDYES )
				return FALSE;
			break;
#ifdef _DEBUG
		case WAIT_FAILED:
			printf("CDocBase::IsLockThread() WaitForSingleObject() Fail!\n");
			::NC_FormatMessage();
			break;
#endif
		}
	}
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//	̧�ٕύX�ʒm�̊Ď��گ��
// �Е�������������ق̂��߁CCMultiLock ���g���Ȃ�
// �װү���ނͳ�޲�̂ŏo�͂��Ȃ�

UINT FileChangeNotificationThread(LPVOID pParam)
{
	HANDLE	hEvent[2];		// 0:�޷���ĸ׽�̏I���ʒm����������
							// 1:̧�ٕύX�ʒm�����

	LPFNCNGTHREADPARAM pThreadParam = reinterpret_cast<LPFNCNGTHREADPARAM>(pParam);
	CString	strFileName(pThreadParam->lpstrFileName), strPath, strFile;
	HWND	hWndFrame = pThreadParam->hWndFrame;
	hEvent[0] = pThreadParam->hFinish;
	delete	pThreadParam;

	// ̧�ق��߽���擾
	Path_Name_From_FullPath(strFileName, strPath, strFile);

	// ̧�ق̌��ݓ������擾
	CFileStatus		fStatus;
	CTime			fLastTime;
	ULONGLONG		llSize;
	if ( !CFile::GetStatus(strFileName, fStatus) ) {
#ifdef _DEBUG
		printf("FCN-Thread GetStatus() Error (TimeStamp) \"%s\"\n", LPCTSTR(strFile));
#endif
		return 0;
	}
	fLastTime = fStatus.m_mtime;	// ̧�ٍŏI�ύX����
	llSize    = fStatus.m_size;		// ̧�ٻ���
	// �ύX�ʒm���
	hEvent[1] = FindFirstChangeNotification(strPath, FALSE,
		FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME);
	if ( hEvent[1] == INVALID_HANDLE_VALUE ) {
#ifdef _DEBUG
		printf("FindFirstChangeNotification() Error \"%s\"\n", LPCTSTR(strPath));
		NC_FormatMessage();
#endif
		return 0;
	}

#ifdef _DEBUG
	printf("FCN-Thread Start! \"%s\"\n", LPCTSTR(strFile));
#endif
	BOOL	bResult = TRUE;
	DWORD	dwResult;
	// �Ď�����ٰ��
	while ( bResult ) {
		dwResult = WaitForMultipleObjects(SIZEOF(hEvent), hEvent, FALSE, INFINITE);
		switch ( dwResult - WAIT_OBJECT_0 ) {
		case 0:		// �ޭ��׽�I���ʒm�����
			bResult = FALSE;
			break;

		case 1:		// ̧�ٕύX�ʒm
			if ( CFile::GetStatus(strFileName, fStatus) ) {
				if ( fLastTime!=fStatus.m_mtime || llSize!=fStatus.m_size ) {
#ifdef _DEBUG
					printf("FCN-Thread Change! \"%s\"\n", LPCTSTR(strFile));
#endif
					fLastTime = fStatus.m_mtime;
					llSize    = fStatus.m_size;
					PostMessage(hWndFrame, WM_USERFILECHANGENOTIFY, NULL, NULL);
				}
			}
#ifdef _DEBUG
			else
				printf("FCN-Thread GetStatus() Error \"%s\"\n", LPCTSTR(strFile));
#endif
			if ( FindNextChangeNotification(hEvent[1]) )
				break;

#ifdef _DEBUG
			printf("FindNextChangeNotification() Error \"%s\"\n", LPCTSTR(strPath));
#endif
			// through;

		default:
#ifdef _DEBUG
			NC_FormatMessage();
#endif
			// ��x����ق�۰�ނ��č����ݼ�
			if ( FindCloseChangeNotification(hEvent[1]) ) {
				hEvent[1] = FindFirstChangeNotification(strPath, FALSE,
					FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME);
				if ( hEvent[1] != INVALID_HANDLE_VALUE )
					break;		// �����ݼސ����I
#ifdef _DEBUG
				else {
					printf("FindFirstChangeNotification() Retry Error \"%s\"\n", LPCTSTR(strPath));
					NC_FormatMessage();
				}
#endif
			}
			bResult = FALSE;
		}
	}

	if ( hEvent[1] != INVALID_HANDLE_VALUE )
		FindCloseChangeNotification(hEvent[1]);
#ifdef _DEBUG
	printf("FCN-Thread End Thread \"%s\"\n", LPCTSTR(strFile));
#endif

	return 0;
}

/////////////////////////////////////////////////////////////////////////////

#ifdef USE_KODATUNO
BODY*	Read3dModel(LPCTSTR lpszFile, LPCTSTR lpszCurrent/*=NULL*/)
{
	extern	LPCTSTR	gg_szEn;	// "\\";
	CString	strPath, strName, strExt;
	TCHAR	szFileName[_MAX_FNAME],
			szExt[_MAX_EXT];

	// ̧�ٖ��̌���
	if ( ::PathIsRelative(lpszFile) ) {
		CString	strCurrent;
		TCHAR	pBuf[MAX_PATH];
		if ( lpszCurrent ) {
			::Path_Name_From_FullPath(lpszCurrent, strCurrent, strName);
			strCurrent += lpszFile;
		}
		else {
			::GetCurrentDirectory(MAX_PATH, pBuf);
			strCurrent = pBuf;
			strCurrent += '\\';
			strCurrent += lpszFile;
		}
		if ( ::PathCanonicalize(pBuf, strCurrent) )
			strPath = pBuf;
	}
	else {
		strPath = lpszFile;
	}
	_tsplitpath_s(strPath, NULL, 0, NULL, 0,
		szFileName, SIZEOF(szFileName), szExt, SIZEOF(szExt));
	if ( lstrlen(szFileName)<=0 || lstrlen(szExt)<=0 )
		return NULL;
	strExt = szExt + 1;		// �h�b�g������

	BODY*	pBody = new BODY;

	// �g���q�Ŕ���
	int	nResult = KOD_FALSE;
	if ( strExt.CompareNoCase("igs")==0 || strExt.CompareNoCase("iges")==0 ) {
		IGES_PARSER	iges;
		if ( (nResult=iges.IGES_Parser_Main(pBody, strPath)) == KOD_TRUE )
			iges.Optimize4OpenGL(pBody);
	}
	else if ( strExt.CompareNoCase("stl") == 0 ) {
		STL_PARSER	stl;
		nResult = stl.STL_Parser_Main(pBody, strPath);
	}
	if ( nResult != KOD_TRUE ) {
		delete	pBody;
		return NULL;
	}

	return pBody;
}
#endif
