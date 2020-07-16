// DXFShapeView.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "DXFdata.h"
#include "DXFshape.h"
#include "Layer.h"
#include "DXFDoc.h"
#include "DXFView.h"
#include "DXFShapeView.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
extern	CMagaDbg	g_dbg;
#endif

using namespace boost;

// ｲﾒｰｼﾞ表示ｲﾝﾃﾞｯｸｽ
#define	TREEIMG_ROOT		0
#define	TREEIMG_LAYER		1
#define	TREEIMG_CHAIN		2
#define	TREEIMG_MAP			3
#define	TREEIMG_WORK		4

/////////////////////////////////////////////////////////////////////////////
// CDXFShapeView

IMPLEMENT_DYNCREATE(CDXFShapeView, CTreeView)

BEGIN_MESSAGE_MAP(CDXFShapeView, CTreeView)
	//{{AFX_MSG_MAP(CDXFShapeView)
	ON_WM_CREATE()
	ON_WM_CONTEXTMENU()
	ON_NOTIFY_REFLECT(TVN_GETDISPINFO, OnGetDispInfo)
	ON_NOTIFY_REFLECT(TVN_BEGINLABELEDIT, OnBeginLabelEdit)
	ON_NOTIFY_REFLECT(TVN_ENDLABELEDIT, OnEndLabelEdit)
	ON_NOTIFY_REFLECT(TVN_KEYDOWN, OnKeydown)
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelChanged)
	ON_NOTIFY_REFLECT(TVN_BEGINDRAG, OnBeginDrag)
	ON_NOTIFY_REFLECT(NM_RCLICK, OnNMRclick)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_COMMAND(ID_EDIT_SORTSHAPE, OnSortShape)
	ON_UPDATE_COMMAND_UI(ID_EDIT_SHAPE_DEL, OnUpdateWorkingDel)
	ON_COMMAND(ID_EDIT_SHAPE_DEL, OnWorkingDel)
	//}}AFX_MSG_MAP
	// 形状加工指示
	ON_UPDATE_COMMAND_UI_RANGE(ID_EDIT_SHAPE_SEL, ID_EDIT_SHAPE_POC, OnUpdateShapePattern)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDXFShapeView クラスの構築/消滅

CDXFShapeView::CDXFShapeView()
{
	m_bDragging = FALSE;
	m_pDragLayer = NULL;
	m_pDragShape = NULL;
	m_dwDragRoot = 0;
	m_bUpdateLayerSequence = FALSE;
	m_pImageList = NULL;
}

CDXFShapeView::~CDXFShapeView()
{
}

/////////////////////////////////////////////////////////////////////////////
// CDXFShapeView クラスのオーバライド関数

BOOL CDXFShapeView::PreCreateWindow(CREATESTRUCT& cs) 
{
	cs.style |= (TVS_HASBUTTONS|TVS_HASLINES|TVS_LINESATROOT|
		TVS_EDITLABELS|TVS_FULLROWSELECT|TVS_SHOWSELALWAYS);
	cs.dwExStyle |= WS_EX_OVERLAPPEDWINDOW;
	return CTreeView::PreCreateWindow(cs);
}

void CDXFShapeView::OnInitialUpdate() 
{
	static	LPTSTR	ss_RootTitle[] = {
		"輪郭集合", "軌跡集合", "除外集合"
	};
	CTreeView::OnInitialUpdate();

	int		i;
	TVINSERTSTRUCT	tvInsert;
	::ZeroMemory(&tvInsert, sizeof(TVINSERTSTRUCT));
	tvInsert.hInsertAfter = TVI_LAST;
	tvInsert.item.mask = TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
	tvInsert.item.iImage = tvInsert.item.iSelectedImage = TREEIMG_ROOT;

	// ﾙｰﾄﾂﾘｰ
	ASSERT( SIZEOF(m_hRootTree) == SIZEOF(ss_RootTitle) );
	tvInsert.hParent = TVI_ROOT;
	for ( i=0; i<SIZEOF(ss_RootTitle); i++ ) {
		tvInsert.item.lParam = (LPARAM)(i+1);		// RootID
		tvInsert.item.pszText = ss_RootTitle[i];
		m_hRootTree[i] = GetTreeCtrl().InsertItem(&tvInsert);
		ASSERT( m_hRootTree[i] );
	}

	// ｼﾘｱﾙ化後の処理
	if ( GetDocument()->IsShape() ) {
		// 形状情報を登録
		SetShapeTree();
		// 加工指示を登録
		AutoWorkingSet(FALSE);
		// 全てのﾂﾘｰｱｲﾃﾑを展開
		HTREEITEM	hTree, hLayerTree;
		for ( i=0; i<SIZEOF(m_hRootTree); i++ ) {
			hLayerTree = GetTreeCtrl().GetChildItem(m_hRootTree[i]);
			while ( hLayerTree ) {
				GetTreeCtrl().Expand(hLayerTree, TVE_EXPAND);
				hTree = GetTreeCtrl().GetChildItem(hLayerTree);
				while ( hTree ) {
					GetTreeCtrl().Expand(hTree, TVE_EXPAND);
					hTree = GetTreeCtrl().GetNextSiblingItem(hTree);
				}
				hLayerTree = GetTreeCtrl().GetNextSiblingItem(hLayerTree);
			}
		}
	}
}

void CDXFShapeView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	switch ( lHint ) {
	case UAV_FILESAVE:			// from CDXFDoc::OnSaveDocument
		UpdateSequence();
		// through
	case UAV_DXFORGUPDATE:
	case UAV_DXFSHAPEID:
		return;		// 再描画不要
	case UAV_DXFSHAPEUPDATE:
		if ( pHint ) {			// from CDXFView::OnLButtonUp
			GetTreeCtrl().SelectItem( ((CDXFshape *)pHint)->GetTreeHandle() );
		}
		else {					// from CDXFDoc::OnEditShape
			SetShapeTree();
			SetFocus();
		}
		break;
	case UAV_DXFAUTOWORKING:	// from CDXFDoc::OnEditAutoShape
		AutoWorkingSet();
		SetFocus();
		break;
	case UAV_DXFAUTODELWORKING:	// from CDXFDoc::OnEditAutoShape
		AutoWorkingDel();
		break;
	case UAV_DXFADDWORKING:		// from CDXFView::OnLButtonUp
		ASSERT( pHint );	// CDXFworking
		AddWorking( (CDXFworking *)pHint );
		break;
	case UAV_DXFADDSHAPE:		// from CDXFView::OnLButtonUp_Sel
		ASSERT( pHint );	// LPDXFADDSHAPE
		OnUpdateShape( (LPDXFADDSHAPE)pHint );
		break;
	}
	CView::OnUpdate(pSender, lHint, pHint);
}

