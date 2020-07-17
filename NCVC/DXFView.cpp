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

/////////////////////////////////////////////////////////////////////////////
// CDXFView

IMPLEMENT_DYNCREATE(CDXFView, CView)

BEGIN_MESSAGE_MAP(CDXFView, CView)
	//{{AFX_MSG_MAP(CDXFView)
	ON_WM_CREATE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
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
	// հ�޲Ƽ�ُ��� & �e�ޭ��ւ�̨��ү����
	ON_MESSAGE (WM_USERVIEWFITMSG, OnUserViewFitMsg)
	// ϳ��ړ��̽ð���ް�X�V
	ON_UPDATE_COMMAND_UI(ID_DXFST_MOUSE, OnUpdateMouseCursor)
	// �ړ�
	ON_COMMAND_RANGE(ID_VIEW_UP, ID_VIEW_RT, OnMoveKey)
	ON_COMMAND_RANGE(ID_VIEW_BEFORE, ID_VIEW_LENSN, OnLensKey)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDXFView �N���X�̍\�z/����

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
// CDXFView �N���X�̃I�[�o���C�h�֐�

BOOL CDXFView::PreCreateWindow(CREATESTRUCT& cs) 
{
	// ���د�����̋��E���ق��ڂ�̂ŁCIDC_ARROW �𖾎��I�Ɏw��
	cs.lpszClass = AfxRegisterWndClass(
			CS_HREDRAW|CS_VREDRAW|CS_OWNDC|CS_DBLCLKS,
			AfxGetApp()->LoadStandardCursor(IDC_ARROW) );
	return CView::PreCreateWindow(cs);
}

void CDXFView::OnInitialUpdate() 
{
	CView::OnInitialUpdate();

	// MDI�q�ڰт̽ð���ް�ɏ��\��
	static_cast<CDXFChild *>(GetParentFrame())->SetDataInfo(
		GetDocument()->GetDxfDataCnt(DXFLINEDATA),
		GetDocument()->GetDxfDataCnt(DXFCIRCLEDATA),
		GetDocument()->GetDxfDataCnt(DXFARCDATA),
		GetDocument()->GetDxfDataCnt(DXFELLIPSEDATA),
		GetDocument()->GetDxfDataCnt(DXFPOINTDATA) );

	// �رى���̐}�`̨��ү���ނ̑��M
	// OnInitialUpdate()�֐����ł́CGetClientRect()�̻��ނ��������Ȃ�
	if ( GetDocument()->IsDXFDocFlag(DXFDOC_SHAPE) )
		GetParentFrame()->PostMessage(WM_USERINITIALUPDATE);
	else
		PostMessage(WM_USERVIEWFITMSG);
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
					AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_WORKER) :
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
	for ( int i=0; i<SIZEOF(m_ltOutline); i++ ) {
		for ( pos=m_ltOutline[i].GetHeadPosition(); pos; ) {
			pData = m_ltOutline[i].GetNext(pos);
			if ( pData )
				pData->DrawTuning(m_dFactor*LOMETRICFACTOR);
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

	// ���_�`��
	CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_ORIGIN));
	if ( pData=GetDocument()->GetCircleObject() )
		pData->Draw(pDC);	// CDXFcircleEx::Draw()
	// ���Ռ��_
	for ( i=0; i<2; i++ ) {
		if ( pData=GetDocument()->GetLatheLine(i) )
			pData->Draw(pDC);	// CDXFline::Draw()
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

	// CDXFpoint�ɏ���
	CRect	rcDraw;
	CPointD	pt( m_ptStart[m_nSelect] * m_dFactor * LOMETRICFACTOR);
	// �ʒu��\���ۈ�͏��2.5�_������
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
// CDXFView �f�f

#ifdef _DEBUG
void CDXFView::AssertValid() const
{
	CView::AssertValid();
}

void CDXFView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CDXFDoc* CDXFView::GetDocument() // ��f�o�b�O �o�[�W�����̓C�����C���ł��B
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CDXFDoc)));
	return static_cast<CDXFDoc *>(m_pDocument);
}
#endif //_DEBUG

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
		// ���_����̋���
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
		rc  = GetDocument()->GetMaxRect();
		pData = GetDocument()->GetCircleObject();
		if ( pData )
			rc |= pData->GetMaxRect();
		for ( int i=0; i<2; i++ ) {
			pData = GetDocument()->GetLatheLine(0);
			if ( pData )
				rc |= pData->GetMaxRect();
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
	// MDI�q�ڰт̽ð���ް�ɏ��\��
	static_cast<CDXFChild *>(GetParentFrame())->SetFactorInfo(m_dFactor);
	// �ޭ��̍ĕ`��
	Invalidate();
}

