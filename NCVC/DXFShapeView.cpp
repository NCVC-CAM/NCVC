// DXFShapeView.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "DXFChild.h"
#include "DXFdata.h"
#include "DXFshape.h"
#include "Layer.h"
#include "DXFDoc.h"
#include "DXFView.h"
#include "DXFShapeView.h"
#include "ShapePropDlg.h"
#include "ThreadDlg.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

using namespace boost;

// �Ұ�ޕ\�����ޯ��
enum {
	TREEIMG_OUTLINE = 0,
	TREEIMG_TRACE,
	TREEIMG_EXCLUDE,
	TREEIMG_LAYER,
	TREEIMG_WORK,
	TREEIMG_CHAIN,
	TREEIMG_MAP
};

extern	LPTSTR	gg_RootTitle[] = {
	"�֊s�W��", "�O�ՏW��", "���O�W��"
};

/////////////////////////////////////////////////////////////////////////////
// CDXFShapeView

IMPLEMENT_DYNCREATE(CDXFShapeView, CTreeView)

BEGIN_MESSAGE_MAP(CDXFShapeView, CTreeView)
	//{{AFX_MSG_MAP(CDXFShapeView)
	ON_WM_CREATE()
	ON_WM_CONTEXTMENU()
	ON_WM_CHAR()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_NOTIFY_REFLECT(TVN_GETDISPINFO, &CDXFShapeView::OnGetDispInfo)
	ON_NOTIFY_REFLECT(TVN_BEGINLABELEDIT, &CDXFShapeView::OnBeginLabelEdit)
	ON_NOTIFY_REFLECT(TVN_ENDLABELEDIT, &CDXFShapeView::OnEndLabelEdit)
	ON_NOTIFY_REFLECT(TVN_KEYDOWN, &CDXFShapeView::OnKeyDown)
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, &CDXFShapeView::OnSelChanged)
	ON_NOTIFY_REFLECT(TVN_BEGINDRAG, &CDXFShapeView::OnBeginDrag)
	ON_COMMAND(ID_EDIT_SORTSHAPE, &CDXFShapeView::OnSortShape)
	ON_UPDATE_COMMAND_UI(ID_EDIT_SHAPE_DEL, &CDXFShapeView::OnUpdateWorkingDel)
	ON_COMMAND(ID_EDIT_SHAPE_DEL, &CDXFShapeView::OnWorkingDel)
	ON_UPDATE_COMMAND_UI(ID_EDIT_SHAPE_PROP, &CDXFShapeView::OnUpdateEditShapeProp)
	ON_COMMAND(ID_EDIT_SHAPE_PROP, &CDXFShapeView::OnEditShapeProp)
	ON_UPDATE_COMMAND_UI(ID_EDIT_SHAPE_NAME, &CDXFShapeView::OnUpdateEditShapeName)
	ON_COMMAND(ID_EDIT_SHAPE_NAME, &CDXFShapeView::OnEditShapeName)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDXFShapeView �N���X�̍\�z/����

CDXFShapeView::CDXFShapeView()
{
	m_bDragging = FALSE;
	m_pDragLayer = NULL;
	m_pDragShape = NULL;
	m_dwDragRoot = 0;
	m_pImageList = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CDXFShapeView �N���X�̃I�[�o���C�h�֐�

BOOL CDXFShapeView::PreCreateWindow(CREATESTRUCT& cs) 
{
	cs.style |= (TVS_HASBUTTONS|TVS_HASLINES|TVS_LINESATROOT|
		TVS_EDITLABELS|TVS_FULLROWSELECT|TVS_SHOWSELALWAYS);
	cs.dwExStyle |= WS_EX_OVERLAPPEDWINDOW;
	return CTreeView::PreCreateWindow(cs);
}

void CDXFShapeView::OnInitialUpdate() 
{
	CTreeView::OnInitialUpdate();

	int		i;
	TVINSERTSTRUCT	tvInsert;
	::ZeroMemory(&tvInsert, sizeof(TVINSERTSTRUCT));
	tvInsert.hInsertAfter = TVI_LAST;
	tvInsert.item.mask = TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
	tvInsert.item.iImage = tvInsert.item.iSelectedImage = TREEIMG_OUTLINE;

	// ٰ��ذ
	ASSERT( SIZEOF(m_hRootTree) == SIZEOF(gg_RootTitle) );
	tvInsert.hParent = TVI_ROOT;
	for ( i=0; i<SIZEOF(gg_RootTitle); i++ ) {
		tvInsert.item.iImage = tvInsert.item.iSelectedImage = TREEIMG_OUTLINE + i;
		tvInsert.item.lParam = (LPARAM)(i+1);		// RootID
		tvInsert.item.pszText = gg_RootTitle[i];
		m_hRootTree[i] = GetTreeCtrl().InsertItem(&tvInsert);
		ASSERT( m_hRootTree[i] );
	}

	// �رى���̏���
	if ( GetDocument()->IsDocFlag(DXFDOC_SHAPE) ) {
		// �`�����o�^
		SetShapeTree();
		// ���H�w����o�^
		AutoWorkingSet(FALSE);
		// �S�Ă��ذ���т�W�J
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
		return;		// �ĕ`��s�v
	case UAV_DXFSHAPEUPDATE:
		if ( pHint ) {			// from CDXFView::OnLButtonUp
			GetTreeCtrl().SelectItem( reinterpret_cast<CDXFshape*>(pHint)->GetTreeHandle() );
		}
		else {					// from CDXFDoc::OnEditShape
			SetShapeTree();
			SetFocus();
		}
		break;
	case UAV_DXFAUTOWORKING:	// from CDXFDoc::OnEditAutoShape
		if ( pHint )			// from CDXFDoc::OnEditStrictOffset
			return;
		AutoWorkingSet(TRUE);
		SetFocus();
		break;
	case UAV_DXFAUTODELWORKING:	// from CDXFDoc::OnEditAutoShape
		AutoWorkingDel();
		break;
	case UAV_DXFADDWORKING:		// from CDXFView::OnLButtonUp
		ASSERT( pHint );	// CDXFworking
		AddWorking( reinterpret_cast<CDXFworking*>(pHint) );
		break;
	case UAV_DXFADDSHAPE:		// from CDXFView::OnLButtonUp_Sel
		ASSERT( pHint );	// LPDXFADDSHAPE
		OnUpdateShape( reinterpret_cast<LPDXFADDSHAPE>(pHint) );
		break;
	}
	CTreeView::OnUpdate(pSender, lHint, pHint);
}

BOOL CDXFShapeView::PreTranslateMessage(MSG* pMsg) 
{
	// �ذ�ޭ�����è�ނȂƂ��ͷ�ү���ނ𒼐��ذ���۰قɑ���
	if ( pMsg->message == WM_KEYDOWN ) {
		// ���ް�ޱ���ڰ��܂ŕ߂܂��Ă��܂��̂ňړ����ނ��w��
		if ( GetTreeCtrl().GetEditControl() ) {
			// ��ި�Ē��Ȃ炱����ү���ނ��ި��߯�
			::TranslateMessage(pMsg);
			::DispatchMessage(pMsg);
			return TRUE;		// ��ި�ĺ��۰ق��K�؂ɏ���
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
			case VK_F2:		// ���ږ��ҏW�p
				GetTreeCtrl().SendMessage(WM_KEYDOWN, pMsg->wParam, pMsg->lParam);
				return TRUE;	// ��󷰓����K�؂ɏ��������
			}
		}
	}
	return CTreeView::PreTranslateMessage(pMsg);
}

/////////////////////////////////////////////////////////////////////////////
// CDXFShapeView �N���X�̃����o�֐�

void CDXFShapeView::OnUpdateShape(LPDXFADDSHAPE lpShape)
{
	TVINSERTSTRUCT	tvInsert;
	::ZeroMemory(&tvInsert, sizeof(TVINSERTSTRUCT));
	tvInsert.hInsertAfter = TVI_LAST;
	tvInsert.item.mask = TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
	tvInsert.item.pszText = LPSTR_TEXTCALLBACK;

	ASSERT( lpShape->pLayer );
	ASSERT( lpShape->pShape );

	// �`����̍폜
	BOOL	bExpand = FALSE;
	HTREEITEM hTree = lpShape->pShape->GetTreeHandle();
	if ( hTree ) {	// �V�K�̏ꍇ��NULL
		// �e����(ڲ�)�擾
		HTREEITEM hTreeParent = GetTreeCtrl().GetParentItem(hTree);
		// ���݂̽ð�����擾���ď���
		TVITEM	tvItem;
		::ZeroMemory(&tvItem, sizeof(TVITEM));
		tvItem.mask = TVIF_STATE;
		tvItem.hItem = hTree;
		GetTreeCtrl().GetItem(&tvItem);
		if ( tvItem.state & TVIS_EXPANDED )
			bExpand = TRUE;
		GetTreeCtrl().DeleteItem(hTree);
		// �폜��CڲԔz���ɑ��̌`���񂪖������
		if ( hTreeParent && !GetTreeCtrl().ItemHasChildren(hTreeParent) )
			GetTreeCtrl().DeleteItem(hTreeParent);	// ڲ��ذ���폜
	}

	// �����W���̌���
	int nID = (int)(lpShape->pShape->GetShapeAssemble());
	if ( nID == DXFSHAPE_OUTLINE ) {
		nID = 1;	// �O�ՏW��(�����ŗ֊s�W���Ɉړ������邱�Ƃ͂Ȃ�)
		lpShape->pShape->SetShapeAssemble(DXFSHAPE_LOCUS);
	}

	// �`����̓o�^
	HTREEITEM hLayerTree = SearchLayerTree(GetTreeCtrl().GetChildItem(m_hRootTree[nID]), lpShape->pLayer);
	if ( !hLayerTree ) {
		tvInsert.hParent = m_hRootTree[nID];
		tvInsert.item.iImage = tvInsert.item.iSelectedImage = TREEIMG_LAYER;
		tvInsert.item.lParam = reinterpret_cast<LPARAM>(lpShape->pLayer);
		hLayerTree = GetTreeCtrl().InsertItem(&tvInsert);
		ASSERT( hLayerTree );
	}
	tvInsert.hParent = hLayerTree;
	tvInsert.item.iImage = tvInsert.item.iSelectedImage = TREEIMG_CHAIN + lpShape->pShape->GetShapeType();
	tvInsert.item.lParam = reinterpret_cast<LPARAM>(lpShape->pShape);
	hTree = GetTreeCtrl().InsertItem(&tvInsert);
	lpShape->pShape->SetTreeHandle(hTree);

	// ���H�w���̓o�^
	tvInsert.hParent = hTree;
	tvInsert.item.iImage = tvInsert.item.iSelectedImage = TREEIMG_WORK;
	PLIST_FOREACH(auto ref, lpShape->pShape->GetWorkList())
		tvInsert.item.lParam = reinterpret_cast<LPARAM>(ref);
		GetTreeCtrl().InsertItem(&tvInsert);
	END_FOREACH
	PLIST_FOREACH(auto ref, lpShape->pShape->GetOutlineList())
		tvInsert.item.lParam = reinterpret_cast<LPARAM>(ref);
		GetTreeCtrl().InsertItem(&tvInsert);
	END_FOREACH
	if ( bExpand )
		GetTreeCtrl().Expand(hTree, TVE_EXPAND);
}

BOOL CDXFShapeView::IsDropItem(HTREEITEM hTree)
{
	if ( !hTree )
		return FALSE;

	if ( IsRootTree(hTree) ) {
		// �����W���ذ�ɂ���ۯ�߂ł��Ȃ�
		if ( m_dwDragRoot == GetParentAssemble(hTree) )
			return FALSE;
	}
	else {
		// ���H�w���ɂ���ۯ�߂ł��Ȃ�
		CObject*	pObject = reinterpret_cast<CObject *>(GetTreeCtrl().GetItemData(hTree));
		if ( !pObject || pObject->IsKindOf(RUNTIME_CLASS(CDXFworking)) )
			return FALSE;
		if ( m_pDragShape ) {
			// �`����̏ꍇ
			if ( pObject->IsKindOf(RUNTIME_CLASS(CLayerData)) ) {
				// ��ۯ�ߐ悪ڲԏ�����ׯ�ތ`�󂪏�������ڲԂƓ����ꍇ����
				if ( m_pDragLayer != static_cast<CLayerData *>(pObject) )
					return FALSE;
			}
			else {
				// ��ۯ�ߐ悪�`����Ȃ炻�̐e(ڲ�)���擾����
				CLayerData* pLayer = reinterpret_cast<CLayerData *>(GetTreeCtrl().GetItemData(GetTreeCtrl().GetParentItem(hTree)));
				// ��ׯ�ތ`�󂪏�������ڲԂƓ����ꍇ����
				if ( m_pDragLayer != pLayer )
					return FALSE;
			}
		}
		else {
			// ڲԂ̏ꍇ
			if ( !pObject->IsKindOf(RUNTIME_CLASS(CLayerData)) )
				return FALSE;
		}
	}

	return TRUE;
}

void CDXFShapeView::DragInsert(void)
{
	// ��ׯ�ވړ��O�̏�Ԃ��擾
	TVITEM	tvItem;
	::ZeroMemory(&tvItem, sizeof(TVITEM));
	tvItem.mask  = TVIF_STATE;
	tvItem.hItem = m_hItemDrag;
	GetTreeCtrl().GetItem(&tvItem);

	// m_hItemDrop��m_hItemDrag��}��
	HTREEITEM hTree = m_pDragShape ? DragInsertShape() : DragInsertLayer();
	if ( !hTree )
		return;

	// ��ׯ���ذ���폜
	HTREEITEM hParentTree = GetTreeCtrl().GetParentItem(m_hItemDrag);
	GetTreeCtrl().DeleteItem(m_hItemDrag);
	// �ړ���őI���C��ԕ���
	GetTreeCtrl().SelectItem(hTree);
	if ( tvItem.state & TVIS_EXPANDED )
		GetTreeCtrl().Expand(hTree, TVE_EXPAND);
	Invalidate();

	// �ύX�L�^��ۑ�
	if ( m_pDragShape ) {
		// �`�������ׯ�ވړ��ŏ���ڲԔz���Ɍ`���񂪖����Ȃ��
		if ( !GetTreeCtrl().ItemHasChildren(hParentTree) )
			GetTreeCtrl().DeleteItem(hParentTree);	// ڲ��ذ���폜
	}

	// �޷���ĕύX�ʒm
	GetDocument()->SetModifiedFlag();
	DragCancel(FALSE);
}

HTREEITEM CDXFShapeView::DragInsertLayer(void)
{
	DWORD_PTR	dwRoot = GetParentAssemble(m_hItemDrop);
	BOOL		bCanNotOutline = FALSE, bOutline = FALSE;
	HTREEITEM	hLayerTree = NULL, hShapeTree;
	CDXFshape*	pShape;

	// �z���̌`�������è�m�F
	hShapeTree = GetTreeCtrl().GetChildItem(m_hItemDrag);
	while ( hShapeTree ) {
		pShape = reinterpret_cast<CDXFshape *>(GetTreeCtrl().GetItemData(hShapeTree));
		if ( dwRoot==ROOTTREE_SHAPE && pShape->GetShapeFlag()&DXFMAPFLG_CANNOTAUTOWORKING ) {
			bCanNotOutline = TRUE;
			break;	// �ȍ~�m�F�K�v�Ȃ�
		}
		if ( dwRoot==ROOTTREE_LOCUS ) {
			if ( pShape->IsOutlineList() ) {
				bOutline = TRUE;
				break;
			}
		}
		hShapeTree = GetTreeCtrl().GetNextSiblingItem(hShapeTree);
	}
	switch ( dwRoot ) {
	case ROOTTREE_SHAPE:
		// �ŏI��ۯ�ߋ�������
		if ( bCanNotOutline ) {
			DragCancel(TRUE);
			AfxMessageBox(IDS_ERR_DXF_DRAGOUTLINE, MB_OK|MB_ICONEXCLAMATION);
			return NULL;
		}
		break;
	case ROOTTREE_LOCUS:
		// �֊s���H�w�����������m�F
		if ( bOutline ) {
			DragCancel(FALSE, FALSE);	// ���Ă����Ȃ���ϳ����ٔ�\���̂܂�
			if ( AfxMessageBox(IDS_ANA_OUTLINE, MB_YESNO|MB_ICONQUESTION) != IDYES )
				return NULL;
		}
		break;
	}

	TVINSERTSTRUCT	tvInsert;
	::ZeroMemory(&tvInsert, sizeof(TVINSERTSTRUCT));
	tvInsert.item.mask = TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
	tvInsert.item.pszText = LPSTR_TEXTCALLBACK;
	TVITEM	tvItem;
	::ZeroMemory(&tvItem, sizeof(TVITEM));
	tvItem.mask = TVIF_STATE;

	// ڲԏ��̓o�^
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
		tvInsert.item.lParam = reinterpret_cast<LPARAM>(m_pDragLayer);
		hLayerTree = GetTreeCtrl().InsertItem(&tvInsert);
		if ( !hLayerTree ) {
			DragCancel(TRUE);
			AfxMessageBox(IDS_ERR_DRAGTREE, MB_OK|MB_ICONEXCLAMATION);
			return NULL;
		}
		GetTreeCtrl().Expand(tvInsert.hParent, TVE_EXPAND);
	}

	// �`����Ɖ��H�w���̓o�^
	tvInsert.hInsertAfter = TVI_LAST;
	hShapeTree = GetTreeCtrl().GetChildItem(m_hItemDrag);
	while ( hShapeTree ) {
		// �ړ��O�̏�Ԃ��擾
		tvItem.hItem = hShapeTree;
		GetTreeCtrl().GetItem(&tvItem);
		// �`����
		pShape = reinterpret_cast<CDXFshape *>(GetTreeCtrl().GetItemData(hShapeTree));
		tvInsert.item.iImage = tvInsert.item.iSelectedImage = TREEIMG_CHAIN + pShape->GetShapeType();
		tvInsert.hParent = hLayerTree;
		tvInsert.item.lParam = reinterpret_cast<LPARAM>(pShape);
		tvInsert.hParent = GetTreeCtrl().InsertItem(&tvInsert);	// ���H�w���̐e�ذ
		if ( !tvInsert.hParent ) {
			DragCancel(TRUE);
			AfxMessageBox(IDS_ERR_DRAGTREE, MB_OK|MB_ICONEXCLAMATION);
			return NULL;
		}
		pShape->SetTreeHandle(tvInsert.hParent);
		// �����W���̍X�V
		if ( m_dwDragRoot != dwRoot )
			pShape->SetShapeAssemble((DXFSHAPE_ASSEMBLE)(dwRoot-1));
		// ���H�w��
		tvInsert.item.iImage = tvInsert.item.iSelectedImage = TREEIMG_WORK;
		PLIST_FOREACH(auto ref, pShape->GetWorkList())
			tvInsert.item.lParam = reinterpret_cast<LPARAM>(ref);
			GetTreeCtrl().InsertItem(&tvInsert);
		END_FOREACH
		if ( dwRoot==ROOTTREE_LOCUS && bOutline ) {
			// ���ذ���c���Ă����Ԃ��߲���폜���邽��
			// �ȍ~�ذ����������OnGetDispInfo()�Ŵװ�̉\��������
			pShape->DelOutlineData();	// �֊s���H�w���S�폜
		}
		else {
			PLIST_FOREACH(auto ref, pShape->GetOutlineList())
				tvInsert.item.lParam = reinterpret_cast<LPARAM>(ref);
				GetTreeCtrl().InsertItem(&tvInsert);
			END_FOREACH
		}
		// �ړ��O�̏�Ԃ𕜌�
		if ( tvItem.state & TVIS_EXPANDED )
			GetTreeCtrl().Expand(tvInsert.hParent, TVE_EXPAND);
		// ���̌`����
		hShapeTree = GetTreeCtrl().GetNextSiblingItem(hShapeTree);
	}

	return hLayerTree;
}

