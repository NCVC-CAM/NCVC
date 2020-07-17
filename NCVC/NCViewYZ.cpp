// NCViewYZ.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "NCChild.h"
#include "NCdata.h"
#include "NCDoc.h"
#include "NCViewTab.h"
#include "NCInfoTab.h"
#include "NCViewYZ.h"
#include "NCListView.h"
#include "ViewOption.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNCViewYZ

IMPLEMENT_DYNCREATE(CNCViewYZ, CView)

BEGIN_MESSAGE_MAP(CNCViewYZ, CView)
	//{{AFX_MSG_MAP(CNCViewYZ)
	ON_WM_CREATE()
	ON_WM_CONTEXTMENU()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_MOUSEACTIVATE()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_KEYDOWN()
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
// CNCViewYZ クラスの構築/消滅

CNCViewYZ::CNCViewYZ()
{
}

CNCViewYZ::~CNCViewYZ()
{
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewYZ クラスのオーバライド関数

BOOL CNCViewYZ::PreCreateWindow(CREATESTRUCT& cs) 
{
	// ｽﾌﾟﾘｯﾀからの境界ｶｰｿﾙが移るので，IDC_ARROW を明示的に指定
	cs.lpszClass = AfxRegisterWndClass(
			CS_HREDRAW|CS_VREDRAW|CS_OWNDC|CS_DBLCLKS,
			AfxGetApp()->LoadStandardCursor(IDC_ARROW) );
	return CView::PreCreateWindow(cs);
}

void CNCViewYZ::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
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
				ConvertWorkRect();
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

BOOL CNCViewYZ::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
	// ここから CDocument::OnCmdMsg() を呼ばないようにする
//	return CView::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
	return CWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewYZ クラスのメンバ関数

void CNCViewYZ::SetGuideData(void)
{
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	double	dSrc = pOpt->GetNCViewFlg(NCVIEWFLG_GUIDELENGTH) ?
					m_dFactor*LOMETRICFACTOR : LOMETRICFACTOR;
	// Ｙ軸のガイド初期化（奥から手前へ）
	m_ptGuid[0][0].x = (int)( pOpt->GetGuideLength(NCA_Y) * dSrc);
	m_ptGuid[0][0].y = 0;
	m_ptGuid[0][1].x = (int)(-pOpt->GetGuideLength(NCA_Y) * dSrc);
	m_ptGuid[0][1].y = 0;
	// Ｚ軸のガイド初期化（上から下へ）
	m_ptGuid[1][0].x = 0;
	m_ptGuid[1][0].y = (int)( pOpt->GetGuideLength(NCA_Z) * dSrc);
	m_ptGuid[1][1].x = 0;
	m_ptGuid[1][1].y = (int)(-pOpt->GetGuideLength(NCA_Z) * dSrc);
}

void CNCViewYZ::ConvertWorkRect(void)
{
	CRect3D	rcWork(GetDocument()->GetWorkRect());
	CRectD	rc(rcWork.top, rcWork.high, rcWork.bottom, rcWork.low);
	rc.NormalizeRect();
	m_rcDrawWork.left	= DrawConvert(rc.left);
	m_rcDrawWork.top	= DrawConvert(rc.top);
	m_rcDrawWork.right	= DrawConvert(rc.right);
	m_rcDrawWork.bottom	= DrawConvert(rc.bottom);
}

void CNCViewYZ::ConvertMaxRect(void)
{
	CRectD	rc(GetDrawMaxRect());
	m_rcDrawMax.left	= DrawConvert(rc.left);
	m_rcDrawMax.top		= DrawConvert(rc.top);
	m_rcDrawMax.right	= DrawConvert(rc.right);
	m_rcDrawMax.bottom	= DrawConvert(rc.bottom);
}

CRectD CNCViewYZ::GetDrawMaxRect(void)
{
	extern	const	double	g_dDefaultGuideLength;	// 50.0 (ViewOption.cpp)
	CRect3D	rcMax(GetDocument()->GetMaxRect());
	CRectD	rc(rcMax.top, rcMax.low, rcMax.bottom, rcMax.high);
	// 占有矩形の補正(不正表示の防止)
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	double	dLength;
	if ( rc.Width() <= NCMIN ) {
		dLength = pOpt->GetGuideLength(NCA_Y);
		if ( dLength == 0.0 )
			dLength = g_dDefaultGuideLength;
		rc.left  = -dLength;
		rc.right =  dLength;
	}
	if ( rc.Height() <= NCMIN ) {
		dLength = pOpt->GetGuideLength(NCA_Z);
		if ( dLength == 0.0 )
			dLength = g_dDefaultGuideLength;
		rc.top    = -dLength;
		rc.bottom =  dLength;
	}
	rc.NormalizeRect();
	return	rc;
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewYZ 描画

void CNCViewYZ::OnDraw(CDC* pDC)
{
	extern	LPCTSTR	g_szNdelimiter;	// "XYZRIJKPLDH" from NCDoc.cpp
	const CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();

	ASSERT_VALID(GetDocument());
	int		i;
	pDC->SetROP2(R2_COPYPEN);
	CPen* pOldPen = (CPen *)pDC->SelectStockObject(NULL_PEN);

	// 平面案内
	if ( pDC->m_hAttribDC ) {
		CPoint	pt(0, 0);
		pDC->DPtoLP(&pt);
		pDC->SetTextAlign(TA_LEFT|TA_TOP);
		pDC->SetTextColor(pOpt->GetNcDrawColor(NCCOL_PANE));
		if ( GetDocument()->IsThumbnail() ) {
			CGdiObject* pFontOld = pDC->SelectStockObject(SYSTEM_FIXED_FONT);
			pDC->TextOut(pt.x, pt.y, GetDocument()->GetTitle());
			pDC->SelectObject(pFontOld);
		}
		else
			pDC->TextOut(pt.x, pt.y, g_szNdelimiter+NCA_Y, 2);	// YZ
	}
	// ｶﾞｲﾄﾞ表示
	if ( !GetDocument()->IsThumbnail() ) {
		pDC->SetTextAlign(TA_CENTER|TA_BOTTOM);
		for ( i=0; i<SIZEOF(m_ptGuid); i++ ) {
			if ( pOpt->GetGuideLength(i+NCA_Y) > 0 ) {
				pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenOrg(i+NCA_Y));
				pDC->MoveTo(m_ptGuid[i][0]);
				pDC->LineTo(m_ptGuid[i][1]);
				pDC->SetTextColor(pOpt->GetNcDrawColor(i+NCCOL_GUIDEY));
				pDC->TextOut(m_ptGuid[i][0].x, m_ptGuid[i][0].y, g_szNdelimiter+i+NCA_Y, 1);
			}
		}
		if ( pOpt->GetNCViewFlg(NCVIEWFLG_GUIDESCALE) )
			DrawGuideScale(pDC);	// 目盛表示
	}
	// NCﾃﾞｰﾀ描画
	CNCdata*	pData;
	int	nDraw = GetDocument()->GetTraceDraw();	// ｸﾘﾃｨｶﾙｾｸｼｮﾝによるﾛｯｸ
	for ( i=GetDocument()->GetTraceStart(); i<nDraw; i++ ) {
		pData = GetDocument()->GetNCdata(i);
		if ( pData->GetGtype() == G_TYPE )
			pData->DrawYZ(pDC, FALSE);
	}
	if ( !GetDocument()->IsThumbnail() ) {
		// 最大切削矩形
		if ( GetDocument()->IsMaxRect() )
			DrawMaxRect(pDC);	// CNCViewBase
		// ﾜｰｸ矩形
		if ( GetDocument()->IsWorkRect() )
			DrawWorkRect(pDC);	// CNCViewBase
		// 現在の拡大矩形を描画(ViewBase.cpp)
		if ( m_bMagRect )
			DrawMagnifyRect(pDC);
	}

	pDC->SelectObject(pOldPen);
}

void CNCViewYZ::DrawGuideScale(CDC* pDC)
{
	const CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();
	CPointD	pt(-pOpt->GetGuideLength(NCA_Y), 0), ptDraw;

	pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenOrg(NCA_Y));
	for ( ; pt.x<=pOpt->GetGuideLength(NCA_Y); pt.x+=1.0 ) {
		if ( fmod(pt.x, 10.0) == 0 )
			pt.y = 1.5;
		else if ( fmod(pt.x, 5.0) == 0 )
			pt.y = 1.0;
		else
			pt.y = 0.5;
		ptDraw = pt * m_dFactor * LOMETRICFACTOR;
		pDC->MoveTo(ptDraw);
		pDC->LineTo((int)ptDraw.x, -(int)ptDraw.y);
	}

	pt.y = -pOpt->GetGuideLength(NCA_Z);
	pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenOrg(NCA_Z));
	for ( ; pt.y<=pOpt->GetGuideLength(NCA_Z); pt.y+=1.0 ) {
		if ( fmod(pt.y, 10.0) == 0 )
			pt.x = 1.5;
		else if ( fmod(pt.y, 5.0) == 0 )
			pt.x = 1.0;
		else
			pt.x = 0.5;
		ptDraw = pt * m_dFactor * LOMETRICFACTOR;
		pDC->MoveTo(ptDraw);
		pDC->LineTo(-(int)ptDraw.x, (int)ptDraw.y);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewYZ 診断

#ifdef _DEBUG
void CNCViewYZ::AssertValid() const
{
	CView::AssertValid();
}

void CNCViewYZ::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CNCDoc* CNCViewYZ::GetDocument() // 非デバッグ バージョンはインラインです。
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CNCDoc)));
	return static_cast<CNCDoc *>(m_pDocument);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CNCViewYZ クラスのメッセージ ハンドラ（メニュー編）

