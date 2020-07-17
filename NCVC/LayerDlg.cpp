// CLayerDlg.cpp : インプリメンテーション ファイル
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
	"切削", "加工開始", "移動", "ｺﾒﾝﾄ"
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
// CLayerDlg ダイアログ

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
	// 親項目に合わせて子ｱｲﾃﾑのﾁｪｯｸ状態を一括変更
	BOOL		bCheck = !m_ctLayerTree.GetCheck(hParent);
	HTREEITEM	hChild = m_ctLayerTree.GetNextItem(hParent, TVGN_CHILD);
	do {
		m_ctLayerTree.SetCheck(hChild, bCheck);
	} while ( hChild = m_ctLayerTree.GetNextItem(hChild, TVGN_NEXT) );
}

void CLayerDlg::SetParentCheck(HTREEITEM hTree)
{
	// 引数のﾊﾝﾄﾞﾙが子ｱｲﾃﾑか子のない親ｱｲﾃﾑか
	HTREEITEM	hParent = m_ctLayerTree.GetParentItem(hTree);
	if ( hParent ) {
		// (親のある)子ｱｲﾃﾑなら，
		// それに属する子ｱｲﾃﾑが全てCheckOFFかどうか
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
		// 子のない親ｱｲﾃﾑなら(ﾁｪｯｸ状態を反転させて)ﾁｪｯｸできないようにする
		m_ctLayerTree.SetCheck(hTree, !m_ctLayerTree.GetCheck(hTree));
	}
}

/////////////////////////////////////////////////////////////////////////////
// CLayerDlg メッセージ ハンドラ

BOOL CLayerDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	// ﾒｲﾝﾂﾘｰの作成
	for ( int i=0; i<SIZEOF(g_szTreeTitle); i++ )
		m_hTree[i] = m_ctLayerTree.InsertItem(g_szTreeTitle[i]);
	// 少し見にくいので縦の間隔を広げる
	m_ctLayerTree.SetItemHeight(m_ctLayerTree.GetItemHeight()+4);
	// SetCheck() が働かないﾊﾞｸﾞの回避策
	m_ctLayerTree.ModifyStyle( TVS_CHECKBOXES, 0 );
	m_ctLayerTree.ModifyStyle( 0, TVS_CHECKBOXES );
	// ﾂﾘｰｺﾝﾄﾛｰﾙの初期化
	OnUserSwitchDocument(NULL, NULL);
	// ｳｨﾝﾄﾞｳ位置読み込み
	CPoint	pt;
	if ( AfxGetNCVCApp()->GetDlgWindow(IDS_REGKEY_WINDOW_LAYERDLG, &pt) )
		SetWindowPos(NULL, pt.x, pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	return TRUE;  // コントロールにフォーカスを設定しないとき、戻り値は TRUE となります
	              // 例外: OCX プロパティ ページの戻り値は FALSE となります
}

void CLayerDlg::OnOK() 
{
	// ﾄﾞｷｭﾒﾝﾄﾋﾞｭｰへの変更通知
	// このｲﾍﾞﾝﾄが発生するときは，必ず CDXFDoc を指している
	CDXFDoc*	pDoc = (CDXFDoc *)(AfxGetNCVCMainWnd()->GetActiveFrame()->GetActiveDocument());
	CLayerData*	pLayer;
	HTREEITEM	hChild;

	for ( int i=0; i<SIZEOF(m_hTree); i++ ) {
		if ( m_ctLayerTree.ItemHasChildren(m_hTree[i]) ) {
			// 子のあるﾚｲﾔ項目だけﾁｪｯｸ状態を反映
			hChild = m_ctLayerTree.GetNextItem(m_hTree[i], TVGN_CHILD);
			do {
				pLayer = reinterpret_cast<CLayerData *>(m_ctLayerTree.GetItemData(hChild));
				ASSERT( pLayer );
				pLayer->m_bView = m_ctLayerTree.GetCheck(hChild);
			} while ( hChild = m_ctLayerTree.GetNextItem(hChild, TVGN_NEXT) );
		}
	}

	// 再描画指示
	pDoc->UpdateAllViews(NULL);
	// ﾂﾘｰｺﾝﾄﾛｰﾙへﾌｫｰｶｽ移動
	m_ctLayerTree.SetFocus();
}

void CLayerDlg::OnCancel() 
{
	// ｳｨﾝﾄﾞｳ位置保存
	AfxGetNCVCApp()->SaveDlgWindow(IDS_REGKEY_WINDOW_LAYERDLG, this);

	DestroyWindow();	// ﾓｰﾄﾞﾚｽﾀﾞｲｱﾛｸﾞ
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

	// 子ｱｲﾃﾑを削除
	for ( i=0; i<SIZEOF(m_hTree); i++ ) {
		m_ctLayerTree.SetCheck(m_hTree[i], FALSE);
		// ↓うまく行かなかった...
//		m_ctLayerTree.Expand(m_hTree[i], TVE_COLLAPSERESET);
		while ( hChild = m_ctLayerTree.GetChildItem(m_hTree[i]) )
			m_ctLayerTree.DeleteItem(hChild);
	}
	// DXFﾄﾞｷｭﾒﾝﾄでなければ終了
	CMDIChildWnd*	pChild   = AfxGetNCVCMainWnd()->MDIGetActive();
	CDocument*		pDocTest = pChild ? pChild->GetActiveDocument() : NULL;
	if ( !pDocTest || !pDocTest->IsKindOf(RUNTIME_CLASS(CDXFDoc)) ) {
		EnableButton(FALSE);
		return 0;
	}

	// ﾚｲﾔﾂﾘｰへの登録と表示・非表示のﾁｪｯｸ
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
	// 子を持つﾚｲﾔには，親にもﾁｪｯｸ
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
