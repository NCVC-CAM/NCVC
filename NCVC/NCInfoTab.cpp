// NCInfoTab.cpp: CNCInfoTab クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "NCDoc.h"
#include "NCInfoTab.h"
#include "NCInfoView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CNCInfoTab, CTabViewBase)

BEGIN_MESSAGE_MAP(CNCInfoTab, CTabViewBase)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	// ﾀﾌﾞ移動
	ON_COMMAND_RANGE(ID_TAB_NEXT, ID_TAB_PREV, &CNCInfoTab::OnMoveTab)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////
// 構築/消滅

CNCInfoTab::CNCInfoTab()
{
}

/////////////////////////////////////////////////////////////////////////////
// CNCInfoTab クラスのオーバライド関数

void CNCInfoTab::OnInitialUpdate() 
{
	__super::OnInitialUpdate();

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
	__super::OnUpdate(pSender, lHint, pHint);
}

BOOL CNCInfoTab::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
	// ﾀﾌﾞ移動だけ特別
	if ( nID == ID_TAB_NEXT || nID == ID_TAB_PREV ) {
		CWnd*	pWnd = GetFocus();
		if ( pWnd->IsKindOf(RUNTIME_CLASS(CNCInfoTab)) ||
				pWnd->IsKindOf(RUNTIME_CLASS(CNCInfoView1)) ||
				pWnd->IsKindOf(RUNTIME_CLASS(CNCInfoView2)) )
			return __super::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
		else
			return FALSE;
	}

	// 自分自身(CWnd)とｱｸﾃｨﾌﾞなﾋﾞｭｰだけにｺﾏﾝﾄﾞﾙｰﾃｨﾝｸﾞ
	// 結果的に CNCDoc へのｺﾏﾝﾄﾞﾙｰﾃｨﾝｸﾞはここからだけになる
	if ( __super::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo) )
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
// CNCInfoTab クラスの診断

#ifdef _DEBUG
void CNCInfoTab::AssertValid() const
{
	__super::AssertValid();
}

void CNCInfoTab::Dump(CDumpContext& dc) const
{
	__super::Dump(dc);
}

CNCDoc* CNCInfoTab::GetDocument() // 非デバッグ バージョンはインラインです。
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CNCDoc)));
	return static_cast<CNCDoc *>(m_pDocument);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CNCInfoTab クラスのメッセージ ハンドラ

int CNCInfoTab::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if ( __super::OnCreate(lpCreateStruct) < 0 )
		return -1;
	GetTabCtrl().SetFont(AfxGetNCVCMainWnd()->GetTextFont(TYPE_NCD), FALSE);

	// Style setting
//	DWORD dwStyle = GetTabCtrl().GetExtendedStyle();
//	GetTabCtrl().SetExtendedStyle( dwStyle | TCS_EX_FLATSEPARATORS );

	// 各ﾍﾟｰｼﾞﾋﾞｭｰの生成
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

	__super::OnDestroy();
}

void CNCInfoTab::OnMoveTab(UINT nID)
{
	if ( nID == ID_TAB_NEXT )
		NextActivatePage();
	else
		PrevActivatePage();
}
