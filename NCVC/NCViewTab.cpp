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

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CNCViewTab, CTabViewBase)

BEGIN_MESSAGE_MAP(CNCViewTab, CTabViewBase)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	// ��ވړ�
	ON_COMMAND_RANGE(ID_TAB_NEXT, ID_TAB_PREV, &CNCViewTab::OnMoveTab)
	// �ڰ�
	ON_UPDATE_COMMAND_UI_RANGE(ID_NCVIEW_TRACE_FAST, ID_NCVIEW_TRACE_LOW, &CNCViewTab::OnUpdateTraceSpeed)
	ON_COMMAND_RANGE(ID_NCVIEW_TRACE_FAST, ID_NCVIEW_TRACE_LOW, &CNCViewTab::OnTraceSpeed)
	ON_UPDATE_COMMAND_UI(ID_NCVIEW_TRACE_RUN, &CNCViewTab::OnUpdateTraceRun)
	ON_COMMAND(ID_NCVIEW_TRACE_RUN, &CNCViewTab::OnTraceRun)
	ON_UPDATE_COMMAND_UI(ID_NCVIEW_TRACE_PAUSE, &CNCViewTab::OnUpdateTracePause)
	ON_COMMAND(ID_NCVIEW_TRACE_PAUSE, &CNCViewTab::OnTracePause)
	ON_COMMAND(ID_NCVIEW_TRACE_STOP, &CNCViewTab::OnTraceStop)
	ON_UPDATE_COMMAND_UI_RANGE(ID_NCVIEW_TRACE_CURSOR, ID_NCVIEW_TRACE_CURSOR2, &CNCViewTab::OnUpdateTraceCursor)
	ON_COMMAND_RANGE(ID_NCVIEW_TRACE_CURSOR, ID_NCVIEW_TRACE_CURSOR2, &CNCViewTab::OnTraceCursor)
	// �u�S�Ă��߲݂̐}�`̨�āv�ƭ�����ނ̎g�p����
	ON_UPDATE_COMMAND_UI(ID_NCVIEW_ALLFIT, &CNCViewTab::OnUpdateAllFitCmd)
	// ��
	ON_UPDATE_COMMAND_UI(ID_OPTION_DEFVIEWINFO, &CNCViewTab::OnUpdateDefViewInfo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, &CNCViewTab::OnUpdateEditCopy)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_UP,  ID_VIEW_RT,  &CNCViewTab::OnUpdateMoveKey)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_RUP, ID_VIEW_RRT, &CNCViewTab::OnUpdateRoundKey)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////
// �\�z/����
//////////////////////////////////////////////////////////////////////

