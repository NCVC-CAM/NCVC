// NCView.cpp : CNCView �N���X�̓���̒�`���s���܂��B
//

#include "stdafx.h"
#include "NCVC.h"
#include "MainFrm.h"
#include "NCChild.h"
#include "NCdata.h"
#include "NCDoc.h"
#include "NCViewTab.h"
#include "NCInfoTab.h"
#include "NCView.h"
#include "NCListView.h"
#include "ViewOption.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNCView

IMPLEMENT_DYNCREATE(CNCView, CView)

BEGIN_MESSAGE_MAP(CNCView, CView)
	//{{AFX_MSG_MAP(CNCView)
	ON_WM_CREATE()
	ON_WM_CONTEXTMENU()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_MOUSEACTIVATE()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_KEYDOWN()
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
// CNCView �N���X�̍\�z/����

CNCView::CNCView()
{
}

CNCView::~CNCView()
{
}

/////////////////////////////////////////////////////////////////////////////
// CNCView �N���X�̃I�[�o���C�h�֐�

BOOL CNCView::PreCreateWindow(CREATESTRUCT& cs)
{
	// ���د�����̋��E���ق��ڂ�̂ŁCIDC_ARROW �𖾎��I�Ɏw��
	cs.lpszClass = AfxRegisterWndClass(
			CS_HREDRAW|CS_VREDRAW|CS_OWNDC|CS_DBLCLKS,
			AfxGetApp()->LoadStandardCursor(IDC_ARROW) );
	return CView::PreCreateWindow(cs);
}

void CNCView::OnInitialUpdate() 
{
	extern	LPCTSTR	g_szNdelimiter;	// "XYZRIJKPLDH" from NCDoc.cpp

	CView::OnInitialUpdate();
	SetMaxRect2D();

	// ���ʈē���������
	if ( GetDocument()->IsNCDocFlag(NCDOC_LATHE) ) {
		m_strGuide  = g_szNdelimiter[NCA_Z];	// NCViewBase.h
		m_strGuide += g_szNdelimiter[NCA_Y];
		m_strGuide += g_szNdelimiter[NCA_X];	// [ZYX]
	}
	else {
		m_strGuide = g_szNdelimiter;
		m_strGuide.Delete(NCXYZ, 999);			// [XYZ]
	}
	CNCViewBase::OnInitialUpdate(0);
}

