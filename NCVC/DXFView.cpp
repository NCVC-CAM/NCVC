// DXFView.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "DXFChild.h"
#include "DXFdata.h"
#include "DXFshape.h"
#include "DXFDoc.h"
#include "DXFView.h"
#include "DXFShapeView.h"
#include "LayerDlg.h"
#include "ViewOption.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using std::vector;
using namespace boost;

// 指定座標との閾値
static	const	float	SELECTGAP = 5.0f;
//
#define	IsBindMode()	GetDocument()->IsDocFlag(DXFDOC_BIND)
#define	IsBindParent()	GetDocument()->IsDocFlag(DXFDOC_BINDPARENT)

/////////////////////////////////////////////////////////////////////////////
// CDXFView

IMPLEMENT_DYNCREATE(CDXFView, CViewBase)

BEGIN_MESSAGE_MAP(CDXFView, CViewBase)
	//{{AFX_MSG_MAP(CDXFView)
	ON_WM_DESTROY()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
#ifdef _DEBUG
	ON_WM_RBUTTONDBLCLK()
#endif
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_KEYDOWN()
	ON_WM_CONTEXTMENU()
	ON_WM_ERASEBKGND()
	ON_WM_NCHITTEST()
	ON_COMMAND(ID_DXFVIEW_LAYER, &CDXFView::OnViewLayer)
	ON_UPDATE_COMMAND_UI(ID_DXFVIEW_LAYER, &CDXFView::OnUpdateViewLayer)
	ON_COMMAND(ID_EDIT_UNDO, &CDXFView::OnEditUndo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, &CDXFView::OnUpdateEditUndo)
	ON_COMMAND(ID_EDIT_COPY, &CDXFView::OnEditCopy)
	//}}AFX_MSG_MAP
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXT, 0, 0xFFFF, &CDXFView::OnToolTipNotify)
	// ﾕｰｻﾞｲﾆｼｬﾙ処理 & 各ﾋﾞｭｰへのﾌｨｯﾄﾒｯｾｰｼﾞ
	ON_MESSAGE(WM_USERVIEWFITMSG, &CDXFView::OnUserViewFitMsg)
	// ﾏｳｽ移動のｽﾃｰﾀｽﾊﾞｰ更新
	ON_UPDATE_COMMAND_UI(ID_DXFST_MOUSE, &CDXFView::OnUpdateMouseCursor)
	// CADﾃﾞｰﾀの統合
	ON_UPDATE_COMMAND_UI(ID_EDIT_BIND_DEL, &CDXFView::OnUpdateEditBind)
	ON_UPDATE_COMMAND_UI(ID_EDIT_BIND_TARGET, &CDXFView::OnUpdateEditBind)
	ON_COMMAND(ID_EDIT_BIND_DEL, &CDXFView::OnEditBindDel)
	ON_COMMAND(ID_EDIT_BIND_TARGET, &CDXFView::OnEditBindTarget)
	ON_MESSAGE(WM_USERBINDINIT, &CDXFView::OnBindInitMsg)
	ON_MESSAGE(WM_USERBIND_LDOWN, &CDXFView::OnBindLButtonDown)
	ON_MESSAGE(WM_USERBIND_ROUND, &CDXFView::OnBindRoundMsg)
	ON_MESSAGE(WM_USERBIND_CANCEL, &CDXFView::OnBindCancel)
	// 移動
	ON_COMMAND_RANGE(ID_VIEW_UP, ID_VIEW_RT, &CDXFView::OnMoveKey)
	ON_COMMAND_RANGE(ID_VIEW_FIT, ID_VIEW_LENSN, &CDXFView::OnLensKey)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDXFView クラスの構築/消滅

CDXFView::CDXFView()
{
	m_nSelect = -1;
	m_pSelData = NULL;
//	m_pToolTip = NULL;
	m_enBind = BD_NONE;
}

CDXFView::~CDXFView()
{
	PLIST_FOREACH(auto p, &m_bindUndo)
		delete	p;
	END_FOREACH
}

/////////////////////////////////////////////////////////////////////////////
// CDXFView クラスのオーバライド関数

BOOL CDXFView::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	if ( IsBindMode() ) {
		CView*		pViewParent = static_cast<CView *>(GetParent());
		ASSERT( pViewParent );
		return pViewParent->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
	}
	return __super::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}
/*
BOOL CDXFView::PreTranslateMessage(MSG* pMsg)
{
	if ( m_pToolTip )
		m_pToolTip->RelayEvent(pMsg);

	return __super::PreTranslateMessage(pMsg);
}
*/
void CDXFView::OnInitialUpdate() 
{
	__super::OnInitialUpdate();


	if ( IsBindMode() ) {
/*
		// ﾂｰﾙﾁｯﾌﾟ準備
		m_pToolTip = new CToolTipCtrl;
//		m_pToolTip = new CMFCToolTipCtrl;
		m_pToolTip->Create(this);

//		m_pToolTip->AddTool(this);	// この内部でﾒﾓﾘﾘｰｸの要因があるらしい??

		TOOLINFO	ti;
		memset(&ti, 0, sizeof(TOOLINFO));
		HWND	hwnd = GetSafeHwnd();
		ti.cbSize	= sizeof(TOOLINFO);
		ti.uFlags	= TTF_IDISHWND;
		ti.hwnd		= ::GetParent(hwnd);
		ti.uId		= (UINT_PTR)hwnd;
		BOOL bResult = m_pToolTip->SendMessage(TTM_ADDTOOL, 0, (LPARAM)&ti);

		m_pToolTip->SetDelayTime(1000);
		m_pToolTip->UpdateTipText(GetDocument()->GetTitle(), this);
//		m_pToolTip->SetDescription(GetDocument()->GetTitle());
*/
		EnableToolTips(TRUE);
	}
	else {
		if ( !IsBindParent() ) {
			// MDI子ﾌﾚｰﾑのｽﾃｰﾀｽﾊﾞｰに情報表示
			// -- IsBindParent()もこの時点では子のDataCntが済んでいない
			// -- OnBindInitMsg()で実行
			static_cast<CDXFChild*>(GetParentFrame())->SetDataInfo(
				static_cast<CDXFDoc*>(GetDocument()) );
		}
		// ｼﾘｱﾙ化後の図形ﾌｨｯﾄﾒｯｾｰｼﾞの送信
		// OnInitialUpdate()関数内では，GetClientRect()のｻｲｽﾞが正しくない
		if ( GetDocument()->IsDocFlag(DXFDOC_SHAPE) )
			GetParentFrame()->PostMessage(WM_USERINITIALUPDATE);
		else
			PostMessage(WM_USERVIEWFITMSG);
	}
}

void CDXFView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	switch ( lHint ) {
	case UAV_FILESAVE:
	case UAV_DXFAUTODELWORKING:
		return;		// 再描画不要
	case UAV_DXFAUTOWORKING:
		GetDocument()->AllChangeFactor(m_dFactor);	// 自動加工指示の拡大率調整
		break;
	case UAV_DXFORGUPDATE:
		ASSERT( pHint );
		reinterpret_cast<CDXFcircleEx*>(pHint)->DrawTuning(m_dFactor);
		if ( IsBindParent() ) {
			// 子の加工原点も更新
			CClientDC	dc(this);
			optional<CPointF>	ptOrg = GetDocument()->GetCutterOrigin();
			ASSERT( ptOrg );
			CPoint	ptDev = *ptOrg * m_dFactor;
			dc.LPtoDP(&ptDev);
			ClientToScreen(&ptDev);
			for ( int i=0; i<GetDocument()->GetBindInfoCnt(); i++ )
				BindMsgPost(&dc, GetDocument()->GetBindInfoData(i), &ptDev);
		}
		break;
	case UAV_DXFSHAPEID:	// from CDXFDoc::OnShapePattern()
		CancelForSelect();
		return;
	case UAV_DXFSHAPEUPDATE:
		if ( pHint ) {		// from CDXFShapeView::OnSelChanged()
			if ( !OnUpdateShape( reinterpret_cast<DXFTREETYPE*>(pHint) ) )
				return;	// 再描画不要
		}
		break;
	case UAV_ADDINREDRAW:
		OnViewLensComm();
		return;
	}
	CView::OnUpdate(pSender, lHint, pHint);
}

#ifdef _DEBUG
CDXFDoc* CDXFView::GetDocument() // 非デバッグ バージョンはインラインです。
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CDXFDoc)));
	return static_cast<CDXFDoc *>(m_pDocument);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CDXFView クラスのメンバ関数

BOOL CDXFView::OnUpdateShape(DXFTREETYPE vSelect[])
{
#ifdef _DEBUG
	printf("CDXFView::OnUpdateShape Start\n");
#endif
	CClientDC	dc(this);
	// 仮加工指示を描画中なら
	CancelForSelect(&dc);

	// 選択された形状ﾂﾘｰの解析
	CDXFshape*		pShape[] = {NULL, NULL};
	CDXFworking*	pWork[]  = {NULL, NULL};
	int		i;
	BOOL	bReDraw = FALSE;

	for ( i=0; i<SIZEOF(pShape); i++ ) {
		switch ( vSelect[i].which() ) {
		case DXFTREETYPE_MUSTER:
		case DXFTREETYPE_LAYER:
			bReDraw = TRUE;
			break;
		case DXFTREETYPE_SHAPE:
			pShape[i] = get<CDXFshape*>(vSelect[i]);
			break;
		case DXFTREETYPE_WORKING:
			pWork[i] = get<CDXFworking*>(vSelect[i]);
			break;
		}
	}
	m_vSelect = vSelect[1];

	// 形状の描画
	if ( !bReDraw ) {
		for ( i=0; i<SIZEOF(pShape); i++ ) {
			if ( pShape[i] )
				pShape[i]->DrawShape(&dc);
		}
		CPen* pOldPen = (CPen *)dc.SelectStockObject(NULL_PEN);
		dc.SetROP2(R2_COPYPEN);
		for ( i=0; i<SIZEOF(pWork); i++ ) {
			if ( pWork[i] ) {
				dc.SelectObject( i==0 ?
					AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_OUTLINE) :
					AfxGetNCVCMainWnd()->GetPenCom(COMPEN_SEL) );
				pWork[i]->Draw(&dc);
			}
		}
		dc.SelectObject(pOldPen);
	}

	return bReDraw;	// 再描画
}

