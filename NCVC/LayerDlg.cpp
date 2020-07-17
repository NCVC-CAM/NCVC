// CLayerDlg.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "DXFDoc.h"
#include "Layer.h"
#include "LayerDlg.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

static	LPCTSTR	g_szTreeTitle[] = {
	"�؍�", "���H�J�n", "�ړ�", "����"
};

BEGIN_MESSAGE_MAP(CLayerDlg, CDialog)
	//{{AFX_MSG_MAP(CLayerDlg)
	ON_NOTIFY(TVN_KEYDOWN, IDC_DXFVIEW_LAYER, OnLayerTreeKeydown)
	ON_NOTIFY(NM_CLICK, IDC_DXFVIEW_LAYER, OnLayerTreeClick)
	ON_NOTIFY(TVN_GETDISPINFO, IDC_DXFVIEW_LAYER, OnLayerTreeGetdispinfo)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USERSWITCHDOCUMENT, OnUserSwitchDocument)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLayerDlg �_�C�A���O

CLayerDlg::CLayerDlg() : CDialog(CLayerDlg::IDD, NULL)
{
	//{{AFX_DATA_INIT(CLayerDlg)
	//}}AFX_DATA_INIT
}

void CLayerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLayerDlg)
	DDX_Control(pDX, IDC_DXFVIEW_LAYER, m_ctLayerTree);
	DDX_Control(pDX, IDOK, m_ctOK);
	//}}AFX_DATA_MAP
}

void CLayerDlg::SetChildCheck(HTREEITEM hParent)
{
	// �e���ڂɍ��킹�Ďq���т�������Ԃ��ꊇ�ύX
	BOOL		bCheck = !m_ctLayerTree.GetCheck(hParent);
	HTREEITEM	hChild = m_ctLayerTree.GetNextItem(hParent, TVGN_CHILD);
	do {
		m_ctLayerTree.SetCheck(hChild, bCheck);
	} while ( hChild = m_ctLayerTree.GetNextItem(hChild, TVGN_NEXT) );
}

void CLayerDlg::SetParentCheck(HTREEITEM hTree)
{
	// ����������ق��q���т��q�̂Ȃ��e���т�
	HTREEITEM	hParent = m_ctLayerTree.GetParentItem(hTree);
	if ( hParent ) {
		// (�e�̂���)�q���тȂ�C
		// ����ɑ�����q���т��S��CheckOFF���ǂ���
		BOOL		bAllCheck = !m_ctLayerTree.GetCheck(hTree);
		HTREEITEM	hChild = m_ctLayerTree.GetNextItem(hParent, TVGN_CHILD);
		do {
			if ( hChild != hTree && m_ctLayerTree.GetCheck(hChild) ) {
				bAllCheck = TRUE;
				break;
			}
		} while ( hChild = m_ctLayerTree.GetNextItem(hChild, TVGN_NEXT) );
		m_ctLayerTree.SetCheck(hParent, bAllCheck);
	}
	else {
		// �q�̂Ȃ��e���тȂ�(������Ԃ𔽓]������)�����ł��Ȃ��悤�ɂ���
		m_ctLayerTree.SetCheck(hTree, !m_ctLayerTree.GetCheck(hTree));
	}
}

/////////////////////////////////////////////////////////////////////////////
// CLayerDlg ���b�Z�[�W �n���h��

BOOL CLayerDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	// Ҳ��ذ�̍쐬
	for ( int i=0; i<SIZEOF(g_szTreeTitle); i++ )
		m_hTree[i] = m_ctLayerTree.InsertItem(g_szTreeTitle[i]);
	// �������ɂ����̂ŏc�̊Ԋu���L����
	m_ctLayerTree.SetItemHeight(m_ctLayerTree.GetItemHeight()+4);
	// SetCheck() �������Ȃ��޸ނ̉����
	m_ctLayerTree.ModifyStyle( TVS_CHECKBOXES, 0 );
	m_ctLayerTree.ModifyStyle( 0, TVS_CHECKBOXES );
	// �ذ���۰ق̏�����
	OnUserSwitchDocument(NULL, NULL);
	// ����޳�ʒu�ǂݍ���
	CPoint	pt;
	if ( AfxGetNCVCApp()->GetDlgWindow(IDS_REGKEY_WINDOW_LAYERDLG, &pt) )
		SetWindowPos(NULL, pt.x, pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	return TRUE;  // �R���g���[���Ƀt�H�[�J�X��ݒ肵�Ȃ��Ƃ��A�߂�l�� TRUE �ƂȂ�܂�
	              // ��O: OCX �v���p�e�B �y�[�W�̖߂�l�� FALSE �ƂȂ�܂�
}

void CLayerDlg::OnOK() 
{
	// �޷�����ޭ��ւ̕ύX�ʒm
	// ���̲���Ă���������Ƃ��́C�K�� CDXFDoc ���w���Ă���
	CDXFDoc*	pDoc = (CDXFDoc *)(AfxGetNCVCMainWnd()->GetActiveFrame()->GetActiveDocument());
	CLayerData*	pLayer;
	HTREEITEM	hChild;

	for ( int i=0; i<SIZEOF(m_hTree); i++ ) {
		if ( m_ctLayerTree.ItemHasChildren(m_hTree[i]) ) {
			// �q�̂���ڲԍ��ڂ���������Ԃ𔽉f
			hChild = m_ctLayerTree.GetNextItem(m_hTree[i], TVGN_CHILD);
			do {
				pLayer = reinterpret_cast<CLayerData *>(m_ctLayerTree.GetItemData(hChild));
				ASSERT( pLayer );
				pLayer->m_bView = m_ctLayerTree.GetCheck(hChild);
			} while ( hChild = m_ctLayerTree.GetNextItem(hChild, TVGN_NEXT) );
		}
	}

	// �ĕ`��w��
	pDoc->UpdateAllViews(NULL);
	// �ذ���۰ق�̫����ړ�
	m_ctLayerTree.SetFocus();
}

