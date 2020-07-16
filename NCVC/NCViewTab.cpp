// NCViewTab.cpp: CNCViewTab クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "NCChild.h"
#include "NCdata.h"
#include "NCDoc.h"
#include "NCViewTab.h"
#include "NCViewSplit.h"
#include "NCView.h"
#include "NCViewXY.h"
#include "NCViewXZ.h"
#include "NCViewYZ.h"
#include "NCListView.h"
#include "ViewOption.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

IMPLEMENT_DYNCREATE(CNCViewTab, CTabView)

BEGIN_MESSAGE_MAP(CNCViewTab, CTabView)
	ON_WM_CREATE()
	ON_WM_DESTROY()
//	ON_WM_ERASEBKGND()
	ON_WM_SETFOCUS()
	// ﾀﾌﾞ移動
	ON_COMMAND_RANGE(ID_TAB_NEXT, ID_TAB_PREV, OnMoveTab)
	// ﾄﾚｰｽ
	ON_UPDATE_COMMAND_UI_RANGE(ID_NCVIEW_TRACE_FAST, ID_NCVIEW_TRACE_LOW, OnUpdateTraceSpeed)
	ON_COMMAND_RANGE(ID_NCVIEW_TRACE_FAST, ID_NCVIEW_TRACE_LOW, OnTraceSpeed)
	ON_UPDATE_COMMAND_UI(ID_NCVIEW_TRACE_RUN, OnUpdateTraceRun)
	ON_COMMAND(ID_NCVIEW_TRACE_RUN, OnTraceRun)
	ON_UPDATE_COMMAND_UI(ID_NCVIEW_TRACE_PAUSE, OnUpdateTracePause)
	ON_COMMAND(ID_NCVIEW_TRACE_PAUSE, OnTracePause)
	ON_COMMAND(ID_NCVIEW_TRACE_STOP, OnTraceStop)
	ON_UPDATE_COMMAND_UI_RANGE(ID_NCVIEW_TRACE_CURSOR, ID_NCVIEW_TRACE_CURSOR2, OnUpdateTraceCursor)
	ON_COMMAND_RANGE(ID_NCVIEW_TRACE_CURSOR, ID_NCVIEW_TRACE_CURSOR2, OnTraceCursor)
	// 「全てのﾍﾟｲﾝの図形ﾌｨｯﾄ」ﾒﾆｭｰｺﾏﾝﾄﾞの使用許可
	ON_UPDATE_COMMAND_UI(ID_NCVIEW_ALLFIT, OnUpdateAllFitCmd)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////

CNCViewTab::CNCViewTab() : m_evTrace(FALSE, TRUE)
{
/*
	ここでﾄﾚｰｽｽﾋﾟｰﾄﾞをﾚｼﾞｽﾄﾘから取得すると，NCViewTab起動の度にｱｸｾｽされることになる
	以下，ｱｸﾃｨﾌﾞﾍﾟｰｼﾞ情報も同じ
*/
	m_nTraceSpeed = AfxGetNCVCApp()->GetTraceSpeed();
	m_nTrace = ID_NCVIEW_TRACE_STOP;
	m_hTrace = NULL;
	m_bTraceContinue = m_bTracePause = FALSE;
	for ( int i=0; i<SIZEOF(m_bSplit); i++ )
		m_bSplit[i] = FALSE;
}

