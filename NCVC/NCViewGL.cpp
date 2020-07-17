// NCViewGL.cpp : 実装ファイル
//

#include "stdafx.h"
#include "NCVC.h"
#include "NCChild.h"
#include "NCdata.h"
#include "NCDoc.h"
#include "NCViewTab.h"
#include "NCInfoTab.h"
#include "NCViewGL.h"
#include "NCListView.h"
#include "ViewOption.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

//	拡大率表示のための変換単位
//	1ｲﾝﾁあたりのﾋﾟｸｾﾙ数 GetDeviceCaps(LOGPIXELS[X|Y]) = 96dpi
//	1ｲﾝﾁ == 25.4mm
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
	ON_WM_KEYDOWN()
	// ﾍﾟｰｼﾞ切替ｲﾍﾞﾝﾄ
	ON_MESSAGE (WM_USERACTIVATEPAGE, OnUserActivatePage)
	// 各ﾋﾞｭｰへのﾌｨｯﾄﾒｯｾｰｼﾞ
	ON_MESSAGE (WM_USERVIEWFITMSG, OnUserViewFitMsg)
	// ﾒﾆｭｰｺﾏﾝﾄﾞ
	ON_COMMAND_RANGE(ID_VIEW_UP, ID_VIEW_RT, OnMoveKey)
	ON_COMMAND_RANGE(ID_VIEW_FIT, ID_VIEW_LENSN, OnLensKey)
END_MESSAGE_MAP()

//	ｳｨﾝﾄﾞｳﾒｯｾｰｼﾞを受信し、OpenGL命令を操作するときは
//	::wglMakeCurrent( pDC->GetSafeHdc(), m_hRC );
//	-- OpenGL命令 --
//	::wglMakeCurrent( NULL, NULL );
//	を必ず入れておく

/////////////////////////////////////////////////////////////////////////////
// CNCViewGL クラスの構築/消滅

CNCViewGL::CNCViewGL()
{
	m_cx = m_cy = 0;
	m_dRate = 0.0;
	m_hRC = NULL;
	m_glCode = m_glWork = 0;

	m_fDepth = NULL;

	m_enTrackingMode = TM_NONE;
	ClearObjectForm();
}

CNCViewGL::~CNCViewGL()
{
	if ( m_fDepth )
		delete	m_fDepth;
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewGL クラスのオーバライド関数

BOOL CNCViewGL::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style |= WS_CLIPSIBLINGS|WS_CLIPCHILDREN;
	return CView::PreCreateWindow(cs);
}

void CNCViewGL::OnInitialUpdate() 
{
#ifdef _DEBUG
	CMagaDbg	dbg("CNCViewGL::OnInitialUpdate()\nStart", DBG_CYAN);
#endif
	CClientDC	dc(this);
	::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );

	// 視点座標変換
	::glMatrixMode( GL_MODELVIEW );
	::glLoadIdentity();

	// 切削ﾊﾟｽのﾃﾞｨｽﾌﾟﾚｲﾘｽﾄ生成
	// ｵﾌﾞｼﾞｪｸﾄ数が多いとﾒﾓﾘ消費量が多すぎる(かも知れない)ので
	// 10,000を閾値とする
	if ( GetDocument()->GetNCsize() <= 10000 ) {
		m_glCode = ::glGenLists(1);
		if( m_glCode > 0 ) {
			// NCﾃﾞｰﾀ描画のﾃﾞｨｽﾌﾟﾚｲﾘｽﾄ生成
			::glNewList( m_glCode, GL_COMPILE );
				RenderCode(TRUE);
			::glEndList();
		}
	}
/*
	// ﾜｰｸ矩形のﾃﾞｨｽﾌﾟﾚｲﾘｽﾄ生成
	m_glWork = ::glGenLists(1);
	if( m_glWork > 0 ) {
		::glNewList( m_glWork, GL_COMPILE );
			RenderWork();
		::glEndList();
	}

	// 切削ﾊﾟｽのﾚﾝﾀﾞﾘﾝｸﾞ準備
	CNCdata*	pData;
	CNCdata*	pDataNext;
	int		i, nLoop = GetDocument()->GetNCsize();

	// 切削底面のﾃﾞｨｽﾌﾟﾚｲﾘｽﾄ
	for ( i=0; i<nLoop; i++ ) {
		pData = GetDocument()->GetNCdata(i);
		pDataNext = i<nLoop-1 ? GetDocument()->GetNCdata(i+1) : NULL;
		pData->CreateOcclusionCulling(pDataNext);
	}
*/
	::wglMakeCurrent( NULL, NULL );
}