CNCViewTab::CNCViewTab() : m_evTrace(FALSE, TRUE)
{
#ifdef _DEBUG_FILEOPEN
	printf("CNCViewTab::CNCViewTab() Start\n");
#endif
/*
	�������ڰ���߰�ނ�ڼ޽�؂���擾����ƁCNCViewTab�N���̓x�ɱ�������邱�ƂɂȂ�
	�ȉ��C��è���߰�ޏ�������
*/
	m_nTraceSpeed = AfxGetNCVCApp()->GetTraceSpeed();
	m_pTraceThread = NULL;
	m_bTraceContinue = m_bTracePause = FALSE;
	ZEROCLR(m_bSplit);	// m_bSplit[i++]=FALSE
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewTab �N���X�̃I�[�o���C�h�֐�

void CNCViewTab::OnInitialUpdate() 
{
	__super::OnInitialUpdate();

	// �ڰ��`��گ�ސ���
	// --- OnCreate�ł�IsDocFlag()���Ԃɍ���Ȃ�
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
	if ( nPage < GetPageCount() )
		ActivatePage(nPage);

	// CNCViewSplit�̻��ޒ���
	// ���ݱ�è���߰�ނƓ������Ƃ��́C����Ɋe�ޭ�WM_USERVIEWFITMSG���M
	m_wndSplitter1.PostMessage(WM_USERINITIALUPDATE, NCDRAWVIEW_NUM,   nPage==NCDRAWVIEW_NUM);
	m_wndSplitter2.PostMessage(WM_USERINITIALUPDATE, NCDRAWVIEW_NUM+1, nPage==NCDRAWVIEW_NUM+1);

	if ( nPage<NCDRAWVIEW_NUM || nPage==NCVIEW_OPENGL ) {
		// �}�`̨��ү���ނ̑��M
		// �e�ޭ���OnInitialUpdate()�֐����ł́CGetClientRect()�̻��ނ��������Ȃ�
		CWnd*	pWnd = GetPage(nPage);
		if ( pWnd )
			pWnd->PostMessage(WM_USERVIEWFITMSG, 0, 1);
	}
	else {
		// ���د��\���̏ꍇ�́C�g�嗦�̍čX�V
		for ( auto& ref : m_bSplit ) ref = TRUE;	// m_bSplit[i++]=TRUE
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
	__super::OnUpdate(pSender, lHint, pHint);
}

BOOL CNCViewTab::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
#ifdef _DEBUG_CMDMSG
	printf("CNCViewTab::OnCmdMsg()\n");
#endif
	// ��ވړ���������
	if ( nID == ID_TAB_NEXT || nID == ID_TAB_PREV ) {
		CWnd*	pWnd = GetFocus();
		if ( pWnd->IsKindOf(RUNTIME_CLASS(CNCViewTab)) ||
				pWnd->IsKindOf(RUNTIME_CLASS(CNCView)) ||
				pWnd->IsKindOf(RUNTIME_CLASS(CNCViewXY)) ||
				pWnd->IsKindOf(RUNTIME_CLASS(CNCViewXZ)) ||
				pWnd->IsKindOf(RUNTIME_CLASS(CNCViewYZ)) ||
				pWnd->IsKindOf(RUNTIME_CLASS(CNCViewGL)) )
			return __super::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
		else
			return FALSE;
	}

	// �������g�Ʊ�è�ނ��ޭ������ɺ����ٰèݸ�
	// ���ʓI�� CNCDoc �ւ̺����ٰèݸނ͂������炾���ɂȂ�
	if ( __super::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo) ) {
#ifdef _DEBUG_CMDMSG
		printf("CTabViewBase::OnCmdMsg() return\n");
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
		// �S�ʕ\���ƒP�ꕽ�ʕ\���̊g�嗦�X�V�̂��߂�ү���ޑ��M
		LPARAM	lSplit = nIndex < NCDRAWVIEW_NUM ? (LPARAM)m_bSplit[nIndex] : 0;
		// WPARAM��CNCViewSplit::OnUserActivatePage()�����̂�
		pWnd->SendMessage(WM_USERACTIVATEPAGE, (WPARAM)nIndex, lSplit);
	}
	if ( nIndex < NCDRAWVIEW_NUM ) {
		m_bSplit[nIndex] = FALSE;
		GetParentFrame()->SetActiveView(static_cast<CView *>(pWnd));
	}
	else if ( nIndex < NCVIEW_OPENGL ) {
		// ���د��\���̏ꍇ�͏�Ɋg�嗦�̍čX�V
		for ( auto& ref : m_bSplit ) ref = TRUE;	// m_bSplit[i++]=TRUE
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
	__super::AssertValid();
}

void CNCViewTab::Dump(CDumpContext& dc) const
{
	__super::Dump(dc);
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
#ifdef _DEBUG_FILEOPEN
	printf("CNCViewTab::OnCreate() Start\n");
#endif
	extern	LPCTSTR	g_szNdelimiter;		// "XYZUVWIJKRPLDH" from NCDoc.cpp

	if ( __super::OnCreate(lpCreateStruct) < 0 )
		return -1;
	GetTabCtrl().SetFont(AfxGetNCVCMainWnd()->GetTextFont(TYPE_NCD), FALSE);

	int		i, nIndex;
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	//	�ÓI���د��쐬
	CCreateContext	ct;
	ct.m_pCurrentDoc = GetDocument();
	ct.m_pCurrentFrame = GetParentFrame();
	ct.m_pLastView = NULL;
	ct.m_pNewDocTemplate = GetDocument()->GetDocTemplate();
	ct.m_pNewViewClass = NULL;
	CRuntimeClass*	pNCView[NCDRAWVIEW_NUM];
	CRuntimeClass*	pView01Class[NCDRAWVIEW_NUM];
	CRuntimeClass*	pView02Class[NCDRAWVIEW_NUM];
	pNCView[0] = RUNTIME_CLASS(CNCView);
	pNCView[1] = RUNTIME_CLASS(CNCViewXY);
	pNCView[2] = RUNTIME_CLASS(CNCViewXZ);
	pNCView[3] = RUNTIME_CLASS(CNCViewYZ);
	for ( i=0; i<SIZEOF(pView01Class); i++ ) {
		pView01Class[i] = pNCView[ pOpt->GetForceView01(i) ];
		pView02Class[i] = pNCView[ pOpt->GetForceView02(i) ];
	}
	// �S��-1
	if ( !m_wndSplitter1.CreateStatic(this, 2, 2, WS_CHILD) ||
			!m_wndSplitter1.CreateView(0, 0, pView01Class[0], CSize(0, 0), &ct) ||
			!m_wndSplitter1.CreateView(0, 1, pView01Class[1], CSize(0, 0), &ct) ||
			!m_wndSplitter1.CreateView(1, 0, pView01Class[2], CSize(0, 0), &ct) ||
			!m_wndSplitter1.CreateView(1, 1, pView01Class[3], CSize(0, 0), &ct) )
		return -1;
	// �S��-2
	if ( !m_wndSplitter2.CreateStatic(this, 1, 2, WS_CHILD) ||
			!m_wndSplitter22.CreateStatic(&m_wndSplitter2, 3, 1,
							WS_CHILD|WS_VISIBLE, m_wndSplitter2.IdFromRowCol(0, 0)) ||
			!m_wndSplitter22.CreateView(0, 0, pView02Class[0], CSize(0, 0), &ct) ||
			!m_wndSplitter22.CreateView(1, 0, pView02Class[1], CSize(0, 0), &ct) ||
			!m_wndSplitter22.CreateView(2, 0, pView02Class[2], CSize(0, 0), &ct) ||
			!m_wndSplitter2.CreateView (0, 1, pView02Class[3], CSize(0, 0), &ct) )
		return -1;

	// �e�߰���ޭ��̐���
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
		nIndex = AddPage("�S��-1", &m_wndSplitter1);
		ASSERT(nIndex >= 0);
		nIndex = AddPage("�S��-2", &m_wndSplitter2);
		ASSERT(nIndex >= 0);
		if ( pOpt->GetNCViewFlg(GLOPTFLG_SOLIDVIEW) ) {
			nIndex = AddPage("OpenGL",
				RUNTIME_CLASS(CNCViewGL), GetDocument(), GetParentFrame());
//			ASSERT(nIndex >= 0);	// OpenGL�ް�ޮ������Ɉ���������\���A��
		}
		// �e�߰�ނ����޲���÷������ق��擾
		CClientDC*	pDC;
		for ( i=0; i<NCDRAWVIEW_NUM; i++ ) {	// �S�ʈȊO
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
	printf("CNCViewTab::OnDestroy()\n");
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
			printf("WaitForSingleObject() Fail!\n");
			::NC_FormatMessage();
		}
		else
			printf("WaitForSingleObject() OK\n");
#else
		::WaitForSingleObject(m_pTraceThread->m_hThread, INFINITE);
#endif
		delete	m_pTraceThread;
	}

	__super::OnDestroy();
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
	if ( GetDocument()->IsDocFlag(NCDOC_ERROR) ) {
		pCmdUI->Enable(FALSE);
	}
	else {
#ifdef NO_TRACE_WORKFILE
		if ( GetDocument()->IsDocFlag(NCDOC_WORKFILE) )
			pCmdUI->Enable(FALSE);	// ���Ƃ�������Ȃ񂾂���...
		else
			pCmdUI->SetCheck( GetDocument()->GetTraceMode() == ID_NCVIEW_TRACE_RUN );
#else
		pCmdUI->SetCheck( GetDocument()->GetTraceMode() == ID_NCVIEW_TRACE_RUN );
#endif
	}
}

void CNCViewTab::OnTraceRun() 
{
	switch ( GetDocument()->GetTraceMode() ) {
	case ID_NCVIEW_TRACE_STOP:
		m_bTraceContinue = TRUE;
		m_bTracePause = FALSE;
		GetDocument()->SetTraceMode(ID_NCVIEW_TRACE_RUN);
		GetDocument()->StartTrace();
		GetDocument()->UpdateAllViews(this, UAV_STARTSTOPTRACE);
		// through
	case ID_NCVIEW_TRACE_PAUSE:
		m_evTrace.SetEvent();	// �׸ޏ�Ԃ��̂܂܂��ݽï�ߎ��s
		break;
	}
}

void CNCViewTab::OnUpdateTracePause(CCmdUI* pCmdUI) 
{
	if ( GetDocument()->IsDocFlag(NCDOC_ERROR) ) {
		pCmdUI->Enable(FALSE);
	}
	else {
#ifdef NO_TRACE_WORKFILE
		if ( GetDocument()->IsDocFlag(NCDOC_WORKFILE) )
			pCmdUI->Enable(FALSE);
		else
			pCmdUI->SetCheck( GetDocument()->GetTraceMode() == ID_NCVIEW_TRACE_PAUSE );
#else
		pCmdUI->SetCheck( GetDocument()->GetTraceMode() == ID_NCVIEW_TRACE_PAUSE );
#endif
	}
}

void CNCViewTab::OnTracePause() 
{
	switch ( GetDocument()->GetTraceMode() ) {
	case ID_NCVIEW_TRACE_RUN:
		m_bTracePause = TRUE;
		GetDocument()->SetTraceMode(ID_NCVIEW_TRACE_PAUSE);
		break;
	case ID_NCVIEW_TRACE_STOP:
		m_bTracePause = m_bTraceContinue = TRUE;
		GetDocument()->SetTraceMode(ID_NCVIEW_TRACE_PAUSE);
		GetDocument()->StartTrace();
		GetDocument()->UpdateAllViews(this, UAV_STARTSTOPTRACE);
		break;
	case ID_NCVIEW_TRACE_PAUSE:
		m_bTracePause = FALSE;
		GetDocument()->SetTraceMode(ID_NCVIEW_TRACE_RUN);
		m_evTrace.SetEvent();
		break;
	}
}

void CNCViewTab::OnTraceStop() 
{
	if ( GetDocument()->GetTraceMode() != ID_NCVIEW_TRACE_STOP ) {
		m_bTraceContinue = FALSE;
		GetDocument()->SetTraceMode(ID_NCVIEW_TRACE_STOP);
		// ���݈ʒu�ȍ~��`��
		GetDocument()->StopTrace();
		GetDocument()->UpdateAllViews(this, UAV_STARTSTOPTRACE);
	}
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
		GetDocument()->SetTraceMode(ID_NCVIEW_TRACE_STOP);
		GetDocument()->StopTrace();
		GetDocument()->UpdateAllViews(this, UAV_STARTSTOPTRACE);
		return;
	}

	int	nLine = pList->GetListCtrl().GetNextSelectedItem(pos);
	if ( nID == ID_NCVIEW_TRACE_CURSOR ) {
		// ؽĺ��۰ق̍s�ԍ������޼ު�Ĕԍ����������ĕ`��
		if ( GetDocument()->SetLineToTrace(FALSE, nLine) ) {
			m_bTracePause = m_bTraceContinue = TRUE;
			GetDocument()->SetTraceMode(ID_NCVIEW_TRACE_PAUSE);	// �����ňꎞ��~
		}
		else {
			m_bTraceContinue = FALSE;
			GetDocument()->SetTraceMode(ID_NCVIEW_TRACE_STOP);
		}
	}
	else {
		if ( GetDocument()->SetLineToTrace(TRUE, nLine) ) {
			m_bTraceContinue = TRUE;
			m_bTracePause = FALSE;
			GetDocument()->SetTraceMode(ID_NCVIEW_TRACE_RUN);
		}
		else {
			m_bTraceContinue = FALSE;
			GetDocument()->SetTraceMode(ID_NCVIEW_TRACE_STOP);
		}
	}

	GetDocument()->UpdateAllViews(this, UAV_TRACECURSOR, (CObject*)(UINT_PTR)nID);

	if ( nID == ID_NCVIEW_TRACE_CURSOR2 ) {
		if ( m_bTraceContinue )
			m_evTrace.SetEvent();			// �ڰ��J�n
		GetDocument()->ResetTraceStart();	// ���̍ĕ`��ɔ�����
	}
}

