// NCViewGL.cpp : �����t�@�C��
//

#include "stdafx.h"
#include "NCVC.h"
#include "NCChild.h"
#include "NCdata.h"
#include "NCDoc.h"
#include "NCViewGL.h"
#include "ViewOption.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

// �g�嗦�\���̂��߂̕ϊ��P��
// 1�����������߸�ِ� GetDeviceCaps(LOGPIXELS[X|Y]) = 96dpi
// 1��� == 25.4mm
static	const double	LOGPIXEL = 96 / 25.4;	

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
	// �߰�ސֲؑ����
	ON_MESSAGE (WM_USERACTIVATEPAGE, OnUserActivatePage)
	// �e�ޭ��ւ�̨��ү����
	ON_MESSAGE (WM_USERVIEWFITMSG, OnUserViewFitMsg)
	// �ƭ������
	ON_COMMAND_RANGE(ID_VIEW_UP, ID_VIEW_RT, OnMoveKey)
	ON_COMMAND_RANGE(ID_VIEW_FIT, ID_VIEW_LENSN, OnLensKey)
END_MESSAGE_MAP()

//	����޳ү���ނ���M���AOpenGL���߂𑀍삷��Ƃ���
//	::wglMakeCurrent( pDC->GetSafeHdc(), m_hRC );
//	��K������Ă���

/////////////////////////////////////////////////////////////////////////////
// CNCViewGL �N���X�̍\�z/����

CNCViewGL::CNCViewGL()
{
	m_cx = m_cy = 0;
	m_dRate = 0.0;
	m_hRC = NULL;
	m_uiAxis = m_uiBack = m_uiWork = m_uiCode = 0;

	m_enTrackingMode = TM_NONE;
	ClearObjectForm();
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
	// ���_���W�ϊ�
	::glMatrixMode( GL_MODELVIEW );
	::glLoadIdentity();

	// �e���ި���ڲؽĐ���
	m_uiAxis = ::glGenLists(1);
	if( m_uiAxis > 0 ) {
		::glNewList( m_uiAxis, GL_COMPILE );
			RenderAxis();
		::glEndList();
	}
	m_uiBack = ::glGenLists(1);
	if( m_uiBack > 0 ) {
		::glNewList( m_uiBack, GL_COMPILE );
			RenderBack();
		::glEndList();
	}

	// ��޼ު�Đ�����������؏���ʂ���������(�����m��Ȃ�)�̂�
	// 10,000��臒l�Ƃ���
	if ( GetDocument()->GetNCsize() > 10000 )
		return;

	m_uiCode = ::glGenLists(1);
	if( m_uiCode > 0 ) {
		// NC�ް��`����ި���ڲؽĐ���
		::glNewList( m_uiCode, GL_COMPILE );
			RenderCode();
		::glEndList();
	}
}

void CNCViewGL::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	switch ( lHint ) {
	case UAV_FILEINSERT:
		OnLensKey(ID_VIEW_FIT);
		return;
	case UAV_DRAWWORKRECT:
		if ( m_uiWork > 0 )
			::glDeleteLists(m_uiWork, 1);
		m_uiWork = ::glGenLists(1);
		if( m_uiWork > 0 ) {
			::glNewList( m_uiWork, GL_COMPILE );
				RenderWork();
			::glEndList();
		}
		Invalidate(FALSE);
		return;
	case UAV_CHANGEFONT:
		// �ި���ڲؽčĐ���
		if ( m_uiAxis > 0 )
			::glDeleteLists(m_uiAxis, 1);
		if ( m_uiBack > 0 )
			::glDeleteLists(m_uiBack, 1);
		if ( m_uiCode > 0 )
			::glDeleteLists(m_uiCode, 1);
		OnInitialUpdate();
		if ( GetDocument()->IsWorkRect() ) {
			if ( m_uiWork > 0 )
				::glDeleteLists(m_uiWork, 1);
			m_uiWork = ::glGenLists(1);
			if( m_uiWork > 0 ) {
				::glNewList( m_uiWork, GL_COMPILE );
					RenderWork();
				::glEndList();
			}
		}
		Invalidate(FALSE);
		return;
	case UAV_ADDINREDRAW:
		Invalidate(FALSE);
		return;
	}
	CView::OnUpdate(pSender, lHint, pHint);
}