BOOL CDXFView::IsRootTree(DWORD dwObject)
{
	return  dwObject==ROOTTREE_SHAPE ||
			dwObject==ROOTTREE_LOCUS ||
			dwObject==ROOTTREE_EXCLUDE;
}

CDXFworking* CDXFView::CreateWorkingData(void)
{
	ASSERT( m_vSelect.which()==DXFTREETYPE_SHAPE );
	ASSERT( m_nSelect >= 0 );
	int		n = 1 - m_nSelect,	// 1->0, 0->1
			nInOut = -1;		// 輪郭指示のみ
	CDXFshape*		pShape = get<CDXFshape*>(m_vSelect);
	CDXFworking*	pWork = NULL;

	try {
		switch ( GetDocument()->GetShapeProcessID() ) {
		case ID_EDIT_SHAPE_VEC:
			pWork = new CDXFworkingDirection(pShape, m_pSelData,
							m_ptArraw[n][1], m_ptArraw[m_nSelect]);
			break;
		case ID_EDIT_SHAPE_START:
			pWork = new CDXFworkingStart(pShape, m_pSelData, m_ptStart[m_nSelect]);
			break;
		case ID_EDIT_SHAPE_OUT:
			if ( !m_ltOutline[m_nSelect].IsEmpty() )
				pWork = new CDXFworkingOutline(pShape, &m_ltOutline[m_nSelect], m_dOffset);
			// m_nSelect分はCDXFworkingOutlineのﾃﾞｽﾄﾗｸﾀにてdelete
			nInOut = n;		// 内外どちらへ指示したか記録しておく
			PLIST_FOREACH(CDXFdata* pData, &m_ltOutline[n])
				if ( pData )
					delete	pData;
			END_FOREACH
			m_ltOutline[0].RemoveAll();
			m_ltOutline[1].RemoveAll();
			break;
//		case ID_EDIT_SHAPE_POC:
//			pWork = new CDXFworkingPocket(pShape);
//			break;
		}
		if ( pWork ) {
			// 加工情報の登録
			if ( nInOut < 0 )
				pShape->AddWorkingData(pWork);
			else
				pShape->AddOutlineData(static_cast<CDXFworkingOutline*>(pWork), nInOut);
			// 形状ﾂﾘｰ(加工指示)の更新と選択
			GetDocument()->UpdateAllViews(this, UAV_DXFADDWORKING, pWork);
			// ﾄﾞｷｭﾒﾝﾄ変更通知
			GetDocument()->SetModifiedFlag();
		}
	}
	catch ( CMemoryException* e ) {
		if ( pWork )
			delete	pWork;
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		return NULL;
	}

	return pWork;
}

BOOL CDXFView::CreateOutlineTempObject(CDXFshape* pShape)
{
	int			i, nError = 0;
	CDXFworkingOutline*	pOutline = pShape->GetOutlineLastObj();

	// ｵﾌｾｯﾄ値の決定
	m_dOffset = pShape->GetOffset();	// ﾍﾞｰｽ値
	if ( pOutline )
		m_dOffset += pOutline->GetOutlineOffset();

#ifdef _DEBUG
	printf("CreateOutlineTempObject()\n");
	printf("ShapeName=%s Orig\n", LPCTSTR(pShape->GetShapeName()));
	CRect	rc( pShape->GetMaxRect() );
	printf("(%d, %d)-(%d, %d)\n", rc.left, rc.top, rc.right, rc.bottom);
#endif

	DeleteOutlineTempObject();

	try {
		for ( i=0; i<SIZEOF(m_ltOutline); i++ ) {
			if ( !pShape->CreateOutlineTempObject(i, &m_ltOutline[i], m_dOffset) ) {
				nError++;	// ｴﾗｰｶｳﾝﾄ
				PLIST_FOREACH(CDXFdata* pData, &m_ltOutline[i])
					if ( pData )
						delete	pData;
				END_FOREACH
				m_ltOutline[i].RemoveAll();
#ifdef _DEBUG
				printf("Loop%d Error\n", i);
#endif
			}
#ifdef _DEBUG
			else {
				printf("Loop%d\n", i);
				rc = m_ltOutline[i].GetMaxRect();
				printf("(%d, %d)-(%d, %d)\n", rc.left, rc.top, rc.right, rc.bottom);
			}
#endif
		}
	}
	catch ( CMemoryException* e ) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		nError = 2;		// FALSE
	}

	return nError < SIZEOF(m_ltOutline);	// ならTRUE
}

void CDXFView::DeleteOutlineTempObject(void)
{
	for ( int i=0; i<SIZEOF(m_ltOutline); i++ ) {
		PLIST_FOREACH(CDXFdata* pData, &m_ltOutline[i])
			if ( pData )
				delete	pData;
		END_FOREACH
		m_ltOutline[i].RemoveAll();
	}
}

BOOL CDXFView::CancelForSelect(CDC* pDC/*=NULL*/)
{
	if ( m_pSelData && m_nSelect>=0 ) {
		if ( GetDocument()->GetShapeProcessID() == ID_EDIT_SHAPE_SEL )
			m_pSelData->SetDxfFlg(DXFFLG_SELECT);	// 選択状態を元に戻す
		DrawTemporaryProcess(pDC ? pDC : &CClientDC(this));
	}
	BOOL	bResult = m_pSelData ? TRUE : FALSE;
	m_nSelect = -1;
	m_pSelData = NULL;
	DeleteOutlineTempObject();

	return bResult;
}

void CDXFView::AllChangeFactor_OutlineTempObject(void)
{
	for ( int i=0; i<SIZEOF(m_ltOutline); i++ ) {
		PLIST_FOREACH(CDXFdata* pData, &m_ltOutline[i])
			if ( pData )
				pData->DrawTuning(m_dFactor);
		END_FOREACH
	}
}

void CDXFView::BindMove(BOOL bFitMsg)
{
	CClientDC		dc(this);
	LPCADBINDINFO	pInfo;
	CPoint			pt;
	CRectF			rc;
	CSize			sz;

	for ( int i=0; i<GetDocument()->GetBindInfoCnt(); i++ ) {
		pInfo = GetDocument()->GetBindInfoData(i);
		pt = pInfo->pt * m_dFactor;
		dc.LPtoDP(&pt);
		rc = pInfo->pDoc->GetMaxRect();
		sz.cx = (int)(rc.Width()  * m_dFactor);
		sz.cy = (int)(rc.Height() * m_dFactor);
		dc.LPtoDP(&sz);
//		pInfo->pView->MoveWindow(pt.x, pt.y, sz.cx, sz.cy);
		pInfo->pView->SetWindowPos(NULL, pt.x, pt.y, sz.cx, sz.cy,
			SWP_NOZORDER|SWP_NOACTIVATE);
		if ( bFitMsg )
			pInfo->pView->SendMessage(WM_USERVIEWFITMSG);
	}
}

void CDXFView::BindMsgPost(CDC* pDC, LPCADBINDINFO pInfo, CPoint* ptDev)
{
	// 位置記憶とﾃﾞﾊﾞｲｽ座標への変換
	CPoint	pt(pInfo->pt*m_dFactor);
	pDC->LPtoDP(&pt);

	// ﾃﾞｰﾀの最大矩形をﾃﾞﾊﾞｲｽ座標に変換
	CRectF	rc(pInfo->pDoc->GetMaxRect());
	CSize	sz((int)(rc.Width()*m_dFactor), (int)(rc.Height()*m_dFactor));
	pDC->LPtoDP(&sz);

	// 配置
//	pInfo->pView->MoveWindow(pt.x, pt.y, sz.cx, sz.cy);
	pInfo->pView->SetWindowPos(NULL, pt.x, pt.y, sz.cx, sz.cy,
		SWP_NOZORDER|SWP_NOACTIVATE);

	// 配置座標をｽｸﾘｰﾝ座標で子ｳｨﾝﾄﾞｳに通知
	ClientToScreen(&pt);
	CPointF	ptOffset(pt);
	pInfo->pView->SendMessage(WM_USERVIEWFITMSG,
			reinterpret_cast<WPARAM>(ptDev),
			reinterpret_cast<LPARAM>(&ptOffset));
	// pt に子ｳｨﾝﾄﾞｳの描画原点ｵﾌｾｯﾄが返る
	pInfo->ptOffset = ptOffset;
}

BOOL CDXFView::IsBindSelected(int nSel)
{
	// Boost::Lambda でメンバ変数にアクセスする書き方
	auto f = find_if(m_bindSel, &lambda::_1 ->* &SELECTBIND::nSel == nSel);
	return f==m_bindSel.end() ? FALSE : TRUE;
}

void CDXFView::ClearBindSelectData(void)
{
	LPCADBINDINFO	pInfo;
	for ( const auto& e : m_bindSel ) {
		pInfo = GetDocument()->GetBindInfoData(e.nSel);
		pInfo->pDoc->AllSetDxfFlg(DXFFLG_SELECT, FALSE);
		pInfo->pView->Invalidate();
	}
	m_bindSel.clear();
	m_enBind = BD_NONE;
}

/////////////////////////////////////////////////////////////////////////////
// CDXFView 描画

