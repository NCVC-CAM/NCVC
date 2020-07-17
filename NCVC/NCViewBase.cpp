// NCViewBase.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "NCChild.h"
#include "NCDoc.h"
#include "NCViewBase.h"
#include "NCViewTab.h"
#include "NCInfoTab.h"
#include "NCListView.h"
#include "ViewOption.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

using namespace boost;
#define	IsThumbnail()	GetDocument()->IsDocFlag(NCDOC_THUMBNAIL)

/////////////////////////////////////////////////////////////////////////////
// CNCViewBase クラス

BEGIN_MESSAGE_MAP(CNCViewBase, CViewBase)
	ON_WM_CONTEXTMENU()
	ON_WM_KEYDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEACTIVATE()
	ON_WM_ERASEBKGND()
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, &CNCViewBase::OnUpdateEditCopy)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_UP,  ID_VIEW_RT,  &CNCViewBase::OnUpdateMoveKey)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_RUP, ID_VIEW_RRT, &CNCViewBase::OnUpdateRoundKey)
	ON_COMMAND(ID_EDIT_COPY, &CNCViewBase::OnEditCopy)
	ON_COMMAND_RANGE(ID_VIEW_UP,  ID_VIEW_RT,    &CNCViewBase::OnMoveKey)
	ON_COMMAND_RANGE(ID_VIEW_FIT, ID_VIEW_LENSN, &CNCViewBase::OnLensKey)
	ON_MESSAGE (WM_USERVIEWFITMSG, &CNCViewBase::OnUserViewFitMsg)
	ON_MESSAGE (WM_USERACTIVATEPAGE, &CNCViewBase::OnUserActivatePage)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNCViewBase クラスのオーバライド関数

BOOL CNCViewBase::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
#ifdef _DEBUG_CMDMSG
	g_dbg.printf("CNCViewBase::OnCmdMsg()");
#endif
	// ここから CDocument::OnCmdMsg() を呼ばないようにする
//	return CView::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
	return CWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

void CNCViewBase::OnInitialUpdate()
{
	// ｶﾞｲﾄﾞﾃﾞｰﾀ
	SetGuideData();
	// 最大ﾃﾞｰﾀ矩形の補填
	SetDataMaxRect();
	//
	if ( GetDocument()->IsDocFlag(NCDOC_LATHE) ) {
		// 親ﾀﾌﾞの名称を変更
		CWnd*	pParent = GetParent();
		if ( pParent->IsKindOf(RUNTIME_CLASS(CNCViewTab)) )
			static_cast<CNCViewTab *>(pParent)->SetPageTitle(m_enView, m_strGuide);
		// (Lathe Mode)文字列を追加
		CString	strLathe;
		VERIFY(strLathe.LoadString(IDCV_LATHE));
		m_strGuide += strLathe;
	}
	else if ( GetDocument()->IsDocFlag(NCDOC_WIRE) ) {
		// (Wire Mode)文字列を追加
		CString	strLathe;
		VERIFY(strLathe.LoadString(IDCV_WIRE));
		m_strGuide += strLathe;
	}
}

