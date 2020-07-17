// DocBase.inl: CDocBase クラスの実装ファイル
//		templateクラスの実装はインクルードファイルに！！
//////////////////////////////////////////////////////////////////////

#pragma once

template<size_t T>
BOOL CDocBase<T>::OnOpenDocumentSP(LPCTSTR lpstrFileName, CFrameWnd* pWnd)
{
	ASSERT( pWnd );
	m_pFileChangeThread = NULL;

	try {
		// ﾌｧｲﾙ監視ｽﾚｯﾄﾞ起動
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

template<size_t T>
void CDocBase<T>::OnCloseDocumentSP(void)
{
	if ( !m_pFileChangeThread )
		return;

	m_evFinish.SetEvent();
#ifdef _DEBUG
	if ( ::WaitForSingleObject(m_pFileChangeThread->m_hThread, INFINITE) == WAIT_FAILED ) {
		g_dbg.printf("OnCloseDocumentSP()　WaitForSingleObject() Fail!");
		::NC_FormatMessage();
	}
	else
		g_dbg.printf("OnCloseDocumentSP() WaitForSingleObject() OK");
#else
	::WaitForSingleObject(m_pFileChangeThread->m_hThread, INFINITE);
#endif
	delete	m_pFileChangeThread;
}

template<size_t T>
BOOL CDocBase<T>::UpdateModifiedTitle(BOOL bModified, CString& strTitle)
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

template<size_t T>
BOOL CDocBase<T>::IsLockThread(void)
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
