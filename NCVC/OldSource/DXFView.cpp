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

#include <stdlib.h>

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

BEGIN_MESSAGE_MAP(CDXFView, CView)
	//{{AFX_MSG_MAP(CDXFView)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_KEYDOWN()
	ON_WM_CONTEXTMENU()
	ON_WM_ERASEBKGND()
	ON_COMMAND(ID_DXFVIEW_LAYER, &CDXFView::OnViewLayer)
	ON_UPDATE_COMMAND_UI(ID_DXFVIEW_LAYER, &CDXFView::OnUpdateViewLayer)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, &CDXFView::OnUpdateEditCopy)
	ON_COMMAND(ID_EDIT_COPY, &CDXFView::OnEditCopy)
	//}}AFX_MSG_MAP
	// հ�޲Ƽ�ُ��� & �e�ޭ��ւ�̨��ү����
	ON_MESSAGE(WM_USERVIEWFITMSG, &CDXFView::OnUserViewFitMsg)
	// ϳ��ړ��̽ð���ް�X�V
	ON_UPDATE_COMMAND_UI(ID_DXFST_MOUSE, &CDXFView::OnUpdateMouseCursor)
	// CAD�ް��̓���
	ON_MESSAGE(WM_USERBINDINIT, &CDXFView::OnBindInitMsg)
	// �ړ�
	ON_COMMAND_RANGE(ID_VIEW_UP, ID_VIEW_RT, &CDXFView::OnMoveKey)
	ON_COMMAND_RANGE(ID_VIEW_BEFORE, ID_VIEW_LENSN, &CDXFView::OnLensKey)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDXFView �N���X�̍\�z/����

