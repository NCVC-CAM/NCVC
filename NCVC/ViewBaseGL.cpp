// ViewBaseGL.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "ViewBaseGL.h"
#include "ViewOption.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#include <mmsystem.h>			// timeGetTime()
#endif

IMPLEMENT_DYNAMIC(CViewBaseGL, CView)

BEGIN_MESSAGE_MAP(CViewBaseGL, CView)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_TIMER()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_COMMAND_RANGE(ID_VIEW_UP,  ID_VIEW_RT,    &CViewBaseGL::OnMoveKey)
	ON_COMMAND_RANGE(ID_VIEW_RUP, ID_VIEW_RRT,   &CViewBaseGL::OnRoundKey)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, &CViewBaseGL::OnUpdateEditCopy)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CViewBaseGL

CViewBaseGL::CViewBaseGL()
{
	m_hRC = NULL;
	m_pFBO = NULL;
	m_cx = m_cy = 0;
	m_enTrackingMode = TM_NONE;
	m_dRate = m_dRoundAngle = m_dRoundStep = 0.0f;
	IdentityMatrix();	// 単位行列に初期化
}

CViewBaseGL::~CViewBaseGL()
{
	if ( m_pFBO )
		delete	m_pFBO;
}

BOOL CViewBaseGL::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style |= WS_CLIPSIBLINGS|WS_CLIPCHILDREN;
	return __super::PreCreateWindow(cs);
}

BOOL CViewBaseGL::CreateFBO(void)
{
//	if ( AfxGetNCVCApp()->GetViewOption()->GetNCViewFlg(GLOPTFLG_USEFBO) ) {
		if ( !m_pFBO && GLEW_EXT_framebuffer_object ) {
			// ｳｨﾝﾄﾞｳｻｲｽﾞでFBO作成
			m_pFBO = new CFrameBuffer(m_cx, m_cy);
			if ( !m_pFBO->IsBind() ) {
				// FBO使用中止
				delete	m_pFBO;
				m_pFBO = NULL;
				return FALSE;
			}
		}
//	}
//	else if ( m_pFBO ) {
//		delete	m_pFBO;
//		m_pFBO = NULL;
//		return FALSE;
//	}
	return TRUE;
}

void CViewBaseGL::IdentityMatrix(void)
{
	m_ptCenter = 0.0f;
	m_objXform[0][0] = 1.0; m_objXform[0][1] = 0.0; m_objXform[0][2] = 0.0; m_objXform[0][3] = 0.0;
	m_objXform[1][0] = 0.0; m_objXform[1][1] = 1.0; m_objXform[1][2] = 0.0; m_objXform[1][3] = 0.0;
	m_objXform[2][0] = 0.0; m_objXform[2][1] = 0.0; m_objXform[2][2] = 1.0; m_objXform[2][3] = 0.0;
	m_objXform[3][0] = 0.0; m_objXform[3][1] = 0.0; m_objXform[3][2] = 0.0; m_objXform[3][3] = 1.0;
}

void CViewBaseGL::SetOrthoView(void)
{
	extern	const	float	g_dDefaultGuideLength;	// 50.0 (ViewOption.cpp)

	float	dW = fabs(m_rcView.Width()),
			dH = fabs(m_rcView.Height()),
			dZ = fabs(m_rcView.Depth()),
			d  = g_dDefaultGuideLength / 2.0f;

	// ｵﾌﾞｼﾞｪｸﾄ矩形を10%(上下左右5%ずつ)大きく
	m_rcView.InflateRect(dW*0.05f, dH*0.05f, dZ*0.05f);

	// 占有矩形の補正(不正表示の防止)
	if ( dW < g_dDefaultGuideLength ) {
		m_rcView.left	= -d;
		m_rcView.right	=  d;
		dW = g_dDefaultGuideLength;
	}
	if ( dH < g_dDefaultGuideLength ) {
		m_rcView.top	= -d;
		m_rcView.bottom	=  d;
		dH = g_dDefaultGuideLength;
	}
	if ( dZ < g_dDefaultGuideLength ) {
		m_rcView.low	= -d;
		m_rcView.high	=  d;
		dZ = g_dDefaultGuideLength;
	}

	// ﾃﾞｨｽﾌﾟﾚｲのｱｽﾍﾟｸﾄ比から視野直方体設定
	CPointF	pt(m_rcView.CenterPoint());
	if ( dW > dH ) {
		d = dW * m_cy / m_cx / 2.0f;
		m_rcView.top    = pt.y - d;
		m_rcView.bottom = pt.y + d;
		m_dRate = m_cx / dW;
	}
	else {
		d = dH * m_cx / m_cy / 2.0f;
		m_rcView.left   = pt.x - d;
		m_rcView.right  = pt.x + d;
		m_dRate = m_cy / dH;
	}
	d = max(max(dW, dH), dZ) * 2.0f;
	m_rcView.low  = -d;		// 手前
	m_rcView.high =  d;		// 奥
}