BOOL CNCViewGL::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
	// �������� CDocument::OnCmdMsg() ���Ă΂Ȃ��悤�ɂ���
//	return CView::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
	return CWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewGL �N���X�̃����o�֐�

BOOL CNCViewGL::SetupPixelFormat(CDC* pDC)
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
	if( !(iPixelFormat = ::ChoosePixelFormat(pDC->GetSafeHdc(), &pfd)) ) {
		TRACE0( "ChoosePixelFormat is failed" );
		return FALSE;
	}

	if( !::SetPixelFormat(pDC->GetSafeHdc(), iPixelFormat, &pfd) ) {
		TRACE0( "SetPixelFormat is failed" );
		return FALSE;
	}

    return TRUE;
}

void CNCViewGL::ClearObjectForm(void)
{
	m_ptCenter = 0.0;
	m_objectXform[0][0]	= 1.0; m_objectXform[0][1] = 0.0; m_objectXform[0][2] = 0.0; m_objectXform[0][3] = 0.0;
	m_objectXform[1][0]	= 0.0; m_objectXform[1][1] = 1.0; m_objectXform[1][2] = 0.0; m_objectXform[1][3] = 0.0;
	m_objectXform[2][0]	= 0.0; m_objectXform[2][1] = 0.0; m_objectXform[2][2] = 1.0; m_objectXform[2][3] = 0.0;
	m_objectXform[3][0]	= 0.0; m_objectXform[3][1] = 0.0; m_objectXform[3][2] = 0.0; m_objectXform[3][3] = 1.0;
}

void CNCViewGL::RenderAxis(void)
{
	extern	const	PENSTYLE	g_penStyle[];	// ViewOption.cpp
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	double		dLength;
	COLORREF	col;

	::glPushAttrib( GL_CURRENT_BIT | GL_LINE_BIT );	// �F | �����
	::glLineWidth( 2.0 );
	::glEnable( GL_LINE_STIPPLE );

	// X���̶޲��
	dLength = pOpt->GetGuideLength(NCA_X);
	col = pOpt->GetNcDrawColor(NCCOL_GUIDEX);
	::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(NCA_X)].nGLpattern);
	::glBegin( GL_LINES );
	::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
	::glVertex3d(-dLength, 0.0, 0.0);
	::glVertex3d( dLength, 0.0, 0.0);
	::glEnd();
	// Y���̶޲��
	dLength = pOpt->GetGuideLength(NCA_Y);
	col = pOpt->GetNcDrawColor(NCCOL_GUIDEY);
	::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(NCA_Y)].nGLpattern);
	::glBegin( GL_LINES );
	::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
	::glVertex3d(0.0, -dLength, 0.0);
	::glVertex3d(0.0,  dLength, 0.0);
	::glEnd();
	// Z���̶޲��
	dLength = pOpt->GetGuideLength(NCA_Z);
	col = pOpt->GetNcDrawColor(NCCOL_GUIDEZ);
	::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(NCA_Z)].nGLpattern);
	::glBegin( GL_LINES );
	::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
	::glVertex3d(0.0, 0.0, -dLength);
	::glVertex3d(0.0, 0.0,  dLength);
	::glEnd();

	::glDisable( GL_LINE_STIPPLE );
	::glPopAttrib();
}