CNCViewTab::~CNCViewTab()
{
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewTab クラスのオーバライド関数

void CNCViewTab::OnInitialUpdate() 
{
	CTabView::OnInitialUpdate();

	// ｱｸﾃｨﾌﾞﾍﾟｰｼﾞ情報
	int	nPage = AfxGetNCVCApp()->GetNCTabPage();
	ActivatePage(nPage);

	// CNCViewSplitのｻｲｽﾞ調整
	// 現在ｱｸﾃｨﾌﾞﾍﾟｰｼﾞと等しいときは，さらに各ﾋﾞｭｰWM_USERVIEWFITMSG送信
	m_wndSplitter1.PostMessage(WM_USERINITIALUPDATE, NC_SINGLEPANE,   nPage==NC_SINGLEPANE);
	m_wndSplitter2.PostMessage(WM_USERINITIALUPDATE, NC_SINGLEPANE+1, nPage==NC_SINGLEPANE+1);

	if ( nPage >= NC_SINGLEPANE ) {
		// ｽﾌﾟﾘｯﾀ表示の場合は，拡大率の再更新
		for ( int i=0; i<SIZEOF(m_bSplit); i++ )
			m_bSplit[i] = TRUE;
	}
	else {
		// 図形ﾌｨｯﾄﾒｯｾｰｼﾞの送信
		// 各ﾋﾞｭｰのOnInitialUpdate()関数内では，GetClientRect()のｻｲｽﾞが正しくない
		GetPage(nPage)->PostMessage(WM_USERVIEWFITMSG, 0, 1);
	}
}

void CNCViewTab::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	switch ( lHint ) {
	case UAV_STARTSTOPTRACE:
	case UAV_TRACECURSOR:
	case UAV_FILEINSERT:
	case UAV_DRAWWORKRECT:
	case UAV_DRAWMAXRECT:
		return;		// 再描画不要
	case UAV_CHANGEFONT:
		GetTabCtrl().SetFont(AfxGetNCVCMainWnd()->GetTextFont(TYPE_NCD), FALSE);
		break;
	}
	CTabView::OnUpdate(pSender, lHint, pHint);
}

BOOL CNCViewTab::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
	// ﾀﾌﾞ移動だけ特別
	if ( nID == ID_TAB_NEXT || nID == ID_TAB_PREV ) {
		CWnd*	pWnd = GetFocus();
		if ( pWnd->IsKindOf(RUNTIME_CLASS(CNCViewTab)) ||
				pWnd->IsKindOf(RUNTIME_CLASS(CNCView)) ||
				pWnd->IsKindOf(RUNTIME_CLASS(CNCViewXY)) ||
				pWnd->IsKindOf(RUNTIME_CLASS(CNCViewXZ)) ||
				pWnd->IsKindOf(RUNTIME_CLASS(CNCViewYZ)) )
			return CTabView::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
		else
			return FALSE;
	}

	// 自分自身とｱｸﾃｨﾌﾞなﾋﾞｭｰだけにｺﾏﾝﾄﾞﾙｰﾃｨﾝｸﾞ
	// 結果的に CNCDoc へのｺﾏﾝﾄﾞﾙｰﾃｨﾝｸﾞはここからだけになる
	if ( CTabView::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo) )
		return TRUE;
	if ( GetPageCount() <= 0 )
		return FALSE;
	return GetActivePageWnd()->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

