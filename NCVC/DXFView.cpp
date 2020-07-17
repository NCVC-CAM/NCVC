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
	static_cast<CDXFChild *>(GetParentFrame())->SetDataInfo(
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
	case UAV_DXFAUTOWORKING:
		GetDocument()->AllChangeFactor(m_dFactor);	// 自動加工指示の拡大率調整
		break;
	case UAV_DXFORGUPDATE:
		ASSERT( pHint );
		reinterpret_cast<CDXFcircleEx*>(pHint)->DrawTuning(m_dFactor*LOMETRICFACTOR);
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
	int		n = 1 - m_nSelect,	// 1->0, 0->1
			nInOut = -1;		// 輪郭指示のみ
	ASSERT( m_vSelect.which()==DXFTREETYPE_SHAPE );
	CDXFdata*		pData;
	CDXFshape*		pShape = get<CDXFshape*>(m_vSelect);
	CDXFworking*	pWork = NULL;

	try {
		switch ( GetDocument()->GetShapePattern() ) {
		case ID_EDIT_SHAPE_VEC:
			pWork = new CDXFworkingDirection(pShape, m_pSelData,
							m_ptArraw[n][1], m_ptArraw[m_nSelect]);
			break;
		case ID_EDIT_SHAPE_START:
			pWork = new CDXFworkingStart(pShape, m_pSelData, m_ptStart[m_nSelect]);
			break;
		case ID_EDIT_SHAPE_OUT:
			if ( !m_ltOutline[m_nSelect].IsEmpty() )
				pWork = new CDXFworkingOutline(pShape, &m_ltOutline[m_nSelect]);
			// m_nSelect分はCDXFworkingOutlineのﾃﾞｽﾄﾗｸﾀにてdelete
			nInOut = n;		// 内外どちらへ指示したか記録しておく
			for ( POSITION pos=m_ltOutline[n].GetHeadPosition(); pos; ) {
				pData = m_ltOutline[n].GetNext(pos);
				if ( pData )
					delete	pData;
			}
			m_ltOutline[0].RemoveAll();
			m_ltOutline[1].RemoveAll();
			break;
//		case ID_EDIT_SHAPE_POC:
//			pWork = new CDXFworkingPocket(pShape);
//			break;
		}
		if ( pWork ) {
			// 加工情報の登録
			pShape->AddWorkingData(pWork, nInOut);
			// 形状ﾂﾘｰ(加工指示)の更新と選択
			GetDocument()->UpdateAllViews(this, UAV_DXFADDWORKING, static_cast<CObject *>(pWork));
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
	int		i, nError = 0;
	CDXFdata*	pData;
	POSITION	pos;

#ifdef _DEBUG
	g_dbg.printf("CreateOutlineTempObject()");
	g_dbg.printf("ShapeName=%s Orig", pShape->GetShapeName());
	CRect	rc( pShape->GetMaxRect() );
	g_dbg.printStruct(&rc);
#endif

	try {
		for ( i=0; i<SIZEOF(m_ltOutline); i++ ) {
			if ( !m_ltOutline[i].IsEmpty() ) {
				for ( pos=m_ltOutline[i].GetHeadPosition(); pos; ) {
					pData = m_ltOutline[i].GetNext(pos);
					if ( pData )
						delete	pData;
				}
				m_ltOutline[i].RemoveAll();
			}
			if ( !pShape->CreateOutlineTempObject(i, &m_ltOutline[i]) ) {
				nError++;	// ｴﾗｰｶｳﾝﾄ
				for ( pos=m_ltOutline[i].GetHeadPosition(); pos; ) {
					pData = m_ltOutline[i].GetNext(pos);
					if ( pData )
						delete	pData;
				}
				m_ltOutline[i].RemoveAll();
#ifdef _DEBUG
				g_dbg.printf("Loop%d Error", i);
#endif
			}
#ifdef _DEBUG
			else {
				g_dbg.printf("Loop%d", i);
				rc = m_ltOutline[i].GetMaxRect();
				g_dbg.printStruct(&rc);
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
	CDXFdata*	pData;
	POSITION	pos;
	for ( int i=0; i<SIZEOF(m_ltOutline); i++ ) {
		for ( pos=m_ltOutline[i].GetHeadPosition(); pos; ) {
			pData = m_ltOutline[i].GetNext(pos);
			if ( pData )
				delete	pData;
		}
		m_ltOutline[i].RemoveAll();
	}
}

BOOL CDXFView::CancelForSelect(CDC* pDC/*=NULL*/)
{
	if ( m_pSelData && m_nSelect >= 0 ) {
		if ( GetDocument()->GetShapePattern() == ID_EDIT_SHAPE_SEL )
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
	CDXFdata*	pData;
	POSITION	pos;
	for ( int i=0; i<SIZEOF(m_ltOutline); i++ ) {
		for ( pos=m_ltOutline[i].GetHeadPosition(); pos; ) {
			pData = m_ltOutline[i].GetNext(pos);
			if ( pData )
				pData->DrawTuning(m_dFactor*LOMETRICFACTOR);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// CDXFView 描画

void CDXFView::OnDraw(CDC* pDC)
{
	ASSERT_VALID(GetDocument());
	int		i, j, nLayerCnt, nDataCnt;
	DWORD	dwSel, dwSelBak = 0;
	CLayerData*		pLayer;
	CDXFdata*		pData;
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();

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
	switch ( GetDocument()->GetShapePattern() ) {
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
		ptDraw[i][1] = m_ptArraw[i][1] * m_dFactor * LOMETRICFACTOR;
		ptDraw[i][0] = m_ptArraw[i][0] + ptDraw[i][1];
		ptDraw[i][2] = m_ptArraw[i][2] + ptDraw[i][1];
	}
	CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_WORKER));
	pDC->SetROP2(R2_XORPEN);
	pDC->Polyline(ptDraw[m_nSelect], SIZEOF(ptDraw[0]));
	pDC->SelectObject(pOldPen);
}

void CDXFView::DrawTempStart(CDC* pDC)
{
	ASSERT( 0<=m_nSelect && m_nSelect<=SIZEOF(m_ptStart) );

	// CDXFpointに準拠
	CRect	rcDraw;
	CPointD	pt( m_ptStart[m_nSelect] * m_dFactor * LOMETRICFACTOR);
	// 位置を表す丸印は常に2.5論理理位
	rcDraw.TopLeft()		= pt - LOMETRICFACTOR*2.5;
	rcDraw.BottomRight()	= pt + LOMETRICFACTOR*2.5;

	CBrush* pOldBrush = (CBrush *)pDC->SelectStockObject(NULL_BRUSH);
	CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_WORKER));
	pDC->SetROP2(R2_XORPEN);
	pDC->Ellipse(&rcDraw);
	pDC->SelectObject(pOldPen);
	pDC->SelectObject(pOldBrush);
}

void CDXFView::DrawTempOutline(CDC* pDC)
{
	ASSERT( 0<=m_nSelect && m_nSelect<=SIZEOF(m_ltOutline) );
	CDXFdata*	pData;

	if ( !m_ltOutline[m_nSelect].IsEmpty() ) {
		CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_WORKER));
		CBrush* pOldBrush = (CBrush *)pDC->SelectStockObject(NULL_BRUSH);
		pDC->SetROP2(R2_XORPEN);
		for ( POSITION pos=m_ltOutline[m_nSelect].GetHeadPosition(); pos; ) {
			pData = m_ltOutline[m_nSelect].GetNext(pos);
			if ( pData )
				pData->Draw(pDC);
		}
		pDC->SelectObject(pOldPen);
		pDC->SelectObject(pOldBrush);
	}
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
	return static_cast<CDXFDoc *>(m_pDocument);
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
		static_cast<CDXFChild *>(GetParentFrame())->OnUpdateMouseCursor(&ptd);
	}
	else
		static_cast<CDXFChild *>(GetParentFrame())->OnUpdateMouseCursor();
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
	static_cast<CDXFChild *>(GetParentFrame())->SetFactorInfo(m_dFactor);
	// ﾋﾞｭｰの再描画
	Invalidate();
}

/////////////////////////////////////////////////////////////////////////////
// CDXFView メッセージ ハンドラ

int CDXFView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if ( CView::OnCreate(lpCreateStruct) < 0 )
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
	// 座標値計算(pointはCViewBase::OnLButtonUp()で論理座標に変換済み)
	CPointD	pt(point);
	pt /= ( m_dFactor * LOMETRICFACTOR );
	CRect	rc;
	GetClientRect(rc);
	dc.DPtoLP(rc);
	rc.NormalizeRect();
	CRectD	rcView(rc);
	rcView /= ( m_dFactor * LOMETRICFACTOR );

	// ﾏｳｽｸﾘｯｸ状態遷移
	if ( m_vSelect.which() == DXFTREETYPE_SHAPE ) {
		BOOL	bResult = FALSE;
		switch ( GetDocument()->GetShapePattern() ) {
		case ID_EDIT_SHAPE_SEL:		// 分離
			bResult = OnLButtonUp_Select(&dc, pt, rcView);
			break;
		case ID_EDIT_SHAPE_VEC:		// 方向
			bResult = OnLButtonUp_Vector(&dc, pt, rcView);
			break;
		case ID_EDIT_SHAPE_START:	// 開始位置
			bResult = OnLButtonUp_Start(&dc, pt, rcView);
			break;
		case ID_EDIT_SHAPE_OUT:		// 輪郭
//		case ID_EDIT_SHAPE_POC:		// ﾎﾟｹｯﾄ
			bResult = OnLButtonUp_Outline(&dc, pt, rcView);
			break;
		}
		if ( bResult )	// 処理済み
			return;
	}

	// 指定座標との距離が閾値以上か集合非選択状態なら，全体検索
	double		dGap;
	CDXFshape*	pShape = NULL;
	tie(pShape, dGap) = GetDocument()->GetSelectObject(pt, rcView);
	if ( pShape && dGap < SELECTGAP/m_dFactor ) {
		// ﾂﾘｰの選択と m_vSelect の更新通知
		GetDocument()->UpdateAllViews(this, UAV_DXFSHAPEUPDATE, pShape);
	}
	else {
		CancelForSelect(&dc);
	}
}

BOOL CDXFView::OnLButtonUp_Select
	(CDC* pDC, const CPointD& ptView, const CRectD& rcView)
{
	CDXFshape*	pShapeSel = get<CDXFshape*>(m_vSelect);
	CDXFdata*	pDataSel = NULL;
	double		dGap;
	dGap = pShapeSel->GetSelectObjectFromShape(ptView, &rcView, &pDataSel);
	if ( !pDataSel || dGap >= SELECTGAP/m_dFactor )
		return FALSE;

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
				CPointD	pt;
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
									pMap->GetMapTypeFlag(), pMap);
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
				return TRUE;	// 処理済扱い
			}
			// 方向指示の確認
			CDXFworking*	pWork;
			CDXFdata*	pData;
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
		// ﾄﾞｷｭﾒﾝﾄ変更通知
		GetDocument()->SetModifiedFlag();
		// 再描画
		Invalidate();
	}
	else {
		// ｵﾌﾞｼﾞｪｸﾄを１つしか持たない集合は分離できない
		if ( pShapeSel->GetObjectCount() <= 1 )
			::MessageBeep(MB_ICONEXCLAMATION);
		else {
			m_pSelData = pDataSel;
			m_nSelect = 0;	// dummy
			// 集合全体が選択状態のハズなので，選択ｵﾌﾞｼﾞｪｸﾄのみ解除
			m_pSelData->SetSelectFlg(FALSE);
			DrawTemporaryProcess(pDC);
		}
	}

	return TRUE;
}

