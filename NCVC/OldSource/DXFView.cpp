// DXFView.cpp : �C���v�������e�[�V���� �t�@�C��
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

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

using namespace boost;

// �w����W�Ƃ�臒l
static	const	double	SELECTGAP = 5.0;
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
	ON_WM_RBUTTONDBLCLK()
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
	// հ�޲Ƽ�ُ��� & �e�ޭ��ւ�̨��ү����
	ON_MESSAGE(WM_USERVIEWFITMSG, &CDXFView::OnUserViewFitMsg)
	// ϳ��ړ��̽ð���ް�X�V
	ON_UPDATE_COMMAND_UI(ID_DXFST_MOUSE, &CDXFView::OnUpdateMouseCursor)
	// CAD�ް��̓���
	ON_COMMAND(ID_EDIT_BIND_DEL, &CDXFView::OnEditBindDel)
	ON_COMMAND(ID_EDIT_BIND_TARGET, &CDXFView::OnEditBindTarget)
	ON_MESSAGE(WM_USERBINDINIT, &CDXFView::OnBindInitMsg)
	ON_MESSAGE(WM_USERBINDMOVE, &CDXFView::OnBindMoveMsg)
	ON_MESSAGE(WM_USERBINDROUND, &CDXFView::OnBindRoundMsg)
	// �ړ�
	ON_COMMAND_RANGE(ID_VIEW_UP, ID_VIEW_RT, &CDXFView::OnMoveKey)
	ON_COMMAND_RANGE(ID_VIEW_FIT, ID_VIEW_LENSN, &CDXFView::OnLensKey)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDXFView �N���X�̍\�z/����

CDXFView::CDXFView()
{
	m_nSelect = m_nSelView = -1;
	m_pSelData = NULL;
	m_bMove = FALSE;
}

CDXFView::~CDXFView()
{
	PLIST_FOREACH(auto p, &m_bindUndo)
		delete	p;
	END_FOREACH
}

/////////////////////////////////////////////////////////////////////////////
// CDXFView �N���X�̃I�[�o���C�h�֐�

BOOL CDXFView::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	if ( IsBindMode() ) {
		CView*		pViewParent = static_cast<CView *>(GetParent());
		ASSERT( pViewParent );
		return pViewParent->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
	}
	return __super::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

void CDXFView::OnInitialUpdate() 
{
	CView::OnInitialUpdate();

	if ( !IsBindMode() ) {
		if ( !IsBindParent() ) {
			// MDI�q�ڰт̽ð���ް�ɏ��\��
			// -- IsBindParent()�����̎��_�ł͎q��DataCnt���ς�ł��Ȃ�
			// -- OnBindInitMsg()�Ŏ��s
			static_cast<CDXFChild*>(GetParentFrame())->SetDataInfo(
				static_cast<CDXFDoc*>(GetDocument()) );
		}
		// �رى���̐}�`̨��ү���ނ̑��M
		// OnInitialUpdate()�֐����ł́CGetClientRect()�̻��ނ��������Ȃ�
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
		return;		// �ĕ`��s�v
	case UAV_DXFAUTOWORKING:
		GetDocument()->AllChangeFactor(m_dFactor);	// �������H�w���̊g�嗦����
		break;
	case UAV_DXFORGUPDATE:
		ASSERT( pHint );
		reinterpret_cast<CDXFcircleEx*>(pHint)->DrawTuning(m_dFactor);
		if ( IsBindParent() ) {
			// �q�̉��H���_���X�V
			CClientDC	dc(this);
			optional<CPointD>	ptOrg = GetDocument()->GetCutterOrigin();
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
				return;	// �ĕ`��s�v
		}
		break;
	case UAV_ADDINREDRAW:
		OnViewLensComm();
		return;
	}
	CView::OnUpdate(pSender, lHint, pHint);
}

#ifdef _DEBUG
CDXFDoc* CDXFView::GetDocument() // ��f�o�b�O �o�[�W�����̓C�����C���ł��B
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CDXFDoc)));
	return static_cast<CDXFDoc *>(m_pDocument);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CDXFView �N���X�̃����o�֐�

BOOL CDXFView::OnUpdateShape(DXFTREETYPE vSelect[])
{
#ifdef _DEBUG
	CMagaDbg	dbg("CDXFView::OnUpdateShape\nStart");
#endif
	CClientDC	dc(this);
	// �����H�w����`�撆�Ȃ�
	CancelForSelect(&dc);

	// �I�����ꂽ�`���ذ�̉��
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

	// �`��̕`��
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

	return bReDraw;	// �ĕ`��
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
			nInOut = -1;		// �֊s�w���̂�
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
			// m_nSelect����CDXFworkingOutline���޽�׸��ɂ�delete
			nInOut = n;		// ���O�ǂ���֎w���������L�^���Ă���
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
			// ���H���̓o�^
			if ( nInOut < 0 )
				pShape->AddWorkingData(pWork);
			else
				pShape->AddOutlineData(static_cast<CDXFworkingOutline*>(pWork), nInOut);
			// �`���ذ(���H�w��)�̍X�V�ƑI��
			GetDocument()->UpdateAllViews(this, UAV_DXFADDWORKING, pWork);
			// �޷���ĕύX�ʒm
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

	// �̾�Ēl�̌���
	m_dOffset = pShape->GetOffset();	// �ް��l
	if ( pOutline )
		m_dOffset += pOutline->GetOutlineOffset();

#ifdef _DEBUG
	g_dbg.printf("CreateOutlineTempObject()");
	g_dbg.printf("ShapeName=%s Orig", pShape->GetShapeName());
	CRect	rc( pShape->GetMaxRect() );
	g_dbg.printStruct(&rc);
#endif

	DeleteOutlineTempObject();

	try {
		for ( i=0; i<SIZEOF(m_ltOutline); i++ ) {
			if ( !pShape->CreateOutlineTempObject(i, &m_ltOutline[i], m_dOffset) ) {
				nError++;	// �װ����
				PLIST_FOREACH(CDXFdata* pData, &m_ltOutline[i])
					if ( pData )
						delete	pData;
				END_FOREACH
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

	return nError < SIZEOF(m_ltOutline);	// �Ȃ�TRUE
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
			m_pSelData->SetDxfFlg(DXFFLG_SELECT);	// �I����Ԃ����ɖ߂�
		DrawTemporaryProcess(pDC ? pDC : &CClientDC(this));
	}
	if ( m_nSelView >= 0 ) {
		LPCADBINDINFO pInfo = GetDocument()->GetBindInfoData(m_nSelView);
		BindMsgPost(pDC ? pDC : &CClientDC(this), pInfo, NULL);
		GetParentFrame()->SetActiveView(this, FALSE);
		Invalidate();
	}

	BOOL	bResult = (m_pSelData||m_nSelView>=0) ? TRUE : FALSE;
	m_nSelect = m_nSelView = -1;
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
	CRectD			rc;
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
	// �ʒu�L�������޲����W�ւ̕ϊ�
	CPoint	pt(pInfo->pt*m_dFactor);
	pDC->LPtoDP(&pt);

	// �ް��̍ő��`�����޲����W�ɕϊ�
	CRectD	rc(pInfo->pDoc->GetMaxRect());
	CSize	sz((int)(rc.Width()*m_dFactor), (int)(rc.Height()*m_dFactor));
	pDC->LPtoDP(&sz);

	// �z�u
//	pInfo->pView->MoveWindow(pt.x, pt.y, sz.cx, sz.cy);
	pInfo->pView->SetWindowPos(NULL, pt.x, pt.y, sz.cx, sz.cy,
		SWP_NOZORDER|SWP_NOACTIVATE);

	// �z�u���W��ذݍ��W�Ŏq����޳�ɒʒm
	ClientToScreen(&pt);
	CPointD	ptOffset(pt);
	pInfo->pView->SendMessage(WM_USERVIEWFITMSG,
			reinterpret_cast<WPARAM>(ptDev),
			reinterpret_cast<LPARAM>(&ptOffset));
	// pt �Ɏq����޳�̕`�挴�_�̾�Ă��Ԃ�
	pInfo->ptOffset = ptOffset;
}

/////////////////////////////////////////////////////////////////////////////
// CDXFView �`��

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
	CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_ORIGIN));

	if ( !IsBindMode() ) {
		// ���_�`��
		if ( pData=GetDocument()->GetCircleObject() )
			pData->Draw(pDC);	// CDXFcircleEx::Draw()
		// ���Ռ��_
		for ( i=0; i<2; i++ ) {
			if ( pData=GetDocument()->GetLatheLine(i) )
				pData->Draw(pDC);	// CDXFline::Draw()
		}
	}

	// DXF�`��
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
		// ���H�w���̕`��
		pLayer->DrawWorking(pDC);
	}

	// ���݂̊g���`��`��(ViewBase.cpp)
	if ( m_bMagRect )
		DrawMagnifyRect(pDC);
	// �����H�w���̕`��
	if ( m_pSelData && m_nSelect>=0 )
		DrawTemporaryProcess(pDC);
	// ܰ���`(bind)
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

	// ���̒������g�嗦�ɉe������Ȃ��悤�Ɍv�Z
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

	// CDXFpoint�ɏ���
	CRect	rcDraw;
	CPointD	pt( m_ptStart[m_nSelect] * m_dFactor );
	// �ʒu��\���ۈ�͏��2.5�_���P��
	double	dFactor = LOMETRICFACTOR * 2.5;
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
// CDXFView �N���X�̃��b�Z�[�W �n���h���i���j���[�ҁj

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
	optional<CPointD>	ptOrg = GetDocument()->GetCutterOrigin();
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
	CWaitCursor		wait;	// �����v����

	CClientDC		dc(this);
	CMetaFileDC		metaDC;
	metaDC.CreateEnhanced(&dc, NULL, NULL, NULL);
	metaDC.SetMapMode(MM_LOMETRIC);
	metaDC.SetBkMode(TRANSPARENT);
	metaDC.SelectStockObject(NULL_BRUSH);
	AfxGetNCVCMainWnd()->SelectGDI(FALSE);	// GDI��޼ު�Ă̐ؑ�
	OnDraw(&metaDC);
	AfxGetNCVCMainWnd()->SelectGDI();

	// �د���ް�ނւ��ް���߰ ViewBase.cpp
	CopyNCDrawForClipboard(metaDC.CloseEnhanced());
}

void CDXFView::OnEditBindDel()
{
	if ( m_nSelView >= 0 ) {
		GetDocument()->RemoveBindData(m_nSelView);
		Invalidate();
		GetDocument()->SetModifiedFlag();
	}
}

void CDXFView::OnEditBindTarget()
{
	if ( m_nSelView >= 0 ) {
		LPCADBINDINFO pInfo = GetDocument()->GetBindInfoData(m_nSelView);
		pInfo->bTarget = !pInfo->bTarget;
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
		// CLayerDlg::OnCancel() �̊ԐڌĂяo��
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
		optional<CPointD>	ptOrg = GetDocument()->GetCutterOrigin();
		if ( pView==this && ptOrg ) {
			POINT	pt;
			::GetCursorPos(&pt);
			ScreenToClient(&pt);
			CClientDC	dc(this);
			dc.DPtoLP(&pt);
			CPointD	ptd( pt.x/m_dFactor, pt.y/m_dFactor );
			// ���_����̋���
			ptd -= *ptOrg;
			static_cast<CDXFChild *>(GetParentFrame())->OnUpdateMouseCursor(&ptd);
		}
		else
			static_cast<CDXFChild *>(GetParentFrame())->OnUpdateMouseCursor();
	}
}