void CNCViewTab::OnActivatePage(int nIndex)
{
	GetPage(nIndex)->SendMessage(WM_USERACTIVATEPAGE,
						(WPARAM)nIndex, (LPARAM)m_bSplit[nIndex]);
	if ( nIndex >= NC_SINGLEPANE ) {
		// ｽﾌﾟﾘｯﾀ表示の場合は，拡大率の再更新
		for ( int i=0; i<SIZEOF(m_bSplit); i++ )
			m_bSplit[i] = TRUE;
		// ｱｸﾃｨﾌﾞﾋﾞｭｰはｽﾌﾟﾘｯﾀ内のSetActivePane()で
	}
	else {
		m_bSplit[nIndex] = FALSE;
		GetParentFrame()->SetActiveView((CView *)GetPage(nIndex));
	}
	AfxGetNCVCApp()->SetNCTabPage(nIndex);
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewTab クラスの診断

#ifdef _DEBUG
void CNCViewTab::AssertValid() const
{
	CTabView::AssertValid();
}

void CNCViewTab::Dump(CDumpContext& dc) const
{
	CTabView::Dump(dc);
}

CNCDoc* CNCViewTab::GetDocument() // 非デバッグ バージョンはインラインです。
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CNCDoc)));
	return (CNCDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CNCViewTab クラスのメッセージ ハンドラ

int CNCViewTab::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	extern	LPCTSTR	g_szNdelimiter;		// "XYZRIJKPLDH" from NCDoc.cpp

	if (CTabView::OnCreate(lpCreateStruct) == -1)
		return -1;
	GetTabCtrl().SetFont(AfxGetNCVCMainWnd()->GetTextFont(TYPE_NCD), FALSE);

	// thisﾎﾟｲﾝﾀのｾｯﾄのみ
	CViewBase::OnCreate(this, FALSE);

	//	静的ｽﾌﾟﾘｯﾀ作成
	CCreateContext	ct;
	ct.m_pCurrentDoc = GetDocument();
	ct.m_pCurrentFrame = GetParentFrame();
	ct.m_pLastView = NULL;
	ct.m_pNewDocTemplate = GetDocument()->GetDocTemplate();
	ct.m_pNewViewClass = NULL;
	// ４面-1
	if ( !m_wndSplitter1.CreateStatic(this, 2, 2, WS_CHILD) ||
		 !m_wndSplitter1.CreateView(0, 0, RUNTIME_CLASS(CNCView),   CSize(0, 0), &ct) ||
		 !m_wndSplitter1.CreateView(0, 1, RUNTIME_CLASS(CNCViewYZ), CSize(0, 0), &ct) ||
		 !m_wndSplitter1.CreateView(1, 0, RUNTIME_CLASS(CNCViewXZ), CSize(0, 0), &ct) ||
		 !m_wndSplitter1.CreateView(1, 1, RUNTIME_CLASS(CNCViewXY), CSize(0, 0), &ct) )
		return -1;
	// ４面-2
	if ( !m_wndSplitter2.CreateStatic(this, 1, 2, WS_CHILD) ||
		 !m_wndSplitter22.CreateStatic(&m_wndSplitter2, 3, 1,
							WS_CHILD|WS_VISIBLE, m_wndSplitter2.IdFromRowCol(0, 0)) ||
		 !m_wndSplitter22.CreateView(0, 0, RUNTIME_CLASS(CNCViewYZ), CSize(0, 0), &ct) ||
		 !m_wndSplitter22.CreateView(1, 0, RUNTIME_CLASS(CNCViewXZ), CSize(0, 0), &ct) ||
		 !m_wndSplitter22.CreateView(2, 0, RUNTIME_CLASS(CNCViewXY), CSize(0, 0), &ct) ||
		 !m_wndSplitter2.CreateView(0, 1, RUNTIME_CLASS(CNCView),   CSize(0, 0), &ct) )
		return -1;

	// 各ﾍﾟｰｼﾞﾋﾞｭｰの生成
	int		i, nIndex;
	CString	strXYZ[NCXYZ];
	for ( i=0; i<NCXYZ; i++ )
		strXYZ[i] = g_szNdelimiter[i];
	try {
		nIndex = AddPage(strXYZ[NCA_X]+strXYZ[NCA_Y]+strXYZ[NCA_Z],
			RUNTIME_CLASS(CNCView), GetDocument(), GetParentFrame());
		ASSERT(nIndex != -1);
		nIndex = AddPage(strXYZ[NCA_X]+strXYZ[NCA_Y],
			RUNTIME_CLASS(CNCViewXY), GetDocument(), GetParentFrame());
		ASSERT(nIndex != -1);
		nIndex = AddPage(strXYZ[NCA_X]+strXYZ[NCA_Z],
			RUNTIME_CLASS(CNCViewXZ), GetDocument(), GetParentFrame());
		ASSERT(nIndex != -1);
		nIndex = AddPage(strXYZ[NCA_Y]+strXYZ[NCA_Z],
			RUNTIME_CLASS(CNCViewYZ), GetDocument(), GetParentFrame());
		ASSERT(nIndex != -1);
		nIndex = AddPage("４面-1", &m_wndSplitter1);
		ASSERT(nIndex != -1);
		nIndex = AddPage("４面-2", &m_wndSplitter2);
		ASSERT(nIndex != -1);
		// 各ﾍﾟｰｼﾞのﾃﾞﾊﾞｲｽｺﾝﾃｷｽﾄﾊﾝﾄﾞﾙを取得
		CClientDC*	pDC;
		for ( i=0; i<NC_SINGLEPANE; i++ ) {	// ４面以外
			pDC = new CClientDC(GetPage(i));
			m_hDC[i] = pDC->GetSafeHdc();
			delete	pDC;
		}

		// ﾄﾚｰｽ描画ｽﾚｯﾄﾞ生成
		LPTRACETHREADPARAM	pParam = new TRACETHREADPARAM;
		pParam->pFrame		= AfxGetNCVCMainWnd();
		pParam->pParent		= this;
		pParam->pListView	= ((CNCChild *)GetParentFrame())->GetListView();
		CTraceThread*	pThread = new CTraceThread(pParam);
		if ( !pThread->CreateThread(CREATE_SUSPENDED) ) {
			delete	pThread;
			delete	pParam;
			::NCVC_CriticalErrorMsg(__FILE__, __LINE__);	// ExitProcess()
		}
		m_hTrace = ::NCVC_DuplicateHandle(pThread->m_hThread);
		if ( !m_hTrace )
			::NCVC_CriticalErrorMsg(__FILE__, __LINE__);
		pThread->ResumeThread();
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		return -1;
	}

	return 0;
}