BOOL CDXFView::OnLButtonUp_Vector
	(CDC* pDC, const CPointD& ptView, const CRectD& rcView)
{
	CDXFshape*	pShapeSel = get<CDXFshape*>(m_vSelect);
	CDXFdata*	pDataSel = NULL;
	double		dGap;
	dGap = pShapeSel->GetSelectObjectFromShape(ptView, &rcView, &pDataSel);
	if ( !pDataSel || dGap >= SELECTGAP/m_dFactor )
		return FALSE;

	if ( m_pSelData ) {
/*
	---
		ここで SetDirectionFixed() を呼び出して座標を入れ替えると、
		輪郭ｵﾌﾞｼﾞｪｸﾄとのリンクが取れなくなる。
		// ｵﾌﾞｼﾞｪｸﾄに対する方向指示
		// 始点は現在位置に遠い方(1-m_nSelect)，終点は近い方(m_nSelect)
		m_nSelect = GAPCALC(m_ptArraw[0][1] - ptView) < GAPCALC(m_ptArraw[1][1] - ptView) ? 0 : 1;
		m_pSelData->SetDirectionFixed(m_ptArraw[1-m_nSelect][1], m_ptArraw[m_nSelect][1]);
	---
*/
		// 加工指示生成
		CDXFworking* pWork = CreateWorkingData();
		// 加工指示描画
		if ( pWork ) {
			pWork->DrawTuning(m_dFactor*LOMETRICFACTOR);
			CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_WORKER));
			pDC->SetROP2(R2_COPYPEN);
			pWork->Draw(pDC);
			pDC->SelectObject(pOldPen);
		}
		// 指定ｵﾌﾞｼﾞｪｸﾄﾘｾｯﾄ
		m_pSelData = NULL;
	}
	else {
		if ( pShapeSel->GetShapeFlag() & DXFMAPFLG_DIRECTION )
			::MessageBeep(MB_ICONEXCLAMATION);
		else {
			m_pSelData = pDataSel;
			// 矢印座標の取得
			m_pSelData->GetDirectionArraw(ptView, m_ptArraw);
			// 現在位置に近い方の矢印を描画
			m_nSelect = GAPCALC(m_ptArraw[0][1] - ptView) < GAPCALC(m_ptArraw[1][1] - ptView) ? 0 : 1;
			DrawTempArraw(pDC);
		}
	}

	return TRUE;
}