void CNCViewGL::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	switch ( lHint ) {
	case UAV_STARTSTOPTRACE:
	case UAV_TRACECURSOR:
	case UAV_DRAWWORKRECT:
	case UAV_DRAWMAXRECT:
		return;				// 無視
	case UAV_FILEINSERT:
		OnLensKey(ID_VIEW_FIT);
		// ﾌﾟﾘﾐﾃｨﾌﾞ再構成
		// ...
		return;
		Invalidate(FALSE);
		return;
	case UAV_CHANGEFONT:	// 色の変更
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
	// ここから CDocument::OnCmdMsg() を呼ばないようにする
//	return CView::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
	return CWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewGL クラスのメンバ関数

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
		0,		// Stencil Buffer
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

void CNCViewGL::ClipDepth(void)
{
	::glDisable( GL_STENCIL_TEST );
	::glDisable( GL_LIGHTING );

	int			nResult;
	CRect3D		rcMax(GetDocument()->GetMaxRect());
	GLint		viewPort[4];
	GLdouble	mvMatrix[16], pjMatrix[16],
				wx1, wy1, wz1, wx2, wy2, wz2;
	GLsizei		cx, cy;

	::glGetIntegerv(GL_VIEWPORT, viewPort);
	::glGetDoublev (GL_MODELVIEW_MATRIX,  mvMatrix);
	::glGetDoublev (GL_PROJECTION_MATRIX, pjMatrix);

	// 矩形領域をﾋﾟｸｾﾙ座標に変換
	nResult = ::gluProject(rcMax.left,  rcMax.top,    0.0, mvMatrix, pjMatrix, viewPort,
					&wx1, &wy1, &wz1);
	nResult = ::gluProject(rcMax.right, rcMax.bottom, 0.0, mvMatrix, pjMatrix, viewPort,
					&wx2, &wy2, &wz2);
	cx = (GLsizei)(wx2 - wx1);
	cy = (GLsizei)(wy2 - wy1);

	// 矩形領域のﾃﾞﾌﾟｽ値を取得（ﾋﾟｸｾﾙ単位）
	m_fDepth = new GLfloat[cx*cy];
	::glReadPixels((GLint)wx1, (GLint)wy1, cx, cy,
			GL_DEPTH_COMPONENT, GL_FLOAT, m_fDepth);

#ifdef _DEBUG
/*
	GLsizei		i, j;
	CStdioFile	f("D:\\tmp\\depth.csv", CFile::typeText|CFile::modeCreate|CFile::modeWrite);
	CString		s, r;
	for ( j=0; j<cy; j++ ) {
		s.Empty();
		for ( i=0; i<cx; i++ ) {
			if ( !s.IsEmpty() )
				s += ", ";
			r.Format(IDS_MAKENCD_FORMAT, m_fDepth[j*cy+i]);
			s += r;
		}
		f.WriteString(s+"\n");
	}
*/
//	double	znr = pjMatrix[14]/(pjMatrix[10]-1.0),
//			zfr = pjMatrix[14]/(pjMatrix[10]+1.0);
//	g_dbg.printf("near=%f far=%f", znr, zfr);
//	g_dbg.printf("depth=%f", zfr*znr/(0.515*(zfr-znr)-zfr));
	nResult = ::gluUnProject(wx1, wy1, 0.515, mvMatrix, pjMatrix, viewPort,
					&wx2, &wy2, &wz2);
	g_dbg.printf("0.515 => %f", wz2);
	nResult = ::gluUnProject(wx1, wy1, 0.482, mvMatrix, pjMatrix, viewPort,
					&wx2, &wy2, &wz2);
	g_dbg.printf("0.482 => %f", wz2);
#endif

	::glEnable ( GL_LIGHTING );
	::glEnable ( GL_STENCIL_TEST );
}

void CNCViewGL::RenderBack(void)
{
	// 背景ﾎﾟﾘｺﾞﾝの描画
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	COLORREF	col1 = pOpt->GetNcDrawColor(NCCOL_BACKGROUND1),
				col2 = pOpt->GetNcDrawColor(NCCOL_BACKGROUND2);
	GLubyte		col1v[3], col2v[3];
	GLdouble	dVertex[3];
	col1v[0] = GetRValue(col1);
	col1v[1] = GetGValue(col1);
	col1v[2] = GetBValue(col1);
	col2v[0] = GetRValue(col2);
	col2v[1] = GetGValue(col2);
	col2v[2] = GetBValue(col2);

	::glPushMatrix();
	::glLoadIdentity();
	::glBegin(GL_QUADS);
	// 左上
	dVertex[0] = m_rcView.left;
	dVertex[1] = m_rcView.bottom;
	dVertex[2] = m_rcView.low+NCMIN;	// 最背面より少し手前
	::glColor3ubv(col1v);
	::glVertex3dv(dVertex);
	// 左下
	dVertex[1] = m_rcView.top;
	::glColor3ubv(col2v);
	::glVertex3dv(dVertex);
	// 右下
	dVertex[0] = m_rcView.right;
	::glColor3ubv(col2v);
	::glVertex3dv(dVertex);
	// 右上
	dVertex[1] = m_rcView.bottom;
	::glColor3ubv(col1v);
	::glVertex3dv(dVertex);
	//
	::glEnd();
	::glPopMatrix();
}

void CNCViewGL::RenderAxis(void)
{
	extern	const	PENSTYLE	g_penStyle[];	// ViewOption.cpp
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	double		dLength;
	COLORREF	col;

	::glPushAttrib( GL_CURRENT_BIT | GL_LINE_BIT );	// 色 | 線情報
	::glLineWidth( 2.0 );
	::glEnable( GL_LINE_STIPPLE );
	::glBegin( GL_LINES );

	// X軸のｶﾞｲﾄﾞ
	dLength = pOpt->GetGuideLength(NCA_X);
	col = pOpt->GetNcDrawColor(NCCOL_GUIDEX);
	::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(NCA_X)].nGLpattern);
	::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
	::glVertex3d(-dLength, 0.0, 0.0);
	::glVertex3d( dLength, 0.0, 0.0);
	// Y軸のｶﾞｲﾄﾞ
	dLength = pOpt->GetGuideLength(NCA_Y);
	col = pOpt->GetNcDrawColor(NCCOL_GUIDEY);
	::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(NCA_Y)].nGLpattern);
	::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
	::glVertex3d(0.0, -dLength, 0.0);
	::glVertex3d(0.0,  dLength, 0.0);
	// Z軸のｶﾞｲﾄﾞ
	dLength = pOpt->GetGuideLength(NCA_Z);
	col = pOpt->GetNcDrawColor(NCCOL_GUIDEZ);
	::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(NCA_Z)].nGLpattern);
	::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
	::glVertex3d(0.0, 0.0, -dLength);
	::glVertex3d(0.0, 0.0,  dLength);

	::glEnd();
	::glDisable( GL_LINE_STIPPLE );
	::glPopAttrib();
}