BOOL CDXFShapeView::PreTranslateMessage(MSG* pMsg) 
{
	// ﾂﾘｰﾋﾞｭｰがｱｸﾃｨﾌﾞなときはｷｰﾒｯｾｰｼﾞを直接ﾂﾘｰｺﾝﾄﾛｰﾙに送る
	if ( pMsg->message == WM_KEYDOWN ) {
		// ｷｰﾎﾞｰﾄﾞｱｸｾﾗﾚｰﾀまで捕まえてしまうので移動ｺｰﾄﾞを指定
		if ( GetTreeCtrl().GetEditControl() ) {
			// ｴﾃﾞｨｯﾄ中ならここでﾒｯｾｰｼﾞをﾃﾞｨｽﾊﾟｯﾁ
			::TranslateMessage(pMsg);
			::DispatchMessage(pMsg);
			return TRUE;		// ｴﾃﾞｨｯﾄｺﾝﾄﾛｰﾙが適切に処理
		}
		else {
			switch ( pMsg->wParam ) {
			case VK_PRIOR:
			case VK_NEXT:
			case VK_END:
			case VK_HOME:
			case VK_LEFT:
			case VK_UP:
			case VK_RIGHT:
			case VK_DOWN:
			case VK_F2:		// 項目名編集用
				GetTreeCtrl().SendMessage(WM_KEYDOWN, pMsg->wParam, pMsg->lParam);
				return TRUE;	// 矢印ｷｰ等が適切に処理される
			}
		}
	}
	return CTreeView::PreTranslateMessage(pMsg);
}

/////////////////////////////////////////////////////////////////////////////
// CDXFShapeView クラスのメンバ関数

void CDXFShapeView::OnUpdateShape(LPDXFADDSHAPE lpShape)
{
	TVINSERTSTRUCT	tvInsert;
	::ZeroMemory(&tvInsert, sizeof(TVINSERTSTRUCT));
	tvInsert.hInsertAfter = TVI_LAST;
	tvInsert.item.mask = TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
	tvInsert.item.pszText = LPSTR_TEXTCALLBACK;

	ASSERT( lpShape->pLayer );
	ASSERT( lpShape->pShape );

	// 形状情報の削除
	BOOL	bExpand = FALSE;
	HTREEITEM hTree = lpShape->pShape->GetTreeHandle();
	if ( hTree ) {	// 新規の場合はNULL
		// 親ｱｲﾃﾑ(ﾚｲﾔ)取得
		HTREEITEM hTreeParent = GetTreeCtrl().GetParentItem(hTree);
		// 現在のｽﾃｰﾀｽを取得して消去
		TVITEM	tvItem;
		::ZeroMemory(&tvItem, sizeof(TVITEM));
		tvItem.mask = TVIF_STATE;
		tvItem.hItem = hTree;
		GetTreeCtrl().GetItem(&tvItem);
		if ( tvItem.state & TVIS_EXPANDED )
			bExpand = TRUE;
		GetTreeCtrl().DeleteItem(hTree);
		// 削除後，ﾚｲﾔ配下に他の形状情報が無ければ
		if ( hTreeParent && !GetTreeCtrl().ItemHasChildren(hTreeParent) )
			GetTreeCtrl().DeleteItem(hTreeParent);	// ﾚｲﾔﾂﾘｰも削除
	}

	// 所属集合の決定
	int nID = (int)(lpShape->pShape->GetShapeAssemble());
	if ( nID == DXFSHAPE_OUTLINE ) {
		nID = 1;	// 軌跡集合(自動で輪郭集合に移動させることはない)
		lpShape->pShape->SetShapeAssemble(DXFSHAPE_LOCUS);
	}

	// 形状情報の登録
	HTREEITEM hLayerTree = SearchLayerTree(GetTreeCtrl().GetChildItem(m_hRootTree[nID]), lpShape->pLayer);
	if ( !hLayerTree ) {
		tvInsert.hParent = m_hRootTree[nID];
		tvInsert.item.iImage = tvInsert.item.iSelectedImage = TREEIMG_LAYER;
		tvInsert.item.lParam = (LPARAM)(lpShape->pLayer);
		hLayerTree = GetTreeCtrl().InsertItem(&tvInsert);
		ASSERT( hLayerTree );
	}
	tvInsert.hParent = hLayerTree;
	tvInsert.item.iImage = tvInsert.item.iSelectedImage = TREEIMG_CHAIN + lpShape->pShape->GetShapeType();
	tvInsert.item.lParam = (LPARAM)(lpShape->pShape);
	hTree = GetTreeCtrl().InsertItem(&tvInsert);
	lpShape->pShape->SetTreeHandle(hTree);

	// 加工指示の登録
	CDXFworkingList*	pList = lpShape->pShape->GetWorkList();
	tvInsert.hParent = hTree;
	tvInsert.item.iImage = tvInsert.item.iSelectedImage = TREEIMG_WORK;
	for ( POSITION pos=pList->GetHeadPosition(); pos; ) {
		tvInsert.item.lParam = (LPARAM)(pList->GetNext(pos));
		GetTreeCtrl().InsertItem(&tvInsert);
	}
	if ( bExpand )
		GetTreeCtrl().Expand(hTree, TVE_EXPAND);

	// 更新情報
	m_mpUpdateLayer.SetAt(lpShape->pLayer->GetStrLayer(), lpShape->pLayer);
}

BOOL CDXFShapeView::IsDropItem(HTREEITEM hTree)
{
	if ( !hTree )
		return FALSE;

	if ( IsRootTree(hTree) ) {
		// 所属集合ﾂﾘｰにはﾄﾞﾛｯﾌﾟできない
		if ( m_dwDragRoot == GetParentAssemble(hTree) )
			return FALSE;
	}
	else {
		// 加工指示にはﾄﾞﾛｯﾌﾟできない
		CObject*	pObject = (CObject *)(GetTreeCtrl().GetItemData(hTree));
		if ( !pObject || pObject->IsKindOf(RUNTIME_CLASS(CDXFworking)) )
			return FALSE;
		if ( m_pDragShape ) {
			// 形状情報の場合
			if ( pObject->IsKindOf(RUNTIME_CLASS(CLayerData)) ) {
				// ﾄﾞﾛｯﾌﾟ先がﾚｲﾔ情報でﾄﾞﾗｯｸﾞ形状が所属するﾚｲﾔと同じ場合だけ
				if ( m_pDragLayer != (CLayerData *)pObject )
					return FALSE;
			}
			else {
				// ﾄﾞﾛｯﾌﾟ先が形状情報ならその親(ﾚｲﾔ)を取得して
				CLayerData* pLayer = (CLayerData *)(GetTreeCtrl().GetItemData(GetTreeCtrl().GetParentItem(hTree)));
				// ﾄﾞﾗｯｸﾞ形状が所属するﾚｲﾔと同じ場合だけ
				if ( m_pDragLayer != pLayer )
					return FALSE;
			}
		}
		else {
			// ﾚｲﾔの場合
			if ( !pObject->IsKindOf(RUNTIME_CLASS(CLayerData)) )
				return FALSE;
		}
	}

	return TRUE;
}