void CNCViewBase::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	switch ( lHint ) {
	case UAV_TRACECURSOR:
		Invalidate();
		UpdateWindow();		// 即再描画
		return;
	case UAV_FILEINSERT:
		SetDataMaxRect();
		OnLensKey(ID_VIEW_FIT);
		UpdateWindow();
		return;
	case UAV_DRAWWORKRECT:
		if ( pHint ) {
			CClientDC	dc(this);
			dc.SetROP2(R2_COPYPEN);
			if ( GetDocument()->IsDocFlag(NCDOC_CYLINDER) ) {
				SetWorkCylinder();
				ConvertWorkCylinder();
				DrawWorkCylinder(&dc);
			}
			else if ( GetDocument()->IsDocFlag(NCDOC_WRKRECT) ) {
				SetWorkRect();
				ConvertWorkRect();
				DrawWorkRect(&dc);
			}
		}
		break;
	case UAV_DRAWMAXRECT:
		{
			CClientDC	dc(this);
			dc.SetROP2(GetDocument()->IsDocFlag(NCDOC_MAXRECT) ? R2_COPYPEN : R2_XORPEN);
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

void CNCViewBase::OnDraw(CDC* pDC)
{
	ASSERT_VALID(GetDocument());
	const CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();

	pDC->SetROP2(R2_COPYPEN);
	CPen* pOldPen = (CPen *)pDC->SelectStockObject(NULL_PEN);

	// 各面初期設定
	int			i;
	size_t		x, y;
	float		dLength[2];
	CPen*		pOrgPen[2];
	COLORREF	colOrg[2];
	tie(x, y) = GetPlaneAxis();
	dLength[0]	= pOpt->GetGuideLength(x);
	dLength[1]	= pOpt->GetGuideLength(y);
	pOrgPen[0]	= AfxGetNCVCMainWnd()->GetPenOrg(x);
	pOrgPen[1]	= AfxGetNCVCMainWnd()->GetPenOrg(y);
	colOrg[0]	= pOpt->GetNcDrawColor(x+NCCOL_GUIDEX);
	colOrg[1]	= pOpt->GetNcDrawColor(y+NCCOL_GUIDEX);

	// 平面案内
	DrawInfo(pDC);

	// ｶﾞｲﾄﾞ表示
	if ( !IsThumbnail() ) {
		pDC->SetTextAlign(TA_CENTER|TA_BOTTOM);
		for ( i=0; i<SIZEOF(dLength); i++ ) {
			if ( dLength[i] > 0 ) {
				pDC->SelectObject(pOrgPen[i]);
				pDC->MoveTo(m_ptGuide[i][0]);
				pDC->LineTo(m_ptGuide[i][1]);
				pDC->SetTextColor(colOrg[i]);
				pDC->TextOut(m_ptGuide[i][0].x, m_ptGuide[i][0].y, m_strGuide.Mid(i, 1));
			}
		}
		if ( pOpt->GetNCViewFlg(NCVIEWFLG_GUIDESCALE) )
			DrawGuideDivi(pDC, x, y);	// 目盛表示
	}

	// NCﾃﾞｰﾀ描画
	DrawNCdata(pDC);

	// 補助矩形
	DrawSupportRect(pDC);
	
	pDC->SelectObject(pOldPen);
}

void CNCViewBase::DrawGuideDivi(CDC* pDC, size_t x, size_t y)
{
	const CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();
	CPointF	pt(-pOpt->GetGuideLength(x), 0), ptDraw;

	pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenOrg(x));
	for ( ; pt.x<=pOpt->GetGuideLength(x); pt.x+=1.0 ) {
		if ( fmod(pt.x, 10.0) == 0 )
			pt.y = 1.5;
		else if ( fmod(pt.x, 5.0) == 0 )
			pt.y = 1.0;
		else
			pt.y = 0.5;
		ptDraw = pt * m_dFactor;
		pDC->MoveTo(ptDraw);
		pDC->LineTo((int)ptDraw.x, -(int)ptDraw.y);
	}

	pt.y = -pOpt->GetGuideLength(y);
	pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenOrg(y));
	for ( ; pt.y<=pOpt->GetGuideLength(y); pt.y+=1.0 ) {
		if ( fmod(pt.y, 10.0) == 0 )
			pt.x = 1.5;
		else if ( fmod(pt.y, 5.0) == 0 )
			pt.x = 1.0;
		else
			pt.x = 0.5;
		ptDraw = pt * m_dFactor;
		pDC->MoveTo(ptDraw);
		pDC->LineTo(-(int)ptDraw.x, (int)ptDraw.y);
	}
}

void CNCViewBase::DrawInfo(CDC* pDC)
{
	const CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();
	CPoint	pt(0, 0);
	pDC->DPtoLP(&pt);
	pDC->SetTextAlign(TA_LEFT|TA_TOP);
	pDC->SetTextColor(pOpt->GetNcDrawColor(NCCOL_PANE));
	if ( IsThumbnail() ) {
		// ﾌｧｲﾙ名を表示
		CGdiObject* pFontOld = pDC->SelectStockObject(SYSTEM_FIXED_FONT);
		pDC->TextOut(pt.x, pt.y, GetDocument()->GetTitle());
		pDC->SelectObject(pFontOld);
	}
	else {
		// 表示平面の案内文字
		pDC->TextOut(pt.x, pt.y, m_strGuide);
	}
}

void CNCViewBase::DrawNCdata(CDC* pDC)
{
	ASSERT( m_pfnDrawProc );
	const CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();
	CNCdata*	pData = NULL;
	size_t	nDraw = GetDocument()->GetTraceDraw();	// ｸﾘﾃｨｶﾙｾｸｼｮﾝによるﾛｯｸ
	for ( size_t i=GetDocument()->GetTraceStart(); i<nDraw; i++ ) {
		pData = GetDocument()->GetNCdata(i);
		if ( pData->GetGtype() == G_TYPE )
			(pData->*m_pfnDrawProc)(pDC, FALSE);
	}
	if ( pOpt->GetNCViewFlg(NCVIEWFLG_TRACEMARKER) && pData && nDraw!=GetDocument()->GetNCsize() ) {
		// 最後に描画したｵﾌﾞｼﾞｪｸﾄのﾄﾚｰｽﾏｰｶｰ
		(pData->*m_pfnDrawProc)(pDC, TRUE);
	}
}