void CLayerDlg::OnCancel() 
{
	// ����޳�ʒu�ۑ�
	AfxGetNCVCApp()->SaveDlgWindow(IDS_REGKEY_WINDOW_LAYERDLG, this);

	DestroyWindow();	// Ӱ��ڽ�޲�۸�
//	CDialog::OnCancel();
}

void CLayerDlg::PostNcDestroy() 
{
	AfxGetNCVCMainWnd()->SetModelessDlg(MLD_DXFLAYER, NULL);
	delete	this;
//	CDialog::PostNcDestroy();
}

LRESULT CLayerDlg::OnUserSwitchDocument(WPARAM, LPARAM)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CLayerDlg::OnUserSwitchDocument()\nCalling");
#endif
	int			i;
	HTREEITEM	hChild;

	// �q���т��폜
	for ( i=0; i<SIZEOF(m_hTree); i++ ) {
		m_ctLayerTree.SetCheck(m_hTree[i], FALSE);
		// �����܂��s���Ȃ�����...
//		m_ctLayerTree.Expand(m_hTree[i], TVE_COLLAPSERESET);
		while ( hChild = m_ctLayerTree.GetChildItem(m_hTree[i]) )
			m_ctLayerTree.DeleteItem(hChild);
	}
	// DXF�޷���ĂłȂ���ΏI��
	CMDIChildWnd*	pChild   = AfxGetNCVCMainWnd()->MDIGetActive();
	CDocument*		pDocTest = pChild ? pChild->GetActiveDocument() : NULL;
	if ( !pDocTest || !pDocTest->IsKindOf(RUNTIME_CLASS(CDXFDoc)) ) {
		EnableButton(FALSE);
		return 0;
	}

	// ڲ��ذ�ւ̓o�^�ƕ\���E��\��������
	EnableButton(TRUE);
	CDXFDoc*	pDoc = (CDXFDoc *)pDocTest;
	CLayerData*	pLayer;
	int			nType, nLoop = pDoc->GetLayerCnt();

	for ( i=0; i<nLoop; i++ ) {
		pLayer = pDoc->GetLayerData(i);
		nType = pLayer->GetLayerType();
#ifdef _DEBUG
		dbg.printf("Layer=\"%s\",%d, %d",
			pLayer->GetStrLayer(), nType, pLayer->m_bView);
#endif
		if ( nType>=DXFCAMLAYER && nType<=DXFCOMLAYER ) {
			hChild = m_ctLayerTree.InsertItem(TVIF_TEXT|TVIF_PARAM, LPSTR_TEXTCALLBACK,
							-1, -1, 0, 0, (LPARAM)pLayer,
							m_hTree[nType - DXFCAMLAYER], TVI_LAST);
			ASSERT( hChild );
			m_ctLayerTree.SetCheck(hChild, pLayer->m_bView);
		}
	}
	// �q������ڲԂɂ́C�e�ɂ�����
	BOOL	bAllCheck;
	for ( i=0; i<SIZEOF(m_hTree); i++ ) {
		bAllCheck = FALSE;
		if ( m_ctLayerTree.ItemHasChildren(m_hTree[i]) ) {
			m_ctLayerTree.SortChildren(m_hTree[i]);
			hChild = m_ctLayerTree.GetNextItem(m_hTree[i], TVGN_CHILD);
			do {
				if ( m_ctLayerTree.GetCheck(hChild) ) {
					bAllCheck = TRUE;
					break;
				}
			} while ( hChild = m_ctLayerTree.GetNextItem(hChild, TVGN_NEXT) );
		}
		m_ctLayerTree.SetCheck(m_hTree[i], bAllCheck);
	}
	m_ctLayerTree.Expand(m_hTree[0], TVE_EXPAND);

	return 0;
}

void CLayerDlg::OnLayerTreeKeydown(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LPNMTVKEYDOWN pTVKeyDown = reinterpret_cast<LPNMTVKEYDOWN>(pNMHDR);

	if ( pTVKeyDown->wVKey == VK_SPACE ) {
		HTREEITEM	hTree = m_ctLayerTree.GetSelectedItem();
		if ( hTree ) {
			if ( m_ctLayerTree.ItemHasChildren(hTree) )
				SetChildCheck(hTree);
			else
				SetParentCheck(hTree);
		}
	}

	*pResult = 0;
}

void CLayerDlg::OnLayerTreeClick(NMHDR* pNMHDR, LRESULT* pResult) 
{
	DWORD	dwPos = ::GetMessagePos();
	CPoint	pt( LOWORD(dwPos), HIWORD(dwPos) );
	m_ctLayerTree.ScreenToClient( &pt );

	UINT		uFlag;
	HTREEITEM	hTree = m_ctLayerTree.HitTest(pt, &uFlag);
	if ( hTree && uFlag == TVHT_ONITEMSTATEICON ) {
		if ( m_ctLayerTree.ItemHasChildren(hTree) )
			SetChildCheck(hTree);
		else
			SetParentCheck(hTree);
	}

	*pResult = 0;
}

void CLayerDlg::OnLayerTreeGetdispinfo(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);

	if ( pTVDispInfo->item.mask & TVIF_TEXT ) {
		CLayerData*	pLayer = reinterpret_cast<CLayerData *>(pTVDispInfo->item.lParam);
		ASSERT( pLayer );
		lstrcpy(pTVDispInfo->item.pszText, pLayer->GetStrLayer());
	}

	*pResult = 0;
}
