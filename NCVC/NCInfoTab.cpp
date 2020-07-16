// NCInfoTab.cpp: CNCInfoTab クラスのインプリメンテーション
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
	// ﾀﾌﾞ移動
	ON_COMMAND_RANGE(ID_TAB_NEXT, ID_TAB_PREV, OnMoveTab)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////
// 構築/消滅

CNCInfoTab::CNCInfoTab()
{
}

CNCInfoTab::~CNCInfoTab()
{
}

/////////////////////////////////////////////////////////////////////////////
// CNCInfoTab クラスのオーバライド関数

void CNCInfoTab::OnInitialUpdate() 
{
	CTabView::OnInitialUpdate();

	//	ｱｸﾃｨﾌﾞﾍﾟｰｼﾞをﾚｼﾞｽﾄﾘから読み出し
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
		return;		// 再描画不要
	case UAV_CHANGEFONT:
		GetTabCtrl().SetFont(AfxGetNCVCMainWnd()->GetTextFont(TYPE_NCD), FALSE);
		break;
	}
	CTabView::OnUpdate(pSender, lHint, pHint);
}

BOOL CNCInfoTab::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
	// ﾀﾌﾞ移動だけ特別
	if ( nID == ID_TAB_NEXT || nID == ID_TAB_PREV ) {
		CWnd*	pWnd = GetFocus();
		if ( pWnd->IsKindOf(RUNTIME_CLASS(CNCInfoTab)) ||
				pWnd->IsKindOf(RUNTIME_CLASS(CNCInfoView1)) ||
				pWnd->IsKindOf(RUNTIME_CLASS(CNCInfoView2)) )
			return CTabView::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
		else
			return FALSE;
	}

	// 自分自身(CWnd)とｱｸﾃｨﾌﾞなﾋﾞｭｰだけにｺﾏﾝﾄﾞﾙｰﾃｨﾝｸﾞ
	// 結果的に CNCDoc へのｺﾏﾝﾄﾞﾙｰﾃｨﾝｸﾞはここからだけになる
	CWnd*	pWnd = GetFocus();
	if ( CTabView::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo) )
		return TRUE;
	if ( GetPageCount() <= 0 )
		return FALSE;
	return GetActivePageWnd()->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

void CNCInfoTab::OnActivatePage(int nIndex)
{
	GetParentFrame()->SetActiveView((CView *)GetPage(nIndex));
}

/////////////////////////////////////////////////////////////////////////////
// CNCInfoTab クラスの診断

#ifdef _DEBUG
void CNCInfoTab::AssertValid() const
{
	CTabView::AssertValid();
}

void CNCInfoTab::Dump(CDumpContext& dc) const
{
	CTabView::Dump(dc);
}

CNCDoc* CNCInfoTab::GetDocument() // 非デバッグ バージョンはインラインです。
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CNCDoc)));
	return (CNCDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CNCInfoTab クラスのメッセージ ハンドラ

int CNCInfoTab::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CTabView::OnCreate(lpCreateStruct) == -1)
		return -1;
	GetTabCtrl().SetFont(AfxGetNCVCMainWnd()->GetTextFont(TYPE_NCD), FALSE);

	// 各ﾍﾟｰｼﾞﾋﾞｭｰの生成
	int		nIndex;
	CString	strTitle;
	try {
		VERIFY(strTitle.LoadString(IDS_TAB_INFO2));
		nIndex = AddPage(strTitle,
			RUNTIME_CLASS(CNCInfoView1), GetDocument(), GetParentFrame());
		ASSERT(nIndex != -1);
		VERIFY(strTitle.LoadString(IDS_TAB_INFO3));
		nIndex = AddPage(strTitle,
			RUNTIME_CLASS(CNCInfoView2), GetDocument(), GetParentFrame());
		ASSERT(nIndex != -1);
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
