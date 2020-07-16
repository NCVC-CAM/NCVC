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

#include <stdlib.h>

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
extern	CMagaDbg	g_dbg;
#endif

using namespace boost;

// 指定座標との閾値
static	const	double	SELECTGAP = 5.0;

/////////////////////////////////////////////////////////////////////////////
// CDXFView

IMPLEMENT_DYNCREATE(CDXFView, CView)

BEGIN_MESSAGE_MAP(CDXFView, CView)
	//{{AFX_MSG_MAP(CDXFView)
	ON_WM_CREATE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_KEYDOWN()
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_DXFVIEW_LAYER, OnViewLayer)
	ON_UPDATE_COMMAND_UI(ID_DXFVIEW_LAYER, OnUpdateViewLayer)
	ON_WM_ERASEBKGND()
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	//}}AFX_MSG_MAP
	// ﾕｰｻﾞｲﾆｼｬﾙ処理 & 各ﾋﾞｭｰへのﾌｨｯﾄﾒｯｾｰｼﾞ
	ON_MESSAGE (WM_USERVIEWFITMSG, OnUserViewFitMsg)
	// ﾏｳｽ移動のｽﾃｰﾀｽﾊﾞｰ更新
	ON_UPDATE_COMMAND_UI(ID_DXFST_MOUSE, OnUpdateMouseCursor)
	// 移動
	ON_COMMAND_RANGE(ID_VIEW_UP, ID_VIEW_RT, OnMoveKey)
	ON_COMMAND_RANGE(ID_VIEW_BEFORE, ID_VIEW_LENSN, OnLensKey)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDXFView クラスの構築/消滅

CDXFView::CDXFView()
{
	m_enProcessDirect = DXFPROCESS_SELECT;
	m_nSelect = -1;
	m_pSelData = NULL;
}

CDXFView::~CDXFView()
{
	DeleteOutlineTempObject();
}

/////////////////////////////////////////////////////////////////////////////
// CDXFView クラスのオーバライド関数

BOOL CDXFView::PreCreateWindow(CREATESTRUCT& cs) 
{
	// ｽﾌﾟﾘｯﾀからの境界ｶｰｿﾙが移るので，IDC_ARROW を明示的に指定
	cs.lpszClass = AfxRegisterWndClass(
			CS_HREDRAW|CS_VREDRAW|CS_OWNDC|CS_DBLCLKS,
			AfxGetApp()->LoadStandardCursor(IDC_ARROW) );
	return CView::PreCreateWindow(cs);
}

void CDXFView::OnInitialUpdate() 
{
	CView::OnInitialUpdate();

	// MDI子ﾌﾚｰﾑのｽﾃｰﾀｽﾊﾞｰに情報表示
	((CDXFChild *)GetParentFrame())->SetDataInfo(
		GetDocument()->GetDxfDataCnt(DXFLINEDATA),
		GetDocument()->GetDxfDataCnt(DXFCIRCLEDATA),
		GetDocument()->GetDxfDataCnt(DXFARCDATA),
		GetDocument()->GetDxfDataCnt(DXFELLIPSEDATA),
		GetDocument()->GetDxfDataCnt(DXFPOINTDATA) );

	// ｼﾘｱﾙ化後の図形ﾌｨｯﾄﾒｯｾｰｼﾞの送信
	// OnInitialUpdate()関数内では，GetClientRect()のｻｲｽﾞが正しくない
	if ( GetDocument()->IsShape() )
		GetParentFrame()->PostMessage(WM_USERINITIALUPDATE);
	else
		PostMessage(WM_USERVIEWFITMSG);
}