BOOL CViewBaseGL::SetupPixelFormat(CDC* pDC)
{
	static PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		32,
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0,
		24,		// Depth   Buffer
		8,		// Stencil Buffer
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

void CViewBaseGL::SetupViewingTransform(void)
{
	::glLoadIdentity();
	::glTranslated( m_ptCenter.x, m_ptCenter.y, 0.0 );
	::glMultMatrixd( (GLdouble *)m_objXform );	// 表示回転（＝モデル回転）
}

void CViewBaseGL::BeginTracking(const CPoint& pt, TRACKINGMODE enTrackingMode)
{
	::ShowCursor(FALSE);
	SetCapture();
	m_enTrackingMode = enTrackingMode;
	switch( m_enTrackingMode ) {
	case TM_SPIN:
		m_ptLastRound = PtoR(pt);
		break;
	case TM_PAN:
		m_ptLastMove = pt;
		break;
//	default:	// TM_NONE
//		break;
	}
}

void CViewBaseGL::EndTracking(void)
{
	ReleaseCapture();
	::ShowCursor(TRUE);
	m_enTrackingMode = TM_NONE;
	Invalidate(FALSE);
}

CPoint3F CViewBaseGL::PtoR(const CPoint& pt)
{
	CPoint3F	ptResult;
	// ﾓﾃﾞﾙ空間の回転
	ptResult.x = ( 2.0f * pt.x - m_cx ) / m_cx * 0.5f;
	ptResult.y = ( m_cy - 2.0f * pt.y ) / m_cy * 0.5f;
	float	 d = _hypotf( ptResult.x, ptResult.y );
	ptResult.z = cos( (PI/2.0f) * min(d, 1.0f) );

	ptResult *= 1.0f / ptResult.hypot();

	return ptResult;
}

void CViewBaseGL::DoTracking( const CPoint& pt )
{
	CClientDC	dc(this);
	::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );

	switch( m_enTrackingMode ) {
	case TM_SPIN:
	{
		CPoint3F	ptRound( PtoR(pt) );
		CPoint3F	ptw( ptRound - m_ptLastRound );
		m_dRoundStep = 180.0f * ptw.hypot();
		m_ptRoundBase.SetPoint(
			m_ptLastRound.y*ptRound.z - m_ptLastRound.z*ptRound.y,
			m_ptLastRound.z*ptRound.x - m_ptLastRound.x*ptRound.z,
			m_ptLastRound.x*ptRound.y - m_ptLastRound.y*ptRound.x );
		DoRotation(m_dRoundStep);
		Invalidate(FALSE);
		m_ptLastRound = ptRound;
	}
		break;
	case TM_PAN:
		m_ptCenter.x += ( pt.x - m_ptLastMove.x ) / m_dRate;
		m_ptCenter.y -= ( pt.y - m_ptLastMove.y ) / m_dRate;
		m_ptLastMove = pt;
		// ﾓﾃﾞﾘﾝｸﾞ&ﾋﾞｭｰｲﾝｸﾞ変換行列
		SetupViewingTransform();
		Invalidate(FALSE);
		break;
//	deault:		// TM_NONE
//		break;
	}

	::wglMakeCurrent( NULL, NULL );
}

