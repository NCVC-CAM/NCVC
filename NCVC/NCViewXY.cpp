// NCViewXY.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "NCChild.h"
#include "NCdata.h"
#include "NCDoc.h"
#include "NCViewTab.h"
#include "NCViewXY.h"
#include "ViewOption.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNCViewXY

IMPLEMENT_DYNCREATE(CNCViewXY, CView)

BEGIN_MESSAGE_MAP(CNCViewXY, CView)
	//{{AFX_MSG_MAP(CNCViewXY)
	ON_WM_CREATE()
	ON_WM_CONTEXTMENU()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_ERASEBKGND()
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	//}}AFX_MSG_MAP
	// �߰�ސֲؑ����
	ON_MESSAGE (WM_USERACTIVATEPAGE, OnUserActivatePage)
	// �e�ޭ��ւ�̨��ү����
	ON_MESSAGE (WM_USERVIEWFITMSG, OnUserViewFitMsg)
	// �ƭ������
	ON_COMMAND_RANGE(ID_VIEW_UP, ID_VIEW_RT, OnMoveKey)
	ON_COMMAND_RANGE(ID_VIEW_BEFORE, ID_VIEW_LENSN, OnLensKey)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNCViewXY �N���X�̍\�z/����

CNCViewXY::CNCViewXY()
{
}