void CNCViewTab::OnDestroy() 
{
#ifdef _DEBUG
	CMagaDbg	dbg("CNCViewTab::OnDestroy()", DBG_BLUE);
#endif

	if ( m_hTrace ) {
		// ﾄﾚｰｽ描画ｽﾚｯﾄﾞの終了指示
		m_bTraceContinue = FALSE;
		m_evTrace.SetEvent();
		MSG		msg;
		while ( ::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE) )
			AfxGetApp()->PumpMessage();
#ifdef _DEBUG
		if ( ::WaitForSingleObject(m_hTrace, INFINITE) == WAIT_FAILED ) {
			dbg.printf("WaitForSingleObject() Fail!");
			::NC_FormatMessage();
		}
		else
			dbg.printf("WaitForSingleObject() OK");
		if ( !::CloseHandle(m_hTrace) ) {
			dbg.printf("CloseHandle() Fail!");
			::NC_FormatMessage();
		}
		else
			dbg.printf("CloseHandle() OK");
#else
		::WaitForSingleObject(m_hTrace, INFINITE);
		::CloseHandle(m_hTrace);
#endif
	}

	CTabView::OnDestroy();
}

void CNCViewTab::OnSetFocus(CWnd*) 
{
	if ( GetActivePage() > 0 )
		GetActivePageWnd()->SetFocus();
}

void CNCViewTab::OnMoveTab(UINT nID)
{
	if ( nID == ID_TAB_NEXT )
		NextActivatePage();
	else
		PrevActivatePage();
}

void CNCViewTab::OnUpdateTraceSpeed(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck( pCmdUI->m_nID == m_nTraceSpeed );
}

void CNCViewTab::OnTraceSpeed(UINT nID)
{
	if ( m_nTraceSpeed != nID )
		m_nTraceSpeed = nID;
	AfxGetNCVCApp()->SetTraceSpeed(nID);
}

void CNCViewTab::OnUpdateTraceRun(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck( m_nTrace == ID_NCVIEW_TRACE_RUN );
}

void CNCViewTab::OnTraceRun() 
{
	switch ( m_nTrace ) {
	case ID_NCVIEW_TRACE_STOP:
		m_bTraceContinue = TRUE;
		m_bTracePause = FALSE;
		m_nTrace = ID_NCVIEW_TRACE_RUN;
		GetDocument()->StartTrace();
		GetDocument()->UpdateAllViews(this, UAV_STARTSTOPTRACE);
		// through
	case ID_NCVIEW_TRACE_PAUSE:
		m_evTrace.PulseEvent();	// ﾌﾗｸﾞ状態そのままでﾜﾝｽﾃｯﾌﾟ実行
		break;
	}
}