void CDXFView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	switch ( lHint ) {
	case UAV_FILESAVE:
	case UAV_DXFAUTODELWORKING:
		return;		// 再描画不要
	case UAV_DXFORGUPDATE:
		ASSERT( pHint );
		((CDXFcircleEx *)pHint)->DrawTuning(m_dFactor*LOMETRICFACTOR);
		break;
	case UAV_DXFSHAPEID:	// from CDXFDoc::OnShapePattern()
		CancelForSelect();
		switch ( GetDocument()->GetShapePattern() ) {
		case ID_EDIT_SHAPE_VEC:
			m_enProcessDirect = DXFPROCESS_ARRAW;
			break;
		case ID_EDIT_SHAPE_OUT:
		case ID_EDIT_SHAPE_POC:
			m_enProcessDirect = DXFPROCESS_OUTLINE;
			break;
		default:
			m_enProcessDirect = DXFPROCESS_SELECT;
			break;
		}
		return;
	case UAV_DXFSHAPEUPDATE:
		if ( pHint ) {		// from CDXFShapeView::OnSelChanged()
			if ( !OnUpdateShape( (DXFTREETYPE*)pHint ) )
				return;	// 再描画不要
		}
		break;
	case UAV_ADDINREDRAW:
		OnViewLensComm();
		return;
	}
	CView::OnUpdate(pSender, lHint, pHint);
}

/////////////////////////////////////////////////////////////////////////////
// CDXFView クラスのメンバ関数

BOOL CDXFView::OnUpdateShape(DXFTREETYPE vSelect[])
{
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
					AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_WORKER) :
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
	int	i, j;
	ASSERT( m_vSelect.which()==DXFTREETYPE_SHAPE );
	CDXFshape*		pShape = get<CDXFshape*>(m_vSelect);
	CDXFworking*	pWork = NULL;

	try {
		switch ( GetDocument()->GetShapePattern() ) {
		case ID_EDIT_SHAPE_VEC:
			pWork = new CDXFworkingDirection(pShape, m_pSelData, m_ptArraw[m_nSelect]);
			break;
		case ID_EDIT_SHAPE_OUT:
			pWork = new CDXFworkingOutline(pShape, m_obOutline[m_nSelect]);
			for ( i=0; i<SIZEOF(m_obOutline); i++ ) {
				if ( i != m_nSelect ) {	// m_nSelect分はCDXFworkingOutlineのﾃﾞｽﾄﾗｸﾀにてdelete
					for ( j=0; j<m_obOutline[i].GetSize(); j++ )
						delete	m_obOutline[i][j];
				}
				m_obOutline[i].RemoveAll();
			}
			break;
		case ID_EDIT_SHAPE_POC:
			pWork = new CDXFworkingPocket(pShape);
			break;
		}
		// 加工情報の登録
		pShape->AddWorkingData(pWork);
		// 形状ﾂﾘｰ(加工指示)の更新と選択
		GetDocument()->UpdateAllViews(this, UAV_DXFADDWORKING, (CObject *)pWork);
		// ﾄﾞｷｭﾒﾝﾄ変更通知
		GetDocument()->SetModifiedFlag();
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

BOOL CDXFView::CreateOutlineTempObject(const CPointD& pt)
{
	BOOL	bLeft = TRUE;

	try {
		if ( GetDocument()->GetShapePattern() == ID_EDIT_SHAPE_OUT ) {
			for ( int i=0; i<SIZEOF(m_obOutline); i++ ) {
				if ( !CreateOutlineTempObject_sub(bLeft, i) )
					return FALSE;
				bLeft = !bLeft;
			}
		}
//		else {
//		}
	}
	catch ( CMemoryException* e ) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
	}

	// ﾏｳｽ座標から内外ｵﾌﾞｼﾞｪｸﾄの選択
// ---
	m_nSelect = 0;
// ---

	return TRUE;
}

