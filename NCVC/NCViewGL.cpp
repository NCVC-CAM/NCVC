// NCViewGL.cpp : �����t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "NCdata.h"
#include "NCDoc.h"
#include "NCViewGL.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

// CNCViewGL

IMPLEMENT_DYNCREATE(CNCViewGL, CView)

BEGIN_MESSAGE_MAP(CNCViewGL, CView)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	// �e�ޭ��ւ�̨��ү����
	ON_MESSAGE (WM_USERVIEWFITMSG, OnUserViewFitMsg)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNCViewGL �N���X�̍\�z/����

CNCViewGL::CNCViewGL()
{
	m_cx = m_cy = 0;
	m_pDC = NULL;
	m_hRC = NULL;
	m_uiDisplayListIndex = 0;

	m_enTrackingMode = TM_NONE;
	m_fRenderingRate = 10;
	m_f3RenderingCenter[0]	= 0.0;
	m_f3RenderingCenter[1]	= 0.0;
	m_f3RenderingCenter[2]	= 0.0;
	m_objectXform[0][0]	= 1.0f; m_objectXform[0][1] = 0.0f; m_objectXform[0][2] = 0.0f; m_objectXform[0][3] = 0.0f;
	m_objectXform[1][0]	= 0.0f; m_objectXform[1][1] = 1.0f; m_objectXform[1][2] = 0.0f; m_objectXform[1][3] = 0.0f;
	m_objectXform[2][0]	= 0.0f; m_objectXform[2][1] = 0.0f; m_objectXform[2][2] = 1.0f; m_objectXform[2][3] = 0.0f;
	m_objectXform[3][0]	= 0.0f; m_objectXform[3][1] = 0.0f; m_objectXform[3][2] = 0.0f; m_objectXform[3][3] = 1.0f;
}

CNCViewGL::~CNCViewGL()
{
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewGL �N���X�̃I�[�o���C�h�֐�

BOOL CNCViewGL::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style |= WS_CLIPSIBLINGS|WS_CLIPCHILDREN;
	return CView::PreCreateWindow(cs);
}

void CNCViewGL::OnInitialUpdate() 
{
	// ��޼ު�ċ�`��5%�傫��
	m_rcMax = GetDocument()->GetMaxRect();
	m_rcMax.InflateRect(m_rcMax.Width()*0.05, m_rcMax.Height()*0.05);
	m_rcMax.NormalizeRect();

	// ���_���W�ϊ�
	::glMatrixMode( GL_MODELVIEW );
	::glLoadIdentity();

	m_uiDisplayListIndex = ::glGenLists(2);
	if( m_uiDisplayListIndex <= 0 )
		return;
	// ���`����ި���ڲؽĐ���
	::glNewList( m_uiDisplayListIndex, GL_COMPILE );
		RenderAxis();
	::glEndList();
	// NC�ް��`����ި���ڲؽĐ���
	::glNewList( m_uiDisplayListIndex+1, GL_COMPILE );
		RenderCode();
	::glEndList();
}

BOOL CNCViewGL::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
	// �������� CDocument::OnCmdMsg() ���Ă΂Ȃ��悤�ɂ���
//	return CView::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
	return CWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewGL �N���X�̃����o�֐�

BOOL CNCViewGL::SetupPixelFormat(void)
{
	static PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		24,
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0,
		32,
		0,
		0,
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};

	int	iPixelFormat;
	if( !(iPixelFormat = ::ChoosePixelFormat(m_pDC->GetSafeHdc(), &pfd)) ) {
		TRACE0( "ChoosePixelFormat is failed" );
		return FALSE;
	}

	if( !::SetPixelFormat(m_pDC->GetSafeHdc(), iPixelFormat, &pfd) ) {
		TRACE0( "SetPixelFormat is failed" );
		return FALSE;
	}

    return TRUE;
}

