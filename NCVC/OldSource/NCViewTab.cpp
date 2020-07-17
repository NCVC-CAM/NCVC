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
#include "NCViewGL.h"
#include "NCListView.h"
#include "ViewOption.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

IMPLEMENT_DYNCREATE(CNCViewTab, CTabViewBase)

BEGIN_MESSAGE_MAP(CNCViewTab, CTabViewBase)
	ON_WM_CREATE()
	ON_WM_DESTROY()
//	ON_WM_SETFOCUS()
	// ﾀﾌﾞ移動
	ON_COMMAND_RANGE(ID_TAB_NEXT, ID_TAB_PREV, &CNCViewTab::OnMoveTab)
	// ﾄﾚｰｽ
	ON_UPDATE_COMMAND_UI_RANGE(ID_NCVIEW_TRACE_FAST, ID_NCVIEW_TRACE_LOW, &CNCViewTab::OnUpdateTraceSpeed)
	ON_COMMAND_RANGE(ID_NCVIEW_TRACE_FAST, ID_NCVIEW_TRACE_LOW, &CNCViewTab::OnTraceSpeed)
	ON_UPDATE_COMMAND_UI(ID_NCVIEW_TRACE_RUN, &CNCViewTab::OnUpdateTraceRun)
	ON_COMMAND(ID_NCVIEW_TRACE_RUN, &CNCViewTab::OnTraceRun)
	ON_UPDATE_COMMAND_UI(ID_NCVIEW_TRACE_PAUSE, &CNCViewTab::OnUpdateTracePause)
	ON_COMMAND(ID_NCVIEW_TRACE_PAUSE, &CNCViewTab::OnTracePause)
	ON_COMMAND(ID_NCVIEW_TRACE_STOP, &CNCViewTab::OnTraceStop)
	ON_UPDATE_COMMAND_UI_RANGE(ID_NCVIEW_TRACE_CURSOR, ID_NCVIEW_TRACE_CURSOR2, &CNCViewTab::OnUpdateTraceCursor)
	ON_COMMAND_RANGE(ID_NCVIEW_TRACE_CURSOR, ID_NCVIEW_TRACE_CURSOR2, &CNCViewTab::OnTraceCursor)
	// 「全てのﾍﾟｲﾝの図形ﾌｨｯﾄ」ﾒﾆｭｰｺﾏﾝﾄﾞの使用許可
	ON_UPDATE_COMMAND_UI(ID_NCVIEW_ALLFIT, &CNCViewTab::OnUpdateAllFitCmd)
	// 「直前の拡大率」ﾒﾆｭｰｺﾏﾝﾄﾞの使用許可
	ON_UPDATE_COMMAND_UI(ID_VIEW_BEFORE, &CNCViewTab::OnUpdateBeforeView)
	// 他
	ON_UPDATE_COMMAND_UI(ID_OPTION_DEFVIEWINFO, &CNCViewTab::OnUpdateDefViewInfo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, &CNCViewTab::OnUpdateEditCopy)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_UP,  ID_VIEW_RT,  &CNCViewTab::OnUpdateMoveKey)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_RUP, ID_VIEW_RRT, &CNCViewTab::OnUpdateRoundKey)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////