void CDXFShapeView::DragInsert(void)
{
	// m_hItemDropにm_hItemDragを挿入
	HTREEITEM hTree = m_pDragShape ? DragInsertShape() : DragInsertLayer();
	if ( hTree ) {
		// ﾄﾞﾗｯｸﾞ前の状態を取得
		TVITEM	tvItem;
		::ZeroMemory(&tvItem, sizeof(TVITEM));
		tvItem.mask  = TVIF_STATE;
		tvItem.hItem = m_hItemDrag;
		GetTreeCtrl().GetItem(&tvItem);
		// 移動先で選択，状態復元
		GetTreeCtrl().SelectItem(hTree);
		if ( tvItem.state & TVIS_EXPANDED )
			GetTreeCtrl().Expand(hTree, TVE_EXPAND);
		// ﾄﾞﾗｯｸﾞﾂﾘｰを削除(SelectItem()後でないと描画不正)
		hTree = GetTreeCtrl().GetParentItem(m_hItemDrag);
		GetTreeCtrl().DeleteItem(m_hItemDrag);
		// 変更記録を保存
		if ( m_pDragShape ) {
			m_mpUpdateLayer.SetAt(m_pDragLayer->GetStrLayer(), m_pDragLayer);
			// 形状情報のﾄﾞﾗｯｸﾞ移動で所属ﾚｲﾔ配下に形状情報が無くなれば
			if ( !GetTreeCtrl().ItemHasChildren(hTree) )
				GetTreeCtrl().DeleteItem(hTree);	// ﾚｲﾔﾂﾘｰも削除
		}
		else
			m_bUpdateLayerSequence = TRUE;
		// ﾄﾞｷｭﾒﾝﾄ変更通知
		GetDocument()->SetModifiedFlag();
		DragCancel(FALSE);
	}
	else {
		DragCancel(TRUE);
		AfxMessageBox(IDS_ERR_DXF_DRAGOUTLINE, MB_OK|MB_ICONEXCLAMATION);
	}
}

HTREEITEM CDXFShapeView::DragInsertLayer(void)
{
	DWORD		dwRoot = GetParentAssemble(m_hItemDrop);
	BOOL		bCanNotOutline = FALSE;
	POSITION	pos;
	HTREEITEM	hLayerTree = NULL, hShapeTree;
	CDXFshape*	pShape;
	CDXFworkingList*	pList;

	// 最終ﾄﾞﾛｯﾌﾟ許可ﾁｪｯｸ
	hShapeTree = GetTreeCtrl().GetChildItem(m_hItemDrag);
	while ( hShapeTree ) {
		pShape = (CDXFshape *)(GetTreeCtrl().GetItemData(hShapeTree));
		if ( pShape->GetShapeFlag() & DXFMAPFLG_CANNOTAUTOWORKING ) {
			bCanNotOutline = TRUE;
			break;
		}
		hShapeTree = GetTreeCtrl().GetNextSiblingItem(hShapeTree);
	}
	if ( bCanNotOutline && dwRoot == ROOTTREE_SHAPE )
		return NULL;

	TVINSERTSTRUCT	tvInsert;
	::ZeroMemory(&tvInsert, sizeof(TVINSERTSTRUCT));
	tvInsert.item.mask = TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
	tvInsert.item.pszText = LPSTR_TEXTCALLBACK;
	TVITEM	tvItem;
	::ZeroMemory(&tvItem, sizeof(TVITEM));
	tvItem.mask = TVIF_STATE;

	// ﾚｲﾔ情報の登録
	if ( IsRootTree(m_hItemDrop) ) {
		tvInsert.hParent = m_hItemDrop;
		tvInsert.hInsertAfter = m_dwDragRoot == dwRoot ? TVI_FIRST : TVI_LAST;
	}
	else {
		tvInsert.hParent = GetTreeCtrl().GetParentItem(m_hItemDrop);
		hLayerTree = GetTreeCtrl().GetPrevSiblingItem(m_hItemDrop);
		tvInsert.hInsertAfter = hLayerTree ? hLayerTree : TVI_FIRST;
	}
	if ( m_dwDragRoot != dwRoot )
		hLayerTree = SearchLayerTree(GetTreeCtrl().GetChildItem(tvInsert.hParent), m_pDragLayer);
	if ( !hLayerTree ) {
		tvInsert.item.iImage = tvInsert.item.iSelectedImage = TREEIMG_LAYER;
		tvInsert.item.lParam = (LPARAM)m_pDragLayer;
		hLayerTree = GetTreeCtrl().InsertItem(&tvInsert);
	}
	ASSERT( hLayerTree );

	// 形状情報と加工指示の登録
	tvInsert.hInsertAfter = TVI_LAST;
	hShapeTree = GetTreeCtrl().GetChildItem(m_hItemDrag);
	while ( hShapeTree ) {
		// 形状情報
		pShape = (CDXFshape *)(GetTreeCtrl().GetItemData(hShapeTree));
		tvInsert.item.iImage = tvInsert.item.iSelectedImage = 
			TREEIMG_CHAIN + pShape->GetShapeType();
		tvInsert.hParent = hLayerTree;
		tvInsert.item.lParam = (LPARAM)pShape;
		tvInsert.hParent = GetTreeCtrl().InsertItem(&tvInsert);	// 加工指示の親ﾂﾘｰ
		ASSERT( tvInsert.hParent );
		pShape->SetTreeHandle(tvInsert.hParent);
		// 所属集合の更新
		if ( m_dwDragRoot != dwRoot )
			pShape->SetShapeAssemble((DXFSHAPE_ASSEMBLE)(dwRoot-1));
		// 加工指示
		pList = pShape->GetWorkList();
		tvInsert.item.iImage = tvInsert.item.iSelectedImage = TREEIMG_WORK;
		for ( pos=pList->GetHeadPosition(); pos; ) {
			tvInsert.item.lParam = (LPARAM)(pList->GetNext(pos));
			GetTreeCtrl().InsertItem(&tvInsert);
		}
		// 移動前の状態を復元
		tvItem.hItem = hShapeTree;
		GetTreeCtrl().GetItem(&tvItem);
		if ( tvItem.state & TVIS_EXPANDED )
			GetTreeCtrl().Expand(tvInsert.hParent, TVE_EXPAND);
		// 次の形状情報
		hShapeTree = GetTreeCtrl().GetNextSiblingItem(hShapeTree);
	}

	return hLayerTree;
}