void CDXFView::OnDraw(CDC* pDC)
{
	ASSERT_VALID(GetDocument());
	INT_PTR			i, j, nLayerCnt, nDataCnt;
	DWORD			dwSel, dwSelBak = 0;
	CLayerData*		pLayer;
	CDXFdata*		pData;
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();

	pDC->SetROP2(R2_COPYPEN);
	pDC->SetTextAlign(TA_LEFT|TA_BOTTOM);
	CFont*	pOldFont = pDC->SelectObject(AfxGetNCVCMainWnd()->GetTextFont(TYPE_DXF));
	CBrush* pOldBrush = (CBrush *)pDC->SelectStockObject(NULL_BRUSH);
	CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_ORIGIN));

	if ( !IsBindMode() ) {
		// 原点描画
		if ( pData=GetDocument()->GetCircleObject() )
			pData->Draw(pDC);	// CDXFcircleEx::Draw()
		// 旋盤原点
		for ( i=0; i<2; i++ ) {
			if ( pData=GetDocument()->GetLatheLine(i) )
				pData->Draw(pDC);	// CDXFline::Draw()
		}
	}

	// DXF描画
	nLayerCnt = GetDocument()->GetLayerCnt();
	for ( i=0; i<nLayerCnt; i++ ) {
		pLayer = GetDocument()->GetLayerData(i);
		if ( !pLayer->IsLayerFlag(LAYER_VIEW) )
			continue;
		switch ( pLayer->GetLayerType() ) {
		case DXFCAMLAYER:
			pDC->SelectStockObject(NULL_BRUSH);
			pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_CUTTER));
			nDataCnt = pLayer->GetDxfSize();
			for ( j=0; j<nDataCnt; j++ ) {
				pData = pLayer->GetDxfData(j);
				dwSel = pData->GetDxfFlg() & DXFFLG_SELECT;
				if ( dwSel != dwSelBak ) {
					dwSelBak = dwSel;
					pDC->SelectObject(pData->GetDrawPen());
				}
				pData->Draw(pDC);
			}
			pDC->SetTextColor(pOpt->GetDxfDrawColor(DXFCOL_CUTTER));
			pDC->SelectObject(AfxGetNCVCMainWnd()->GetBrushDXF(DXFBRUSH_CUTTER));
			nDataCnt = pLayer->GetDxfTextSize();
			for ( j=0; j<nDataCnt; j++ )
				pLayer->GetDxfTextData(j)->Draw(pDC);
			break;
		case DXFSTRLAYER:
			pDC->SelectStockObject(NULL_BRUSH);
			pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_START));
			nDataCnt = pLayer->GetDxfSize();
			for ( j=0; j<nDataCnt; j++ )
				pLayer->GetDxfData(j)->Draw(pDC);
			pDC->SetTextColor(pOpt->GetDxfDrawColor(DXFCOL_START));
			pDC->SelectObject(AfxGetNCVCMainWnd()->GetBrushDXF(DXFBRUSH_START));
			nDataCnt = pLayer->GetDxfTextSize();
			for ( j=0; j<nDataCnt; j++ )
				pLayer->GetDxfTextData(j)->Draw(pDC);
			break;
		case DXFMOVLAYER:
			pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_MOVE));
			pDC->SelectStockObject(NULL_BRUSH);
			nDataCnt = pLayer->GetDxfSize();
			for ( j=0; j<nDataCnt; j++ )
				pLayer->GetDxfData(j)->Draw(pDC);
			pDC->SetTextColor(pOpt->GetDxfDrawColor(DXFCOL_MOVE));
			pDC->SelectObject(AfxGetNCVCMainWnd()->GetBrushDXF(DXFBRUSH_MOVE));
			nDataCnt = pLayer->GetDxfTextSize();
			for ( j=0; j<nDataCnt; j++ )
				pLayer->GetDxfTextData(j)->Draw(pDC);
			break;
		case DXFCOMLAYER:
			pDC->SetTextColor(pOpt->GetDxfDrawColor(DXFCOL_TEXT));
			pDC->SelectObject(AfxGetNCVCMainWnd()->GetBrushDXF(DXFBRUSH_TEXT));
			pDC->SelectStockObject(NULL_PEN);
			nDataCnt = pLayer->GetDxfTextSize();
			for ( j=0; j<nDataCnt; j++ )
				pLayer->GetDxfTextData(j)->Draw(pDC);
			break;
		}
		// 加工指示の描画
		pLayer->DrawWorking(pDC);
	}

	// 現在の拡大矩形を描画(ViewBase.cpp)
	if ( m_bMagRect )
		DrawMagnifyRect(pDC);
	// 仮加工指示の描画
	if ( m_pSelData && m_nSelect>=0 )
		DrawTemporaryProcess(pDC);
	// ﾜｰｸ矩形(bind)
	if ( IsBindParent() ) {
		pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_WORK));
		pDC->Rectangle(m_rcDrawWork);
	}
	else if ( IsBindMode() ) {
		CDXFView*	pView  = static_cast<CDXFView*>(GetParent());
		ASSERT(pView);
		int n = pView->GetDocument()->GetBindInfo_fromView(this);
		if ( n>=0 && !pView->GetDocument()->GetBindInfoData(n)->bTarget ) {
			CRect	rc;
			GetClientRect(&rc);
			pDC->DPtoLP(&rc);
			pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenCom(COMPEN_SEL));
			pDC->MoveTo(rc.TopLeft());
			pDC->LineTo(rc.BottomRight());
			pDC->MoveTo(rc.left, rc.bottom);
			pDC->LineTo(rc.right,rc.top);
		}
	}

	pDC->SelectObject(pOldPen);
	pDC->SelectObject(pOldBrush);
	pDC->SelectObject(pOldFont);
}

void CDXFView::DrawTemporaryProcess(CDC* pDC)
{
	switch ( GetDocument()->GetShapeProcessID() ) {
	case ID_EDIT_SHAPE_SEL:
		{
			CPen*	pOldPen = pDC->SelectObject(m_pSelData->GetDrawPen());
			pDC->SetROP2(R2_COPYPEN);
			m_pSelData->Draw(pDC);
			pDC->SelectObject(pOldPen);
		}
		break;
	case ID_EDIT_SHAPE_VEC:
		DrawTempArraw(pDC);
		break;
	case ID_EDIT_SHAPE_START:
		DrawTempStart(pDC);
		break;
	case ID_EDIT_SHAPE_OUT:
		DrawTempOutline(pDC);
		break;
	}
}

void CDXFView::DrawTempArraw(CDC* pDC)
{
	ASSERT( 0<=m_nSelect && m_nSelect<=SIZEOF(m_ptArraw) );

	// 矢印の長さが拡大率に影響されないように計算
	CPoint	ptDraw[SIZEOF(m_ptArraw)][SIZEOF(m_ptArraw[0])];

	for ( int i=0; i<SIZEOF(m_ptArraw); i++ ) {
		ptDraw[i][1] = m_ptArraw[i][1] * m_dFactor;
		ptDraw[i][0] = m_ptArraw[i][0] + ptDraw[i][1];
		ptDraw[i][2] = m_ptArraw[i][2] + ptDraw[i][1];
	}
	CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_OUTLINE));
	pDC->SetROP2(R2_XORPEN);
	pDC->Polyline(ptDraw[m_nSelect], SIZEOF(ptDraw[0]));
	pDC->SelectObject(pOldPen);
}

void CDXFView::DrawTempStart(CDC* pDC)
{
	ASSERT( 0<=m_nSelect && m_nSelect<=SIZEOF(m_ptStart) );

	// CDXFpointに準拠
	CRect	rcDraw;
	CPointF	pt( m_ptStart[m_nSelect] * m_dFactor );
	// 位置を表す丸印は常に2.5論理単位
	float	dFactor = LOMETRICFACTOR * 2.5;
	rcDraw.TopLeft()		= pt - dFactor;
	rcDraw.BottomRight()	= pt + dFactor;

	CBrush* pOldBrush = (CBrush *)pDC->SelectStockObject(NULL_BRUSH);
	CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_OUTLINE));
	pDC->SetROP2(R2_XORPEN);
	pDC->Ellipse(&rcDraw);
	pDC->SelectObject(pOldPen);
	pDC->SelectObject(pOldBrush);
}

void CDXFView::DrawTempOutline(CDC* pDC)
{
	ASSERT( 0<=m_nSelect && m_nSelect<=SIZEOF(m_ltOutline) );

	if ( !m_ltOutline[m_nSelect].IsEmpty() ) {
		CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_OUTLINE));
		CBrush* pOldBrush = (CBrush *)pDC->SelectStockObject(NULL_BRUSH);
		pDC->SetROP2(R2_XORPEN);
		PLIST_FOREACH(CDXFdata* pData, &m_ltOutline[m_nSelect])
			if ( pData )
				pData->Draw(pDC);
		END_FOREACH
		pDC->SelectObject(pOldPen);
		pDC->SelectObject(pOldBrush);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CDXFView クラスのメッセージ ハンドラ（メニュー編）

void CDXFView::OnUpdateEditUndo(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( !m_bindUndo.IsEmpty() );
}

void CDXFView::OnEditUndo() 
{
	LPCADBINDINFO	pInfo, pUndo = m_bindUndo.GetTail();
	int		n = GetDocument()->GetBindInfo_fromView(pUndo->pView);
	pInfo = GetDocument()->GetBindInfoData(n);
	pInfo->pt = pUndo->pt;

	CClientDC	dc(this);
	optional<CPointF>	ptOrg = GetDocument()->GetCutterOrigin();
	ASSERT( ptOrg );
	CPoint	ptDev = *ptOrg * m_dFactor;
	dc.LPtoDP(&ptDev);
	ClientToScreen(&ptDev);
	BindMsgPost(&dc, pInfo, &ptDev);
	Invalidate();

	delete	pUndo;
	m_bindUndo.RemoveTail();
}

void CDXFView::OnEditCopy() 
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

void CDXFView::OnUpdateEditBind(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( !m_bindSel.empty() );
}

void CDXFView::OnEditBindDel()
{
	if ( !m_bindSel.empty() ) {
		// 降順に並べ替えてから削除（しないとASSERTｴﾗｰ）
//		std::sort(m_bindSel.begin(), m_bindSel.end());
		boost::sort(m_bindSel);		// boost/range/algorithm.hpp
		for ( const auto& e : m_bindSel )
			GetDocument()->RemoveBindData(e.nSel);
		Invalidate();
		GetDocument()->SetModifiedFlag();
		// 後片付け
		m_bindSel.clear();
		m_enBind = BD_NONE;
	}
}

void CDXFView::OnEditBindTarget()
{
	if ( !m_bindSel.empty() ) {
		LPCADBINDINFO pInfo;
		for ( const auto& e : m_bindSel ) {
			pInfo = GetDocument()->GetBindInfoData(e.nSel);
			pInfo->bTarget = !pInfo->bTarget;
		}
		Invalidate();
		GetDocument()->SetModifiedFlag();
	}
}

