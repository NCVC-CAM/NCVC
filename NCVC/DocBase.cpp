// DXFDoc.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "DocBase.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

//	ﾌｧｲﾙ変更通知の監視ｽﾚｯﾄﾞ
static UINT FileChangeNotificationThread(LPVOID pParam);

/////////////////////////////////////////////////////////////////////////////
// CDocBase

BOOL CDocBase::OnOpenDocument(LPCTSTR lpstrFileName, CFrameWnd* pWnd)
{
	ASSERT( pWnd );
	m_hFileChangeThread = NULL;

	try {
		// ﾌｧｲﾙ監視ｽﾚｯﾄﾞ起動
		LPFNCNGTHREADPARAM pParam = new FNCNGTHREADPARAM;
		pParam->lpstrFileName = lpstrFileName;
		pParam->hWndFrame	= pWnd->GetSafeHwnd();
		pParam->hFinish		= HANDLE(m_evFinish);
		CWinThread* pThread	= AfxBeginThread(FileChangeNotificationThread, pParam,
									THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
		m_hFileChangeThread	= ::NCVC_DuplicateHandle(pThread->m_hThread);
		if ( !m_hFileChangeThread )
			::NCVC_CriticalErrorMsg(__FILE__, __LINE__);
		pThread->ResumeThread();
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		return FALSE;
	}

	return TRUE;
}

void CDocBase::OnCloseDocument(void)
{
	if ( !m_hFileChangeThread )
		return;

	m_evFinish.SetEvent();
#ifdef _DEBUG
	CMagaDbg	dbg("CDocBase::OnCloseDocument()", DBG_BLUE);
	if ( ::WaitForSingleObject(m_hFileChangeThread, INFINITE) == WAIT_FAILED ) {
		dbg.printf("WaitForSingleObject() Fail!");
		::NC_FormatMessage();
	}
	else
		dbg.printf("WaitForSingleObject() OK");
	if ( !::CloseHandle(m_hFileChangeThread) ) {
		dbg.printf("CloseHandle() Fail!");
		::NC_FormatMessage();
	}
	else
		dbg.printf("CloseHandle() OK");
#else
	::WaitForSingleObject(m_hFileChangeThread, INFINITE);
	::CloseHandle(m_hFileChangeThread);
#endif
	m_hFileChangeThread = NULL;
}

BOOL CDocBase::UpdateModifiedTitle(BOOL bModified, CString& strTitle)
{
	int		nLength = strTitle.GetLength();
	if ( nLength <= 0 )
		return FALSE;

	TCHAR	szModified = strTitle.GetAt(nLength-1);
	BOOL	bUpdate = FALSE;
	
	// 更新ﾏｰｸ付与
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
		// 一応3秒だけ待つ
		DWORD	dwResult = ::WaitForSingleObject(m_hAddinThread, 3000);
		switch ( dwResult ) {
		case WAIT_TIMEOUT:
			if ( AfxMessageBox(IDS_ANA_LOCKADDIN, MB_YESNO|MB_ICONQUESTION) != IDYES )
				return FALSE;
			break;
#ifdef _DEBUG
		case WAIT_FAILED:
			g_dbg.printf("CDocBase::IsLockThread() WaitForSingleObject() Fail!");
			::NC_FormatMessage();
			break;
#endif
		}
	}
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//	ﾌｧｲﾙ変更通知の監視ｽﾚｯﾄﾞ
// 片方が純粋なﾊﾝﾄﾞﾙのため，CMultiLock が使えない
// ｴﾗｰﾒｯｾｰｼﾞはｳｻﾞｲので出力しない

UINT FileChangeNotificationThread(LPVOID pParam)
{
#ifdef _DEBUG
	CMagaDbg	dbg("FileChangeNotifyThread()", DBG_RED);
	CString		dbg_strFile;
#endif
	HANDLE	hEvent[2];		// 0:ﾄﾞｷｭﾒﾝﾄｸﾗｽの終了通知ｲﾍﾞﾝﾄﾊﾝﾄﾞﾙ
							// 1:ﾌｧｲﾙ変更通知ﾊﾝﾄﾞﾙ

	LPFNCNGTHREADPARAM pThreadParam = (LPFNCNGTHREADPARAM)pParam;
	CString	strFileName(pThreadParam->lpstrFileName);
	HWND	hWndFrame = pThreadParam->hWndFrame;
	hEvent[0] = pThreadParam->hFinish;
	delete	pThreadParam;
#ifdef _DEBUG
	{
		CString	strPath;
		Path_Name_From_FullPath(strFileName, strPath, dbg_strFile);
	}
#endif

	// ﾌｧｲﾙの現在日時を取得
	CFileStatus		fStatus;
	CTime			fLastTime;
	ULONGLONG		llSize;
	if ( !CFile::GetStatus(strFileName, fStatus) ) {
#ifdef _DEBUG
		dbg.printf("GetStatus() Error (TimeStamp) \"%s\"", dbg_strFile);
#endif
		return 0;
	}
	fLastTime = fStatus.m_mtime;	// ﾌｧｲﾙ最終変更日時
	llSize    = fStatus.m_size;		// ﾌｧｲﾙｻｲｽﾞ
	// ﾌｧｲﾙのﾊﾟｽ名取得と変更通知ｾｯﾄ
	{
		CString	strPath, strFile;
		Path_Name_From_FullPath(strFileName, strPath, strFile);
		hEvent[1] = FindFirstChangeNotification(strPath, FALSE,
						FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE);
		if ( hEvent[1] == INVALID_HANDLE_VALUE ) {
#ifdef _DEBUG
			NC_FormatMessage();
			dbg.printf("FindFirstChangeNotification() Error \"%s\"", strPath);
#endif
			return 0;
		}
	}

#ifdef _DEBUG
	dbg.printf("Start! \"%s\"", dbg_strFile);
#endif
	BOOL	bResult = TRUE;
	DWORD	dwResult;
	// 監視無限ﾙｰﾌﾟ
	while ( bResult ) {
		dwResult = WaitForMultipleObjects(SIZEOF(hEvent), hEvent, FALSE, INFINITE);
		switch ( dwResult - WAIT_OBJECT_0 ) {
		case 0:		// ﾋﾞｭｰｸﾗｽ終了通知ｲﾍﾞﾝﾄ
			bResult = FALSE;
			break;

		case 1:		// ﾌｧｲﾙ変更通知
			if ( CFile::GetStatus(strFileName, fStatus) ) {
				if ( fLastTime!=fStatus.m_mtime || llSize!=fStatus.m_size ) {
#ifdef _DEBUG
					dbg.printf("Change! \"%s\"", dbg_strFile);
#endif
					fLastTime = fStatus.m_mtime;
					llSize    = fStatus.m_size;
					PostMessage(hWndFrame, WM_USERFILECHANGENOTIFY, NULL, NULL);
				}
#ifdef _DEBUG
				else
					dbg.printf("File does not changed. \"%s\"", dbg_strFile);
#endif
			}
#ifdef _DEBUG
			else
				dbg.printf("GetStatus() Error \"%s\"", dbg_strFile);
#endif
			if ( !FindNextChangeNotification(hEvent[1]) ) {
#ifdef _DEBUG
				NC_FormatMessage();
#endif
				bResult = FALSE;
			}
			break;

		default:
#ifdef _DEBUG
			NC_FormatMessage();
#endif
			bResult = FALSE;
		}
	}

	FindCloseChangeNotification(hEvent[1]);
#ifdef _DEBUG
	dbg.printf("End Thread \"%s\"", dbg_strFile);
#endif
	return 0;
}