void CNCViewGL::RenderWork(void)
{
	// ﾜｰｸ矩形の描画(光源設定により発色)
	CRect3D		rc(GetDocument()->GetMaxRect());

	// ﾜｰｸ矩形を上下左右1%ずつ大きく、深さを1%小さくする
	rc.InflateRect(rc.Width()*0.01, rc.Height()*0.01, -rc.Depth()*0.01);
/*
	::glBegin(GL_QUADS);
	// 上面
	::glNormal3d(0.0, 0.0, 1.0);
	::glVertex3d(rc.left,	rc.bottom,	rc.high);
	::glVertex3d(rc.left,	rc.top,		rc.high);
	::glVertex3d(rc.right,	rc.top,		rc.high);
	::glVertex3d(rc.right,	rc.bottom,	rc.high);
	// 前面
	::glNormal3d(0.0, 1.0, 0.0);
	::glVertex3d(rc.left,	rc.top,		rc.high);
	::glVertex3d(rc.left,	rc.top,		rc.low );
	::glVertex3d(rc.right,	rc.top,		rc.low );
	::glVertex3d(rc.right,	rc.top,		rc.high);
	// 底面
	::glNormal3d(0.0, 0.0, -1.0);
	::glVertex3d(rc.right,	rc.bottom,	rc.low );
	::glVertex3d(rc.right,	rc.top,		rc.low );
	::glVertex3d(rc.left,	rc.top,		rc.low );
	::glVertex3d(rc.left,	rc.bottom,	rc.low );
	// 奥面
	::glNormal3d(0.0, -1.0, 0.0);
	::glVertex3d(rc.right,	rc.bottom,	rc.high);
	::glVertex3d(rc.right,	rc.bottom,	rc.low );
	::glVertex3d(rc.left,	rc.bottom,	rc.low );
	::glVertex3d(rc.left,	rc.bottom,	rc.high);
	// 左側面
	::glNormal3d(-1.0, 0.0, 0.0);
	::glVertex3d(rc.left,	rc.bottom,	rc.high);
	::glVertex3d(rc.left,	rc.bottom,	rc.low );
	::glVertex3d(rc.left,	rc.top,		rc.low );
	::glVertex3d(rc.left,	rc.top,		rc.high);
	// 右側面
	::glNormal3d(1.0, 0.0, 0.0);
	::glVertex3d(rc.right,	rc.bottom,	rc.high);
	::glVertex3d(rc.right,	rc.top,		rc.high);
	::glVertex3d(rc.right,	rc.top,		rc.low );
	::glVertex3d(rc.right,	rc.bottom,	rc.low );
	::glEnd();
*/
/*
	// 側面をｸﾞﾘｯﾄﾞで表現
	int		i;
	double	d, s=rc.Height() / 1000;	// 1000分割
	::glBegin(GL_QUADS);
	::glNormal3d(0.0, 1.0, 0.0);
	for ( i=0, d=0; i<=1000; i++, d+=s ) {
		::glVertex3d(rc.left,	rc.top+d,	rc.high);
		::glVertex3d(rc.left,	rc.top+d,	rc.low );
		::glVertex3d(rc.right,	rc.top+d,	rc.low );
		::glVertex3d(rc.right,	rc.top+d,	rc.high);
	}
	::glNormal3d(-1.0, 0.0, 0.0);
	s = rc.Width() / 1000;
	for ( i=0, d=0; i<=1000; i++, d+=s ) {
		::glVertex3d(rc.left+d,	rc.bottom,	rc.high);
		::glVertex3d(rc.left+d,	rc.bottom,	rc.low );
		::glVertex3d(rc.left+d,	rc.top,		rc.low );
		::glVertex3d(rc.left+d,	rc.top,		rc.high);
	}
	::glEnd();
*/
	// 100x100 ﾃﾞｸｾﾙ表示
	int		i, j;
	double	dx, sx = rc.Width()  / 100,
			dy, sy = rc.Height() / 100;
	::glBegin(GL_QUADS);
	for ( i=0, dx=rc.left; i<100; i++, dx+=sx ) {
		for ( j=0, dy=rc.top; j<100; j++, dy+=sy ) {
			// 上面
			::glNormal3d(0.0, 0.0, 1.0);
			::glVertex3d(dx,	dy,		rc.high);
			::glVertex3d(dx+sx,	dy,		rc.high);
			::glVertex3d(dx+sx,	dy+sy,	rc.high);
			::glVertex3d(dx,	dy+sy,	rc.high);
			// 底面
			::glNormal3d(0.0, 0.0, -1.0);
			::glVertex3d(dx,	dy,		rc.low );
			::glVertex3d(dx,	dy+sy,	rc.low );
			::glVertex3d(dx+sx,	dy+sy,	rc.low );
			::glVertex3d(dx+sx,	dy,		rc.low );
			// 前面
			::glNormal3d(0.0, 1.0, 0.0);
			::glVertex3d(dx,	dy,		rc.high);
			::glVertex3d(dx,	dy,		rc.low );
			::glVertex3d(dx+sx,	dy,		rc.low );
			::glVertex3d(dx+sx,	dy,		rc.high);
			// 奥面
			::glNormal3d(0.0, -1.0, 0.0);
			::glVertex3d(dx,	dy+sy,	rc.high);
			::glVertex3d(dx+sx,	dy+sy,	rc.high);
			::glVertex3d(dx+sx,	dy+sy,	rc.low );
			::glVertex3d(dx,	dy+sy,	rc.low );
			// 左側面
			::glNormal3d(-1.0, 0.0, 0.0);
			::glVertex3d(dx,	dy,		rc.high);
			::glVertex3d(dx,	dy+sy,	rc.high);
			::glVertex3d(dx,	dy+sy,	rc.low );
			::glVertex3d(dx,	dy,		rc.low );
			// 右側面
			::glNormal3d(1.0, 0.0, 0.0);
			::glVertex3d(dx+sx,	dy,		rc.high);
			::glVertex3d(dx+sx,	dy,		rc.low );
			::glVertex3d(dx+sx,	dy+sy,	rc.low );
			::glVertex3d(dx+sx,	dy+sy,	rc.high);
		}
	}
	::glEnd();
}

