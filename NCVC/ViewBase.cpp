// ViewBase.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "ViewBase.h"
#include "ViewOption.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

// ﾏｳｽﾄﾞﾗｯｸﾞ時の最低移動量
static	const	int		MAGNIFY_RANGE = 10;	// ﾋﾟｸｾﾙ

/////////////////////////////////////////////////////////////////////////////
// CViewBase

CViewBase::CViewBase()
{
	m_dFactor = m_dBeforeFactor = 0.0;
	m_nLState = m_nRState = -1;
	m_nBoth = 0;
	m_bMagRect = FALSE;
}

BOOL CViewBase::PreCreateWindow(CREATESTRUCT& cs) 
{
	// ｽﾌﾟﾘｯﾀからの境界ｶｰｿﾙが移るので，IDC_ARROW を明示的に指定
	cs.lpszClass = AfxRegisterWndClass(
			CS_HREDRAW|CS_VREDRAW|CS_OWNDC|CS_DBLCLKS,
			AfxGetApp()->LoadStandardCursor(IDC_ARROW) );
	return __super::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CViewBase クラスのメッセージ ハンドラ (メニューコマンド)

void CViewBase::OnViewFit(const CRectD& rcMax, BOOL bInflate/*=TRUE*/)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CViewBase::OnViewFit()");
#endif
	CRectD	rcObj(rcMax);

	if ( bInflate ) {
		// ｵﾌﾞｼﾞｪｸﾄ矩形を10%(上下左右5%ずつ)大きく
		rcObj.InflateRect(rcMax.Width()*0.05, rcMax.Height()*0.05);
	}
	CClientDC	dc(this);
	// 直前の拡大率
	if ( m_dFactor != 0.0 ) {
		m_dBeforeFactor = m_dFactor;
		m_ptBeforeOrg = dc.GetWindowOrg();
	}

	// 画面の表示領域を 1mm単位で取得
	CRect		rc;
	GetClientRect(rc);
	CSize	sz(rc.Width(), rc.Height());
	dc.DPtoLP(&sz);
	double	cx = sz.cx / LOMETRICFACTOR;
	double	cy = sz.cy / LOMETRICFACTOR;
	// 拡大率の計算
	double	dFactorH = cx / rcObj.Width();
	double	dFactorV = cy / rcObj.Height();
#ifdef _DEBUG
	dbg.printf("cx=%f cy=%f", cx, cy);
	dbg.printf("dFactorH=%f dFactorV=%f", dFactorH, dFactorV);
#endif
	// ﾋﾞｭｰ原点の設定(ｵﾌﾞｼﾞｪｸﾄ矩形(上下反対)の左上隅をﾃﾞﾊﾞｲｽ座標の原点(左上隅)へ)と
	// 画面中央への補正
	CPoint	pt;
	if ( dFactorH < dFactorV ) {
		m_dFactor = dFactorH;
		// 横倍率採用->縦原点の補正
		pt.x = DrawConvert(rcObj.left);
		pt.y = (int)((rcObj.bottom*m_dFactor +
						(cy-rcObj.Height()*m_dFactor)/2) * LOMETRICFACTOR);
	}
	else {
		m_dFactor = dFactorV;
		// 縦倍率採用->横原点の補正
		pt.x = (int)((rcObj.left*m_dFactor -
						(cx-rcObj.Width()*m_dFactor)/2) * LOMETRICFACTOR);
		pt.y = DrawConvert(rcObj.bottom);
	}
#ifdef _DEBUG
	dbg.printf("ptorg.x=%d ptorg.y=%d", pt.x, pt.y);
#endif
	dc.SetWindowOrg(pt);

	// 初期の拡大率
	if ( m_dBeforeFactor == 0.0 ) {
		m_dBeforeFactor = m_dFactor;
		m_ptBeforeOrg = pt;
	}
}