BOOL CDXFView::CreateOutlineTempObject_sub(BOOL bLeft, int n)
{
	ASSERT( m_vSelect.which() == DXFTREETYPE_SHAPE );
	CDXFshape*	pShape = get<CDXFshape*>(m_vSelect);
	CDXFchain*	pChain = pShape->GetShapeChain();
	if ( !pChain || pChain->IsEmpty() )
		return FALSE;

	m_obOutline[n].SetSize(0, pChain->GetCount());
	POSITION	pos = pChain->GetHeadPosition();
	CDXFdata*	pData;
	CDXFdata*	pData1 = pChain->GetNext(pos);
	CDXFdata*	pData2;
	CPointD		pte;
	optional<CPointD>	ptResult, pt;

	// TH_ShapeSearch.cppにて始点終点の調整が行われている(SetDirectionFixed)ので
	// 単純ﾙｰﾌﾟで良い
	while ( pos ) {
		pData2 = pChain->GetNext(pos);
		// ｵﾌｾｯﾄ座標計算
		ptResult = pData1->CalcOffsetIntersectionPoint(pData2, 1.0, bLeft);
		if ( !ptResult )
			return FALSE;
		// 輪郭ｵﾌﾞｼﾞｪｸﾄ生成(初回は無視)
		if ( pt ) {
			pData = CreateOutlineTempObject_new(pData1, *pt, *ptResult);
			if ( pData )
				m_obOutline[n].Add(pData);
		}
		else
			pte = *ptResult;	// 最初の輪郭ｵﾌﾞｼﾞｪｸﾄの終点
		pt = ptResult;
		pData1 = pData2;
	}
	// 最初の輪郭ｵﾌﾞｼﾞｪｸﾄ生成
	pData = CreateOutlineTempObject_new(pChain->GetHead(), *pt, pte);
	if ( pData )
		m_obOutline[n].Add(pData);

	return TRUE;
}

CDXFdata* CDXFView::CreateOutlineTempObject_new
	(const CDXFdata* pDataSrc, const CPointD& pts, const CPointD& pte)
{
	CDXFdata*	pData = NULL;
	DXFLARGV	dxfLine;

	switch ( pDataSrc->GetType() ) {
	case DXFLINEDATA:
		dxfLine.pLayer = pDataSrc->GetParentLayer();
		dxfLine.s = pts;
		dxfLine.e = pte;
		pData = new CDXFline(&dxfLine);
		break;
	}

	return pData;
}

void CDXFView::DeleteOutlineTempObject(void)
{
	int	i, j;
	for ( i=0; i<SIZEOF(m_obOutline); i++ ) {
		for ( j=0; j<m_obOutline[i].GetSize(); j++ )
			delete	m_obOutline[i][j];
		m_obOutline[i].RemoveAll();
	}
}

BOOL CDXFView::CancelForSelect(CDC* pDC/*=NULL*/)
{
	if ( m_pSelData && m_nSelect >= 0 ) {
		if ( m_enProcessDirect == DXFPROCESS_SELECT )
			m_pSelData->SetSelectFlg(TRUE);	// 選択状態を元に戻す
		if ( !pDC ) {
			CClientDC	dc(this);
			DrawTemporaryProcess(&dc);
		}
		else
			DrawTemporaryProcess(pDC);
		m_nSelect = -1;
	}

	BOOL	bResult = m_pSelData ? TRUE : FALSE;
	m_pSelData = NULL;
	DeleteOutlineTempObject();

	return bResult;
}