BOOL CDXFView::OnLButtonUp_Start
	(CDC* pDC, const CPointD& ptView, const CRectD& rcView)
{
	CDXFshape*	pShapeSel = get<CDXFshape*>(m_vSelect);
	CDXFdata*	pDataSel = NULL;
	double		dGap;
	dGap = pShapeSel->GetSelectObjectFromShape(ptView, &rcView, &pDataSel);
	if ( !pDataSel || dGap >= SELECTGAP/m_dFactor )
		return FALSE;

	if ( m_pSelData ) {
		// 加工指示生成
		CDXFworking* pWork = CreateWorkingData();
		// 加工指示描画
		if ( pWork ) {
			pWork->DrawTuning(m_dFactor*LOMETRICFACTOR);
			CPen*	pOldPen	  = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_WORKER));
			CBrush*	pOldBrush = pDC->SelectObject(AfxGetNCVCMainWnd()->GetBrushDXF(DXFBRUSH_START));
			pDC->SetROP2(R2_COPYPEN);
			pWork->Draw(pDC);
			pDC->SelectObject(pOldBrush);
			pDC->SelectObject(pOldPen);
		}
		// 指定ｵﾌﾞｼﾞｪｸﾄﾘｾｯﾄ
		m_pSelData = NULL;
	}
	else {
		if ( pShapeSel->GetShapeFlag() & DXFMAPFLG_START )
			::MessageBeep(MB_ICONEXCLAMATION);
		else {
			m_pSelData = pDataSel;
			// ｵﾌﾞｼﾞｪｸﾄの座標の取得
			ASSERT( m_pSelData->GetPointNumber() <= SIZEOF(m_ptStart) );
			double	dGapMin = HUGE_VAL;
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

	return TRUE;
}

BOOL CDXFView::OnLButtonUp_Outline
	(CDC* pDC, const CPointD& ptView, const CRectD& rcView)
{
	double		dGap1, dGap2;

	if ( m_pSelData ) {
		int			nSelect;
		CDXFdata*	pData1;
		CDXFdata*	pData2;
		dGap1 = m_ltOutline[0].GetSelectObjectFromShape(ptView, &rcView, &pData1);
		dGap2 = m_ltOutline[1].GetSelectObjectFromShape(ptView, &rcView, &pData2);
		if ( dGap1 > dGap2 ) {
			dGap1 = dGap2;
			pData1 = pData2;
			nSelect = 1;
		}
		else
			nSelect = 0;
		if ( !pData1 || dGap1 >= SELECTGAP/m_dFactor )
			return FALSE;
		// 加工指示生成
		m_nSelect = nSelect;
		CDXFworking* pWork = CreateWorkingData();
		// 加工指示描画
		if ( pWork ) {
			pWork->DrawTuning(m_dFactor*LOMETRICFACTOR);
			CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_WORKER));
			CBrush* pOldBrush = (CBrush *)pDC->SelectStockObject(NULL_BRUSH);
			pDC->SetROP2(R2_COPYPEN);
			pWork->Draw(pDC);
			pDC->SelectObject(pOldBrush);
			pDC->SelectObject(pOldPen);
		}
		// 指定ｵﾌﾞｼﾞｪｸﾄﾘｾｯﾄ
		m_pSelData = NULL;
	}
	else {
		CDXFshape*	pShapeSel = get<CDXFshape*>(m_vSelect);
		CDXFdata*	pDataSel  = NULL;
		dGap1 = pShapeSel->GetSelectObjectFromShape(ptView, &rcView, &pDataSel);
		if ( !pDataSel || dGap1 >= SELECTGAP/m_dFactor )
			return FALSE;
		if ( pShapeSel->GetShapeAssemble() != DXFSHAPE_OUTLINE ||
				pShapeSel->GetShapeFlag() & (DXFMAPFLG_OUTLINE|DXFMAPFLG_POCKET) ) {
			::MessageBeep(MB_ICONEXCLAMATION);
			return TRUE;	// 処理済扱い
		}
		m_pSelData = pDataSel;
		// 輪郭ｵﾌﾞｼﾞｪｸﾄ生成
		if ( !CreateOutlineTempObject(pShapeSel) ) {
			AfxMessageBox(IDS_ERR_DXF_CREATEOUTELINE, MB_OK|MB_ICONEXCLAMATION);
			return TRUE;
		}
		// ﾏｳｽ座標から内外ｵﾌﾞｼﾞｪｸﾄの選択
		dGap1 = m_ltOutline[0].GetSelectObjectFromShape(ptView, &rcView);
		dGap2 = m_ltOutline[1].GetSelectObjectFromShape(ptView, &rcView);
		m_nSelect = dGap1 < dGap2 ? 0 : 1;
		//
		AllChangeFactor_OutlineTempObject();
		DrawTempOutline(pDC);
	}

	return TRUE;
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

	// CViewBase::OnMouseMove() で処理がなければ
	// ここで論理座標に変換する必要がある
	CClientDC	dc(this);
	CPoint		ptLog(point);
	dc.DPtoLP(&ptLog);
	CPointD		pt(ptLog);
	pt /= ( m_dFactor * LOMETRICFACTOR );
	CRect	rc;
	GetClientRect(rc);
	dc.DPtoLP(rc);
	rc.NormalizeRect();
	CRectD	rcView(rc);
	rcView /= ( m_dFactor * LOMETRICFACTOR );

	UINT	nShape = GetDocument()->GetShapePattern();
	BOOL	bResult = FALSE;

	// 選択集合との距離計算
	switch ( nShape ) {
	case ID_EDIT_SHAPE_SEL:
		bResult = OnMouseMove_Select(&dc, pt, rcView);
		break;
	case ID_EDIT_SHAPE_VEC:
		bResult = OnMouseMove_Vector(&dc, pt, rcView);
		break;
	case ID_EDIT_SHAPE_START:
		bResult = OnMouseMove_Start(&dc, pt, rcView);
		break;
	case ID_EDIT_SHAPE_OUT:
//	case ID_EDIT_SHAPE_POC:
		bResult = OnMouseMove_Outline(&dc, pt, rcView);
		break;
	}

	// 閾値を超えると
	if ( !bResult && m_nSelect >= 0 ) {
		if ( nShape == ID_EDIT_SHAPE_SEL )
			m_pSelData->SetSelectFlg(TRUE);	// 選択状態を元に戻す
		// 現在の加工指示の仮表示を消去
		DrawTemporaryProcess(&dc);
		m_nSelect = -1;
	}
}