void CNCViewGL::RenderCode(BOOL bInitial)
{
	BOOL	bG00 = bInitial ? FALSE :
				AfxGetNCVCApp()->GetViewOption()->GetNCViewFlg(NCVIEWFLG_G00VIEW);

	::glEnable( GL_LINE_STIPPLE );
	// NCﾃﾞｰﾀ描画
	for ( int i=0; i<GetDocument()->GetNCsize(); i++ )
		GetDocument()->GetNCdata(i)->DrawGL(bG00);
	::glDisable( GL_LINE_STIPPLE );
}

CPoint3D CNCViewGL::PtoR(const CPoint& pt)
{
	CPoint3D	ptResult;
	// ﾓﾃﾞﾙ空間の回転
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
	Invalidate(FALSE);		// 線画からﾚﾝﾀﾞﾘﾝｸﾞ描画へ
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

	// MDI子ﾌﾚｰﾑのｽﾃｰﾀｽﾊﾞｰに情報表示
	static_cast<CNCChild *>(GetParentFrame())->SetFactorInfo(NC_XYZ_PLANE, m_dRate/LOGPIXEL);
}

void CNCViewGL::DoRotation(double dAngle, const CPoint3D& pt)
{
	// 回転ﾏﾄﾘｯｸｽを現在のｵﾌﾞｼﾞｪｸﾄﾌｫｰﾑﾏﾄﾘｯｸｽに掛け合わせる
	::glLoadIdentity();
	::glRotated( dAngle, pt.x, pt.y, pt.z );
	::glMultMatrixd( (GLdouble *)m_objectXform );
	::glGetDoublev( GL_MODELVIEW_MATRIX, (GLdouble *)m_objectXform );

	// ﾓﾃﾞﾘﾝｸﾞ&ﾋﾞｭｰｲﾝｸﾞ変換行列
	SetupViewingTransform();
}