void CNCView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	switch ( lHint ) {
	case UAV_TRACECURSOR:
		Invalidate();
		UpdateWindow();		// ���ĕ`��
		return;
	case UAV_FILEINSERT:
		SetMaxRect2D();
		OnLensKey(ID_VIEW_FIT);
		UpdateWindow();
		return;
	case UAV_DRAWWORKRECT:
		{
			CClientDC	dc(this);
			// �O��̕`�������
			if ( GetDocument()->IsNCDocFlag(NCDOC_WRKRECT) ) {
				dc.SetROP2(R2_XORPEN);
				DrawWorkRect(&dc);
				dc.SetROP2(R2_COPYPEN);
			}
			// �`��p���ް��X�V
			if ( pHint ) {
				SetWorkRect2D();
				ConvertWorkRect();
				dc.SetROP2(R2_COPYPEN);
				DrawWorkRect(&dc);
			}
		}
		return;
	case UAV_DRAWMAXRECT:
		{
			CClientDC	dc(this);
			dc.SetROP2(GetDocument()->IsNCDocFlag(NCDOC_MAXRECT) ? R2_COPYPEN : R2_XORPEN);
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

BOOL CNCView::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
	// �������� CDocument::OnCmdMsg() ���Ă΂Ȃ��悤�ɂ���
//	return CView::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
	return CWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

/////////////////////////////////////////////////////////////////////////////
// CNCView �N���X�̃����o�֐�

void CNCView::SetGuideData(void)
{
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	double	dSrc = pOpt->GetNCViewFlg(NCVIEWFLG_GUIDELENGTH) ?
					m_dFactor*LOMETRICFACTOR : LOMETRICFACTOR;
	CPoint3D	pt;
	double		dLength;

	// �w���̃K�C�h�������i������E�ցj
	dLength = pOpt->GetGuideLength(NCA_X);
	pt.SetPoint(-dLength, 0.0, 0.0);
	m_ptGuide[NCA_X][0] = pt.PointConvert() * dSrc;
	pt.x = dLength;
	m_ptGuide[NCA_X][1] = pt.PointConvert() * dSrc;
	// �x���̃K�C�h�������i�������O�ցj
	dLength = pOpt->GetGuideLength(NCA_Y);
	pt.SetPoint(0.0, dLength, 0.0);
	m_ptGuide[NCA_Y][0] = pt.PointConvert() * dSrc;
	pt.y = -dLength;
	m_ptGuide[NCA_Y][1] = pt.PointConvert() * dSrc;
	// �y���̃K�C�h�������i�ォ�牺�ցj
	dLength = pOpt->GetGuideLength(NCA_Z);
	pt.SetPoint(0.0, 0.0, dLength);
	m_ptGuide[NCA_Z][0] = pt.PointConvert() * dSrc;
	pt.z = -dLength;
	m_ptGuide[NCA_Z][1] = pt.PointConvert() * dSrc;
}

void CNCView::SetMaxRect2D(void)
{
	extern	const	double	g_dDefaultGuideLength;	// 50.0 (ViewOption.cpp)
	CRect3D		rc(GetDocument()->GetMaxRect());
	// ��L��`�̕␳(�s���\���̖h�~)
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	double	dLength;
	if ( rc.Width() <= NCMIN ) {
		dLength = pOpt->GetGuideLength(NCA_X);
		if ( dLength == 0.0 )
			dLength = g_dDefaultGuideLength;
		rc.left  = -dLength;
		rc.right =  dLength;
	}
	if ( rc.Height() <= NCMIN ) {
		dLength = pOpt->GetGuideLength(NCA_Y);
		if ( dLength == 0.0 )
			dLength = g_dDefaultGuideLength;
		rc.top    = -dLength;
		rc.bottom =  dLength;
	}
	if ( rc.Depth() <= NCMIN ) {
		dLength = pOpt->GetGuideLength(NCA_Z);
		if ( dLength == 0.0 )
			dLength = g_dDefaultGuideLength;
		rc.low  = -dLength;
		rc.high =  dLength;
	}

	CPoint3D	pt;
	pt.SetPoint(rc.left,  rc.top,    rc.low);
	m_ptdMaxRect[0][0] = pt.PointConvert();
	pt.SetPoint(rc.right, rc.top,    rc.low);
	m_ptdMaxRect[0][1] = pt.PointConvert();
	pt.SetPoint(rc.right, rc.bottom, rc.low);
	m_ptdMaxRect[0][2] = pt.PointConvert();
	pt.SetPoint(rc.left,  rc.bottom, rc.low);
	m_ptdMaxRect[0][3] = pt.PointConvert();
	pt.SetPoint(rc.left,  rc.top,    rc.high);
	m_ptdMaxRect[1][0] = pt.PointConvert();
	pt.SetPoint(rc.right, rc.top,    rc.high);
	m_ptdMaxRect[1][1] = pt.PointConvert();
	pt.SetPoint(rc.right, rc.bottom, rc.high);
	m_ptdMaxRect[1][2] = pt.PointConvert();
	pt.SetPoint(rc.left,  rc.bottom, rc.high);
	m_ptdMaxRect[1][3] = pt.PointConvert();
	// �ި���ڲ�ɉf���o�����2D�ő��`
	m_rcMaxRect.SetRectMinimum();
	for ( int i=0; i<SIZEOF(m_ptdMaxRect); i++ ) {
		for ( int j=0; j<SIZEOF(m_ptdMaxRect[0]); j++ ) {
			if ( m_rcMaxRect.left > m_ptdMaxRect[i][j].x )
				m_rcMaxRect.left = m_ptdMaxRect[i][j].x;
			if ( m_rcMaxRect.top > m_ptdMaxRect[i][j].y )
				m_rcMaxRect.top = m_ptdMaxRect[i][j].y;
			if ( m_rcMaxRect.right < m_ptdMaxRect[i][j].x )
				m_rcMaxRect.right = m_ptdMaxRect[i][j].x;
			if ( m_rcMaxRect.bottom < m_ptdMaxRect[i][j].y )
				m_rcMaxRect.bottom = m_ptdMaxRect[i][j].y;
		}
	}
}

void CNCView::SetWorkRect2D(void)
{
	CRect3D		rc(GetDocument()->GetWorkRect());
	CPoint3D	pt;

	// ܰ��̈��3D�ް�(Z�ő�)
	pt.SetPoint(rc.left,  rc.top,    rc.high);
	m_ptdWorkRect[0][0] = pt.PointConvert();
	pt.SetPoint(rc.right, rc.top,    rc.high);
	m_ptdWorkRect[0][1] = pt.PointConvert();
	pt.SetPoint(rc.right, rc.bottom, rc.high);
	m_ptdWorkRect[0][2] = pt.PointConvert();
	pt.SetPoint(rc.left,  rc.bottom, rc.high);
	m_ptdWorkRect[0][3] = pt.PointConvert();
	// ܰ��̈��3D�ް�(Z�ŏ�)
	pt.SetPoint(rc.left,  rc.top,    rc.low);
	m_ptdWorkRect[1][0] = pt.PointConvert();
	pt.SetPoint(rc.right, rc.top,    rc.low);
	m_ptdWorkRect[1][1] = pt.PointConvert();
	pt.SetPoint(rc.right, rc.bottom, rc.low);
	m_ptdWorkRect[1][2] = pt.PointConvert();
	pt.SetPoint(rc.left,  rc.bottom, rc.low);
	m_ptdWorkRect[1][3] = pt.PointConvert();
}

void CNCView::ConvertWorkRect(void)
{
	for ( int i=0; i<SIZEOF(m_ptDrawWorkRect); i++ ) {
		for ( int j=0; j<SIZEOF(m_ptDrawWorkRect[0]); j++ )
			m_ptDrawWorkRect[i][j] = DrawConvert(m_ptdWorkRect[i][j]);
	}
}

void CNCView::ConvertMaxRect(void)
{
	for ( int i=0; i<SIZEOF(m_ptDrawMaxRect); i++ ) {
		for ( int j=0; j<SIZEOF(m_ptDrawMaxRect[0]); j++ )
			m_ptDrawMaxRect[i][j] = DrawConvert(m_ptdMaxRect[i][j]);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CNCView �N���X�̕`��

void CNCView::OnDraw(CDC* pDC)
{
	const CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();

	ASSERT_VALID(GetDocument());
	int		i;
	pDC->SetROP2(R2_COPYPEN);
	CPen* pOldPen = (CPen *)pDC->SelectStockObject(NULL_PEN);

	// ���ʈē�
	if ( pDC->m_hAttribDC ) {
		CPoint	pt(0, 0);
		pDC->DPtoLP(&pt);
		pDC->SetTextAlign(TA_LEFT|TA_TOP);
		pDC->SetTextColor(pOpt->GetNcDrawColor(NCCOL_PANE));
		if ( GetDocument()->IsNCDocFlag(NCDOC_THUMBNAIL) ) {
			CGdiObject* pFontOld = pDC->SelectStockObject(SYSTEM_FIXED_FONT);
			pDC->TextOut(pt.x, pt.y, GetDocument()->GetTitle());
			pDC->SelectObject(pFontOld);
		}
		else
			pDC->TextOut(pt.x, pt.y, m_strGuide);
	}
	// �޲�ޕ\��
	if ( !GetDocument()->IsNCDocFlag(NCDOC_THUMBNAIL) ) {
		pDC->SetTextAlign(TA_CENTER|TA_BOTTOM);
		for ( i=0; i<SIZEOF(m_ptGuide); i++ ) {
			if ( pOpt->GetGuideLength(i) > 0 ) {
				pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenOrg(i));
				pDC->MoveTo(m_ptGuide[i][0]);
				pDC->LineTo(m_ptGuide[i][1]);
				pDC->SetTextColor(pOpt->GetNcDrawColor(i+NCCOL_GUIDEX));
				pDC->TextOut(m_ptGuide[i][0].x, m_ptGuide[i][0].y, m_strGuide.Mid(i, 1));
			}
		}
	}
	// NC�ް��`��
	CNCdata*	pData;
	int	nDraw = GetDocument()->GetTraceDraw();	// ��è�پ���݂ɂ��ۯ�
	for ( i=GetDocument()->GetTraceStart(); i<nDraw; i++ ) {
		pData = GetDocument()->GetNCdata(i);
		if ( pData->GetGtype() == G_TYPE )
			pData->Draw(pDC, FALSE);
	}
	if ( !GetDocument()->IsNCDocFlag(NCDOC_THUMBNAIL) ) {
		// �ő�؍��`
		if ( GetDocument()->IsNCDocFlag(NCDOC_MAXRECT) )
			DrawMaxRect(pDC);
		// ܰ���`
		if ( GetDocument()->IsNCDocFlag(NCDOC_WRKRECT) )
			DrawWorkRect(pDC);
		// ���݂̊g���`��`��(ViewBase.cpp)
		if ( m_bMagRect )
			DrawMagnifyRect(pDC);
	}

	pDC->SelectObject(pOldPen);
}

void CNCView::DrawMaxRect(CDC* pDC)
{
	pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(NCPEN_MAXCUT));
	pDC->Polygon(m_ptDrawMaxRect[0], SIZEOF(m_ptDrawMaxRect[0]));
	pDC->Polygon(m_ptDrawMaxRect[1], SIZEOF(m_ptDrawMaxRect[0]));
	for ( int i=0; i<SIZEOF(m_ptDrawMaxRect[0]); i++ ) {
		pDC->MoveTo(m_ptDrawMaxRect[0][i]);
		pDC->LineTo(m_ptDrawMaxRect[1][i]);
	}
}

void CNCView::DrawWorkRect(CDC* pDC)
{
	pDC->SelectObject(AfxGetNCVCMainWnd()->GetPenNC(NCPEN_WORK));
	pDC->Polygon(m_ptDrawWorkRect[0], SIZEOF(m_ptDrawWorkRect[0]));
	pDC->Polygon(m_ptDrawWorkRect[1], SIZEOF(m_ptDrawWorkRect[0]));
	for ( int i=0; i<SIZEOF(m_ptDrawWorkRect[0]); i++ ) {
		pDC->MoveTo(m_ptDrawWorkRect[0][i]);
		pDC->LineTo(m_ptDrawWorkRect[1][i]);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CNCView �N���X�̐f�f

#ifdef _DEBUG
void CNCView::AssertValid() const
{
	CView::AssertValid();
}

void CNCView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CNCDoc* CNCView::GetDocument() // ��f�o�b�O �o�[�W�����̓C�����C���ł��B
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CNCDoc)));
	return static_cast<CNCDoc *>(m_pDocument);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CNCView �N���X�̃��b�Z�[�W �n���h���i���j���[�ҁj

void CNCView::OnUpdateEditCopy(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(TRUE);
}

void CNCView::OnEditCopy() 
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

void CNCView::OnMoveKey(UINT nID)
{
	CViewBase::OnMoveKey(nID);
}

void CNCView::OnLensKey(UINT nID)
{
	switch ( nID ) {
	case ID_VIEW_BEFORE:
		CViewBase::OnBeforeMagnify();
		break;
	case ID_VIEW_FIT:
		CViewBase::OnViewFit(m_rcMaxRect);
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

void CNCView::OnViewLensComm(void)
{
	// �g�嗦�ɂ��`����̍X�V
	GetDocument()->AllChangeFactor(m_dFactor);
	if ( !GetDocument()->IsNCDocFlag(NCDOC_THUMBNAIL) ) {
		// �޲�ގ�
		if ( AfxGetNCVCApp()->GetViewOption()->GetNCViewFlg(NCVIEWFLG_GUIDELENGTH) )
			SetGuideData();
		// �e��`���̍X�V
		if ( GetDocument()->IsNCDocFlag(NCDOC_WRKRECT) )
			ConvertWorkRect();
		ConvertMaxRect();
		// MDI�q�ڰт̽ð���ް�ɏ��\��
		static_cast<CNCChild *>(GetParentFrame())->SetFactorInfo(m_dFactor, m_strGuide.Left(3));
	}
	// �ޭ��̍ĕ`��
	Invalidate();
}

/////////////////////////////////////////////////////////////////////////////
// CNCView �N���X�̃��b�Z�[�W �n���h��

int CNCView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if ( CView::OnCreate(lpCreateStruct) < 0 )
		return -1;

	// ϯ��ݸ�Ӱ�ނ̕ύX�Ȃ�
	CViewBase::OnCreate(this);
	// �޲���ް�
	SetGuideData();

	return 0;
}

void CNCView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) 
{
	if ( bActivate && !GetDocument()->IsNCDocFlag(NCDOC_THUMBNAIL) ) {
		// CNCViewSplit::SetActivePane() ����̱�è�ސݒ�ɑΉ�
		// MDI�q�ڰт̽ð���ް�ɏ��\��
		static_cast<CNCChild *>(GetParentFrame())->SetFactorInfo(m_dFactor, m_strGuide.Left(3));
	}
	CView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}

LRESULT CNCView::OnUserActivatePage(WPARAM, LPARAM lParam)
{
	if ( m_dFactor == 0.0 ) {
		// OnUserInitialUpdate() �����s����Ă��Ȃ��Ƃ�
		OnLensKey(ID_VIEW_FIT);
	}
	else if ( lParam ) {
		// �g�嗦�̍čX�V
		OnViewLensComm();
	}

	return 0;
}

LRESULT CNCView::OnUserViewFitMsg(WPARAM, LPARAM lParam)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CNCView::OnUserViewFitMsg()\nStart");
#endif
	// �g�嗦�̌v�Z�Ɛ}�`̨��
	CViewBase::OnViewFit(m_rcMaxRect);
	if ( lParam )		// �ĕ`���׸� == ���ݱ�è���߰��
		OnViewLensComm();
	return 0;
}

void CNCView::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	CViewBase::OnContextMenu(point, IDR_NCPOPUP1);
}

void CNCView::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CViewBase::OnLButtonDown(point);
}