BOOL CDXFView::OnMouseMove_Select
	(CDC* pDC, const CPointD& ptView, const CRectD& rcView)
{
	CDXFshape*	pShape = get<CDXFshape*>(m_vSelect);
	CDXFdata*	pData = NULL;
	double		dGap = pShape->GetSelectObjectFromShape(ptView, &rcView, &pData);
	if ( !pData || dGap >= SELECTGAP/m_dFactor )
		return FALSE;

	if ( m_pSelData != pData ) {
		m_pSelData->SetSelectFlg(TRUE);	// 選択状態を元に戻す
		if ( m_nSelect >= 0 )
			DrawTemporaryProcess(pDC);
	}
	m_nSelect = 0;	// dummy
	m_pSelData = pData;
	m_pSelData->SetSelectFlg(FALSE);
	DrawTemporaryProcess(pDC);

	return TRUE;
}

BOOL CDXFView::OnMouseMove_Vector
	(CDC* pDC, const CPointD& ptView, const CRectD& rcView)
{
	CDXFshape*	pShape = get<CDXFshape*>(m_vSelect);
	CDXFdata*	pData = NULL;
	double		dGap = pShape->GetSelectObjectFromShape(ptView, &rcView, &pData);
	if ( !pData || dGap >= SELECTGAP/m_dFactor )
		return FALSE;

	int nSelect = GAPCALC(m_ptArraw[0][1] - ptView) < GAPCALC(m_ptArraw[1][1] - ptView) ? 0 : 1;
	if ( m_nSelect != nSelect ) {
		if ( m_nSelect >= 0 ) {
			// 前回の矢印を消去
			DrawTempArraw(pDC);
		}
		m_nSelect = nSelect;
		// 新しい矢印の描画
		DrawTempArraw(pDC);
	}

	return TRUE;
}