CNCViewTab::CNCViewTab() : m_evTrace(FALSE, TRUE)
{
#ifdef _DEBUG_FILEOPEN
	g_dbg.printf("CNCViewTab::CNCViewTab() Start");
#endif
/*
	ここでﾄﾚｰｽｽﾋﾟｰﾄﾞをﾚｼﾞｽﾄﾘから取得すると，NCViewTab起動の度にｱｸｾｽされることになる
	以下，ｱｸﾃｨﾌﾞﾍﾟｰｼﾞ情報も同じ
*/
	m_nTraceSpeed = AfxGetNCVCApp()->GetTraceSpeed();
	m_nTrace = ID_NCVIEW_TRACE_STOP;
	m_pTraceThread = NULL;
	m_bTraceContinue = m_bTracePause = FALSE;
	m_pDataTraceSel  = NULL;
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
	CTabViewBase::OnInitialUpdate();

	// ﾄﾚｰｽ描画ｽﾚｯﾄﾞ生成
	// --- OnCreateではIsDocFlag()が間に合わない
	LPTRACETHREADPARAM	pParam = new TRACETHREADPARAM;
	pParam->pMainFrame	= AfxGetNCVCMainWnd();
	pParam->pParent		= this;
	pParam->pListView	= static_cast<CNCChild *>(GetParentFrame())->GetListView();
	m_pTraceThread = new CTraceThread(pParam);
	if ( !m_pTraceThread->CreateThread() ) {
		delete	m_pTraceThread;
		delete	pParam;
		::NCVC_CriticalErrorMsg(__FILE__, __LINE__);	// ExitProcess()
	}

	// ｱｸﾃｨﾌﾞﾍﾟｰｼﾞ情報
	int	nPage = AfxGetNCVCApp()->GetNCTabPage();
	ActivatePage(nPage);

	// CNCViewSplitのｻｲｽﾞ調整
	// 現在ｱｸﾃｨﾌﾞﾍﾟｰｼﾞと等しいときは，さらに各ﾋﾞｭｰWM_USERVIEWFITMSG送信
	m_wndSplitter1.PostMessage(WM_USERINITIALUPDATE, NCVIEW_FOURVIEW,   nPage==NCVIEW_FOURVIEW);
	m_wndSplitter2.PostMessage(WM_USERINITIALUPDATE, NCVIEW_FOURVIEW+1, nPage==NCVIEW_FOURVIEW+1);

	if ( nPage<NCVIEW_FOURVIEW || nPage==NCVIEW_OPENGL ) {
		// 図形ﾌｨｯﾄﾒｯｾｰｼﾞの送信
		// 各ﾋﾞｭｰのOnInitialUpdate()関数内では，GetClientRect()のｻｲｽﾞが正しくない
		CWnd*	pWnd = GetPage(nPage);
		if ( pWnd )
			pWnd->PostMessage(WM_USERVIEWFITMSG, 0, 1);
	}
	else {
		// ｽﾌﾟﾘｯﾀ表示の場合は，拡大率の再更新
		for ( int i=0; i<SIZEOF(m_bSplit); i++ )
			m_bSplit[i] = TRUE;
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
	CTabViewBase::OnUpdate(pSender, lHint, pHint);
}

BOOL CNCViewTab::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
#ifdef _DEBUG_CMDMSG
	g_dbg.printf("CNCViewTab::OnCmdMsg()");
#endif
	// ﾀﾌﾞ移動だけ特別
	if ( nID == ID_TAB_NEXT || nID == ID_TAB_PREV ) {
		CWnd*	pWnd = GetFocus();
		if ( pWnd->IsKindOf(RUNTIME_CLASS(CNCViewTab)) ||
				pWnd->IsKindOf(RUNTIME_CLASS(CNCView)) ||
				pWnd->IsKindOf(RUNTIME_CLASS(CNCViewXY)) ||
				pWnd->IsKindOf(RUNTIME_CLASS(CNCViewXZ)) ||
				pWnd->IsKindOf(RUNTIME_CLASS(CNCViewYZ)) ||
				pWnd->IsKindOf(RUNTIME_CLASS(CNCViewGL)) )
			return CTabViewBase::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
		else
			return FALSE;
	}

	// 自分自身とｱｸﾃｨﾌﾞなﾋﾞｭｰだけにｺﾏﾝﾄﾞﾙｰﾃｨﾝｸﾞ
	// 結果的に CNCDoc へのｺﾏﾝﾄﾞﾙｰﾃｨﾝｸﾞはここからだけになる
	if ( CTabViewBase::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo) ) {
#ifdef _DEBUG_CMDMSG
		g_dbg.printf("CTabViewBase::OnCmdMsg() return");
#endif
		return TRUE;
	}
	if ( GetPageCount() <= 0 )
		return FALSE;
	return GetActivePageWnd()->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