void CViewBaseGL::DoScale(int nRate)
{
	if ( nRate != 0 ) {
		m_rcView.InflateRect(
			copysign(m_rcView.Width(),  (float)nRate) * 0.05f,
			copysign(m_rcView.Height(), (float)nRate) * 0.05f );
		float	dW = m_rcView.Width(), dH = m_rcView.Height();
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
#ifdef _DEBUGOLD
		printf("DoScale() ---\n");
		printf("  (%f,%f)-(%f,%f)\n", m_rcView.left, m_rcView.top, m_rcView.right, m_rcView.bottom);
		printf("  (%f,%f)\n", m_rcView.low, m_rcView.high);
#endif
	}
}

void CViewBaseGL::DoRotation(float dAngle)
{
#ifdef _DEBUG
//	printf("DoRotation() Angle=%f (%f, %f, %f)\n", dAngle,
//		m_ptRoundBase.x, m_ptRoundBase.y, m_ptRoundBase.z);
#endif
	// 回転ﾏﾄﾘｯｸｽを現在のｵﾌﾞｼﾞｪｸﾄﾌｫｰﾑﾏﾄﾘｯｸｽに掛け合わせる
	::glLoadIdentity();
	::glRotated( dAngle, m_ptRoundBase.x, m_ptRoundBase.y, m_ptRoundBase.z );
	::glMultMatrixd( (GLdouble *)m_objXform );
	::glGetDoublev( GL_MODELVIEW_MATRIX, (GLdouble *)m_objXform );

	SetupViewingTransform();
}

void CViewBaseGL::RenderBackground(const COLORREF col1, const COLORREF col2)
{
	// 色情報
	const GLubyte	col1v[] = {
		GetRValue(col1), GetGValue(col1), GetBValue(col1)
	},
					col2v[] = {
		GetRValue(col2), GetGValue(col2), GetBValue(col2)
	};
	const GLubyte	col[] = {
		col1v[0], col1v[1], col1v[2],		// 左下
		col2v[0], col2v[1], col2v[2],		// 左上
		col2v[0], col2v[1], col2v[2],		// 右上
		col1v[0], col1v[1], col1v[2]		// 右下
	};
	// 座標値設定(2D)
	const GLfloat	vertex[] = {
		m_rcView.left,	m_rcView.bottom,	// 左下
		m_rcView.left,	m_rcView.top,		// 左上
		m_rcView.right,	m_rcView.top,		// 右上
		m_rcView.right,	m_rcView.bottom		// 右下
	};

	// 配列有効
	::glColorPointer(3, GL_UNSIGNED_BYTE, 0, col);
	::glVertexPointer(2, GL_FLOAT, 0, vertex);

	::glPushMatrix();
	::glLoadIdentity();
	::glDrawArrays(GL_QUADS, 0, 4);		// 背面描画
	::glPopMatrix();
}

void CViewBaseGL::RenderAxis(void)
{
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();

	const COLORREF	colX = pOpt->GetNcDrawColor(NCCOL_GUIDEX),
					colY = pOpt->GetNcDrawColor(NCCOL_GUIDEY),
					colZ = pOpt->GetNcDrawColor(NCCOL_GUIDEZ);
	const GLubyte	col[] = {
		GetRValue(colX), GetGValue(colX), GetBValue(colX),		// Xs
		GetRValue(colX), GetGValue(colX), GetBValue(colX),		// Xe
		GetRValue(colY), GetGValue(colY), GetBValue(colY),		// Ys
		GetRValue(colY), GetGValue(colY), GetBValue(colY),		// Ye
		GetRValue(colZ), GetGValue(colZ), GetBValue(colZ),		// Zs
		GetRValue(colZ), GetGValue(colZ), GetBValue(colZ)		// Ze
	};
	const float	dx = pOpt->GetGuideLength(NCA_X),
				dy = pOpt->GetGuideLength(NCA_Y),
				dz = pOpt->GetGuideLength(NCA_Z);
	const GLfloat	vertex[] = {
		-dx, 0, 0, dx, 0, 0,
		0, -dy, 0, 0, dy, 0,
		0, 0, -dz, 0, 0, dz
	};

	::glColorPointer(3, GL_UNSIGNED_BYTE, 0, col);
	::glVertexPointer(3, GL_FLOAT, 0, vertex);

	::glDrawArrays(GL_LINES, 0, 3*2);	// 軸描画
}

/////////////////////////////////////////////////////////////////////////////
// CViewBaseGL メッセージ ハンドラ