void CNCViewYZ::OnUpdateEditCopy(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(TRUE);
}

void CNCViewYZ::OnEditCopy() 
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

void CNCViewYZ::OnMoveKey(UINT nID)
{
	CViewBase::OnMoveKey(nID);
}

void CNCViewYZ::OnLensKey(UINT nID)
{
	switch ( nID ) {
	case ID_VIEW_BEFORE:
		CViewBase::OnBeforeMagnify();
		break;
	case ID_VIEW_FIT:
		CViewBase::OnViewFit(GetDrawMaxRect());
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

void CNCViewYZ::OnViewLensComm(void)
{
	// 拡大率による描画情報の更新
	GetDocument()->AllChangeFactorYZ(m_dFactor);
	if ( !GetDocument()->IsThumbnail() ) {
		// ｶﾞｲﾄﾞ軸
		if ( AfxGetNCVCApp()->GetViewOption()->GetNCViewFlg(NCVIEWFLG_GUIDELENGTH) )
			SetGuideData();
		// 各矩形情報の更新
		if ( GetDocument()->IsWorkRect() )
			ConvertWorkRect();
		ConvertMaxRect();
		// MDI子ﾌﾚｰﾑのｽﾃｰﾀｽﾊﾞｰに情報表示
		static_cast<CNCChild *>(GetParentFrame())->SetFactorInfo(NC_YZ_PLANE, m_dFactor);
	}
	// ﾋﾞｭｰの再描画
	Invalidate();
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewYZ メッセージ ハンドラ

int CNCViewYZ::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if ( CView::OnCreate(lpCreateStruct) < 0 )
		return -1;

	// ﾏｯﾋﾟﾝｸﾞﾓｰﾄﾞの変更など
	CViewBase::OnCreate(this);
	// ｶﾞｲﾄﾞﾃﾞｰﾀ
	SetGuideData();

	return 0;
}

void CNCViewYZ::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) 
{
	if ( bActivate && !GetDocument()->IsThumbnail() ) {
		// CNCViewSplit::SetActivePane() からのｱｸﾃｨﾌﾞ設定に対応
		// MDI子ﾌﾚｰﾑのｽﾃｰﾀｽﾊﾞｰに情報表示
		static_cast<CNCChild *>(GetParentFrame())->SetFactorInfo(NC_YZ_PLANE, m_dFactor);
	}
	CView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}

LRESULT CNCViewYZ::OnUserActivatePage(WPARAM, LPARAM lParam)
{
	if ( m_dFactor == 0.0 ) {
		// OnUserInitialUpdate() が実行されていないとき
		OnLensKey(ID_VIEW_FIT);
	}
	else if ( lParam ) {
		// 拡大率の再更新
		OnViewLensComm();
	}

	return 0;
}

LRESULT CNCViewYZ::OnUserViewFitMsg(WPARAM, LPARAM lParam)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CNCViewYZ::OnUserViewFitMsg()\nStart");
#endif
	// 拡大率の計算と図形ﾌｨｯﾄ
	CViewBase::OnViewFit(GetDrawMaxRect());
	if ( lParam )		// 再描画ﾌﾗｸﾞ == 現在ｱｸﾃｨﾌﾞﾍﾟｰｼﾞ
		OnViewLensComm();
	return 0;
}