void CNCViewGL::RenderBack(void)
{
	// �w�i��غ�݂̕`��
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	COLORREF	col1 = pOpt->GetNcDrawColor(NCCOL_BACKGROUND1),
				col2 = pOpt->GetNcDrawColor(NCCOL_BACKGROUND2);
	::glPushMatrix();
	::glLoadIdentity();
	::glBegin(GL_QUADS);
	// ����
	::glColor3ub( GetRValue(col1), GetGValue(col1), GetBValue(col1) );
	::glVertex3d(m_rcView.left,  m_rcView.bottom, m_rcView.low);
	// ����
	::glColor3ub( GetRValue(col2), GetGValue(col2), GetBValue(col2) );
	::glVertex3d(m_rcView.left,  m_rcView.top,    m_rcView.low);
	// �E��
	::glColor3ub( GetRValue(col2), GetGValue(col2), GetBValue(col2) );
	::glVertex3d(m_rcView.right, m_rcView.top,    m_rcView.low);
	// �E��
	::glColor3ub( GetRValue(col1), GetGValue(col1), GetBValue(col1) );
	::glVertex3d(m_rcView.right, m_rcView.bottom, m_rcView.low);
	//
	::glEnd();
	::glPopMatrix();
}

void CNCViewGL::RenderWork(void)
{
	// ܰ���`�̕`��
	CRect3D		rc(GetDocument()->GetWorkRect());
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	COLORREF	col = pOpt->GetNcDrawColor(NCCOL_WORK);
	GLfloat		materialCol[4], lightPos[4];
	materialCol[0] = (GLfloat)GetRValue(col) / 255;
	materialCol[1] = (GLfloat)GetGValue(col) / 255;
	materialCol[2] = (GLfloat)GetBValue(col) / 255;
	materialCol[3] = 1.0f;

	::glPushAttrib( GL_LIGHTING_BIT );		// �Ɩ�����
	// �Ɩ��ʒu�ݒ�
//	lightPos[0] = (GLfloat)m_rcView.left;
//	lightPos[1] = (GLfloat)m_rcView.top;
//	lightPos[2] = (GLfloat)m_rcView.high;
	lightPos[0] = (GLfloat)(m_rcView.Width()/2  + m_rcView.left);
	lightPos[1] = (GLfloat)(m_rcView.Height()/2 + m_rcView.top);
	lightPos[2] = (GLfloat)(m_rcView.high + max(max(m_rcView.Width(), m_rcView.Height()), m_rcView.Depth()));
	lightPos[3] = 1.0f;
//	::glMatrixMode( GL_PROJECTION );
	::glPushMatrix();
	::glLoadIdentity();
	::glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
	::glPopMatrix();
//	::glMatrixMode( GL_MODELVIEW );
/*
	// ���ʏƌ����[�h�ɐݒ肷��
	::glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	// �����Ɋ֌W���Ȃ�������ݒ�
	GLfloat model_ambient[] = {0.4f,0.4f,0.4f,1.0f};
	::glLightModelfv(GL_LIGHT_MODEL_AMBIENT, model_ambient);
	// �����̊���(ambient),�g�U��(diffuse),���ʌ�(specular)��ݒ�
	GLfloat light0_ambient[]  = {0.5f,0.5f,0.5f,1.0f};
	GLfloat light0_diffuse[]  = {1.0f,1.0f,1.0f,1.0f};
	GLfloat light0_specular[] = {1.0f,1.0f,1.0f,1.0f};
	::glLightfv(GL_LIGHT0, GL_AMBIENT,  light0_ambient);
	::glLightfv(GL_LIGHT0, GL_DIFFUSE,  light0_diffuse);
	::glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);
*/
	// �����ݒ�
//	::glMaterialfv(GL_FRONT, GL_DIFFUSE, materialCol);
//	::glMaterialfv(GL_FRONT, GL_AMBIENT, materialCol);
	// �Ɩ��I��
//	::glEnable( GL_NORMALIZE );
	::glEnable( GL_LIGHTING );
	::glEnable( GL_LIGHT0 );
	::glEnable( GL_COLOR_MATERIAL );	// glColor3ub() �Ŏ���(�F)���w��
	::glBegin(GL_QUADS);
	::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
	// ���
	::glNormal3d(rc.left,	rc.bottom,	rc.high+1.0);
	::glVertex3d(rc.left,	rc.bottom,	rc.high);
	::glVertex3d(rc.right,	rc.bottom,	rc.high);
	::glVertex3d(rc.right,	rc.top,		rc.high);
	::glVertex3d(rc.left,	rc.top,		rc.high);
	// �O��
	::glNormal3d(rc.left,	rc.top+1.0,	rc.high);
	::glVertex3d(rc.left,	rc.top,		rc.high);
	::glVertex3d(rc.right,	rc.top,		rc.high);
	::glVertex3d(rc.right,	rc.top,		rc.low );
	::glVertex3d(rc.left,	rc.top,		rc.low );
	// ���
	::glNormal3d(rc.left,	rc.bottom,	rc.low-1.0 );
	::glVertex3d(rc.left,	rc.bottom,	rc.low );
	::glVertex3d(rc.left,	rc.top,		rc.low );
	::glVertex3d(rc.right,	rc.top,		rc.low );
	::glVertex3d(rc.right,	rc.bottom,	rc.low );
	// ����
	::glNormal3d(rc.left,	rc.bottom-1.0,rc.high);
	::glVertex3d(rc.left,	rc.bottom,	rc.high);
	::glVertex3d(rc.left,	rc.bottom,	rc.low );
	::glVertex3d(rc.right,	rc.bottom,	rc.low );
	::glVertex3d(rc.right,	rc.bottom,	rc.high);
	// ������
	::glNormal3d(rc.left-1.0,rc.bottom,	rc.high);
	::glVertex3d(rc.left,	rc.bottom,	rc.high);
	::glVertex3d(rc.left,	rc.top,		rc.high);
	::glVertex3d(rc.left,	rc.top,		rc.low );
	::glVertex3d(rc.left,	rc.bottom,	rc.low);
	// �E����
	::glNormal3d(rc.right+1.0,rc.bottom,rc.high);
	::glVertex3d(rc.right,	rc.bottom,	rc.high);
	::glVertex3d(rc.right,	rc.bottom,	rc.low );
	::glVertex3d(rc.right,	rc.top,		rc.low );
	::glVertex3d(rc.right,	rc.top,		rc.high);
	//
	::glEnd();
	::glDisable( GL_COLOR_MATERIAL );
	::glDisable( GL_LIGHT0 );
	::glDisable( GL_LIGHTING );
//	::glDisable( GL_NORMALIZE );
	::glPopAttrib();
}