CDXFView::CDXFView()
{
	m_nSelect = -1;
	m_pSelData = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CDXFView �N���X�̃I�[�o���C�h�֐�

void CDXFView::OnInitialUpdate() 
{
	CView::OnInitialUpdate();

	if ( !IsBindMode() ) {
		// MDI�q�ڰт̽ð���ް�ɏ��\��
		static_cast<CDXFChild *>(GetParentFrame())->SetDataInfo(
			GetDocument()->GetDxfDataCnt(DXFLINEDATA),
			GetDocument()->GetDxfDataCnt(DXFCIRCLEDATA),
			GetDocument()->GetDxfDataCnt(DXFARCDATA),
			GetDocument()->GetDxfDataCnt(DXFELLIPSEDATA),
			GetDocument()->GetDxfDataCnt(DXFPOINTDATA) );

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
		reinterpret_cast<CDXFcircleEx*>(pHint)->DrawTuning(m_dFactor*LOMETRICFACTOR);
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
/*
void CDXFView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView)
{
	// TODO: �����ɓ���ȃR�[�h��ǉ����邩�A�������͊�{�N���X���Ăяo���Ă��������B
#ifdef _DEBUG
	CMagaDbg	dbg("CDXFView::OnActivateView()");
	dbg.printf("bActivate=%d IsBindParent()=%d", bActivate, IsBindParent()?1:0);
#endif
	if ( bActivate && IsBindMode() && !IsBindParent() ) {
		GetParent()->GetParent()->SetFocus();
		return;
	}
	CViewBase::OnActivateView(bActivate, pActivateView, pDeactiveView);
}
*/
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

BOOL CDXFView::SendParentMessage(UINT msg, UINT nFlags, CPoint& pt)
{
	if ( IsBindMode() ) {
		// CAD�ް�����Ӱ�ނ�ϳ�����Ă�eView�ɓ]������
		CWnd* pParent = GetParent()->GetParent();
		ClientToScreen(&pt);
		pParent->ScreenToClient(&pt);
		pParent->SendMessage(msg, (WPARAM)nFlags, (LPARAM)(pt.x|(pt.y<<16)));
		pParent->SetFocus();
		return TRUE;
	}
	return FALSE;
}

CDXFworking* CDXFView::CreateWorkingData(void)
{
	ASSERT( m_vSelect.which()==DXFTREETYPE_SHAPE );
	ASSERT( m_nSelect >= 0 );
	int		n = 1 - m_nSelect,	// 1->0, 0->1
			nInOut = -1;		// �֊s�w���̂�
	CDXFdata*		pData;
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
	POSITION	pos;
	CDXFdata*	pData;
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

	return nError < SIZEOF(m_ltOutline);	// �Ȃ�TRUE
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
	if ( m_pSelData && m_nSelect>=0 ) {
		if ( GetDocument()->GetShapeProcessID() == ID_EDIT_SHAPE_SEL )
			m_pSelData->SetDxfFlg(DXFFLG_SELECT);	// �I����Ԃ����ɖ߂�
		if ( !pDC ) {
			CClientDC	dc(this);
			DrawTemporaryProcess(&dc);
		}
		else
			DrawTemporaryProcess(pDC);
	}

	BOOL	bResult = m_pSelData ? TRUE : FALSE;
	m_nSelect = -1;
	m_pSelData = NULL;
	DeleteOutlineTempObject();

	return bResult;
}

void CDXFView::AllChangeFactor_OutlineTempObject(void)
{
	CDXFdata*	pData;
	POSITION	pos;
	double		dFactor = m_dFactor * LOMETRICFACTOR;
	for ( int i=0; i<SIZEOF(m_ltOutline); i++ ) {
		for ( pos=m_ltOutline[i].GetHeadPosition(); pos; ) {
			pData = m_ltOutline[i].GetNext(pos);
			if ( pData )
				pData->DrawTuning(dFactor);
		}
	}
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
	double	dFactor = m_dFactor * LOMETRICFACTOR;

	for ( int i=0; i<SIZEOF(m_ptArraw); i++ ) {
		ptDraw[i][1] = m_ptArraw[i][1] * dFactor;
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
	CPointD	pt( m_ptStart[m_nSelect] * m_dFactor * LOMETRICFACTOR);
	// �ʒu��\���ۈ�͏��2.5�_������
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
	CDXFdata*	pData;

	if ( !m_ltOutline[m_nSelect].IsEmpty() ) {
		CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_OUTLINE));
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
// CDXFView �N���X�̃��b�Z�[�W �n���h���i���j���[�ҁj

void CDXFView::OnUpdateEditCopy(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(TRUE);
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
			double	dFactor = m_dFactor * LOMETRICFACTOR;
			CPointD	ptd( pt.x/dFactor, pt.y/dFactor );
			// ���_����̋���
			ptd -= *ptOrg;
			static_cast<CDXFChild *>(GetParentFrame())->OnUpdateMouseCursor(&ptd);
		}
		else
			static_cast<CDXFChild *>(GetParentFrame())->OnUpdateMouseCursor();
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
	case ID_VIEW_BEFORE:
		CViewBase::OnBeforeMagnify();
		break;
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
		static_cast<CDXFChild *>(GetParentFrame())->SetFactorInfo(m_dFactor);
	}
	// �ޭ��̍ĕ`��
	Invalidate();
}

void CDXFView::BindMove(BOOL bFitMsg)
{
	CClientDC		dc(this);
	LPCADBINDINFO	pInfo;
	CPoint			pt;
	CRect			rc;
	CSize			sz;
	double	dFactor = m_dFactor * LOMETRICFACTOR;

	for ( int i=0; i<GetDocument()->GetBindInfoCnt(); i++ ) {
		pInfo = GetDocument()->GetBindInfoData(i);
		pt = pInfo->pt * dFactor;
		dc.LPtoDP(&pt);
		rc = pInfo->pDoc->GetMaxRect();
		dc.LPtoDP(&rc);
		rc.NormalizeRect();
		sz.cx = (int)(rc.Width()  * dFactor);
		sz.cy = (int)(rc.Height() * dFactor);
		pInfo->pParent->MoveWindow(pt.x, pt.y, sz.cx, sz.cy);
		if ( bFitMsg ) {
			pInfo->pParent->GetClientRect(&rc);
			pInfo->pView->MoveWindow(0, 0, rc.Width(), rc.Height());
			pInfo->pView->SendMessage(WM_USERVIEWFITMSG);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// CDXFView ���b�Z�[�W �n���h��

int CDXFView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	// ϯ��ݸ�Ӱ�ނ̕ύX�Ȃ�
	return CViewBase::OnCreate(lpCreateStruct);
}

void CDXFView::OnDestroy()
{
	DeleteOutlineTempObject();
	__super::OnDestroy();
}

LRESULT CDXFView::OnUserViewFitMsg(WPARAM, LPARAM)
{
	if ( IsBindMode() ) {
		CRectD	rc;
		rc = GetDocument()->GetMaxRect();
		CViewBase::OnViewFit(rc, FALSE);
		OnViewLensComm();
	}
	else {
		OnLensKey(ID_VIEW_FIT);
	}
	return 0;
}

void _OnBindInitMsgPost(CClientDC& dc, LPCADBINDINFO pInfo, const CPointD& ptL, double dFactor)
{
	CPoint	pt;
	CRect	rc;
	CSize	sz;

	// �ʒu�L�������޲����W�ւ̕ϊ�
	pInfo->pt = ptL;
	pt = ptL * dFactor;
	dc.LPtoDP(&pt);

	// �ް��̍ő��`�����޲����W�ɕϊ�
	rc = pInfo->pDoc->GetMaxRect();
	dc.LPtoDP(&rc);
	rc.NormalizeRect();		// Y���t�]

	// �g�嗦�̌v�Z
	sz.cx = (int)(rc.Width()  * dFactor);
	sz.cy = (int)(rc.Height() * dFactor);

	// �q�ڰт̔z�u
	pInfo->pParent->MoveWindow(pt.x, pt.y, sz.cx, sz.cy);
	pInfo->pParent->GetClientRect(&rc);
	pInfo->pView->MoveWindow(0,0, rc.Width(), rc.Height());
	pInfo->pView->SendMessage(WM_USERVIEWFITMSG);
}

LRESULT CDXFView::OnBindInitMsg(WPARAM, LPARAM)
{
#ifdef _DEBUG
	CMagaDbg	dbg("OnBindInitMsg");
#endif
	int		i, nLoop = GetDocument()->GetBindInfoCnt();
	BOOL	bResult, bXover = FALSE;
	double	dMargin = AfxGetNCVCApp()->GetDXFOption()->GetBindMargin();
	CClientDC		dc(this);
	LPCADBINDINFO	pInfo;
	CRectD	rcWork( GetDocument()->GetMaxRect() ), rc;
	CPointD	pt(0, rcWork.bottom);
	double	W = rcWork.Width(), w, h, dw, dh,
			dFactor = m_dFactor * LOMETRICFACTOR;
	CVPointD	X, Y;		// BL����_
	CVPointD::iterator	it;
#ifdef _DEBUG
	dbg.printf("rcWork = (%f, %f) - (%f, %f)",
		rcWork.left, rcWork.top, rcWork.right, rcWork.bottom);
#endif

	// --- �����`�l�ߍ��ݖ��iBLF�@�j---

	// 1�ߔz�u
	ASSERT( nLoop > 0 );
	pInfo = GetDocument()->GetBindInfoData(0);
	_OnBindInitMsgPost(dc, pInfo, pt, dFactor);
	// BL����_
	rc = pInfo->pDoc->GetMaxRect();
	pt.x = rc.Width() + dMargin;
	pt.y = rcWork.bottom;
	if ( W > pt.x ) {
		X.push_back(pt);
#ifdef _DEBUG
		dbg.printf("first X = (%f, %f)", pt.x, pt.y);
#endif
	}
	else if ( W < rc.Width() )
		bXover = TRUE;
	pt.x = 0;
	pt.y -= rc.Height() + dMargin;
	Y.push_back(pt);	// Y������_�͕K���o�^ww
#ifdef _DEBUG
	dbg.printf("first Y = (%f, %f)", pt.x, pt.y);
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
		bResult = FALSE;

		// X���D���₩�猟��(������)
		for ( it=X.begin(); it!=X.end(); ++it ) {
			pt = *it;
			dw = W - pt.x;
			if ( dw >= w ) {
				_OnBindInitMsgPost(dc, pInfo, pt, dFactor);
				X.erase(it);
				// X������_�̓o�^
				if ( dw - w > dMargin ) {
					X.push_back( CPointD(pt.x+w, pt.y) );
#ifdef _DEBUG
					dbg.printf("X new X = (%f, %f)", pt.x+w, pt.y);
#endif
				}
				// Y������_�̓o�^
				if ( pt.y-h - Y.front().y > dMargin ) {
					Y.insert(Y.begin(), CPointD(pt.x, pt.y-h));
#ifdef _DEBUG
					dbg.printf("X new Y = (%f, %f)", pt.x, pt.y-h);
#endif
				}
				bResult = TRUE;
				break;
			}
		}
		if ( bResult )
			continue;

		// Y���D���₩�猟��(�����Ԃ̍�������)
		for ( it=Y.begin(); it!=Y.end()-1; ++it ) {
			pt = *it;
			dw = W - pt.x;
			dh = pt.y - (*boost::next(it)).y;
			if ( dh >= h && dw >= w ) {
				_OnBindInitMsgPost(dc, pInfo, pt, dFactor);
				Y.erase(it);
				if ( dw - w > dMargin ) {
					X.push_back( CPointD(pt.x+w, pt.y) );
#ifdef _DEBUG
					dbg.printf("Y new X = (%f, %f)", pt.x+w, pt.y);
#endif
				}
				if ( dh - h > dMargin ) {
					Y.insert(Y.begin(), CPointD(pt.x, pt.y-h));
#ifdef _DEBUG
					dbg.printf("Y new Y = (%f, %f)", pt.x, pt.y-h);
#endif
				}
				bResult = TRUE;
				break;
			}
		}

		// ���ɂ������Ԃɂ�����Ȃ�
		if ( !bResult ) {
			pt = Y.back();
			_OnBindInitMsgPost(dc, pInfo, pt, dFactor);
			Y.pop_back();
			if ( W > w ) {
				pt.x = w;
				X.push_back(pt);
#ifdef _DEBUG
				dbg.printf("new X = (%f, %f)", pt.x, pt.y);
#endif
			}
			else if ( W < rc.Width() )
				bXover = TRUE;
			pt.x = 0;
			pt.y -= h;
			Y.push_back(pt);
#ifdef _DEBUG
			dbg.printf("new Y = (%f, %f)", pt.x, pt.y);
#endif
		}
	}

	if ( Y.back().y < 0 || bXover )
		AfxMessageBox(IDS_ERR_BINDAREAOVER, MB_OK|MB_ICONEXCLAMATION);

	return 0;
}

void CDXFView::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	CViewBase::OnContextMenu(point, IDR_DXFPOPUP1);
}

void CDXFView::OnLButtonDown(UINT nFlags, CPoint point) 
{
#ifdef _DEBUG
	CMagaDbg	dbg("CDXFView::OnLButtonDown()");
	dbg.printf("IsBindParent()=%d", IsBindParent()?1:0);
#endif
	if ( SendParentMessage(WM_LBUTTONDOWN, nFlags, point) )
		return;
	CViewBase::OnLButtonDown(point);
}

void CDXFView::OnLButtonUp(UINT nFlags, CPoint point) 
{
#ifdef _DEBUG
	CMagaDbg	dbg("CDXFView::OnLButtonUp\nStart");
#endif
	if ( SendParentMessage(WM_LBUTTONUP, nFlags, point) )
		return;

	// Down����Ă��������������I���������s��
	BOOL	bSelect = m_nLState == 0 ? TRUE : FALSE;

	if ( CViewBase::OnLButtonUp(point) == 1 ) {
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
	CPointD	pt(point);
	double	dFactor = m_dFactor * LOMETRICFACTOR;
	pt /= dFactor;
	CRect	rc;
	GetClientRect(rc);
	dc.DPtoLP(rc);
	rc.NormalizeRect();
	CRectD	rcView(rc);
	rcView /= dFactor;
	// �د��߲�Ă���W������
	if ( !m_pSelData ) {
		CDXFshape*	pShape = NULL;
		double		dGap;
		tie(pShape, pData, dGap) = GetDocument()->GetSelectObject(pt, rcView);
		if ( !pShape || !pData || dGap >= SELECTGAP/m_dFactor )
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

void CDXFView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CDXFView::OnLButtonDblClk\nStart");
#endif
	if ( SendParentMessage(WM_LBUTTONDBLCLK, nFlags, point) )
		return;
	if ( !m_pSelData || m_nSelect<0 || m_vSelect.which()!=DXFTREETYPE_SHAPE )
		return;

	// ����ٸد�����ł� OnLButtonUp() ����Ă��������邪�A
	// �uDown����Ă��������������I���������s���v�Ȃ̂�
	// ����ٸد�����Ă�߂܂��āA��������(�m��)���s��
	CClientDC	dc(this);
	dc.DPtoLP(&point);
	CPointD		pt(point);
	double		dFactor = m_dFactor * LOMETRICFACTOR;
	pt /= dFactor;
	CRect	rc;
	GetClientRect(rc);
	dc.DPtoLP(rc);
	rc.NormalizeRect();
	CRectD	rcView(rc);
	rcView /= dFactor;

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
/*
	---
		������ SwapNativePt() ���Ăяo���č��W�����ւ���ƁA
		�֊s��޼ު�ĂƂ̃����N�����Ȃ��Ȃ�B
	---
*/
		// ���H�w������
		CDXFworking* pWork = CreateWorkingData();
		// ���H�w���`��
		if ( pWork ) {
			pWork->DrawTuning(m_dFactor*LOMETRICFACTOR);
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
			pWork->DrawTuning(m_dFactor*LOMETRICFACTOR);
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
			pWork->DrawTuning(m_dFactor*LOMETRICFACTOR);
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

void CDXFView::OnRButtonDown(UINT nFlags, CPoint point) 
{
	if ( SendParentMessage(WM_RBUTTONDOWN,  nFlags, point) )
		return;
	CViewBase::OnRButtonDown(point);
}

void CDXFView::OnRButtonUp(UINT nFlags, CPoint point) 
{
	if ( SendParentMessage(WM_RBUTTONUP, nFlags, point) )
		return;
	switch ( CViewBase::OnRButtonUp(point) ) {
	case 1:		// �g�又��
		OnLensKey(ID_VIEW_LENSP);
		break;
	case 2:		// �I���޼ު�Ă̷�ݾ� or ��÷���ƭ��\��
		if ( !CancelForSelect() )
			CView::OnRButtonUp(nFlags, point);
		break;
	}
}

void CDXFView::OnMouseMove(UINT nFlags, CPoint point) 
{
	if ( SendParentMessage(WM_MOUSEMOVE, nFlags, point) )
		return;
	if ( CViewBase::OnMouseMove(nFlags, point) ) {
		if ( m_nRState>0 && IsBindParent() )
			BindMove(FALSE);
		if ( !m_pSelData || m_vSelect.which()!=DXFTREETYPE_SHAPE )
			return;
	}

	// CViewBase::OnMouseMove() �ŏ������Ȃ����
	// �����Ř_�����W�ɕϊ�����K�v������
	CClientDC	dc(this);
	dc.DPtoLP(&point);
	CPointD		pt(point);
	double		dFactor = m_dFactor * LOMETRICFACTOR;
	pt /= dFactor;
	CRect	rc;
	GetClientRect(rc);
	dc.DPtoLP(rc);
	rc.NormalizeRect();
	CRectD	rcView(rc);
	rcView /= dFactor;

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
	if ( CViewBase::OnMouseWheel(nFlags, zDelta, pt) )
		OnViewLensComm();

	return TRUE;
}

void CDXFView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	switch ( nChar ) {
	case VK_TAB:
		if ( GetDocument()->IsDocFlag(DXFDOC_SHAPE) )
			static_cast<CDXFChild *>(GetParentFrame())->GetTreeView()->SetFocus();
		break;
	case VK_ESCAPE:
		CancelForSelect();
		break;
	}

	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

BOOL CDXFView::OnEraseBkgnd(CDC* pDC) 
{
	BOOL	bResult = FALSE;
	if ( !IsBindMode() ) {
		const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
		COLORREF	col1 = pOpt->GetDxfDrawColor(DXFCOL_BACKGROUND1),
					col2 = pOpt->GetDxfDrawColor(DXFCOL_BACKGROUND2);
		bResult = CViewBase::OnEraseBkgnd(pDC, col1, col2);
	}
	return bResult;
}
