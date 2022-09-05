// TabViewBase.cpp: CTabViewBase クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TabViewBase.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CTabViewBase, CCtrlView)

BEGIN_MESSAGE_MAP(CTabViewBase, CCtrlView)
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_WM_ERASEBKGND()
	ON_NOTIFY_REFLECT(TCN_SELCHANGING, &CTabViewBase::OnSelChanging)
	ON_NOTIFY_REFLECT(TCN_SELCHANGE, &CTabViewBase::OnSelChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTabViewBase クラスのメンバ関数

int CTabViewBase::AddPage
	(LPCTSTR pszTitle, CRuntimeClass* pClass, CDocument* pDoc, CFrameWnd* pFrameWnd)
{
	CCreateContext	ct;
	ct.m_pCurrentDoc = pDoc;
	ct.m_pCurrentFrame = pFrameWnd;
	ct.m_pLastView = NULL;
	ct.m_pNewDocTemplate = pDoc->GetDocTemplate();
	ct.m_pNewViewClass = pClass;

	CWnd*	pWnd = static_cast<CWnd *>(pClass->CreateObject());
	if ( !pWnd )
		return -1;
	if ( !pWnd->Create(NULL, NULL, AFX_WS_DEFAULT_VIEW,
				CRect(0, 0, 0, 0), this, 0, &ct) ) {
		TRACE0(_T("Failed to create AddPageView.\n"));
		return -1;
	}

	return AddPage(pszTitle, pWnd);
}

int CTabViewBase::AddPage(LPCTSTR pszTitle, CWnd* pWnd)
{
	TC_ITEM tci;
	tci.mask	= TCIF_TEXT;
	tci.pszText	= const_cast<LPTSTR>(pszTitle);
	int	nIndex = GetTabCtrl().GetItemCount();
	GetTabCtrl().InsertItem(nIndex, &tci);

	if ( !OnInitPage(nIndex) )
		return -1;

	m_pPages.Add(pWnd);

	return nIndex;
}

void CTabViewBase::RemovePage(int nIndex)
{
	ASSERT( nIndex >= 0 && nIndex < GetPageCount() );

	// Notify derived classes that the page is being destroyed.
	OnDestroyPage(nIndex);

	// Switch pages if the page being deleted is the current page and it's
	// not the only remaining page.
	int nCount = GetPageCount();
	if ( nCount>1 && nIndex==GetActivePage() ) {
		int nPage = nIndex + 1;
		if ( nPage >= nCount )
			nPage = nCount - 2;
		ActivatePage(nPage);
	}

	// Remove the page from the tab control.
	GetTabCtrl().DeleteItem(nIndex);

	// Destroy the dialog (if any) that represents the page.
	CWnd* pWnd = m_pPages[nIndex];
	if ( pWnd )
		pWnd->DestroyWindow();	

	// Clean up, repaint, and return.
	m_pPages.RemoveAt(nIndex);

	Invalidate();
}

BOOL CTabViewBase::GetPageTitle(int nIndex, CString &strTitle)
{
	ASSERT( nIndex >= 0 && nIndex < GetPageCount() );

	TCHAR szTitle[1024];
	TC_ITEM item;
	item.mask = TCIF_TEXT;
	item.pszText = szTitle;
	item.cchTextMax = sizeof(szTitle);

	if ( !GetTabCtrl().GetItem(nIndex, &item) )
		return FALSE;

	strTitle = item.pszText;
	return TRUE;
}

BOOL CTabViewBase::SetPageTitle(int nIndex, LPCTSTR pszTitle)
{
	ASSERT( nIndex >= 0 && nIndex < GetPageCount() );

	TC_ITEM item;
	item.mask = TCIF_TEXT;
	item.pszText = const_cast<LPTSTR>(pszTitle);
	
	BOOL bResult = GetTabCtrl().SetItem(nIndex, &item);
	if ( bResult )
		Invalidate();
	return bResult;
}

void CTabViewBase::ActivatePage(int nIndex)
{
	ASSERT( nIndex >= 0 && nIndex < GetPageCount() );

	// Deactivate the current page.
	int nOldIndex = GetActivePage();

	// Do nothing if the specified page is already active.
	if ( nIndex == nOldIndex )
		return;

	CWnd* pWnd;
	if ( nOldIndex >= 0 ) {
		OnDeactivatePage(nOldIndex);
		pWnd = m_pPages[nOldIndex];
		if ( pWnd ) {
//			pWnd->EnableWindow(FALSE);
			pWnd->ShowWindow(SW_HIDE);
		}
	}

	// Activate the new one.
	GetTabCtrl().SetCurSel(nIndex);
	pWnd = m_pPages[nIndex];
	if ( pWnd ) {
		ResizePage(pWnd);
//		pWnd->EnableWindow(TRUE);
		pWnd->ShowWindow(SW_SHOW);
	}
	OnActivatePage(nIndex);
}

int CTabViewBase::NextActivatePage(void)
{
	int	nIndex = GetActivePage() + 1;
	if ( nIndex < 0 || nIndex >= GetPageCount() )
		nIndex = 0;
	ActivatePage(nIndex);
	return nIndex;
}

int CTabViewBase::PrevActivatePage(void)
{
	int	nIndex = GetActivePage() - 1;
	if ( nIndex < 0 || nIndex >= GetPageCount() )
		nIndex = GetPageCount() - 1;
	ActivatePage(nIndex);
	return nIndex;
}

void CTabViewBase::ResizePage(CWnd* pWnd)
{
	CRect	rc;
	GetTabCtrl().GetClientRect(rc);
	GetTabCtrl().AdjustRect(FALSE, rc);
	pWnd->MoveWindow(rc);
}

/////////////////////////////////////////////////////////////////////////////
// CTabViewBase クラスのメッセージ ハンドラ

void CTabViewBase::OnSize(UINT nType, int cx, int cy)
{
	// When the view's size changes, resize the dialog (if any) shown in the
	// view to prevent the dialog from clipping the view's inside border.
	__super::OnSize (nType, cx, cy);
	if ( GetPageCount() > 0 )
		ResizePage(GetActivePageWnd());
}

void CTabViewBase::OnDestroy() 
{
	m_pPages.RemoveAll();
	__super::OnDestroy();
}

BOOL CTabViewBase::OnEraseBkgnd(CDC* pDC)
{
	// -----------------------------------------------------------
	// VS2012から、これがないとタブの背景が描画されない不具合...
	// なので自分で描画
	// -----------------------------------------------------------
	CRect	rc;
	GetClientRect(rc);
	pDC->FillSolidRect(&rc, ::GetSysColor(COLOR_INACTIVEBORDER));
	return TRUE;
}

void CTabViewBase::OnSelChanging(NMHDR* pNMHDR, LRESULT* pResult)
{
	// Notify derived classes that the selection is changing.
	int nIndex = GetActivePage();
	if ( nIndex < 0 )
		return;

	OnDeactivatePage(nIndex);

	// Save the input focus and hide the old page.
	CWnd* pWnd = m_pPages[nIndex];
	if ( pWnd ) {
//		pWnd->EnableWindow(FALSE);
		pWnd->ShowWindow(SW_HIDE);
	}
	*pResult = 0;
}

void CTabViewBase::OnSelChange(NMHDR* pNMHDR, LRESULT* pResult)
{
	int nIndex = GetActivePage();
	if ( nIndex < 0 )
		return;

	// Show the new page.
	CWnd* pWnd = m_pPages[nIndex];
	if ( pWnd ) {
		ResizePage(pWnd);
//		pWnd->EnableWindow(TRUE);
		pWnd->ShowWindow(SW_SHOW);
	}

	// Notify derived classes that the selection has changed.
	OnActivatePage(nIndex);
	*pResult = 0;
}