void CDXFView::OnUpdateViewLayer(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck( AfxGetNCVCMainWnd()->GetModelessDlg(MLD_DXFLAYER) != NULL );
}

void CDXFView::OnViewLayer() 
{
	if ( AfxGetNCVCMainWnd()->GetModelessDlg(MLD_DXFLAYER) ) {
		// CLayerDlg::OnCancel() の間接呼び出し
		AfxGetNCVCMainWnd()->GetModelessDlg(MLD_DXFLAYER)->PostMessage(WM_CLOSE);
		return;
	}
	CLayerDlg*	pDlg = new CLayerDlg;
	pDlg->Create(IDD_DXFVIEW_LAYER);
	AfxGetNCVCMainWnd()->SetModelessDlg(MLD_DXFLAYER, pDlg);
}

void CDXFView::OnUpdateMouseCursor(CCmdUI* pCmdUI) 
{
	if ( !IsBindMode() ) {
		CFrameWnd*	pChild = AfxGetNCVCMainWnd()->GetActiveFrame();
		CView*		pView  = pChild ? pChild->GetActiveView() : NULL;
		optional<CPointF>	ptOrg = GetDocument()->GetCutterOrigin();
		if ( pView==this && ptOrg ) {
			POINT	pt;
			::GetCursorPos(&pt);
			ScreenToClient(&pt);
			CClientDC	dc(this);
			dc.DPtoLP(&pt);
			CPointF	ptd( pt.x/m_dFactor, pt.y/m_dFactor );
			// 原点からの距離
			ptd -= *ptOrg;
			static_cast<CDXFChild *>(GetParentFrame())->OnUpdateMouseCursor(&ptd);
		}
		else
			static_cast<CDXFChild *>(GetParentFrame())->OnUpdateMouseCursor();
	}
}

void CDXFView::OnMoveKey(UINT nID) 
{
	if ( IsBindMode() ) {
		// 移動中など子ｳｨﾝﾄﾞｳがﾌｫｰｶｽを持っている場合がある
		CDXFView*	pView  = static_cast<CDXFView*>(GetParent());
		ASSERT(pView);
		pView->SendMessage(WM_COMMAND, nID);
		return;
	}

	CViewBase::OnMoveKey(nID);
	if ( IsBindParent() ) {
		// 子ｳｨﾝﾄﾞｳ移動
		BindMove(FALSE);
	}
}

