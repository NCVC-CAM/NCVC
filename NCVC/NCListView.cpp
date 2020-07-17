// NCListView.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "NCChild.h"
#include "NCdata.h"
#include "NCDoc.h"
#include "NCViewTab.h"
#include "NCInfoTab.h"
#include "NCListView.h"
#include "NCJumpDlg.h"
#include "NCFindDlg.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

using namespace boost;

// ｲﾒｰｼﾞ表示ｲﾝﾃﾞｯｸｽ
enum {
	LISTIMG_NORMAL=0, LISTIMG_BREAK, LISTIMG_ERROR, LISTIMG_FOLDER,
};

/////////////////////////////////////////////////////////////////////////////
// CNCListView

IMPLEMENT_DYNCREATE(CNCListView, CListView)

BEGIN_MESSAGE_MAP(CNCListView, CListView)
	//{{AFX_MSG_MAP(CNCListView)
	ON_WM_CREATE()
	ON_WM_CONTEXTMENU()
	ON_NOTIFY_REFLECT(LVN_KEYDOWN, &CNCListView::OnKeyDown)
	ON_NOTIFY_REFLECT(LVN_GETDISPINFO, &CNCListView::OnGetDispInfo)
	ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, &CNCListView::OnItemChanged)
	ON_UPDATE_COMMAND_UI(ID_NCVIEW_TRACE_BREAK, &CNCListView::OnUpdateTraceBreak)
	ON_UPDATE_COMMAND_UI(ID_NCVIEW_JUMP, &CNCListView::OnUpdateViewJump)
	ON_UPDATE_COMMAND_UI(ID_EDIT_FIND, &CNCListView::OnUpdateViewFind)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, &CNCListView::OnUpdateEditCopy)
	ON_UPDATE_COMMAND_UI(ID_FILE_NCINSERT, &CNCListView::OnUpdateFileInsert)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_UP,  ID_VIEW_RT,  &CNCListView::OnUpdateMoveRoundKey)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_RUP, ID_VIEW_RRT, &CNCListView::OnUpdateMoveRoundKey)
	ON_COMMAND(ID_NCVIEW_TRACE_BREAK, &CNCListView::OnTraceBreak)
	ON_COMMAND(ID_NCVIEW_TRACE_BREAKOFF, &CNCListView::OnTraceBreakOFF)
	ON_COMMAND(ID_NCVIEW_JUMP, &CNCListView::OnViewJump)
	ON_COMMAND(ID_EDIT_FIND, &CNCListView::OnViewFind)
	ON_COMMAND(ID_FILE_NCINSERT, &CNCListView::OnFileInsert)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_USERTRACESELECT, &CNCListView::OnSelectTrace)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNCListView クラスのオーバライド関数

BOOL CNCListView::PreCreateWindow(CREATESTRUCT& cs) 
{
	cs.style &= ~LVS_TYPEMASK;
	cs.style |= (LVS_REPORT|LVS_NOSORTHEADER|LVS_SINGLESEL|LVS_SHOWSELALWAYS|
		LVS_SHAREIMAGELISTS|LVS_OWNERDATA);
	return CListView::PreCreateWindow(cs);
}

void CNCListView::OnInitialUpdate() 
{
	CListView::OnInitialUpdate();

	int	nSize = (int)(GetDocument()->GetNCBlockSize());
	// ﾘｽﾄﾋﾞｭｰｺﾝﾄﾛｰﾙの最大数をｾｯﾄ
	GetListCtrl().SetItemCountEx(nSize);
	// 列幅設定
	GetListCtrl().SetColumnWidth(1, LVSCW_AUTOSIZE);
	// 子ﾌﾚｰﾑのｽﾃｰﾀｽﾊﾞｰ初期化
	CNCChild*	pFrame = static_cast<CNCChild *>(GetParentFrame());
	pFrame->SetStatusMaxLine(nSize);
	pFrame->SetStatusInfo(0, (CNCdata *)NULL);
	pFrame->SendMessage(WM_USERSTATUSLINENO, (WPARAM)GetDocument());
}