void CNCViewTab::OnActivatePage(int nIndex)
{
	CWnd*	pWnd = GetPage(nIndex);
	if ( pWnd ) {
		// ４面表示と単一平面表示の拡大率更新のためにﾒｯｾｰｼﾞ送信
		LPARAM	lSplit = nIndex < NCVIEW_FOURVIEW ? (LPARAM)m_bSplit[nIndex] : 0;
		// WPARAMはCNCViewSplit::OnUserActivatePage()向けのみ
		pWnd->SendMessage(WM_USERACTIVATEPAGE, (WPARAM)nIndex, lSplit);
	}
	if ( nIndex < NCVIEW_FOURVIEW ) {
		m_bSplit[nIndex] = FALSE;
		GetParentFrame()->SetActiveView(static_cast<CView *>(pWnd));
	}
	else if ( nIndex < NCVIEW_OPENGL ) {
		// ｽﾌﾟﾘｯﾀ表示の場合は常に拡大率の再更新
		for ( int i=0; i<SIZEOF(m_bSplit); i++ )
			m_bSplit[i] = TRUE;
		// ｱｸﾃｨﾌﾞﾋﾞｭｰはｽﾌﾟﾘｯﾀ内のSetActivePane()で
	}
	else if ( nIndex == NCVIEW_OPENGL ) {
		GetParentFrame()->SetActiveView(static_cast<CView *>(pWnd));
	}
	AfxGetNCVCApp()->SetNCTabPage(nIndex);
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewTab クラスの診断

#ifdef _DEBUG
void CNCViewTab::AssertValid() const
{
	CTabViewBase::AssertValid();
}

void CNCViewTab::Dump(CDumpContext& dc) const
{
	CTabViewBase::Dump(dc);
}

CNCDoc* CNCViewTab::GetDocument() // 非デバッグ バージョンはインラインです。
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CNCDoc)));
	return static_cast<CNCDoc *>(m_pDocument);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CNCViewTab クラスのメッセージ ハンドラ

int CNCViewTab::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
#ifdef _DEBUG_FILEOPEN
	g_dbg.printf("CNCViewTab::OnCreate() Start");