HTREEITEM CDXFShapeView::DragInsertShape(void)
{
	DWORD_PTR	dwRoot = GetParentAssemble(m_hItemDrop);

	switch ( dwRoot ) {
	case ROOTTREE_SHAPE:
		// �ŏI��ۯ�ߋ�������
		if ( m_pDragShape->GetShapeFlag() & DXFMAPFLG_CANNOTAUTOWORKING ) {
			DragCancel(TRUE);
			AfxMessageBox(IDS_ERR_DXF_DRAGOUTLINE, MB_OK|MB_ICONEXCLAMATION);
			return NULL;
		}
		break;
	case ROOTTREE_LOCUS:
		// �֊s���H�w�����������m�F
		if ( m_pDragShape->IsOutlineList() ) {
			DragCancel(FALSE, FALSE);
			if ( AfxMessageBox(IDS_ANA_OUTLINE, MB_YESNO|MB_ICONQUESTION) != IDYES )
				return NULL;
		}
		break;
	}

	HTREEITEM	hTree;
	TVINSERTSTRUCT	tvInsert;
	::ZeroMemory(&tvInsert, sizeof(TVINSERTSTRUCT));
	tvInsert.item.mask = TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
	tvInsert.item.pszText = LPSTR_TEXTCALLBACK;

	// �`����̓o�^
	if ( IsRootTree(m_hItemDrop) ) {
		// ��ۯ�ߐ悪ٰ��ذ��ڲԏ�񂪂Ȃ���Βǉ�
		hTree = SearchLayerTree(GetTreeCtrl().GetChildItem(m_hItemDrop), m_pDragLayer);
		if ( !hTree ) {
			tvInsert.hParent = m_hItemDrop;
			tvInsert.hInsertAfter = TVI_LAST;
			tvInsert.item.iImage = tvInsert.item.iSelectedImage = TREEIMG_LAYER;
			tvInsert.item.lParam = reinterpret_cast<LPARAM>(m_pDragLayer);
			hTree = GetTreeCtrl().InsertItem(&tvInsert);
			if ( !hTree ) {
				DragCancel(TRUE);
				AfxMessageBox(IDS_ERR_DRAGTREE, MB_OK|MB_ICONEXCLAMATION);
				return NULL;
			}
			GetTreeCtrl().Expand(tvInsert.hParent, TVE_EXPAND);
			m_hItemDrop = NULL;
		}
		else {
			m_hItemDrop = GetTreeCtrl().GetChildItem(m_hItemDrop);
		}
	}
	else {
		CObject* pObject = reinterpret_cast<CObject *>(GetTreeCtrl().GetItemData(m_hItemDrop));
		if ( pObject->IsKindOf(RUNTIME_CLASS(CLayerData)) ) {
			hTree = m_hItemDrop;
			m_hItemDrop = GetTreeCtrl().GetChildItem(m_hItemDrop);
		}
		else {
			hTree = GetTreeCtrl().GetParentItem(m_hItemDrop);
		}
	}

	// hTree�ɂ�ڲԁCm_hItemDrop�ɂ͌`����ذ�����(�܂���NULL)�������Ă���
	tvInsert.hParent = hTree;
	hTree = GetTreeCtrl().GetPrevSiblingItem(m_hItemDrop);
	tvInsert.hInsertAfter = hTree ? hTree : TVI_FIRST;	// ��ۯ�ߐ�
	tvInsert.item.iImage = tvInsert.item.iSelectedImage = TREEIMG_CHAIN + m_pDragShape->GetShapeType();
	tvInsert.item.lParam = reinterpret_cast<LPARAM>(m_pDragShape);
	hTree = GetTreeCtrl().InsertItem(&tvInsert);
	if ( !hTree ) {
		DragCancel(TRUE);
		AfxMessageBox(IDS_ERR_DRAGTREE, MB_OK|MB_ICONEXCLAMATION);
		return NULL;
	}
	m_pDragShape->SetTreeHandle(hTree);
	// �����W���̍X�V
	if ( m_dwDragRoot != dwRoot )
		m_pDragShape->SetShapeAssemble((DXFSHAPE_ASSEMBLE)(dwRoot-1));

	// ���H�w���̓o�^
	tvInsert.hInsertAfter = TVI_LAST;
	tvInsert.hParent = hTree;
	tvInsert.item.iImage = tvInsert.item.iSelectedImage = TREEIMG_WORK;
	PLIST_FOREACH(auto ref, m_pDragShape->GetWorkList())
		tvInsert.item.lParam = reinterpret_cast<LPARAM>(ref);
		GetTreeCtrl().InsertItem(&tvInsert);
	END_FOREACH
	if ( dwRoot==ROOTTREE_LOCUS )
		m_pDragShape->DelOutlineData();	// �֊s���H�w���S�폜
	else {
		PLIST_FOREACH(auto ref, m_pDragShape->GetOutlineList())
			tvInsert.item.lParam = reinterpret_cast<LPARAM>(ref);
			GetTreeCtrl().InsertItem(&tvInsert);
		END_FOREACH
	}

	return hTree;
}