void CDXFView::AllChangeFactor_OutlineTempObject(void)
{
	int	i, j;
	for ( i=0; i<SIZEOF(m_obOutline); i++ ) {
		for ( j=0; j<m_obOutline[i].GetSize(); j++ )
			m_obOutline[i][j]->DrawTuning(m_dFactor*LOMETRICFACTOR);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CDXFView 描画

void CDXFView::OnDraw(CDC* pDC)
{
	ASSERT_VALID(GetDocument());
	int		i, j, nLayerCnt, nDataCnt;
	DWORD	dwSel, dwSelBak = 0;
	CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();
	CLayerData*		pLayer;
	CDXFdata*		pData;

	pDC->SetROP2(R2_COPYPEN);
	pDC->SetTextAlign(TA_LEFT|TA_BOTTOM);
	CFont*	pOldFont = pDC->SelectObject(AfxGetNCVCMainWnd()->GetTextFont(TYPE_DXF));
	CBrush* pOldBrush = (CBrush *)pDC->SelectStockObject(NULL_BRUSH);

	// 原点描画
	CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_ORIGIN));
	if ( pData=GetDocument()->GetCircleObject() )
		pData->Draw(pDC);	// CDXFcircleEx::Draw()

	// DXF描画
	nLayerCnt = GetDocument()->GetLayerCnt();
	for ( i=0; i<nLayerCnt; i++ ) {
		pLayer = GetDocument()->GetLayerData(i);
		if ( !pLayer->IsViewLayer() )
			continue;
		switch ( pLayer->GetLayerType() ) {
		case DXFCAMLAYER:
			pDC->SelectStockObject(NULL_BRUSH);
			pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_CUTTER));
			nDataCnt = pLayer->GetDxfSize();
			for ( j=0; j<nDataCnt; j++ ) {
				pData = pLayer->GetDxfData(j);
				dwSel = pData->GetSelectFlg() & DXFSEL_SELECT;
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
	if ( m_pSelData && m_nSelect >= 0 )
		DrawTemporaryProcess(pDC);

	pDC->SelectObject(pOldPen);
	pDC->SelectObject(pOldBrush);
	pDC->SelectObject(pOldFont);
}

void CDXFView::DrawTemporaryProcess(CDC* pDC)
{
	switch ( m_enProcessDirect ) {
	case DXFPROCESS_SELECT:
		{
			CPen*	pOldPen = pDC->SelectObject(m_pSelData->GetDrawPen());
			pDC->SetROP2(R2_COPYPEN);
			m_pSelData->Draw(pDC);
			pDC->SelectObject(pOldPen);
		}
		break;
	case DXFPROCESS_ARRAW:
		DrawArraw(pDC);
		break;
	case DXFPROCESS_OUTLINE:
		DrawOutline(pDC);
		break;
	}
}

void CDXFView::DrawArraw(CDC* pDC)
{
	ASSERT( 0<=m_nSelect && m_nSelect<=1 );

	// 矢印の長さが拡大率に影響されないように計算
	CPoint	ptDraw[SIZEOF(m_ptArraw)][SIZEOF(m_ptArraw[0])];

	for ( int i=0; i<SIZEOF(m_ptArraw); i++ ) {
		ptDraw[i][1] = m_ptArraw[i][1] * m_dFactor * LOMETRICFACTOR;
		ptDraw[i][0] = m_ptArraw[i][0] + ptDraw[i][1];
		ptDraw[i][2] = m_ptArraw[i][2] + ptDraw[i][1];
	}
	CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_WORKER));
	pDC->SetROP2(R2_XORPEN);
	pDC->Polyline(ptDraw[m_nSelect], SIZEOF(ptDraw[0]));
	pDC->SelectObject(pOldPen);
}

void CDXFView::DrawOutline(CDC* pDC)
{
	ASSERT( 0<=m_nSelect && m_nSelect<=1 );

	CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_WORKER));
	pDC->SetROP2(R2_XORPEN);
	for ( int i=0; i<m_obOutline[m_nSelect].GetSize(); i++ )
		m_obOutline[m_nSelect][i]->Draw(pDC);
	pDC->SelectObject(pOldPen);
}

/////////////////////////////////////////////////////////////////////////////
// CDXFView 診断

#ifdef _DEBUG
void CDXFView::AssertValid() const
{
	CView::AssertValid();
}