void CNCViewBase::DrawSupportRect(CDC* pDC)
{
	if ( !IsThumbnail() ) {
		// 最大切削矩形
		if ( GetDocument()->IsDocFlag(NCDOC_MAXRECT) )
			DrawMaxRect(pDC);
		// ﾜｰｸ
		if ( GetDocument()->IsDocFlag(NCDOC_CYLINDER) )
			DrawWorkCylinder(pDC);
		else if ( GetDocument()->IsDocFlag(NCDOC_WRKRECT) )
			DrawWorkRect(pDC);
		// 現在の拡大矩形を描画(ViewBase.cpp)
		if ( m_bMagRect )
			DrawMagnifyRect(pDC);
	}
}

void CNCViewBase::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) 
{
	if ( bActivate && !IsThumbnail() ) {
		// CNCViewSplit::SetActivePane() からのｱｸﾃｨﾌﾞ設定に対応
		// MDI子ﾌﾚｰﾑのｽﾃｰﾀｽﾊﾞｰに情報表示
		static_cast<CNCChild *>(GetParentFrame())->SetFactorInfo(m_dFactor/LOMETRICFACTOR,
			m_strGuide.Left(m_enView==NCDRAWVIEW_XYZ ? 3 : 2));
	}
	CView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewBase クラスのメンバ関数

void CNCViewBase::SetDataMaxRect(void)
{
	extern	const	float	g_dDefaultGuideLength;	// 50.0 (ViewOption.cpp)
	m_rcDataMax = ConvertRect(GetDocument()->GetMaxRect());
	size_t	x, y;
	tie(x, y) = GetPlaneAxis();
	// 占有矩形の補正(不正表示の防止)
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	float	dLength;
	if ( m_rcDataMax.Width() <= NCMIN ) {
		dLength = pOpt->GetGuideLength(x);
		if ( dLength == 0.0f )
			dLength = g_dDefaultGuideLength;
		m_rcDataMax.left  = -dLength;
		m_rcDataMax.right =  dLength;
	}
	if ( m_rcDataMax.Height() <= NCMIN ) {
		dLength = pOpt->GetGuideLength(y);
		if ( dLength == 0.0f )
			dLength = g_dDefaultGuideLength;
		m_rcDataMax.top    = -dLength;
		m_rcDataMax.bottom =  dLength;
	}
	m_rcDataMax.NormalizeRect();
}

void CNCViewBase::OnViewLensComm(void)
{
	// 拡大率による描画情報の更新
	GetDocument()->AllChangeFactor(m_enView, m_dFactor);
	if ( !IsThumbnail() ) {
		// ｶﾞｲﾄﾞ軸
		if ( AfxGetNCVCApp()->GetViewOption()->GetNCViewFlg(NCVIEWFLG_GUIDELENGTH) )
			SetGuideData();
		// 各矩形情報の更新
		ConvertMaxRect();
		if ( GetDocument()->IsDocFlag(NCDOC_CYLINDER) )
			ConvertWorkCylinder();
		else if ( GetDocument()->IsDocFlag(NCDOC_WRKRECT) )
			ConvertWorkRect();
		// MDI子ﾌﾚｰﾑのｽﾃｰﾀｽﾊﾞｰに情報表示
		static_cast<CNCChild *>(GetParentFrame())->SetFactorInfo(m_dFactor/LOMETRICFACTOR,
			m_strGuide.Left(m_enView==NCDRAWVIEW_XYZ ? 3 : 2));
	}
	// ﾋﾞｭｰの再描画
	Invalidate();
}

#ifdef _DEBUG
CNCDoc* CNCViewBase::GetDocument()
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CNCDoc)));
	return static_cast<CNCDoc *>(m_pDocument);
}
#endif

/////////////////////////////////////////////////////////////////////////////
// CNCViewBase クラスのメッセージ ハンドラ
// 各派生Viewの共通処理

void CNCViewBase::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	CViewBase::_OnContextMenu(point, IDR_NCPOPUP1);
}

void CNCViewBase::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if ( !IsThumbnail() ) {
		if ( nChar == VK_TAB ) {
			CNCChild*	pFrame = static_cast<CNCChild *>(GetParentFrame());
			if ( ::GetKeyState(VK_SHIFT) < 0 )
				pFrame->GetListView()->SetFocus();
			else
				pFrame->GetInfoView()->SetFocus();
		}
	}

	__super::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CNCViewBase::OnLButtonUp(UINT nFlags, CPoint point) 
{
	if ( CViewBase::_OnLButtonUp(point) == 1 )
		OnLensKey(ID_VIEW_LENSP);		// 拡大処理
}