void CViewBase::OnViewLensP(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CViewBase::OnViewLensP()");
#endif
	CClientDC	dc(this);

	// 前処理
	CSize	sz( OnViewLens(dc) );

	// 拡大率の計算
	double	dFactorH = (double)sz.cx / m_rcMagnify.Width();
	double	dFactorV = (double)sz.cy / m_rcMagnify.Height();
	double	dFactor  = m_dFactor;
#ifdef _DEBUG
	dbg.printf("dFactorH=%f dFactorV=%f", dFactorH, dFactorV);
#endif
	// ﾋﾞｭｰ原点の設定(ｵﾌﾞｼﾞｪｸﾄ矩形(上下反対)の左上隅をﾃﾞﾊﾞｲｽ座標の原点(左上隅)へ)と
	// 画面中央への補正
	CPoint	pt;
	if ( dFactorH < dFactorV ) {
		m_dFactor *= dFactorH;
		// 横倍率採用->縦原点の補正
		pt.x = (int)(m_rcMagnify.left / dFactor * m_dFactor);
		pt.y = (int)(m_rcMagnify.bottom / dFactor * m_dFactor +
						(sz.cy-m_rcMagnify.Height()/dFactor*m_dFactor)/2);
#ifdef _DEBUG
		dbg.printf("OffsetY=%f", (sz.cy-m_rcMagnify.Height()/dFactor*m_dFactor)/2);
#endif
	}
	else {
		m_dFactor *= dFactorV;
		// 縦倍率採用->横原点の補正
		pt.x = (int)(m_rcMagnify.left/dFactor*m_dFactor -
						(sz.cx-m_rcMagnify.Width()/dFactor*m_dFactor)/2);
		pt.y = (int)(m_rcMagnify.bottom / dFactor * m_dFactor);
#ifdef _DEBUG
		dbg.printf("OffsetX=%f", (sz.cx-m_rcMagnify.Width()/dFactor*m_dFactor)/2);
#endif
	}
#ifdef _DEBUG
	dbg.printf("NewFactor=%f", m_dFactor);
	dbg.printf("ptorg.x=%d ptorg.y=%d", pt.x, pt.y);
#endif
	dc.SetWindowOrg(pt);
}

void CViewBase::OnViewLensN(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CViewBase::OnViewLensN()");
#endif
	CClientDC	dc(this);

	// 前処理
	CSize	sz( OnViewLens(dc) );

	// 拡大率の計算
	double	dFactorH = (double)m_rcMagnify.Width()  / sz.cx;
	double	dFactorV = (double)m_rcMagnify.Height() / sz.cy;
	double	dFactor  = m_dFactor;
#ifdef _DEBUG
	dbg.printf("dFactorH=%f dFactorV=%f", dFactorH, dFactorV);
#endif
	// ﾋﾞｭｰ原点の設定(ｵﾌﾞｼﾞｪｸﾄ矩形(上下反対)の左上隅をﾃﾞﾊﾞｲｽ座標の原点(左上隅)へ)と
	// 画面中央への補正(縮小の場合はX,Y共に補正する必要あり)
	CPoint	pt;
	if ( dFactorH > dFactorV )	// 拡大とは分子分母が逆なので
		m_dFactor *= dFactorH;
	else
		m_dFactor *= dFactorV;
	pt.x = (int)(m_rcMagnify.left/dFactor*m_dFactor -
					(sz.cx-m_rcMagnify.Width()/dFactor*m_dFactor)/2);
	pt.y = (int)(m_rcMagnify.bottom/dFactor*m_dFactor +
					(sz.cy-m_rcMagnify.Height()/dFactor*m_dFactor)/2);
#ifdef _DEBUG
	dbg.printf("OffsetX=%f", (sz.cx-m_rcMagnify.Width()/dFactor*m_dFactor)/2);
	dbg.printf("OffsetY=%f", (sz.cy-m_rcMagnify.Height()/dFactor*m_dFactor)/2);
	dbg.printf("NewFactor=%f", m_dFactor);
	dbg.printf("ptorg.x=%d ptorg.y=%d", pt.x, pt.y);
#endif
	dc.SetWindowOrg(pt);
}