void CDXFView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CDXFDoc* CDXFView::GetDocument() // 非デバッグ バージョンはインラインです。
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CDXFDoc)));
	return (CDXFDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CDXFView クラスのメッセージ ハンドラ（メニュー編）

void CDXFView::OnUpdateEditCopy(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(TRUE);
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

void CDXFView::OnUpdateMouseCursor(CCmdUI* pCmdUI) 
{
	CFrameWnd*	pChild = AfxGetNCVCMainWnd()->GetActiveFrame();
	CView*		pView  = pChild ? pChild->GetActiveView() : NULL;
	optional<CPointD>	ptOrg = GetDocument()->GetCutterOrigin();
	if ( pView==this && ptOrg ) {
		POINT	pt;
		::GetCursorPos(&pt);
		ScreenToClient(&pt);
		CClientDC	dc(this);
		dc.DPtoLP(&pt);
		CPointD	ptd( pt.x/m_dFactor/LOMETRICFACTOR,
					 pt.y/m_dFactor/LOMETRICFACTOR );
		// 原点からの距離
		ptd -= *ptOrg;
		((CDXFChild *)GetParentFrame())->OnUpdateMouseCursor(&ptd);
	}
	else
		((CDXFChild *)GetParentFrame())->OnUpdateMouseCursor();
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

void CDXFView::OnMoveKey(UINT nID) 
{
	CViewBase::OnMoveKey(nID);
}

void CDXFView::OnLensKey(UINT nID)
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

void CDXFView::OnViewLensComm(void)
{
	// 拡大率による描画情報の更新
	GetDocument()->AllChangeFactor(m_dFactor);
	// 一時ｵﾌﾞｼﾞｪｸﾄ分
	AllChangeFactor_OutlineTempObject();
	// MDI子ﾌﾚｰﾑのｽﾃｰﾀｽﾊﾞｰに情報表示
	((CDXFChild *)GetParentFrame())->SetFactorInfo(m_dFactor);
	// ﾋﾞｭｰの再描画
	Invalidate();
}

/////////////////////////////////////////////////////////////////////////////
// CDXFView メッセージ ハンドラ

int CDXFView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	// ﾏｯﾋﾟﾝｸﾞﾓｰﾄﾞの変更など
	CViewBase::OnCreate(this);

	return 0;
}

LRESULT CDXFView::OnUserViewFitMsg(WPARAM, LPARAM)
{
	OnLensKey(ID_VIEW_FIT);
	return 0;
}

void CDXFView::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	CViewBase::OnContextMenu(point, IDR_DXFPOPUP1);
}

void CDXFView::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CViewBase::OnLButtonDown(point);
}

void CDXFView::OnLButtonUp(UINT nFlags, CPoint point) 
{
	// Downｲﾍﾞﾝﾄがあった時だけ選択処理を行う
	BOOL	bSelect = m_nLState == 0 ? TRUE : FALSE;

	if ( CViewBase::OnLButtonUp(point) == 1 ) {
		// 拡大処理
		OnLensKey(ID_VIEW_LENSP);
		return;
	}
	// 加工指示[しない|できない]条件
	if ( !bSelect || m_bMagRect || !GetDocument()->IsShape() )
		return;

	// 加工指示
	CClientDC	dc(this);
	double		dGap;
	CDXFdata*	pData  = NULL;
	CDXFshape*	pShape = NULL;
	// 座標値計算(pointはCViewBase::OnLButtonUp()で論理座標に変換済み)
	CPointD	pt(point);
	pt /= ( m_dFactor * LOMETRICFACTOR );
	CRect	rc;
	GetClientRect(&rc);
	dc.DPtoLP(&rc);
	rc.NormalizeRect();
	CRectD	rcView(rc);
	rcView /= ( m_dFactor * LOMETRICFACTOR );

	// ﾏｳｽｸﾘｯｸ状態遷移
	if ( m_vSelect.which() == DXFTREETYPE_SHAPE ) {
		// 集合選択状態
		pShape = get<CDXFshape*>(m_vSelect);
		tie(pData, dGap) = pShape->GetSelectViewGap(pt, rcView);
		if ( pData && dGap < SELECTGAP/m_dFactor ) {
			// 指定座標との距離が閾値未満なら
			switch ( GetDocument()->GetShapePattern() ) {
			case ID_EDIT_SHAPE_SEL:	// 分離
				OnLButtonUp_Sel(pShape, pData, &dc);
				break;
			case ID_EDIT_SHAPE_VEC:	// 方向
				OnLButtonUp_Vec(pShape, pData, pt, &dc);
				break;
			case ID_EDIT_SHAPE_OUT:	// 輪郭
			case ID_EDIT_SHAPE_POC:	// ﾎﾟｹｯﾄ
				OnLButtonUp_Out(pShape, pData, pt, &dc);
				break;
			}
			return;
		}
	}

	// 指定座標との距離が閾値以上か集合非選択状態なら，全体検索
	tie(pShape, dGap) = GetDocument()->GetSelectViewGap(pt, rcView);
	if ( pShape && dGap < SELECTGAP/m_dFactor ) {
		// ﾂﾘｰの選択と m_vSelect の更新通知
		GetDocument()->UpdateAllViews(this, UAV_DXFSHAPEUPDATE, pShape);
	}
	else {
		CancelForSelect(&dc);
	}
}