HTREEITEM CDXFShapeView::DragInsertShape(void)
{
	DWORD		dwRoot = GetParentAssemble(m_hItemDrop);
	HTREEITEM	hTree;
	CDXFworkingList*	pList;

	// 最終ﾄﾞﾛｯﾌﾟ許可ﾁｪｯｸ
	if ( m_pDragShape->GetShapeFlag() & DXFMAPFLG_CANNOTAUTOWORKING && dwRoot == ROOTTREE_SHAPE )
		return NULL;

	TVINSERTSTRUCT	tvInsert;
	::ZeroMemory(&tvInsert, sizeof(TVINSERTSTRUCT));
	tvInsert.item.mask = TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
	tvInsert.item.pszText = LPSTR_TEXTCALLBACK;

	// 形状情報の登録
	if ( IsRootTree(m_hItemDrop) ) {
		// ﾄﾞﾛｯﾌﾟ先がﾙｰﾄﾂﾘｰでﾚｲﾔ情報がなければ追加
		hTree = SearchLayerTree(GetTreeCtrl().GetChildItem(m_hItemDrop), m_pDragLayer);
		if ( !hTree ) {
			tvInsert.hParent = m_hItemDrop;
			tvInsert.hInsertAfter = TVI_LAST;
			tvInsert.item.iImage = tvInsert.item.iSelectedImage = TREEIMG_LAYER;
			tvInsert.item.lParam = (LPARAM)m_pDragLayer;
			hTree = GetTreeCtrl().InsertItem(&tvInsert);
			ASSERT( hTree );
			m_hItemDrop = NULL;
		}
		else {
			m_hItemDrop = GetTreeCtrl().GetChildItem(m_hItemDrop);
		}
	}
	else {
		CObject* pObject = (CObject *)(GetTreeCtrl().GetItemData(m_hItemDrop));
		if ( pObject->IsKindOf(RUNTIME_CLASS(CLayerData)) ) {
			hTree = m_hItemDrop;
			m_hItemDrop = GetTreeCtrl().GetChildItem(m_hItemDrop);
		}
		else {
			hTree = GetTreeCtrl().GetParentItem(m_hItemDrop);
		}
	}

	// hTreeにはﾚｲﾔ，m_hItemDropには形状のﾂﾘｰﾊﾝﾄﾞﾙ(またはNULL)が入っている
	tvInsert.hParent = hTree;
	hTree = GetTreeCtrl().GetPrevSiblingItem(m_hItemDrop);
	tvInsert.hInsertAfter = hTree ? hTree : TVI_FIRST;	// ﾄﾞﾛｯﾌﾟ先
	tvInsert.item.iImage = tvInsert.item.iSelectedImage = 
		TREEIMG_CHAIN + m_pDragShape->GetShapeType();
	tvInsert.item.lParam = (LPARAM)m_pDragShape;
	hTree = GetTreeCtrl().InsertItem(&tvInsert);
	ASSERT( hTree );
	m_pDragShape->SetTreeHandle(hTree);
	// 所属集合の更新
	if ( m_dwDragRoot != dwRoot )
		m_pDragShape->SetShapeAssemble((DXFSHAPE_ASSEMBLE)(dwRoot-1));

	// 加工指示の登録
	tvInsert.hInsertAfter = TVI_LAST;
	tvInsert.hParent = hTree;
	tvInsert.item.iImage = tvInsert.item.iSelectedImage = TREEIMG_WORK;
	pList = m_pDragShape->GetWorkList();
	for ( POSITION pos=pList->GetHeadPosition(); pos; ) {
		tvInsert.item.lParam = (LPARAM)(pList->GetNext(pos));
		GetTreeCtrl().InsertItem(&tvInsert);
	}

	return hTree;
}

void CDXFShapeView::DragLink(void)
{
	// 最終結合許可ﾁｪｯｸ
	if ( IsRootTree(m_hItemDrop) ) {
		DragCancel(TRUE);
		::MessageBeep(MB_ICONEXCLAMATION);
		return;
	}
	CDXFshape*	pShape = (CDXFshape *)(GetTreeCtrl().GetItemData(m_hItemDrop));
	CLayerData*	pLayer = (CLayerData *)(GetTreeCtrl().GetItemData(GetTreeCtrl().GetParentItem(m_hItemDrop)));
	if ( !pShape || !pShape->IsKindOf(RUNTIME_CLASS(CDXFshape)) || m_pDragLayer!=pLayer ) {
		DragCancel(TRUE);
		::MessageBeep(MB_ICONEXCLAMATION);
		return;
	}

	// 結合処理(ﾄﾞﾛｯﾌﾟｱｲﾃﾑにﾄﾞﾗｯｸﾞｱｲﾃﾑを結合)
	if ( !pShape->LinkShape(m_pDragShape) ) {
		DragCancel(TRUE);
		AfxMessageBox(IDS_ERR_DXF_DRAGLINK, MB_OK|MB_ICONEXCLAMATION);
		return;
	}
	
	// ﾄﾞﾗｯｸﾞ元ﾂﾘｰの削除
	GetTreeCtrl().DeleteItem(m_hItemDrag);
	// ﾄﾞﾗｯｸﾞ元ｱｲﾃﾑ削除
	m_pDragLayer->RemoveShape(m_pDragShape);
	// ﾄﾞﾗｯｸﾞ先の加工指示ﾂﾘｰを削除
	HTREEITEM hTree = GetTreeCtrl().GetChildItem(m_hItemDrop), hTreeTmp;
	while ( hTree ) {
		hTreeTmp = hTree;
		hTree = GetTreeCtrl().GetNextSiblingItem(hTree);
		GetTreeCtrl().DeleteItem(hTreeTmp);
	}
	// ﾄﾞﾗｯｸﾞ先の所属集合検査(降格)
	if ( GetParentAssemble(m_hItemDrop)==ROOTTREE_SHAPE && !pShape->GetShapeChain() ) {
		// 所属集合の変更
		DXFADDSHAPE	addShape;	// DocBase.h
		addShape.pLayer = pLayer;
		addShape.pShape = pShape;
		OnUpdateShape(&addShape);	// 加工指示も登録
		// 結合先ｱｲﾃﾑの選択
		GetTreeCtrl().SelectItem( pShape->GetTreeHandle() );
		// 再描画(しないと表示が乱れる)
		Invalidate();
	}
	else {
		// 加工指示の再登録
		CDXFworkingList* pList = pShape->GetWorkList();
		for ( POSITION pos=pList->GetHeadPosition(); pos; ) {
			hTree = GetTreeCtrl().InsertItem(TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM,
				LPSTR_TEXTCALLBACK, TREEIMG_WORK, TREEIMG_WORK, 0, 0,
				(LPARAM)(pList->GetNext(pos)), m_hItemDrop, TVI_LAST);
			ASSERT( hTree );
		}
		// 結合先ｱｲﾃﾑの選択
		GetTreeCtrl().SelectItem(m_hItemDrop);
	}
	// ｵﾌﾞｼﾞｪｸﾄ選択
	pShape->SetShapeSwitch(TRUE);
	// ﾄﾞｷｭﾒﾝﾄ変更通知
	GetDocument()->SetModifiedFlag();
	DragCancel(FALSE);
}