void CDXFView::OnLensKey(UINT nID)
{
	if ( IsBindMode() ) {
		CDXFView*	pView  = static_cast<CDXFView*>(GetParent());
		ASSERT(pView);
		pView->SendMessage(WM_COMMAND, nID);
		return;
	}

	CRectF		rc;
	CDXFdata*	pData;

	switch ( nID ) {
	case ID_VIEW_FIT:
		rc = GetDocument()->GetMaxRect();
		if ( !IsBindMode() ) {
			// 原点補間
			pData = GetDocument()->GetCircleObject();
			if ( pData )
				rc |= pData->GetMaxRect();
			if ( GetDocument()->IsDocFlag(DXFDOC_LATHE) ) {
				for ( int i=0; i<2; i++ ) {
					pData = GetDocument()->GetLatheLine(0);
					if ( pData )
						rc |= pData->GetMaxRect();
				}
			}
		}
		CViewBase::OnViewFit(rc);
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

void CDXFView::OnViewLensComm(void)
{
	// 拡大率による描画情報の更新
	GetDocument()->AllChangeFactor(m_dFactor);
	// 一時ｵﾌﾞｼﾞｪｸﾄ分
	AllChangeFactor_OutlineTempObject();
	//
	if ( IsBindParent() ) {
		// ﾜｰｸ矩形
		m_rcDrawWork = DrawConvert(GetDocument()->GetMaxRect());
		// 統合ﾃﾞｰﾀの再配置
		BindMove(TRUE);
	}
	if ( !IsBindMode() ) {
		// MDI子ﾌﾚｰﾑのｽﾃｰﾀｽﾊﾞｰに情報表示
		static_cast<CDXFChild *>(GetParentFrame())->SetFactorInfo(m_dFactor/LOMETRICFACTOR);
	}
	// ﾋﾞｭｰの再描画
	Invalidate();
}

/////////////////////////////////////////////////////////////////////////////
// CDXFView メッセージ ハンドラ

void CDXFView::OnDestroy()
{
/*
	if ( m_pToolTip ) {
		m_pToolTip->Activate(FALSE);
		m_pToolTip->DelTool(this);
		m_pToolTip->DestroyWindow();
		delete	m_pToolTip;
	}
*/
	DeleteOutlineTempObject();
	__super::OnDestroy();
}

void CDXFView::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	CViewBase::_OnContextMenu(point, IsBindMode() ? IDR_DXFPOPUP3 : IDR_DXFPOPUP1);
}

LRESULT CDXFView::OnUserViewFitMsg(WPARAM wParam, LPARAM lParam)
{
	if ( IsBindMode() ) {
		CRectF	rc;
		rc = GetDocument()->GetMaxRect();
		CViewBase::OnViewFit(rc, FALSE);
		OnViewLensComm();
		CClientDC	dc(this);
		if ( wParam ) {
			// ｽｸﾘｰﾝ座標から加工原点を更新
			CPoint*	ptParam = reinterpret_cast<CPoint *>(wParam);
			CPoint	ptD(ptParam->x, ptParam->y);
			ScreenToClient(&ptD);
			dc.DPtoLP(&ptD);
			CPointF	ptL(ptD.x/m_dFactor, ptD.y/m_dFactor);
			GetDocument()->CreateCutterOrigin(ptL);
#ifdef _DEBUG
			printf("%s\n", LPCTSTR(GetDocument()->GetPathName()));
			printf("-- New Origin = %f, %f\n", ptL.x, ptL.y);
#endif
		}
		if ( lParam ) {
			// 配置座標のｵﾌｾｯﾄ計算
			CPointF* ptParam = reinterpret_cast<CPointF *>(lParam);
			CPoint	ptD((int)ptParam->x, (int)ptParam->y);
			ScreenToClient(&ptD);
			dc.DPtoLP(&ptD);
			ptParam->x = ptD.x / m_dFactor;
			ptParam->y = ptD.y / m_dFactor;
		}
	}
	else {
		OnLensKey(ID_VIEW_FIT);
	}
	return 0;
}

LRESULT CDXFView::OnBindInitMsg(WPARAM wParam, LPARAM)
{
#ifdef _DEBUG
	printf("CDXFView::OnBindInitMsg() Start\n");
#endif
	INT_PTR	i, nLoop = GetDocument()->GetBindInfoCnt();
	size_t	j;
	BOOL	bResult, bXover = FALSE;
	float	dNextBaseY, dMargin = AfxGetNCVCApp()->GetDXFOption()->GetBindMargin();
	CClientDC		dc(this);
	LPCADBINDINFO	pInfo;
	CRectF	rcWork( GetDocument()->GetMaxRect() ), rc;
	CPointF	pt1(0, rcWork.bottom), pt2;
	float	W = rcWork.Width(), w, h, dw, dh;

	// 子ﾃﾞｰﾀ加算後の表示更新
	static_cast<CDXFChild*>(GetParentFrame())->SetDataInfo(
		static_cast<CDXFDoc*>(GetDocument()) );
	if ( wParam )
		return 0;		// from CDXFDoc::OnOpenDocument()

	// 加工原点をｽｸﾘｰﾝ座標に変換
	optional<CPointF>	ptOrg = GetDocument()->GetCutterOrigin();
	ASSERT( ptOrg );
	CPoint	ptDev = *ptOrg * m_dFactor;
	dc.LPtoDP(&ptDev);
	ClientToScreen(&ptDev);
#ifdef _DEBUG
	printf("rcWork = (%f, %f) - (%f, %f)\n",
		rcWork.left, rcWork.top, rcWork.right, rcWork.bottom);
	printf("ptDev  = (%d, %d)\n", ptDev.x, ptDev.y);
#endif

	// --- 長方形詰め込み問題---
	std::vector<CVPointF>	YY;	// 配置可能なYの二次元可変配列
	CVPointF	Y;				// 配置点
	CVPointF::iterator	it;

	// 1つめ配置
	ASSERT( nLoop > 0 );
	pInfo = GetDocument()->GetBindInfoData(0);
	pInfo->pt = pt1;
	BindMsgPost(&dc, pInfo, &ptDev);
	rc = pInfo->pDoc->GetMaxRect();
	if ( W < rc.Width() )
		bXover = TRUE;
	pt1.x = rc.Width() + dMargin;
	// 配置点登録
	dNextBaseY = pt2.y = pt1.y - rc.Height() + dMargin;
	Y.push_back( pt2 );
#ifdef _DEBUG
	printf("first Y = (%f, %f)\n", pt2.x, pt2.y);
#endif

	// 2つめ以降
	for ( i=1; i<nLoop; i++ ) {
		pInfo = GetDocument()->GetBindInfoData(i);
		rc = pInfo->pDoc->GetMaxRect();
		w = rc.Width()  + dMargin;
		h = rc.Height() + dMargin;
#ifdef _DEBUG
		printf("w=%f h=%f\n", w, h);
#endif
		// 幅検索
		if ( pt1.x+w < W ) {
			pInfo->pt = pt1;
			BindMsgPost(&dc, pInfo, &ptDev);
			if ( pt1.y - Y.back().y > dMargin ) {
				pt2 = pt1;	pt2.y -= h;
				Y.push_back( pt2 );
#ifdef _DEBUG
				printf("(%f, %f)\n", pt2.x, pt2.y);
#endif
			}
			pt1.x += w;
			continue;
		}

		if ( !Y.empty() ) {
			YY.push_back(Y);
			Y.clear();
		}

		// すき間の高さ検索
		bResult = FALSE;
		for ( j=0; j<YY.size() && !bResult; j++ ) {
			for ( it=YY[j].begin()+1; it!=YY[j].end(); ++it ) {
				pt1 = *it;
				pt2 = *boost::prior(it);
				// すき間検索除外条件(次のﾃﾞｰﾀとX値が同じ)
				if ( fabs(pt1.x-pt2.x) < NCMIN )
					continue;
				dw = W - pt1.x;
				dh = pt1.y - pt2.y;
				if ( dh>=h && dw>=w ) {
					pInfo->pt = pt1;
					BindMsgPost(&dc, pInfo, &ptDev);
					// 配置点の登録
					if ( dh - h > dMargin ) {
						pt2 = pt1;	pt2.y -= h;
						YY[j].insert(it, pt2);
						Y.push_back( pt2 );
#ifdef _DEBUG
						printf("sukima (%f, %f)\n", pt2.x, pt2.y);
#endif
						pt1.x += w;
					}
					bResult = TRUE;
					break;
				}
			}
		}

		// 幅にもすき間にも入らない
		if ( !bResult ) {
			pt1.x = 0;
			pt1.y = dNextBaseY;
			pInfo->pt = pt1;
			BindMsgPost(&dc, pInfo, &ptDev);
			pt2 = pt1;	pt2.y -= h;
			Y.push_back( pt2 );
#ifdef _DEBUG
			printf("new (%f, %f)\n", pt2.x, pt2.y);
#endif
			if ( W < rc.Width() )
				bXover = TRUE;
			pt1.x += w;
			dNextBaseY = pt2.y;
		}

	}

	bResult = FALSE;
	for ( j=0; j<YY.size() && !bResult; j++ ) {
		for ( it=YY[j].begin(); it!=YY[j].end(); ++it ) {
			if ( it->y < 0 ) {
				bResult = TRUE;
				break;
			}
		}
	}
	if ( bResult || bXover )
		AfxMessageBox(IDS_ERR_BINDAREAOVER, MB_OK|MB_ICONEXCLAMATION);

	return 0;
}

LRESULT CDXFView::OnBindLButtonDown(WPARAM wParam, LPARAM lParam)
{
	LPCADBINDINFO	pInfo;
	// 選択ﾋﾞｭｰのｾｯﾄ
	SELECTBIND	sb;
	sb.nSel = (int)(wParam);

	if ( sb.nSel >= 0 ) {
		BOOL	bAdd = TRUE;
		pInfo = GetDocument()->GetBindInfoData(sb.nSel);
		// 既に登録されていれば選択解除
		for ( auto it=m_bindSel.begin(); it!=m_bindSel.end(); ++it ) {
			if ( (*it).nSel == sb.nSel ) {
				pInfo->pDoc->AllSetDxfFlg(DXFFLG_SELECT, FALSE);
				m_bindSel.erase(it);
				bAdd = FALSE;
				break;
			}
		}
		if ( bAdd ) {
			pInfo->pDoc->AllSetDxfFlg(DXFFLG_SELECT);
			m_bindSel.push_back(sb);
		}
	}
	m_enBind = m_bindSel.empty() ? BD_NONE : BD_SELECT;

	if ( lParam ) {
		// 親ﾋﾞｭｰのｸﾗｲｱﾝﾄ座標に変換 (m_ptMouse : ViewBase.h)
		m_ptMouse.x = reinterpret_cast<CPoint*>(lParam)->x;
		m_ptMouse.y = reinterpret_cast<CPoint*>(lParam)->y;
		ScreenToClient(&m_ptMouse);
		// ｸﾘｯｸﾎﾟｲﾝﾄと子ｳｨﾝﾄﾞｳの差分を計算
		for ( auto& e : m_bindSel ) {	// 内容更新のための"&"
			pInfo = GetDocument()->GetBindInfoData(e.nSel);
			pInfo->pView->GetWindowRect(&e.rc);
			ScreenToClient(&e.rc);
			e.ptDiff = m_ptMouse - e.rc.TopLeft();
		}
	}

	return 0;
}

LRESULT CDXFView::OnBindRoundMsg(WPARAM wParam, LPARAM lParam)
{
	// -- from OnLButtonDblClk()
	CClientDC	dc(this);
	float	w, h;
	LPCADBINDINFO pInfo = GetDocument()->GetBindInfoData(wParam);
	CRectF	rc(pInfo->pDoc->GetMaxRect());
	w = rc.Width()  / 2.0f;
	h = rc.Height() / 2.0f;
	CPointF	pto(pInfo->pt.x+w, pInfo->pt.y-h);	// 配置の中心

	pInfo->pDoc->AllRoundObjPoint(copysign(RAD(90.0f), (float)(int)lParam));

	// 新配置座標の計算と配置
	pInfo->pt.x = pto.x - h;
	pInfo->pt.y = pto.y + w;
	BindMsgPost(&dc, pInfo, NULL);

	GetDocument()->SetModifiedFlag();

	return 0;
}

LRESULT CDXFView::OnBindCancel(WPARAM, LPARAM)
{
#ifdef _DEBUG
	printf("CDXFView::OnBindCancel() Start\n");
#endif
	m_enBind = BD_CANCEL;
	// 配置を元に戻す
	CClientDC	dc(this);
	for ( const auto& e : m_bindSel )
		BindMsgPost(&dc, GetDocument()->GetBindInfoData(e.nSel), NULL);
	Invalidate();
	UpdateWindow();	// 即再描画

	return 0;
}

void CDXFView::OnLButtonDown(UINT nFlags, CPoint point)
{
#ifdef _DEBUG
	CString	strDbg;
	if ( IsBindParent() )
		strDbg = "P";
	else if ( IsBindMode() )
		strDbg = "C";
	strDbg = "CDXFView::OnLButtonDown("+strDbg+") Start\n";
	printf(LPCTSTR(strDbg));
#endif
	if ( IsBindMode() ) {
		CDXFView*	pView  = static_cast<CDXFView*>(GetParent());
		ASSERT(pView);
		int n = pView->GetDocument()->GetBindInfo_fromView(this);
		if ( n >= 0 ) {
			if ( !(nFlags & MK_CONTROL) ) {
				if ( pView->IsBindSelected(n) ) {
					// 自分の選択を解除しない
					n = -1;
				}
				else {
					// 現在の選択をｸﾘｱして自分を選択
					pView->ClearBindSelectData();
				}
			}
			// 子ｲﾝｽﾀﾝｽでもｸﾘｯｸﾎﾟｲﾝﾄの保存
			m_ptMouse = point;
			// 親へ通知
			ClientToScreen(&point);
			pView->SendMessage(WM_USERBIND_LDOWN, n,
					reinterpret_cast<LPARAM>(&point));
			Invalidate();
			UpdateWindow();
		}
		return;
	}
	else if ( !m_bindSel.empty() )
		ClearBindSelectData();

	CViewBase::OnLButtonDown(nFlags, point);
}

void CDXFView::OnLButtonUp(UINT nFlags, CPoint point) 
{
#ifdef _DEBUG
	CString	strDbg;
	if ( IsBindParent() )
		strDbg = "P";
	else if ( IsBindMode() )
		strDbg = "C";
	strDbg = "CDXFView::OnLButtonUp("+strDbg+") Start\n";
	printf(LPCTSTR(strDbg));
#endif
	if ( !m_bindSel.empty() && m_enBind==BD_MOVE ) {
		optional<CPointF> ptOrg = GetDocument()->GetCutterOrigin();
		CClientDC	dc(this);
		CPoint		pt, ptDev;
		CPointF		ptD;
		LPCADBINDINFO	pInfo, pUndo;
		for ( const auto& e : m_bindSel ) {
			// 子ﾋﾞｭｰの移動処理
			pInfo = GetDocument()->GetBindInfoData(e.nSel);
			// UNDO情報の保存
			pUndo = new CADBINDINFO(pInfo);
			m_bindUndo.AddTail(pUndo);
			//
			pt = point - e.ptDiff;
			if ( nFlags & MK_SHIFT ) {
				int ppx = abs(pt.x - e.rc.left),
					ppy = abs(pt.y - e.rc.top);
				if ( ppx < ppy )
					pt.x = e.rc.left;
				else
					pt.y = e.rc.top;
			}
			dc.DPtoLP(&pt);
			ptD = pt;
			ptD /= m_dFactor;
			pInfo->pt = ptD;
			// 加工原点の再設定
			ptDev = *ptOrg * m_dFactor;
			dc.LPtoDP(&ptDev);
			ClientToScreen(&ptDev);
			BindMsgPost(&dc, pInfo, &ptDev);	// SendMessage(WM_USERVIEWFITMSG)
		}
		Invalidate();
		GetDocument()->SetModifiedFlag();
		m_enBind = BD_SELECT;
		return;
	}

	// Downｲﾍﾞﾝﾄがあった時だけ選択処理を行う
	// CViewBase::_OnLButtonUp() より先に判定
	BOOL	bSelect = m_enLstate == MS_DOWN ? TRUE : FALSE;

	if ( CViewBase::_OnLButtonUp(point) == 1 ) {
		// 拡大処理
		OnLensKey(ID_VIEW_LENSP);
		return;
	}

	// 範囲選択処理
	if ( m_bMagRect && IsBindParent() ) {
		BOOL	bSelect = FALSE;
		CPoint	pt(point);
		ClientToScreen(&pt);
		LPCADBINDINFO	pInfo;
		CRectF	rcRange(m_rcMagnify), rc, rcCross;
		rcRange /= m_dFactor;
		rcRange.NormalizeRect();
		for ( int i=0; i<GetDocument()->GetBindInfoCnt(); i++ ) {
			pInfo = GetDocument()->GetBindInfoData(i);
			rc = pInfo->pDoc->GetMaxRect();
			rc.SetRect(pInfo->pt, rc.Width(), -rc.Height());
			rc.NormalizeRect();
			if ( rcCross.CrossRect(rcRange, rc) ) {
				OnBindLButtonDown(i, reinterpret_cast<LPARAM>(&pt));
				bSelect = TRUE;
			}
		}
		if ( bSelect ) {
			m_bMagRect = FALSE;	// 範囲表示消去
			Invalidate();
			UpdateWindow();
			return;
		}
	}

	// 加工指示[しない|できない]条件
	if ( !bSelect || m_bMagRect || !GetDocument()->IsDocFlag(DXFDOC_SHAPE) )
		return;

	// --- 加工指示
	CDXFdata*	pData = NULL;
	CClientDC	dc(this);
	// 座標値計算(pointはCViewBase::OnLButtonUp()で論理座標に変換済み)
	//   -> DPtoLP()不要
	CPointF	pt(point);
	pt /= m_dFactor;
	CRect	rc;
	GetClientRect(rc);
	dc.DPtoLP(&rc);
	rc.NormalizeRect();
	CRectF	rcView(rc);
	rcView /= m_dFactor;
	// ｸﾘｯｸﾎﾟｲﾝﾄから集合検索
	if ( !m_pSelData ) {
		CDXFshape*	pShape = NULL;
		float		dGap;
		tie(pShape, pData, dGap) = GetDocument()->GetSelectObject(pt, rcView);
		if ( !pShape || !pData || dGap>=SELECTGAP/m_dFactor )
			return;
		// ﾂﾘｰの選択と m_vSelect の更新通知
		GetDocument()->UpdateAllViews(this, UAV_DXFSHAPEUPDATE, pShape);
	}

	// 加工指示ごとの処理
	switch ( GetDocument()->GetShapeProcessID() ) {
	case ID_EDIT_SHAPE_SEL:		// 分離
		OnLButtonUp_Separate(&dc, pData, pt, rcView);
		break;
	case ID_EDIT_SHAPE_VEC:		// 方向
		OnLButtonUp_Vector(&dc, pData, pt, rcView);
		break;
	case ID_EDIT_SHAPE_START:	// 開始位置
		OnLButtonUp_Start(&dc, pData, pt, rcView);
		break;
	case ID_EDIT_SHAPE_OUT:		// 輪郭
//	case ID_EDIT_SHAPE_POC:		// ﾎﾟｹｯﾄ
		OnLButtonUp_Outline(&dc, pData, pt, rcView);
		break;
	}
}

void CDXFView::OnRButtonDown(UINT nFlags, CPoint point)
{
#ifdef _DEBUG
	CString	strDbg;
	if ( IsBindParent() )
		strDbg = "P";
	else if ( IsBindMode() )
		strDbg = "C";
	strDbg = "CDXFView::OnRButtonDown("+strDbg+") Start\n";
	printf(LPCTSTR(strDbg));
#endif
	if ( IsBindMode() ) {
		if ( nFlags == MK_RBUTTON ) {
			// 右ﾎﾞﾀﾝ押しただけなら通常動作
			CView::OnRButtonDown(nFlags, point);
		}
		else {
			// 親ﾋﾞｭｰに転送
			CDXFView*	pView  = static_cast<CDXFView*>(GetParent());
			ASSERT(pView);
			ClientToScreen(&point);
			pView->ScreenToClient(&point);
			pView->SendMessage(WM_RBUTTONDOWN, (WPARAM)nFlags,
						(LPARAM)(point.x|(point.y<<16)));
		}
	}
	else if ( m_enBind == BD_MOVE )
		OnBindCancel(0, 0);
	else
		CViewBase::OnRButtonDown(nFlags, point);
}

void CDXFView::OnRButtonUp(UINT nFlags, CPoint point) 
{
#ifdef _DEBUG
	CString	strDbg;
	if ( IsBindParent() )
		strDbg = "P";
	else if ( IsBindMode() )
		strDbg = "C";
	strDbg = "CDXFView::OnRButtonUp("+strDbg+") Start\n";
	printf(LPCTSTR(strDbg));
#endif
	if ( IsBindMode() ) {
		CDXFView*	pView  = static_cast<CDXFView*>(GetParent());
		ASSERT(pView);
		int n = pView->GetDocument()->GetBindInfo_fromView(this);
		if ( pView->IsBindSelected(n) ) {
			// 等しいのがあればｺﾝﾃｷｽﾄﾒﾆｭｰ表示へ
			CView::OnRButtonUp(nFlags, point);
			return;
		}
		// 現在選択されている情報になければ自分自身のｺﾝﾃｷｽﾄﾒﾆｭｰ表示へ
		pView->ClearBindSelectData();
		if ( n >= 0 ) {
			pView->SendMessage(WM_USERBIND_LDOWN, n);
			Invalidate();
			CView::OnRButtonUp(nFlags, point);
		}
	}
	else {
		switch ( CViewBase::_OnRButtonUp(point) ) {
		case 1:		// 拡大処理
			OnLensKey(ID_VIEW_LENSP);
			break;
		case 2:		// 選択ｵﾌﾞｼﾞｪｸﾄのｷｬﾝｾﾙ or ｺﾝﾃｷｽﾄﾒﾆｭｰ表示
			if ( !CancelForSelect() )
				CView::OnRButtonUp(nFlags, point);
			break;
		}
	}
}

void CDXFView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
#ifdef _DEBUG
	CString	strDbg;
	if ( IsBindParent() )
		strDbg = "P";
	else if ( IsBindMode() )
		strDbg = "C";
	strDbg = "CDXFView::OnLButtonDblClk("+strDbg+") Start\n";
	printf(LPCTSTR(strDbg));
#endif
	if ( IsBindMode() ) {
		// 子ﾋﾞｭｰ回転
		CDXFView*	pView  = static_cast<CDXFView*>(GetParent());
		ASSERT(pView);
		int n = pView->GetDocument()->GetBindInfo_fromView(this);
		if ( n >= 0 )
			pView->SendMessage(WM_USERBIND_ROUND, n, nFlags & MK_SHIFT ? -1 : 1);
		return;
	}

	if ( !m_pSelData || m_nSelect<0 || m_vSelect.which()!=DXFTREETYPE_SHAPE )
		return;

	// ﾀﾞﾌﾞﾙｸﾘｯｸ動作でも OnLButtonUp() ｲﾍﾞﾝﾄが発生するが、
	// 「Downｲﾍﾞﾝﾄがあった時だけ選択処理を行う」なので
	// ﾀﾞﾌﾞﾙｸﾘｯｸｲﾍﾞﾝﾄを捕まえて、同じ処理(確定)を行う
	CClientDC	dc(this);
	dc.DPtoLP(&point);
	CPointF		pt(point);
	pt /= m_dFactor;
	CRect	rc;
	GetClientRect(rc);
	dc.DPtoLP(&rc);
	rc.NormalizeRect();
	CRectF	rcView(rc);
	rcView /= m_dFactor;

	// 加工指示ごとの処理
	switch ( GetDocument()->GetShapeProcessID() ) {
	case ID_EDIT_SHAPE_SEL:		// 分離
		OnLButtonUp_Separate(&dc, NULL, pt, rcView);
		break;
	case ID_EDIT_SHAPE_VEC:		// 方向
		OnLButtonUp_Vector(&dc, NULL, pt, rcView);
		break;
	case ID_EDIT_SHAPE_START:	// 開始位置
		OnLButtonUp_Start(&dc, NULL, pt, rcView);
		break;
	case ID_EDIT_SHAPE_OUT:		// 輪郭
//	case ID_EDIT_SHAPE_POC:		// ﾎﾟｹｯﾄ
		OnLButtonUp_Outline(&dc, NULL, pt, rcView);
		break;
	}
}
#ifdef _DEBUG
void CDXFView::OnRButtonDblClk(UINT nFlags, CPoint point)
{
#ifdef _DEBUG
	CString	strDbg;
	if ( IsBindParent() )
		strDbg = "P";
	else if ( IsBindMode() )
		strDbg = "C";
	strDbg = "CDXFView::OnRButtonDblClk("+strDbg+") Start\n";
	printf(LPCTSTR(strDbg));
#endif
/*
	// ｺﾝﾃｷｽﾄﾒﾆｭｰ表示では右ﾀﾞﾌﾞﾙｸﾘｯｸ効かない
	if ( IsBindMode() ) {
		// 子ﾋﾞｭｰ右回転(-90度)
		CDXFView*	pView  = static_cast<CDXFView*>(GetParent());
		ASSERT(pView);
		int n = pView->GetDocument()->GetBindInfo_fromView(this);
		if ( n >= 0 )
			pView->SendMessage(WM_USERBIND_ROUND, n, -1);
	}
*/
}
#endif
void CDXFView::OnLButtonUp_Separate
	(CDC* pDC, CDXFdata* pDataSel, const CPointF& ptView, const CRectF& rcView)
{
	ASSERT( m_vSelect.which()==DXFTREETYPE_SHAPE );
	CDXFshape*	pShapeSel = get<CDXFshape*>(m_vSelect);
	if ( !pDataSel ) {
		pShapeSel->GetSelectObjectFromShape(ptView, &rcView, &pDataSel);	// 戻り値不要
		if ( !pDataSel )
			return;
	}

	if ( m_pSelData ) {
		// 他の集合(軌跡集合のみ)にﾘﾝｸできるかどうか
		CLayerData*	pLayer = m_pSelData->GetParentLayer();
		CDXFshape*	pShape;
		CDXFshape*	pShapeLink = NULL;
		CDXFmap*	pMap;
		for ( int i=0; i<pLayer->GetShapeSize(); i++ ) {
			pShape = pLayer->GetShapeData(i);
			if ( pShapeSel!=pShape && (pMap=pShape->GetShapeMap()) ) {
				// 座標検索
				CPointF	pt;
				for ( int j=0; j<pDataSel->GetPointNumber(); j++ ) {
					pt = pDataSel->GetNativePoint(j);
					if ( pMap->PLookup(pt) ) {
						pShapeLink = pShape;
						break;
					}
				}
				if ( pShapeLink ) {
					pMap->SetPointMap(pDataSel);
					break;
				}
			}
		}
		pShape = NULL;
		DXFADDSHAPE	addShape;	// DocBase.h
		if ( !pShapeLink ) {
			// 集合を新規作成
			pMap = NULL;
			try {
				pMap = new CDXFmap;
				pMap->SetPointMap(pDataSel);
				pShape = new CDXFshape(DXFSHAPE_LOCUS, pShapeSel->GetShapeName()+"(分離)",
									pMap->GetMapTypeFlag(), pLayer, pMap);
				// ﾚｲﾔ情報に追加
				pLayer->AddShape(pShape);
			}
			catch (CMemoryException* e) {
				AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
				e->Delete();
				if ( pMap )
					delete	pMap;
				if ( pShape )
					delete	pShape;
				return;
			}
			// 方向指示の確認
			CDXFworking*	pWork;
			CDXFdata*		pData;
			tie(pWork, pData) = pShapeSel->GetDirectionObject();
			if ( pData == pDataSel )
				pShapeSel->DelWorkingData(pWork, pShape);	// 付け替え
			// 開始位置の確認
			tie(pWork, pData) = pShapeSel->GetStartObject();
			if ( pData == pDataSel )
				pShapeSel->DelWorkingData(pWork, pShape);
			// ﾂﾘｰﾋﾞｭｰへの通知
			addShape.pLayer = pLayer;
			addShape.pShape = pShape;
			GetDocument()->UpdateAllViews(this, UAV_DXFADDSHAPE, reinterpret_cast<CObject *>(&addShape));
		}
		// 選択ﾌﾗｸﾞのｸﾘｱ
		pShapeSel->SetShapeSwitch(FALSE);
		// 元集合からｵﾌﾞｼﾞｪｸﾄを消去
		pShapeSel->RemoveObject(pDataSel);
		// 集合検査
		if ( pShapeSel->LinkObject() ) {
			// ﾂﾘｰﾋﾞｭｰへの通知
			addShape.pLayer = pLayer;
			addShape.pShape = pShapeSel;
			GetDocument()->UpdateAllViews(this, UAV_DXFADDSHAPE, reinterpret_cast<CObject *>(&addShape));
		}
		// ﾘﾝｸ出来たときはﾘﾝｸ先集合も検査
		if ( pShapeLink ) {
			pShape = pShapeLink;
			if ( pShapeLink->LinkObject() ) {
				// ﾂﾘｰﾋﾞｭｰへの通知
				addShape.pLayer = pLayer;
				addShape.pShape = pShapeLink;
				GetDocument()->UpdateAllViews(this, UAV_DXFADDSHAPE, reinterpret_cast<CObject *>(&addShape));
			}
		}
		// 分離集合を選択
		ASSERT ( pShape );
		pShape->SetShapeSwitch(TRUE);
		m_vSelect = pShape;
		GetDocument()->UpdateAllViews(this, UAV_DXFSHAPEUPDATE, pShape);
		// 指定ｵﾌﾞｼﾞｪｸﾄﾘｾｯﾄ
		m_pSelData = NULL;
		m_nSelect  = -1;
		// ﾄﾞｷｭﾒﾝﾄ変更通知
		GetDocument()->SetModifiedFlag();
		// 再描画
		Invalidate();
	}
	else {
		// ｵﾌﾞｼﾞｪｸﾄを１つしか持たない集合は分離できない
		if ( pShapeSel->GetObjectCount() > 1 ) {
			m_pSelData = pDataSel;
			m_nSelect = 0;	// dummy
			// 集合全体が選択状態のハズなので，選択ｵﾌﾞｼﾞｪｸﾄのみ解除
			m_pSelData->SetDxfFlg(DXFFLG_SELECT, FALSE);
			DrawTemporaryProcess(pDC);
		}
	}
}