void CDXFView::OnLButtonUp_Sel
	(CDXFshape* pShapeOrg, const CDXFdata* pDataOrg, CDC* pDC)
{
	if ( m_pSelData ) {
		// 他の集合(軌跡集合のみ)にﾘﾝｸできるかどうか
		CLayerData*	pLayer = m_pSelData->GetParentLayer();
		CDXFshape*	pShape;
		CDXFshape*	pShapeLink = NULL;
		CDXFmap*	pMap;
		for ( int i=0; i<pLayer->GetShapeSize(); i++ ) {
			pShape = pLayer->GetShapeData(i);
			if ( pShapeOrg!=pShape && (pMap=pShape->GetShapeMap()) ) {
				// 座標検索
				CPointD	pt;
				for ( int j=0; j<pDataOrg->GetPointNumber(); j++ ) {
					pt = pDataOrg->GetNativePoint(j);	// 固有座標値
					if ( pt!=HUGE_VAL && pMap->PLookup(pt) ) {
						pShapeLink = pShape;
						break;
					}
				}
				if ( pShapeLink ) {
					pMap->SetPointMap(const_cast<CDXFdata*>(pDataOrg));
					break;
				}
			}
		}
		pShape = NULL;
		DXFADDSHAPE	addShape;	// DocBase.h
		if ( !pShapeLink ) {
			// 方向指示の確認
			CDXFworking*	pWork;
			CDXFdata*	pData;
			tie(pWork, pData) = pShapeOrg->GetDirectionObject();
			// 集合を新規作成
			pMap = NULL;
			try {
				pMap = new CDXFmap;
				pMap->SetPointMap(const_cast<CDXFdata*>(pDataOrg));
				pShape = new CDXFshape(DXFSHAPE_LOCUS, pShapeOrg->GetShapeName()+"(分離)",
									pMap->GetShapeFlag(), pMap);
				// 方向指示の継承
				if ( pData == pDataOrg )
					pShapeOrg->DelWorkingData(pWork, pShape);	// 付け替え
				// ﾚｲﾔ情報に追加
				pLayer->AddShape(pShape);
				// ﾂﾘｰﾋﾞｭｰへの通知
				addShape.pLayer = pLayer;
				addShape.pShape = pShape;
				GetDocument()->UpdateAllViews(this, UAV_DXFADDSHAPE, (CObject *)&addShape);
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
		}
		// 選択ﾌﾗｸﾞのｸﾘｱ
		pShapeOrg->SetShapeSwitch(FALSE);
		// 元集合からｵﾌﾞｼﾞｪｸﾄを消去
		pShapeOrg->RemoveObject(pDataOrg);
		// 集合検査
		if ( pShapeOrg->LinkObject() ) {
			// ﾂﾘｰﾋﾞｭｰへの通知
			addShape.pLayer = pLayer;
			addShape.pShape = pShapeOrg;
			GetDocument()->UpdateAllViews(this, UAV_DXFADDSHAPE, (CObject *)&addShape);
		}
		// ﾘﾝｸ出来たときはﾘﾝｸ先集合も検査
		if ( pShapeLink ) {
			pShape = pShapeLink;
			if ( pShapeLink->LinkObject() ) {
				// ﾂﾘｰﾋﾞｭｰへの通知
				addShape.pLayer = pLayer;
				addShape.pShape = pShapeLink;
				GetDocument()->UpdateAllViews(this, UAV_DXFADDSHAPE, (CObject *)&addShape);
			}
		}
		// 分離集合を選択
		ASSERT ( pShape );
		pShape->SetShapeSwitch(TRUE);
		m_vSelect = pShape;
		GetDocument()->UpdateAllViews(this, UAV_DXFSHAPEUPDATE, pShape);
		// 指定ｵﾌﾞｼﾞｪｸﾄﾘｾｯﾄ
		m_pSelData = NULL;
		// ﾄﾞｷｭﾒﾝﾄ変更通知
		GetDocument()->SetModifiedFlag();
		// 再描画
		Invalidate();
	}
	else {
		// ｵﾌﾞｼﾞｪｸﾄを１つしか持たない集合は分離できない
		if ( pShapeOrg->GetObjectCount() <= 1 ) {
			::MessageBeep(MB_ICONEXCLAMATION);
			return;
		}
		m_enProcessDirect = DXFPROCESS_SELECT;
		m_pSelData = const_cast<CDXFdata*>(pDataOrg);
		m_nSelect = 0;	// dummy
		// 集合全体が選択状態のハズなので，選択ｵﾌﾞｼﾞｪｸﾄのみ解除
		m_pSelData->SetSelectFlg(FALSE);
		DrawTemporaryProcess(pDC);
	}
}

void CDXFView::OnLButtonUp_Vec
	(const CDXFshape* pShape, const CDXFdata* pData, const CPointD& pt, CDC* pDC)
{
	if ( m_pSelData ) {
		// ｵﾌﾞｼﾞｪｸﾄに対する方向指示
		// 始点は現在位置に遠い方(1-m_nSelect)，終点は近い方(m_nSelect)
		m_nSelect = GAPCALC(m_ptArraw[0][1] - pt) < GAPCALC(m_ptArraw[1][1] - pt) ? 0 : 1;
		m_pSelData->SetDirectionFixed(m_ptArraw[1-m_nSelect][1], m_ptArraw[m_nSelect][1]);
		// 加工指示生成
		CDXFworking* pWork = CreateWorkingData();
		// 加工指示描画
		pWork->DrawTuning(m_dFactor*LOMETRICFACTOR);
		CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_WORKER));
		pDC->SetROP2(R2_COPYPEN);
		pWork->Draw(pDC);
		pDC->SelectObject(pOldPen);
		// 指定ｵﾌﾞｼﾞｪｸﾄﾘｾｯﾄ
		m_pSelData = NULL;
	}
	else {
		if ( pShape->GetShapeFlag() & DXFMAPFLG_DIRECTION ) {
			::MessageBeep(MB_ICONEXCLAMATION);
			return;
		}
		m_enProcessDirect = DXFPROCESS_ARRAW;
		m_pSelData = const_cast<CDXFdata*>(pData);
		// 矢印座標の取得
		pData->GetDirectionArraw(pt, m_ptArraw);
		// 現在位置に近い方の矢印を描画
		m_nSelect = GAPCALC(m_ptArraw[0][1] - pt) < GAPCALC(m_ptArraw[1][1] - pt) ? 0 : 1;
		DrawArraw(pDC);
	}
}