CSize CViewBase::OnViewLens(CClientDC& dc)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CViewBase::OnViewLens()");
#endif
	// 現在のｸﾗｲｱﾝﾄ領域の大きさ
	CRect		rc;
	GetClientRect(rc);
	CSize	sz(rc.Width(), rc.Height());
	dc.DPtoLP(&sz);

	// 拡大矩形が指定されているか否か
	if ( m_bMagRect ) {
		// 直前の拡大率
		m_dBeforeFactor = m_dFactor;
		m_ptBeforeOrg = dc.GetWindowOrg();
		m_bMagRect = FALSE;
	}
	else {
		dc.DPtoLP(&rc);
		m_rcMagnify = rc;
		// ｸﾗｲｱﾝﾄ矩形の10%を拡大縮小領域とする
		m_rcMagnify.DeflateRect((int)(rc.Width()*0.1), (int)(rc.Height()*0.1));
	}
	m_rcMagnify.NormalizeRect();

#ifdef _DEBUG
	dbg.printf("sz.cx=%d sz.cy=%d", sz.cx, sz.cy);
	dbg.printf("m_rcMagnify.left=%d m_rcMagnify.bottom=%d",
		m_rcMagnify.left, m_rcMagnify.bottom);
	dbg.printf("m_rcMagnify.Width()=%d m_rcMagnify.Height()=%d",
		m_rcMagnify.Width(), m_rcMagnify.Height());
#endif
	return sz;
}

void CViewBase::OnMoveKey(UINT nID)
{
	CClientDC	dc(this);

	CRect		rc;
	GetClientRect(rc);
	// ｸﾗｲｱﾝﾄ座標の1/6を移動範囲とする
	CSize	sz(rc.Width()/6, rc.Height()/6);
	dc.DPtoLP(&sz);
	CPoint	pt(dc.GetWindowOrg());
	
	switch (nID) {
	case ID_VIEW_UP:
		pt.y += sz.cy;
		break;
	case ID_VIEW_DW:
		pt.y -= sz.cy;
		break;
	case ID_VIEW_LT:
		pt.x -= sz.cx;
		break;
	case ID_VIEW_RT:
		pt.x += sz.cx;
		break;
	}
	dc.SetWindowOrg(pt);
	Invalidate();
}

void CViewBase::OnBeforeMagnify(void)
{
	if ( m_dBeforeFactor == 0.0 )
		return;

	CClientDC	dc(this);

	// 現在表示中の値をﾊﾞｯｸｱｯﾌﾟ
	CPoint	ptOrg(dc.GetWindowOrg());
	double	dFactor(m_dFactor);

	dc.SetWindowOrg(m_ptBeforeOrg);
	m_dFactor = m_dBeforeFactor;

	// 直前の拡大率
	m_ptBeforeOrg = ptOrg;
	m_dBeforeFactor = dFactor;
}

/////////////////////////////////////////////////////////////////////////////
// CViewBase クラスのメッセージ ハンドラ

int CViewBase::OnCreate(LPCREATESTRUCT lpCreateStruct, BOOL bDC/*=TRUE*/)
{
	if ( __super::OnCreate(lpCreateStruct) < 0 )
		return -1;

	if ( bDC ) {
		// View の WNDCLASS に CS_OWNDC を指定しているため
		// OnCreate() にて、マッピングモード等の変更
		CClientDC	dc(this);
		dc.SetMapMode(MM_LOMETRIC);		// 0.1mm
		dc.SetBkMode(TRANSPARENT);
		dc.SelectStockObject(NULL_BRUSH);
	}

	return 0;
}

void CViewBase::OnLButtonDown(const CPoint& pt)
{
	CClientDC	dc(this);

	// 表示中なら前回矩形を消去
	if ( m_bMagRect )
		DrawMagnifyRect(&dc);
	m_bMagRect = FALSE;
	// 既にR処理中なら
	if ( m_nRState >= 0 ) {
		m_nRState = -1;		// R処理ｷｬﾝｾﾙ
		m_nBoth = 2;		// 両ﾎﾞﾀﾝ処理ｶｳﾝﾀ
	}
	else {
		SetCapture();
		m_nBoth = 1;
	}

	m_nLState = 0;
	m_ptMouse = pt;		// ﾃﾞﾊﾞｲｽ座標
	// Lﾏｳｽ押した座標を論理座標に変換し保存
	CPoint	ptLog(pt);
	dc.DPtoLP(&ptLog);
	m_rcMagnify.TopLeft() = ptLog;
}