#endif
	extern	LPCTSTR	g_szNdelimiter;		// "XYZUVWIJKRPLDH" from NCDoc.cpp

	if ( CTabViewBase::OnCreate(lpCreateStruct) < 0 )
		return -1;
	GetTabCtrl().SetFont(AfxGetNCVCMainWnd()->GetTextFont(TYPE_NCD), FALSE);

	// thisﾎﾟｲﾝﾀのｾｯﾄのみ
	CViewBase::OnCreate(this, FALSE);

	int		i, nIndex;
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	//	静的ｽﾌﾟﾘｯﾀ作成
	CCreateContext	ct;
	ct.m_pCurrentDoc = GetDocument();
	ct.m_pCurrentFrame = GetParentFrame();
	ct.m_pLastView = NULL;
	ct.m_pNewDocTemplate = GetDocument()->GetDocTemplate();
	ct.m_pNewViewClass = NULL;
	CRuntimeClass*	pNCView[NCVIEW_FOURVIEW];
	CRuntimeClass*	pView01Class[NCVIEW_FOURVIEW];
	CRuntimeClass*	pView02Class[NCVIEW_FOURVIEW];
	pNCView[0] = RUNTIME_CLASS(CNCView);
	pNCView[1] = RUNTIME_CLASS(CNCViewXY);
	pNCView[2] = RUNTIME_CLASS(CNCViewXZ);
	pNCView[3] = RUNTIME_CLASS(CNCViewYZ);
	for ( i=0; i<SIZEOF(pView01Class); i++ ) {
		pView01Class[i] = pNCView[ pOpt->GetForceView01(i) ];
		pView02Class[i] = pNCView[ pOpt->GetForceView02(i) ];
	}
	// ４面-1
	if ( !m_wndSplitter1.CreateStatic(this, 2, 2, WS_CHILD) ||
			!m_wndSplitter1.CreateView(0, 0, pView01Class[0], CSize(0, 0), &ct) ||
			!m_wndSplitter1.CreateView(0, 1, pView01Class[1], CSize(0, 0), &ct) ||
			!m_wndSplitter1.CreateView(1, 0, pView01Class[2], CSize(0, 0), &ct) ||
			!m_wndSplitter1.CreateView(1, 1, pView01Class[3], CSize(0, 0), &ct) )
		return -1;
	// ４面-2
	if ( !m_wndSplitter2.CreateStatic(this, 1, 2, WS_CHILD) ||
			!m_wndSplitter22.CreateStatic(&m_wndSplitter2, 3, 1,
							WS_CHILD|WS_VISIBLE, m_wndSplitter2.IdFromRowCol(0, 0)) ||
			!m_wndSplitter22.CreateView(0, 0, pView02Class[0], CSize(0, 0), &ct) ||
			!m_wndSplitter22.CreateView(1, 0, pView02Class[1], CSize(0, 0), &ct) ||
			!m_wndSplitter22.CreateView(2, 0, pView02Class[2], CSize(0, 0), &ct) ||
			!m_wndSplitter2.CreateView (0, 1, pView02Class[3], CSize(0, 0), &ct) )
		return -1;

	// 各ﾍﾟｰｼﾞﾋﾞｭｰの生成
	CString	strXYZ[NCXYZ];
	for ( i=0; i<NCXYZ; i++ )
		strXYZ[i] = g_szNdelimiter[i];
	try {
		nIndex = AddPage(strXYZ[NCA_X]+strXYZ[NCA_Y]+strXYZ[NCA_Z],
			pNCView[0], GetDocument(), GetParentFrame());
		ASSERT(nIndex >= 0);
		nIndex = AddPage(strXYZ[NCA_X]+strXYZ[NCA_Y],
			pNCView[1], GetDocument(), GetParentFrame());
		ASSERT(nIndex >= 0);
		nIndex = AddPage(strXYZ[NCA_X]+strXYZ[NCA_Z],
			pNCView[2], GetDocument(), GetParentFrame());
		ASSERT(nIndex >= 0);
		nIndex = AddPage(strXYZ[NCA_Y]+strXYZ[NCA_Z],
			pNCView[3], GetDocument(), GetParentFrame());
		ASSERT(nIndex >= 0);
		nIndex = AddPage("４面-1", &m_wndSplitter1);
		ASSERT(nIndex >= 0);
		nIndex = AddPage("４面-2", &m_wndSplitter2);
		ASSERT(nIndex >= 0);
		nIndex = AddPage("OpenGL",
			RUNTIME_CLASS(CNCViewGL), GetDocument(), GetParentFrame());
		// 各ﾍﾟｰｼﾞのﾃﾞﾊﾞｲｽｺﾝﾃｷｽﾄﾊﾝﾄﾞﾙを取得
		CClientDC*	pDC;
		for ( i=0; i<NCVIEW_FOURVIEW; i++ ) {	// ４面以外
			pDC = new CClientDC(GetPage(i));
			m_hDC[i] = pDC->GetSafeHdc();
			delete	pDC;
		}
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

	if ( m_pTraceThread ) {
		// ﾄﾚｰｽ描画ｽﾚｯﾄﾞの終了指示
		m_bTraceContinue = FALSE;
		m_evTrace.SetEvent();
		MSG		msg;
		while ( ::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE) )
			AfxGetApp()->PumpMessage();
#ifdef _DEBUG
		if ( ::WaitForSingleObject(m_pTraceThread->m_hThread, INFINITE) == WAIT_FAILED ) {
			dbg.printf("WaitForSingleObject() Fail!");
			::NC_FormatMessage();
		}
		else
			dbg.printf("WaitForSingleObject() OK");
#else
		::WaitForSingleObject(m_pTraceThread->m_hThread, INFINITE);
#endif
		delete	m_pTraceThread;
	}

	CTabViewBase::OnDestroy();
}
/*
void CNCViewTab::OnSetFocus(CWnd*) 
{
#ifdef _DEBUG
	g_dbg.print("CNCViewTab::OnSetFocus()");
#endif
	int	nIndex = GetActivePage();
	if ( nIndex >= 0 ) {
//		GetPage(nIndex)->SetFocus();
		GetTabCtrl().SetCurSel(nIndex);
		GetTabCtrl().SetCurFocus(nIndex);
	}
#ifdef _DEBUG
	else {
		g_dbg.print("not select active page");
	}
#endif
}
*/
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
		m_pDataTraceSel  = NULL;
		m_nTrace = ID_NCVIEW_TRACE_STOP;
		// 現在位置以降を描画
		GetDocument()->StopTrace();
	}
	GetDocument()->UpdateAllViews(this, UAV_STARTSTOPTRACE);
}