void CNCViewGL::SetupViewingTransform(void)
{
	::glLoadIdentity();
	::glTranslated( m_ptCenter.x, m_ptCenter.y, 0.0 );
	::glMultMatrixd( (GLdouble *)m_objectXform );	// 表示回転（＝モデル回転）
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewGL 描画

void CNCViewGL::OnDraw(CDC* pDC)
{
	ASSERT_VALID(GetDocument());
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();

	// ｶﾚﾝﾄｺﾝﾃｷｽﾄの割り当て
	::wglMakeCurrent( pDC->GetSafeHdc(), m_hRC );

	::glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	::glDisable( GL_LIGHTING );
	::glDepthFunc( GL_LESS );

	// 背景の描画
	RenderBack();
	// 軸の描画
	RenderAxis();
/*
	if ( m_enTrackingMode==TM_NONE || pOpt->GetNCViewFlg(NCVIEWFLG_DRAGRENDER) ) {
		::glEnable(GL_LIGHT0);  
		::glEnable(GL_LIGHT1);
		::glDisable(GL_LIGHT2);  
		::glDisable(GL_LIGHT3);
		::glEnable ( GL_LIGHTING );
		// ﾜｰｸ矩形(ﾃﾞｸｾﾙ)を描画 => ﾃﾞﾌﾟｽﾊﾞｯﾌｧ更新
		::glCallList(m_glWork);
		// 切削ﾊﾟｽのｿﾘｯﾄﾞ描画
		::glDepthFunc( GL_GEQUAL );
		::glDisable(GL_LIGHT0);  
		::glDisable(GL_LIGHT1);
		::glEnable(GL_LIGHT2);  
		::glEnable(GL_LIGHT3);
		for ( int i=0; i<GetDocument()->GetNCsize(); i++ )
			GetDocument()->GetNCdata(i)->DrawMill();
		::glDepthFunc( GL_LESS );
		// 移動ﾊﾟｽのﾜｲﾔｰ描画
		::glDisable( GL_LIGHTING );
		if ( pOpt->GetNCViewFlg(NCVIEWFLG_G00VIEW) ) {
			if ( m_glCode > 0 )
				::glCallList( m_glCode );
			else
				RenderCode(TRUE);
		}
	}
	else {
		// 線画
		if ( m_glCode > 0 )
			::glCallList( m_glCode );
		else
			RenderCode(TRUE);
	}
*/

	// 線画
	if ( m_glCode > 0 )
		::glCallList( m_glCode );
	else
		RenderCode(TRUE);

//	::glFinish();		// SwapBuffers() に含まれる
	::SwapBuffers( pDC->GetSafeHdc() );

	::wglMakeCurrent(NULL, NULL);
}

/////////////////////////////////////////////////////////////////////////////
// CNCViewGL 診断

#ifdef _DEBUG
void CNCViewGL::AssertValid() const
{
	CView::AssertValid();
}

void CNCViewGL::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CNCDoc* CNCViewGL::GetDocument() // 非デバッグ バージョンはインラインです。
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CNCDoc)));
	return static_cast<CNCDoc *>(m_pDocument);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CNCViewGL メッセージ ハンドラ

