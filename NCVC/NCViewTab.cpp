// NCViewTab.cpp: CNCViewTab �N���X�̃C���v�������e�[�V����
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

IMPLEMENT_DYNCREATE(CNCViewTab, CTabView)

BEGIN_MESSAGE_MAP(CNCViewTab, CTabView)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SETFOCUS()
	// ��ވړ�
	ON_COMMAND_RANGE(ID_TAB_NEXT, ID_TAB_PREV, OnMoveTab)
	// �ڰ�
	ON_UPDATE_COMMAND_UI_RANGE(ID_NCVIEW_TRACE_FAST, ID_NCVIEW_TRACE_LOW, OnUpdateTraceSpeed)
	ON_COMMAND_RANGE(ID_NCVIEW_TRACE_FAST, ID_NCVIEW_TRACE_LOW, OnTraceSpeed)
	ON_UPDATE_COMMAND_UI(ID_NCVIEW_TRACE_RUN, OnUpdateTraceRun)
	ON_COMMAND(ID_NCVIEW_TRACE_RUN, OnTraceRun)
	ON_UPDATE_COMMAND_UI(ID_NCVIEW_TRACE_PAUSE, OnUpdateTracePause)
	ON_COMMAND(ID_NCVIEW_TRACE_PAUSE, OnTracePause)
	ON_COMMAND(ID_NCVIEW_TRACE_STOP, OnTraceStop)
	ON_UPDATE_COMMAND_UI_RANGE(ID_NCVIEW_TRACE_CURSOR, ID_NCVIEW_TRACE_CURSOR2, OnUpdateTraceCursor)
	ON_COMMAND_RANGE(ID_NCVIEW_TRACE_CURSOR, ID_NCVIEW_TRACE_CURSOR2, OnTraceCursor)
	// �u�S�Ă��߲݂̐}�`̨�āv�ƭ�����ނ̎g�p����
	ON_UPDATE_COMMAND_UI(ID_NCVIEW_ALLFIT, OnUpdateAllFitCmd)
	// �u���O�̊g�嗦�v�ƭ�����ނ̎g�p����
	ON_UPDATE_COMMAND_UI(ID_VIEW_BEFORE, OnUpdateBeforeView)
	//
	ON_UPDATE_COMMAND_UI(ID_OPTION_DEFVIEWINFO, OnUpdateDefViewInfo)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////
// �\�z/����
//////////////////////////////////////////////////////////////////////

CNCViewTab::CNCViewTab() : m_evTrace(FALSE, TRUE)
{
/*
	�������ڰ���߰�ނ�ڼ޽�؂���擾����ƁCNCViewTab�N���̓x�ɱ�������邱�ƂɂȂ�
	�ȉ��C��è���߰�ޏ�������
*/
	m_nTraceSpeed = AfxGetNCVCApp()->GetTraceSpeed();
	m_nTrace = ID_NCVIEW_TRACE_STOP;
	m_pTraceThread = NULL;
	m_bTraceContinue = m_bTracePause = FALSE;
	for ( int i=0; i<SIZEOF(m_bSplit); i++ )
		m_bSplit[i] = FALSE;
}