void CNCListView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	switch ( lHint ) {
	case UAV_DRAWWORKRECT:
		break;		// ｺﾒﾝﾄ更新の可能性があるので再描画
	case UAV_STARTSTOPTRACE:	// 選択解除
		{
			POSITION pos = GetListCtrl().GetFirstSelectedItemPosition();
			if ( pos ) {
				int nIndex = GetListCtrl().GetNextSelectedItem(pos);
				GetListCtrl().SetItemState(nIndex, ~LVIS_SELECTED, LVIS_SELECTED);
				GetListCtrl().EnsureVisible(nIndex, FALSE);
			}
		}
		// through
	case UAV_TRACECURSOR:
	case UAV_DRAWMAXRECT:
		return;		// 再描画不要
	case UAV_FILEINSERT:	// ﾘｽﾄ再読込(ｶｰｿﾙ位置に読み込み)
		{
			int	nSize = (int)(GetDocument()->GetNCBlockSize());
			// ﾘｽﾄﾋﾞｭｰｺﾝﾄﾛｰﾙの最大数をｾｯﾄ
			GetListCtrl().SetItemCountEx(nSize);
			// 列幅設定
			GetListCtrl().SetColumnWidth(1, LVSCW_AUTOSIZE);
			// 子ﾌﾚｰﾑのｽﾃｰﾀｽﾊﾞｰ初期化
			static_cast<CNCChild *>(GetParentFrame())->SetStatusMaxLine(nSize);
		}
		GetListCtrl().Invalidate();
		break;
	case UAV_CHANGEFONT:	// ﾌｫﾝﾄの変更
		GetListCtrl().SetFont(AfxGetNCVCMainWnd()->GetTextFont(TYPE_NCD), FALSE);
		break;
	}

	CListView::OnUpdate(pSender, lHint, pHint);
}

BOOL CNCListView::PreTranslateMessage(MSG* pMsg) 
{
	// ﾘｽﾄﾋﾞｭｰがｱｸﾃｨﾌﾞなときはｷｰﾒｯｾｰｼﾞを直接ﾘｽﾄｺﾝﾄﾛｰﾙに送る
	if ( pMsg->message == WM_KEYDOWN ) {
		// ｷｰﾎﾞｰﾄﾞｱｸｾﾗﾚｰﾀまで捕まえてしまうので移動ｺｰﾄﾞを指定
		switch ( pMsg->wParam ) {
		case VK_PRIOR:
		case VK_NEXT:
		case VK_END:
		case VK_HOME:
		case VK_LEFT:
		case VK_UP:
		case VK_RIGHT:
		case VK_DOWN:
			GetListCtrl().SendMessage(WM_KEYDOWN, pMsg->wParam, pMsg->lParam);
			return TRUE;	// ﾘｽﾄｱｲﾃﾑ移動のための矢印ｷｰ等が適切に処理される
		// 以下、なぜかｱｸｾﾗﾚｰﾀが効かない
		case VK_F9:
			OnTraceBreak();
			return TRUE;
		case VK_INSERT:
			OnFileInsert();
			return TRUE;
		}
	}
	return CListView::PreTranslateMessage(pMsg);
}

BOOL CNCListView::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
	// ここから CDocument::OnCmdMsg() を呼ばないようにする
//	return CListView::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
	return CWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

/////////////////////////////////////////////////////////////////////////////
// CNCListView クラスのメンバ関数