void CNCViewGL::RenderAxis(void)
{
	extern	const	PENSTYLE	g_penStyle[];	// ViewOption.cpp
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	CPoint3D	pt1, pt2;
	double		dLength;
	COLORREF	col;

	::glPushAttrib( GL_CURRENT_BIT | GL_LINE_BIT );	// �F | �����

	::glLineWidth( 2.0 );
	::glEnable( GL_LINE_STIPPLE );

	// X���̶޲��
	dLength = pOpt->GetGuideLength(NCA_X);
	pt1.SetPoint(-dLength, 0.0, 0.0);
	pt2.SetPoint( dLength, 0.0, 0.0);
	col = pOpt->GetNcDrawColor(NCCOL_GUIDEX);
	::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(NCA_X)].nGLpattern);
	::glBegin( GL_LINES );
	::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
	::glVertex3d(pt1.x, pt1.y, pt1.z);
	::glVertex3d(pt2.x, pt2.y, pt2.z);
	::glEnd();
	// Y���̶޲��
	dLength = pOpt->GetGuideLength(NCA_Y);
	pt1.SetPoint(0.0, -dLength, 0.0);
	pt2.SetPoint(0.0,  dLength, 0.0);
	col = pOpt->GetNcDrawColor(NCCOL_GUIDEY);
	::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(NCA_Y)].nGLpattern);
	::glBegin( GL_LINES );
	::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
	::glVertex3d(pt1.x, pt1.y, pt1.z);
	::glVertex3d(pt2.x, pt2.y, pt2.z);
	::glEnd();
	// Z���̶޲��
	dLength = pOpt->GetGuideLength(NCA_Z);
	pt1.SetPoint(0.0, 0.0, -dLength);
	pt2.SetPoint(0.0, 0.0,  dLength);
	col = pOpt->GetNcDrawColor(NCCOL_GUIDEZ);
	::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(NCA_Z)].nGLpattern);
	::glBegin( GL_LINES );
	::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
	::glVertex3d(pt1.x, pt1.y, pt1.z);
	::glVertex3d(pt2.x, pt2.y, pt2.z);
	::glEnd();

	::glPopAttrib();
}

void CNCViewGL::RenderCode(void)
{
	::glPushAttrib( GL_CURRENT_BIT | GL_LINE_BIT );	// �F | �����
	::glEnable( GL_LINE_STIPPLE );
	// NC�ް��`��
	for ( int i=0; i<GetDocument()->GetNCsize(); i++ )
		GetDocument()->GetNCdata(i)->DrawGL();
	::glPopAttrib();
}

void CNCViewGL::ptov( const CPoint& pt, GLfloat v[3] )
{
	GLfloat	d, a;
	v[0] = ( 2.0f * pt.x - m_cx ) / m_cx * 0.5f;
	v[1] = ( m_cy - 2.0f * pt.y ) / m_cy * 0.5f;
	d = (GLfloat)_hypot( v[0], v[1] );
	v[2] = (GLfloat)cos( (PI/2.0) * ( ( d < 1.0 ) ? d : 1.0 ) );

	a = 1.0f / (GLfloat)sqrt( v[0]*v[0] + v[1]*v[1] + v[2]*v[2] );
	v[0] *= a;
	v[1] *= a;
	v[2] *= a;
}

void CNCViewGL::BeginTracking(const CPoint& pt, ENTRACKINGMODE enTrackingMode)
{
	SetCapture();
	m_enTrackingMode = enTrackingMode;
	switch( m_enTrackingMode ) {
	case TM_NONE:
		break;
	case TM_SPIN:
		ptov( pt, m_f3LastPos );
		break;
	case TM_PAN:
		m_ptLast = pt;
		break;
	case TM_ZOOM:
		m_ptLast = pt;
		break;
	}
}

void CNCViewGL::EndTracking(void)
{
	ReleaseCapture();
	m_enTrackingMode = TM_NONE;
}

void CNCViewGL::DoTracking( const CPoint& pt )
{
	switch( m_enTrackingMode ) {
	case TM_NONE:
		break;
	case TM_SPIN:
	{
		GLfloat f3CurPos[3];
		ptov( pt, f3CurPos );
		GLfloat dx = f3CurPos[0] - m_f3LastPos[0];
		GLfloat dy = f3CurPos[1] - m_f3LastPos[1];
		GLfloat dz = f3CurPos[2] - m_f3LastPos[2];
		float fAngle = 180.0f * (GLfloat)sqrt( dx*dx + dy*dy + dz *dz );
		float fX = m_f3LastPos[1]*f3CurPos[2] - m_f3LastPos[2]*f3CurPos[1];
		float fY = m_f3LastPos[2]*f3CurPos[0] - m_f3LastPos[0]*f3CurPos[2];
		float fZ = m_f3LastPos[0]*f3CurPos[1] - m_f3LastPos[1]*f3CurPos[0];
		m_f3LastPos[0] = f3CurPos[0];
		m_f3LastPos[1] = f3CurPos[1];
		m_f3LastPos[2] = f3CurPos[2];
		DoRotation( fAngle, fX, fY, fZ );
		Invalidate( FALSE );
	}
		break;
	case TM_PAN:
		m_f3RenderingCenter[0] -= ( pt.x - m_ptLast.x ) / m_fRenderingRate;
		m_f3RenderingCenter[1] += ( pt.y - m_ptLast.y ) / m_fRenderingRate;
		m_ptLast = pt;
		// ���f�����O���r���[�C���O�ϊ��s��
		::glMatrixMode( GL_MODELVIEW );
		::glLoadIdentity();
		SetupViewingTransform();
		Invalidate( FALSE );
		break;
	case TM_ZOOM:
		// �g��E�k���ɂ��`��{������
		// +500�Ŕ{��2�{�ɁA+250�Ŕ{��1.5�{�ɁA
		// -500�Ŕ{��0.5�{�ɁA-1000�Ŕ{��0.25�{��
		m_fRenderingRate *= (float)pow( 2, ( m_ptLast.y - pt.y ) * 0.002 );
		m_ptLast = pt;
		//����p����̐ݒ�
		::glMatrixMode( GL_PROJECTION );
		::glLoadIdentity();
		SetupViewingFrustum();
		::glMatrixMode( GL_MODELVIEW );
		Invalidate( FALSE );
		break;
	}
}