int CNCViewGL::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if ( CView::OnCreate(lpCreateStruct) < 0 )
		return -1;

	// ｽﾌﾟﾘｯﾀからの境界ｶｰｿﾙが移るので，IDC_ARROW を明示的に指定
	// 他の NCView.cpp のように PreCreateWindow() での AfxRegisterWndClass() ではｴﾗｰ??
	::SetClassLongPtr(m_hWnd, GCLP_HCURSOR,
		(LONG_PTR)AfxGetApp()->LoadStandardCursor(IDC_ARROW));

	// OpenGL初期化処理
	CClientDC	dc(this);

	// OpenGL pixel format の設定
    if( !SetupPixelFormat(&dc) ) {
		TRACE0("SetupPixelFormat failed\n");
		return -1;
	}

	// ﾚﾝﾀﾞﾘﾝｸﾞｺﾝﾃｷｽﾄの作成
	if( !(m_hRC = ::wglCreateContext(dc.GetSafeHdc())) ) {
		TRACE0("wglCreateContext failed\n");
		return -1;
	}

	// ﾚﾝﾀﾞﾘﾝｸﾞｺﾝﾃｷｽﾄをｶﾚﾝﾄのﾃﾞﾊﾞｲｽｺﾝﾃｷｽﾄに設定
	if( !::wglMakeCurrent(dc.GetSafeHdc(), m_hRC) ) {
		TRACE0("wglMakeCurrent failed\n");
		return -1;
	}

	// OpenGL Extention 使用準備
	GLenum glewResult = ::glewInit();
	if ( glewResult != GLEW_OK ) {
		TRACE1("glewInit() failed code=%s\n", ::glewGetErrorString(glewResult));
		return -1;
	}

	// Enable two OpenGL lights
    GLfloat light_diffuse_red[] = {1.0, 0.0, 0.0, 1.0};		// Red diffuse light
    GLfloat light_diffuse_yellow[] = {1.0, 1.0, 0.0, 1.0};	// Yellow diffuse light
    GLfloat light_position0[] = {-1.0, -1.0, -1.0, 0.0};	// Infinite light location
    GLfloat light_position1[] = {1.0, 1.0, 1.0, 0.0};		// Infinite light location

	::glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse_red);
	::glLightfv(GL_LIGHT0, GL_POSITION, light_position0);
	::glLightfv(GL_LIGHT1, GL_DIFFUSE, light_diffuse_red);
	::glLightfv(GL_LIGHT1, GL_POSITION, light_position1);
	::glLightfv(GL_LIGHT2, GL_DIFFUSE, light_diffuse_yellow);
	::glLightfv(GL_LIGHT2, GL_POSITION, light_position0);
	::glLightfv(GL_LIGHT3, GL_DIFFUSE, light_diffuse_yellow);
	::glLightfv(GL_LIGHT3, GL_POSITION, light_position1);