void CNCListView::SetJumpList(int nJump)
{
	int		nIndex, nCount = GetListCtrl().GetItemCount();
	if ( nJump < 1 )
		nIndex = 0;
	else if ( nJump > nCount )
		nIndex = nCount - 1;
	else
		nIndex = nJump;
	GetListCtrl().SetItemState(nIndex, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
	GetListCtrl().EnsureVisible(nIndex, FALSE);
}

void CNCListView::SetFindList(int nUpDown, const CString& strFind)
{
	if ( strFind != m_regFind.str().c_str() ) {
		try {
			m_regFind = strFind;
		}
		catch (boost::regex_error&) {
			AfxMessageBox(IDS_ERR_REGEX, MB_OK|MB_ICONEXCLAMATION);
			return;
		}
	}

	BOOL	bReverse;
	INT_PTR	nIndex = 0;
	// 現在の選択行
	POSITION pos = GetListCtrl().GetFirstSelectedItemPosition();
	if ( pos )
		nIndex = GetListCtrl().GetNextSelectedItem(pos);
	if ( nUpDown == 0 ) {
		// 上方向
		if ( --nIndex < 0 )
			nIndex = GetDocument()->GetNCBlockSize() - 1;
		bReverse = TRUE;
	}
	else {
		// 下方向
		if ( ++nIndex >= GetDocument()->GetNCBlockSize() )
			nIndex = 0;
		bReverse = FALSE;
	}
	// 次の検索
	nIndex = GetDocument()->SearchBlockRegex(m_regFind, nIndex, bReverse);
	if ( nIndex < 0 )
		::MessageBeep(MB_ICONASTERISK);
	else {
		GetListCtrl().SetItemState((int)nIndex, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
		GetListCtrl().EnsureVisible((int)nIndex, FALSE);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CNCListView 診断

#ifdef _DEBUG
void CNCListView::AssertValid() const
{
	CListView::AssertValid();
}

void CNCListView::Dump(CDumpContext& dc) const
{
	CListView::Dump(dc);
}

CNCDoc* CNCListView::GetDocument() // 非デバッグ バージョンはインラインです。
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CNCDoc)));
	return static_cast<CNCDoc *>(m_pDocument);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CNCListView クラスのメッセージ ハンドラ（メニュー編）

void CNCListView::OnUpdateViewJump(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck( AfxGetNCVCMainWnd()->GetModelessDlg(MLD_NCJUMP) != NULL );
}

void CNCListView::OnViewJump() 
{
	if ( AfxGetNCVCMainWnd()->GetModelessDlg(MLD_NCJUMP) ) {
		// CNCJumpDlg::OnCancel() の間接呼び出し
		AfxGetNCVCMainWnd()->GetModelessDlg(MLD_NCJUMP)->PostMessage(WM_CLOSE);
		return;
	}
	CNCJumpDlg*	pDlg = new CNCJumpDlg;
	pDlg->Create(IDD_NCVIEW_JUMP);
	AfxGetNCVCMainWnd()->SetModelessDlg(MLD_NCJUMP, pDlg);
}

void CNCListView::OnUpdateViewFind(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck( AfxGetNCVCMainWnd()->GetModelessDlg(MLD_NCFIND) != NULL );
}

void CNCListView::OnViewFind() 
{
	if ( AfxGetNCVCMainWnd()->GetModelessDlg(MLD_NCFIND) ) {
		// CNCJumpDlg::OnCancel() の間接呼び出し
		AfxGetNCVCMainWnd()->GetModelessDlg(MLD_NCFIND)->PostMessage(WM_CLOSE);
		return;
	}
	CNCFindDlg*	pDlg = new CNCFindDlg;
	pDlg->Create(IDD_NCVIEW_FIND);
	AfxGetNCVCMainWnd()->SetModelessDlg(MLD_NCFIND, pDlg);
}

void CNCListView::OnUpdateTraceBreak(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(GetListCtrl().GetFirstSelectedItemPosition() ? TRUE : FALSE);
}

void CNCListView::OnTraceBreak() 
{
	POSITION pos;
	if ( !(pos=GetListCtrl().GetFirstSelectedItemPosition()) )
		return;
	int nIndex = GetListCtrl().GetNextSelectedItem(pos);
	GetDocument()->CheckBreakPoint(nIndex);
	GetListCtrl().Update(nIndex);
}

void CNCListView::OnTraceBreakOFF() 
{
	GetDocument()->ClearBreakPoint();
	GetListCtrl().Invalidate();
}

void CNCListView::OnUpdateEditCopy(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(FALSE);
}

void CNCListView::OnUpdateMoveRoundKey(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(FALSE);
}

void CNCListView::OnUpdateFileInsert(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(GetListCtrl().GetFirstSelectedItemPosition() ? TRUE : FALSE);
}

void CNCListView::OnFileInsert()
{
	POSITION pos;
	if ( !(pos=GetListCtrl().GetFirstSelectedItemPosition()) )
		return;
	int	nInsert = GetListCtrl().GetNextSelectedItem(pos);

	CString	strFileName(AfxGetNCVCApp()->GetRecentFileName());
	if ( ::NCVC_FileDlgCommon(ID_FILE_NCINSERT,
				AfxGetNCVCApp()->GetFilterString(TYPE_NCD), TRUE, strFileName) != IDOK )
		return;

	GetDocument()->InsertBlock(nInsert, strFileName);
}

/////////////////////////////////////////////////////////////////////////////
// CNCListView メッセージ ハンドラ

int CNCListView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if ( CListView::OnCreate(lpCreateStruct) < 0 )
		return -1;

	// ｲﾒｰｼﾞﾘｽﾄ
	GetListCtrl().SetImageList(AfxGetNCVCMainWnd()->GetListImage(), LVSIL_SMALL);
	// ﾘｽﾄﾋﾞｭｰｺﾝﾄﾛｰﾙﾌｫﾝﾄ設定
	GetListCtrl().SetFont(AfxGetNCVCMainWnd()->GetTextFont(TYPE_NCD), FALSE);
	// 列追加
	GetListCtrl().InsertColumn(0, "Line", LVCFMT_LEFT);
	GetListCtrl().InsertColumn(1, "Code", LVCFMT_LEFT);
	GetListCtrl().SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
	GetListCtrl().SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);
	// 全ての列を選択可能にする
	DWORD	dwStyle = GetListCtrl().GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;
	GetListCtrl().SetExtendedStyle(dwStyle);

	return 0;
}