void CNCViewGL::RenderCode(void)
{
	::glEnable( GL_LINE_STIPPLE );
	// NC�ް��`��
	for ( int i=0; i<GetDocument()->GetNCsize(); i++ )
		GetDocument()->GetNCdata(i)->DrawGL();
	::glDisable( GL_LINE_STIPPLE );
}

CPoint3D CNCViewGL::PtoR(const CPoint& pt)
{
	CPoint3D	ptResult;
	// ���ً�Ԃ̉�]
	ptResult.x = ( 2.0 * pt.x - m_cx ) / m_cx * 0.5;
	ptResult.y = ( m_cy - 2.0 * pt.y ) / m_cy * 0.5;
	double	d  = _hypot( ptResult.x, ptResult.y );
	ptResult.z = cos( (PI/2.0) * ( ( d < 1.0 ) ? d : 1.0 ) );

	ptResult *= 1.0 / ptResult.hypot();

	return ptResult;
}

void CNCViewGL::BeginTracking(const CPoint& pt, ENTRACKINGMODE enTrackingMode)
{
	::ShowCursor(FALSE);
	SetCapture();
	m_enTrackingMode = enTrackingMode;
	switch( m_enTrackingMode ) {
	case TM_NONE:
		break;
	case TM_SPIN:
		m_ptLastRound = PtoR(pt);
		break;
	case TM_PAN:
		m_ptLastMove = pt;
		break;
	}
}

void CNCViewGL::EndTracking(void)
{
	ReleaseCapture();
	::ShowCursor(TRUE);
	m_enTrackingMode = TM_NONE;
}