void CDXFView::OnLButtonUp_Vector
	(CDC* pDC, CDXFdata* pDataSel, const CPointF& ptView, const CRectF& rcView)
{
	ASSERT( m_vSelect.which()==DXFTREETYPE_SHAPE );
	CDXFshape*	pShapeSel = get<CDXFshape*>(m_vSelect);
	if ( !pDataSel ) {
		pShapeSel->GetSelectObjectFromShape(ptView, &rcView, &pDataSel);
		if ( !pDataSel )
			return;
	}

	if ( m_pSelData ) {
/*	---
		ここで SwapNativePt() を呼び出して座標を入れ替えると、
		輪郭ｵﾌﾞｼﾞｪｸﾄとのリンクが取れなくなる。
--- */
		// 加工指示生成
		CDXFworking* pWork = CreateWorkingData();
		// 加工指示描画
		if ( pWork ) {
			pWork->DrawTuning(m_dFactor);
			CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_OUTLINE));
			pDC->SetROP2(R2_COPYPEN);
			pWork->Draw(pDC);
			pDC->SelectObject(pOldPen);
		}
		// 指定ｵﾌﾞｼﾞｪｸﾄﾘｾｯﾄ
		m_pSelData = NULL;
		m_nSelect  = -1;
	}
	else {
		if ( !(pShapeSel->GetShapeFlag()&DXFMAPFLG_DIRECTION) ) {
			m_pSelData = pDataSel;
			// 矢印座標の取得
			m_pSelData->GetDirectionArraw(ptView, m_ptArraw);
			// 現在位置に近い方の矢印を描画
			m_nSelect = GAPCALC(m_ptArraw[0][1] - ptView) < GAPCALC(m_ptArraw[1][1] - ptView) ? 0 : 1;
			DrawTempArraw(pDC);
		}
	}
}

