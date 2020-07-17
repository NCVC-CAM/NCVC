// NCViewXY.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "NCChild.h"
#include "NCdata.h"
#include "NCDoc.h"
#include "NCViewTab.h"
#include "NCViewXY.h"
#include "ViewOption.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNCViewXY

IMPLEMENT_DYNCREATE(CNCViewXY, CView)

BEGIN_MESSAGE_MAP(CNCViewXY, CView)
	//{{AFX_MSG_MAP(CNCViewXY)
	ON_WM_CREATE()
	ON_WM_CONTEXTMENU()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_ERASEBKGND()
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	//}}AFX_MSG_MAP
	// ﾍﾟｰｼﾞ切替ｲﾍﾞﾝﾄ
	ON_MESSAGE (WM_USERACTIVATEPAGE, OnUserActivatePage)
	// 各ﾋﾞｭｰへのﾌｨｯﾄﾒｯｾｰｼﾞ
	ON_MESSAGE (WM_USERVIEWFITMSG, OnUserViewFitMsg)
	// ﾒﾆｭｰｺﾏﾝﾄﾞ
	ON_COMMAND_RANGE(ID_VIEW_UP, ID_VIEW_RT, OnMoveKey)
	ON_COMMAND_RANGE(ID_VIEW_BEFORE, ID_VIEW_LENSN, OnLensKey)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNCViewXY クラスの構築/消滅

CNCViewXY::CNCViewXY()
{
}

CNCViewXY::~CNCViewXY()
{
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewXY クラスのオーバライド関数

BOOL CNCViewXY::PreCreateWindow(CREATESTRUCT& cs) 
{
	// ｽﾌﾟﾘｯﾀからの境界ｶｰｿﾙが移るので，IDC_ARROW を明示的に指定
	cs.lpszClass = AfxRegisterWndClass(
			CS_HREDRAW|CS_VREDRAW|CS_OWNDC|CS_DBLCLKS,
			AfxGetApp()->LoadStandardCursor(IDC_ARROW) );
	return CView::PreCreateWindow(cs);
}

void CNCViewXY::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	switch ( lHint ) {
	case UAV_TRACECURSOR:
		Invalidate();
		UpdateWindow();		// 即再描画
		return;
	case UAV_FILEINSERT:
		OnLensKey(ID_VIEW_FIT);
		UpdateWindow();
		return;
	case UAV_DRAWWORKRECT:
		{
			CClientDC	dc(this);
			// 前回の描画を消去
			if ( GetDocument()->IsWorkRect() ) {
				dc.SetROP2(R2_XORPEN);
				DrawWorkRect(&dc);	// CNCViewBase
				dc.SetROP2(R2_COPYPEN);
			}
			// 描画用にﾃﾞｰﾀ更新
			if ( pHint ) {
				DrawConvertWorkRect();
				dc.SetROP2(R2_COPYPEN);
				DrawWorkRect(&dc);
			}
		}
		return;
	case UAV_DRAWMAXRECT:
		{
			CClientDC	dc(this);
			dc.SetROP2(GetDocument()->IsMaxRect() ? R2_COPYPEN : R2_XORPEN);
			DrawMaxRect(&dc);
			dc.SetROP2(R2_COPYPEN);
		}
		return;
	case UAV_CHANGEFONT:
		SetGuideData();
		break;
	case UAV_ADDINREDRAW:
		OnViewLensComm();
		return;
	}
	CView::OnUpdate(pSender, lHint, pHint);
}

BOOL CNCViewXY::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
	// ここから CDocument::OnCmdMsg() を呼ばないようにする
//	return CView::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
	return CWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewXY クラスのメンバ関数

void CNCViewXY::SetGuideData(void)
{
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	double	dSrc = pOpt->IsGuideSync() ? m_dFactor*LOMETRICFACTOR : LOMETRICFACTOR;
	// Ｘ軸のガイド初期化（左から右へ）
	m_ptGuid[NCA_X][0].x = (int)(-pOpt->GetGuideLength(NCA_X) * dSrc);
	m_ptGuid[NCA_X][0].y = 0;
	m_ptGuid[NCA_X][1].x = (int)( pOpt->GetGuideLength(NCA_X) * dSrc);
	m_ptGuid[NCA_X][1].y = 0;
	// Ｙ軸のガイド初期化（奥から手前へ）
	m_ptGuid[NCA_Y][0].x = 0;
	m_ptGuid[NCA_Y][0].y = (int)( pOpt->GetGuideLength(NCA_Y) * dSrc);
	m_ptGuid[NCA_Y][1].x = 0;
	m_ptGuid[NCA_Y][1].y = (int)(-pOpt->GetGuideLength(NCA_Y) * dSrc);
}

void CNCViewXY::DrawConvertWorkRect(void)
{
	CRectD	rc(GetDocument()->GetWorkRect());
	m_rcDrawWork.left	= DrawConvert(rc.left);
	m_rcDrawWork.top	= DrawConvert(rc.top);
	m_rcDrawWork.right	= DrawConvert(rc.right);
	m_rcDrawWork.bottom	= DrawConvert(rc.bottom);
}