void CDXFShapeView::DragCancel(BOOL bCancel)
{
	ASSERT( m_pImageList );
	m_pImageList->DragLeave(this);
	m_pImageList->EndDrag();
	delete m_pImageList;
	m_pImageList = NULL;
	m_bDragging = FALSE;
	m_pDragLayer = NULL;
	m_pDragShape = NULL;
	m_dwDragRoot = 0;
	::ShowCursor(TRUE);
	ReleaseCapture();
	GetTreeCtrl().SelectDropTarget(NULL);
	if ( bCancel )
		GetTreeCtrl().SelectItem(m_hItemDrag);
}

HTREEITEM CDXFShapeView::SearchLayerTree(HTREEITEM hTree, const CLayerData* pLayer)
{
	HTREEITEM	hLayerTree = NULL;

	while ( hTree ) {
		if ( pLayer == (CLayerData *)(GetTreeCtrl().GetItemData(hTree)) ) {
			hLayerTree = hTree;
			break;
		}
		hTree = GetTreeCtrl().GetNextSiblingItem(hTree);
	}

	return hLayerTree;
}

DWORD CDXFShapeView::GetParentAssemble(HTREEITEM hTree)
{
	DWORD	dwResult = 0;

	if ( hTree && !IsRootTree(hTree) ) {
		HTREEITEM	hTreeParent;
		hTreeParent = GetTreeCtrl().GetParentItem(hTree);
		while ( hTreeParent ) {
			hTree = hTreeParent;
			hTreeParent = GetTreeCtrl().GetParentItem(hTree);
		}
	}
	if ( hTree )
		dwResult = GetTreeCtrl().GetItemData(hTree);

	return dwResult;
}

void CDXFShapeView::SetShapeSwitch_SubordinateTree(HTREEITEM hParentTree, BOOL bSelect)
{
	HTREEITEM	hTree = GetTreeCtrl().GetChildItem(hParentTree);
	if ( !hTree )
		return;
	CObject*	pObject = (CObject *)GetTreeCtrl().GetItemData(hTree);
	if ( !pObject )
		return;
	if ( pObject->IsKindOf(RUNTIME_CLASS(CLayerData)) ) {
		hTree = GetTreeCtrl().GetChildItem(hTree);
		if ( !hTree )
			return;
	}
	CDXFshape*	pShape;
	while ( hTree ) {
		pShape = (CDXFshape *)GetTreeCtrl().GetItemData(hTree);
		ASSERT( pShape->IsKindOf(RUNTIME_CLASS(CDXFshape)) );
		pShape->SetShapeSwitch(bSelect);
		hTree = GetTreeCtrl().GetNextSiblingItem(hTree);
	}
}

void CDXFShapeView::SetShapeTree(void)
{
	int			i, j, k, nLoop, nLayerLoop = GetDocument()->GetLayerCnt();
	CLayerData*	pLayer;
	CDXFshape*	pShape;
	HTREEITEM	hLayerTree;

	TVINSERTSTRUCT	tvInsert;
	::ZeroMemory(&tvInsert, sizeof(TVINSERTSTRUCT));
	tvInsert.hInsertAfter = TVI_LAST;
	tvInsert.item.mask = TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
	tvInsert.item.pszText = LPSTR_TEXTCALLBACK;

	// 形状情報の登録
	for ( i=0; i<nLayerLoop; i++ ) {
		pLayer = GetDocument()->GetLayerData(i);
		if ( !pLayer->IsCutType() )
			continue;
		nLoop = pLayer->GetShapeSize();
		for ( j=0; j<nLoop; j++ ) {
			pShape = pLayer->GetShapeData(j);
			k = (int)pShape->GetShapeAssemble();	// 輪郭[0],軌跡[1],除外[2]
			ASSERT( 0<=k && k<SIZEOF(m_hRootTree) );
			// ﾚｲﾔ検索
			hLayerTree = SearchLayerTree(GetTreeCtrl().GetChildItem(m_hRootTree[k]), pLayer);
			if ( !hLayerTree ) {
				tvInsert.hParent = m_hRootTree[k];
				tvInsert.item.iImage = tvInsert.item.iSelectedImage = TREEIMG_LAYER;
				tvInsert.item.lParam = (LPARAM)pLayer;
				hLayerTree = GetTreeCtrl().InsertItem(&tvInsert);
				ASSERT( hLayerTree );
			}
			tvInsert.hParent = hLayerTree;
			tvInsert.item.iImage = tvInsert.item.iSelectedImage = TREEIMG_CHAIN + pShape->GetShapeType();
			tvInsert.item.lParam = (LPARAM)pShape;
			pShape->SetTreeHandle( GetTreeCtrl().InsertItem(&tvInsert) );
		}
	}

	for ( i=0; i<SIZEOF(m_hRootTree); i++ )
		GetTreeCtrl().Expand(m_hRootTree[i], TVE_EXPAND);
}

void CDXFShapeView::AddWorking(CDXFworking* pWork)
{
	HTREEITEM hTree = GetTreeCtrl().GetSelectedItem(), hParentTree;
	if ( IsRootTree(hTree) )
		return;
	CObject* pObject = (CObject *)(GetTreeCtrl().GetItemData(hTree));
	ASSERT( pObject );
	hParentTree = pObject->IsKindOf(RUNTIME_CLASS(CDXFworking)) ?
		GetTreeCtrl().GetParentItem(hTree) : hTree;
	ASSERT( hParentTree );
	hTree = GetTreeCtrl().InsertItem(TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM,
		LPSTR_TEXTCALLBACK, TREEIMG_WORK, TREEIMG_WORK, 0, 0,
		(LPARAM)pWork, hParentTree, TVI_LAST);
	ASSERT( hTree );
	GetTreeCtrl().Expand(hParentTree, TVE_EXPAND);
	GetTreeCtrl().SelectItem(hParentTree);	// 形状集合を選択
	GetTreeCtrl().EnsureVisible(hTree);
}