void CNCViewGL::DoTracking( const CPoint& pt )
{
	CClientDC	dc(this);
	::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );

	switch( m_enTrackingMode ) {
	case TM_NONE:
		break;
	case TM_SPIN:
	{
		CPoint3D	ptRound( PtoR(pt) );
		CPoint3D	ptw( ptRound - m_ptLastRound );
		CPoint3D	ptr(
			m_ptLastRound.y*ptRound.z - m_ptLastRound.z*ptRound.y,
			m_ptLastRound.z*ptRound.x - m_ptLastRound.x*ptRound.z,
			m_ptLastRound.x*ptRound.y - m_ptLastRound.y*ptRound.x );
		DoRotation( 180.0 * ptw.hypot(), ptr );
		Invalidate( FALSE );
		m_ptLastRound = ptRound;
	}
		break;
	case TM_PAN:
		m_ptCenter.x += ( pt.x - m_ptLastMove.x ) / m_dRate;
		m_ptCenter.y -= ( pt.y - m_ptLastMove.y ) / m_dRate;
		m_ptLastMove = pt;
		// ����ݸ�&�ޭ��ݸޕϊ��s��
		SetupViewingTransform();
		Invalidate( FALSE );
		break;
	}

	::wglMakeCurrent( NULL, NULL );
}

void CNCViewGL::DoScale(int nRate)
{
	m_rcView.InflateRect(
		_copysign(m_rcView.Width(),  nRate) * 0.05,
		_copysign(m_rcView.Height(), nRate) * 0.05 );
	double	dW = m_rcView.Width(), dH = m_rcView.Height();
	if ( dW > dH )
		m_dRate = m_cx / dW;
	else
		m_dRate = m_cy / dH;

	CClientDC	dc(this);
	::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );

	::glMatrixMode( GL_PROJECTION );
	::glLoadIdentity();
	::glOrtho(m_rcView.left, m_rcView.right, m_rcView.top, m_rcView.bottom,
		m_rcView.low, m_rcView.high);
	::glMatrixMode( GL_MODELVIEW );
	Invalidate(FALSE);

	::wglMakeCurrent( NULL, NULL );

	// MDI�q�ڰт̽ð���ް�ɏ��\��
	static_cast<CNCChild *>(GetParentFrame())->SetFactorInfo(NC_XYZ_PLANE, m_dRate/LOGPIXEL);
}

void CNCViewGL::DoRotation(double dAngle, const CPoint3D& pt)
{
	// ��]��د�������̵݂�޼ު��̫����د���Ɋ|�����킹��
	::glLoadIdentity();
	::glRotated( dAngle, pt.x, pt.y, pt.z );
	::glMultMatrixd( (GLdouble *)m_objectXform );
	::glGetDoublev( GL_MODELVIEW_MATRIX, (GLdouble *)m_objectXform );

	// ����ݸ�&�ޭ��ݸޕϊ��s��
	SetupViewingTransform();
}