void CDXFShapeView::DragLink(void)
{
	// �ŏI������������
	if ( IsRootTree(m_hItemDrop) ) {
		DragCancel(TRUE);
		::MessageBeep(MB_ICONEXCLAMATION);
		return;
	}
	CDXFshape*	pShape = reinterpret_cast<CDXFshape *>(GetTreeCtrl().GetItemData(m_hItemDrop));
	CLayerData*	pLayer = reinterpret_cast<CLayerData *>(GetTreeCtrl().GetItemData(GetTreeCtrl().GetParentItem(m_hItemDrop)));
	if ( !pShape || !pShape->IsKindOf(RUNTIME_CLASS(CDXFshape)) || m_pDragLayer!=pLayer ) {
		DragCancel(TRUE);
		::MessageBeep(MB_ICONEXCLAMATION);
		return;
	}
	if ( pShape->IsOutlineList() ) {
		DragCancel(FALSE, FALSE);
		if ( AfxMessageBox(IDS_ANA_OUTLINE, MB_YESNO|MB_ICONQUESTION) != IDYES )
			return;
	}

	// ��������(��ۯ�߱��т���ׯ�ޱ��т�����)
	if ( !pShape->LinkShape(m_pDragShape) ) {
		DragCancel(TRUE);
		AfxMessageBox(IDS_ERR_DXF_DRAGLINK, MB_OK|MB_ICONEXCLAMATION);
		return;
	}
	
	// ��ׯ�ތ��ذ�̍폜
	GetTreeCtrl().DeleteItem(m_hItemDrag);
	// ��ׯ�ތ����э폜
	m_pDragLayer->RemoveShape(m_pDragShape);
	// ��ׯ�ސ�̉��H�w���ذ���폜
	HTREEITEM hTree = GetTreeCtrl().GetChildItem(m_hItemDrop), hTreeNext;
	while ( hTree ) {
		hTreeNext = GetTreeCtrl().GetNextSiblingItem(hTree);
		GetTreeCtrl().DeleteItem(hTree);
		hTree = hTreeNext;
	}
	// ��ׯ�ސ�̏����W������(�~�i)
	if ( GetParentAssemble(m_hItemDrop)==ROOTTREE_SHAPE && !pShape->GetShapeChain() ) {
		// �����W���̕ύX
		DXFADDSHAPE	addShape;	// DocBase.h
		addShape.pLayer = pLayer;
		addShape.pShape = pShape;
		OnUpdateShape(&addShape);	// ���H�w�����o�^
		// �����汲�т̑I��
		GetTreeCtrl().SelectItem( pShape->GetTreeHandle() );
		// �ĕ`��(���Ȃ��ƕ\���������)
		Invalidate();
	}
	else {
		// ���H�w���̍ēo�^(�֊s�w���͖���)
		PLIST_FOREACH(auto ref, pShape->GetWorkList())
			hTree = GetTreeCtrl().InsertItem(TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM,
				LPSTR_TEXTCALLBACK, TREEIMG_WORK, TREEIMG_WORK, 0, 0,
				reinterpret_cast<LPARAM>(ref), m_hItemDrop, TVI_LAST);
			ASSERT( hTree );
		END_FOREACH
		// �����汲�т̑I��
		GetTreeCtrl().SelectItem(m_hItemDrop);
	}
	// ��޼ު�đI��
	pShape->SetShapeSwitch(TRUE);
	// �޷���ĕύX�ʒm
	GetDocument()->SetModifiedFlag();
	DragCancel(FALSE);
}

void CDXFShapeView::DragCancel(BOOL bCancel, BOOL bHandleClean/*=TRUE*/)
{
	if ( m_bDragging ) {
		m_bDragging = FALSE;
		ASSERT( m_pImageList );
		m_pImageList->DragLeave(this);
		m_pImageList->EndDrag();
		delete m_pImageList;
		m_pImageList = NULL;
		::ShowCursor(TRUE);
		ReleaseCapture();
		GetTreeCtrl().SelectDropTarget(NULL);
	}
	if ( bCancel ) {
		GetTreeCtrl().SelectItem(m_hItemDrag);
		m_hItemDrag = m_hItemDrop = NULL;
	}
	if ( bHandleClean ) {
		m_pDragLayer = NULL;
		m_pDragShape = NULL;
		m_dwDragRoot = 0;
	}
}

HTREEITEM CDXFShapeView::SearchLayerTree(HTREEITEM hTree, const CLayerData* pLayer)
{
	HTREEITEM	hLayerTree = NULL;

	while ( hTree ) {
		if ( pLayer == reinterpret_cast<CLayerData *>(GetTreeCtrl().GetItemData(hTree)) ) {
			hLayerTree = hTree;
			break;
		}
		hTree = GetTreeCtrl().GetNextSiblingItem(hTree);
	}

	return hLayerTree;
}

DWORD_PTR CDXFShapeView::GetParentAssemble(HTREEITEM hTree)
{
	DWORD_PTR	dwResult = 0;

	// �ذ����ق�������W����Ԃ�
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
	CObject*	pObject = reinterpret_cast<CObject *>(GetTreeCtrl().GetItemData(hTree));
	if ( !pObject )
		return;

	// �`��W���̑I���׸ނ��X�V
	CDXFshape*	pShape;
	if ( pObject->IsKindOf(RUNTIME_CLASS(CLayerData)) ) {
		HTREEITEM	hLayerTree = hTree;
		while ( hLayerTree ) {
			hTree = GetTreeCtrl().GetChildItem(hLayerTree);
			while ( hTree ) {
				pShape = reinterpret_cast<CDXFshape *>(GetTreeCtrl().GetItemData(hTree));
				ASSERT( pShape->IsKindOf(RUNTIME_CLASS(CDXFshape)) );
				pShape->SetShapeSwitch(bSelect);
				hTree = GetTreeCtrl().GetNextSiblingItem(hTree);
			}
			hLayerTree = GetTreeCtrl().GetNextSiblingItem(hLayerTree);
		}
	}
	else {
		while ( hTree ) {
			pShape = reinterpret_cast<CDXFshape *>(GetTreeCtrl().GetItemData(hTree));
			ASSERT( pShape->IsKindOf(RUNTIME_CLASS(CDXFshape)) );
			pShape->SetShapeSwitch(bSelect);
			hTree = GetTreeCtrl().GetNextSiblingItem(hTree);
		}
	}
}