void CNCViewGL::DoRotation( float fAngle, float fX, float fY, float fZ )
{
	// ��]��د�������̵݂�޼ު��̫����د���Ɋ|�����킹��
	::glPushMatrix();
		::glLoadIdentity();
		::glRotatef( fAngle, fX, fY, fZ );
		::glMultMatrixf( (GLfloat*) m_objectXform );
		::glGetFloatv( GL_MODELVIEW_MATRIX, (GLfloat*)m_objectXform );
	::glPopMatrix();

	// ����ݸ�&�ޭ��ݸޕϊ��s��
	::glMatrixMode( GL_MODELVIEW );
	::glLoadIdentity();
	SetupViewingTransform();
}

void CNCViewGL::SetupViewingTransform(void)
{
	::gluLookAt (	m_f3RenderingCenter[0],  m_f3RenderingCenter[1], m_f3RenderingCenter[2] + 1.0,	// ���_
					m_f3RenderingCenter[0],  m_f3RenderingCenter[1], m_f3RenderingCenter[2],		// �����_
					0.0,  1.0, 0.0 );	// ��x�N�g��

	// ���f�����W�n�̌��_�����_���W�n�̑O���N���b�v�ʂƌ���N���b�v�ʂ̒��Ԃɕ��s�ړ�����
	::glTranslatef( 0, 0, -50.0 );

	::glMultMatrixf( (GLfloat*)m_objectXform );	// �\����]�i�����f����]�j
}

void CNCViewGL::SetupViewingFrustum(void)
{
	::glOrtho(	- m_cx * 0.5 / m_fRenderingRate,	// left
				  m_cx * 0.5 / m_fRenderingRate,	// right
				- m_cy * 0.5 / m_fRenderingRate,	// buttom
				  m_cy * 0.5 / m_fRenderingRate,	// top
				  10,			// near
				  1000 );		// far
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewGL �`��

void CNCViewGL::OnDraw(CDC* pDC)
{
	ASSERT_VALID(GetDocument());

	if( !m_pDC )
		return;

	// ��ײ�ނ��޸ނ̂�����̨�����ނɂ����Ă�SDI�̍ŏ��̕`��ƁA
	// ���ׂĂ̸��̨�����ނɂ����Ă�MDI�`��ɂ�����
	// �ȉ��̖��߂́A���ʂ𔭊�����B
	::wglMakeCurrent( m_pDC->GetSafeHdc(), m_hRC );

	::glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	if ( m_uiDisplayListIndex > 0 ) {
		::glPushMatrix();
			// ���̕`��
			::glCallList( m_uiDisplayListIndex );
			// NC�ް��`��
			::glCallList( m_uiDisplayListIndex+1 );
		::glPopMatrix();
	}
	else {
		// �ʏ�`��Ӱ��(�ި���ڲؽĂ��g�p���Ȃ�)
		::glPushMatrix();
			RenderAxis();
		::glPopMatrix();
		::glPushMatrix();
			RenderCode();
		::glPopMatrix();
	}

	::glFinish();
	::SwapBuffers( m_pDC->GetSafeHdc() );
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewGL �f�f

#ifdef _DEBUG
void CNCViewGL::AssertValid() const
{
	CView::AssertValid();
}

void CNCViewGL::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CNCDoc* CNCViewGL::GetDocument() // ��f�o�b�O �o�[�W�����̓C�����C���ł��B
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CNCDoc)));
	return static_cast<CNCDoc *>(m_pDocument);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CNCViewGL ���b�Z�[�W �n���h��

int CNCViewGL::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if ( CView::OnCreate(lpCreateStruct) < 0 )
		return -1;

	// ���د�����̋��E���ق��ڂ�̂ŁCIDC_ARROW �𖾎��I�Ɏw��
	// ���� NCView.cpp �̂悤�� PreCreateWindow() �ł� AfxRegisterWndClass() �łʹװ
	::SetClassLongPtr(m_hWnd, GCLP_HCURSOR,
		(LONG_PTR)AfxGetApp()->LoadStandardCursor(IDC_ARROW));

	// OpenGL����������
	m_pDC = new CClientDC(this);
    if( !m_pDC ) {
		TRACE0("m_pDC is NULL\n");
		return -1;
	}

	// OpenGL pixel format �̐ݒ�
    if( !SetupPixelFormat() ) {
		TRACE0("SetupPixelFormat failed\n");
		return -1;
	}

	// �����ݸ޺�÷�Ă̍쐬
	if( !(m_hRC = ::wglCreateContext(m_pDC->GetSafeHdc())) ) {
		TRACE0("wglCreateContext failed\n");
		return -1;
	}

	// �����ݸ޺�÷�Ă���Ă����޲���÷�Ăɐݒ�
	if( !::wglMakeCurrent(m_pDC->GetSafeHdc(), m_hRC) ) {
		TRACE0("wglMakeCurrent failed\n");
		return -1;
	}

	// �ر�װ�̐ݒ�
	::glClearColor( 0, 0, 0, 0 );	// ��

	// ���߽�ޯ̧�̸ر
	::glClearDepth( 1.0f );

	// ���߽ý�
	::glEnable( GL_DEPTH_TEST );

	// ���̧߽ݸ�i�������A��O�ɂ�����̂ŏ�`���Ă����j
	::glDepthFunc( GL_LEQUAL );

	return 0;
}