void CDXFShapeView::AutoWorkingDel(void)
{
	// 現在登録されている輪郭・ﾎﾟｹｯﾄ加工のﾂﾘｰを消去
	int		i, j;
	HTREEITEM		hLayerTree, hShapeTree, hTree;
	CDXFshape*		pShape;
	CDXFworking*	pPara;
	CPtrArray		obDel;
	obDel.SetSize(0, 64);

	for ( i=0; i<SIZEOF(m_hRootTree); i++ ) {
		hLayerTree = GetTreeCtrl().GetChildItem(m_hRootTree[i]);
		// ﾚｲﾔﾙｰﾌﾟ
		while ( hLayerTree ) {
			hShapeTree = GetTreeCtrl().GetChildItem(hLayerTree);
			// 形状ﾙｰﾌﾟ
			while ( hShapeTree ) {
				pShape = (CDXFshape *)(GetTreeCtrl().GetItemData(hShapeTree));
				ASSERT( pShape );
				hTree = GetTreeCtrl().GetChildItem(hShapeTree);
				// 現在の輪郭・ﾎﾟｹｯﾄ加工指示を削除するﾙｰﾌﾟ
				while ( hTree ) {
					pPara = (CDXFworking *)(GetTreeCtrl().GetItemData(hTree));
					ASSERT(pPara);
					switch ( pPara->GetWorkingType() ) {
					case OUTLINE:
					case POCKET:
						pShape->DelWorkingData(pPara);
						obDel.Add(hTree);
						break;
					}
					hTree = GetTreeCtrl().GetNextSiblingItem(hTree);
				}
				for ( j=0; j<obDel.GetSize(); j++ )
					GetTreeCtrl().DeleteItem((HTREEITEM)obDel[j]);
				obDel.RemoveAll();
				hShapeTree = GetTreeCtrl().GetNextSiblingItem(hShapeTree);
			}
			hLayerTree = GetTreeCtrl().GetNextSiblingItem(hLayerTree);
		}
	}
}

void CDXFShapeView::AutoWorkingSet(BOOL bAuto/*=TRUE*/)
{
	// 形状情報から加工指示を登録
	int		i, j, nLoop, nLayerLoop = GetDocument()->GetLayerCnt();
	POSITION	pos;
	HTREEITEM	hTree;
	CLayerData*			pLayer;
	CDXFshape*			pShape;
	CDXFworkingList*	pList;
	CDXFworking*		pWork;

	TVINSERTSTRUCT	tvInsert;
	::ZeroMemory(&tvInsert, sizeof(TVINSERTSTRUCT));
	tvInsert.hInsertAfter = TVI_LAST;
	tvInsert.item.mask = TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
	tvInsert.item.pszText = LPSTR_TEXTCALLBACK;
	tvInsert.item.iImage = tvInsert.item.iSelectedImage = TREEIMG_WORK;

	for ( i=0; i<nLayerLoop; i++ ) {
		pLayer = GetDocument()->GetLayerData(i);
		if ( !pLayer->IsCutType() )
			continue;
		nLoop = pLayer->GetShapeSize();
		for ( j=0; j<nLoop; j++ ) {
			pShape = pLayer->GetShapeData(j);
			hTree = pShape->GetTreeHandle();
			pList = pShape->GetWorkList();
			tvInsert.hParent = hTree;
			for ( pos=pList->GetHeadPosition(); pos; ) {
				pWork = pList->GetNext(pos);
				if ( !bAuto || pWork->IsAutoWorking() ) {
					tvInsert.item.lParam = (LPARAM)pWork;
					GetTreeCtrl().InsertItem(&tvInsert);
				}
			}
		}
	}
}

void CDXFShapeView::UpdateSequence(void)
{
	int			i;
	CLayerData*	pLayer;
	HTREEITEM	hLayerTree, hTree;

	// ﾚｲﾔ順が変更されていれば
	if ( m_bUpdateLayerSequence ) {
		for ( i=0; i<SIZEOF(m_hRootTree); i++ ) {
			// 子を持つ最初の集合の
			if ( GetTreeCtrl().ItemHasChildren(m_hRootTree[i]) ) {
				int	ii=0;
				// ﾚｲﾔ順を記録
				hLayerTree = GetTreeCtrl().GetChildItem(m_hRootTree[i]);
				while ( hLayerTree ) {
					pLayer = (CLayerData *)(GetTreeCtrl().GetItemData(hLayerTree));
					pLayer->SetListNo(ii++);
					hLayerTree = GetTreeCtrl().GetNextSiblingItem(hLayerTree);
				}
				// ﾚｲﾔ順を更新
				GetDocument()->UpdateLayerSequence();
				m_bUpdateLayerSequence = FALSE;
				break;
			}
		}
	}

	// 形状順が変更されていれば
	if ( !m_mpUpdateLayer.IsEmpty() ) {
		CShapeArray	obArray;
		obArray.SetSize(0, GetTreeCtrl().GetCount());
		CString		strLayer;
		// 全ての集合から該当ﾚｲﾔ配下の形状順を取得
		for ( POSITION pos=m_mpUpdateLayer.GetStartPosition(); pos; ) {
			m_mpUpdateLayer.GetNextAssoc(pos, strLayer, pLayer);
			for ( i=0; i<SIZEOF(m_hRootTree); i++ ) {
				hLayerTree = SearchLayerTree( GetTreeCtrl().GetChildItem(m_hRootTree[i]), pLayer );
				if ( hLayerTree ) {
					// 子ﾂﾘｰ(形状)を取得
					hTree = GetTreeCtrl().GetChildItem(hLayerTree);
					while ( hTree ) {
						obArray.Add( (CDXFshape *)(GetTreeCtrl().GetItemData(hTree)) );
						hTree = GetTreeCtrl().GetNextSiblingItem(hTree);
					}
				}
			}
			// 形状順を更新
			ASSERT( pLayer->GetShapeSize() == obArray.GetSize() );
			pLayer->CopyShape(obArray);
			obArray.RemoveAll();
		}
		m_mpUpdateLayer.RemoveAll();
	}
}

/////////////////////////////////////////////////////////////////////////////
// CDXFShapeView 診断

#ifdef _DEBUG
void CDXFShapeView::AssertValid() const
{
	CTreeView::AssertValid();
}

void CDXFShapeView::Dump(CDumpContext& dc) const
{
	CTreeView::Dump(dc);
}

