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
//	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDBLCLK()
	// ﾕｰｻﾞｲﾆｼｬﾙ処理
	ON_MESSAGE (WM_USERINITIALUPDATE, OnUserInitialUpdate)
	// ﾍﾟｰｼﾞ切替ｲﾍﾞﾝﾄ
	ON_MESSAGE (WM_USERACTIVATEPAGE, OnUserActivatePage)
	// 各ﾋﾞｭｰへのﾌｨｯﾄﾒｯｾｰｼﾞ
	ON_MESSAGE (WM_USERVIEWFITMSG, OnUserViewFitMsg)
	// 全てのﾍﾟｲﾝの図形ﾌｨｯﾄ
	ON_COMMAND(ID_NCVIEW_ALLFIT, OnAllFitCmd)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////

CNCViewSplit::CNCViewSplit()
{
}

CNCViewSplit::~CNCViewSplit()
{
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewSplit クラスのオーバライド関数

/////////////////////////////////////////////////////////////////////////////
// CNCViewSplit クラスのメンバ関数

void CNCViewSplit::DrawData(CNCdata* pData)
{
	CDC		dc;

	if ( dc.Attach(m_hDC[0]) ) {
		pData->Draw(&dc);
		dc.Detach();
	}
	if ( dc.Attach(m_hDC[1]) ) {
		pData->DrawYZ(&dc);
		dc.Detach();
	}
	if ( dc.Attach(m_hDC[2]) ) {
		pData->DrawXZ(&dc);
		dc.Detach();
	}
	if ( dc.Attach(m_hDC[3]) ) {
		pData->DrawXY(&dc);
		dc.Detach();
	}
}

void CNCViewSplit::AllPane_PostMessage(int nID, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int		i, j;

	if ( nID == NC_SINGLEPANE ) {	// ４面-1
		for ( i=0; i<GetRowCount(); i++ ) {			// 行
			for ( j=0; j<GetColumnCount(); j++ ) {	// 列
				GetPane(i, j)->PostMessage(msg, wParam, lParam);
			}
		}
		SetActivePane(0, 0);	// XYZ表示をｱｸﾃｨﾌﾞに
	}
	else {								// ４面-2
		GetPane(0, 1)->PostMessage(msg, wParam, lParam);	// XYZ
		CSplitterWnd* pWnd = (CSplitterWnd *)GetPane(0, 0);
		for ( i=0; i<pWnd->GetRowCount(); i++ ) {
			pWnd->GetPane(i, 0)->PostMessage(msg, wParam, lParam);	// YZ, XZ, XY
		}
		SetActivePane(0, 1);	// XYZ表示をｱｸﾃｨﾌﾞに
	}
}

void CNCViewSplit::CalcPane(int nID, BOOL bInitial/*=FALSE*/)
{
	CString	strRegKey, strEntry;
	VERIFY(strRegKey.LoadString(IDS_REGKEY_NC));
	int		nCxEdge = ::GetSystemMetrics(SM_CXEDGE) * 2;	// 外側の縁含む
	int		nCyEdge = ::GetSystemMetrics(SM_CYEDGE) * 2;
	int		nCol, nRow, nRow2;
	CRect	rc;
	GetParent()->GetClientRect(&rc);

	// ｽﾌﾟﾘｯﾀｳｨﾝﾄﾞｳの初期ｻｲｽﾞ設定
	if ( nID == NC_SINGLEPANE ) {	// ４面-1
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
		CSplitterWnd* pWnd = (CSplitterWnd *)GetPane(0, 0);
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
	if ( wParam == NC_SINGLEPANE ) {	// ４面-1
		for ( i=0; i<GetRowCount(); i++ ) {			// 行
			for ( j=0; j<GetColumnCount(); j++ ) {	// 列
				if ( (BOOL)lParam )
					GetPane(i, j)->SendMessage(WM_USERVIEWFITMSG, 0, lParam);
				// ﾃﾞﾊﾞｲｽｺﾝﾃｷｽﾄﾊﾝﾄﾞﾙを取得(XYZ, YZ, XZ, XY 順)
				pDC = new CClientDC(GetPane(i, j));
				m_hDC[j+i*2] = pDC->GetSafeHdc();
				delete	pDC;
			}
		}
		if ( (BOOL)lParam )
			SetActivePane(0, 0);	// XYZ表示をｱｸﾃｨﾌﾞに
	}
	else {								// ４面-2
		if ( (BOOL)lParam )
			GetPane(0, 1)->SendMessage(WM_USERVIEWFITMSG, 0, lParam);	// XYZ
		pDC = new CClientDC(GetPane(0, 1));
		m_hDC[0] = pDC->GetSafeHdc();
		delete	pDC;
		CSplitterWnd* pWnd = (CSplitterWnd *)GetPane(0, 0);
		for ( i=0; i<pWnd->GetRowCount(); i++ ) {
			if ( (BOOL)lParam )
				pWnd->GetPane(i, 0)->SendMessage(WM_USERVIEWFITMSG, 0, lParam);	// YZ, XZ, XY
			pDC = new CClientDC(pWnd->GetPane(i, 0));
			m_hDC[i+1] = pDC->GetSafeHdc();
			delete	pDC;
		}
		if ( (BOOL)lParam )
			SetActivePane(0, 1);	// XYZ表示をｱｸﾃｨﾌﾞに
	}

	return 0;
}

LRESULT CNCViewSplit::OnUserActivatePage(WPARAM wParam, LPARAM)
{
	// 各ﾍﾟｲﾝへｱｸﾃｨﾌﾞﾒｯｾｰｼﾞの送信
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
	CWnd*	pWnd = GetParent();
	if ( pWnd->IsKindOf(RUNTIME_CLASS(CNCViewTab)) )
		OnUserViewFitMsg( ((CNCViewTab *)pWnd)->GetActivePage(), TRUE );
	else
		pWnd->PostMessage(WM_LBUTTONDBLCLK);
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
		AllPane_PostMessage(((CNCViewTab *)pParent)->GetActivePage(),
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
/*
BOOL CNCViewSplit::OnEraseBkgnd(CDC* pDC) 
{
#ifdef _DEBUG
	CMagaDbg	dbg("CNCViewSplit::OnEraseBkgnd()\nStart");
#endif
	return TRUE;
}
*/
