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
	int		n = 1 - m_nSelect,	// 1->0, 0->1
			nInOut = -1;		// �֊s�w���̂�
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
			pShape->AddWorkingData(pWork, nInOut);
			// �`���ذ(���H�w��)�̍X�V�ƑI��
			GetDocument()->UpdateAllViews(this, UAV_DXFADDWORKING, static_cast<CObject *>(pWork));
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
	if ( m_pSelData && m_nSelect >= 0 ) {
		if ( GetDocument()->GetShapePattern() == ID_EDIT_SHAPE_SEL )
			m_pSelData->SetSelectFlg(TRUE);	// �I����Ԃ����ɖ߂�
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

	// DXF�`��
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
		// ���H�w���̕`��
		pLayer->DrawWorking(pDC);
	}

	// ���݂̊g���`��`��(ViewBase.cpp)
	if ( m_bMagRect )
		DrawMagnifyRect(pDC);
	// �����H�w���̕`��
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
	CViewBase::OnLButtonDown(point);
}

void CDXFView::OnLButtonUp(UINT nFlags, CPoint point) 
{
	// Down����Ă��������������I���������s��
	BOOL	bSelect = m_nLState == 0 ? TRUE : FALSE;

	if ( CViewBase::OnLButtonUp(point) == 1 ) {
		// �g�又��
		OnLensKey(ID_VIEW_LENSP);
		return;
	}
	// ���H�w��[���Ȃ�|�ł��Ȃ�]����
	if ( !bSelect || m_bMagRect || !GetDocument()->IsShape() )
		return;

	// ���H�w��
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

	// ϳ��د���ԑJ��
	if ( m_vSelect.which() == DXFTREETYPE_SHAPE ) {
		BOOL	bResult = FALSE;
		switch ( GetDocument()->GetShapePattern() ) {
		case ID_EDIT_SHAPE_SEL:		// ����
			bResult = OnLButtonUp_Select(&dc, pt, rcView);
			break;
		case ID_EDIT_SHAPE_VEC:		// ����
			bResult = OnLButtonUp_Vector(&dc, pt, rcView);
			break;
		case ID_EDIT_SHAPE_START:	// �J�n�ʒu
			bResult = OnLButtonUp_Start(&dc, pt, rcView);
			break;
		case ID_EDIT_SHAPE_OUT:		// �֊s
//		case ID_EDIT_SHAPE_POC:		// �߹��
			bResult = OnLButtonUp_Outline(&dc, pt, rcView);
			break;
		}
		if ( bResult )	// �����ς�
			return;
	}

	// �w����W�Ƃ̋�����臒l�ȏォ�W����I����ԂȂ�C�S�̌���
	double		dGap;
	CDXFshape*	pShape = NULL;
	tie(pShape, dGap) = GetDocument()->GetSelectObject(pt, rcView);
	if ( pShape && dGap < SELECTGAP/m_dFactor ) {
		// �ذ�̑I���� m_vSelect �̍X�V�ʒm
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
				return TRUE;	// �����ψ���
			}
			// �����w���̊m�F
			CDXFworking*	pWork;
			CDXFdata*	pData;
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
		// �޷���ĕύX�ʒm
		GetDocument()->SetModifiedFlag();
		// �ĕ`��
		Invalidate();
	}
	else {
		// ��޼ު�Ă��P���������Ȃ��W���͕����ł��Ȃ�
		if ( pShapeSel->GetObjectCount() <= 1 )
			::MessageBeep(MB_ICONEXCLAMATION);
		else {
			m_pSelData = pDataSel;
			m_nSelect = 0;	// dummy
			// �W���S�̂��I����Ԃ̃n�Y�Ȃ̂ŁC�I���޼ު�Ă̂݉���
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
		������ SetDirectionFixed() ���Ăяo���č��W�����ւ���ƁA
		�֊s��޼ު�ĂƂ̃����N�����Ȃ��Ȃ�B
		// ��޼ު�Ăɑ΂�������w��
		// �n�_�͌��݈ʒu�ɉ�����(1-m_nSelect)�C�I�_�͋߂���(m_nSelect)
		m_nSelect = GAPCALC(m_ptArraw[0][1] - ptView) < GAPCALC(m_ptArraw[1][1] - ptView) ? 0 : 1;
		m_pSelData->SetDirectionFixed(m_ptArraw[1-m_nSelect][1], m_ptArraw[m_nSelect][1]);
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
	}
	else {
		if ( pShapeSel->GetShapeFlag() & DXFMAPFLG_DIRECTION )
			::MessageBeep(MB_ICONEXCLAMATION);
		else {
			m_pSelData = pDataSel;
			// �����W�̎擾
			m_pSelData->GetDirectionArraw(ptView, m_ptArraw);
			// ���݈ʒu�ɋ߂����̖���`��
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
	}
	else {
		if ( pShapeSel->GetShapeFlag() & DXFMAPFLG_START )
			::MessageBeep(MB_ICONEXCLAMATION);
		else {
			m_pSelData = pDataSel;
			// ��޼ު�Ă̍��W�̎擾
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
			// ���݈ʒu�ɋ߂�����`��
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
			return TRUE;	// �����ψ���
		}
		m_pSelData = pDataSel;
		// �֊s��޼ު�Đ���
		if ( !CreateOutlineTempObject(pShapeSel) ) {
			AfxMessageBox(IDS_ERR_DXF_CREATEOUTELINE, MB_OK|MB_ICONEXCLAMATION);
			return TRUE;
		}
		// ϳ����W������O��޼ު�Ă̑I��
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

	// �I���W���Ƃ̋����v�Z
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

	// 臒l�𒴂����
	if ( !bResult && m_nSelect >= 0 ) {
		if ( nShape == ID_EDIT_SHAPE_SEL )
			m_pSelData->SetSelectFlg(TRUE);	// �I����Ԃ����ɖ߂�
		// ���݂̉��H�w���̉��\��������
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
		m_pSelData->SetSelectFlg(TRUE);	// �I����Ԃ����ɖ߂�
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
			// �O��̖�������
			DrawTempArraw(pDC);
		}
		m_nSelect = nSelect;
		// �V�������̕`��
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
			// �O��̊J�n�ʒu������
			DrawTempStart(pDC);
		}
		m_nSelect = nSelect;
		// �V�����J�n�ʒu�̕`��
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
			// �O��̗֊s������
			DrawTempOutline(pDC);
		}
		m_nSelect = nSelect;
		// �V�������̕`��
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