int CViewBase::OnLButtonUp(const CPoint& pt)
{
	int nResult = OnMouseButtonUp(m_nLState, pt);
	m_nLState = -1;
	return nResult;
}

void CViewBase::OnRButtonDown(const CPoint& pt)
{
	// 既にL処理中なら
	if ( m_nLState >= 0 ) {
		m_nBoth = 2;		// 両ﾎﾞﾀﾝ処理ｶｳﾝﾀ
		return;				// m_nRState==-1 のまま
	}
	else
		m_nBoth = 1;

	CClientDC	dc(this);

	SetCapture();
	m_nRState = 0;
	m_ptMouse = pt;		// ﾃﾞﾊﾞｲｽ座標
	// 移動基点をｾｯﾄ
	m_ptMovOrg = pt;
	dc.DPtoLP(&m_ptMovOrg);
}

int CViewBase::OnRButtonUp(const CPoint& pt)
{
	int nResult = OnMouseButtonUp(m_nRState, pt);
	switch ( nResult ) {
	case 1:
		m_nLState = -1;		// 処理済み
		break;
	case 2:
		if ( m_nRState < 0 )
			nResult = 0;	// ｺﾝﾃｷｽﾄﾒﾆｭｰ非表示
		break;
	}
	m_nRState = -1;
	return nResult;
}

int CViewBase::OnMouseButtonUp(int nState, const CPoint& pt)
{
	if ( m_nBoth-1 <= 0 )
		ReleaseCapture();

	int	nResult = 0;
	CClientDC	dc(this);
	CPoint	ptLog(pt);
	dc.DPtoLP(&ptLog);

	if ( m_nLState >= 0 ) {
		if ( abs(m_ptMouse.x - pt.x) >= MAGNIFY_RANGE &&
			 abs(m_ptMouse.y - pt.y) >= MAGNIFY_RANGE ) {
			m_rcMagnify.BottomRight() = ptLog;
			m_bMagRect = TRUE;
		}
		if ( m_nBoth > 1 && m_bMagRect )
			nResult = 1;	// 拡大処理
	}
	else {
		if ( nState <= 0 )
			nResult = 2;	// ｺﾝﾃｷｽﾄﾒﾆｭｰ表示(Rのみ)
	}

	if ( --m_nBoth < 0 )
		m_nBoth = 0;

	return nResult;
}

BOOL CViewBase::OnMouseMove(UINT nFlags, const CPoint& pt)
{
	if ( !(nFlags & (MK_LBUTTON|MK_RBUTTON)) )
		return FALSE;
	
	// 移動量が最低に満たない場合の判断はLとRで区別
	if ( nFlags & MK_LBUTTON ) {
		if ( m_nLState < 0 ||
			(abs(m_ptMouse.x - pt.x) < MAGNIFY_RANGE ||
			 abs(m_ptMouse.y - pt.y) < MAGNIFY_RANGE) )
			return FALSE;
	}
	else {
		if ( m_nRState < 0 ||
			(abs(m_ptMouse.x - pt.x) < MAGNIFY_RANGE &&
			 abs(m_ptMouse.y - pt.y) < MAGNIFY_RANGE) )
			return FALSE;
	}

	CClientDC	dc(this);
	CPoint		ptLog(pt);
	dc.DPtoLP(&ptLog);

	if ( nFlags & MK_LBUTTON ) {
		// 現在矩形を消去
		if ( m_bMagRect )
			DrawMagnifyRect(&dc);
		else
			m_bMagRect = TRUE;
		m_nLState = 1;
		// 現在値を論理座標に変換し矩形情報ｾｯﾄ
		m_rcMagnify.BottomRight() = ptLog;
		DrawMagnifyRect(&dc);
	}
	else {
		m_nRState = 1;
		// 移動基点との差分を論理座標原点に加算
		dc.SetWindowOrg(dc.GetWindowOrg() + (m_ptMovOrg - ptLog));
		Invalidate();
		UpdateWindow();
		m_ptMouse = pt;
	}

	return TRUE;	// 矩形表示を処理した
}