void CNCViewTab::OnUpdateTracePause(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck( m_nTrace == ID_NCVIEW_TRACE_PAUSE );
}

void CNCViewTab::OnTracePause() 
{
	switch ( m_nTrace ) {
	case ID_NCVIEW_TRACE_RUN:
		m_bTracePause = TRUE;
		m_nTrace = ID_NCVIEW_TRACE_PAUSE;
		break;
	case ID_NCVIEW_TRACE_STOP:
		m_bTracePause = m_bTraceContinue = TRUE;
		m_nTrace = ID_NCVIEW_TRACE_PAUSE;
		GetDocument()->StartTrace();
		GetDocument()->UpdateAllViews(this, UAV_STARTSTOPTRACE);
		break;
	case ID_NCVIEW_TRACE_PAUSE:
		m_bTracePause = FALSE;
		m_nTrace = ID_NCVIEW_TRACE_RUN;
		m_evTrace.PulseEvent();
		break;
	}
}

void CNCViewTab::OnTraceStop() 
{
	if ( m_nTrace != ID_NCVIEW_TRACE_STOP ) {
		m_bTraceContinue = FALSE;
		m_nTrace = ID_NCVIEW_TRACE_STOP;
		// 現在位置以降を描画
		GetDocument()->StopTrace();
	}
	GetDocument()->UpdateAllViews(this, UAV_STARTSTOPTRACE);
}

void CNCViewTab::OnUpdateTraceCursor(CCmdUI* pCmdUI) 
{
	CNCListView*	pList = ((CNCChild *)GetParentFrame())->GetListView();
	pCmdUI->Enable(pList->GetListCtrl().GetFirstSelectedItemPosition() ? TRUE : FALSE);
}

void CNCViewTab::OnTraceCursor(UINT nID) 
{
	// ﾘｽﾄｺﾝﾄﾛｰﾙの現在位置取得
	CNCListView*	pList = ((CNCChild *)GetParentFrame())->GetListView();
	POSITION pos;
	if ( !(pos=pList->GetListCtrl().GetFirstSelectedItemPosition()) ) {
		m_bTraceContinue = FALSE;
		m_nTrace = ID_NCVIEW_TRACE_STOP;
		GetDocument()->StopTrace();
		GetDocument()->UpdateAllViews(this, UAV_STARTSTOPTRACE);
		return;
	}

	int	nLine = pList->GetListCtrl().GetNextSelectedItem(pos);
	if ( nID == ID_NCVIEW_TRACE_CURSOR ) {
		// ﾘｽﾄｺﾝﾄﾛｰﾙの行番号からｵﾌﾞｼﾞｪｸﾄ番号を検索し再描画
		if ( GetDocument()->SetLineToTrace(FALSE, nLine) ) {
			m_bTracePause = m_bTraceContinue = TRUE;
			m_nTrace = ID_NCVIEW_TRACE_PAUSE;	// そこで一時停止
		}
		else {
			m_bTraceContinue = FALSE;
			m_nTrace = ID_NCVIEW_TRACE_STOP;
		}
	}
	else {
		if ( GetDocument()->SetLineToTrace(TRUE, nLine) ) {
			m_bTraceContinue = TRUE;
			m_bTracePause = FALSE;
			m_nTrace = ID_NCVIEW_TRACE_RUN;
			m_evTrace.PulseEvent();		// ﾄﾚｰｽ開始
		}
		else {
			m_bTraceContinue = FALSE;
			m_nTrace = ID_NCVIEW_TRACE_STOP;
		}
	}

	GetDocument()->UpdateAllViews(this, UAV_TRACECURSOR);
	if ( nID == ID_NCVIEW_TRACE_CURSOR2 )
		GetDocument()->ResetTraceStart();	// 次の再描画に備える
}