void CDXFView::OnLButtonUp_Out
	(const CDXFshape* pShape, const CDXFdata* pData, const CPointD& pt, CDC* pDC)
{
	if ( m_pSelData ) {
	}
	else {
		if ( pShape->GetShapeAssemble() != DXFSHAPE_OUTLINE ||
				pShape->GetShapeFlag() & (DXFMAPFLG_OUTLINE|DXFMAPFLG_POCKET) ) {
			::MessageBeep(MB_ICONEXCLAMATION);
			return;
		}
		m_enProcessDirect = DXFPROCESS_OUTLINE;
		m_pSelData = const_cast<CDXFdata*>(pData);
		// 輪郭ｵﾌﾞｼﾞｪｸﾄ生成
		if ( !CreateOutlineTempObject(pt) ) {
			AfxMessageBox(IDS_ERR_DXF_CREATEOUTELINE, MB_OK|MB_ICONEXCLAMATION);
			return;
		}
		AllChangeFactor_OutlineTempObject();
		DrawOutline(pDC);
	}
}

void CDXFView::OnRButtonDown(UINT nFlags, CPoint point) 
{
	CViewBase::OnRButtonDown(point);
}

void CDXFView::OnRButtonUp(UINT nFlags, CPoint point) 
{
	switch ( CViewBase::OnRButtonUp(point) ) {
	case 1:		// 拡大処理
		OnLensKey(ID_VIEW_LENSP);
		break;
	case 2:		// 選択ｵﾌﾞｼﾞｪｸﾄのｷｬﾝｾﾙ or ｺﾝﾃｷｽﾄﾒﾆｭｰ表示
		if ( !CancelForSelect() )
			CView::OnRButtonUp(nFlags, point);
		break;
	}
}