BOOL CViewBase::OnMouseWheel(UINT nFlags, short zDelta, const CPoint& pt)
{
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	if ( !pOpt->IsMouseWheel() )
		return FALSE;

	if ( zDelta <= -WHEEL_DELTA ) {
		if ( pOpt->GetWheelType() == 0 )
			OnViewLensP();
		else
			OnViewLensN();
	}
	else if ( zDelta >= WHEEL_DELTA ) {
		if ( pOpt->GetWheelType() == 0 )
			OnViewLensN();
		else
			OnViewLensP();
	}

	return TRUE;
}

void CViewBase::OnContextMenu(CPoint pt, UINT nID)
{
	CMenu	menu;
	menu.LoadMenu(nID);
	CMenu*	pMenu = menu.GetSubMenu(0);
	pMenu->TrackPopupMenu(TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_RIGHTBUTTON,
		pt.x, pt.y, AfxGetMainWnd());
}

BOOL CViewBase::OnEraseBkgnd(CDC* pDC, COLORREF col1, COLORREF col2) 
{
	CRect	rc;
	GetClientRect(rc);
	pDC->DPtoLP(&rc);

	if ( col1 == col2 ) {
		pDC->FillSolidRect(&rc, col1);
	}
	else {
		TRIVERTEX	tv[2];
		tv[0].x = rc.left;	tv[0].y = rc.top;
		tv[1].x = rc.right;	tv[1].y = rc.bottom;
		tv[0].Red	= (COLOR16)(GetRValue(col1) << 8);
		tv[0].Green	= (COLOR16)(GetGValue(col1) << 8);
		tv[0].Blue	= (COLOR16)(GetBValue(col1) << 8);
		tv[0].Alpha	= 0;
		tv[1].Red	= (COLOR16)(GetRValue(col2) << 8);
		tv[1].Green	= (COLOR16)(GetGValue(col2) << 8);
		tv[1].Blue	= (COLOR16)(GetBValue(col2) << 8);
		tv[1].Alpha	= 0;
		GRADIENT_RECT	rcGrad;
		rcGrad.UpperLeft	= 0;
		rcGrad.LowerRight	= 1;
		VERIFY( pDC->GradientFill(tv, 2, &rcGrad, 1, GRADIENT_FILL_RECT_V) );
	}

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CViewBase クラスの診断

#ifdef _DEBUG
void CViewBase::AssertValid() const
{
	__super::AssertValid();
}

void CViewBase::Dump(CDumpContext& dc) const
{
	__super::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////

void CViewBase::CopyNCDrawForClipboard(HENHMETAFILE hEmf)
{
	// ｸﾘｯﾌﾟﾎﾞｰﾄﾞへのﾃﾞｰﾀｺﾋﾟｰ
	if ( !OpenClipboard() ) {
		::DeleteEnhMetaFile(hEmf);
		AfxMessageBox(IDS_ERR_CLIPBOARD, MB_OK|MB_ICONEXCLAMATION);
		return;
	}
	if ( !::EmptyClipboard() ) {
		::DeleteEnhMetaFile(hEmf);
		::CloseClipboard();
		AfxMessageBox(IDS_ERR_CLIPBOARD, MB_OK|MB_ICONEXCLAMATION);
		return;
	}
	if ( !::SetClipboardData(CF_ENHMETAFILE, hEmf) ) {
		::DeleteEnhMetaFile(hEmf);
		AfxMessageBox(IDS_ERR_CLIPBOARD, MB_OK|MB_ICONEXCLAMATION);
	}

	::CloseClipboard();
}