/////////////////////////////////////////////////////////////////////////////
// CDXFView ���b�Z�[�W �n���h��

int CDXFView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if ( CView::OnCreate(lpCreateStruct) < 0 )
		return -1;

	// ϯ��ݸ�Ӱ�ނ̕ύX�Ȃ�
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
#ifdef _DEBUG
	CMagaDbg	dbg("CDXFView::OnLButtonDown\nStart");
#endif
	CViewBase::OnLButtonDown(point);
}

void CDXFView::OnLButtonUp(UINT nFlags, CPoint point) 
{
#ifdef _DEBUG
	CMagaDbg	dbg("CDXFView::OnLButtonUp\nStart");
#endif
	// Down����Ă��������������I���������s��
	BOOL	bSelect = m_nLState == 0 ? TRUE : FALSE;

	if ( CViewBase::OnLButtonUp(point) == 1 ) {
		// �g�又��
		OnLensKey(ID_VIEW_LENSP);
		return;
	}
	// ���H�w��[���Ȃ�|�ł��Ȃ�]����
	if ( !bSelect || m_bMagRect || !GetDocument()->IsDXFDocFlag(DXFDOC_SHAPE) )
		return;

	// --- ���H�w��
	CDXFdata*	pData = NULL;
	CClientDC	dc(this);
	// ���W�l�v�Z(point��CViewBase::OnLButtonUp()�Ř_�����W�ɕϊ��ς�)
	CPointD	pt(point);
	pt /= ( m_dFactor * LOMETRICFACTOR );
	CRect	rc;
	GetClientRect(rc);
	dc.DPtoLP(rc);
	rc.NormalizeRect();
	CRectD	rcView(rc);
	rcView /= ( m_dFactor * LOMETRICFACTOR );
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
	if ( !m_pSelData || m_nSelect<0 || m_vSelect.which()!=DXFTREETYPE_SHAPE )
		return;

	// ����ٸد�����ł� OnLButtonUp() ����Ă��������邪�A
	// �uDown����Ă��������������I���������s���v�Ȃ̂�
	// ����ٸد�����Ă�߂܂��āA��������(�m��)���s��
	CClientDC	dc(this);
	dc.DPtoLP(&point);
	CPointD		pt(point);
	pt /= ( m_dFactor * LOMETRICFACTOR );
	CRect	rc;
	GetClientRect(rc);
	dc.DPtoLP(rc);
	rc.NormalizeRect();
	CRectD	rcView(rc);
	rcView /= ( m_dFactor * LOMETRICFACTOR );

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
									pMap->GetMapTypeFlag(), pMap);
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
			CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_WORKER));
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
			CPen*	pOldPen	  = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_WORKER));
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
			CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_WORKER));
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
	CViewBase::OnRButtonDown(point);
}

void CDXFView::OnRButtonUp(UINT nFlags, CPoint point) 
{
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
	if ( CViewBase::OnMouseMove(nFlags, point) ||
				!m_pSelData || m_vSelect.which()!=DXFTREETYPE_SHAPE )
		return;

	// CViewBase::OnMouseMove() �ŏ������Ȃ����
	// �����Ř_�����W�ɕϊ�����K�v������
	CClientDC	dc(this);
	dc.DPtoLP(&point);
	CPointD		pt(point);
	pt /= ( m_dFactor * LOMETRICFACTOR );
	CRect	rc;
	GetClientRect(rc);
	dc.DPtoLP(rc);
	rc.NormalizeRect();
	CRectD	rcView(rc);
	rcView /= ( m_dFactor * LOMETRICFACTOR );

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
		if ( GetDocument()->IsDXFDocFlag(DXFDOC_SHAPE) )
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
	CRect	rc;
	GetClientRect(rc);

	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	COLORREF	col1 = pOpt->GetDxfDrawColor(DXFCOL_BACKGROUND2),
				col2 = pOpt->GetDxfDrawColor(DXFCOL_BACKGROUND1);

	return AfxGetNCVCMainWnd()->DrawBackGroundView(pDC, &rc, col1, col2);
}