void CNCViewGL::SetupViewingTransform(void)
{
//	::glMatrixMode( GL_MODELVIEW );
	::glLoadIdentity();
	::glTranslated( m_ptCenter.x, m_ptCenter.y, 0.0 );
	::glMultMatrixd( (GLdouble *)m_objectXform );	// �\����]�i�����f����]�j
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewGL �`��

void CNCViewGL::OnDraw(CDC* pDC)
{
	ASSERT_VALID(GetDocument());

	// ���ĺ�÷�Ă̊��蓖��
	::wglMakeCurrent( pDC->GetSafeHdc(), m_hRC );

	::glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
//	::glMatrixMode( GL_MODELVIEW );

	// �w�i�̕`��
	if ( m_uiBack > 0 )
		::glCallList( m_uiBack );
	else
		RenderBack();	// �ʏ�`��Ӱ��(�ި���ڲؽĂ��g�p���Ȃ�)
	// ���̕`��
	if ( m_uiAxis > 0 )
		::glCallList( m_uiAxis );
	else
		RenderAxis();
	// ܰ���`
	if ( GetDocument()->IsWorkRect() ) {
		if ( m_uiWork > 0 )
			::glCallList( m_uiWork );
		else
			RenderWork();
	}
	// NC�ް��`��
	if ( m_uiCode > 0 )
		::glCallList( m_uiCode );
	else
		RenderCode();

	::glFinish();
	::SwapBuffers( pDC->GetSafeHdc() );

	::wglMakeCurrent(NULL, NULL);
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
	// ���� NCView.cpp �̂悤�� PreCreateWindow() �ł� AfxRegisterWndClass() �łʹװ??
	::SetClassLongPtr(m_hWnd, GCLP_HCURSOR,
		(LONG_PTR)AfxGetApp()->LoadStandardCursor(IDC_ARROW));

	// OpenGL����������
	CClientDC	dc(this);

	// OpenGL pixel format �̐ݒ�
    if( !SetupPixelFormat(&dc) ) {
		TRACE0("SetupPixelFormat failed\n");
		return -1;
	}

	// �����ݸ޺�÷�Ă̍쐬
	if( !(m_hRC = ::wglCreateContext(dc.GetSafeHdc())) ) {
		TRACE0("wglCreateContext failed\n");
		return -1;
	}

	// �����ݸ޺�÷�Ă���Ă����޲���÷�Ăɐݒ�
	if( !::wglMakeCurrent(dc.GetSafeHdc(), m_hRC) ) {
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

	// ���ʂ͕`���Ȃ��ݒ�
	::glFrontFace(GL_CCW);
	::glCullFace(GL_BACK);
	::glEnable(GL_CULL_FACE);
/*
#ifdef _DEBUG
	g_dbg.printf("GetDeviceCaps([HORZSIZE|VERTSIZE])=%d, %d",
		dc.GetDeviceCaps(HORZSIZE), dc.GetDeviceCaps(VERTSIZE) );
	g_dbg.printf("GetDeviceCaps([LOGPIXELSX|LOGPIXELSY])=%d, %d",
		dc.GetDeviceCaps(LOGPIXELSX), dc.GetDeviceCaps(LOGPIXELSY) );
#endif
*/
	return 0;
}

void CNCViewGL::OnDestroy()
{
	CClientDC	dc(this);
	::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );
	if ( m_uiAxis > 0 )
		::glDeleteLists(m_uiAxis, 1);
	if ( m_uiBack > 0 )
		::glDeleteLists(m_uiBack, 1);
	if ( m_uiCode > 0 )
		::glDeleteLists(m_uiCode, 1);
	::wglDeleteContext( m_hRC );
	::wglMakeCurrent(NULL, NULL);
	
	CView::OnDestroy();
}

void CNCViewGL::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);
	if( cx <= 0 || cy <= 0 )
		return;

	m_cx = cx;
	m_cy = cy;

	// �ޭ��߰Đݒ菈��
	CClientDC	dc(this);
	::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );
	::glViewport(0, 0, cx, cy);
	::wglMakeCurrent(NULL, NULL);
}

void CNCViewGL::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) 
{
	if ( bActivate ) {
#ifdef _DEBUG
		g_dbg.printf("OnActivateView() %s", GetDocument()->GetTitle());
#endif
		// MDI�q�ڰт̽ð���ް�ɏ��\��
		static_cast<CNCChild *>(GetParentFrame())->SetFactorInfo(NC_XYZ_PLANE, m_dRate/LOGPIXEL);
	}
	CView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}

LRESULT CNCViewGL::OnUserActivatePage(WPARAM, LPARAM lParam)
{
	if ( m_dRate == 0 )
		OnLensKey(ID_VIEW_FIT);
	// �����ޭ��Ƃ͈���āA�g�嗦���X�V����K�v�͖���
	// �ð���ް�ւ̕\���� OnActivateView() �ōs��
	return 0;
}