void CDXFView::OnMouseMove(UINT nFlags, CPoint point) 
{
	if ( CViewBase::OnMouseMove(nFlags, point) ||
				!m_pSelData || m_vSelect.which()!=DXFTREETYPE_SHAPE )
		return;

	UINT		nShape = GetDocument()->GetShapePattern();
	int			nSelect;
	CDXFshape*	pShape = get<CDXFshape*>(m_vSelect);
	CDXFdata*	pData = NULL;
	double		dGap;
	// CViewBase::OnMouseMove() で処理がなければ
	// ここで論理座標に変換する必要がある
	CClientDC	dc(this);
	CPoint		ptLog(point);
	dc.DPtoLP(&ptLog);
	CPointD		pt(ptLog);
	pt /= ( m_dFactor * LOMETRICFACTOR );
	CRect	rc;
	GetClientRect(&rc);
	dc.DPtoLP(&rc);
	rc.NormalizeRect();
	CRectD	rcView(rc);
	rcView /= ( m_dFactor * LOMETRICFACTOR );

	// 選択集合との距離計算
	tie(pData, dGap) = pShape->GetSelectViewGap(pt, rcView);
	if ( pData && dGap < SELECTGAP/m_dFactor ) {
		switch ( nShape ) {
		case ID_EDIT_SHAPE_SEL:
			if ( m_pSelData != pData ) {
				m_pSelData->SetSelectFlg(TRUE);	// 選択状態を元に戻す
				if ( m_nSelect >= 0 )
					DrawTemporaryProcess(&dc);
			}
			m_nSelect = 0;	// dummy
			m_pSelData = pData;
			m_pSelData->SetSelectFlg(FALSE);
			DrawTemporaryProcess(&dc);
			break;
		case ID_EDIT_SHAPE_VEC:
			nSelect = GAPCALC(m_ptArraw[0][1] - pt) < GAPCALC(m_ptArraw[1][1] - pt) ? 0 : 1;
			if ( m_nSelect != nSelect ) {
				if ( m_nSelect >= 0 ) {
					// 前回の矢印を消去
					DrawArraw(&dc);
				}
				m_nSelect = nSelect;
				// 新しい矢印の描画
				DrawArraw(&dc);
			}
			break;
		case ID_EDIT_SHAPE_OUT:
		case ID_EDIT_SHAPE_POC:
			break;
		}
	}
	else {
		// 閾値を超えると
		if ( m_nSelect >= 0 ) {
			if ( nShape == ID_EDIT_SHAPE_SEL )
				m_pSelData->SetSelectFlg(TRUE);	// 選択状態を元に戻す
			DrawTemporaryProcess(&dc);
			m_nSelect = -1;
		}
	}
}

BOOL CDXFView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
	if ( CViewBase::OnMouseWheel(nFlags, zDelta, pt) )
		OnViewLensComm();

	return TRUE;
}

void CDXFView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if ( nChar == VK_ESCAPE )
		CancelForSelect();

	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

BOOL CDXFView::OnEraseBkgnd(CDC* pDC) 
{
	CRect	rc;
	GetClientRect(&rc);

	CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();
	COLORREF	col1 = pOpt->GetDxfDrawColor(DXFCOL_BACKGROUND2),
				col2 = pOpt->GetDxfDrawColor(DXFCOL_BACKGROUND1);

	return AfxGetNCVCMainWnd()->DrawBackGroundView(pDC, &rc, col1, col2);
}
