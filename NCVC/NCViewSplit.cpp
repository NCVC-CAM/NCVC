// NCViewSplit.cpp: CNCViewSplit クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "NCdata.h"
#include "NCDoc.h"
#include "NCViewTab.h"
#include "NCViewSplit.h"
#include "NCView.h"
#include "NCViewXY.h"
#include "NCViewXZ.h"
#include "NCViewYZ.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

BEGIN_MESSAGE_MAP(CNCViewSplit, CSplitterWnd)
	ON_WM_DESTROY()
	ON_WM_LBUTTONDBLCLK()
	// ﾕｰｻﾞｲﾆｼｬﾙ処理
	ON_MESSAGE (WM_USERINITIALUPDATE, &CNCViewSplit::OnUserInitialUpdate)
	// ﾍﾟｰｼﾞ切替ｲﾍﾞﾝﾄ
	ON_MESSAGE (WM_USERACTIVATEPAGE, &CNCViewSplit::OnUserActivatePage)
	// 各ﾋﾞｭｰへのﾌｨｯﾄﾒｯｾｰｼﾞ
	ON_MESSAGE (WM_USERVIEWFITMSG, &CNCViewSplit::OnUserViewFitMsg)
	// 全てのﾍﾟｲﾝの図形ﾌｨｯﾄ
	ON_COMMAND(ID_NCVIEW_ALLFIT, &CNCViewSplit::OnAllFitCmd)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////