CDXFDoc* CDXFShapeView::GetDocument() // 非デバッグ バージョンはインラインです。
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CDXFDoc)));
	return (CDXFDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CDXFShapeView クラスのメッセージ ハンドラ（メニュー編）

void CDXFShapeView::OnSortShape() 
{
	CLayerData*	pLayer;
	HTREEITEM	hLayerTree;

	for ( int i=0; i<SIZEOF(m_hRootTree); i++ ) {
		// ﾚｲﾔ情報の並べ替え
		GetTreeCtrl().SortChildren(m_hRootTree[i]);
		m_bUpdateLayerSequence = TRUE;
		// 形状順の並べ替え
		hLayerTree = GetTreeCtrl().GetChildItem(m_hRootTree[i]);
		while ( hLayerTree ) {
			GetTreeCtrl().SortChildren(hLayerTree);
			// 順序更新情報ｾｯﾄ
			if ( i == 0 ) {
				pLayer = (CLayerData *)(GetTreeCtrl().GetItemData(hLayerTree));
				m_mpUpdateLayer.SetAt(pLayer->GetStrLayer(), pLayer);
			}
			hLayerTree = GetTreeCtrl().GetNextSiblingItem(hLayerTree);
		}
	}

	// ﾄﾞｷｭﾒﾝﾄ変更通知
	GetDocument()->SetModifiedFlag();
}

void CDXFShapeView::OnUpdateShapePattern(CCmdUI* pCmdUI)
{
	HTREEITEM hTree = GetTreeCtrl().GetSelectedItem();
	BOOL	bEnable = TRUE;
	if ( !GetDocument()->IsShape() || !hTree || IsRootTree(hTree) )
		bEnable = FALSE;
	else {
		CObject* pObject = (CObject *)(GetTreeCtrl().GetItemData(hTree));
		if ( !pObject || !pObject->IsKindOf(RUNTIME_CLASS(CDXFshape)) )
			bEnable = FALSE;
		// 輪郭，ﾎﾟｹｯﾄ加工の場合は，さらに
		if ( pCmdUI->m_nID == ID_EDIT_SHAPE_OUT || pCmdUI->m_nID == ID_EDIT_SHAPE_POC ) {
			if ( ((CDXFshape *)pObject)->GetShapeFlag() & DXFMAPFLG_CANNOTOUTLINE )
				bEnable = FALSE;
		}
	}
	pCmdUI->Enable(bEnable);
	if ( bEnable )
		pCmdUI->SetCheck( pCmdUI->m_nID == GetDocument()->GetShapePattern() );
}

void CDXFShapeView::OnUpdateWorkingDel(CCmdUI* pCmdUI) 
{
	HTREEITEM hTree = GetTreeCtrl().GetSelectedItem();
	BOOL	bEnable = FALSE;
	if ( GetDocument()->IsShape() && hTree && !IsRootTree(hTree) ) {
		CDXFworking* pWork = (CDXFworking *)(GetTreeCtrl().GetItemData(hTree));
		bEnable = pWork && pWork->IsKindOf(RUNTIME_CLASS(CDXFworking));
	}
	pCmdUI->Enable(bEnable);
}

void CDXFShapeView::OnWorkingDel() 
{
	HTREEITEM hTree = GetTreeCtrl().GetSelectedItem();
	if ( hTree && !IsRootTree(hTree) ) {
		CDXFworking* pPara = (CDXFworking *)(GetTreeCtrl().GetItemData(hTree));
		if ( pPara && pPara->IsKindOf(RUNTIME_CLASS(CDXFworking)) ) {
			// ｱｲﾃﾑ消去
			HTREEITEM hParentTree = GetTreeCtrl().GetParentItem(hTree);
			CDXFshape* pShape = (CDXFshape *)(GetTreeCtrl().GetItemData(hParentTree));
			GetTreeCtrl().DeleteItem(hTree);
			pShape->DelWorkingData(pPara);
			// ﾄﾞｷｭﾒﾝﾄ変更通知
			GetDocument()->SetModifiedFlag();
			// ﾋﾞｭｰの再描画
			GetDocument()->UpdateAllViews(this);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// CDXFShapeView メッセージ ハンドラ

int CDXFShapeView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CTreeView::OnCreate(lpCreateStruct) == -1)
		return -1;

	// ｲﾒｰｼﾞﾘｽﾄ
	GetTreeCtrl().SetImageList(AfxGetNCVCMainWnd()->GetTreeImage(), TVSIL_NORMAL);

	return 0;
}

void CDXFShapeView::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	CMenu	menu;
	menu.LoadMenu(IDR_DXFPOPUP2);
	CMenu*	pMenu = menu.GetSubMenu(0);
	pMenu->TrackPopupMenu(TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_RIGHTBUTTON,
		point.x, point.y, AfxGetMainWnd());
}

void CDXFShapeView::OnGetDispInfo(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);

	CObject*	pParam = (CObject *)(pTVDispInfo->item.lParam);
	if ( pParam && pTVDispInfo->item.mask & TVIF_TEXT && !IsRootTree(pTVDispInfo->item.hItem) ) {
		if ( pParam->IsKindOf(RUNTIME_CLASS(CLayerData)) ) {
			lstrcpy(pTVDispInfo->item.pszText, ((CLayerData *)pParam)->GetStrLayer());
		}
		else if ( pParam->IsKindOf(RUNTIME_CLASS(CDXFshape)) ) {
			lstrcpy(pTVDispInfo->item.pszText, ((CDXFshape *)pParam)->GetShapeName());
		}
		else /* if ( pParam->IsKindOf(RUNTIME_CLASS(CDXFworking)) ) */ {
			lstrcpy(pTVDispInfo->item.pszText, ((CDXFworking *)pParam)->GetWorkingName());
		}
	}

	*pResult = 0;
}

void CDXFShapeView::OnBeginLabelEdit(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);

	CObject* pParam = (CObject *)(pTVDispInfo->item.lParam);
	// ﾙｰﾄとﾚｲﾔ名はｴﾃﾞｨｯﾄ出来ない
	*pResult = IsRootTree(pTVDispInfo->item.hItem) ||
				!pParam ||
				pParam->IsKindOf(RUNTIME_CLASS(CLayerData)) ? 1 : 0;
}

void CDXFShapeView::OnEndLabelEdit(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);

	CObject* pParam = (CObject *)(pTVDispInfo->item.lParam);
	ASSERT( pParam );

	if ( pTVDispInfo->item.pszText && lstrlen(pTVDispInfo->item.pszText) > 0 ) {
		if ( pParam->IsKindOf(RUNTIME_CLASS(CDXFshape)) ) {
			CDXFshape* pShape = (CDXFshape *)pParam;
			if ( pShape->GetShapeName() != pTVDispInfo->item.pszText ) {
				pShape->SetShapeName(pTVDispInfo->item.pszText);
				GetDocument()->SetModifiedFlag();
			}
		}
		else if ( pParam->IsKindOf(RUNTIME_CLASS(CDXFworking)) ) {
			CDXFworking* pWork = (CDXFworking *)pParam;
			if ( pWork->GetWorkingName() != pTVDispInfo->item.pszText ) {
				pWork->SetWorkingName(pTVDispInfo->item.pszText);
				GetDocument()->SetModifiedFlag();
			}
		}
	}

	*pResult = 0;
}