BOOL CDXFView::OnMouseMove_Start
	(CDC* pDC, const CPointD& ptView, const CRectD& rcView)
{
	CDXFshape*	pShape = get<CDXFshape*>(m_vSelect);
	CDXFdata*	pData = NULL;
	double		dGap = pShape->GetSelectObjectFromShape(ptView, &rcView, &pData);
	if ( !pData || dGap >= SELECTGAP/m_dFactor )
		return FALSE;

	int		i, nSelect;
	double	dGapMin = HUGE_VAL;

	for ( i=0; i<m_pSelData->GetPointNumber(); i++ ) {
		dGap = GAPCALC(m_ptStart[i] - ptView);
		if ( dGap < dGapMin ) {
			dGapMin = dGap;
			nSelect = i;
		}
	}

	if ( m_nSelect != nSelect ) {
		if ( m_nSelect >= 0 ) {
			// 前回の開始位置を消去
			DrawTempStart(pDC);
		}
		m_nSelect = nSelect;
		// 新しい開始位置の描画
		DrawTempStart(pDC);
	}

	return TRUE;
}

BOOL CDXFView::OnMouseMove_Outline
	(CDC* pDC, const CPointD& ptView, const CRectD& rcView)
{
	int			nSelect;
	double		dGap1 = m_ltOutline[0].GetSelectObjectFromShape(ptView, &rcView),
				dGap2 = m_ltOutline[1].GetSelectObjectFromShape(ptView, &rcView);

	if ( dGap1 > dGap2 ) {
		dGap1 = dGap2;
		nSelect = 1;
	}
	else
		nSelect = 0;
	if ( dGap1 >= SELECTGAP/m_dFactor )
		return FALSE;

	if ( m_nSelect != nSelect ) {
		if ( m_nSelect >= 0 ) {
			// 前回の輪郭を消去
			DrawTempOutline(pDC);
		}
		m_nSelect = nSelect;
		// 新しい矢印の描画
		DrawTempOutline(pDC);
	}

	return TRUE;
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
	GetClientRect(rc);

	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	COLORREF	col1 = pOpt->GetDxfDrawColor(DXFCOL_BACKGROUND2),
				col2 = pOpt->GetDxfDrawColor(DXFCOL_BACKGROUND1);

	return AfxGetNCVCMainWnd()->DrawBackGroundView(pDC, &rc, col1, col2);
}