void CDXFView::OnLButtonUp_Start
	(CDC* pDC, CDXFdata* pDataSel, const CPointF& ptView, const CRectF& rcView)
{
	ASSERT( m_vSelect.which()==DXFTREETYPE_SHAPE );
	CDXFshape*	pShapeSel = get<CDXFshape*>(m_vSelect);
	if ( !pDataSel ) {
		pShapeSel->GetSelectObjectFromShape(ptView, &rcView, &pDataSel);
		if ( !pDataSel )
			return;
	}

	if ( m_pSelData ) {
		// 加工指示生成
		CDXFworking* pWork = CreateWorkingData();
		// 加工指示描画
		if ( pWork ) {
			pWork->DrawTuning(m_dFactor);
			CPen*	pOldPen	  = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_OUTLINE));
			CBrush*	pOldBrush = pDC->SelectObject(AfxGetNCVCMainWnd()->GetBrushDXF(DXFBRUSH_START));
			pDC->SetROP2(R2_COPYPEN);
			pWork->Draw(pDC);
			pDC->SelectObject(pOldBrush);
			pDC->SelectObject(pOldPen);
		}
		// 指定ｵﾌﾞｼﾞｪｸﾄﾘｾｯﾄ
		m_pSelData = NULL;
		m_nSelect  = -1;
	}
	else {
		if ( !(pShapeSel->GetShapeFlag()&DXFMAPFLG_START) ) {
			m_pSelData = pDataSel;
			// ｵﾌﾞｼﾞｪｸﾄの座標の取得
			ASSERT( m_pSelData->GetPointNumber() <= SIZEOF(m_ptStart) );
			float	dGap, dGapMin = FLT_MAX;
			for ( int i=0; i<m_pSelData->GetPointNumber(); i++ ) {
				m_ptStart[i] = m_pSelData->GetNativePoint(i);
				dGap = GAPCALC(m_ptStart[i] - ptView);
				if ( dGap < dGapMin ) {
					dGapMin = dGap;
					m_nSelect = i;
				}
			}
			// 現在位置に近い方を描画
			DrawTempStart(pDC);
		}
	}
}

void CDXFView::OnLButtonUp_Outline
	(CDC* pDC, CDXFdata* pDataSel, const CPointF& ptView, const CRectF& rcView)
{
	float		dGap1, dGap2;

	if ( m_pSelData ) {
		CDXFdata*	pData1;
		CDXFdata*	pData2;
		dGap1 = m_ltOutline[0].GetSelectObjectFromShape(ptView, &rcView, &pData1);
		dGap2 = m_ltOutline[1].GetSelectObjectFromShape(ptView, &rcView, &pData2);
		int		nSelect = dGap1 > dGap2 ? 1 : 0;
		// 加工指示生成
		m_nSelect = nSelect;
		CDXFworking* pWork = CreateWorkingData();
		// 加工指示描画
		if ( pWork ) {
			pWork->DrawTuning(m_dFactor);
			CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_OUTLINE));
			CBrush* pOldBrush = (CBrush *)pDC->SelectStockObject(NULL_BRUSH);
			pDC->SetROP2(R2_COPYPEN);
			pWork->Draw(pDC);
			pDC->SelectObject(pOldBrush);
			pDC->SelectObject(pOldPen);
		}
		// 指定ｵﾌﾞｼﾞｪｸﾄﾘｾｯﾄ
		m_pSelData = NULL;
		m_nSelect  = -1;
	}
	else {
		ASSERT( m_vSelect.which()==DXFTREETYPE_SHAPE );
		CDXFshape*	pShapeSel = get<CDXFshape*>(m_vSelect);
		if ( !pDataSel ) {
			pShapeSel->GetSelectObjectFromShape(ptView, &rcView, &pDataSel);
			if ( !pDataSel )
				return;
		}
		// 複数輪郭が設定できるように変更
//		if ( pShapeSel->GetShapeAssemble() != DXFSHAPE_OUTLINE ||
//				pShapeSel->GetShapeFlag() & (DXFMAPFLG_OUTLINE|DXFMAPFLG_POCKET) )
		if ( pShapeSel->GetShapeAssemble() != DXFSHAPE_OUTLINE )
			return;
		m_pSelData = pDataSel;
		// 輪郭ｵﾌﾞｼﾞｪｸﾄ生成
		if ( !CreateOutlineTempObject(pShapeSel) ) {
			AfxMessageBox(IDS_ERR_DXF_CREATEOUTELINE, MB_OK|MB_ICONEXCLAMATION);
			return;
		}
		// ﾏｳｽ座標から内外ｵﾌﾞｼﾞｪｸﾄの選択
		dGap1 = m_ltOutline[0].GetSelectObjectFromShape(ptView, &rcView);
		dGap2 = m_ltOutline[1].GetSelectObjectFromShape(ptView, &rcView);
		m_nSelect = dGap1 > dGap2 ? 1 : 0;
		//
		AllChangeFactor_OutlineTempObject();
		DrawTempOutline(pDC);
	}
}