void CNCViewTab::OnUpdateTraceCursor(CCmdUI* pCmdUI) 
{
	CNCListView*	pList = static_cast<CNCChild *>(GetParentFrame())->GetListView();
	pCmdUI->Enable(pList->GetListCtrl().GetFirstSelectedItemPosition() ? TRUE : FALSE);
}

void CNCViewTab::OnTraceCursor(UINT nID) 
{
	// ﾘｽﾄｺﾝﾄﾛｰﾙの現在位置取得
	CNCListView*	pList = static_cast<CNCChild *>(GetParentFrame())->GetListView();
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
	int nIndex = GetActivePage();
	pCmdUI->Enable( NCVIEW_FOURVIEW<=nIndex && nIndex<NCVIEW_OPENGL );
}

void CNCViewTab::OnUpdateBeforeView(CCmdUI* pCmdUI)
{
	pCmdUI->Enable( GetActivePage() < NCVIEW_OPENGL );
}

void CNCViewTab::OnUpdateDefViewInfo(CCmdUI *pCmdUI)
{
	pCmdUI->Enable( GetActivePage() == NCVIEW_OPENGL );
}

void CNCViewTab::OnUpdateEditCopy(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( GetActivePage() < NCVIEW_OPENGL );
}

void CNCViewTab::OnUpdateMoveKey(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(TRUE);
}

void CNCViewTab::OnUpdateRoundKey(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( GetActivePage() == NCVIEW_OPENGL );
}

/////////////////////////////////////////////////////////////////////////////
// CTraceThread