int CViewBaseGL::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
#ifdef _DEBUG
	printf("CViewBaseGL::OnCreate() Start\n");
#endif
	if ( __super::OnCreate(lpCreateStruct) < 0 )
		return -1;

	// OpenGL初期化処理
	CClientDC	dc(this);
	HDC	hDC = dc.GetSafeHdc();

	// OpenGL pixel format の設定
    if( !SetupPixelFormat(&dc) ) {
		TRACE0("SetupPixelFormat failed\n");
		return -1;
	}
#ifdef _DEBUG
	printf("CViewBaseGL::SetupPixelFormat() End\n");
#endif

	// ﾚﾝﾀﾞﾘﾝｸﾞｺﾝﾃｷｽﾄの作成
	if( !(m_hRC = ::wglCreateContext(hDC)) ) {
		TRACE0("wglCreateContext failed\n");
		return -1;
	}
#ifdef _DEBUG
	printf("CViewBaseGL::wglCreateContext() End\n");
#endif

	// ﾚﾝﾀﾞﾘﾝｸﾞｺﾝﾃｷｽﾄをｶﾚﾝﾄのﾃﾞﾊﾞｲｽｺﾝﾃｷｽﾄに設定
	if( !::wglMakeCurrent(hDC, m_hRC) ) {
		TRACE0("wglMakeCurrent failed\n");
		return -1;
	}
#ifdef _DEBUG
	printf("CViewBaseGL::wglMakeCurrent() End\n");
	DWORD	t1 = ::timeGetTime();
#endif

	// OpenGL Extention 使用準備
	GLenum glewResult = ::glewInit();
	if ( glewResult != GLEW_OK ) {
		TRACE1("glewInit() failed code=%s\n", ::glewGetErrorString(glewResult));
		return -1;
	}
#ifdef _DEBUG
	DWORD	t2 = ::timeGetTime();
	printf("CViewBaseGL::glewInit() End %d[ms]\n", t2 - t1);
#endif

	::glEnable(GL_DEPTH_TEST);
	::glEnable(GL_NORMALIZE);	// ここにこれがないとInitialBoxel()を通らないﾜｲﾔ加工機ﾓｰﾄﾞでﾗｲﾃｨﾝｸﾞ色が出ない
	::glClearColor( 0, 0, 0, 0 );

#ifdef _DEBUG
	CString		strVer( ::glGetString(GL_VERSION) );
	int			nVer = atoi(strVer);
	printf("GetDeviceCaps([HORZSIZE|VERTSIZE])=%d, %d\n",
		dc.GetDeviceCaps(HORZSIZE), dc.GetDeviceCaps(VERTSIZE) );
	printf("GetDeviceCaps([LOGPIXELSX|LOGPIXELSY])=%d, %d\n",
		dc.GetDeviceCaps(LOGPIXELSX), dc.GetDeviceCaps(LOGPIXELSY) );
	GLint	nGLResult;
	printf("Using OpenGL version:%s\n", ::glGetString(GL_VERSION));
	printf("Using GLEW   version:%s\n", ::glewGetString(GLEW_VERSION));
	::glGetIntegerv(GL_STENCIL_BITS, &nGLResult);
	printf(" Stencil bits=%d\n", nGLResult);
	::glGetIntegerv(GL_DEPTH_BITS, &nGLResult);
	printf(" Depth   bits=%d\n", nGLResult);
	::glGetIntegerv(GL_MAX_ELEMENTS_VERTICES, &nGLResult);
	printf(" MaxElementVertices=%d\n", nGLResult);
	::glGetIntegerv(GL_MAX_ELEMENTS_INDICES, &nGLResult);
	printf(" MaxElementIndices =%d\n", nGLResult);
	if ( nVer >= 4 ) {
		::glGetIntegerv(GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS, &nGLResult);
		printf(" GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS =%d\n", nGLResult);
		::glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES, &nGLResult);
		printf(" GL_MAX_GEOMETRY_OUTPUT_VERTICES =%d\n", nGLResult);
		::glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE_EXT, &nGLResult);
		printf(" GL_MAX_RENDERBUFFER_SIZE_EXT =%d\n", nGLResult);
	}
	GetGLError();	// error flash