void CNCViewTab::OnUpdateAllFitCmd(CCmdUI* pCmdUI)
{
	int nIndex = GetActivePage();
	pCmdUI->Enable( NCDRAWVIEW_NUM<=nIndex && nIndex<NCVIEW_OPENGL );
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
	printf("CTraceThread::InitInstance() Start\n");
#endif
	CNCDoc*			pDoc  = m_pParent->GetDocument();
	CNCdata*		pData;
	CNCViewGL*		pWndGL;
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	int		nPage;
	INT_PTR	nTraceDraw;
	BOOL	bBreak;
	boost::optional<INT_PTR>	nGLDraw;

	// �X���b�h��main�������[�v
	while ( TRUE ) {
		m_pParent->m_evTrace.Lock();
#ifdef _DEBUG
		printf("m_evTrace Lock() return\n");
#endif
		m_pParent->m_evTrace.ResetEvent();
		if ( !m_pParent->m_bTraceContinue )
			break;

		// OpenGL�^�u�ȊO�Ɉړ��������Ƃ��̍ĊJ�ԍ�
		nGLDraw.reset();

		// OpenGL�^�u�̃E�B���h�E�n���h�����擾
		pWndGL = static_cast<CNCViewGL *>(m_pParent->GetPage(NCVIEW_OPENGL));

		// �g���[�X�J�n�E�ĊJ
		do {
			bBreak = pDoc->IncrementTrace(nTraceDraw);
			if ( nTraceDraw < 0 ) {
				// �c�[���{�^���𑦎��X�V
				pDoc->SetTraceMode(ID_NCVIEW_TRACE_STOP);
				AfxGetNCVCMainWnd()->PostMessage(WM_NULL);
				// ���̎��s���ɁC�O��̕`����c���Ȃ�����
				m_pListView->SendMessage(WM_USERTRACESELECT);
				break;
			}
			// ���ݑI���f�[�^
			pData = pDoc->GetNCdata(nTraceDraw-1);
			// �A�N�e�B�u�ȃ^�u���擾
			nPage = m_pParent->GetActivePage();
			if ( nPage==NCVIEW_OPENGL && pWndGL ) {
				// OpenGL�^�u����SendMessage()
				if ( nGLDraw ) {
					pWndGL->SendMessage(WM_USERTRACESELECT, (WPARAM)(*nGLDraw), (LPARAM)nTraceDraw);
					nGLDraw.reset();
				}
				else {
					pWndGL->SendMessage(WM_USERTRACESELECT, (WPARAM)pData);
				}
				Sleep(0);
			}
			else {
				// OpenGL�^�u�ȊO�̎��͂��̔ԍ����L�^
				if ( !nGLDraw )
					nGLDraw = nTraceDraw - 1;
			}
			// ���X�g�̑I��
			// ���ʓI�ɂ�������e�r���[�̍X�V
			m_pListView->SendMessage(WM_USERTRACESELECT, (WPARAM)pData);
			//
			if ( bBreak ) {
				m_pParent->m_bTracePause = TRUE;
				pDoc->SetTraceMode(ID_NCVIEW_TRACE_PAUSE);
				AfxGetNCVCMainWnd()->PostMessage(WM_NULL);
			}
			else if ( pDoc->GetTraceMode() != ID_NCVIEW_TRACE_PAUSE )
				::Sleep( pOpt->GetTraceSpeed(m_pParent->m_nTraceSpeed-ID_NCVIEW_TRACE_FAST) );

		} while ( m_pParent->m_bTraceContinue && !m_pParent->m_bTracePause );
#ifdef _DEBUG
		printf("Stop the trace loop\n");
#endif
	}

#ifdef _DEBUG
	printf("CTraceThread::InitInstance() end\n");
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
