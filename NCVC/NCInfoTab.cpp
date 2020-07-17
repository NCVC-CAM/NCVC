// NCInfoTab.cpp: CNCInfoTab �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "NCDoc.h"
#include "NCInfoTab.h"
#include "NCInfoView.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

IMPLEMENT_DYNCREATE(CNCInfoTab, CTabView)

BEGIN_MESSAGE_MAP(CNCInfoTab, CTabView)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SETFOCUS()
	// ��ވړ�
	ON_COMMAND_RANGE(ID_TAB_NEXT, ID_TAB_PREV, OnMoveTab)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////
// �\�z/����

CNCInfoTab::CNCInfoTab()
{
}

CNCInfoTab::~CNCInfoTab()
{
}

/////////////////////////////////////////////////////////////////////////////
// CNCInfoTab �N���X�̃I�[�o���C�h�֐�

void CNCInfoTab::OnInitialUpdate() 
{
	CTabView::OnInitialUpdate();

	//	��è���߰�ނ�ڼ޽�؂���ǂݏo��
	CString	strRegKey, strEntry;
	VERIFY(strRegKey.LoadString(IDS_REGKEY_NC));
	VERIFY(strEntry.LoadString(IDS_REG_NCV_INFOPAGE));
	int nPage = AfxGetApp()->GetProfileInt(strRegKey, strEntry, 0);
	if ( nPage < 0 || nPage >= GetPageCount() )
		nPage = 0;
	ActivatePage(nPage);
}

void CNCInfoTab::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
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

BOOL CNCInfoTab::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
	// ��ވړ���������
	if ( nID == ID_TAB_NEXT || nID == ID_TAB_PREV ) {
		CWnd*	pWnd = GetFocus();
		if ( pWnd->IsKindOf(RUNTIME_CLASS(CNCInfoTab)) ||
				pWnd->IsKindOf(RUNTIME_CLASS(CNCInfoView1)) ||
				pWnd->IsKindOf(RUNTIME_CLASS(CNCInfoView2)) )
			return CTabView::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
		else
			return FALSE;
	}

	// �������g(CWnd)�Ʊ�è�ނ��ޭ������ɺ����ٰèݸ�
	// ���ʓI�� CNCDoc �ւ̺����ٰèݸނ͂������炾���ɂȂ�
	CWnd*	pWnd = GetFocus();
	if ( CTabView::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo) )
		return TRUE;
	if ( GetPageCount() <= 0 )
		return FALSE;
	return GetActivePageWnd()->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

void CNCInfoTab::OnActivatePage(int nIndex)
{
	GetParentFrame()->SetActiveView(static_cast<CView *>(GetPage(nIndex)));
}

/////////////////////////////////////////////////////////////////////////////
// CNCInfoTab �N���X�̐f�f

#ifdef _DEBUG
void CNCInfoTab::AssertValid() const
{
	CTabView::AssertValid();
}

void CNCInfoTab::Dump(CDumpContext& dc) const
{
	CTabView::Dump(dc);
}

CNCDoc* CNCInfoTab::GetDocument() // ��f�o�b�O �o�[�W�����̓C�����C���ł��B
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CNCDoc)));
	return static_cast<CNCDoc *>(m_pDocument);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CNCInfoTab �N���X�̃��b�Z�[�W �n���h��

int CNCInfoTab::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if ( CTabView::OnCreate(lpCreateStruct) < 0 )
		return -1;
	GetTabCtrl().SetFont(AfxGetNCVCMainWnd()->GetTextFont(TYPE_NCD), FALSE);

	// �e�߰���ޭ��̐���
	int		nIndex;
	CString	strTitle;
	try {
		VERIFY(strTitle.LoadString(IDS_TAB_INFO2));
		nIndex = AddPage(strTitle,
			RUNTIME_CLASS(CNCInfoView1), GetDocument(), GetParentFrame());
		ASSERT(nIndex >= 0);
		VERIFY(strTitle.LoadString(IDS_TAB_INFO3));
		nIndex = AddPage(strTitle,
			RUNTIME_CLASS(CNCInfoView2), GetDocument(), GetParentFrame());
		ASSERT(nIndex >= 0);
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		return -1;
	}

	return 0;
}

void CNCInfoTab::OnDestroy() 
{
	CString	strRegKey, strEntry;
	VERIFY(strRegKey.LoadString(IDS_REGKEY_NC));
	VERIFY(strEntry.LoadString(IDS_REG_NCV_INFOPAGE));
	AfxGetApp()->WriteProfileInt(strRegKey, strEntry, GetActivePage());

	CTabView::OnDestroy();
}

void CNCInfoTab::OnSetFocus(CWnd*) 
{
	if ( GetActivePage() > 0 )
		GetActivePageWnd()->SetFocus();
}

void CNCInfoTab::OnMoveTab(UINT nID)
{
	if ( nID == ID_TAB_NEXT )
		NextActivatePage();
	else
		PrevActivatePage();
}
