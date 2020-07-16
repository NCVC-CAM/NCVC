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
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
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
	m_enProcessDirect = DXFPROCESS_SELECT;
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
	((CDXFChild *)GetParentFrame())->SetDataInfo(
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
				if ( i != m_nSelect ) {	// m_nSelect����CDXFworkingOutline���޽�׸��ɂ�delete
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
		// ���H���̓o�^
		pShape->AddWorkingData(pWork);
		// �`���ذ(���H�w��)�̍X�V�ƑI��
		GetDocument()->UpdateAllViews(this, UAV_DXFADDWORKING, (CObject *)pWork);
		// �޷���ĕύX�ʒm
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

	// ϳ����W������O��޼ު�Ă̑I��
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

	// TH_ShapeSearch.cpp�ɂĎn�_�I�_�̒������s���Ă���(SetDirectionFixed)�̂�
	// �P��ٰ�߂ŗǂ�
	while ( pos ) {
		pData2 = pChain->GetNext(pos);
		// �̾�č��W�v�Z
		ptResult = pData1->CalcOffsetIntersectionPoint(pData2, 1.0, bLeft);
		if ( !ptResult )
			return FALSE;
		// �֊s��޼ު�Đ���(����͖���)
		if ( pt ) {
			pData = CreateOutlineTempObject_new(pData1, *pt, *ptResult);
			if ( pData )
				m_obOutline[n].Add(pData);
		}
		else
			pte = *ptResult;	// �ŏ��̗֊s��޼ު�Ă̏I�_
		pt = ptResult;
		pData1 = pData2;
	}
	// �ŏ��̗֊s��޼ު�Đ���
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
	int	i, j;
	for ( i=0; i<SIZEOF(m_obOutline); i++ ) {
		for ( j=0; j<m_obOutline[i].GetSize(); j++ )
			m_obOutline[i][j]->DrawTuning(m_dFactor*LOMETRICFACTOR);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CDXFView �`��

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
	return (CDXFDoc*)m_pDocument;
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
	((CDXFChild *)GetParentFrame())->SetFactorInfo(m_dFactor);
	// �ޭ��̍ĕ`��
	Invalidate();
}

/////////////////////////////////////////////////////////////////////////////
// CDXFView ���b�Z�[�W �n���h��

int CDXFView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
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
	double		dGap;
	CDXFdata*	pData  = NULL;
	CDXFshape*	pShape = NULL;
	// ���W�l�v�Z(point��CViewBase::OnLButtonUp()�Ř_�����W�ɕϊ��ς�)
	CPointD	pt(point);
	pt /= ( m_dFactor * LOMETRICFACTOR );
	CRect	rc;
	GetClientRect(&rc);
	dc.DPtoLP(&rc);
	rc.NormalizeRect();
	CRectD	rcView(rc);
	rcView /= ( m_dFactor * LOMETRICFACTOR );

	// ϳ��د���ԑJ��
	if ( m_vSelect.which() == DXFTREETYPE_SHAPE ) {
		// �W���I�����
		pShape = get<CDXFshape*>(m_vSelect);
		tie(pData, dGap) = pShape->GetSelectViewGap(pt, rcView);
		if ( pData && dGap < SELECTGAP/m_dFactor ) {
			// �w����W�Ƃ̋�����臒l�����Ȃ�
			switch ( GetDocument()->GetShapePattern() ) {
			case ID_EDIT_SHAPE_SEL:	// ����
				OnLButtonUp_Sel(pShape, pData, &dc);
				break;
			case ID_EDIT_SHAPE_VEC:	// ����
				OnLButtonUp_Vec(pShape, pData, pt, &dc);
				break;
			case ID_EDIT_SHAPE_OUT:	// �֊s
			case ID_EDIT_SHAPE_POC:	// �߹��
				OnLButtonUp_Out(pShape, pData, pt, &dc);
				break;
			}
			return;
		}
	}

	// �w����W�Ƃ̋�����臒l�ȏォ�W����I����ԂȂ�C�S�̌���
	tie(pShape, dGap) = GetDocument()->GetSelectViewGap(pt, rcView);
	if ( pShape && dGap < SELECTGAP/m_dFactor ) {
		// �ذ�̑I���� m_vSelect �̍X�V�ʒm
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
		// ���̏W��(�O�ՏW���̂�)���ݸ�ł��邩�ǂ���
		CLayerData*	pLayer = m_pSelData->GetParentLayer();
		CDXFshape*	pShape;
		CDXFshape*	pShapeLink = NULL;
		CDXFmap*	pMap;
		for ( int i=0; i<pLayer->GetShapeSize(); i++ ) {
			pShape = pLayer->GetShapeData(i);
			if ( pShapeOrg!=pShape && (pMap=pShape->GetShapeMap()) ) {
				// ���W����
				CPointD	pt;
				for ( int j=0; j<pDataOrg->GetPointNumber(); j++ ) {
					pt = pDataOrg->GetNativePoint(j);	// �ŗL���W�l
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
			// �����w���̊m�F
			CDXFworking*	pWork;
			CDXFdata*	pData;
			tie(pWork, pData) = pShapeOrg->GetDirectionObject();
			// �W����V�K�쐬
			pMap = NULL;
			try {
				pMap = new CDXFmap;
				pMap->SetPointMap(const_cast<CDXFdata*>(pDataOrg));
				pShape = new CDXFshape(DXFSHAPE_LOCUS, pShapeOrg->GetShapeName()+"(����)",
									pMap->GetShapeFlag(), pMap);
				// �����w���̌p��
				if ( pData == pDataOrg )
					pShapeOrg->DelWorkingData(pWork, pShape);	// �t���ւ�
				// ڲԏ��ɒǉ�
				pLayer->AddShape(pShape);
				// �ذ�ޭ��ւ̒ʒm
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
		// �I���׸ނ̸ر
		pShapeOrg->SetShapeSwitch(FALSE);
		// ���W�������޼ު�Ă�����
		pShapeOrg->RemoveObject(pDataOrg);
		// �W������
		if ( pShapeOrg->LinkObject() ) {
			// �ذ�ޭ��ւ̒ʒm
			addShape.pLayer = pLayer;
			addShape.pShape = pShapeOrg;
			GetDocument()->UpdateAllViews(this, UAV_DXFADDSHAPE, (CObject *)&addShape);
		}
		// �ݸ�o�����Ƃ����ݸ��W��������
		if ( pShapeLink ) {
			pShape = pShapeLink;
			if ( pShapeLink->LinkObject() ) {
				// �ذ�ޭ��ւ̒ʒm
				addShape.pLayer = pLayer;
				addShape.pShape = pShapeLink;
				GetDocument()->UpdateAllViews(this, UAV_DXFADDSHAPE, (CObject *)&addShape);
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
		if ( pShapeOrg->GetObjectCount() <= 1 ) {
			::MessageBeep(MB_ICONEXCLAMATION);
			return;
		}
		m_enProcessDirect = DXFPROCESS_SELECT;
		m_pSelData = const_cast<CDXFdata*>(pDataOrg);
		m_nSelect = 0;	// dummy
		// �W���S�̂��I����Ԃ̃n�Y�Ȃ̂ŁC�I���޼ު�Ă̂݉���
		m_pSelData->SetSelectFlg(FALSE);
		DrawTemporaryProcess(pDC);
	}
}

void CDXFView::OnLButtonUp_Vec
	(const CDXFshape* pShape, const CDXFdata* pData, const CPointD& pt, CDC* pDC)
{
	if ( m_pSelData ) {
		// ��޼ު�Ăɑ΂�������w��
		// �n�_�͌��݈ʒu�ɉ�����(1-m_nSelect)�C�I�_�͋߂���(m_nSelect)
		m_nSelect = GAPCALC(m_ptArraw[0][1] - pt) < GAPCALC(m_ptArraw[1][1] - pt) ? 0 : 1;
		m_pSelData->SetDirectionFixed(m_ptArraw[1-m_nSelect][1], m_ptArraw[m_nSelect][1]);
		// ���H�w������
		CDXFworking* pWork = CreateWorkingData();
		// ���H�w���`��
		pWork->DrawTuning(m_dFactor*LOMETRICFACTOR);
		CPen* pOldPen = pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenDXF(DXFPEN_WORKER));
		pDC->SetROP2(R2_COPYPEN);
		pWork->Draw(pDC);
		pDC->SelectObject(pOldPen);
		// �w���޼ު��ؾ��
		m_pSelData = NULL;
	}
	else {
		if ( pShape->GetShapeFlag() & DXFMAPFLG_DIRECTION ) {
			::MessageBeep(MB_ICONEXCLAMATION);
			return;
		}
		m_enProcessDirect = DXFPROCESS_ARRAW;
		m_pSelData = const_cast<CDXFdata*>(pData);
		// �����W�̎擾
		pData->GetDirectionArraw(pt, m_ptArraw);
		// ���݈ʒu�ɋ߂����̖���`��
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
		// �֊s��޼ު�Đ���
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

	UINT		nShape = GetDocument()->GetShapePattern();
	int			nSelect;
	CDXFshape*	pShape = get<CDXFshape*>(m_vSelect);
	CDXFdata*	pData = NULL;
	double		dGap;
	// CViewBase::OnMouseMove() �ŏ������Ȃ����
	// �����Ř_�����W�ɕϊ�����K�v������
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

	// �I���W���Ƃ̋����v�Z
	tie(pData, dGap) = pShape->GetSelectViewGap(pt, rcView);
	if ( pData && dGap < SELECTGAP/m_dFactor ) {
		switch ( nShape ) {
		case ID_EDIT_SHAPE_SEL:
			if ( m_pSelData != pData ) {
				m_pSelData->SetSelectFlg(TRUE);	// �I����Ԃ����ɖ߂�
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
					// �O��̖�������
					DrawArraw(&dc);
				}
				m_nSelect = nSelect;
				// �V�������̕`��
				DrawArraw(&dc);
			}
			break;
		case ID_EDIT_SHAPE_OUT:
		case ID_EDIT_SHAPE_POC:
			break;
		}
	}
	else {
		// 臒l�𒴂����
		if ( m_nSelect >= 0 ) {
			if ( nShape == ID_EDIT_SHAPE_SEL )
				m_pSelData->SetSelectFlg(TRUE);	// �I����Ԃ����ɖ߂�
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