//	::glEnable(GL_LIGHTING);
	::glEnable(GL_NORMALIZE);

	// ｸﾘｱｶﾗｰの設定
	::glClearColor( 0, 0, 0, 0 );	// 黒

	// ﾃﾞﾌﾟｽﾊﾞｯﾌｧのｸﾘｱ
	::glClearDepth( 1.0 );

	// ﾃﾞﾌﾟｽﾃｽﾄ
	::glEnable( GL_DEPTH_TEST );

	// ﾃﾞﾌﾟｽﾌｧﾝｸ（手前にあるもので上描いていく）
//	::glDepthFunc( GL_LESS );

	// 裏面は描かない設定(Debug)
//	::glFrontFace(GL_CCW);
//	::glCullFace(GL_BACK);
//	::glEnable(GL_CULL_FACE);

#ifdef _DEBUGOLD
	g_dbg.printf("GetDeviceCaps([HORZSIZE|VERTSIZE])=%d, %d",
		dc.GetDeviceCaps(HORZSIZE), dc.GetDeviceCaps(VERTSIZE) );
	g_dbg.printf("GetDeviceCaps([LOGPIXELSX|LOGPIXELSY])=%d, %d",
		dc.GetDeviceCaps(LOGPIXELSX), dc.GetDeviceCaps(LOGPIXELSY) );
	GLint	nGLResult;
	g_dbg.printf("Using OpenGL version:%s", ::glGetString(GL_VERSION));
	g_dbg.printf("Using GLEW   version:%s", ::glewGetString(GLEW_VERSION));
	::glGetIntegerv(GL_STENCIL_BITS, &nGLResult);
	g_dbg.printf(" Stencil bits=%d", nGLResult);
	::glGetIntegerv(GL_DEPTH_BITS, &nGLResult);
	g_dbg.printf(" Depth   bits=%d", nGLResult);
#endif

	::wglMakeCurrent( NULL, NULL );

	return 0;
}

void CNCViewGL::OnDestroy()
{
	CClientDC	dc(this);
	::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );

	// NCﾃﾞｰﾀ各ｵﾌﾞｼﾞｪｸﾄのﾃﾞｨｽﾌﾟﾚｲﾘｽﾄ消去
	CNCdata*	pData;
	for ( int i=0; i<GetDocument()->GetNCsize(); i++ ) {
		pData = GetDocument()->GetNCdata(i);
		if ( pData->m_glList > 0 )
			::glDeleteLists(pData->m_glList, 1);
	}

	// 全体の切削パスを示すﾃﾞｨｽﾌﾟﾚｲﾘｽﾄ消去
	if ( m_glCode > 0 )
		::glDeleteLists(m_glCode, 1);
	if ( m_glWork > 0 )
		::glDeleteLists(m_glWork, 1);

	::wglMakeCurrent(NULL, NULL);
	::wglDeleteContext( m_hRC );
	
	CView::OnDestroy();
}

void CNCViewGL::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);
	if( cx <= 0 || cy <= 0 )
		return;

	m_cx = cx;
	m_cy = cy;

	// ﾋﾞｭｰﾎﾟｰﾄ設定処理
	CClientDC	dc(this);
	::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );
	::glViewport(0, 0, cx, cy);
	::wglMakeCurrent(NULL, NULL);
}