void CNCViewXY::DrawConvertMaxRect(void)
{
	CRectD	rc(GetDocument()->GetMaxRect());
	m_rcDrawMax.left	= DrawConvert(rc.left);
	m_rcDrawMax.top		= DrawConvert(rc.top);
	m_rcDrawMax.right	= DrawConvert(rc.right);
	m_rcDrawMax.bottom	= DrawConvert(rc.bottom);
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewXY 描画

void CNCViewXY::OnDraw(CDC* pDC)
{
	extern	LPCTSTR	g_szNdelimiter;	// "XYZRIJKPLDH" from NCDoc.cpp

	ASSERT_VALID(GetDocument());
	int		i;
	pDC->SetROP2(R2_COPYPEN);
	CPen* pOldPen = (CPen *)pDC->SelectStockObject(NULL_PEN);

	// 平面案内
	if ( pDC->m_hAttribDC ) {
		CPoint	pt(0, 0);
		pDC->DPtoLP(&pt);
		pDC->SetTextAlign(TA_LEFT|TA_TOP);
		pDC->SetTextColor(AfxGetNCVCApp()->GetViewOption()->GetNcDrawColor(NCCOL_PANE));
		pDC->TextOut(pt.x, pt.y, g_szNdelimiter, 2);	// XY
	}
	// ｶﾞｲﾄﾞ表示
	pDC->SetTextAlign(TA_CENTER|TA_BOTTOM);
	for ( i=0; i<SIZEOF(m_ptGuid); i++ ) {
		pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenOrg(i));
		pDC->MoveTo(m_ptGuid[i][0]);
		pDC->LineTo(m_ptGuid[i][1]);
		pDC->SetTextColor(AfxGetNCVCMainWnd()->GetOrgColor(i));
		pDC->TextOut(m_ptGuid[i][0].x, m_ptGuid[i][0].y, g_szNdelimiter+i, 1);
	}
	// NCﾃﾞｰﾀ描画
	CNCdata*	pData;
	int	nDraw = GetDocument()->GetTraceDraw();	// ｸﾘﾃｨｶﾙｾｸｼｮﾝによるﾛｯｸ
	for ( i=GetDocument()->GetTraceStart(); i<nDraw; i++ ) {
		pData = GetDocument()->GetNCdata(i);
		if ( pData->GetGtype() == G_TYPE )
			pData->DrawXY(pDC, FALSE);
	}
	// 最大切削矩形
	if ( GetDocument()->IsMaxRect() )
		DrawMaxRect(pDC);	// CNCViewBase
	// ﾜｰｸ矩形
	if ( GetDocument()->IsWorkRect() )
		DrawWorkRect(pDC);	// CNCViewBase
	// 現在の拡大矩形を描画(ViewBase.cpp)
	if ( m_bMagRect )
		DrawMagnifyRect(pDC);

	pDC->SelectObject(pOldPen);
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewXY 診断

#ifdef _DEBUG
void CNCViewXY::AssertValid() const
{
	CView::AssertValid();
}

void CNCViewXY::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CNCDoc* CNCViewXY::GetDocument() // 非デバッグ バージョンはインラインです。
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CNCDoc)));
	return (CNCDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CNCViewXY クラスのメッセージ ハンドラ（メニュー編）

void CNCViewXY::OnUpdateEditCopy(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(TRUE);
}

void CNCViewXY::OnEditCopy() 
{
	CWaitCursor		wait;	// 砂時計ｶｰｿﾙ

	CClientDC		dc(this);
	CMetaFileDC		metaDC;
	metaDC.CreateEnhanced(&dc, NULL, NULL, NULL);
	metaDC.SetMapMode(MM_LOMETRIC);
	metaDC.SetBkMode(TRANSPARENT);
	metaDC.SelectStockObject(NULL_BRUSH);
	AfxGetNCVCMainWnd()->SelectGDI(FALSE);	// GDIｵﾌﾞｼﾞｪｸﾄの切替
	OnDraw(&metaDC);
	AfxGetNCVCMainWnd()->SelectGDI();

	// ｸﾘｯﾌﾟﾎﾞｰﾄﾞへのﾃﾞｰﾀｺﾋﾟｰ ViewBase.cpp
	CopyNCDrawForClipboard(metaDC.CloseEnhanced());
}

void CNCViewXY::OnMoveKey(UINT nID)
{
	CViewBase::OnMoveKey(nID);
}

void CNCViewXY::OnLensKey(UINT nID)
{
	switch ( nID ) {
	case ID_VIEW_BEFORE:
		CViewBase::OnBeforeMagnify();
		break;
	case ID_VIEW_FIT:
		CViewBase::OnViewFit(GetDocument()->GetMaxRect());
		break;
	case ID_VIEW_LENSP:
		CViewBase::OnViewLensP();
		break;
	case ID_VIEW_LENSN:
		CViewBase::OnViewLensN();
		break;
	default:
		return;
	}
	OnViewLensComm();
}

void CNCViewXY::OnViewLensComm(void)
{
	// 拡大率による描画情報の更新
	GetDocument()->AllChangeFactorXY(m_dFactor);
	// ｶﾞｲﾄﾞ軸
	if ( AfxGetNCVCApp()->GetViewOption()->IsGuideSync() )
		SetGuideData();
	// 各矩形情報の更新
	if ( GetDocument()->IsWorkRect() )
		DrawConvertWorkRect();
	DrawConvertMaxRect();
	// MDI子ﾌﾚｰﾑのｽﾃｰﾀｽﾊﾞｰに情報表示
	static_cast<CNCChild *>(GetParentFrame())->SetFactorInfo(NC_XY_PLANE, m_dFactor);
	// ﾋﾞｭｰの再描画
	Invalidate();
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewXY メッセージ ハンドラ

int CNCViewXY::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	// ﾏｯﾋﾟﾝｸﾞﾓｰﾄﾞの変更など
	CViewBase::OnCreate(this);
	// ｶﾞｲﾄﾞﾃﾞｰﾀ
	SetGuideData();

	return 0;
}

LRESULT CNCViewXY::OnUserActivatePage(WPARAM, LPARAM lParam)
{
	if ( m_dFactor == 0.0 ) {
		// OnUserInitialUpdate() が実行されていないとき
		OnLensKey(ID_VIEW_FIT);
	}
	else {
		if ( (BOOL)lParam ) {
			// 拡大率の再更新
			OnViewLensComm();
		}
	}
	return 0;
}

LRESULT CNCViewXY::OnUserViewFitMsg(WPARAM, LPARAM lParam)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CNCViewXY::OnUserViewFitMsg()\nStart");
#endif
	// 拡大率の計算と図形ﾌｨｯﾄ
	CViewBase::OnViewFit(GetDocument()->GetMaxRect());
	if ( (BOOL)lParam )		// 再描画ﾌﾗｸﾞ == 現在ｱｸﾃｨﾌﾞﾍﾟｰｼﾞ
		OnViewLensComm();
	return 0;
}