void CNCListView::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	CMenu	menu;
	menu.LoadMenu(IDR_NCPOPUP2);
	CMenu*	pMenu = menu.GetSubMenu(0);
	pMenu->TrackPopupMenu(TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_RIGHTBUTTON,
		point.x, point.y, AfxGetMainWnd());
}

void CNCListView::OnGetDispInfo(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NMLVDISPINFO* plvdi = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	CNCblock*	pBlock;

	if ( plvdi->item.mask & LVIF_TEXT ) {
		pBlock = GetDocument()->GetNCblock(plvdi->item.iItem);
		switch ( plvdi->item.iSubItem ) {
		case 0:		// Line No.
			lstrcpy(plvdi->item.pszText, pBlock->GetStrLine());
			break;
		case 1:		// G Code
			lstrcpy(plvdi->item.pszText, pBlock->GetStrGcode());
			break;
		}
	}
	if ( plvdi->item.mask & LVIF_IMAGE ) {
		// ﾌﾞﾚｲｸ表示優先
		if ( GetDocument()->IsBreakPoint(plvdi->item.iItem) ) {
			plvdi->item.iImage = LISTIMG_BREAK;
		}
		else {
			pBlock = GetDocument()->GetNCblock(plvdi->item.iItem);
			if ( pBlock->GetNCBlkErrorCode() > 0 )
				plvdi->item.iImage = LISTIMG_ERROR;
			else if ( pBlock->GetBlockFlag() & NCF_FOLDER )
				plvdi->item.iImage = LISTIMG_FOLDER;
			else
				plvdi->item.iImage = LISTIMG_NORMAL;
		}
	}

	*pResult = 0;
}

void CNCListView::OnItemChanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LPNMLISTVIEW pNMListView = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	// m_bTraceSelect が真の時は、SelectTrace() から WM_USERSTATUSLINENO 呼び出し
	if ( !m_bTraceSelect && (pNMListView->uNewState & LVIS_SELECTED) ) {
		int	nItem = pNMListView->iItem;
		// ｽﾃｰﾀｽﾊﾞｰの更新
		CNCChild*	pFrame = static_cast<CNCChild *>(GetParentFrame());
		pFrame->SetStatusInfo(nItem+1,
			nItem<0 || nItem>=GetDocument()->GetNCBlockSize() ?
			(CNCblock *)NULL : GetDocument()->GetNCblock(nItem) );
		pFrame->SendMessage(WM_USERSTATUSLINENO, (WPARAM)GetDocument());
	}
	*pResult = 0;
}

LRESULT CNCListView::OnSelectTrace(WPARAM wParam, LPARAM)
{
	CNCdata*	pData = reinterpret_cast<CNCdata *>(wParam);
	ASSERT( pData );
	// 別ｽﾚｯﾄﾞ(CTraceThread)からの呼び出しのため、ﾒｯｾｰｼﾞ送信にて不具合対応
	int	nIndex = pData->GetBlockLineNo();
	m_bTraceSelect = TRUE;	// OnItemChanged を発動しない
	GetListCtrl().SetItemState(nIndex, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
	m_bTraceSelect = FALSE;
	// ｽﾃｰﾀｽﾊﾞｰ更新
	CNCChild*	pFrame = static_cast<CNCChild *>(GetParentFrame());
	pFrame->SetStatusInfo(nIndex+1, pData);
	pFrame->SendMessage(WM_USERSTATUSLINENO, (WPARAM)GetDocument());
	// 強制ｽｸﾛｰﾙの可能性もあるのでUpdate()ではNG
	GetListCtrl().EnsureVisible(nIndex, FALSE);

	return 0;
}

void CNCListView::OnKeyDown(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LPNMLVKEYDOWN pTVKeyDown = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);

	if ( pTVKeyDown->wVKey == VK_TAB ) {
		CNCChild*	pFrame = static_cast<CNCChild *>(GetParentFrame());
		if ( ::GetKeyState(VK_SHIFT) < 0 )
			pFrame->GetInfoView()->SetFocus();
		else
			pFrame->GetMainView()->SetFocus();
	}

	*pResult = 0;
}