CNCViewXY::~CNCViewXY()
{
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewXY �N���X�̃I�[�o���C�h�֐�

BOOL CNCViewXY::PreCreateWindow(CREATESTRUCT& cs) 
{
	// ���د�����̋��E���ق��ڂ�̂ŁCIDC_ARROW �𖾎��I�Ɏw��
	cs.lpszClass = AfxRegisterWndClass(
			CS_HREDRAW|CS_VREDRAW|CS_OWNDC|CS_DBLCLKS,
			AfxGetApp()->LoadStandardCursor(IDC_ARROW) );
	return CView::PreCreateWindow(cs);
}

void CNCViewXY::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	switch ( lHint ) {
	case UAV_TRACECURSOR:
		Invalidate();
		UpdateWindow();		// ���ĕ`��
		return;
	case UAV_FILEINSERT:
		OnLensKey(ID_VIEW_FIT);
		UpdateWindow();
		return;
	case UAV_DRAWWORKRECT:
		{
			CClientDC	dc(this);
			// �O��̕`�������
			if ( GetDocument()->IsWorkRect() ) {
				dc.SetROP2(R2_XORPEN);
				DrawWorkRect(&dc);	// CNCViewBase
				dc.SetROP2(R2_COPYPEN);
			}
			// �`��p���ް��X�V
			if ( pHint ) {
				DrawConvertWorkRect();
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

BOOL CNCViewXY::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
	// �������� CDocument::OnCmdMsg() ���Ă΂Ȃ��悤�ɂ���
//	return CView::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
	return CWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewXY �N���X�̃����o�֐�

void CNCViewXY::SetGuideData(void)
{
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	double	dSrc = pOpt->IsGuideSync() ? m_dFactor*LOMETRICFACTOR : LOMETRICFACTOR;
	// �w���̃K�C�h�������i������E�ցj
	m_ptGuid[NCA_X][0].x = (int)(-pOpt->GetGuideLength(NCA_X) * dSrc);
	m_ptGuid[NCA_X][0].y = 0;
	m_ptGuid[NCA_X][1].x = (int)( pOpt->GetGuideLength(NCA_X) * dSrc);
	m_ptGuid[NCA_X][1].y = 0;
	// �x���̃K�C�h�������i�������O�ցj
	m_ptGuid[NCA_Y][0].x = 0;
	m_ptGuid[NCA_Y][0].y = (int)( pOpt->GetGuideLength(NCA_Y) * dSrc);
	m_ptGuid[NCA_Y][1].x = 0;
	m_ptGuid[NCA_Y][1].y = (int)(-pOpt->GetGuideLength(NCA_Y) * dSrc);
}

void CNCViewXY::DrawConvertWorkRect(void)
{
	CRectD	rc(GetDocument()->GetWorkRect());
	m_rcDrawWork.left	= DrawConvert(rc.left);
	m_rcDrawWork.top	= DrawConvert(rc.top);
	m_rcDrawWork.right	= DrawConvert(rc.right);
	m_rcDrawWork.bottom	= DrawConvert(rc.bottom);
}

void CNCViewXY::DrawConvertMaxRect(void)
{
	CRectD	rc(GetDocument()->GetMaxRect());
	m_rcDrawMax.left	= DrawConvert(rc.left);
	m_rcDrawMax.top		= DrawConvert(rc.top);
	m_rcDrawMax.right	= DrawConvert(rc.right);
	m_rcDrawMax.bottom	= DrawConvert(rc.bottom);
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewXY �`��

void CNCViewXY::OnDraw(CDC* pDC)
{
	extern	LPCTSTR	g_szNdelimiter;	// "XYZRIJKPLDH" from NCDoc.cpp

	ASSERT_VALID(GetDocument());
	int		i;
	pDC->SetROP2(R2_COPYPEN);
	CPen* pOldPen = (CPen *)pDC->SelectStockObject(NULL_PEN);

	// ���ʈē�
	if ( pDC->m_hAttribDC ) {
		CPoint	pt(0, 0);
		pDC->DPtoLP(&pt);
		pDC->SetTextAlign(TA_LEFT|TA_TOP);
		pDC->SetTextColor(AfxGetNCVCApp()->GetViewOption()->GetNcDrawColor(NCCOL_PANE));
		pDC->TextOut(pt.x, pt.y, g_szNdelimiter, 2);	// XY
	}
	// �޲�ޕ\��
	pDC->SetTextAlign(TA_CENTER|TA_BOTTOM);
	for ( i=0; i<SIZEOF(m_ptGuid); i++ ) {
		pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenOrg(i));
		pDC->MoveTo(m_ptGuid[i][0]);
		pDC->LineTo(m_ptGuid[i][1]);
		pDC->SetTextColor(AfxGetNCVCMainWnd()->GetOrgColor(i));
		pDC->TextOut(m_ptGuid[i][0].x, m_ptGuid[i][0].y, g_szNdelimiter+i, 1);
	}
	// NC�ް��`��
	CNCdata*	pData;
	int	nDraw = GetDocument()->GetTraceDraw();	// ��è�پ���݂ɂ��ۯ�
	for ( i=GetDocument()->GetTraceStart(); i<nDraw; i++ ) {
		pData = GetDocument()->GetNCdata(i);
		if ( pData->GetGtype() == G_TYPE )
			pData->DrawXY(pDC, FALSE);
	}
	// �ő�؍��`
	if ( GetDocument()->IsMaxRect() )
		DrawMaxRect(pDC);	// CNCViewBase
	// ܰ���`
	if ( GetDocument()->IsWorkRect() )
		DrawWorkRect(pDC);	// CNCViewBase
	// ���݂̊g���`��`��(ViewBase.cpp)
	if ( m_bMagRect )
		DrawMagnifyRect(pDC);

	pDC->SelectObject(pOldPen);
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewXY �f�f

#ifdef _DEBUG
void CNCViewXY::AssertValid() const
{
	CView::AssertValid();
}

void CNCViewXY::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CNCDoc* CNCViewXY::GetDocument() // ��f�o�b�O �o�[�W�����̓C�����C���ł��B
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CNCDoc)));
	return (CNCDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CNCViewXY �N���X�̃��b�Z�[�W �n���h���i���j���[�ҁj

void CNCViewXY::OnUpdateEditCopy(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(TRUE);
}

void CNCViewXY::OnEditCopy() 
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

void CNCViewXY::OnMoveKey(UINT nID)
{
	CViewBase::OnMoveKey(nID);
}

void CNCViewXY::OnLensKey(UINT nID)
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

void CNCViewXY::OnViewLensComm(void)
{
	// �g�嗦�ɂ��`����̍X�V
	GetDocument()->AllChangeFactorXY(m_dFactor);
	// �޲�ގ�
	if ( AfxGetNCVCApp()->GetViewOption()->IsGuideSync() )
		SetGuideData();
	// �e��`���̍X�V
	if ( GetDocument()->IsWorkRect() )
		DrawConvertWorkRect();
	DrawConvertMaxRect();
	// MDI�q�ڰт̽ð���ް�ɏ��\��
	static_cast<CNCChild *>(GetParentFrame())->SetFactorInfo(NC_XY_PLANE, m_dFactor);
	// �ޭ��̍ĕ`��
	Invalidate();
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewXY ���b�Z�[�W �n���h��

int CNCViewXY::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	// ϯ��ݸ�Ӱ�ނ̕ύX�Ȃ�
	CViewBase::OnCreate(this);
	// �޲���ް�
	SetGuideData();

	return 0;
}

LRESULT CNCViewXY::OnUserActivatePage(WPARAM, LPARAM lParam)
{
	if ( m_dFactor == 0.0 ) {
		// OnUserInitialUpdate() �����s����Ă��Ȃ��Ƃ�
		OnLensKey(ID_VIEW_FIT);
	}
	else {
		if ( (BOOL)lParam ) {
			// �g�嗦�̍čX�V
			OnViewLensComm();
		}
	}
	return 0;
}

LRESULT CNCViewXY::OnUserViewFitMsg(WPARAM, LPARAM lParam)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CNCViewXY::OnUserViewFitMsg()\nStart");
#endif
	// �g�嗦�̌v�Z�Ɛ}�`̨��
	CViewBase::OnViewFit(GetDocument()->GetMaxRect());
	if ( (BOOL)lParam )		// �ĕ`���׸� == ���ݱ�è���߰��
		OnViewLensComm();
	return 0;
}

void CNCViewXY::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) 
{
	if ( bActivate ) {
		// MDI�q�ڰт̽ð���ް�ɏ��\��
		static_cast<CNCChild *>(GetParentFrame())->SetFactorInfo(NC_XY_PLANE, m_dFactor);
	}
	CView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}

void CNCViewXY::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	CViewBase::OnContextMenu(point, IDR_NCPOPUP1);
}

void CNCViewXY::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CViewBase::OnLButtonDown(point);
}

void CNCViewXY::OnLButtonUp(UINT nFlags, CPoint point) 
{
	if ( CViewBase::OnLButtonUp(point) == 1 )
		OnLensKey(ID_VIEW_LENSP);		// �g�又��
}

void CNCViewXY::OnRButtonDown(UINT nFlags, CPoint point) 
{
	CViewBase::OnRButtonDown(point);
}

void CNCViewXY::OnRButtonUp(UINT nFlags, CPoint point) 
{
	switch ( CViewBase::OnRButtonUp(point) ) {
	case 1:		// �g�又��
		OnLensKey(ID_VIEW_LENSP);
		break;
	case 2:		// ��÷���ƭ��\��
		CView::OnRButtonUp(nFlags, point);
		break;
	}
}

void CNCViewXY::OnMouseMove(UINT nFlags, CPoint point) 
{
	CViewBase::OnMouseMove(nFlags, point);
}

BOOL CNCViewXY::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
	if ( CViewBase::OnMouseWheel(nFlags, zDelta, pt) )
		OnViewLensComm();

	return TRUE;
}

void CNCViewXY::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	// ���د�����޳������ٸد������΁CXY�P�̕\���ɐ؂�ւ�
	if ( !GetParent()->IsKindOf(RUNTIME_CLASS(CNCViewTab)) ) {
		static_cast<CNCChild *>(GetParentFrame())->GetMainView()->DblClkChange(1);
		return;
	}

	CView::OnLButtonDblClk(nFlags, point);
}

BOOL CNCViewXY::OnEraseBkgnd(CDC* pDC) 
{
	CRect	rc;
	GetClientRect(&rc);

	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	COLORREF	col1 = pOpt->GetNcDrawColor(NCCOL_BACKGROUND2),
				col2 = pOpt->GetNcDrawColor(NCCOL_BACKGROUND1);

	return AfxGetNCVCMainWnd()->DrawBackGroundView(pDC, &rc, col1, col2);
}