void CDXFShapeView::OnKeydown(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LPNMTVKEYDOWN pTVKeyDown = reinterpret_cast<LPNMTVKEYDOWN>(pNMHDR);
	HTREEITEM	hTree;

	switch ( pTVKeyDown->wVKey ) {
	case VK_DELETE:
		OnWorkingDel();
		break;
	case VK_ESCAPE:
		if ( m_bDragging )
			DragCancel(TRUE);
		break;
	case VK_F2:
		hTree = GetTreeCtrl().GetSelectedItem();
		if ( !IsRootTree(hTree) ) {
			CObject* pParam = (CObject *)GetTreeCtrl().GetItemData(hTree);
			if ( pParam && !pParam->IsKindOf(RUNTIME_CLASS(CLayerData)) )
				GetTreeCtrl().EditLabel(hTree);
		}
		break;
	}

	*pResult = 0;
}

void CDXFShapeView::OnSelChanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	
	CObject*	pObject[] = {
		(CObject *)(pNMTreeView->itemOld.lParam),
		(CObject *)(pNMTreeView->itemNew.lParam)
	};
	HTREEITEM	hItem[] = {
		pNMTreeView->itemOld.hItem,
		pNMTreeView->itemNew.hItem
	};
	DXFTREETYPE	vSelect[SIZEOF(pObject)];	// DXFView.h
	CDXFshape*		pShape;
	CDXFworking*	pWork;

	// 各ｵﾌﾞｼﾞｪｸﾄの選択ﾌﾗｸﾞ更新
	for ( int i=0; i<SIZEOF(pObject); i++ ) {
		if ( pObject[i] && hItem[i] ) {
			if ( IsRootTree(hItem[i]) ) {
				vSelect[i] = (DWORD)pObject[i];
				SetShapeSwitch_SubordinateTree(hItem[i], i);
			}
			else if ( pObject[i]->IsKindOf(RUNTIME_CLASS(CLayerData)) ) {
				vSelect[i] = (CLayerData *)pObject[i];
				SetShapeSwitch_SubordinateTree(hItem[i], i);
			}
			else if ( pObject[i]->IsKindOf(RUNTIME_CLASS(CDXFshape)) ) {
				pShape = (CDXFshape *)pObject[i];
				vSelect[i] = pShape;
				pShape->SetShapeSwitch(i);
			}
			else if ( pObject[i]->IsKindOf(RUNTIME_CLASS(CDXFworking)) ) {
				pWork = (CDXFworking *)pObject[i];
				vSelect[i] = pWork;
				pWork->SetSelectFlag(i);
			}
			else
				vSelect[i] = (DWORD)-1;	// 未選択
		}
		else
			vSelect[i] = (DWORD)-1;
	}

	// 選択表示の更新 ( To DXFView.cpp )
	GetDocument()->UpdateAllViews(this, UAV_DXFSHAPEUPDATE, (CObject *)vSelect);

	*pResult = 0;
}

void CDXFShapeView::OnBeginDrag(NMHDR *pNMHDR, LRESULT *pResult)
{
	ASSERT( !m_bDragging );
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	CPoint	pt;
	UINT	nFlags;
	*pResult = 0;

	::GetCursorPos(&pt);
	ScreenToClient(&pt);
	HTREEITEM	hTree = GetTreeCtrl().HitTest(pt, &nFlags);

	// ﾄﾞﾗｯｸﾞ許可ﾁｪｯｸ
	if ( !hTree || IsRootTree(hTree) )
		return;
	CObject*	pObject = (CObject *)(GetTreeCtrl().GetItemData(hTree));
	if ( !pObject || pObject->IsKindOf(RUNTIME_CLASS(CDXFworking)) )
		return;

	// ﾄﾞﾗｯｸﾞOK(CLayerData or CDXFshape)
	m_bDragging = TRUE;
	m_hItemDrag = hTree;
	// ﾄﾞﾗｯｸﾞ準備前に選択ｱｲﾃﾑの変更通知
	GetTreeCtrl().SelectItem(m_hItemDrag);	// OnSelChanged()
	// ﾄﾞﾗｯｸﾞ準備
	if ( pObject->IsKindOf(RUNTIME_CLASS(CLayerData)) ) {
		m_pDragLayer = (CLayerData *)pObject;
		m_pDragShape = NULL;
		m_dwDragRoot = GetParentAssemble(m_hItemDrag);
	}
	else {
		hTree = GetTreeCtrl().GetParentItem(m_hItemDrag);
		m_pDragLayer = (CLayerData *)(GetTreeCtrl().GetItemData(hTree));
		m_pDragShape = (CDXFshape *)pObject;
		m_dwDragRoot = (DWORD)(m_pDragShape->GetShapeAssemble()) + 1;
	}
	ASSERT(m_dwDragRoot);
	m_hItemDrop = NULL;
	ASSERT(!m_pImageList);
	m_pImageList = GetTreeCtrl().CreateDragImage(m_hItemDrag);
	ASSERT(m_pImageList);
	m_pImageList->DragShowNolock(TRUE);
	m_pImageList->SetDragCursorImage(0, CPoint(0, 0));
	::ShowCursor(FALSE);
	m_pImageList->BeginDrag(0, CPoint(0,0));
	m_pImageList->DragMove(pt);
	m_pImageList->DragEnter(this, pt);
	SetCapture();
}

void CDXFShapeView::OnMouseMove(UINT nFlags, CPoint pt)
{
	if ( m_bDragging ) {
		ASSERT(m_pImageList);
		m_pImageList->DragMove(pt);
		UINT		nFlags;
		HTREEITEM	hTree = GetTreeCtrl().HitTest(pt, &nFlags);
		// ﾄﾞﾛｯﾌﾟ先のﾁｪｯｸ
		if ( IsDropItem(hTree) ) {
			m_pImageList->DragLeave(this);
			GetTreeCtrl().SelectDropTarget(hTree);
			m_hItemDrop = hTree;
			m_pImageList->DragEnter(this, pt);
		}
		else
			m_hItemDrop = NULL;
	}

	CTreeView::OnMouseMove(nFlags, pt);
}

void CDXFShapeView::OnLButtonUp(UINT nFlags, CPoint point)
{
	if ( m_bDragging ) {
		if ( m_hItemDrop && m_hItemDrag!=m_hItemDrop ) {
			if ( m_pDragShape && nFlags&MK_CONTROL )
				DragLink();		// 結合処理
			else
				DragInsert();	// 移動処理
		}
		else
			DragCancel(TRUE);
	}
	CTreeView::OnLButtonUp(nFlags, point);
}

void CDXFShapeView::OnNMRclick(NMHDR *pNMHDR, LRESULT *pResult)
{
#ifdef _DEBUG
	CMagaDbg	dbg("OnNMRclick()\nStart");
#endif
	// OnRButtonUpが呼ばれない(??)，OnRButtonDownでは反応が悪い，のでその代わり
	if ( m_bDragging )
		DragCancel(TRUE);
	else
		OnContextMenu(this, CPoint(::GetMessagePos()));
	*pResult = 0;
}