void CNCView::OnLButtonUp(UINT nFlags, CPoint point) 
{
	if ( CViewBase::OnLButtonUp(point) == 1 )
		OnLensKey(ID_VIEW_LENSP);		// �g�又��
}

void CNCView::OnRButtonDown(UINT nFlags, CPoint point) 
{
	CViewBase::OnRButtonDown(point);
}

void CNCView::OnRButtonUp(UINT nFlags, CPoint point) 
{
	switch ( CViewBase::OnRButtonUp(point) ) {
	case 1:		// �g�又��
		OnLensKey(ID_VIEW_LENSP);
		break;
	case 2:		// ��÷���ƭ��\��
		// ��Ȳٕ\��Ӱ�ނź�÷���ƭ��͕\�������Ȃ�
		if ( !GetDocument()->IsNCDocFlag(NCDOC_THUMBNAIL) )
			CView::OnRButtonUp(nFlags, point);
		break;
	}
}

void CNCView::OnMouseMove(UINT nFlags, CPoint point) 
{
	CViewBase::OnMouseMove(nFlags, point);
}

BOOL CNCView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
	if ( CViewBase::OnMouseWheel(nFlags, zDelta, pt) )
		OnViewLensComm();

	return TRUE;
}

void CNCView::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	if ( GetDocument()->IsNCDocFlag(NCDOC_THUMBNAIL) ) {
		// ��Ȳٺ��۰قɒʒm
		GetParent()->SendMessage(WM_USERFINISH, (WPARAM)this);
	}
	else if ( !GetParent()->IsKindOf(RUNTIME_CLASS(CNCViewTab)) ) {
		// ���د�����޳������ٸد������΁CXYZ�P�̕\���ɐ؂�ւ�
		static_cast<CNCChild *>(GetParentFrame())->GetMainView()->DblClkChange(0);
		return;
	}

	CView::OnLButtonDblClk(nFlags, point);
}

