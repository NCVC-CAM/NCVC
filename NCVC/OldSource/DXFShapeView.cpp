// DXFShapeView.cpp : �C���v�������e�[�V���� �t�@�C��
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

// �Ұ�ޕ\�����ޯ��
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
	// �`����H�w��
	ON_UPDATE_COMMAND_UI_RANGE(ID_EDIT_SHAPE_SEL, ID_EDIT_SHAPE_POC, OnUpdateShapePattern)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDXFShapeView �N���X�̍\�z/����

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
	static	LPTSTR	ss_RootTitle[] = {
		"�֊s�W��", "�O�ՏW��", "���O�W��"
	};
	CTreeView::OnInitialUpdate();

	int		i;
	TVINSERTSTRUCT	tvInsert;
	::ZeroMemory(&tvInsert, sizeof(TVINSERTSTRUCT));
	tvInsert.hInsertAfter = TVI_LAST;
	tvInsert.item.mask = TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
	tvInsert.item.iImage = tvInsert.item.iSelectedImage = TREEIMG_ROOT;

	// ٰ��ذ
	ASSERT( SIZEOF(m_hRootTree) == SIZEOF(ss_RootTitle) );
	tvInsert.hParent = TVI_ROOT;
	for ( i=0; i<SIZEOF(ss_RootTitle); i++ ) {
		tvInsert.item.lParam = (LPARAM)(i+1);		// RootID
		tvInsert.item.pszText = ss_RootTitle[i];
		m_hRootTree[i] = GetTreeCtrl().InsertItem(&tvInsert);
		ASSERT( m_hRootTree[i] );
	}

	// �رى���̏���
	if ( GetDocument()->IsShape() ) {
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
		tvInsert.item.lParam = (LPARAM)(lpShape->pLayer);
		hLayerTree = GetTreeCtrl().InsertItem(&tvInsert);
		ASSERT( hLayerTree );
	}
	tvInsert.hParent = hLayerTree;
	tvInsert.item.iImage = tvInsert.item.iSelectedImage = TREEIMG_CHAIN + lpShape->pShape->GetShapeType();
	tvInsert.item.lParam = (LPARAM)(lpShape->pShape);
	hTree = GetTreeCtrl().InsertItem(&tvInsert);
	lpShape->pShape->SetTreeHandle(hTree);

	// ���H�w���̓o�^
	CDXFworkingList*	pList = lpShape->pShape->GetWorkList();
	tvInsert.hParent = hTree;
	tvInsert.item.iImage = tvInsert.item.iSelectedImage = TREEIMG_WORK;
	for ( POSITION pos=pList->GetHeadPosition(); pos; ) {
		tvInsert.item.lParam = (LPARAM)(pList->GetNext(pos));
		GetTreeCtrl().InsertItem(&tvInsert);
	}
	if ( bExpand )
		GetTreeCtrl().Expand(hTree, TVE_EXPAND);

	// �X�V���
	m_mpUpdateLayer.SetAt(lpShape->pLayer->GetStrLayer(), lpShape->pLayer);
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
		CObject*	pObject = (CObject *)(GetTreeCtrl().GetItemData(hTree));
		if ( !pObject || pObject->IsKindOf(RUNTIME_CLASS(CDXFworking)) )
			return FALSE;
		if ( m_pDragShape ) {
			// �`����̏ꍇ
			if ( pObject->IsKindOf(RUNTIME_CLASS(CLayerData)) ) {
				// ��ۯ�ߐ悪ڲԏ�����ׯ�ތ`�󂪏�������ڲԂƓ����ꍇ����
				if ( m_pDragLayer != (CLayerData *)pObject )
					return FALSE;
			}
			else {
				// ��ۯ�ߐ悪�`����Ȃ炻�̐e(ڲ�)���擾����
				CLayerData* pLayer = (CLayerData *)(GetTreeCtrl().GetItemData(GetTreeCtrl().GetParentItem(hTree)));
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
	// m_hItemDrop��m_hItemDrag��}��
	HTREEITEM hTree = m_pDragShape ? DragInsertShape() : DragInsertLayer();
	if ( hTree ) {
		// ��ׯ�ޑO�̏�Ԃ��擾
		TVITEM	tvItem;
		::ZeroMemory(&tvItem, sizeof(TVITEM));
		tvItem.mask  = TVIF_STATE;
		tvItem.hItem = m_hItemDrag;
		GetTreeCtrl().GetItem(&tvItem);
		// �ړ���őI���C��ԕ���
		GetTreeCtrl().SelectItem(hTree);
		if ( tvItem.state & TVIS_EXPANDED )
			GetTreeCtrl().Expand(hTree, TVE_EXPAND);
		// ��ׯ���ذ���폜(SelectItem()��łȂ��ƕ`��s��)
		hTree = GetTreeCtrl().GetParentItem(m_hItemDrag);
		GetTreeCtrl().DeleteItem(m_hItemDrag);
		// �ύX�L�^��ۑ�
		if ( m_pDragShape ) {
			m_mpUpdateLayer.SetAt(m_pDragLayer->GetStrLayer(), m_pDragLayer);
			// �`�������ׯ�ވړ��ŏ���ڲԔz���Ɍ`���񂪖����Ȃ��
			if ( !GetTreeCtrl().ItemHasChildren(hTree) )
				GetTreeCtrl().DeleteItem(hTree);	// ڲ��ذ���폜
		}
		else
			m_bUpdateLayerSequence = TRUE;
		// �޷���ĕύX�ʒm
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

	// �ŏI��ۯ�ߋ�������
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
		tvInsert.item.lParam = (LPARAM)m_pDragLayer;
		hLayerTree = GetTreeCtrl().InsertItem(&tvInsert);
	}
	ASSERT( hLayerTree );

	// �`����Ɖ��H�w���̓o�^
	tvInsert.hInsertAfter = TVI_LAST;
	hShapeTree = GetTreeCtrl().GetChildItem(m_hItemDrag);
	while ( hShapeTree ) {
		// �`����
		pShape = (CDXFshape *)(GetTreeCtrl().GetItemData(hShapeTree));
		tvInsert.item.iImage = tvInsert.item.iSelectedImage = 
			TREEIMG_CHAIN + pShape->GetShapeType();
		tvInsert.hParent = hLayerTree;
		tvInsert.item.lParam = (LPARAM)pShape;
		tvInsert.hParent = GetTreeCtrl().InsertItem(&tvInsert);	// ���H�w���̐e�ذ
		ASSERT( tvInsert.hParent );
		pShape->SetTreeHandle(tvInsert.hParent);
		// �����W���̍X�V
		if ( m_dwDragRoot != dwRoot )
			pShape->SetShapeAssemble((DXFSHAPE_ASSEMBLE)(dwRoot-1));
		// ���H�w��
		pList = pShape->GetWorkList();
		tvInsert.item.iImage = tvInsert.item.iSelectedImage = TREEIMG_WORK;
		for ( pos=pList->GetHeadPosition(); pos; ) {
			tvInsert.item.lParam = (LPARAM)(pList->GetNext(pos));
			GetTreeCtrl().InsertItem(&tvInsert);
		}
		// �ړ��O�̏�Ԃ𕜌�
		tvItem.hItem = hShapeTree;
		GetTreeCtrl().GetItem(&tvItem);
		if ( tvItem.state & TVIS_EXPANDED )
			GetTreeCtrl().Expand(tvInsert.hParent, TVE_EXPAND);
		// ���̌`����
		hShapeTree = GetTreeCtrl().GetNextSiblingItem(hShapeTree);
	}

	return hLayerTree;
}

HTREEITEM CDXFShapeView::DragInsertShape(void)
{
	DWORD		dwRoot = GetParentAssemble(m_hItemDrop);
	HTREEITEM	hTree;
	CDXFworkingList*	pList;

	// �ŏI��ۯ�ߋ�������
	if ( m_pDragShape->GetShapeFlag() & DXFMAPFLG_CANNOTAUTOWORKING && dwRoot == ROOTTREE_SHAPE )
		return NULL;

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

	// hTree�ɂ�ڲԁCm_hItemDrop�ɂ͌`����ذ�����(�܂���NULL)�������Ă���
	tvInsert.hParent = hTree;
	hTree = GetTreeCtrl().GetPrevSiblingItem(m_hItemDrop);
	tvInsert.hInsertAfter = hTree ? hTree : TVI_FIRST;	// ��ۯ�ߐ�
	tvInsert.item.iImage = tvInsert.item.iSelectedImage = 
		TREEIMG_CHAIN + m_pDragShape->GetShapeType();
	tvInsert.item.lParam = (LPARAM)m_pDragShape;
	hTree = GetTreeCtrl().InsertItem(&tvInsert);
	ASSERT( hTree );
	m_pDragShape->SetTreeHandle(hTree);
	// �����W���̍X�V
	if ( m_dwDragRoot != dwRoot )
		m_pDragShape->SetShapeAssemble((DXFSHAPE_ASSEMBLE)(dwRoot-1));

	// ���H�w���̓o�^
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
	// �ŏI������������
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
	HTREEITEM hTree = GetTreeCtrl().GetChildItem(m_hItemDrop), hTreeTmp;
	while ( hTree ) {
		hTreeTmp = hTree;
		hTree = GetTreeCtrl().GetNextSiblingItem(hTree);
		GetTreeCtrl().DeleteItem(hTreeTmp);
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
		// ���H�w���̍ēo�^
		CDXFworkingList* pList = pShape->GetWorkList();
		for ( POSITION pos=pList->GetHeadPosition(); pos; ) {
			hTree = GetTreeCtrl().InsertItem(TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM,
				LPSTR_TEXTCALLBACK, TREEIMG_WORK, TREEIMG_WORK, 0, 0,
				(LPARAM)(pList->GetNext(pos)), m_hItemDrop, TVI_LAST);
			ASSERT( hTree );
		}
		// �����汲�т̑I��
		GetTreeCtrl().SelectItem(m_hItemDrop);
	}
	// ��޼ު�đI��
	pShape->SetShapeSwitch(TRUE);
	// �޷���ĕύX�ʒm
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
	GetTreeCtrl().SelectItem(hParentTree);	// �`��W����I��
	GetTreeCtrl().EnsureVisible(hTree);
}

void CDXFShapeView::AutoWorkingDel(void)
{
	// ���ݓo�^����Ă���֊s�E�߹�ĉ��H���ذ������
	int		i, j;
	HTREEITEM		hLayerTree, hShapeTree, hTree;
	CDXFshape*		pShape;
	CDXFworking*	pPara;
	CPtrArray		obDel;
	obDel.SetSize(0, 64);

	for ( i=0; i<SIZEOF(m_hRootTree); i++ ) {
		hLayerTree = GetTreeCtrl().GetChildItem(m_hRootTree[i]);
		// ڲ�ٰ��
		while ( hLayerTree ) {
			hShapeTree = GetTreeCtrl().GetChildItem(hLayerTree);
			// �`��ٰ��
			while ( hShapeTree ) {
				pShape = (CDXFshape *)(GetTreeCtrl().GetItemData(hShapeTree));
				ASSERT( pShape );
				hTree = GetTreeCtrl().GetChildItem(hShapeTree);
				// ���݂̗֊s�E�߹�ĉ��H�w�����폜����ٰ��
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
	// �`���񂩂���H�w����o�^
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

	// ڲԏ����ύX����Ă����
	if ( m_bUpdateLayerSequence ) {
		for ( i=0; i<SIZEOF(m_hRootTree); i++ ) {
			// �q�����ŏ��̏W����
			if ( GetTreeCtrl().ItemHasChildren(m_hRootTree[i]) ) {
				int	ii=0;
				// ڲԏ����L�^
				hLayerTree = GetTreeCtrl().GetChildItem(m_hRootTree[i]);
				while ( hLayerTree ) {
					pLayer = (CLayerData *)(GetTreeCtrl().GetItemData(hLayerTree));
					pLayer->SetListNo(ii++);
					hLayerTree = GetTreeCtrl().GetNextSiblingItem(hLayerTree);
				}
				// ڲԏ����X�V
				GetDocument()->UpdateLayerSequence();
				m_bUpdateLayerSequence = FALSE;
				break;
			}
		}
	}

	// �`�󏇂��ύX����Ă����
	if ( !m_mpUpdateLayer.IsEmpty() ) {
		CShapeArray	obArray;
		obArray.SetSize(0, GetTreeCtrl().GetCount());
		CString		strLayer;
		// �S�Ă̏W������Y��ڲԔz���̌`�󏇂��擾
		for ( POSITION pos=m_mpUpdateLayer.GetStartPosition(); pos; ) {
			m_mpUpdateLayer.GetNextAssoc(pos, strLayer, pLayer);
			for ( i=0; i<SIZEOF(m_hRootTree); i++ ) {
				hLayerTree = SearchLayerTree( GetTreeCtrl().GetChildItem(m_hRootTree[i]), pLayer );
				if ( hLayerTree ) {
					// �q�ذ(�`��)���擾
					hTree = GetTreeCtrl().GetChildItem(hLayerTree);
					while ( hTree ) {
						obArray.Add( (CDXFshape *)(GetTreeCtrl().GetItemData(hTree)) );
						hTree = GetTreeCtrl().GetNextSiblingItem(hTree);
					}
				}
			}
			// �`�󏇂��X�V
			ASSERT( pLayer->GetShapeSize() == obArray.GetSize() );
			pLayer->CopyShape(obArray);
			obArray.RemoveAll();
		}
		m_mpUpdateLayer.RemoveAll();
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
	return (CDXFDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CDXFShapeView �N���X�̃��b�Z�[�W �n���h���i���j���[�ҁj

void CDXFShapeView::OnSortShape() 
{
	CLayerData*	pLayer;
	HTREEITEM	hLayerTree;

	for ( int i=0; i<SIZEOF(m_hRootTree); i++ ) {
		// ڲԏ��̕��בւ�
		GetTreeCtrl().SortChildren(m_hRootTree[i]);
		m_bUpdateLayerSequence = TRUE;
		// �`�󏇂̕��בւ�
		hLayerTree = GetTreeCtrl().GetChildItem(m_hRootTree[i]);
		while ( hLayerTree ) {
			GetTreeCtrl().SortChildren(hLayerTree);
			// �����X�V����
			if ( i == 0 ) {
				pLayer = (CLayerData *)(GetTreeCtrl().GetItemData(hLayerTree));
				m_mpUpdateLayer.SetAt(pLayer->GetStrLayer(), pLayer);
			}
			hLayerTree = GetTreeCtrl().GetNextSiblingItem(hLayerTree);
		}
	}

	// �޷���ĕύX�ʒm
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
		// �֊s�C�߹�ĉ��H�̏ꍇ�́C�����
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
			// ���я���
			HTREEITEM hParentTree = GetTreeCtrl().GetParentItem(hTree);
			CDXFshape* pShape = (CDXFshape *)(GetTreeCtrl().GetItemData(hParentTree));
			GetTreeCtrl().DeleteItem(hTree);
			pShape->DelWorkingData(pPara);
			// �޷���ĕύX�ʒm
			GetDocument()->SetModifiedFlag();
			// �ޭ��̍ĕ`��
			GetDocument()->UpdateAllViews(this);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// CDXFShapeView ���b�Z�[�W �n���h��

int CDXFShapeView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CTreeView::OnCreate(lpCreateStruct) == -1)
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
	// ٰĂ�ڲԖ��ʹ�ި�ďo���Ȃ�
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

	// �e��޼ު�Ă̑I���׸ލX�V
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
				vSelect[i] = (DWORD)-1;	// ���I��
		}
		else
			vSelect[i] = (DWORD)-1;
	}

	// �I��\���̍X�V ( To DXFView.cpp )
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

	// ��ׯ�ދ�������
	if ( !hTree || IsRootTree(hTree) )
		return;
	CObject*	pObject = (CObject *)(GetTreeCtrl().GetItemData(hTree));
	if ( !pObject || pObject->IsKindOf(RUNTIME_CLASS(CDXFworking)) )
		return;

	// ��ׯ��OK(CLayerData or CDXFshape)
	m_bDragging = TRUE;
	m_hItemDrag = hTree;
	// ��ׯ�ޏ����O�ɑI���т̕ύX�ʒm
	GetTreeCtrl().SelectItem(m_hItemDrag);	// OnSelChanged()
	// ��ׯ�ޏ���
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
	CTreeView::OnLButtonUp(nFlags, point);
}

void CDXFShapeView::OnNMRclick(NMHDR *pNMHDR, LRESULT *pResult)
{
#ifdef _DEBUG
	CMagaDbg	dbg("OnNMRclick()\nStart");
#endif
	// OnRButtonUp���Ă΂�Ȃ�(??)�COnRButtonDown�ł͔����������C�̂ł��̑���
	if ( m_bDragging )
		DragCancel(TRUE);
	else
		OnContextMenu(this, CPoint(::GetMessagePos()));
	*pResult = 0;
}
