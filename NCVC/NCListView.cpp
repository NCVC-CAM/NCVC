// NCListView.cpp : �C���v�������e�[�V���� �t�@�C��
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

// �Ұ�ޕ\�����ޯ��
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

	int	nSize = (int)(GetDocument()->GetNCBlockSize());
	// ؽ��ޭ����۰ق̍ő吔���
	GetListCtrl().SetItemCountEx(nSize);
	// �񕝐ݒ�
	GetListCtrl().SetColumnWidth(1, LVSCW_AUTOSIZE);
	// �q�ڰт̽ð���ް������
	CNCChild*	pFrame = static_cast<CNCChild *>(GetParentFrame());
	pFrame->SetStatusMaxLine(nSize);
	pFrame->SetStatusInfo(0, (CNCdata *)NULL);
	pFrame->SendMessage(WM_USERSTATUSLINENO, (WPARAM)GetDocument());
}

void CNCListView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	switch ( lHint ) {
	case UAV_DRAWWORKRECT:
		break;		// ���čX�V�̉\��������̂ōĕ`��
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
	case UAV_DRAWMAXRECT:
		return;		// �ĕ`��s�v
	case UAV_FILEINSERT:	// ؽčēǍ�(���وʒu�ɓǂݍ���)
		{
			int	nSize = (int)(GetDocument()->GetNCBlockSize());
			// ؽ��ޭ����۰ق̍ő吔���
			GetListCtrl().SetItemCountEx(nSize);
			// �񕝐ݒ�
			GetListCtrl().SetColumnWidth(1, LVSCW_AUTOSIZE);
			// �q�ڰт̽ð���ް������
			static_cast<CNCChild *>(GetParentFrame())->SetStatusMaxLine(nSize);
		}
		GetListCtrl().Invalidate();
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
		// �ȉ��A�Ȃ�������ڰ��������Ȃ�
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
	// �������� CDocument::OnCmdMsg() ���Ă΂Ȃ��悤�ɂ���
//	return CListView::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
	return CWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

/////////////////////////////////////////////////////////////////////////////
// CNCListView �N���X�̃����o�֐�

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
	// ���݂̑I���s
	POSITION pos = GetListCtrl().GetFirstSelectedItemPosition();
	if ( pos )
		nIndex = GetListCtrl().GetNextSelectedItem(pos);
	if ( nUpDown == 0 ) {
		// �����
		if ( --nIndex < 0 )
			nIndex = GetDocument()->GetNCBlockSize() - 1;
		bReverse = TRUE;
	}
	else {
		// ������
		if ( ++nIndex >= GetDocument()->GetNCBlockSize() )
			nIndex = 0;
		bReverse = FALSE;
	}
	// ���̌���
	nIndex = GetDocument()->SearchBlockRegex(m_regFind, nIndex, bReverse);
	if ( nIndex < 0 )
		::MessageBeep(MB_ICONASTERISK);
	else {
		GetListCtrl().SetItemState((int)nIndex, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
		GetListCtrl().EnsureVisible((int)nIndex, FALSE);
	}
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
	return static_cast<CNCDoc *>(m_pDocument);
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

void CNCListView::OnUpdateViewFind(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck( AfxGetNCVCMainWnd()->GetModelessDlg(MLD_NCFIND) != NULL );
}

void CNCListView::OnViewFind() 
{
	if ( AfxGetNCVCMainWnd()->GetModelessDlg(MLD_NCFIND) ) {
		// CNCJumpDlg::OnCancel() �̊ԐڌĂяo��
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
// CNCListView ���b�Z�[�W �n���h��

int CNCListView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if ( CListView::OnCreate(lpCreateStruct) < 0 )
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

	// m_bTraceSelect ���^�̎��́ASelectTrace() ���� WM_USERSTATUSLINENO �Ăяo��
	if ( !m_bTraceSelect && (pNMListView->uNewState & LVIS_SELECTED) ) {
		int	nItem = pNMListView->iItem;
		// �ð���ް�̍X�V
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
	// �ʽگ��(CTraceThread)����̌Ăяo���̂��߁Aү���ޑ��M�ɂĕs��Ή�
	int	nIndex = pData->GetBlockLineNo();
	m_bTraceSelect = TRUE;	// OnItemChanged �𔭓����Ȃ�
	GetListCtrl().SetItemState(nIndex, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
	m_bTraceSelect = FALSE;
	// �ð���ް�X�V
	CNCChild*	pFrame = static_cast<CNCChild *>(GetParentFrame());
	pFrame->SetStatusInfo(nIndex+1, pData);
	pFrame->SendMessage(WM_USERSTATUSLINENO, (WPARAM)GetDocument());
	// ������۰ق̉\��������̂�Update()�ł�NG
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