void CNCView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if ( !GetDocument()->IsNCDocFlag(NCDOC_THUMBNAIL) ) {
		if ( nChar == VK_TAB ) {
			CNCChild*	pFrame = static_cast<CNCChild *>(GetParentFrame());
			if ( ::GetKeyState(VK_SHIFT) < 0 )
				pFrame->GetListView()->SetFocus();
			else
				pFrame->GetInfoView()->SetFocus();
		}
	}

	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

int CNCView::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message)
{
	// ���׽���Ăяo���ƁACFrameWnd�֘A���Ă΂��̂�
	// ��Ȳٕ\��Ӱ�ނł͕s�(ASSERT)����������
	if ( GetDocument()->IsNCDocFlag(NCDOC_THUMBNAIL) ) {
		SetFocus();		// ���͂�̫������擾
		return MA_ACTIVATE;
	}
	return CView::OnMouseActivate(pDesktopWnd, nHitTest, message);
}

BOOL CNCView::OnEraseBkgnd(CDC* pDC) 
{
	CRect	rc;
	GetClientRect(rc);
#ifdef _DEBUG
	CMagaDbg	dbg("CNCView::OnEraseBkgnd()", DBG_RED);
	dbg.printf("Start MapMode=%d", pDC->GetMapMode());
	dbg.printf("rc=(%d, %d)", rc.Width(), rc.Height());
#endif

	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	COLORREF	col1 = pOpt->GetNcDrawColor(NCCOL_BACKGROUND2),
				col2 = pOpt->GetNcDrawColor(NCCOL_BACKGROUND1);


	return AfxGetNCVCMainWnd()->DrawBackGroundView(pDC, &rc, col1, col2);
}