#endif

	::wglMakeCurrent( NULL, NULL );

	return 0;
}

void CViewBaseGL::OnDestroy()
{
	if ( m_dRoundStep != 0.0f )
		KillTimer(IDC_OPENGL_DRAGROUND);
}

void CViewBaseGL::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);

	if ( cx > 0 && cy > 0 ) {
		m_cx = cx;
		m_cy = cy;
		// ﾋﾞｭｰﾎﾟｰﾄ設定処理
		CClientDC	dc(this);
		::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );
		::glViewport(0, 0, cx, cy);
		::wglMakeCurrent(NULL, NULL);
	}
}

BOOL CViewBaseGL::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}

void CViewBaseGL::OnTimer(UINT_PTR nIDEvent) 
{
	if ( m_dRoundStep != 0.0f ) {
#ifdef _DEBUG
		printf("CViewBaseGL::OnTimer()\n");
#endif
//		m_dRoundAngle += m_dRoundStep / 10.0;
		m_dRoundAngle += copysign(0.05f, m_dRoundStep);
		if ( fabs(m_dRoundAngle) > 360.0f )
			m_dRoundAngle -= copysign(360.0f, m_dRoundStep);

		CClientDC	dc(this);
		::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );
		DoRotation(m_dRoundAngle);
		Invalidate(FALSE);
		::wglMakeCurrent( NULL, NULL );
	}

	__super::OnTimer(nIDEvent);
}

void CViewBaseGL::OnLButtonDown(UINT nFlags, CPoint point)
{
	if ( m_dRoundStep != 0.0f ) {
		KillTimer(IDC_OPENGL_DRAGROUND);
		m_dRoundStep = 0.0f;	// KillTimer()でもﾒｯｾｰｼﾞｷｭｰは消えない
	}
	BeginTracking( point, TM_SPIN );
}

void CViewBaseGL::OnLButtonUp(UINT nFlags, CPoint point)
{
	EndTracking();
	if ( m_dRoundStep != 0.0f ) {
		KillTimer(IDC_OPENGL_DRAGROUND);	// KillTimer() 連発してもエエのかな...
		m_dRoundStep = 0.0f;
	}
}

void CViewBaseGL::OnRButtonDown(UINT nFlags, CPoint point)
{
	if ( m_dRoundStep != 0.0f )
		KillTimer(IDC_OPENGL_DRAGROUND);	// 連続回転一時停止

	m_ptDownClick = point;
	BeginTracking( point, TM_PAN );
}

void CViewBaseGL::OnRButtonUp(UINT nFlags, CPoint point)
{
	EndTracking();
	if ( m_dRoundStep != 0.0f )
		SetTimer(IDC_OPENGL_DRAGROUND, 150, NULL);	// 連続回転再開

	if ( m_ptDownClick == point )
		__super::OnRButtonUp(nFlags, point);	// ｺﾝﾃｷｽﾄﾒﾆｭｰの表示
}

void CViewBaseGL::OnMButtonDown(UINT nFlags, CPoint point)
{
	if ( m_dRoundStep != 0.0f ) {
		KillTimer(IDC_OPENGL_DRAGROUND);
		m_dRoundStep = 0.0f;
	}
	BeginTracking( point, TM_SPIN );
}

void CViewBaseGL::OnMButtonUp(UINT nFlags, CPoint point)
{
	EndTracking();
	// ﾀｲﾏｲﾍﾞﾝﾄで連続回転
#ifdef _DEBUG
	printf("OnMButtonUp() Angle=%f (%f, %f, %f)\n", m_dRoundStep,
		m_ptRoundBase.x, m_ptRoundBase.y, m_ptRoundBase.z);
#endif
	if ( m_dRoundStep != 0.0f ) {
		m_dRoundAngle = m_dRoundStep;
		SetTimer(IDC_OPENGL_DRAGROUND, 150, NULL);
	}
}

void CViewBaseGL::OnMouseMove(UINT nFlags, CPoint point)
{
	if ( m_enTrackingMode != TM_NONE )
		DoTracking( point );
}

BOOL CViewBaseGL::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	if ( !pOpt->IsMouseWheel() )
		return FALSE;

	// 高精度ﾏｳｽでは微細値の場合があるので、一応閾値の判定を入れる
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