void CDXFShapeView::SetShapeTree(void)
{
	INT_PTR			i, j, k, nLoop;
	const INT_PTR	nLayerLoop = GetDocument()->GetLayerCnt();
	CLayerData*	pLayer;
	CDXFshape*	pShape;
	HTREEITEM	hLayerTree;

	TVINSERTSTRUCT	tvInsert;
	::ZeroMemory(&tvInsert, sizeof(TVINSERTSTRUCT));
	tvInsert.hInsertAfter = TVI_LAST;
	tvInsert.item.mask = TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
	tvInsert.item.pszText = LPSTR_TEXTCALLBACK;

	// �`����̓o�^
	for ( i=0; i<nLayerLoop; i++ ) {
		pLayer = GetDocument()->GetLayerData(i);
		if ( !pLayer->IsCutType() )
			continue;
		nLoop = pLayer->GetShapeSize();
		for ( j=0; j<nLoop; j++ ) {
			pShape = pLayer->GetShapeData(j);
			k = (int)pShape->GetShapeAssemble();	// �֊s[0],�O��[1],���O[2]
			ASSERT( 0<=k && k<SIZEOF(m_hRootTree) );
			// ڲԌ���
			hLayerTree = SearchLayerTree(GetTreeCtrl().GetChildItem(m_hRootTree[k]), pLayer);
			if ( !hLayerTree ) {
				tvInsert.hParent = m_hRootTree[k];
				tvInsert.item.iImage = tvInsert.item.iSelectedImage = TREEIMG_LAYER;
				tvInsert.item.lParam = reinterpret_cast<LPARAM>(pLayer);
				hLayerTree = GetTreeCtrl().InsertItem(&tvInsert);
				ASSERT( hLayerTree );
			}
			tvInsert.hParent = hLayerTree;
			tvInsert.item.iImage = tvInsert.item.iSelectedImage = TREEIMG_CHAIN + pShape->GetShapeType();
			tvInsert.item.lParam = reinterpret_cast<LPARAM>(pShape);
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
	CObject* pObject = reinterpret_cast<CObject *>(GetTreeCtrl().GetItemData(hTree));
	ASSERT( pObject );
	hParentTree = pObject->IsKindOf(RUNTIME_CLASS(CDXFworking)) ?
		GetTreeCtrl().GetParentItem(hTree) : hTree;
	ASSERT( hParentTree );
	hTree = GetTreeCtrl().InsertItem(TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM,
		LPSTR_TEXTCALLBACK, TREEIMG_WORK, TREEIMG_WORK, 0, 0,
		reinterpret_cast<LPARAM>(pWork), hParentTree, TVI_LAST);
	ASSERT( hTree );
	GetTreeCtrl().Expand(hParentTree, TVE_EXPAND);
	GetTreeCtrl().SelectItem(hParentTree);	// �`��W����I��
	GetTreeCtrl().EnsureVisible(hTree);
}

void CDXFShapeView::AutoWorkingDel
	(const CLayerData* pLayerSrc/*=NULL*/, const CDXFshape* pShapeSrc/*=NULL*/)
{
	// ���ݓo�^����Ă���֊s�E�߹�ĉ��H���ذ������
	HTREEITEM		hLayerTree, hShapeTree, hTree, hTreeNext;
	CDXFshape*		pShape;
	CDXFworking*	pWork;

	// �֊s�W���݂̂��Ώ�
	hLayerTree = GetTreeCtrl().GetChildItem(m_hRootTree[0]);
	// ڲ�ٰ��
	while ( hLayerTree ) {
		if ( pLayerSrc && pLayerSrc!=reinterpret_cast<CLayerData *>(GetTreeCtrl().GetItemData(hLayerTree)) )
			hShapeTree = NULL;
		else
			hShapeTree = GetTreeCtrl().GetChildItem(hLayerTree);
		// �`��ٰ��
		while ( hShapeTree ) {
			pShape = reinterpret_cast<CDXFshape *>(GetTreeCtrl().GetItemData(hShapeTree));
			ASSERT( pShape );
			if ( pShapeSrc && pShapeSrc!=pShape )
				hTree = NULL;
			else
				hTree = GetTreeCtrl().GetChildItem(hShapeTree);
			// ���݂̗֊s�E�߹�ĉ��H�w�����폜����ٰ��
			while ( hTree ) {
				pWork = reinterpret_cast<CDXFworking *>(GetTreeCtrl().GetItemData(hTree));
				ASSERT(pWork);
				hTreeNext = GetTreeCtrl().GetNextSiblingItem(hTree);
				if ( pWork->GetWorkingType() >= WORK_OUTLINE )	// WORK_OUTLINE, WORK_POCKET
					GetTreeCtrl().DeleteItem(hTree);
				hTree = hTreeNext;
			}
			pShape->DelOutlineData();
			hShapeTree = GetTreeCtrl().GetNextSiblingItem(hShapeTree);
		}
		hLayerTree = GetTreeCtrl().GetNextSiblingItem(hLayerTree);
	}
}

void CDXFShapeView::AutoWorkingSet
	(BOOL bAuto, const CLayerData* pLayerSrc/*=NULL*/, const CDXFshape* pShapeSrc/*=NULL*/)
{
	// �`���񂩂���H�w����o�^
	INT_PTR			i, j, nLoop;
	const INT_PTR	nLayerLoop = GetDocument()->GetLayerCnt();
	HTREEITEM		hTree;
	CLayerData*		pLayer;
	CDXFshape*		pShape;

	TVINSERTSTRUCT	tvInsert;
	::ZeroMemory(&tvInsert, sizeof(TVINSERTSTRUCT));
	tvInsert.hInsertAfter = TVI_LAST;
	tvInsert.item.mask = TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
	tvInsert.item.pszText = LPSTR_TEXTCALLBACK;
	tvInsert.item.iImage = tvInsert.item.iSelectedImage = TREEIMG_WORK;

	for ( i=0; i<nLayerLoop; i++ ) {
		pLayer = GetDocument()->GetLayerData(i);
		if ( !pLayer->IsCutType() || (pLayerSrc && pLayerSrc!=pLayer) )
			continue;
		nLoop = pLayer->GetShapeSize();
		for ( j=0; j<nLoop; j++ ) {
			pShape = pLayer->GetShapeData(j);
			if ( pShapeSrc && pShapeSrc!=pShape )
				continue;
			hTree = pShape->GetTreeHandle();
			tvInsert.hParent = hTree;
			if ( !bAuto ) {
				// �����E�J�n�ʒu�Ȃǂ̉��H�w���ɂ� IsAutoWorking() ������
				PLIST_FOREACH(auto ref, pShape->GetWorkList())
					tvInsert.item.lParam = reinterpret_cast<LPARAM>(ref);
					GetTreeCtrl().InsertItem(&tvInsert);
				END_FOREACH
			}
			PLIST_FOREACH(auto ref, pShape->GetOutlineList())
				if ( !bAuto || ref->IsAutoWorking() ) {
					tvInsert.item.lParam = reinterpret_cast<LPARAM>(ref);
					GetTreeCtrl().InsertItem(&tvInsert);
				}
			END_FOREACH
			GetTreeCtrl().Expand(hTree, TVE_EXPAND);
		}
	}
}

void CDXFShapeView::UpdateSequence(void)
{
	int			i, n = 0;
	BOOL		bLayer = TRUE;
	CLayerData*	pLayer;
	CDXFshape*	pShape;
	HTREEITEM	hLayerTree, hTree;

	// ڲԂƌ`��W�����̍X�V
	for ( i=0; i<SIZEOF(m_hRootTree); i++ ) {
		hLayerTree = GetTreeCtrl().GetChildItem(m_hRootTree[i]);
		if ( !hLayerTree )
			continue;
		// �q�����ŏ��̏W����ڲԏ����L�^
		if ( bLayer ) {
			hTree = hLayerTree;
			for ( int j=0; hTree; j++ ) {
				pLayer = reinterpret_cast<CLayerData *>(GetTreeCtrl().GetItemData(hTree));
				pLayer->SetLayerListNo(j);
				hTree = GetTreeCtrl().GetNextSiblingItem(hTree);
			}
			// ڲԏ����X�V
			GetDocument()->UpdateLayerSequence();
			bLayer = FALSE;
		}
		// �`�󏇂̎擾
		pLayer = reinterpret_cast<CLayerData *>(GetTreeCtrl().GetItemData(hLayerTree));
		for ( hTree=GetTreeCtrl().GetChildItem(hLayerTree); hTree; n++ ) {
			pShape = reinterpret_cast<CDXFshape *>(GetTreeCtrl().GetItemData(hTree));
			pShape->SetSerializeSeq(n);		// �ذ�S�̂Œʂ��ԍ�
			hTree = GetTreeCtrl().GetNextSiblingItem(hTree);
		}
		// �`�󏇂��X�V
		pLayer->SerializeShapeSort();
	}
}

/////////////////////////////////////////////////////////////////////////////
// CDXFShapeView �f�f

#ifdef _DEBUG
void CDXFShapeView::AssertValid() const
{
	CTreeView::AssertValid();
}

void CDXFShapeView::Dump(CDumpContext& dc) const
{
	CTreeView::Dump(dc);
}

CDXFDoc* CDXFShapeView::GetDocument() // ��f�o�b�O �o�[�W�����̓C�����C���ł��B
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CDXFDoc)));
	return static_cast<CDXFDoc *>(m_pDocument);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CDXFShapeView �N���X�̃��b�Z�[�W �n���h���i���j���[�ҁj

void CDXFShapeView::OnSortShape() 
{
	HTREEITEM	hLayerTree;

	for ( int i=0; i<SIZEOF(m_hRootTree); i++ ) {
		// ڲԏ��̕��בւ�
		GetTreeCtrl().SortChildren(m_hRootTree[i]);
		// �`�󏇂̕��בւ�
		hLayerTree = GetTreeCtrl().GetChildItem(m_hRootTree[i]);
		while ( hLayerTree ) {
			GetTreeCtrl().SortChildren(hLayerTree);
			hLayerTree = GetTreeCtrl().GetNextSiblingItem(hLayerTree);
		}
	}

	// �޷���ĕύX�ʒm
	GetDocument()->SetModifiedFlag();
}

void CDXFShapeView::OnUpdateWorkingDel(CCmdUI* pCmdUI) 
{
	HTREEITEM hTree = GetTreeCtrl().GetSelectedItem();
	BOOL	bEnable = FALSE;
	if ( GetDocument()->IsDocFlag(DXFDOC_SHAPE) && hTree && !IsRootTree(hTree) ) {
		CDXFworking* pWork = reinterpret_cast<CDXFworking *>(GetTreeCtrl().GetItemData(hTree));
		bEnable = pWork && pWork->IsKindOf(RUNTIME_CLASS(CDXFworking));
	}
	pCmdUI->Enable(bEnable);
}

void CDXFShapeView::OnWorkingDel() 
{
	HTREEITEM hTree = GetTreeCtrl().GetSelectedItem();
	if ( hTree && !IsRootTree(hTree) ) {
		CDXFworking* pWork = reinterpret_cast<CDXFworking *>(GetTreeCtrl().GetItemData(hTree));
		if ( pWork && pWork->IsKindOf(RUNTIME_CLASS(CDXFworking)) ) {
			// ���я���
			HTREEITEM hParentTree = GetTreeCtrl().GetParentItem(hTree);
			CDXFshape* pShape = reinterpret_cast<CDXFshape *>(GetTreeCtrl().GetItemData(hParentTree));
			GetTreeCtrl().DeleteItem(hTree);
			if ( pWork->GetWorkingType() >= WORK_OUTLINE )	// WORK_OUTLINE, WORK_POCKET
				pShape->DelOutlineData(static_cast<CDXFworkingOutline*>(pWork));
			else
				pShape->DelWorkingData(pWork);
			// �޷���ĕύX�ʒm
			GetDocument()->SetModifiedFlag();
			// �ޭ��̍ĕ`��
			GetDocument()->UpdateAllViews(this);
		}
	}
}

void CDXFShapeView::OnUpdateEditShapeProp(CCmdUI *pCmdUI)
{
	HTREEITEM	hTree = GetTreeCtrl().GetSelectedItem();
	if ( hTree ) {
		if ( hTree != m_hRootTree[0] ) {	// į�ߏW���͗֊s�̂�
			if ( IsRootTree(hTree) )
				hTree = NULL;
			else {
				CObject* pSelect = reinterpret_cast<CObject *>(GetTreeCtrl().GetItemData(hTree));
				if ( !pSelect || pSelect->IsKindOf(RUNTIME_CLASS(CDXFworking)) )
					hTree = NULL;			// ���H�w���ɂ������è����
			}
		}
	}
	pCmdUI->Enable( hTree ? TRUE : FALSE );
}

void CDXFShapeView::OnEditShapeProp()
{
	HTREEITEM	hTree, hParentTree,
				hSelectTree = GetTreeCtrl().GetSelectedItem();
	if ( !hSelectTree )
		return;

	CObject*	pSelect = reinterpret_cast<CObject *>(GetTreeCtrl().GetItemData(hSelectTree));
	if ( !pSelect )
		return;

	BOOL	bChain = FALSE, bRecalc = FALSE;
	int		nShape = -1;
	optional<float>	dOffset;
	BOOL			bAcute;
	CLayerData*		pLayer;
	CDXFshape*		pShape;
	DXFTREETYPE		vSelect;

	// ���̵݂̾�Ēl�Ə����W���󋵂��擾
	if ( hSelectTree == m_hRootTree[0] ) {
		vSelect = (DWORD_PTR)pSelect;
		dOffset = HUGE_VALF;
		bAcute  = TRUE;
		hParentTree = GetTreeCtrl().GetChildItem(hSelectTree);
		while ( hParentTree ) {
			hTree = GetTreeCtrl().GetChildItem(hParentTree);
			while ( hTree ) {
				pShape = reinterpret_cast<CDXFshape *>(GetTreeCtrl().GetItemData(hTree));
				if ( pShape && pShape->IsKindOf(RUNTIME_CLASS(CDXFshape)) ) {
					if ( *dOffset == HUGE_VALF )
						dOffset = pShape->GetOffset();
					else if ( *dOffset != pShape->GetOffset() ) {
						dOffset = HUGE_VALF;	// �޲�۸ޏ����l����
						hTree = NULL;	// break
					}
				}
				if ( hTree )
					hTree = GetTreeCtrl().GetNextSiblingItem(hTree);
				else
					hParentTree = NULL;	// break
			}
			if ( hParentTree )
				hParentTree = GetTreeCtrl().GetNextSiblingItem(hParentTree);
		}
	}
	else {
		if ( IsRootTree(hSelectTree) )
			return;
		if ( pSelect->IsKindOf(RUNTIME_CLASS(CLayerData)) ) {
			pLayer = static_cast<CLayerData *>(pSelect);
			vSelect = pLayer;
			dOffset = HUGE_VALF;
			bChain  = TRUE;
			hParentTree = GetTreeCtrl().GetParentItem(hSelectTree);
			if ( hParentTree == m_hRootTree[0] ) {
				// �֊s�W���ɏ�������ڲ�
				bAcute = TRUE;
				nShape = 0;		// �����ޯ���̏����l
				hTree = GetTreeCtrl().GetChildItem(hSelectTree);
				while ( hTree ) {
					pShape = reinterpret_cast<CDXFshape *>(GetTreeCtrl().GetItemData(hTree));
					if ( pShape && pShape->IsKindOf(RUNTIME_CLASS(CDXFshape)) ) {
						if ( *dOffset == HUGE_VALF )
							dOffset = pShape->GetOffset();
						else if ( *dOffset != pShape->GetOffset() ) {
							dOffset = HUGE_VALF;
							break;
						}
					}
					hTree = GetTreeCtrl().GetNextSiblingItem(hTree);
				}
			}
			else {
				// �O�ՁE���O�W���ɏ�������ڲ�
				bAcute = FALSE;
				nShape = hParentTree == m_hRootTree[1] ? 1 : 2;	// �����ޯ���̏����l
				hTree = GetTreeCtrl().GetChildItem(hSelectTree);
				while ( hTree ) {
					pShape = reinterpret_cast<CDXFshape *>(GetTreeCtrl().GetItemData(hTree));
					if ( pShape && pShape->IsKindOf(RUNTIME_CLASS(CDXFshape)) &&
								pShape->GetShapeType()==DXFSHAPETYPE_CHAIN ) {
						if ( *dOffset == HUGE_VALF )
							dOffset = pShape->GetOffset();
						else if ( *dOffset != pShape->GetOffset() )
							dOffset = HUGE_VALF;
					}
					else {
						// �P�ł�CDXFchain�W���łȂ��Ȃ�
						dOffset.reset();	// �̾�ē��͕s��
						bChain = FALSE;		// �֊s�W���I��s��
						nShape--;
						break;
					}
					hTree = GetTreeCtrl().GetNextSiblingItem(hTree);
				}
			}
		}
		else if ( pSelect->IsKindOf(RUNTIME_CLASS(CDXFshape)) ) {
			// �eڲԂ��擾���Ă���
			hParentTree = GetTreeCtrl().GetParentItem(hSelectTree);
			pLayer = reinterpret_cast<CLayerData *>(GetTreeCtrl().GetItemData(hParentTree));
			// �`�������è�擾
			pShape = static_cast<CDXFshape *>(pSelect);
			vSelect = pShape;
			nShape = pShape->GetShapeAssemble();
			if ( pShape->GetShapeType() == DXFSHAPETYPE_CHAIN ) {
				bChain = TRUE;
				dOffset = pShape->GetOffset();
			}
			bAcute = pShape->GetAcuteRound();
		}
		else
			return;
	}

	CShapePropDlg	dlg(vSelect, bChain, nShape, dOffset, bAcute);
	if ( dlg.DoModal() != IDOK )
		return;

	// �e�`����̍X�V
	switch ( vSelect.which() ) {
	case DXFTREETYPE_MUSTER:
		if ( *dOffset==dlg.m_dOffset && bAcute==dlg.m_bAcuteRound )
			return;		// �X�V�K�v�Ȃ�
		// �����̗֊s�W����S�čX�V
		hParentTree = GetTreeCtrl().GetChildItem(hSelectTree);
		while ( hParentTree ) {
			hTree = GetTreeCtrl().GetChildItem(hParentTree);
			while ( hTree ) {
				pShape = reinterpret_cast<CDXFshape *>(GetTreeCtrl().GetItemData(hTree));
				if ( pShape && pShape->IsKindOf(RUNTIME_CLASS(CDXFshape)) ) {
					pShape->SetOffset(dlg.m_dOffset);
					pShape->SetAcuteRound(dlg.m_bAcuteRound);
				}
				hTree = GetTreeCtrl().GetNextSiblingItem(hTree);
			}
			hParentTree = GetTreeCtrl().GetNextSiblingItem(hParentTree);
		}
		bRecalc = TRUE;		// �Čv�Z
		break;
	case DXFTREETYPE_LAYER:
		ASSERT( pLayer );
		if ( *dOffset!=dlg.m_dOffset || bAcute!=dlg.m_bAcuteRound ) {
			hTree = GetTreeCtrl().GetChildItem(hSelectTree);
			while ( hTree ) {
				pShape = reinterpret_cast<CDXFshape *>(GetTreeCtrl().GetItemData(hTree));
				if ( pShape && pShape->IsKindOf(RUNTIME_CLASS(CDXFshape)) ) {
					pShape->SetOffset(dlg.m_dOffset);
					pShape->SetAcuteRound(dlg.m_bAcuteRound);
				}
				hTree = GetTreeCtrl().GetNextSiblingItem(hTree);
			}
			bRecalc = TRUE;
		}
		if ( nShape != dlg.m_nShape ) {
			nShape = dlg.m_nShape;
			// D&D�Ɠ�������
			m_hItemDrag  = hSelectTree;
			m_pDragLayer = pLayer;
			m_pDragShape = NULL;
			m_dwDragRoot = GetParentAssemble(hParentTree);
			if ( !bChain )
				nShape++;
			m_hItemDrop  = m_hRootTree[nShape];
			DragInsert();
		}
		break;
	case DXFTREETYPE_SHAPE:
		ASSERT( pLayer );
		ASSERT( pShape );
		if ( pShape->GetShapeName() != dlg.m_strShapeName ) {
			pShape->SetShapeName(dlg.m_strShapeName);
			GetDocument()->SetModifiedFlag();
		}
		if ( *dOffset != dlg.m_dOffset ) {
			pShape->SetOffset(dlg.m_dOffset);
			bRecalc = TRUE;
		}
		if ( bAcute != dlg.m_bAcuteRound ) {
			pShape->SetAcuteRound(dlg.m_bAcuteRound);
			bRecalc = TRUE;
		}
		if ( nShape != dlg.m_nShape ) {
			nShape = dlg.m_nShape;
			m_hItemDrag  = hSelectTree;
			m_pDragLayer = pLayer;
			m_pDragShape = pShape;
			m_dwDragRoot = (DWORD)(pShape->GetShapeAssemble()) + 1;
			if ( !bChain )
				nShape++;
			m_hItemDrop  = m_hRootTree[nShape];
			DragInsert();
		}
		break;
	}

	if ( !bRecalc )
		return;

	// ���H�w���̍Čv�Z
	CThreadDlg	dlgThread(ID_EDIT_SHAPE_AUTO, GetDocument(),
					(WPARAM)AUTORECALCWORKING, (LPARAM)&vSelect);
	if ( dlgThread.DoModal() != IDOK )
		return;

	// �ذ�ޭ��̍X�V
	switch ( vSelect.which() ) {
	case DXFTREETYPE_MUSTER:
		// ���ݓo�^����Ă�����H�w�����폜
		AutoWorkingDel();				// UpdateAllViews(NULL, UAV_DXFAUTODELWORKING);
		// �������H�w���o�^(�ƍĕ`��)
		GetDocument()->UpdateAllViews(NULL, UAV_DXFAUTOWORKING);	// AutoWorkingSet(TRUE);
		break;
	case DXFTREETYPE_LAYER:
		AutoWorkingDel(pLayer);
		AutoWorkingSet(TRUE, pLayer);
		Invalidate();	// �ĕ`��
		GetDocument()->UpdateAllViews(this, UAV_DXFAUTOWORKING);	// AutoWorkingSet()�Ă΂Ȃ�
		break;
	case DXFTREETYPE_SHAPE:
		AutoWorkingDel(NULL, pShape);
		AutoWorkingSet(TRUE, pLayer, pShape);
		Invalidate();
		GetDocument()->UpdateAllViews(this, UAV_DXFAUTOWORKING);
		break;
	}

	// �X�V�׸�
	GetDocument()->SetModifiedFlag();
}

void CDXFShapeView::OnUpdateEditShapeName(CCmdUI *pCmdUI)
{
	BOOL	bEnable = FALSE;
	HTREEITEM	hTree = GetTreeCtrl().GetSelectedItem();
	if ( hTree && !IsRootTree(hTree) ) {
		CObject* pParam = reinterpret_cast<CObject *>(GetTreeCtrl().GetItemData(hTree));
		if ( pParam && !pParam->IsKindOf(RUNTIME_CLASS(CLayerData)) )
			bEnable = TRUE;
	}
	pCmdUI->Enable(bEnable);
}

void CDXFShapeView::OnEditShapeName()
{
	HTREEITEM	hTree = GetTreeCtrl().GetSelectedItem();
	if ( hTree && !IsRootTree(hTree) ) {
		CObject* pParam = reinterpret_cast<CObject *>(GetTreeCtrl().GetItemData(hTree));
		if ( pParam && !pParam->IsKindOf(RUNTIME_CLASS(CLayerData)) )
			GetTreeCtrl().EditLabel(hTree);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CDXFShapeView ���b�Z�[�W �n���h��

int CDXFShapeView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if ( CTreeView::OnCreate(lpCreateStruct) < 0 )
		return -1;

	// �Ұ��ؽ�
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

	CObject*	pParam = reinterpret_cast<CObject *>(pTVDispInfo->item.lParam);
	if ( pParam && (pTVDispInfo->item.mask&TVIF_TEXT) && !IsRootTree(pTVDispInfo->item.hItem) ) {
		if ( pParam->IsKindOf(RUNTIME_CLASS(CLayerData)) ) {
			lstrcpy(pTVDispInfo->item.pszText, static_cast<CLayerData *>(pParam)->GetLayerName());
		}
		else if ( pParam->IsKindOf(RUNTIME_CLASS(CDXFshape)) ) {
			lstrcpy(pTVDispInfo->item.pszText, static_cast<CDXFshape *>(pParam)->GetShapeName());
		}
		else /* if ( pParam->IsKindOf(RUNTIME_CLASS(CDXFworking)) ) */ {
			lstrcpy(pTVDispInfo->item.pszText, static_cast<CDXFworking *>(pParam)->GetWorkingName());
		}
	}

	*pResult = 0;
}

void CDXFShapeView::OnBeginLabelEdit(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);

	CObject* pParam = reinterpret_cast<CObject *>(pTVDispInfo->item.lParam);
	// ٰĂ�ڲԖ��ʹ�ި�ďo���Ȃ�
	*pResult = IsRootTree(pTVDispInfo->item.hItem) ||
				!pParam ||
				pParam->IsKindOf(RUNTIME_CLASS(CLayerData)) ? 1 : 0;
}

void CDXFShapeView::OnEndLabelEdit(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);

	CObject* pParam = reinterpret_cast<CObject *>(pTVDispInfo->item.lParam);
	ASSERT( pParam );

	if ( pTVDispInfo->item.pszText && lstrlen(pTVDispInfo->item.pszText) > 0 ) {
		if ( pParam->IsKindOf(RUNTIME_CLASS(CDXFshape)) ) {
			CDXFshape* pShape = static_cast<CDXFshape *>(pParam);
			if ( pShape->GetShapeName() != pTVDispInfo->item.pszText ) {
				pShape->SetShapeName(pTVDispInfo->item.pszText);
				GetDocument()->SetModifiedFlag();
			}
		}
		else if ( pParam->IsKindOf(RUNTIME_CLASS(CDXFworking)) ) {
			CDXFworking* pWork = static_cast<CDXFworking *>(pParam);
			if ( pWork->GetWorkingName() != pTVDispInfo->item.pszText ) {
				pWork->SetWorkingName(pTVDispInfo->item.pszText);
				GetDocument()->SetModifiedFlag();
			}
		}
	}

	*pResult = 0;
}

void CDXFShapeView::OnKeyDown(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LPNMTVKEYDOWN pTVKeyDown = reinterpret_cast<LPNMTVKEYDOWN>(pNMHDR);

	switch ( pTVKeyDown->wVKey ) {
	case VK_TAB:
		static_cast<CDXFChild *>(GetParentFrame())->GetMainView()->SetFocus();
		break;
	case VK_DELETE:
		OnWorkingDel();
		break;
	case VK_ESCAPE:
		DragCancel(TRUE);
		break;
	case VK_F2:
		OnEditShapeName();
		break;
	}

	*pResult = 0;
}

void CDXFShapeView::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if ( nChar == VK_TAB )
		return;		// �ް��׽���Ăяo�����ް�߉�����

	CTreeView::OnChar(nChar, nRepCnt, nFlags);
}

void CDXFShapeView::OnSelChanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	
	CObject*	pObject[] = {
		reinterpret_cast<CObject *>(pNMTreeView->itemOld.lParam),
		reinterpret_cast<CObject *>(pNMTreeView->itemNew.lParam)
	};
	HTREEITEM	hItem[] = {
		pNMTreeView->itemOld.hItem,
		pNMTreeView->itemNew.hItem
	};
	DXFTREETYPE		vSelect[SIZEOF(pObject)];	// DXFView.h
	CDXFshape*		pShape;
	CDXFworking*	pWork;

	// �e��޼ު�Ă̑I���׸ލX�V
	for ( int i=0; i<SIZEOF(pObject); i++ ) {
		if ( pObject[i] && hItem[i] ) {
			if ( IsRootTree(hItem[i]) ) {
				vSelect[i] = reinterpret_cast<DWORD_PTR>(pObject[i]);
				SetShapeSwitch_SubordinateTree(hItem[i], i);
			}
			else if ( pObject[i]->IsKindOf(RUNTIME_CLASS(CLayerData)) ) {
				vSelect[i] = static_cast<CLayerData *>(pObject[i]);
				SetShapeSwitch_SubordinateTree(hItem[i], i);
			}
			else if ( pObject[i]->IsKindOf(RUNTIME_CLASS(CDXFshape)) ) {
				pShape = static_cast<CDXFshape *>(pObject[i]);
				vSelect[i] = pShape;
				pShape->SetShapeSwitch(i);
			}
			else if ( pObject[i]->IsKindOf(RUNTIME_CLASS(CDXFworking)) ) {
				pWork = static_cast<CDXFworking *>(pObject[i]);
				vSelect[i] = pWork;
				pWork->SetSelectFlag(i);
			}
			else
				vSelect[i] = (DWORD)-1;	// ���I��
		}
		else
			vSelect[i] = (DWORD)-1;
	}

	// �I��\���̍X�V ( To DXFView.cpp )
	GetDocument()->UpdateAllViews(this, UAV_DXFSHAPEUPDATE,
		reinterpret_cast<CObject *>(vSelect));

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

	// ��ׯ�ދ�������
	if ( !hTree || IsRootTree(hTree) )
		return;
	CObject*	pObject = reinterpret_cast<CObject *>(GetTreeCtrl().GetItemData(hTree));
	if ( !pObject || pObject->IsKindOf(RUNTIME_CLASS(CDXFworking)) )
		return;

	// ��ׯ��OK(CLayerData or CDXFshape)
	m_bDragging = TRUE;
	m_hItemDrag = hTree;
	// ��ׯ�ޏ����O�ɑI���т̕ύX�ʒm
	GetTreeCtrl().SelectItem(m_hItemDrag);	// OnSelChanged()
	// ��ׯ�ޏ���
	if ( pObject->IsKindOf(RUNTIME_CLASS(CLayerData)) ) {
		m_pDragLayer = static_cast<CLayerData *>(pObject);
		m_pDragShape = NULL;
		m_dwDragRoot = GetParentAssemble(m_hItemDrag);
	}
	else {
		hTree = GetTreeCtrl().GetParentItem(m_hItemDrag);
		m_pDragLayer = reinterpret_cast<CLayerData *>(GetTreeCtrl().GetItemData(hTree));
		m_pDragShape = static_cast<CDXFshape *>(pObject);
		m_dwDragRoot = static_cast<DWORD>(m_pDragShape->GetShapeAssemble()) + 1;
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
		// ��ۯ�ߐ������
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
				DragLink();		// ��������
			else
				DragInsert();	// �ړ�����
		}
		else
			DragCancel(TRUE);
	}
}

void CDXFShapeView::OnRButtonDown(UINT nFlags, CPoint point)
{
	// �ް��׽���Ă΂Ȃ��悤�ɂ��Ȃ��� OnRButtonUp() ���Ă΂�Ȃ�
//	CTreeView::OnRButtonDown(nFlags, point);
}

void CDXFShapeView::OnRButtonUp(UINT nFlags, CPoint point)
{
	if ( m_bDragging )
		DragCancel(TRUE);
	else {
		// Select or NULL
		GetTreeCtrl().SelectItem( GetTreeCtrl().HitTest(point) );
		// ��÷���ƭ��\��
		CTreeView::OnRButtonUp(nFlags, point);
	}
}