void CDXFView::OnMouseMove(UINT nFlags, CPoint point) 
{
	if ( nFlags & MK_LBUTTON ) {
		if ( IsBindMode() ) {
			CDXFView*	pView  = static_cast<CDXFView*>(GetParent());
			ASSERT(pView);
			if ( !pView->m_bindSel.empty() && pView->m_enBind==BD_SELECT ) {
				if ( abs(m_ptMouse.x - point.x) >= MAGNIFY_RANGE ||
					 abs(m_ptMouse.y - point.y) >= MAGNIFY_RANGE ) {
					// OnNcHitTest() 条件
					pView->m_enBind = BD_MOVE;	// 以降、親でOnMouseMove()検知
					return;
				}
			}
		}
		else {
			if ( m_enBind == BD_SELECT ) {
				if ( abs(m_ptMouse.x - point.x) >= MAGNIFY_RANGE ||
					 abs(m_ptMouse.y - point.y) >= MAGNIFY_RANGE )
					m_enBind = BD_MOVE;
			}
			if ( m_enBind == BD_MOVE ) {
				CPoint	pt;
				LPCADBINDINFO pInfo;
				for ( const auto& e : m_bindSel ) {
					pt = point - e.ptDiff;
					pInfo = GetDocument()->GetBindInfoData(e.nSel);
					if ( nFlags & MK_SHIFT ) {
						int ppx = abs(pt.x - e.rc.left),
							ppy = abs(pt.y - e.rc.top);
						if ( ppx < ppy )
							pt.x = e.rc.left;
						else
							pt.y = e.rc.top;
					}
//					pInfo->pView->MoveWindow(pt.x, pt.y, m_rcMove.Width(), m_rcMove.Height());
					pInfo->pView->SetWindowPos(NULL, pt.x, pt.y, e.rc.Width(), e.rc.Height(),
						SWP_NOZORDER|SWP_NOACTIVATE);
				}
				return;
			}
		}
	}

	if ( CViewBase::_OnMouseMove(nFlags, point) ||
			!m_pSelData || m_vSelect.which()!=DXFTREETYPE_SHAPE ) {
		if ( IsBindParent() ) {
			BindMove(FALSE);
			UpdateWindow();
		}
		return;
	}

	// CViewBase::OnMouseMove() で処理がなければ
	// ここで論理座標に変換する必要がある
	CClientDC	dc(this);
	dc.DPtoLP(&point);
	CPointF		pt(point);
	pt /= m_dFactor;
	CRect	rc;
	GetClientRect(rc);
	dc.DPtoLP(&rc);
	rc.NormalizeRect();
	CRectF	rcView(rc);
	rcView /= m_dFactor;

	// 加工指示ごとの処理
	switch ( GetDocument()->GetShapeProcessID() ) {
	case ID_EDIT_SHAPE_SEL:
		OnMouseMove_Separate(&dc, pt, rcView);
		break;
	case ID_EDIT_SHAPE_VEC:
		OnMouseMove_Vector(&dc, pt, rcView);
		break;
	case ID_EDIT_SHAPE_START:
		OnMouseMove_Start(&dc, pt, rcView);
		break;
	case ID_EDIT_SHAPE_OUT:
//	case ID_EDIT_SHAPE_POC:
		OnMouseMove_Outline(&dc, pt, rcView);
		break;
	}
}

void CDXFView::OnMouseMove_Separate
	(CDC* pDC, const CPointF& ptView, const CRectF& rcView)
{
	ASSERT( m_vSelect.which()==DXFTREETYPE_SHAPE );
	CDXFshape*	pShape = get<CDXFshape*>(m_vSelect);
	CDXFdata*	pDataSel = NULL;
	pShape->GetSelectObjectFromShape(ptView, &rcView, &pDataSel);
	if ( !pDataSel )
		return;

	if ( m_pSelData != pDataSel ) {
		// 前回の選択状態を元に戻す
		m_pSelData->SetDxfFlg(DXFFLG_SELECT);	// 分離の場合は選択状態が逆
		DrawTemporaryProcess(pDC);
		// 新しい選択状態を描画
		m_pSelData = pDataSel;
		m_pSelData->SetDxfFlg(DXFFLG_SELECT, FALSE);
		DrawTemporaryProcess(pDC);
	}
}

void CDXFView::OnMouseMove_Vector
	(CDC* pDC, const CPointF& ptView, const CRectF& rcView)
{
	float	dGap1 = GAPCALC(m_ptArraw[0][1] - ptView),
			dGap2 = GAPCALC(m_ptArraw[1][1] - ptView);
	int		nSelect = dGap1 > dGap2 ? 1 : 0;

	if ( m_nSelect != nSelect ) {
		// 前回の矢印を消去
		DrawTempArraw(pDC);
		// 新しい矢印の描画
		m_nSelect = nSelect;
		DrawTempArraw(pDC);
	}
}

void CDXFView::OnMouseMove_Start
	(CDC* pDC, const CPointF& ptView, const CRectF& rcView)
{
	int		i, nSelect;
	float	dGap, dGapMin = FLT_MAX;

	for ( i=0; i<m_pSelData->GetPointNumber(); i++ ) {
		dGap = GAPCALC(m_ptStart[i] - ptView);
		if ( dGap < dGapMin ) {
			dGapMin = dGap;
			nSelect = i;
		}
	}

	if ( m_nSelect != nSelect ) {
		// 前回の開始位置を消去
		DrawTempStart(pDC);
		// 新しい開始位置の描画
		m_nSelect = nSelect;
		DrawTempStart(pDC);
	}
}

void CDXFView::OnMouseMove_Outline
	(CDC* pDC, const CPointF& ptView, const CRectF& rcView)
{
	float	dGap1 = m_ltOutline[0].GetSelectObjectFromShape(ptView, &rcView),
			dGap2 = m_ltOutline[1].GetSelectObjectFromShape(ptView, &rcView);
	int		nSelect = dGap1 > dGap2 ? 1 : 0;

	if ( m_nSelect != nSelect ) {
		// 前回の輪郭を消去
		DrawTempOutline(pDC);
		// 新しい矢印の描画
		m_nSelect = nSelect;
		DrawTempOutline(pDC);
	}
}

BOOL CDXFView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
	if ( IsBindMode() ) {
		CDXFView* pView = static_cast<CDXFView*>(GetParent());
		ASSERT(pView);
		pView->OnMouseWheel(nFlags, zDelta, pt);
		return FALSE;
	}
	return __super::OnMouseWheel(nFlags, zDelta, pt);
}

void CDXFView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	switch ( nChar ) {
	case VK_TAB:
		if ( !IsBindMode() && GetDocument()->IsDocFlag(DXFDOC_SHAPE) )
			static_cast<CDXFChild *>(GetParentFrame())->GetTreeView()->SetFocus();
		break;
	case VK_ESCAPE:
		if ( IsBindMode() ) {
			// 親へ通知
			CDXFView*	pView  = static_cast<CDXFView*>(GetParent());
			ASSERT(pView);
			if ( pView->m_enBind == BD_MOVE )
				pView->SendMessage(WM_USERBIND_CANCEL);
		}
		break;
//	case VK_DELETE:		// Delｷｰはｱｸｾﾗﾚｰﾀにて
//		break;
	}
	__super::OnKeyDown(nChar, nRepCnt, nFlags);
}

BOOL CDXFView::OnEraseBkgnd(CDC* pDC) 
{
	BOOL	bResult = FALSE;
	if ( !IsBindMode() ) {
		const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
		COLORREF	col1 = pOpt->GetDxfDrawColor(DXFCOL_BACKGROUND1),
					col2 = pOpt->GetDxfDrawColor(DXFCOL_BACKGROUND2);
		bResult = CViewBase::_OnEraseBkgnd(pDC, col1, col2);
	}
	return bResult;
}

LRESULT CDXFView::OnNcHitTest(CPoint point)
{
	if ( IsBindMode() ) {
		CDXFView* pView = static_cast<CDXFView*>(GetParent());
		ASSERT(pView);
		if ( !pView->m_bindSel.empty() && pView->m_enBind==BD_MOVE ) {
			// 子ﾋﾞｭｰ移動中
			return HTTRANSPARENT;
		}
	}
	return __super::OnNcHitTest(point);
}

BOOL CDXFView::OnToolTipNotify(UINT id, NMHDR *pNMH, LRESULT *pResult)
{
#ifdef _DEBUG
	CString	strDbg;
	if ( IsBindParent() )
		strDbg = "P";
	else if ( IsBindMode() )
		strDbg = "C";
	strDbg = "CDXFView::OnToolTipNotify("+strDbg+") Start\n";
	printf(LPCTSTR(strDbg));
#endif
	// 親ｳｨﾝﾄﾞｳでﾒｯｾｰｼﾞが処理される
	TOOLTIPTEXT *pText = (TOOLTIPTEXT *)pNMH;
	if ( pText->lParam > 0 ) {
		INT_PTR	n = pText->lParam - 1;
		if ( 0<=n && n<GetDocument()->GetBindInfoCnt() ) {
			CString	strTip(GetDocument()->GetBindInfoData(n)->pDoc->GetTitle());
			lstrcpyn(pText->lpszText, strTip, sizeof(pText->szText)-1);
			pText->hinst = AfxGetInstanceHandle();
			return TRUE;
		}
	}

	// 子ﾃﾞｰﾀのﾂｰﾙﾋﾝﾄ以外（ﾂｰﾙﾊﾞｰなど）はMainFrm.cppで処理
	return FALSE;
}

INT_PTR CDXFView::OnToolHitTest(CPoint point, TOOLINFO* pTI) const
{
	HWND	hwnd = GetSafeHwnd();
	pTI->cbSize	= sizeof(TOOLINFO);
	pTI->uFlags	= TTF_IDISHWND;
//	pTI->hwnd	= ::GetParent(hwnd);
	pTI->hwnd	= hwnd;	// hwndでも親ﾋﾞｭｰにOnToolTipNotify()が飛ぶ??
	pTI->uId	= (UINT_PTR)hwnd;
	pTI->lpszText = LPSTR_TEXTCALLBACK;
	CDXFDoc*	pParentDoc = static_cast<const CDXFDoc*>(m_pDocument)->GetBindParentDoc();
	// ｾﾞﾛﾍﾞｰｽだと通常のﾂｰﾙﾁｯﾌﾟと区別がつかない
	pTI->lParam	= pParentDoc ? (pParentDoc->GetBindInfo_fromView(this)+1) : 0;

	return 0;
}
