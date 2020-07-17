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

/////////////////////////////////////////////////////////////////////////////
//	ﾌｧｲﾙ変更通知の監視ｽﾚｯﾄﾞ
// 片方が純粋なﾊﾝﾄﾞﾙのため，CMultiLock が使えない
// ｴﾗｰﾒｯｾｰｼﾞはｳｻﾞｲので出力しない

UINT FileChangeNotificationThread(LPVOID pParam)
{
#ifdef _DEBUG
	CMagaDbg	dbg("FileChangeNotifyThread()", DBG_RED);
#endif
	HANDLE	hEvent[2];		// 0:ﾄﾞｷｭﾒﾝﾄｸﾗｽの終了通知ｲﾍﾞﾝﾄﾊﾝﾄﾞﾙ
							// 1:ﾌｧｲﾙ変更通知ﾊﾝﾄﾞﾙ

	LPFNCNGTHREADPARAM pThreadParam = reinterpret_cast<LPFNCNGTHREADPARAM>(pParam);
	CString	strFileName(pThreadParam->lpstrFileName), strPath, strFile;
	HWND	hWndFrame = pThreadParam->hWndFrame;
	hEvent[0] = pThreadParam->hFinish;
	delete	pThreadParam;

	// ﾌｧｲﾙのﾊﾟｽ名取得
	Path_Name_From_FullPath(strFileName, strPath, strFile);

	// ﾌｧｲﾙの現在日時を取得
	CFileStatus		fStatus;
	CTime			fLastTime;
	ULONGLONG		llSize;
	if ( !CFile::GetStatus(strFileName, fStatus) ) {
#ifdef _DEBUG
		dbg.printf("GetStatus() Error (TimeStamp) \"%s\"", strFile);
#endif
		return 0;
	}
	fLastTime = fStatus.m_mtime;	// ﾌｧｲﾙ最終変更日時
	llSize    = fStatus.m_size;		// ﾌｧｲﾙｻｲｽﾞ
	// 変更通知ｾｯﾄ
	hEvent[1] = FindFirstChangeNotification(strPath, FALSE,
		FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME);
	if ( hEvent[1] == INVALID_HANDLE_VALUE ) {
#ifdef _DEBUG
		dbg.printf("FindFirstChangeNotification() Error \"%s\"", strPath);
		NC_FormatMessage();
#endif
		return 0;
	}

#ifdef _DEBUG
	dbg.printf("Start! \"%s\"", strFile);
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
					dbg.printf("Change! \"%s\"", strFile);
#endif
					fLastTime = fStatus.m_mtime;
					llSize    = fStatus.m_size;
					PostMessage(hWndFrame, WM_USERFILECHANGENOTIFY, NULL, NULL);
				}
			}
#ifdef _DEBUG
			else
				dbg.printf("GetStatus() Error \"%s\"", strFile);
#endif
			if ( FindNextChangeNotification(hEvent[1]) )
				break;

#ifdef _DEBUG
			dbg.printf("FindNextChangeNotification() Error \"%s\"", strPath);
#endif
			// through;

		default:
#ifdef _DEBUG
			NC_FormatMessage();
#endif
			// 一度ﾊﾝﾄﾞﾙをｸﾛｰｽﾞして再ﾁｬﾚﾝｼﾞ
			if ( FindCloseChangeNotification(hEvent[1]) ) {
				hEvent[1] = FindFirstChangeNotification(strPath, FALSE,
					FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME);
				if ( hEvent[1] != INVALID_HANDLE_VALUE )
					break;		// 再ﾁｬﾚﾝｼﾞ成功！
#ifdef _DEBUG
				else {
					dbg.printf("FindFirstChangeNotification() Retry Error \"%s\"", strPath);
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
	dbg.printf("End Thread \"%s\"", strFile);
#endif

	return 0;
}