void CNCViewBase::OnRButtonUp(UINT nFlags, CPoint point) 
{
	switch ( CViewBase::_OnRButtonUp(point) ) {
	case 1:		// 拡大処理
		OnLensKey(ID_VIEW_LENSP);
		break;
	case 2:		// ｺﾝﾃｷｽﾄﾒﾆｭｰ表示
		// ｻﾑﾈｲﾙ表示ﾓｰﾄﾞでｺﾝﾃｷｽﾄﾒﾆｭｰは表示させない
		if ( !IsThumbnail() )
			CView::OnRButtonUp(nFlags, point);
		break;
	}
}

void CNCViewBase::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	if ( IsThumbnail() ) {
		// ｻﾑﾈｲﾙｺﾝﾄﾛｰﾙに通知
		GetParent()->SendMessage(WM_USERFINISH, (WPARAM)this);
	}
	else if ( !GetParent()->IsKindOf(RUNTIME_CLASS(CNCViewTab)) ) {
		// ｽﾌﾟﾘｯﾀｳｨﾝﾄﾞｳでﾀﾞﾌﾞﾙｸﾘｯｸされれば，XY単体表示に切り替え
		static_cast<CNCChild *>(GetParentFrame())->GetMainView()->DblClkChange(m_enView);
		return;
	}

	__super::OnLButtonDblClk(nFlags, point);
}

void CNCViewBase::OnMouseMove(UINT nFlags, CPoint point) 
{
	CViewBase::_OnMouseMove(nFlags, point);
}

int CNCViewBase::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message)
{
	if ( IsThumbnail() ) {
		SetFocus();		// 自力でﾌｫｰｶｽを取得
		return MA_ACTIVATE;
	}
	return __super::OnMouseActivate(pDesktopWnd, nHitTest, message);
}

void CNCViewBase::OnUpdateEditCopy(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(TRUE);	// OpenGLﾋﾞｭｰでﾌﾞﾛｯｸしているので保険
}

void CNCViewBase::OnUpdateMoveKey(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(TRUE);
}

void CNCViewBase::OnUpdateRoundKey(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(FALSE);
}

void CNCViewBase::OnEditCopy() 
{
	CWaitCursor		wait;	// 砂時計ｶｰｿﾙ

	CClientDC		dc(this);
	CMetaFileDC		metaDC;
	metaDC.CreateEnhanced(&dc, NULL, NULL, NULL);
	metaDC.SetMapMode(MM_LOMETRIC);
	metaDC.SetBkMode(TRANSPARENT);
	metaDC.SelectStockObject(NULL_BRUSH);
	AfxGetNCVCMainWnd()->SelectGDI(FALSE);	// GDIｵﾌﾞｼﾞｪｸﾄの切替
	OnDraw(&metaDC);	// virtual
	AfxGetNCVCMainWnd()->SelectGDI();

	// ｸﾘｯﾌﾟﾎﾞｰﾄﾞへのﾃﾞｰﾀｺﾋﾟｰ ViewBase.cpp
	CopyNCDrawForClipboard(metaDC.CloseEnhanced());
}

void CNCViewBase::OnMoveKey(UINT nID)
{
	CViewBase::OnMoveKey(nID);
}

void CNCViewBase::OnLensKey(UINT nID)
{
	switch ( nID ) {
	case ID_VIEW_FIT:
		CViewBase::OnViewFit(m_rcDataMax);
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

LRESULT CNCViewBase::OnUserViewFitMsg(WPARAM, LPARAM lParam)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CNCViewBase::OnUserViewFitMsg()\nStart");
#endif
	// 拡大率の計算と図形ﾌｨｯﾄ
	CViewBase::OnViewFit(m_rcDataMax);
	if ( lParam )		// 再描画ﾌﾗｸﾞ == 現在ｱｸﾃｨﾌﾞﾍﾟｰｼﾞ
		OnViewLensComm();
	return 0;
}

LRESULT CNCViewBase::OnUserActivatePage(WPARAM, LPARAM lParam)
{
	if ( m_dFactor == 0.0f ) {
		// OnUserInitialUpdate() が実行されていないとき
		OnLensKey(ID_VIEW_FIT);
	}
	else if ( lParam ) {
		// 拡大率の再更新
		OnViewLensComm();
	}

	return 0;
}

BOOL CNCViewBase::OnEraseBkgnd(CDC* pDC) 
{
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	COLORREF	col1 = pOpt->GetNcDrawColor(NCCOL_BACKGROUND1),
				col2 = pOpt->GetNcDrawColor(NCCOL_BACKGROUND2);
	return CViewBase::_OnEraseBkgnd(pDC, col1, col2);
}