void CNCViewGL::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) 
{
	if ( bActivate ) {
#ifdef _DEBUG
		g_dbg.printf("CNCViewGL::OnActivateView()");
#endif
		// MDI子ﾌﾚｰﾑのｽﾃｰﾀｽﾊﾞｰに情報表示
		static_cast<CNCChild *>(GetParentFrame())->SetFactorInfo(NC_XYZ_PLANE, m_dRate/LOGPIXEL);
	}
	CView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}

LRESULT CNCViewGL::OnUserActivatePage(WPARAM, LPARAM lParam)
{
	if ( m_dRate == 0 )
		OnLensKey(ID_VIEW_FIT);
	// 他のﾋﾞｭｰとは違って、拡大率を更新する必要は無い
	// ｽﾃｰﾀｽﾊﾞｰへの表示も OnActivateView() で行う
	return 0;
}

LRESULT CNCViewGL::OnUserViewFitMsg(WPARAM, LPARAM)
{
	extern	const	double	g_dDefaultGuideLength;	// 50.0 (ViewOption.cpp)
#ifdef _DEBUG
	CMagaDbg	dbg("CNCViewGL::OnUserViewFitMsg()\nStart");
#endif
	m_rcView = GetDocument()->GetMaxRect();
	// 占有矩形の補正(不正表示の防止)
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	double	dLength;
	if ( m_rcView.Width() <= NCMIN ) {
		dLength = pOpt->GetGuideLength(NCA_X);
		if ( dLength == 0.0 )
			dLength = g_dDefaultGuideLength;
		m_rcView.left  = -dLength;
		m_rcView.right =  dLength;
	}
	if ( m_rcView.Height() <= NCMIN ) {
		dLength = pOpt->GetGuideLength(NCA_Y);
		if ( dLength == 0.0 )
			dLength = g_dDefaultGuideLength;
		m_rcView.top    = -dLength;
		m_rcView.bottom =  dLength;
	}
	if ( m_rcView.Depth() <= NCMIN ) {
		dLength = pOpt->GetGuideLength(NCA_Z);
		if ( dLength == 0.0 )
			dLength = g_dDefaultGuideLength;
		m_rcView.low  = -dLength;
		m_rcView.high =  dLength;
	}
	// ｵﾌﾞｼﾞｪｸﾄ矩形を10%(上下左右5%ずつ)大きく
	m_rcView.InflateRect(m_rcView.Width()*0.05, m_rcView.Height()*0.05);
	m_rcView.NormalizeRect();

	// ﾃﾞｨｽﾌﾟﾚｲのｱｽﾍﾟｸﾄ比から視野直方体設定
	CPointD	pt(m_rcView.CenterPoint());
	double	dW = m_rcView.Width(), dH = m_rcView.Height(), dZ;
	if ( dW > dH ) {
		dH = dW * m_cy / m_cx / 2.0;
		m_rcView.top    = pt.y - dH;
		m_rcView.bottom = pt.y + dH;
		m_dRate = m_cx / dW;
	}
	else {
		dW = dH * m_cx / m_cy / 2.0;
		m_rcView.left   = pt.x - dW;
		m_rcView.right  = pt.x + dW;
		m_dRate = m_cy / dH;
	}
	dZ = max(max(m_rcView.Width(), m_rcView.Height()), m_rcView.Depth()) * 2.0;
	m_rcView.high =  dZ;	// 奥
	m_rcView.low  = -dZ;	// 手前
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
		// MDI子ﾌﾚｰﾑのｽﾃｰﾀｽﾊﾞｰに情報表示
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

void CNCViewGL::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if ( nChar == VK_TAB ) {
		CNCChild*	pFrame = static_cast<CNCChild *>(GetParentFrame());
		if ( ::GetKeyState(VK_SHIFT) < 0 )
			pFrame->GetListView()->SetFocus();
		else
			pFrame->GetInfoView()->SetFocus();
	}

	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

BOOL CNCViewGL::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}