void CNCViewTab::OnUpdateAllFitCmd(CCmdUI* pCmdUI)
{
	pCmdUI->Enable( GetActivePage() >= NC_SINGLEPANE );
}
/*
BOOL CNCViewTab::OnEraseBkgnd(CDC* pDC) 
{
#ifdef _DEBUG
	CMagaDbg	dbg("CNCViewTab::OnEraseBkgnd()\nStart");
#endif
	GetActivePageWnd()->Invalidate();
	return FALSE;
}
*/
/////////////////////////////////////////////////////////////////////////////
// CTraceThread

BOOL CTraceThread::InitInstance()
{
#ifdef _DEBUG
	CMagaDbg	dbg("CTraceThread::InitInstance()\nStart", DBG_BLUE);
#endif
	CNCDoc*			pDoc  = m_pParent->GetDocument();
	CWnd*			pWnd;
	CNCdata*		pData;
	CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();
	CDC		dc;
	int		nPage, nTraceDraw;
	BOOL	bBreak;

	// ｽﾚｯﾄﾞのﾙｰﾌﾟ
	while ( TRUE ) {
		m_pParent->m_evTrace.Lock();
#ifdef _DEBUG
		dbg.printf("m_evTrace Lock() return");
#endif
		if ( !m_pParent->m_bTraceContinue )
			break;
		do {
			bBreak = pDoc->IncrementTrace(nTraceDraw);
			if ( nTraceDraw <= 0 ) {
				m_pParent->m_nTrace = ID_NCVIEW_TRACE_STOP;
				AfxGetNCVCMainWnd()->PostMessage(WM_NULL);	// ﾂｰﾙﾎﾞﾀﾝを即時更新
				break;
			}
			pData = pDoc->GetNCdata(nTraceDraw-1);
			m_pListView->SelectTrace(pData);
			nPage = m_pParent->GetActivePage();
			if ( nPage < 0 )
				continue;
			if ( nPage < NC_SINGLEPANE ) {
				if ( !dc.Attach(m_pParent->m_hDC[nPage]) ) {
					::NCVC_CriticalErrorMsg(__FILE__, __LINE__);
				}
			}
			pWnd  = m_pParent->GetPage(nPage);
			switch ( nPage ) {
			case 0:
				pData->Draw(&dc);
				break;
			case 1:
				pData->DrawXY(&dc);
				break;
			case 2:
				pData->DrawXZ(&dc);
				break;
			case 3:
				pData->DrawYZ(&dc);
				break;
			case 4:
			case 5:
				((CNCViewSplit *)pWnd)->DrawData(pData);
				break;
			}
			if ( nPage < NC_SINGLEPANE )
				dc.Detach();
			GdiFlush();
			if ( bBreak ) {
				m_pParent->m_bTracePause = TRUE;
				m_pParent->m_nTrace = ID_NCVIEW_TRACE_PAUSE;
				AfxGetNCVCMainWnd()->PostMessage(WM_NULL);
			}
			else if ( m_pParent->m_nTrace != ID_NCVIEW_TRACE_PAUSE ) {
				Sleep( pOpt->GetTraceSpeed(m_pParent->m_nTraceSpeed-ID_NCVIEW_TRACE_FAST) );
			}
		} while ( m_pParent->m_bTraceContinue && !m_pParent->m_bTracePause );
#ifdef _DEBUG
		dbg.printf("Stop the trace loop");
#endif
	}

#ifdef _DEBUG
	dbg.printf("Finish");
#endif

	// InitInstance() から異常終了で返し，ｽﾚｯﾄﾞの終了
	return FALSE;
}

int CTraceThread::ExitInstance()
{
	// InitInstance() で異常終了を返すので正常終了ｺｰﾄﾞを返す
	CWinThread::ExitInstance();
	return 0;	// ｽﾚｯﾄﾞ正常終了ｺｰﾄﾞ
}