CNCViewTab::~CNCViewTab()
{
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewTab �N���X�̃I�[�o���C�h�֐�

void CNCViewTab::OnInitialUpdate() 
{
	CTabView::OnInitialUpdate();

	// �ڰ��`��گ�ސ���
	// --- OnCreate�ł�IsNCDocFlag()���Ԃɍ���Ȃ�
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

	// ��è���߰�ޏ��
	int	nPage = AfxGetNCVCApp()->GetNCTabPage();
	ActivatePage(nPage);

	// CNCViewSplit�̻��ޒ���
	// ���ݱ�è���߰�ނƓ������Ƃ��́C����Ɋe�ޭ�WM_USERVIEWFITMSG���M
	m_wndSplitter1.PostMessage(WM_USERINITIALUPDATE, NCVIEW_FOURSVIEW,   nPage==NCVIEW_FOURSVIEW);
	m_wndSplitter2.PostMessage(WM_USERINITIALUPDATE, NCVIEW_FOURSVIEW+1, nPage==NCVIEW_FOURSVIEW+1);

	if ( nPage<NCVIEW_FOURSVIEW || nPage==NCVIEW_OPENGL ) {
		// �}�`̨��ү���ނ̑��M
		// �e�ޭ���OnInitialUpdate()�֐����ł́CGetClientRect()�̻��ނ��������Ȃ�
		CWnd*	pWnd = GetPage(nPage);
		if ( pWnd )
			pWnd->PostMessage(WM_USERVIEWFITMSG, 0, 1);
	}
	else {
		// ���د��\���̏ꍇ�́C�g�嗦�̍čX�V
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
		return;		// �ĕ`��s�v
	case UAV_CHANGEFONT:
		GetTabCtrl().SetFont(AfxGetNCVCMainWnd()->GetTextFont(TYPE_NCD), FALSE);
		break;
	}
	CTabView::OnUpdate(pSender, lHint, pHint);
}

BOOL CNCViewTab::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
	// ��ވړ���������
	if ( nID == ID_TAB_NEXT || nID == ID_TAB_PREV ) {
		CWnd*	pWnd = GetFocus();
		if ( pWnd->IsKindOf(RUNTIME_CLASS(CNCViewTab)) ||
				pWnd->IsKindOf(RUNTIME_CLASS(CNCView)) ||
				pWnd->IsKindOf(RUNTIME_CLASS(CNCViewXY)) ||
				pWnd->IsKindOf(RUNTIME_CLASS(CNCViewXZ)) ||
				pWnd->IsKindOf(RUNTIME_CLASS(CNCViewYZ)) ||
				pWnd->IsKindOf(RUNTIME_CLASS(CNCViewGL)) )
			return CTabView::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
		else
			return FALSE;
	}

	// �������g�Ʊ�è�ނ��ޭ������ɺ����ٰèݸ�
	// ���ʓI�� CNCDoc �ւ̺����ٰèݸނ͂������炾���ɂȂ�
	if ( CTabView::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo) )
		return TRUE;
	if ( GetPageCount() <= 0 )
		return FALSE;
	return GetActivePageWnd()->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

void CNCViewTab::OnActivatePage(int nIndex)
{
	CWnd*	pWnd = GetPage(nIndex);
	if ( pWnd ) {
		// �S�ʕ\���ƒP�ꕽ�ʕ\���̊g�嗦�X�V�̂��߂�ү���ޑ��M
		LPARAM	lSplit = nIndex < NCVIEW_FOURSVIEW ? (LPARAM)m_bSplit[nIndex] : 0;
		// WPARAM��CNCViewSplit::OnUserActivatePage()�����̂�
		pWnd->SendMessage(WM_USERACTIVATEPAGE, (WPARAM)nIndex, lSplit);
	}
	if ( nIndex < NCVIEW_FOURSVIEW ) {
		m_bSplit[nIndex] = FALSE;
		GetParentFrame()->SetActiveView(static_cast<CView *>(pWnd));
	}
	else if ( nIndex < NCVIEW_OPENGL ) {
		// ���د��\���̏ꍇ�͏�Ɋg�嗦�̍čX�V
		for ( int i=0; i<SIZEOF(m_bSplit); i++ )
			m_bSplit[i] = TRUE;
		// ��è���ޭ��ͽ��د�����SetActivePane()��
	}
	else if ( nIndex == NCVIEW_OPENGL ) {
		GetParentFrame()->SetActiveView(static_cast<CView *>(pWnd));
	}
	AfxGetNCVCApp()->SetNCTabPage(nIndex);
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewTab �N���X�̐f�f

#ifdef _DEBUG
void CNCViewTab::AssertValid() const
{
	CTabView::AssertValid();
}

void CNCViewTab::Dump(CDumpContext& dc) const
{
	CTabView::Dump(dc);
}

CNCDoc* CNCViewTab::GetDocument() // ��f�o�b�O �o�[�W�����̓C�����C���ł��B
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CNCDoc)));
	return static_cast<CNCDoc *>(m_pDocument);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CNCViewTab �N���X�̃��b�Z�[�W �n���h��

int CNCViewTab::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	extern	LPCTSTR	g_szNdelimiter;		// "XYZUVWIJKRPLDH" from NCDoc.cpp

	if ( CTabView::OnCreate(lpCreateStruct) < 0 )
		return -1;
	GetTabCtrl().SetFont(AfxGetNCVCMainWnd()->GetTextFont(TYPE_NCD), FALSE);

	// this�߲���̾�Ă̂�
	CViewBase::OnCreate(this, FALSE);

	//	�ÓI���د��쐬
	CCreateContext	ct;
	ct.m_pCurrentDoc = GetDocument();
	ct.m_pCurrentFrame = GetParentFrame();
	ct.m_pLastView = NULL;
	ct.m_pNewDocTemplate = GetDocument()->GetDocTemplate();
	ct.m_pNewViewClass = NULL;
	// �S��-1
	if ( !m_wndSplitter1.CreateStatic(this, 2, 2, WS_CHILD) ||
			!m_wndSplitter1.CreateView(0, 0, RUNTIME_CLASS(CNCView),   CSize(0, 0), &ct) ||
			!m_wndSplitter1.CreateView(0, 1, RUNTIME_CLASS(CNCViewYZ), CSize(0, 0), &ct) ||
			!m_wndSplitter1.CreateView(1, 0, RUNTIME_CLASS(CNCViewXZ), CSize(0, 0), &ct) ||
			!m_wndSplitter1.CreateView(1, 1, RUNTIME_CLASS(CNCViewXY), CSize(0, 0), &ct) )
		return -1;
	// �S��-2
	if ( !m_wndSplitter2.CreateStatic(this, 1, 2, WS_CHILD) ||
			!m_wndSplitter22.CreateStatic(&m_wndSplitter2, 3, 1,
							WS_CHILD|WS_VISIBLE, m_wndSplitter2.IdFromRowCol(0, 0)) ||
			!m_wndSplitter22.CreateView(0, 0, RUNTIME_CLASS(CNCViewYZ), CSize(0, 0), &ct) ||
			!m_wndSplitter22.CreateView(1, 0, RUNTIME_CLASS(CNCViewXZ), CSize(0, 0), &ct) ||
			!m_wndSplitter22.CreateView(2, 0, RUNTIME_CLASS(CNCViewXY), CSize(0, 0), &ct) ||
			!m_wndSplitter2.CreateView(0, 1, RUNTIME_CLASS(CNCView),   CSize(0, 0), &ct) )
		return -1;

	// �e�߰���ޭ��̐���
	int		i, nIndex;
	CString	strXYZ[NCXYZ];
	for ( i=0; i<NCXYZ; i++ )
		strXYZ[i] = g_szNdelimiter[i];
	try {
		nIndex = AddPage(strXYZ[NCA_X]+strXYZ[NCA_Y]+strXYZ[NCA_Z],
			RUNTIME_CLASS(CNCView), GetDocument(), GetParentFrame());
		ASSERT(nIndex >= 0);
		nIndex = AddPage(strXYZ[NCA_X]+strXYZ[NCA_Y],
			RUNTIME_CLASS(CNCViewXY), GetDocument(), GetParentFrame());
		ASSERT(nIndex >= 0);
		nIndex = AddPage(strXYZ[NCA_X]+strXYZ[NCA_Z],
			RUNTIME_CLASS(CNCViewXZ), GetDocument(), GetParentFrame());
		ASSERT(nIndex >= 0);
		nIndex = AddPage(strXYZ[NCA_Y]+strXYZ[NCA_Z],
			RUNTIME_CLASS(CNCViewYZ), GetDocument(), GetParentFrame());
		ASSERT(nIndex >= 0);
		nIndex = AddPage("�S��-1", &m_wndSplitter1);
		ASSERT(nIndex >= 0);
		nIndex = AddPage("�S��-2", &m_wndSplitter2);
		ASSERT(nIndex >= 0);
		nIndex = AddPage("OpenGL",
			RUNTIME_CLASS(CNCViewGL), GetDocument(), GetParentFrame());
		// �e�߰�ނ����޲���÷������ق��擾
		CClientDC*	pDC;
		for ( i=0; i<NCVIEW_FOURSVIEW; i++ ) {	// �S�ʈȊO
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
		// �ڰ��`��گ�ނ̏I���w��
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

	CTabView::OnDestroy();
}

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
		m_evTrace.PulseEvent();	// �׸ޏ�Ԃ��̂܂܂��ݽï�ߎ��s
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
		// ���݈ʒu�ȍ~��`��
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
	// ؽĺ��۰ق̌��݈ʒu�擾
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
		// ؽĺ��۰ق̍s�ԍ������޼ު�Ĕԍ����������ĕ`��
		if ( GetDocument()->SetLineToTrace(FALSE, nLine) ) {
			m_bTracePause = m_bTraceContinue = TRUE;
			m_nTrace = ID_NCVIEW_TRACE_PAUSE;	// �����ňꎞ��~
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
			m_evTrace.PulseEvent();		// �ڰ��J�n
		}
		else {
			m_bTraceContinue = FALSE;
			m_nTrace = ID_NCVIEW_TRACE_STOP;
		}
	}

	GetDocument()->UpdateAllViews(this, UAV_TRACECURSOR);
	if ( nID == ID_NCVIEW_TRACE_CURSOR2 )
		GetDocument()->ResetTraceStart();	// ���̍ĕ`��ɔ�����
}