CNCViewSplit::CNCViewSplit()
{
#ifdef _DEBUG_FILEOPEN
	g_dbg.printf("CNCViewSplit::CNCViewSplit() Start");
#endif
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewSplit クラスのオーバライド関数

/////////////////////////////////////////////////////////////////////////////
// CNCViewSplit クラスのメンバ関数

void CNCViewSplit::DrawData(CNCdata* pData, BOOL bSelect, PFNNCDRAWPROC pfnDrawProc[])
{
	CDC		dc;
	for ( int i=0; i<SIZEOF(m_hDC); i++ ) {
		if ( dc.Attach(m_hDC[i]) ) {
			dc.SetROP2(R2_COPYPEN);
			(pData->*pfnDrawProc[i])(&dc, bSelect);
			dc.Detach();
		}
	}
}

void CNCViewSplit::AllPane_PostMessage(UINT_PTR nID, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int		i, j;

	if ( nID == NCDRAWVIEW_NUM ) {	// ４面-1
		for ( i=0; i<2; i++ ) {			// 行
			for ( j=0; j<2; j++ ) {		// 列
				GetPane(i, j)->PostMessage(msg, wParam, lParam);
			}
		}
		SetActivePane(0, 0);	// XYZ表示をｱｸﾃｨﾌﾞに
	}
	else {								// ４面-2
		GetPane(0, 1)->PostMessage(msg, wParam, lParam);	// XYZ
		CSplitterWnd* pWnd = static_cast<CSplitterWnd *>(GetPane(0, 0));
		for ( i=0; i<3; i++ )
			pWnd->GetPane(i, 0)->PostMessage(msg, wParam, lParam);	// YZ, XZ, XY
		SetActivePane(0, 1);	// XYZ表示をｱｸﾃｨﾌﾞに
	}
}

void CNCViewSplit::CalcPane(UINT_PTR nID, BOOL bInitial/*=FALSE*/)
{
	CString	strRegKey, strEntry;
	VERIFY(strRegKey.LoadString(IDS_REGKEY_NC));
	int		nCxEdge = ::GetSystemMetrics(SM_CXEDGE) * 2;	// 外側の縁含む
	int		nCyEdge = ::GetSystemMetrics(SM_CYEDGE) * 2;
	int		nCol, nRow, nRow2;
	CRect	rc;
	GetParent()->GetClientRect(rc);

	// ｽﾌﾟﾘｯﾀｳｨﾝﾄﾞｳの初期ｻｲｽﾞ設定
	if ( nID == NCDRAWVIEW_NUM ) {	// ４面-1
		nRow = (rc.Height() >> 1) - nCyEdge;
		nCol = (rc.Width()  >> 1) - nCxEdge;
		if ( bInitial ) {
			strEntry.Format(IDS_REG_NCV_VIEWPANESIZE, 1, 1);
			nRow = AfxGetApp()->GetProfileInt(strRegKey, strEntry, nRow);
			strEntry.Format(IDS_REG_NCV_VIEWPANESIZE, 1, 2);
			nCol = AfxGetApp()->GetProfileInt(strRegKey, strEntry, nCol);
		}
		SetRowInfo(0, nRow, 0);
		SetColumnInfo(0, nCol, 0);
	}
	else {							// ４面-2
		nRow = nRow2 = rc.Height() / 3 - nCyEdge;
		nCol = rc.Width() / 3 - nCxEdge;
		if ( bInitial ) {
			strEntry.Format(IDS_REG_NCV_VIEWPANESIZE, 2, 1);
			nCol = AfxGetApp()->GetProfileInt(strRegKey, strEntry, nCol);
			strEntry.Format(IDS_REG_NCV_VIEWPANESIZE, 2, 2);
			nRow = AfxGetApp()->GetProfileInt(strRegKey, strEntry, nRow);
			strEntry.Format(IDS_REG_NCV_VIEWPANESIZE, 2, 3);
			nRow2 = AfxGetApp()->GetProfileInt(strRegKey, strEntry, nRow2);
		}
		SetColumnInfo(0, nCol, 0);
		CSplitterWnd* pWnd = static_cast<CSplitterWnd *>(GetPane(0, 0));
		pWnd->SetRowInfo(0, nRow,  0);
		pWnd->SetRowInfo(1, nRow2, 0);
	}
	RecalcLayout();
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewSplit クラスのメッセージ ハンドラ

LRESULT CNCViewSplit::OnUserInitialUpdate(WPARAM wParam, LPARAM lParam)
{
	int		i, j;
	CClientDC*	pDC;

	// ｽﾌﾟﾘｯﾀｳｨﾝﾄﾞｳの初期ｻｲｽﾞ設定
	CalcPane(wParam, TRUE);
	// 各ﾍﾟｲﾝへ図形ﾌｨｯﾄﾒｯｾｰｼﾞの送信
	if ( wParam == NCDRAWVIEW_NUM ) {	// ４面-1
		// ﾃﾞﾊﾞｲｽｺﾝﾃｷｽﾄﾊﾝﾄﾞﾙを取得
		for ( i=0; i<2; i++ ) {				// 行
			for ( j=0; j<2; j++ ) {			// 列
				pDC = new CClientDC(GetPane(i, j));
				m_hDC[i*2+j] = pDC->GetSafeHdc();
				delete	pDC;
			}
		}
		if ( lParam ) {
			for ( i=0; i<2; i++ ) {
				for ( j=0; j<2; j++ ) {
					GetPane(i, j)->SendMessage(WM_USERVIEWFITMSG, 0, lParam);
				}
			}
			SetActivePane(0, 0);
		}
	}
	else {
		// ４面-2
		CSplitterWnd* pWnd = static_cast<CSplitterWnd *>(GetPane(0, 0));
		for ( i=0; i<3; i++ ) {
			pDC = new CClientDC(pWnd->GetPane(i, 0));
			m_hDC[i] = pDC->GetSafeHdc();
			delete	pDC;
		}
		pDC = new CClientDC(GetPane(0, 1));
		m_hDC[i] = pDC->GetSafeHdc();
		delete	pDC;
		if ( lParam ) {
			for ( i=0; i<3; i++ )
				pWnd->GetPane(i, 0)->SendMessage(WM_USERVIEWFITMSG, 0, lParam);
			GetPane(0, 1)->SendMessage(WM_USERVIEWFITMSG, 0, lParam);
			SetActivePane(0, 1);
		}
	}

	return 0;
}

LRESULT CNCViewSplit::OnUserActivatePage(WPARAM wParam, LPARAM)
{
	// 各ﾍﾟｲﾝへｱｸﾃｨﾌﾞﾒｯｾｰｼﾞの送信(常に拡大率更新)
	AllPane_PostMessage(wParam, WM_USERACTIVATEPAGE, NULL, TRUE);
	return 0;
}

void CNCViewSplit::OnDestroy() 
{
	int		i, nInfo, nMin;
	CString	strRegKey, strEntry;
	VERIFY(strRegKey.LoadString(IDS_REGKEY_NC));

	if ( GetParent()->IsKindOf(RUNTIME_CLASS(CNCViewTab)) ) {
		if ( GetRowCount() > 1 ) {
			// ４面-1
			GetRowInfo(0, nInfo, nMin);
			if ( nInfo > 0 ) {
				strEntry.Format(IDS_REG_NCV_VIEWPANESIZE, 1, 1);
				AfxGetApp()->WriteProfileInt(strRegKey, strEntry, nInfo);
			}
			GetColumnInfo(0, nInfo, nMin);
			if ( nInfo > 0 ) {
				strEntry.Format(IDS_REG_NCV_VIEWPANESIZE, 1, 2);
				AfxGetApp()->WriteProfileInt(strRegKey, strEntry, nInfo);
			}
		}
		else {
			// ４面-2
			GetColumnInfo(0, nInfo, nMin);
			if ( nInfo > 0 ) {
				strEntry.Format(IDS_REG_NCV_VIEWPANESIZE, 2, 1);
				AfxGetApp()->WriteProfileInt(strRegKey, strEntry, nInfo);
			}
		}
	}
	else {
		// ４面-2 の左ﾍﾟｲﾝ
		for ( i=0; i<GetRowCount()-1; i++ ) {
			GetRowInfo(i, nInfo, nMin);
			if ( nInfo > 0 ) {
				strEntry.Format(IDS_REG_NCV_VIEWPANESIZE, 2, i+2);
				AfxGetApp()->WriteProfileInt(strRegKey, strEntry, nInfo);
			}
		}
	}
}

void CNCViewSplit::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	// 各ﾍﾟｲﾝを初期状態に戻す
	CWnd*	pParent = GetParent();
	if ( pParent->IsKindOf(RUNTIME_CLASS(CNCViewTab)) )
		OnUserViewFitMsg( static_cast<CNCViewTab *>(pParent)->GetActivePage(), TRUE );
	else
		pParent->PostMessage(WM_LBUTTONDBLCLK);
}

void CNCViewSplit::OnAllFitCmd()
{
	// 各ﾍﾟｲﾝに ID_VIEW_FIT ｺﾏﾝﾄﾞﾒｯｾｰｼﾞを送信
	CWnd*	pParent = GetParent();
	// CNCViewSplit が２重になっている４面-2 の左ﾍﾟｲﾝは
	// 先に上位の CNCViewSplit がこのﾒｯｾｰｼﾞを捕まえるので
	// 必ず GetParent() は CNCViewTab を指すハズ
	if ( !pParent->IsKindOf(RUNTIME_CLASS(CNCViewTab)) ) {		// 保険ｺｰﾄﾞ
		// 上位 CNCViewSplitに ID_NCVIEW_ALLFIT ﾒｯｾｰｼﾞﾘﾌﾚｸﾄ
		pParent->PostMessage(WM_COMMAND, MAKEWPARAM(ID_NCVIEW_ALLFIT, 0));
	}
	else {
		AllPane_PostMessage(static_cast<CNCViewTab *>(pParent)->GetActivePage(),
			WM_COMMAND, MAKEWPARAM(ID_VIEW_FIT, 0));
	}
}

LRESULT CNCViewSplit::OnUserViewFitMsg(WPARAM wParam, LPARAM lParam)
{
	// ｽﾌﾟﾘｯﾀｳｨﾝﾄﾞｳの初期ｻｲｽﾞ設定
	CalcPane(wParam);
	AllPane_PostMessage(wParam, WM_USERVIEWFITMSG, 0, lParam);
	return 0;
}