LRESULT CNCViewGL::OnUserViewFitMsg(WPARAM, LPARAM)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CNCViewGL::OnUserViewFitMsg()\nStart");
#endif
	// ��޼ު�ċ�`��10%(�㉺���E5%����)�傫��
	m_rcView = GetDocument()->GetMaxRect();
	m_rcView.InflateRect(m_rcView.Width()*0.05, m_rcView.Height()*0.05);
	m_rcView.NormalizeRect();

	// �ި���ڲ�̱��߸Ĕ䂩�王�쒼���̐ݒ�
	CPointD	pt(m_rcView.CenterPoint());
	double	dW = m_rcView.Width(), dH = m_rcView.Height(), dZ;
	if ( dW > dH ) {
		dH = dW * m_cy / m_cx / 2.0;
		dZ = max(dW, m_rcView.Depth());
		m_rcView.top    = pt.y - dH;
		m_rcView.bottom = pt.y + dH;
		m_dRate = m_cx / dW;
	}
	else {
		dW = dH * m_cx / m_cy / 2.0;
		dZ = max(dH, m_rcView.Depth());
		m_rcView.left   = pt.x - dW;
		m_rcView.right  = pt.x + dW;
		m_dRate = m_cy / dH;
	}
	m_rcView.high =  dZ;	// ��
	m_rcView.low  = -dZ;	// ��O
	m_rcView.NormalizeRect();

	CClientDC	dc(this);
	::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );
	::glMatrixMode( GL_PROJECTION );
	::glLoadIdentity();
	::glOrtho(m_rcView.left, m_rcView.right, m_rcView.top, m_rcView.bottom,
		m_rcView.low, m_rcView.high);
	::glMatrixMode( GL_MODELVIEW );
	::wglMakeCurrent(NULL, NULL);

#ifdef _DEBUG
	dbg.printf("(%f,%f)-(%f,%f)", m_rcView.left, m_rcView.top, m_rcView.right, m_rcView.bottom);
	dbg.printf("(%f,%f)", m_rcView.low, m_rcView.high);
	dbg.printf("Rate=%f", m_dRate);
#endif

	return 0;
}

void CNCViewGL::OnMoveKey(UINT nID)
{
	// �ײ��č��W��1/6���ړ��͈͂Ƃ���
	CSize	sz(m_cx/6, m_cy/6);
	CPoint	pt(0, 0);

	switch (nID) {
	case ID_VIEW_UP:
		pt.y += sz.cy;
		break;
	case ID_VIEW_DW:
		pt.y -= sz.cy;
		break;
	case ID_VIEW_LT:
		pt.x += sz.cx;
		break;
	case ID_VIEW_RT:
		pt.x -= sz.cx;
		break;
	}

	m_enTrackingMode = TM_PAN;
	m_ptLastMove = 0;
	DoTracking(pt);
	m_enTrackingMode = TM_NONE;
}

void CNCViewGL::OnLensKey(UINT nID)
{
	switch ( nID ) {
	case ID_VIEW_FIT:
		ClearObjectForm();
		OnUserViewFitMsg(0, 0);
		{
			CClientDC	dc(this);
			::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );
			SetupViewingTransform();
			::wglMakeCurrent( NULL, NULL );
		}
		Invalidate(FALSE);
		// MDI�q�ڰт̽ð���ް�ɏ��\��
		static_cast<CNCChild *>(GetParentFrame())->SetFactorInfo(NC_XYZ_PLANE, m_dRate/LOGPIXEL);
		break;
	case ID_VIEW_LENSP:
		DoScale(-1);
		break;
	case ID_VIEW_LENSN:
		DoScale(1);
		break;
	}
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

	// �����xϳ��ł͔��גl�̏ꍇ������̂ŁA�ꉞ臒l�̔��������
	if ( zDelta <= -WHEEL_DELTA ) {
		if ( pOpt->GetWheelType() != 0 )
			zDelta = -zDelta;
		DoScale(zDelta);
	}
	else if ( zDelta >= WHEEL_DELTA ) {
		if ( pOpt->GetWheelType() != 0 )
			zDelta = -zDelta;
		DoScale(zDelta);
	}

	return TRUE;
}
