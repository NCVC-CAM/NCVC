// NCViewXZ.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "NCChild.h"
#include "NCdata.h"
#include "NCDoc.h"
#include "NCViewTab.h"
#include "NCViewXZ.h"
#include "ViewOption.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNCViewXZ

IMPLEMENT_DYNCREATE(CNCViewXZ, CView)

BEGIN_MESSAGE_MAP(CNCViewXZ, CView)
	//{{AFX_MSG_MAP(CNCViewXZ)
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
// CNCViewXZ �N���X�̍\�z/����

CNCViewXZ::CNCViewXZ()
{
}

CNCViewXZ::~CNCViewXZ()
{
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewXZ �N���X�̃I�[�o���C�h�֐�

BOOL CNCViewXZ::PreCreateWindow(CREATESTRUCT& cs) 
{
	// ���د�����̋��E���ق��ڂ�̂ŁCIDC_ARROW �𖾎��I�Ɏw��
	cs.lpszClass = AfxRegisterWndClass(
			CS_HREDRAW|CS_VREDRAW|CS_OWNDC|CS_DBLCLKS,
			AfxGetApp()->LoadStandardCursor(IDC_ARROW) );
	return CView::PreCreateWindow(cs);
}

void CNCViewXZ::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
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

BOOL CNCViewXZ::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
	// �������� CDocument::OnCmdMsg() ���Ă΂Ȃ��悤�ɂ���
//	return CView::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
	return CWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewXZ �N���X�̃����o�֐�

void CNCViewXZ::SetGuideData(void)
{
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	double	dSrc = pOpt->IsGuideSync() ? m_dFactor*LOMETRICFACTOR : LOMETRICFACTOR;
	// �w���̃K�C�h�������i������E�ցj
	m_ptGuid[0][0].x = (int)(-pOpt->GetGuideLength(NCA_X) * dSrc);
	m_ptGuid[0][0].y = 0;
	m_ptGuid[0][1].x = (int)( pOpt->GetGuideLength(NCA_X) * dSrc);
	m_ptGuid[0][1].y = 0;
	// �y���̃K�C�h�������i�ォ�牺�ցj
	m_ptGuid[1][0].x = 0;
	m_ptGuid[1][0].y = (int)( pOpt->GetGuideLength(NCA_Z) * dSrc);
	m_ptGuid[1][1].x = 0;
	m_ptGuid[1][1].y = (int)(-pOpt->GetGuideLength(NCA_Z) * dSrc);
}

void CNCViewXZ::DrawConvertWorkRect(void)
{
	CRect3D	rcWork(GetDocument()->GetWorkRect());
	CRectD	rc(rcWork.left, rcWork.high, rcWork.right, rcWork.low);
	rc.NormalizeRect();
	m_rcDrawWork.left	= DrawConvert(rc.left);
	m_rcDrawWork.top	= DrawConvert(rc.top);
	m_rcDrawWork.right	= DrawConvert(rc.right);
	m_rcDrawWork.bottom	= DrawConvert(rc.bottom);
}

void CNCViewXZ::DrawConvertMaxRect(void)
{
	CRect3D	rcMax(GetDocument()->GetMaxRect());
	CRectD	rc(rcMax.left, rcMax.high, rcMax.right, rcMax.low);
	rc.NormalizeRect();
	m_rcDrawMax.left	= DrawConvert(rc.left);
	m_rcDrawMax.top		= DrawConvert(rc.top);
	m_rcDrawMax.right	= DrawConvert(rc.right);
	m_rcDrawMax.bottom	= DrawConvert(rc.bottom);
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewXZ �`��

void CNCViewXZ::OnDraw(CDC* pDC)
{
	extern	LPCTSTR	g_szNdelimiter;	// "XYZRIJKPLDH" from NCDoc.cpp

	ASSERT_VALID(GetDocument());
	int		i;
	pDC->SetROP2(R2_COPYPEN);
	CPen* pOldPen = (CPen *)pDC->SelectStockObject(NULL_PEN);

	// ٰ�߂̂��߂̏����ݒ�
	CPen*		pOrgPen[2];
	COLORREF	colOrg[2];
	CString		strXZ[2];
	pOrgPen[0]	= AfxGetNCVCMainWnd()->GetPenOrg(NCA_X);
	pOrgPen[1]	= AfxGetNCVCMainWnd()->GetPenOrg(NCA_Z);
	colOrg[0]	= AfxGetNCVCMainWnd()->GetOrgColor(NCA_X);
	colOrg[1]	= AfxGetNCVCMainWnd()->GetOrgColor(NCA_Z);
	strXZ[0] = g_szNdelimiter[NCA_X];
	strXZ[1] = g_szNdelimiter[NCA_Z];

	// ���ʈē�
	if ( pDC->m_hAttribDC ) {
		CPoint	pt(0, 0);
		pDC->DPtoLP(&pt);
		pDC->SetTextAlign(TA_LEFT|TA_TOP);
		pDC->SetTextColor(AfxGetNCVCApp()->GetViewOption()->GetNcDrawColor(NCCOL_PANE));
		pDC->TextOut(pt.x, pt.y, strXZ[0]+strXZ[1]);
	}
	// �޲�ޕ\��
	pDC->SetTextAlign(TA_CENTER|TA_BOTTOM);
	for ( i=0; i<SIZEOF(m_ptGuid); i++ ) {
		pDC->SelectObject(pOrgPen[i]);
		pDC->MoveTo(m_ptGuid[i][0]);
		pDC->LineTo(m_ptGuid[i][1]);
		pDC->SetTextColor(colOrg[i]);
		pDC->TextOut(m_ptGuid[i][0].x, m_ptGuid[i][0].y, strXZ[i]);
	}
	// NC�ް��`��
	CNCdata*	pData;
	int	nDraw = GetDocument()->GetTraceDraw();	// ��è�پ���݂ɂ��ۯ�
	for ( i=GetDocument()->GetTraceStart(); i<nDraw; i++ ) {
		pData = GetDocument()->GetNCdata(i);
		if ( pData->GetGtype() == G_TYPE )
			pData->DrawXZ(pDC, FALSE);
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
// CNCViewXZ �f�f

#ifdef _DEBUG
void CNCViewXZ::AssertValid() const
{
	CView::AssertValid();
}

void CNCViewXZ::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CNCDoc* CNCViewXZ::GetDocument() // ��f�o�b�O �o�[�W�����̓C�����C���ł��B
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CNCDoc)));
	return (CNCDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CNCViewXZ �N���X�̃��b�Z�[�W �n���h���i���j���[�ҁj

void CNCViewXZ::OnUpdateEditCopy(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(TRUE);
}

void CNCViewXZ::OnEditCopy() 
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

void CNCViewXZ::OnMoveKey(UINT nID)
{
	CViewBase::OnMoveKey(nID);
}

void CNCViewXZ::OnLensKey(UINT nID)
{
	switch ( nID ) {
	case ID_VIEW_BEFORE:
		CViewBase::OnBeforeMagnify();
		break;
	case ID_VIEW_FIT:
		CViewBase::OnViewFit(GetMaxRect());
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

void CNCViewXZ::OnViewLensComm(void)
{
	// �g�嗦�ɂ��`����̍X�V
	GetDocument()->AllChangeFactorXZ(m_dFactor);
	// �޲�ގ�
	if ( AfxGetNCVCApp()->GetViewOption()->IsGuideSync() )
		SetGuideData();
	// �e��`���̍X�V
	if ( GetDocument()->IsWorkRect() )
		DrawConvertWorkRect();
	DrawConvertMaxRect();
	// MDI�q�ڰт̽ð���ް�ɏ��\��
	static_cast<CNCChild *>(GetParentFrame())->SetFactorInfo(NC_XZ_PLANE, m_dFactor);
	// �ޭ��̍ĕ`��
	Invalidate();
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewXZ ���b�Z�[�W �n���h��

int CNCViewXZ::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	// ϯ��ݸ�Ӱ�ނ̕ύX�Ȃ�
	CViewBase::OnCreate(this);
	// �޲���ް�
	SetGuideData();

	return 0;
}

LRESULT CNCViewXZ::OnUserActivatePage(WPARAM, LPARAM lParam)
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

LRESULT CNCViewXZ::OnUserViewFitMsg(WPARAM, LPARAM lParam)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CNCViewXZ::OnUserViewFitMsg()\nStart");
#endif
	// �g�嗦�̌v�Z�Ɛ}�`̨��
	CViewBase::OnViewFit(GetMaxRect());
	if ( (BOOL)lParam )		// �ĕ`���׸� == ���ݱ�è���߰��
		OnViewLensComm();
	return 0;
}

void CNCViewXZ::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) 
{
	if ( bActivate ) {
		// MDI�q�ڰт̽ð���ް�ɏ��\��
		static_cast<CNCChild *>(GetParentFrame())->SetFactorInfo(NC_XZ_PLANE, m_dFactor);
	}
	CView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}

void CNCViewXZ::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	CViewBase::OnContextMenu(point, IDR_NCPOPUP1);
}

void CNCViewXZ::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CViewBase::OnLButtonDown(point);
}

void CNCViewXZ::OnLButtonUp(UINT nFlags, CPoint point) 
{
	if ( CViewBase::OnLButtonUp(point) == 1 )
		OnLensKey(ID_VIEW_LENSP);		// �g�又��
}

void CNCViewXZ::OnRButtonDown(UINT nFlags, CPoint point) 
{
	CViewBase::OnRButtonDown(point);
}

void CNCViewXZ::OnRButtonUp(UINT nFlags, CPoint point) 
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

void CNCViewXZ::OnMouseMove(UINT nFlags, CPoint point) 
{
	CViewBase::OnMouseMove(nFlags, point);
}

BOOL CNCViewXZ::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
	if ( CViewBase::OnMouseWheel(nFlags, zDelta, pt) )
		OnViewLensComm();

	return TRUE;
}

void CNCViewXZ::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	// ���د�����޳������ٸد������΁CXZ�P�̕\���ɐ؂�ւ�
	if ( !GetParent()->IsKindOf(RUNTIME_CLASS(CNCViewTab)) ) {
		static_cast<CNCChild *>(GetParentFrame())->GetMainView()->DblClkChange(2);
		return;
	}

	CView::OnLButtonDblClk(nFlags, point);
}

BOOL CNCViewXZ::OnEraseBkgnd(CDC* pDC) 
{
	CRect	rc;
	GetClientRect(&rc);

	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	COLORREF	col1 = pOpt->GetNcDrawColor(NCCOL_BACKGROUND2),
				col2 = pOpt->GetNcDrawColor(NCCOL_BACKGROUND1);

	return AfxGetNCVCMainWnd()->DrawBackGroundView(pDC, &rc, col1, col2);
}