void CNCViewTab::OnUpdateAllFitCmd(CCmdUI* pCmdUI)
{
	int nIndex = GetActivePage();
	pCmdUI->Enable( NCVIEW_FOURSVIEW<=nIndex && nIndex<NCVIEW_OPENGL );
}

void CNCViewTab::OnUpdateBeforeView(CCmdUI* pCmdUI)
{
	int nIndex = GetActivePage();
	pCmdUI->Enable( nIndex < NCVIEW_OPENGL );
}

void CNCViewTab::OnUpdateDefViewInfo(CCmdUI *pCmdUI)
{
	int nIndex = GetActivePage();
	pCmdUI->Enable( nIndex == NCVIEW_OPENGL );
}

/////////////////////////////////////////////////////////////////////////////
// CTraceThread

BOOL CTraceThread::InitInstance()
{
#ifdef _DEBUG
	CMagaDbg	dbg("CTraceThread::InitInstance()\nStart", DBG_BLUE);
#endif
	CNCDoc*			pDoc  = m_pParent->GetDocument();
	CNCdata*		pData1;
	CNCdata*		pData2 = NULL;
	CNCViewSplit*	pWnd;
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	CDC		dc;
	int		nPage, nTraceDraw;
	BOOL	bBreak, bSelect;
	PFNNCDRAWPROC	pfnDrawProc[NCVIEW_FOURSVIEW];

	if ( pDoc->IsNCDocFlag(NCDOC_WIRE) ) {
		pfnDrawProc[0] = &(CNCdata::DrawWire);
		pfnDrawProc[1] = &(CNCdata::DrawWireXY);
		pfnDrawProc[2] = &(CNCdata::DrawWireXZ);
		pfnDrawProc[3] = &(CNCdata::DrawWireYZ);
	}
	else {
		pfnDrawProc[0] = &(CNCdata::Draw);
		pfnDrawProc[1] = &(CNCdata::DrawXY);
		pfnDrawProc[2] = &(CNCdata::DrawXZ);
		pfnDrawProc[3] = &(CNCdata::DrawYZ);
	}

	// �گ�ނ�ٰ��
	while ( TRUE ) {
		m_pParent->m_evTrace.Lock();
#ifdef _DEBUG
		dbg.printf("m_evTrace Lock() return");
#endif
		if ( !m_pParent->m_bTraceContinue )
			break;
		// �ڰ��J�n�E�ĊJ
		bSelect = pOpt->GetNCViewFlg(NCVIEWFLG_TRACEMARKER);
		do {
			nPage = m_pParent->GetActivePage();
			if ( nPage < 0 )
				continue;
			bBreak = pDoc->IncrementTrace(nTraceDraw);
			if ( nTraceDraw <= 0 ) {
				// �Ō�̑I������
				if ( bSelect && pData2 ) {
					if ( nPage < NCVIEW_FOURSVIEW ) {
						if ( !dc.Attach(m_pParent->m_hDC[nPage]) )
							::NCVC_CriticalErrorMsg(__FILE__, __LINE__);
						dc.SetROP2(R2_XORPEN);
						(pData2->*pfnDrawProc[nPage])(&dc, TRUE);
						dc.SetROP2(R2_COPYPEN);
						(pData2->*pfnDrawProc[nPage])(&dc, FALSE);
						dc.Detach();
					}
					else if ( nPage < NCVIEW_OPENGL ) {
						pWnd = static_cast<CNCViewSplit *>(m_pParent->GetPage(nPage));
						if ( pWnd ) {
							pWnd->DrawData(pData2, TRUE,  TRUE,  pfnDrawProc);
							pWnd->DrawData(pData2, FALSE, FALSE, pfnDrawProc);
						}
					}
				}
				// °����݂𑦎��X�V
				m_pParent->m_nTrace = ID_NCVIEW_TRACE_STOP;
				AfxGetNCVCMainWnd()->PostMessage(WM_NULL);
				// ���̍ĊJ�ɔ�����
				pData2 = NULL;
				break;
			}
			pData1 = pDoc->GetNCdata(nTraceDraw-1);
			m_pListView->SendMessage(WM_USERTRACESELECT, (WPARAM)pData1);
			if ( nPage < NCVIEW_FOURSVIEW ) {
				if ( !dc.Attach(m_pParent->m_hDC[nPage]) )
					::NCVC_CriticalErrorMsg(__FILE__, __LINE__);
				if ( bSelect && pData2 ) {
					dc.SetROP2(R2_XORPEN);
					(pData2->*pfnDrawProc[nPage])(&dc, TRUE);
					dc.SetROP2(R2_COPYPEN);
					(pData2->*pfnDrawProc[nPage])(&dc, FALSE);
				}
				(pData1->*pfnDrawProc[nPage])(&dc, bSelect);
				dc.Detach();
			}
			else if ( nPage < NCVIEW_OPENGL ) {
				pWnd = static_cast<CNCViewSplit *>(m_pParent->GetPage(nPage));
				if ( pWnd && bSelect && pData2 ) {
					pWnd->DrawData(pData2, TRUE,  TRUE,  pfnDrawProc);	// XOR PEN
					pWnd->DrawData(pData2, FALSE, FALSE, pfnDrawProc);	// COPY PEN
				}
				pWnd->DrawData(pData1, bSelect, FALSE, pfnDrawProc);
			}
			::GdiFlush();
			if ( bBreak ) {
				m_pParent->m_bTracePause = TRUE;
				m_pParent->m_nTrace = ID_NCVIEW_TRACE_PAUSE;
				AfxGetNCVCMainWnd()->PostMessage(WM_NULL);
			}
			else if ( m_pParent->m_nTrace != ID_NCVIEW_TRACE_PAUSE )
				::Sleep( pOpt->GetTraceSpeed(m_pParent->m_nTraceSpeed-ID_NCVIEW_TRACE_FAST) );
			// ���̑I�������p
			pData2 = pData1;
		} while ( m_pParent->m_bTraceContinue && !m_pParent->m_bTracePause );
#ifdef _DEBUG
		dbg.printf("Stop the trace loop");
#endif
		if ( !m_pParent->m_bTraceContinue )
			pData2 = FALSE;	// ���̍ĊJ�ɔ�����
	}

#ifdef _DEBUG
	dbg.printf("Finish");
#endif

	// InitInstance() ����ُ�I���ŕԂ��C�گ�ނ̏I��
	return FALSE;
}

int CTraceThread::ExitInstance()
{
	// InitInstance() �ňُ�I����Ԃ��̂Ő���I�����ނ�Ԃ�
	CWinThread::ExitInstance();
	return 0;	// �گ�ސ���I������
}
