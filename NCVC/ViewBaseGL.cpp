// ViewBaseGL.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "ViewBaseGL.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#include <mmsystem.h>			// timeGetTime()
#endif

IMPLEMENT_DYNAMIC(CViewBaseGL, CView)

BEGIN_MESSAGE_MAP(CViewBaseGL, CView)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CViewBaseGL

CViewBaseGL::CViewBaseGL()
{
	m_hRC = NULL;
	m_cx = m_cy = 0;
}

BOOL CViewBaseGL::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style |= WS_CLIPSIBLINGS|WS_CLIPCHILDREN;
	return __super::PreCreateWindow(cs);
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

BOOL CViewBaseGL::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}

BOOL CViewBaseGL::_OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);
	if( cx <= 0 || cy <= 0 )
		return FALSE;

	m_cx = cx;
	m_cy = cy;
	// ﾋﾞｭｰﾎﾟｰﾄ設定処理
	CClientDC	dc(this);
	::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );
	::glViewport(0, 0, cx, cy);
	::wglMakeCurrent(NULL, NULL);

	return TRUE;
}
