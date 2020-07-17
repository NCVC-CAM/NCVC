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

//	̧�ٕύX�ʒm�̊Ď��گ��
static UINT FileChangeNotificationThread(LPVOID pParam);

/////////////////////////////////////////////////////////////////////////////
// CDocBase

BOOL CDocBase::OnOpenDocumentSP(LPCTSTR lpstrFileName, CFrameWnd* pWnd)
{
	ASSERT( pWnd );
	m_pFileChangeThread = NULL;

	try {
		// ̧�يĎ��گ�ދN��
		LPFNCNGTHREADPARAM pParam = new FNCNGTHREADPARAM;
		pParam->lpstrFileName = lpstrFileName;
		pParam->hWndFrame	= pWnd->GetSafeHwnd();
		pParam->hFinish		= HANDLE(m_evFinish);
		m_pFileChangeThread = AfxBeginThread(FileChangeNotificationThread, pParam,
									THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
		if ( m_pFileChangeThread ) {
			m_pFileChangeThread->m_bAutoDelete = FALSE;
			m_pFileChangeThread->ResumeThread();
		}
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		return FALSE;
	}

	return TRUE;
}

void CDocBase::OnCloseDocumentSP(void)
{
	if ( !m_pFileChangeThread )
		return;

	m_evFinish.SetEvent();
#ifdef _DEBUG
	CMagaDbg	dbg("CDocBase::OnCloseDocument()", DBG_BLUE);
	if ( ::WaitForSingleObject(m_pFileChangeThread->m_hThread, INFINITE) == WAIT_FAILED ) {
		dbg.printf("WaitForSingleObject() Fail!");
		::NC_FormatMessage();
	}
	else
		dbg.printf("WaitForSingleObject() OK");
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
			g_dbg.printf("CDocBase::IsLockThread() WaitForSingleObject() Fail!");
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