void CNCViewGL::OnDestroy()
{
	CView::OnDestroy();
	if ( m_uiDisplayListIndex > 0 )
		::glDeleteLists(m_uiDisplayListIndex, 2);
	::wglDeleteContext( m_hRC );
	if ( m_pDC )
		delete	m_pDC;
}

void CNCViewGL::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);
	if( cx <= 0 || cy <= 0 )
		return;

	m_cx = cx;
	m_cy = cy;

	// �ޭ��߰Đݒ菈��
	::glViewport(0, 0, cx, cy);
}

LRESULT CNCViewGL::OnUserViewFitMsg(WPARAM, LPARAM)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CNCViewGL::OnUserViewFitMsg()\nStart");
#endif
	// �ި���ڲ�̱��߸Ĕ䂩�王�쒼���̐ݒ�
	CRect3D	rc(m_rcMax);
	CPointD	pt(rc.CenterPoint());
	double	dW = rc.Width(), dH = rc.Height(), dZ;
	if ( dW > dH ) {
		dH = dW * m_cy / m_cx / 2.0;
		rc.top    = pt.y - dH;
		rc.bottom = pt.y + dH;
		dZ = dW;
	}
	else {
		dW = dH * m_cx / m_cy / 2.0;
		rc.left   = pt.x - dW;
		rc.right  = pt.x + dW;
		dZ = dH;
	}
	rc.NormalizeRect();
	::glMatrixMode( GL_PROJECTION );
	::glLoadIdentity();
	::glOrtho(rc.left, rc.right, rc.top, rc.bottom, -dZ, dZ);
#ifdef _DEBUG
	dbg.printf("(%f,%f)-(%f,%f)", rc.left, rc.top, rc.right, rc.bottom);
#endif

	// ���f�����W�n�̌��_��
	// ���_���W�n�̑O���N���b�v�ʂƌ���N���b�v�ʂ̒��Ԃɕ��s�ړ�����
//	::glTranslated( pt.x, pt.y, -50.0 );
//	::glTranslated( 0.0, 0.0, 0.0 );
	// ���_���W�ϊ�
//	::glMatrixMode( GL_MODELVIEW );
//	::glLoadIdentity();
//	::gluLookAt(pt.x, pt.y,  dZ,	// ���_���W
//				pt.x, pt.y, -dZ,	// ���_���W���猩����W
//				0.0,  1.0,  0.0);	// Y������
//	::gluLookAt(0.0, 0.0,  dZ,	// ���_���W
//				0.0, 0.0, -dZ,	// ���_���W���猩����W
//				0.0,  1.0,  0.0);	// Y������
	return 0;
}

BOOL CNCViewGL::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}

void CNCViewGL::OnLButtonDown(UINT nFlags, CPoint point)
{
	BeginTracking( point, TM_SPIN );
}

void CNCViewGL::OnLButtonUp(UINT nFlags, CPoint point)
{
	EndTracking();
}

void CNCViewGL::OnRButtonDown(UINT nFlags, CPoint point)
{
	BeginTracking( point, TM_PAN );
}

void CNCViewGL::OnRButtonUp(UINT nFlags, CPoint point)
{
	EndTracking();
}

void CNCViewGL::OnMouseMove(UINT nFlags, CPoint point)
{
	DoTracking( point );
}

BOOL CNCViewGL::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	if ( !pOpt->IsMouseWheel() )
		return FALSE;

	if ( pOpt->GetWheelType() == 0 )
		zDelta = -zDelta;

	BeginTracking( pt, TM_ZOOM );
	CPoint	pt_delta( pt.x - zDelta / 3, pt.y - zDelta / 3 );
	DoTracking( pt_delta );
	EndTracking();

	return TRUE;
}