void CViewBaseGL::OnMoveKey(UINT nID)
{
	// ｸﾗｲｱﾝﾄ座標の1/6を移動範囲とする
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

void CViewBaseGL::OnRoundKey(UINT nID)
{
	if ( m_dRoundStep == 0.0f )
		SetTimer(IDC_OPENGL_DRAGROUND, 150, NULL);

	switch (nID) {
	case ID_VIEW_RUP:
		m_ptRoundBase.SetPoint(1.0f, 0.0f, 0.0f);
		m_dRoundAngle = m_dRoundStep = -1.0f;
		break;
	case ID_VIEW_RDW:
		m_ptRoundBase.SetPoint(1.0f, 0.0f, 0.0f);
		m_dRoundAngle = m_dRoundStep = 1.0f;
		break;
	case ID_VIEW_RLT:
		m_ptRoundBase.SetPoint(0.0, 1.0, 0.0);
		m_dRoundAngle = m_dRoundStep = -1.0f;
		break;
	case ID_VIEW_RRT:
		m_ptRoundBase.SetPoint(0.0f, 1.0f, 0.0f);
		m_dRoundAngle = m_dRoundStep = 1.0f;
		break;
	}
}

void CViewBaseGL::OnUpdateEditCopy(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(FALSE);	// OpenGLではcopy不可
}

/////////////////////////////////////////////////////////////////////////////

void OutputGLErrorMessage(GLenum errCode, LPCTSTR pszFile, UINT nline)
{
	CString	strMsg, strPath, strName;
	::Path_Name_From_FullPath(pszFile, strPath, strName);
	strMsg.Format(IDS_ERR_OPENGL, ::gluErrorString(errCode), strName, nline);
	AfxMessageBox(strMsg, MB_OK|MB_ICONEXCLAMATION);
}

void IDtoRGB(int id, GLubyte rgb[])
{
	// 0～254 の数値を RGB に変換
	// （255 は白色クリア値）
	div_t	d;
	d.quot = id;
	for ( int i=0; i<3; i++ ) {
		d = div(d.quot, 254);
		rgb[i] = d.rem;
	}
}

int RGBtoID(GLubyte rgb[])
{
	return rgb[0]==255 && rgb[1]==255 && rgb[2]==255 ?
		-1 : (rgb[2]*254*254 + rgb[1]*254 + rgb[0]);
}

int SearchSelectID(GLubyte buf[])
{
	GLubyte			rgb[3];
	std::map<int, int>	mp;
	int				id, maxct=0, maxid=-1;

	for ( int i=0; i<READBUF; i+=4 ) {
		rgb[0] = buf[i+0];
		rgb[1] = buf[i+1];
		rgb[2] = buf[i+2];
		id = RGBtoID(rgb);
		if ( id >= 0 ) {
			mp[id] = mp[id] + 1;	// mp[id]++; ではワーニング
		}
	}

	typedef std::map<int, int>::const_reference	T;
	BOOST_FOREACH(T x, mp) {
		if ( maxct < x.second ) {
			maxid = x.first;
			maxct = x.second;
		}
	}

#ifdef _DEBUG
	for ( int y=0; y<2*PICKREGION; y++ ) {
		CString	str, s;
		for ( int x=0; x<2*PICKREGION; x++ ) {
			rgb[0] = buf[4*(y*2*PICKREGION+x)+0];
			rgb[1] = buf[4*(y*2*PICKREGION+x)+1];
			rgb[2] = buf[4*(y*2*PICKREGION+x)+2];
			s.Format(" %d", RGBtoID(rgb));
//			s.Format(" %u", buf[4*(y*2*PICKREGION+x)+0]);
			str += s;
		}
		printf("pBuf[%d]=%s\n", y, LPCTSTR(str));
	}
	printf("size=%zd\n", mp.size());
	printf("maxid=%d, cnt=%d\n", maxid, maxct);
#endif

	return maxid;
}

/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
void CViewBaseGL::AssertValid() const
{
	__super::AssertValid();
}

void CViewBaseGL::Dump(CDumpContext& dc) const
{
	__super::Dump(dc);
}
#endif