BOOL CTraceThread::InitInstance()
{
#ifdef _DEBUG
	CMagaDbg	dbg("CTraceThread::InitInstance()\nStart", DBG_BLUE);
#endif
	CNCDoc*			pDoc  = m_pParent->GetDocument();
	CNCdata*		pData;
	CNCViewSplit*	pWnd;
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	CDC		dc;
	int		i, nPage, nTraceDraw;
	BOOL	bBreak, bSelect;
	PFNNCDRAWPROC	pfnDrawXYZ[NCVIEW_FOURVIEW],
					pfnDrawSplit01[NCVIEW_FOURVIEW],
					pfnDrawSplit02[NCVIEW_FOURVIEW];

	if ( pDoc->IsDocFlag(NCDOC_WIRE) ) {
		pfnDrawXYZ[0] = &(CNCdata::DrawWire);
		pfnDrawXYZ[1] = &(CNCdata::DrawWireXY);
		pfnDrawXYZ[2] = &(CNCdata::DrawWireXZ);
		pfnDrawXYZ[3] = &(CNCdata::DrawWireYZ);
	}
	else {
		pfnDrawXYZ[0] = &(CNCdata::Draw);
		pfnDrawXYZ[1] = &(CNCdata::DrawXY);
		pfnDrawXYZ[2] = &(CNCdata::DrawXZ);
		pfnDrawXYZ[3] = &(CNCdata::DrawYZ);
	}
	for ( i=0; i<SIZEOF(pfnDrawXYZ); i++ ) {
		pfnDrawSplit01[i] = pfnDrawXYZ[ pOpt->GetForceView01(i) ];
		pfnDrawSplit02[i] = pfnDrawXYZ[ pOpt->GetForceView02(i) ];
	}

	// ｽﾚｯﾄﾞのﾙｰﾌﾟ
	while ( TRUE ) {
		m_pParent->m_evTrace.Lock();
#ifdef _DEBUG
		dbg.printf("m_evTrace Lock() return");
#endif
		if ( !m_pParent->m_bTraceContinue )
			break;

		// ﾏｰｶ表示状態取得
		bSelect = pOpt->GetNCViewFlg(NCVIEWFLG_TRACEMARKER);

		// ﾄﾚｰｽ開始・再開
		do {
			nPage = m_pParent->GetActivePage();
			if ( nPage < 0 )
				break;
			bBreak = pDoc->IncrementTrace(nTraceDraw);
			if ( nTraceDraw < 0 ) {
				// 最後の選択消去
				if ( bSelect && m_pParent->m_pDataTraceSel ) {
					if ( nPage < NCVIEW_FOURVIEW ) {
						if ( !dc.Attach(m_pParent->m_hDC[nPage]) )
							::NCVC_CriticalErrorMsg(__FILE__, __LINE__);
						dc.SetROP2(R2_COPYPEN);
						(m_pParent->m_pDataTraceSel->*pfnDrawXYZ[nPage])(&dc, FALSE);
						dc.Detach();
					}
					else if ( nPage < NCVIEW_OPENGL ) {
						pWnd = static_cast<CNCViewSplit *>(m_pParent->GetPage(nPage));
						if ( pWnd )
							pWnd->DrawData(m_pParent->m_pDataTraceSel, FALSE,
								nPage == NCVIEW_FOURVIEW ? pfnDrawSplit01 : pfnDrawSplit02);
					}
				}
				// ﾂｰﾙﾎﾞﾀﾝを即時更新
				m_pParent->m_nTrace = ID_NCVIEW_TRACE_STOP;
				AfxGetNCVCMainWnd()->PostMessage(WM_NULL);
				// 次の再開に備える
				m_pParent->m_pDataTraceSel = NULL;
				break;
			}
			pData = pDoc->GetNCdata(nTraceDraw-1);
			m_pListView->SendMessage(WM_USERTRACESELECT, (WPARAM)pData);
			if ( nPage < NCVIEW_FOURVIEW ) {
				if ( !dc.Attach(m_pParent->m_hDC[nPage]) )
					::NCVC_CriticalErrorMsg(__FILE__, __LINE__);
				if ( bSelect && m_pParent->m_pDataTraceSel ) {
					dc.SetROP2(R2_COPYPEN);
					(m_pParent->m_pDataTraceSel->*pfnDrawXYZ[nPage])(&dc, FALSE);
				}
				(pData->*pfnDrawXYZ[nPage])(&dc, bSelect);
				dc.Detach();
			}
			else if ( nPage < NCVIEW_OPENGL ) {
				pWnd = static_cast<CNCViewSplit *>(m_pParent->GetPage(nPage));
				if ( pWnd && bSelect && m_pParent->m_pDataTraceSel ) {
					pWnd->DrawData(m_pParent->m_pDataTraceSel, FALSE,
						nPage == NCVIEW_FOURVIEW ? pfnDrawSplit01 : pfnDrawSplit02);
				}
				pWnd->DrawData(pData, bSelect,
					nPage == NCVIEW_FOURVIEW ? pfnDrawSplit01 : pfnDrawSplit02);
			}
			::GdiFlush();
			if ( bBreak ) {
				m_pParent->m_bTracePause = TRUE;
				m_pParent->m_nTrace = ID_NCVIEW_TRACE_PAUSE;
				AfxGetNCVCMainWnd()->PostMessage(WM_NULL);
			}
			else if ( m_pParent->m_nTrace != ID_NCVIEW_TRACE_PAUSE )
				::Sleep( pOpt->GetTraceSpeed(m_pParent->m_nTraceSpeed-ID_NCVIEW_TRACE_FAST) );
			// 次の選択解除用
			m_pParent->m_pDataTraceSel = pData;
		} while ( m_pParent->m_bTraceContinue && !m_pParent->m_bTracePause );
#ifdef _DEBUG
		dbg.printf("Stop the trace loop");
#endif
		if ( !m_pParent->m_bTraceContinue )
			m_pParent->m_pDataTraceSel = FALSE;	// 次の再開に備える
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
