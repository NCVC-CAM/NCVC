// DXFDoc.cpp : �C���v�������e�[�V���� �t�@�C��
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
//	̧�ٕύX�ʒm�̊Ď��گ��
// �Е�������������ق̂��߁CCMultiLock ���g���Ȃ�
// �װү���ނͳ�޲�̂ŏo�͂��Ȃ�

UINT FileChangeNotificationThread(LPVOID pParam)
{
#ifdef _DEBUG
	CMagaDbg	dbg("FileChangeNotifyThread()", DBG_RED);
#endif
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
		dbg.printf("GetStatus() Error (TimeStamp) \"%s\"", strFile);
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
			// ��x����ق�۰�ނ��č����ݼ�
			if ( FindCloseChangeNotification(hEvent[1]) ) {
				hEvent[1] = FindFirstChangeNotification(strPath, FALSE,
					FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME);
				if ( hEvent[1] != INVALID_HANDLE_VALUE )
					break;		// �����ݼސ����I
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