void CNCViewXY::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) 
{
	if ( bActivate ) {
		// MDI子ﾌﾚｰﾑのｽﾃｰﾀｽﾊﾞｰに情報表示
		static_cast<CNCChild *>(GetParentFrame())->SetFactorInfo(NC_XY_PLANE, m_dFactor);
	}
	CView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}

void CNCViewXY::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	CViewBase::OnContextMenu(point, IDR_NCPOPUP1);
}

void CNCViewXY::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CViewBase::OnLButtonDown(point);
}

void CNCViewXY::OnLButtonUp(UINT nFlags, CPoint point) 
{
	if ( CViewBase::OnLButtonUp(point) == 1 )
		OnLensKey(ID_VIEW_LENSP);		// 拡大処理
}

void CNCViewXY::OnRButtonDown(UINT nFlags, CPoint point) 
{
	CViewBase::OnRButtonDown(point);
}

void CNCViewXY::OnRButtonUp(UINT nFlags, CPoint point) 
{
	switch ( CViewBase::OnRButtonUp(point) ) {
	case 1:		// 拡大処理
		OnLensKey(ID_VIEW_LENSP);
		break;
	case 2:		// ｺﾝﾃｷｽﾄﾒﾆｭｰ表示
		CView::OnRButtonUp(nFlags, point);
		break;
	}
}

void CNCViewXY::OnMouseMove(UINT nFlags, CPoint point) 
{
	CViewBase::OnMouseMove(nFlags, point);
}

BOOL CNCViewXY::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
	if ( CViewBase::OnMouseWheel(nFlags, zDelta, pt) )
		OnViewLensComm();

	return TRUE;
}

void CNCViewXY::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	// ｽﾌﾟﾘｯﾀｳｨﾝﾄﾞｳでﾀﾞﾌﾞﾙｸﾘｯｸされれば，XY単体表示に切り替え
	if ( !GetParent()->IsKindOf(RUNTIME_CLASS(CNCViewTab)) ) {
		static_cast<CNCChild *>(GetParentFrame())->GetMainView()->DblClkChange(1);
		return;
	}

	CView::OnLButtonDblClk(nFlags, point);
}

BOOL CNCViewXY::OnEraseBkgnd(CDC* pDC) 
{
	CRect	rc;
	GetClientRect(&rc);

	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	COLORREF	col1 = pOpt->GetNcDrawColor(NCCOL_BACKGROUND2),
				col2 = pOpt->GetNcDrawColor(NCCOL_BACKGROUND1);

	return AfxGetNCVCMainWnd()->DrawBackGroundView(pDC, &rc, col1, col2);
}
