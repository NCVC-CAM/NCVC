// NCListView.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "NCChild.h"
#include "NCdata.h"
#include "NCDoc.h"
#include "NCListView.h"
#include "NCJumpDlg.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

// �Ұ�ޕ\�����ޯ��
#define	LISTIMG_NORMAL		0
#define	LISTIMG_BREAK		1
#define	LISTIMG_ERROR		2
#define	LISTIMG_FOLDER		3

/////////////////////////////////////////////////////////////////////////////
// CNCListView

IMPLEMENT_DYNCREATE(CNCListView, CListView)

CNCListView::CNCListView()
{
}

CNCListView::~CNCListView()
{
}

BEGIN_MESSAGE_MAP(CNCListView, CListView)
	//{{AFX_MSG_MAP(CNCListView)
	ON_WM_CREATE()
	ON_WM_CONTEXTMENU()
	ON_NOTIFY_REFLECT(LVN_GETDISPINFO, OnGetDispInfo)
	ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, OnItemChanged)
	ON_UPDATE_COMMAND_UI(ID_NCVIEW_TRACE_BREAK, OnUpdateTraceBreak)
	ON_COMMAND(ID_NCVIEW_TRACE_BREAK, OnTraceBreak)
	ON_COMMAND(ID_NCVIEW_TRACE_BREAKOFF, OnTraceBreakOFF)
	ON_COMMAND(ID_NCVIEW_JUMP, OnViewJump)
	ON_UPDATE_COMMAND_UI(ID_NCVIEW_JUMP, OnUpdateViewJump)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNCListView �N���X�̃I�[�o���C�h�֐�

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
	// ؽ��ޭ����۰ق̍ő吔���
	GetListCtrl().SetItemCountEx( GetDocument()->GetNCBlockSize() );
	// �񕝐ݒ�
	GetListCtrl().SetColumnWidth(1, LVSCW_AUTOSIZE);
	// MDI�q�ڰт̽ð���ް������
	((CNCChild *)GetParentFrame())->OnUpdateStatusLineNo(
		0, GetDocument()->GetNCBlockSize(), NULL);
}

void CNCListView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	switch ( lHint ) {
	case UAV_STARTSTOPTRACE:	// �I������
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
	case UAV_DRAWWORKRECT:
	case UAV_DRAWMAXRECT:
		return;		// �ĕ`��s�v
	case UAV_FILEINSERT:	// ؽčēǍ�(���وʒu�ɓǂݍ���)
		// ؽ��ޭ����۰ق̍ő吔���
		GetListCtrl().SetItemCountEx( GetDocument()->GetNCBlockSize() );
		// �񕝐ݒ�
		GetListCtrl().SetColumnWidth(1, LVSCW_AUTOSIZE);
		break;
	case UAV_CHANGEFONT:	// ̫�Ă̕ύX
		GetListCtrl().SetFont(AfxGetNCVCMainWnd()->GetTextFont(TYPE_NCD), FALSE);
		break;
	}

	CListView::OnUpdate(pSender, lHint, pHint);
}

BOOL CNCListView::PreTranslateMessage(MSG* pMsg) 
{
	// ؽ��ޭ�����è�ނȂƂ��ͷ�ү���ނ𒼐�ؽĺ��۰قɑ���
	if ( pMsg->message == WM_KEYDOWN ) {
		// ���ް�ޱ���ڰ��܂ŕ߂܂��Ă��܂��̂ňړ����ނ��w��
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
			return TRUE;	// ؽı��шړ��̂��߂̖�󷰓����K�؂ɏ��������
		}
	}
	return CListView::PreTranslateMessage(pMsg);
}

BOOL CNCListView::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
	// �������� CDocument::OnCmdMsg() ���Ă΂Ȃ��悤�ɂ���
//	return CListView::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
	return CWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

/////////////////////////////////////////////////////////////////////////////
// CNCListView �N���X�̃����o�֐�

void CNCListView::SetJumpList(int nJump)
{
	int		nIndex;
	if ( nJump < 1 )
		nIndex = 0;
	else if ( nJump > GetListCtrl().GetItemCount() )
		nIndex = GetListCtrl().GetItemCount() - 1;
	else
		nIndex = nJump - 1;
	GetListCtrl().SetItemState(nIndex, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
	GetListCtrl().EnsureVisible(nIndex, FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// CNCListView �f�f

#ifdef _DEBUG
void CNCListView::AssertValid() const
{
	CListView::AssertValid();
}

void CNCListView::Dump(CDumpContext& dc) const
{
	CListView::Dump(dc);
}

CNCDoc* CNCListView::GetDocument() // ��f�o�b�O �o�[�W�����̓C�����C���ł��B
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CNCDoc)));
	return (CNCDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CNCListView �N���X�̃��b�Z�[�W �n���h���i���j���[�ҁj

void CNCListView::OnUpdateViewJump(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck( AfxGetNCVCMainWnd()->GetModelessDlg(MLD_NCJUMP) != NULL );
}

void CNCListView::OnViewJump() 
{
	if ( AfxGetNCVCMainWnd()->GetModelessDlg(MLD_NCJUMP) ) {
		// CNCJumpDlg::OnCancel() �̊ԐڌĂяo��
		AfxGetNCVCMainWnd()->GetModelessDlg(MLD_NCJUMP)->PostMessage(WM_CLOSE);
		return;
	}
	CNCJumpDlg*	pDlg = new CNCJumpDlg;
	pDlg->Create(IDD_NCVIEW_JUMP);
	AfxGetNCVCMainWnd()->SetModelessDlg(MLD_NCJUMP, pDlg);
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

/////////////////////////////////////////////////////////////////////////////
// CNCListView ���b�Z�[�W �n���h��

int CNCListView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CListView::OnCreate(lpCreateStruct) == -1)
		return -1;

	// �Ұ��ؽ�
	GetListCtrl().SetImageList(AfxGetNCVCMainWnd()->GetListImage(), LVSIL_SMALL);
	// ؽ��ޭ����۰�̫�Đݒ�
	GetListCtrl().SetFont(AfxGetNCVCMainWnd()->GetTextFont(TYPE_NCD), FALSE);
	// ��ǉ�
	GetListCtrl().InsertColumn(0, "Line", LVCFMT_LEFT);
	GetListCtrl().InsertColumn(1, "Code", LVCFMT_LEFT);
	GetListCtrl().SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
	GetListCtrl().SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);
	// �S�Ă̗��I���\�ɂ���
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
		// ��ڲ��\���D��
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

	if ( pNMListView->uNewState & LVIS_SELECTED ) {
		int	nItem = pNMListView->iItem;
		// �ð���ް�̍X�V
		((CNCChild *)GetParentFrame())->OnUpdateStatusLineNo(
			nItem + 1, GetDocument()->GetNCBlockSize(),
				nItem<0 || nItem>=GetDocument()->GetNCBlockSize() ?
					NULL : GetDocument()->GetNCblock(nItem) );
	}
	*pResult = 0;
}