void CDXFView::OnMoveKey(UINT nID) 
{
	CViewBase::OnMoveKey(nID);
	if ( IsBindParent() )
		BindMove(FALSE);
}

void CDXFView::OnLensKey(UINT nID)
{
	CRectD		rc;
	CDXFdata*	pData;

	switch ( nID ) {
	case ID_VIEW_FIT:
		rc = GetDocument()->GetMaxRect();
		if ( !IsBindMode() ) {
			// ���_���
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
	// �g�嗦�ɂ��`����̍X�V
	GetDocument()->AllChangeFactor(m_dFactor);
	// �ꎞ��޼ު�ĕ�
	AllChangeFactor_OutlineTempObject();
	//
	if ( IsBindParent() ) {
		// ܰ���`
		m_rcDrawWork = DrawConvert(GetDocument()->GetMaxRect());
		// �����ް��̍Ĕz�u
		BindMove(TRUE);
	}
	if ( !IsBindMode() ) {
		// MDI�q�ڰт̽ð���ް�ɏ��\��
		static_cast<CDXFChild *>(GetParentFrame())->SetFactorInfo(m_dFactor/LOMETRICFACTOR);
	}
	// �ޭ��̍ĕ`��
	Invalidate();
}

/////////////////////////////////////////////////////////////////////////////
// CDXFView ���b�Z�[�W �n���h��

void CDXFView::OnDestroy()
{
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
		CRectD	rc;
		rc = GetDocument()->GetMaxRect();
		CViewBase::OnViewFit(rc, FALSE);
		OnViewLensComm();
		CClientDC	dc(this);
		if ( wParam ) {
			// ��ذݍ��W������H���_���X�V
			CPoint*	ptParam = reinterpret_cast<CPoint *>(wParam);
			CPoint	ptD(ptParam->x, ptParam->y);
			ScreenToClient(&ptD);
			dc.DPtoLP(&ptD);
			CPointD	ptL(ptD.x/m_dFactor, ptD.y/m_dFactor);
			GetDocument()->CreateCutterOrigin(ptL);
#ifdef _DEBUG
			g_dbg.printf("%s", GetDocument()->GetPathName());
			g_dbg.printf("-- New Origin = %f, %f", ptL.x, ptL.y);
#endif
		}
		if ( lParam ) {
			// �z�u���W�̵̾�Čv�Z
			CPointD* ptParam = reinterpret_cast<CPointD *>(lParam);
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
	CMagaDbg	dbg("OnBindInitMsg");
#endif
	int		i, nLoop = GetDocument()->GetBindInfoCnt();
	size_t	j;
	BOOL	bResult, bXover = FALSE;
	double	dNextBaseY, dMargin = AfxGetNCVCApp()->GetDXFOption()->GetBindMargin();
	CClientDC		dc(this);
	LPCADBINDINFO	pInfo;
	CRectD	rcWork( GetDocument()->GetMaxRect() ), rc;
	CPointD	pt1(0, rcWork.bottom), pt2;
	double	W = rcWork.Width(), w, h, dw, dh;

	// �q�ް����Z��̕\���X�V
	static_cast<CDXFChild*>(GetParentFrame())->SetDataInfo(
		static_cast<CDXFDoc*>(GetDocument()) );
	if ( wParam )
		return 0;		// from CDXFDoc::OnOpenDocument()

	// ���H���_��ذݍ��W�ɕϊ�
	optional<CPointD>	ptOrg = GetDocument()->GetCutterOrigin();
	ASSERT( ptOrg );
	CPoint	ptDev = *ptOrg * m_dFactor;
	dc.LPtoDP(&ptDev);
	ClientToScreen(&ptDev);
#ifdef _DEBUG
	dbg.printf("rcWork = (%f, %f) - (%f, %f)",
		rcWork.left, rcWork.top, rcWork.right, rcWork.bottom);
	dbg.printf("ptDev  = (%f, %f)", ptDev.x, ptDev.y);
#endif

	// --- �����`�l�ߍ��ݖ��---
	std::vector<CVPointD>	YY;	// �z�u�\��Y�̓񎟌��ϔz��
	CVPointD	Y;				// �z�u�_
	CVPointD::iterator	it;

	// 1�ߔz�u
	ASSERT( nLoop > 0 );
	pInfo = GetDocument()->GetBindInfoData(0);
	pInfo->pt = pt1;
	BindMsgPost(&dc, pInfo, &ptDev);
	rc = pInfo->pDoc->GetMaxRect();
	if ( W < rc.Width() )
		bXover = TRUE;
	pt1.x = rc.Width() + dMargin;
	// �z�u�_�o�^
	dNextBaseY = pt2.y = pt1.y - rc.Height() + dMargin;
	Y.push_back( pt2 );
#ifdef _DEBUG
	dbg.printf("first Y = (%f, %f)", pt2.x, pt2.y);
#endif

	// 2�߈ȍ~
	for ( i=1; i<nLoop; i++ ) {
		pInfo = GetDocument()->GetBindInfoData(i);
		rc = pInfo->pDoc->GetMaxRect();
		w = rc.Width()  + dMargin;
		h = rc.Height() + dMargin;
#ifdef _DEBUG
		dbg.printf("w=%f h=%f", w, h);
#endif
		// ������
		if ( pt1.x+w < W ) {
			pInfo->pt = pt1;
			BindMsgPost(&dc, pInfo, &ptDev);
			if ( pt1.y - Y.back().y > dMargin ) {
				pt2 = pt1;	pt2.y -= h;
				Y.push_back( pt2 );
#ifdef _DEBUG
				dbg.printf("(%f, %f)", pt2.x, pt2.y);
#endif
			}
			pt1.x += w;
			continue;
		}

		if ( !Y.empty() ) {
			YY.push_back(Y);
			Y.clear();
		}

		// �����Ԃ̍�������
		bResult = FALSE;
		for ( j=0; j<YY.size() && !bResult; j++ ) {
			for ( it=YY[j].begin()+1; it!=YY[j].end(); ++it ) {
				pt1 = *it;
				pt2 = *boost::prior(it);
				// �����Ԍ������O����(�����ް���X�l������)
				if ( fabs(pt1.x-pt2.x) < NCMIN )
					continue;
				dw = W - pt1.x;
				dh = pt1.y - pt2.y;
				if ( dh>=h && dw>=w ) {
					pInfo->pt = pt1;
					BindMsgPost(&dc, pInfo, &ptDev);
					// �z�u�_�̓o�^
					if ( dh - h > dMargin ) {
						pt2 = pt1;	pt2.y -= h;
						YY[j].insert(it, pt2);
						Y.push_back( pt2 );
#ifdef _DEBUG
						dbg.printf("sukima (%f, %f)", pt2.x, pt2.y);
#endif
						pt1.x += w;
					}
					bResult = TRUE;
					break;
				}
			}
		}

		// ���ɂ������Ԃɂ�����Ȃ�
		if ( !bResult ) {
			pt1.x = 0;
			pt1.y = dNextBaseY;
			pInfo->pt = pt1;
			BindMsgPost(&dc, pInfo, &ptDev);
			pt2 = pt1;	pt2.y -= h;
			Y.push_back( pt2 );
#ifdef _DEBUG
			dbg.printf("new (%f, %f)", pt2.x, pt2.y);
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
			if ( (*it).y < 0 ) {
				bResult = TRUE;
				break;
			}
		}
	}
	if ( bResult || bXover )
		AfxMessageBox(IDS_ERR_BINDAREAOVER, MB_OK|MB_ICONEXCLAMATION);

	return 0;
}

LRESULT CDXFView::OnBindMoveMsg(WPARAM wParam, LPARAM lParam)
{
	m_nSelView = (int)(wParam);

	// �د��߲�ĂƎq����޳�̍������v�Z
	LPCADBINDINFO pInfo = GetDocument()->GetBindInfoData(m_nSelView);
	pInfo->pView->GetWindowRect(&m_rcMove);
	ScreenToClient(&m_rcMove);
	// �e�ޭ��̸ײ��č��W�ɕϊ�
	CPoint	ptClick(reinterpret_cast<CPoint*>(lParam)->x,
					reinterpret_cast<CPoint*>(lParam)->y);
	ScreenToClient(&ptClick);
	m_ptMove.x = ptClick.x - m_rcMove.left;
	m_ptMove.y = ptClick.y - m_rcMove.top;

	return 0;
}

LRESULT CDXFView::OnBindRoundMsg(WPARAM wParam, LPARAM lParam)
{
	CClientDC	dc(this);
	double	w, h;

	LPCADBINDINFO pInfo = GetDocument()->GetBindInfoData(wParam);
	CRectD	rc(pInfo->pDoc->GetMaxRect());
	w = rc.Width()  / 2.0;
	h = rc.Height() / 2.0;
	CPointD	pto(pInfo->pt.x+w, pInfo->pt.y-h);	// �z�u�̒��S

	pInfo->pDoc->AllRoundObjPoint(_copysign(RAD(90.0), (int)lParam));

	// �V�z�u���W�̌v�Z�Ɣz�u
	pInfo->pt.x = pto.x - h;
	pInfo->pt.y = pto.y + w;
	BindMsgPost(&dc, pInfo, NULL);

	GetDocument()->SetModifiedFlag();
	m_nSelView = -1;

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
	strDbg = "CDXFView::OnLButtonDown("+strDbg+")\nStart";
	CMagaDbg	dbg((LPSTR)(LPCTSTR)strDbg);
#endif
	if ( IsBindMode() ) {
		CDXFView*	pView  = static_cast<CDXFView*>(GetParent());
		ASSERT(pView);
		int n = pView->GetDocument()->GetBindInfo_fromView(this);
		if ( n >= 0 ) {
			ClientToScreen(&point);
			pView->SendMessage(WM_USERBINDMOVE, n,
					reinterpret_cast<LPARAM>(&point));
		}
		return;
	}

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
	strDbg = "CDXFView::OnLButtonUp("+strDbg+")\nStart";
	CMagaDbg	dbg((LPSTR)(LPCTSTR)strDbg);
#endif
	if ( m_nSelView>=0 && m_bMove ) {
		// �q�ޭ��̈ړ�����
		LPCADBINDINFO pInfo = GetDocument()->GetBindInfoData(m_nSelView);
		// UNDO���̕ۑ�
		LPCADBINDINFO pUndo = new CADBINDINFO(pInfo);
		m_bindUndo.AddTail(pUndo);
		//
		CClientDC	dc(this);
		CPoint		pt(point - m_ptMove);
		if ( nFlags & MK_SHIFT ) {
			int ppx = abs(pt.x - m_rcMove.left),
				ppy = abs(pt.y - m_rcMove.top);
			if ( ppx < ppy )
				pt.x = m_rcMove.left;
			else
				pt.y = m_rcMove.top;
		}
		dc.DPtoLP(&pt);
		CPointD		ptD(pt);
		ptD /= m_dFactor;
		pInfo->pt = ptD;
		// ���H���_�̍Đݒ�
		optional<CPointD> ptOrg = GetDocument()->GetCutterOrigin();
		CPoint	ptDev = *ptOrg * m_dFactor;
		dc.LPtoDP(&ptDev);
		ClientToScreen(&ptDev);
		BindMsgPost(&dc, pInfo, &ptDev);	// SendMessage(WM_USERVIEWFITMSG)
		//
		Invalidate();
		m_bMove = FALSE;
		GetDocument()->SetModifiedFlag();
		// �e�ޭ��ź����ٰè݂����悤��
		GetParentFrame()->SetActiveView(this);
		return;
	}
	m_nSelView = -1;

	// Down����Ă��������������I���������s��
	// CViewBase::_OnLButtonUp() ����ɔ���
	BOOL	bSelect = m_nLState == 0 ? TRUE : FALSE;

	if ( CViewBase::_OnLButtonUp(point) == 1 ) {
		// �g�又��
		OnLensKey(ID_VIEW_LENSP);
		return;
	}

	// ���H�w��[���Ȃ�|�ł��Ȃ�]����
	if ( !bSelect || m_bMagRect || !GetDocument()->IsDocFlag(DXFDOC_SHAPE) )
		return;

	// --- ���H�w��
	CDXFdata*	pData = NULL;
	CClientDC	dc(this);
	// ���W�l�v�Z(point��CViewBase::OnLButtonUp()�Ř_�����W�ɕϊ��ς�)
	//   -> DPtoLP()�s�v
	CPointD	pt(point);
	pt /= m_dFactor;
	CRect	rc;
	GetClientRect(rc);
	dc.DPtoLP(&rc);
	rc.NormalizeRect();
	CRectD	rcView(rc);
	rcView /= m_dFactor;
	// �د��߲�Ă���W������
	if ( !m_pSelData ) {
		CDXFshape*	pShape = NULL;
		double		dGap;
		tie(pShape, pData, dGap) = GetDocument()->GetSelectObject(pt, rcView);
		if ( !pShape || !pData || dGap>=SELECTGAP/m_dFactor )
			return;
		// �ذ�̑I���� m_vSelect �̍X�V�ʒm
		GetDocument()->UpdateAllViews(this, UAV_DXFSHAPEUPDATE, pShape);
	}

	// ���H�w�����Ƃ̏���
	switch ( GetDocument()->GetShapeProcessID() ) {
	case ID_EDIT_SHAPE_SEL:		// ����
		OnLButtonUp_Separate(&dc, pData, pt, rcView);
		break;
	case ID_EDIT_SHAPE_VEC:		// ����
		OnLButtonUp_Vector(&dc, pData, pt, rcView);
		break;
	case ID_EDIT_SHAPE_START:	// �J�n�ʒu
		OnLButtonUp_Start(&dc, pData, pt, rcView);
		break;
	case ID_EDIT_SHAPE_OUT:		// �֊s
//	case ID_EDIT_SHAPE_POC:		// �߹��
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
	strDbg = "CDXFView::OnRButtonDown("+strDbg+")\nStart";
	CMagaDbg	dbg((LPSTR)(LPCTSTR)strDbg);
#endif
	if ( IsBindMode() ) {
		if ( nFlags == MK_RBUTTON ) {
			// �E���݉����������Ȃ�ʏ퓮��
			CView::OnRButtonDown(nFlags, point);
		}
		else {
			// �e�ޭ��ɓ]��
			CDXFView*	pView  = static_cast<CDXFView*>(GetParent());
			ASSERT(pView);
			ClientToScreen(&point);
			pView->ScreenToClient(&point);
			pView->SendMessage(WM_RBUTTONDOWN, (WPARAM)nFlags,
						(LPARAM)(point.x|(point.y<<16)));
		}
	}
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
	strDbg = "CDXFView::OnRButtonUp("+strDbg+")\nStart";
	CMagaDbg	dbg((LPSTR)(LPCTSTR)strDbg);
#endif
	if ( IsBindMode() ) {
		// ��÷���ƭ��̏����p��m_nSelView���
		CDXFView*	pView  = static_cast<CDXFView*>(GetParent());
		ASSERT(pView);
		pView->m_nSelView = pView->GetDocument()->GetBindInfo_fromView(this);
		CView::OnRButtonUp(nFlags, point);
	}
	else {
		switch ( CViewBase::_OnRButtonUp(point) ) {
		case 1:		// �g�又��
			OnLensKey(ID_VIEW_LENSP);
			break;
		case 2:		// �I���޼ު�Ă̷�ݾ� or ��÷���ƭ��\��
			if ( !CancelForSelect() )
				CView::OnRButtonUp(nFlags, point);
			break;
		}
		m_nSelView = -1;
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
	strDbg = "CDXFView::OnLButtonDblClk("+strDbg+")\nStart";
	CMagaDbg	dbg((LPSTR)(LPCTSTR)strDbg);
#endif
	if ( IsBindMode() ) {
		// �q�ޭ���]
		CDXFView*	pView  = static_cast<CDXFView*>(GetParent());
		ASSERT(pView);
		int n = pView->GetDocument()->GetBindInfo_fromView(this);
		if ( n >= 0 ) {
			LPARAM lParam = (nFlags&MK_SHIFT) ? -1 : 1;
			pView->SendMessage(WM_USERBINDROUND, n, lParam);
		}
		return;
	}

	if ( !m_pSelData || m_nSelect<0 || m_vSelect.which()!=DXFTREETYPE_SHAPE )
		return;

	// ����ٸد�����ł� OnLButtonUp() ����Ă��������邪�A
	// �uDown����Ă��������������I���������s���v�Ȃ̂�
	// ����ٸد�����Ă�߂܂��āA��������(�m��)���s��
	CClientDC	dc(this);
	dc.DPtoLP(&point);
	CPointD		pt(point);
	pt /= m_dFactor;
	CRect	rc;
	GetClientRect(rc);
	dc.DPtoLP(&rc);
	rc.NormalizeRect();
	CRectD	rcView(rc);
	rcView /= m_dFactor;

	// ���H�w�����Ƃ̏���
	switch ( GetDocument()->GetShapeProcessID() ) {
	case ID_EDIT_SHAPE_SEL:		// ����
		OnLButtonUp_Separate(&dc, NULL, pt, rcView);
		break;
	case ID_EDIT_SHAPE_VEC:		// ����
		OnLButtonUp_Vector(&dc, NULL, pt, rcView);
		break;
	case ID_EDIT_SHAPE_START:	// �J�n�ʒu
		OnLButtonUp_Start(&dc, NULL, pt, rcView);
		break;
	case ID_EDIT_SHAPE_OUT:		// �֊s
//	case ID_EDIT_SHAPE_POC:		// �߹��
		OnLButtonUp_Outline(&dc, NULL, pt, rcView);
		break;
	}
}

void CDXFView::OnRButtonDblClk(UINT nFlags, CPoint point)
{
#ifdef _DEBUG
	CString	strDbg;
	if ( IsBindParent() )
		strDbg = "P";
	else if ( IsBindMode() )
		strDbg = "C";
	strDbg = "CDXFView::OnRButtonDblClk("+strDbg+")\nStart";
	CMagaDbg	dbg((LPSTR)(LPCTSTR)strDbg);
#endif
/*
	// ��÷���ƭ��\���ł͉E����ٸد������Ȃ�
	if ( IsBindMode() ) {
		// �q�ޭ��E��](-90�x)
		CDXFView*	pView  = static_cast<CDXFView*>(GetParent());
		ASSERT(pView);
		int n = pView->GetDocument()->GetBindInfo_fromView(this);
		if ( n >= 0 )
			pView->SendMessage(WM_USERBINDROUND, n, -1);
	}
*/
}

void CDXFView::OnLButtonUp_Separate
	(CDC* pDC, CDXFdata* pDataSel, const CPointD& ptView, const CRectD& rcView)
{
	ASSERT( m_vSelect.which()==DXFTREETYPE_SHAPE );
	CDXFshape*	pShapeSel = get<CDXFshape*>(m_vSelect);
	if ( !pDataSel ) {
		pShapeSel->GetSelectObjectFromShape(ptView, &rcView, &pDataSel);	// �߂�l�s�v
		if ( !pDataSel )
			return;
	}

	if ( m_pSelData ) {
		// ���̏W��(�O�ՏW���̂�)���ݸ�ł��邩�ǂ���
		CLayerData*	pLayer = m_pSelData->GetParentLayer();
		CDXFshape*	pShape;
		CDXFshape*	pShapeLink = NULL;
		CDXFmap*	pMap;
		for ( int i=0; i<pLayer->GetShapeSize(); i++ ) {
			pShape = pLayer->GetShapeData(i);
			if ( pShapeSel!=pShape && (pMap=pShape->GetShapeMap()) ) {
				// ���W����
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
			// �W����V�K�쐬
			pMap = NULL;
			try {
				pMap = new CDXFmap;
				pMap->SetPointMap(pDataSel);
				pShape = new CDXFshape(DXFSHAPE_LOCUS, pShapeSel->GetShapeName()+"(����)",
									pMap->GetMapTypeFlag(), pLayer, pMap);
				// ڲԏ��ɒǉ�
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
			// �����w���̊m�F
			CDXFworking*	pWork;
			CDXFdata*		pData;
			tie(pWork, pData) = pShapeSel->GetDirectionObject();
			if ( pData == pDataSel )
				pShapeSel->DelWorkingData(pWork, pShape);	// �t���ւ�
			// �J�n�ʒu�̊m�F
			tie(pWork, pData) = pShapeSel->GetStartObject();
			if ( pData == pDataSel )
				pShapeSel->DelWorkingData(pWork, pShape);
			// �ذ�ޭ��ւ̒ʒm
			addShape.pLayer = pLayer;
			addShape.pShape = pShape;
			GetDocument()->UpdateAllViews(this, UAV_DXFADDSHAPE, reinterpret_cast<CObject *>(&addShape));
		}
		// �I���׸ނ̸ر
		pShapeSel->SetShapeSwitch(FALSE);
		// ���W�������޼ު�Ă�����
		pShapeSel->RemoveObject(pDataSel);
		// �W������
		if ( pShapeSel->LinkObject() ) {
			// �ذ�ޭ��ւ̒ʒm
			addShape.pLayer = pLayer;
			addShape.pShape = pShapeSel;
			GetDocument()->UpdateAllViews(this, UAV_DXFADDSHAPE, reinterpret_cast<CObject *>(&addShape));
		}
		// �ݸ�o�����Ƃ����ݸ��W��������
		if ( pShapeLink ) {
			pShape = pShapeLink;
			if ( pShapeLink->LinkObject() ) {
				// �ذ�ޭ��ւ̒ʒm
				addShape.pLayer = pLayer;
				addShape.pShape = pShapeLink;
				GetDocument()->UpdateAllViews(this, UAV_DXFADDSHAPE, reinterpret_cast<CObject *>(&addShape));
			}
		}
		// �����W����I��
		ASSERT ( pShape );
		pShape->SetShapeSwitch(TRUE);
		m_vSelect = pShape;
		GetDocument()->UpdateAllViews(this, UAV_DXFSHAPEUPDATE, pShape);
		// �w���޼ު��ؾ��
		m_pSelData = NULL;
		m_nSelect  = -1;
		// �޷���ĕύX�ʒm
		GetDocument()->SetModifiedFlag();
		// �ĕ`��
		Invalidate();
	}
	else {
		// ��޼ު�Ă��P���������Ȃ��W���͕����ł��Ȃ�
		if ( pShapeSel->GetObjectCount() > 1 ) {
			m_pSelData = pDataSel;
			m_nSelect = 0;	// dummy
			// �W���S�̂��I����Ԃ̃n�Y�Ȃ̂ŁC�I���޼ު�Ă̂݉���
			m_pSelData->SetDxfFlg(DXFFLG_SELECT, FALSE);
			DrawTemporaryProcess(pDC);
		}
	}
}

void CDXFView::OnLButtonUp_Vector
	(CDC* pDC, CDXFdata* pDataSel, const CPointD& ptView, const CRectD& rcView)
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
		������ SwapNativePt() ���Ăяo���č��W�����ւ���ƁA
		�֊s��޼ު�ĂƂ̃����N�����Ȃ��Ȃ�B
--- */
		// ���H�w������
		CDXFworking* pWork = CreateWorkingData();
		// ���H�w���`��
		if ( pWork ) {
			pWork->DrawTuning(m_dFactor);
			CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_OUTLINE));
			pDC->SetROP2(R2_COPYPEN);
			pWork->Draw(pDC);
			pDC->SelectObject(pOldPen);
		}
		// �w���޼ު��ؾ��
		m_pSelData = NULL;
		m_nSelect  = -1;
	}
	else {
		if ( !(pShapeSel->GetShapeFlag()&DXFMAPFLG_DIRECTION) ) {
			m_pSelData = pDataSel;
			// �����W�̎擾
			m_pSelData->GetDirectionArraw(ptView, m_ptArraw);
			// ���݈ʒu�ɋ߂����̖���`��
			m_nSelect = GAPCALC(m_ptArraw[0][1] - ptView) < GAPCALC(m_ptArraw[1][1] - ptView) ? 0 : 1;
			DrawTempArraw(pDC);
		}
	}
}

void CDXFView::OnLButtonUp_Start
	(CDC* pDC, CDXFdata* pDataSel, const CPointD& ptView, const CRectD& rcView)
{
	ASSERT( m_vSelect.which()==DXFTREETYPE_SHAPE );
	CDXFshape*	pShapeSel = get<CDXFshape*>(m_vSelect);
	if ( !pDataSel ) {
		pShapeSel->GetSelectObjectFromShape(ptView, &rcView, &pDataSel);
		if ( !pDataSel )
			return;
	}

	if ( m_pSelData ) {
		// ���H�w������
		CDXFworking* pWork = CreateWorkingData();
		// ���H�w���`��
		if ( pWork ) {
			pWork->DrawTuning(m_dFactor);
			CPen*	pOldPen	  = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_OUTLINE));
			CBrush*	pOldBrush = pDC->SelectObject(AfxGetNCVCMainWnd()->GetBrushDXF(DXFBRUSH_START));
			pDC->SetROP2(R2_COPYPEN);
			pWork->Draw(pDC);
			pDC->SelectObject(pOldBrush);
			pDC->SelectObject(pOldPen);
		}
		// �w���޼ު��ؾ��
		m_pSelData = NULL;
		m_nSelect  = -1;
	}
	else {
		if ( !(pShapeSel->GetShapeFlag()&DXFMAPFLG_START) ) {
			m_pSelData = pDataSel;
			// ��޼ު�Ă̍��W�̎擾
			ASSERT( m_pSelData->GetPointNumber() <= SIZEOF(m_ptStart) );
			double	dGap, dGapMin = DBL_MAX;
			for ( int i=0; i<m_pSelData->GetPointNumber(); i++ ) {
				m_ptStart[i] = m_pSelData->GetNativePoint(i);
				dGap = GAPCALC(m_ptStart[i] - ptView);
				if ( dGap < dGapMin ) {
					dGapMin = dGap;
					m_nSelect = i;
				}
			}
			// ���݈ʒu�ɋ߂�����`��
			DrawTempStart(pDC);
		}
	}
}

void CDXFView::OnLButtonUp_Outline
	(CDC* pDC, CDXFdata* pDataSel, const CPointD& ptView, const CRectD& rcView)
{
	double		dGap1, dGap2;

	if ( m_pSelData ) {
		CDXFdata*	pData1;
		CDXFdata*	pData2;
		dGap1 = m_ltOutline[0].GetSelectObjectFromShape(ptView, &rcView, &pData1);
		dGap2 = m_ltOutline[1].GetSelectObjectFromShape(ptView, &rcView, &pData2);
		int		nSelect = dGap1 > dGap2 ? 1 : 0;
		// ���H�w������
		m_nSelect = nSelect;
		CDXFworking* pWork = CreateWorkingData();
		// ���H�w���`��
		if ( pWork ) {
			pWork->DrawTuning(m_dFactor);
			CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_OUTLINE));
			CBrush* pOldBrush = (CBrush *)pDC->SelectStockObject(NULL_BRUSH);
			pDC->SetROP2(R2_COPYPEN);
			pWork->Draw(pDC);
			pDC->SelectObject(pOldBrush);
			pDC->SelectObject(pOldPen);
		}
		// �w���޼ު��ؾ��
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
		// �����֊s���ݒ�ł���悤�ɕύX
//		if ( pShapeSel->GetShapeAssemble() != DXFSHAPE_OUTLINE ||
//				pShapeSel->GetShapeFlag() & (DXFMAPFLG_OUTLINE|DXFMAPFLG_POCKET) )
		if ( pShapeSel->GetShapeAssemble() != DXFSHAPE_OUTLINE )
			return;
		m_pSelData = pDataSel;
		// �֊s��޼ު�Đ���
		if ( !CreateOutlineTempObject(pShapeSel) ) {
			AfxMessageBox(IDS_ERR_DXF_CREATEOUTELINE, MB_OK|MB_ICONEXCLAMATION);
			return;
		}
		// ϳ����W������O��޼ު�Ă̑I��
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
	if ( m_nSelView>=0 && nFlags&MK_LBUTTON ) {
		CPoint	pt(point - m_ptMove);
		if ( nFlags & MK_SHIFT ) {
			int ppx = abs(pt.x - m_rcMove.left),
				ppy = abs(pt.y - m_rcMove.top);
			if ( ppx < ppy )
				pt.x = m_rcMove.left;
			else
				pt.y = m_rcMove.top;
		}
		m_bMove = TRUE;
		LPCADBINDINFO pInfo = GetDocument()->GetBindInfoData(m_nSelView);
//		pInfo->pView->MoveWindow(pt.x, pt.y, m_rcMove.Width(), m_rcMove.Height());
		pInfo->pView->SetWindowPos(NULL, pt.x, pt.y, m_rcMove.Width(), m_rcMove.Height(),
			SWP_NOZORDER|SWP_NOACTIVATE);
		return;
	}

	if ( CViewBase::_OnMouseMove(nFlags, point) ||
			!m_pSelData || m_vSelect.which()!=DXFTREETYPE_SHAPE ) {
		if ( IsBindParent() )
			BindMove(FALSE);
		return;
	}

	// CViewBase::OnMouseMove() �ŏ������Ȃ����
	// �����Ř_�����W�ɕϊ�����K�v������
	CClientDC	dc(this);
	dc.DPtoLP(&point);
	CPointD		pt(point);
	pt /= m_dFactor;
	CRect	rc;
	GetClientRect(rc);
	dc.DPtoLP(&rc);
	rc.NormalizeRect();
	CRectD	rcView(rc);
	rcView /= m_dFactor;

	// ���H�w�����Ƃ̏���
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
	(CDC* pDC, const CPointD& ptView, const CRectD& rcView)
{
	ASSERT( m_vSelect.which()==DXFTREETYPE_SHAPE );
	CDXFshape*	pShape = get<CDXFshape*>(m_vSelect);
	CDXFdata*	pDataSel = NULL;
	pShape->GetSelectObjectFromShape(ptView, &rcView, &pDataSel);
	if ( !pDataSel )
		return;

	if ( m_pSelData != pDataSel ) {
		// �O��̑I����Ԃ����ɖ߂�
		m_pSelData->SetDxfFlg(DXFFLG_SELECT);	// �����̏ꍇ�͑I����Ԃ��t
		DrawTemporaryProcess(pDC);
		// �V�����I����Ԃ�`��
		m_pSelData = pDataSel;
		m_pSelData->SetDxfFlg(DXFFLG_SELECT, FALSE);
		DrawTemporaryProcess(pDC);
	}
}

void CDXFView::OnMouseMove_Vector
	(CDC* pDC, const CPointD& ptView, const CRectD& rcView)
{
	double	dGap1 = GAPCALC(m_ptArraw[0][1] - ptView),
			dGap2 = GAPCALC(m_ptArraw[1][1] - ptView);
	int		nSelect = dGap1 > dGap2 ? 1 : 0;

	if ( m_nSelect != nSelect ) {
		// �O��̖�������
		DrawTempArraw(pDC);
		// �V�������̕`��
		m_nSelect = nSelect;
		DrawTempArraw(pDC);
	}
}

void CDXFView::OnMouseMove_Start
	(CDC* pDC, const CPointD& ptView, const CRectD& rcView)
{
	int		i, nSelect;
	double	dGap, dGapMin = DBL_MAX;

	for ( i=0; i<m_pSelData->GetPointNumber(); i++ ) {
		dGap = GAPCALC(m_ptStart[i] - ptView);
		if ( dGap < dGapMin ) {
			dGapMin = dGap;
			nSelect = i;
		}
	}

	if ( m_nSelect != nSelect ) {
		// �O��̊J�n�ʒu������
		DrawTempStart(pDC);
		// �V�����J�n�ʒu�̕`��
		m_nSelect = nSelect;
		DrawTempStart(pDC);
	}
}

void CDXFView::OnMouseMove_Outline
	(CDC* pDC, const CPointD& ptView, const CRectD& rcView)
{
	double	dGap1 = m_ltOutline[0].GetSelectObjectFromShape(ptView, &rcView),
			dGap2 = m_ltOutline[1].GetSelectObjectFromShape(ptView, &rcView);
	int		nSelect = dGap1 > dGap2 ? 1 : 0;

	if ( m_nSelect != nSelect ) {
		// �O��̗֊s������
		DrawTempOutline(pDC);
		// �V�������̕`��
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
		{
			CDXFView* pView = IsBindMode() ? static_cast<CDXFView*>(GetParent()) : this;
			pView->CancelForSelect();
		}
		break;
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
		if ( pView->m_nSelView >= 0 ) {
			// �q�ޭ��ړ���
			return HTTRANSPARENT;
		}
	}
	return __super::OnNcHitTest(point);
}