void CNCViewYZ::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	CViewBase::OnContextMenu(point, IDR_NCPOPUP1);
}

void CNCViewYZ::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CViewBase::OnLButtonDown(point);
}

void CNCViewYZ::OnLButtonUp(UINT nFlags, CPoint point) 
{
	if ( CViewBase::OnLButtonUp(point) == 1 )
		OnLensKey(ID_VIEW_LENSP);		// 拡大処理
}

void CNCViewYZ::OnRButtonDown(UINT nFlags, CPoint point) 
{
	CViewBase::OnRButtonDown(point);
}

void CNCViewYZ::OnRButtonUp(UINT nFlags, CPoint point) 
{
	switch ( CViewBase::OnRButtonUp(point) ) {
	case 1:		// 拡大処理
		OnLensKey(ID_VIEW_LENSP);
		break;
	case 2:		// ｺﾝﾃｷｽﾄﾒﾆｭｰ表示
		// ｻﾑﾈｲﾙ表示ﾓｰﾄﾞでｺﾝﾃｷｽﾄﾒﾆｭｰは表示させない
		if ( !GetDocument()->IsThumbnail() )
			CView::OnRButtonUp(nFlags, point);
		break;
	}
}

void CNCViewYZ::OnMouseMove(UINT nFlags, CPoint point) 
{
	CViewBase::OnMouseMove(nFlags, point);
}

BOOL CNCViewYZ::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
	if ( CViewBase::OnMouseWheel(nFlags, zDelta, pt) )
		OnViewLensComm();

	return TRUE;
}

void CNCViewYZ::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	if ( GetDocument()->IsThumbnail() ) {
		// ｻﾑﾈｲﾙｺﾝﾄﾛｰﾙに通知
		GetParent()->SendMessage(WM_USERFINISH, (WPARAM)this);
	}
	else if ( !GetParent()->IsKindOf(RUNTIME_CLASS(CNCViewTab)) ) {
		// ｽﾌﾟﾘｯﾀｳｨﾝﾄﾞｳでﾀﾞﾌﾞﾙｸﾘｯｸされれば，XYZ単体表示に切り替え
		static_cast<CNCChild *>(GetParentFrame())->GetMainView()->DblClkChange(3);
		return;
	}

	CView::OnLButtonDblClk(nFlags, point);
}

void CNCViewYZ::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if ( !GetDocument()->IsThumbnail() ) {
		if ( nChar == VK_TAB ) {
			CNCChild*	pFrame = static_cast<CNCChild *>(GetParentFrame());
			if ( ::GetKeyState(VK_SHIFT) < 0 )
				pFrame->GetListView()->SetFocus();
			else
				pFrame->GetInfoView()->SetFocus();
		}
	}

	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

int CNCViewYZ::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message)
{
	// 基底ｸﾗｽを呼び出すと、CFrameWnd関連が呼ばれるので
	// ｻﾑﾈｲﾙ表示ﾓｰﾄﾞでは不具合(ASSERT)が発生する
	if ( GetDocument()->IsThumbnail() ) {
		SetFocus();		// 自力でﾌｫｰｶｽを取得
		return MA_ACTIVATE;
	}
	return CView::OnMouseActivate(pDesktopWnd, nHitTest, message);
}

BOOL CNCViewYZ::OnEraseBkgnd(CDC* pDC) 
{
	CRect	rc;
	GetClientRect(rc);

	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	COLORREF	col1 = pOpt->GetNcDrawColor(NCCOL_BACKGROUND2),
				col2 = pOpt->GetNcDrawColor(NCCOL_BACKGROUND1);

	return AfxGetNCVCMainWnd()->DrawBackGroundView(pDC, &rc, col1, col2);
}
