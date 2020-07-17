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
//#define	_DEBUG_FILEOUT_		// Depth File out
//#define	_DEBUG_DRAWTEST_
extern	CMagaDbg	g_dbg;
#include <mmsystem.h>			// timeGetTime()
#endif

// 頂点配列生成ｽﾚｯﾄﾞ用
typedef	std::vector<GLuint>	CVElement;
typedef struct tagCREATEELEMENTPARAM {
	const GLfloat*	pfXYZ;
	GLfloat*		pfNOR;
	std::vector<CVElement>*	pElement;
	int		cx, cy;
	GLfloat	h, l;
	BOOL	bResult;
} CREATEELEMENTPARAM, *LPCREATEELEMENTPARAM;

static	DWORD WINAPI CreateElement_WrkThread(LPVOID);
static	DWORD WINAPI CreateElement_CutThread(LPVOID);
static	BOOL		 CreateElement_Side(LPCREATEELEMENTPARAM);

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
	ON_WM_CONTEXTMENU()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
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
	m_bActive = FALSE;
	m_cx = m_cy = m_icx = m_icy = 0;
	m_dRate = 0.0;
	m_hRC = NULL;
	m_glCode = 0;

	m_nVertexID = m_nNormalID =
		m_nPictureID = m_nTextureID = 0;
	m_pGenBuf = NULL;

	m_enTrackingMode = TM_NONE;
	ClearObjectForm();
}

CNCViewGL::~CNCViewGL()
{
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
	extern	LPCTSTR	g_szNdelimiter;	// "XYZRIJKPLDH" from NCDoc.cpp

#ifdef _DEBUG
	CMagaDbg	dbg("CNCViewGL::OnInitialUpdate()\nStart", DBG_CYAN);
#endif
	CView::OnInitialUpdate();

	if ( GetDocument()->IsNCDocFlag(NCDOC_LATHE) ) {
		m_strGuide  = g_szNdelimiter[NCA_Z];	// [ZYX]
		m_strGuide += g_szNdelimiter[NCA_Y];
		m_strGuide += g_szNdelimiter[NCA_X];
	}
	else {
		m_strGuide = g_szNdelimiter;
		m_strGuide.Delete(NCXYZ, 999);			// [XYZ]
	}

	// まだｳｨﾝﾄﾞｳがｱｸﾃｨﾌﾞでない可能性があるので
	// ここでの OpenGLｺﾏﾝﾄﾞ は使用しない
}

void CNCViewGL::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();

	switch ( lHint ) {
	case UAV_STARTSTOPTRACE:
	case UAV_TRACECURSOR:
	case UAV_DRAWMAXRECT:
		return;		// 無視
	case UAV_FILEINSERT:	// 占有矩形の変更
		pOpt->m_dwUpdateFlg = VIEWUPDATE_DISPLAYLIST | VIEWUPDATE_BOXEL;
		// through
	case UAV_DRAWWORKRECT:	// ﾜｰｸ矩形変更
		if ( pOpt->GetNCViewFlg(NCVIEWFLG_SOLIDVIEW) &&
				GLEW_ARB_vertex_buffer_object &&	// OpenGL拡張ｻﾎﾟｰﾄ
				m_rcDraw != GetDocument()->GetWorkRect() )
			pOpt->m_dwUpdateFlg |= VIEWUPDATE_BOXEL;
		// through
	case UAV_CHANGEFONT:	// 色の変更 etc.
		if ( m_bActive ) {
			// 各ﾍﾟｰｼﾞごとに更新情報がｾｯﾄされている
			UpdateViewOption();
			Invalidate(FALSE);
		}
		pOpt->m_dwUpdateFlg = 0;
		return;
	case UAV_ADDINREDRAW:
		Invalidate(FALSE);
		return;
	}
	CView::OnUpdate(pSender, lHint, pHint);
}

void CNCViewGL::UpdateViewOption(void)
{
	CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();

	// 初期視点に戻す
	if ( pOpt->m_dwUpdateFlg & VIEWUPDATE_BOXEL )
		OnLensKey(ID_VIEW_FIT);

	CClientDC	dc(this);
	::wglMakeCurrent( dc.GetSafeHdc(), m_hRC );

	// ﾃｸｽﾁｬ画像の読み込み
	if ( pOpt->GetNCViewFlg(NCVIEWFLG_SOLIDVIEW) &&
			pOpt->m_dwUpdateFlg & VIEWUPDATE_TEXTURE &&
			GLEW_ARB_vertex_buffer_object ) {	// OpenGL拡張ｻﾎﾟｰﾄ
		ClearTexture();
		if ( pOpt->GetNCViewFlg(NCVIEWFLG_TEXTURE) ) {
			if ( ReadTexture(pOpt->GetTextureFile()) &&	// m_nPictureID値ｾｯﾄ
					!(pOpt->m_dwUpdateFlg & VIEWUPDATE_BOXEL) ) {
				// 画像だけ変更
				if ( GetDocument()->IsNCDocFlag(NCDOC_LATHE) )
					CreateTextureLathe();
				else
					CreateTextureMill();
			}
		}
	}

	// 光源の色設定
	if ( pOpt->m_dwUpdateFlg & (VIEWUPDATE_LIGHT|VIEWUPDATE_TEXTURE) ) {
		GLfloat light_Wrk[] = {0.0, 0.0, 0.0, 1.0};
		GLfloat light_Cut[] = {0.0, 0.0, 0.0, 1.0};
		GLfloat light_Position0[] = {-1.0, -1.0, -1.0,  0.0};
		GLfloat light_Position1[] = { 1.0,  1.0,  1.0,  0.0};
		if ( m_nPictureID > 0 ) {
			// ﾃｸｽﾁｬ色を有効にするため白色光源
			for ( int i=0; i<NCXYZ; i++ )
				light_Wrk[i] = light_Cut[i] = 1.0;
		}
		else {
			// 表示設定に基づく光源
			COLORREF	col;
			col = pOpt->GetNcDrawColor(NCCOL_GL_WRK);
			light_Wrk[0] = (GLfloat)GetRValue(col) / 255;	// 255 -> 1.0
			light_Wrk[1] = (GLfloat)GetGValue(col) / 255;
			light_Wrk[2] = (GLfloat)GetBValue(col) / 255;
			col = pOpt->GetNcDrawColor(NCCOL_GL_CUT);
			light_Cut[0] = (GLfloat)GetRValue(col) / 255;
			light_Cut[1] = (GLfloat)GetGValue(col) / 255;
			light_Cut[2] = (GLfloat)GetBValue(col) / 255;
		}
		::glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_Wrk);
		::glLightfv(GL_LIGHT1, GL_DIFFUSE,  light_Wrk);
		::glLightfv(GL_LIGHT2, GL_DIFFUSE,  light_Cut);
		::glLightfv(GL_LIGHT3, GL_DIFFUSE,  light_Cut);
		if ( !m_bActive ) {
			::glLightfv(GL_LIGHT0, GL_POSITION, light_Position0);
			::glLightfv(GL_LIGHT1, GL_POSITION, light_Position1);
			::glLightfv(GL_LIGHT2, GL_POSITION, light_Position0);
			::glLightfv(GL_LIGHT3, GL_POSITION, light_Position1);
		}
	}

	// ﾜｲﾔｰ(ﾃﾞｨｽﾌﾟﾚｲﾘｽﾄ)
	if ( pOpt->m_dwUpdateFlg & VIEWUPDATE_DISPLAYLIST ) {
		if ( m_glCode > 0 )
			::glDeleteLists(m_glCode, 1);
		CreateWire();
	}

	// ソリッドモデルの表示
	if ( pOpt->m_dwUpdateFlg & VIEWUPDATE_BOXEL ) {
		ClearVBO();
		// ﾜｰｸ矩形の描画用座標
		if ( pOpt->GetNCViewFlg(NCVIEWFLG_SOLIDVIEW) ) {
			if ( GLEW_ARB_vertex_buffer_object ) {	// OpenGL拡張ｻﾎﾟｰﾄ
				BOOL	bResult;
				// 切削領域の設定
				m_rcDraw = GetDocument()->GetWorkRect();
				if ( GetDocument()->IsNCDocFlag(NCDOC_LATHE) ) {
					// 旋盤用回転モデルの生成
					bResult = CreateLathe();
					if ( bResult && m_nPictureID > 0 ) {
						// ﾃｸｽﾁｬ座標の生成
						CreateTextureLathe();
					}
				}
				else {
					// ﾌﾗｲｽ用ﾎﾞｸｾﾙの生成
					bResult = CreateBoxel();
					if ( bResult && m_nPictureID > 0 )
						CreateTextureMill();
				}
			}
			else {
				ClearTexture();		// 一応ｸﾘｱしておく
				CString	strErr;
				strErr.Format(IDS_ERR_OPENGLVER, ::glGetString(GL_VERSION));
				AfxMessageBox(strErr, MB_OK|MB_ICONEXCLAMATION);
				pOpt->m_bSolidView  = FALSE;	// ﾌﾗｸﾞ強制OFF
				pOpt->m_bG00View    = FALSE;
				pOpt->m_bDragRender = FALSE;
				if ( !pOpt->SaveViewOption() )
					AfxMessageBox(IDS_ERR_REGISTRY, MB_OK|MB_ICONEXCLAMATION);
			}
		}
	}

	::wglMakeCurrent( NULL, NULL );
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
		32,		// Depth   Buffer
		0,		// Stencil Buffer(32)
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

void CNCViewGL::CreateWire(void)
{
	::glGetError();		// ｴﾗｰをﾌﾗｯｼｭ

	m_glCode = ::glGenLists(1);
	if( m_glCode > 0 ) {
		// NCﾃﾞｰﾀ描画のﾃﾞｨｽﾌﾟﾚｲﾘｽﾄ生成
		::glNewList( m_glCode, GL_COMPILE );
			RenderCode();
		::glEndList();
		if ( ::glGetError() != GL_NO_ERROR ) {
			::glDeleteLists(m_glCode, 1);
			m_glCode = 0;
		}
	}
}

BOOL CNCViewGL::CreateBoxel(void)
{
	// ﾎﾞｸｾﾙ生成のための初期設定
	InitialBoxel();
	// 画面いっぱいに描画
	::glMatrixMode(GL_PROJECTION);
	::glPushMatrix();
	::glLoadIdentity();
	::glOrtho(m_rcDraw.left, m_rcDraw.right, m_rcDraw.top, m_rcDraw.bottom,
		m_rcView.low, m_rcView.high);	// m_rcDraw ではｷﾞﾘｷﾞﾘなので m_rcView を使う
	::glMatrixMode(GL_MODELVIEW);
	
	// 切削底面描画
	for ( int i=0; i<GetDocument()->GetNCsize(); i++ )
		GetDocument()->GetNCdata(i)->DrawBottomFace();
//	ASSERT( ::glGetError() == GL_NO_ERROR );
//	GLenum glError = ::glGetError();
	::glFinish();

	// ﾃﾞﾌﾟｽ値の取得
	BOOL	bResult = GetClipDepthMill();
	if ( !bResult )
		ClearVBO();

	// 終了処理
	FinalBoxel();

	return bResult;
}

BOOL CNCViewGL::GetClipDepthMill(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("GetClipDepthMill()");
#endif
	int			i, j, n, nn, nSize;
	GLint		viewPort[4];
	GLdouble	mvMatrix[16], pjMatrix[16],
				wx1, wy1, wz1, wx2, wy2, wz2;
	GLfloat		fz;
	GLfloat*	pfDepth = NULL;		// ﾃﾞﾌﾟｽ値取得配列一時領域
	GLfloat*	pfXYZ = NULL;		// 変換されたﾜｰﾙﾄﾞ座標
	GLfloat*	pfNOR = NULL;		// 法線ﾍﾞｸﾄﾙ

	::glGetIntegerv(GL_VIEWPORT, viewPort);
	::glGetDoublev (GL_MODELVIEW_MATRIX,  mvMatrix);
	::glGetDoublev (GL_PROJECTION_MATRIX, pjMatrix);

	// 矩形領域をﾋﾟｸｾﾙ座標に変換
	::gluProject(m_rcDraw.left,  m_rcDraw.top,    0.0, mvMatrix, pjMatrix, viewPort,
		&wx1, &wy1, &wz1);
	::gluProject(m_rcDraw.right, m_rcDraw.bottom, 0.0, mvMatrix, pjMatrix, viewPort,
		&wx2, &wy2, &wz2);

	m_icx = (int)(wx2 - wx1);
	m_icy = (int)(wy2 - wy1);
	if ( m_icx<=0 || m_icy<=0 )
		return FALSE;

#ifdef _DEBUG
	dbg.printf("left,  top   =(%f, %f)", m_rcDraw.left,  m_rcDraw.top);
	dbg.printf("right, bottom=(%f, %f)", m_rcDraw.right, m_rcDraw.bottom);
	dbg.printf("wx1,   wy1   =(%f, %f)", wx1, wy1);
	dbg.printf("wx2,   wy2   =(%f, %f)", wx2, wy2);
	dbg.printf("m_icx=%d m_icy=%d", m_icx, m_icy);
#endif

	try {
		// 領域確保
		nSize = m_icx * m_icy;
		pfDepth = new GLfloat[nSize];
		nSize *= NCXYZ;
		pfXYZ   = new GLfloat[nSize*2];
		pfNOR   = new GLfloat[nSize*2];
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		if ( pfDepth )
			delete	pfDepth;
		if ( pfXYZ )
			delete	pfXYZ;
		if ( pfNOR )
			delete	pfNOR;
		return FALSE;
	}

	// ｸﾗｲｱﾝﾄ領域のﾃﾞﾌﾟｽ値を取得（ﾋﾟｸｾﾙ単位）
#ifdef _DEBUG
	DWORD	t1 = ::timeGetTime();
#endif
	::glPixelStorei(GL_PACK_ALIGNMENT, 1);
	::glReadPixels((GLint)wx1, (GLint)wy1, m_icx, m_icy,
					GL_DEPTH_COMPONENT, GL_FLOAT, pfDepth);
//	ASSERT( ::glGetError() == GL_NO_ERROR );
//	GLenum glError = ::glGetError();
#ifdef _DEBUG
	DWORD	t2 = ::timeGetTime();
	dbg.printf( "glReadPixels()=%d[ms]", t2 - t1 );
#endif

	// ﾜｰｸ上面と切削面の座標登録
	for ( j=0, n=0, nn=0; j<m_icy; j++ ) {
		for ( i=0; i<m_icx; i++, n++, nn=n*NCXYZ ) {
			::gluUnProject(i+wx1, j+wy1, pfDepth[n],
					mvMatrix, pjMatrix, viewPort,
					&wx2, &wy2, &wz2);	// それほど遅くないので自作変換は中止
			fz = (GLfloat)( (pfDepth[n]==0.0) ?	// ﾃﾞﾌﾟｽ値が初期値(矩形範囲内で切削面でない)なら
				m_rcDraw.high :		// ﾜｰｸ矩形上面座標
				wz2 );				// 変換座標
			// ﾜｰﾙﾄﾞ座標
			pfXYZ[nn+NCA_X] = (GLfloat)wx2;
			pfXYZ[nn+NCA_Y] = (GLfloat)wy2;
			pfXYZ[nn+NCA_Z] = fz;
			// ﾃﾞﾌｫﾙﾄの法線ﾍﾞｸﾄﾙ
			pfNOR[nn+NCA_X] = 0.0;
			pfNOR[nn+NCA_Y] = 0.0;
			pfNOR[nn+NCA_Z] = 1.0;
		}
	}
#ifdef _DEBUG
	DWORD	t3 = ::timeGetTime();
	dbg.printf( "AddMatrix1=%d[ms]", t3 - t2 );
#endif

	// 底面用座標の登録
	fz = (GLfloat)m_rcDraw.low;	// m_rcDraw.low座標で敷き詰める
	for ( j=0; j<m_icy; j++ ) {
		for ( i=0; i<m_icx; i++ ) {
			n  = (j*m_icx+i) * NCXYZ;
			nn = nSize + n;
			pfXYZ[nn+NCA_X] = pfXYZ[n+NCA_X];
			pfXYZ[nn+NCA_Y] = pfXYZ[n+NCA_Y];
			pfXYZ[nn+NCA_Z] = fz;
			pfNOR[nn+NCA_X] = 0.0;
			pfNOR[nn+NCA_Y] = 0.0;
			pfNOR[nn+NCA_Z] = -1.0;
		}
	}
#ifdef _DEBUG
	DWORD	t4 = ::timeGetTime();
	dbg.printf( "AddMatrix2=%d[ms]", t4 - t3 );
#endif

#ifdef _DEBUG_FILEOUT_
	CStdioFile	dbg_f ("C:\\Users\\magara\\Documents\\tmp\\depth_f.csv", CFile::typeText|CFile::modeCreate|CFile::modeWrite);
	CStdioFile	dbg_fx("C:\\Users\\magara\\Documents\\tmp\\depth_x.csv", CFile::typeText|CFile::modeCreate|CFile::modeWrite);
	CStdioFile	dbg_fy("C:\\Users\\magara\\Documents\\tmp\\depth_y.csv", CFile::typeText|CFile::modeCreate|CFile::modeWrite);
	CStdioFile	dbg_fz("C:\\Users\\magara\\Documents\\tmp\\depth_z.csv", CFile::typeText|CFile::modeCreate|CFile::modeWrite);
	CString		r,
				s, sx, sy, sz;
	for ( j=0, n=0; j<m_icy; j++ ) {
		s.Empty();
		sx.Empty();	sy.Empty();	sz.Empty();
		for ( i=0; i<m_icx; i++, n++ ) {
//			if ( pfDepth[n] != 0.0 ) {
				if ( !s.IsEmpty() ) {
					s  += ", ";
//				if ( !sx.IsEmpty() ) {
					sx += ", ";	sy += ", ";	sz += ", ";
				}
				r.Format(IDS_MAKENCD_FORMAT, pfDepth[n]);
				s += r;
				r.Format(IDS_MAKENCD_FORMAT, pfXYZ[n*NCXYZ+NCA_X]);
				sx += r;
				r.Format(IDS_MAKENCD_FORMAT, pfXYZ[n*NCXYZ+NCA_Y]);
				sy += r;
				r.Format(IDS_MAKENCD_FORMAT, pfXYZ[n*NCXYZ+NCA_Z]);
				sz += r;
			}
//		}
		if ( !s.IsEmpty() ) {
			dbg_f.WriteString (s +"\n");
//		if ( !sx.IsEmpty() ) {
			dbg_fx.WriteString(sx+"\n");
			dbg_fy.WriteString(sy+"\n");
			dbg_fz.WriteString(sz+"\n");
		}
	}
#endif	// _DEBUG_FILEOUT_

	delete	pfDepth;

	// 頂点配列ﾊﾞｯﾌｧｵﾌﾞｼﾞｪｸﾄ生成
	BOOL	bResult = CreateVBOMill(pfXYZ, pfNOR);

	delete	pfXYZ;
	delete	pfNOR;

	return bResult;
}

BOOL CNCViewGL::CreateVBOMill(const GLfloat* pfXYZ, GLfloat* pfNOR)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CreateVBOMill()", DBG_BLUE);
#endif

	std::vector<CVElement>	vElementWrk,	// 頂点配列ｲﾝﾃﾞｯｸｽ(可変長２次元配列)
							vElementCut;
	vElementWrk.reserve(m_icy*2);
	vElementCut.reserve(m_icy);

	// ﾜｰｸ上面と切削面を２つのｽﾚｯﾄﾞで処理
	HANDLE	hThread[2];
	CREATEELEMENTPARAM	param[2];
	for ( int i=0; i<SIZEOF(hThread); i++ ) {
		param[i].pfXYZ = pfXYZ;		// const
		param[i].pfNOR = pfNOR;
		param[i].cx = m_icx;
		param[i].cy = m_icy;
		param[i].h  = (GLfloat)m_rcDraw.high;
		param[i].l  = (GLfloat)m_rcDraw.low;
		param[i].bResult = TRUE;
	}
	// 切削面処理の方が相対的に時間がかかるため先にスレッド実行
	param[0].pElement = &vElementCut;
	param[1].pElement = &vElementWrk;
	hThread[0] = ::CreateThread(NULL, 0, CreateElement_CutThread, &param[0], 0, NULL);
	hThread[1] = ::CreateThread(NULL, 0, CreateElement_WrkThread, &param[1], 0, NULL);
	::WaitForMultipleObjects(SIZEOF(hThread), hThread, TRUE, INFINITE);
	::CloseHandle(hThread[0]);
	::CloseHandle(hThread[1]);
	if ( !param[0].bResult || !param[1].bResult )
		return FALSE;

	// ﾜｰｸ側面は法線ﾍﾞｸﾄﾙの更新があり
	// 切削面処理との排他制御を考慮
	if ( !CreateElement_Side(&param[0]) )
		return FALSE;

	::glGetError();		// ｴﾗｰをﾌﾗｯｼｭ

	// 頂点配列をGPUﾒﾓﾘに転送
	if ( m_nVertexID > 0 )
		::glDeleteBuffersARB(1, &m_nVertexID);
	::glGenBuffersARB(1, &m_nVertexID);
	::glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nVertexID);
	::glBufferDataARB(GL_ARRAY_BUFFER_ARB,
			m_icx*m_icy*NCXYZ*2*sizeof(GLfloat), pfXYZ,
			GL_STATIC_DRAW_ARB);
	if ( ::glGetError() != GL_NO_ERROR ) {	// GL_OUT_OF_MEMORY
		::glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		ClearVBO();
		AfxMessageBox(IDS_ERR_OUTOFVRAM, MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	// 法線ﾍﾞｸﾄﾙをGPUﾒﾓﾘに転送
	if ( m_nNormalID > 0 )
		::glDeleteBuffersARB(1, &m_nNormalID);
	::glGenBuffersARB(1, &m_nNormalID);
	::glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nNormalID);
	::glBufferDataARB(GL_ARRAY_BUFFER_ARB,
			m_icx*m_icy*NCXYZ*2*sizeof(GLfloat), pfNOR,
			GL_STATIC_DRAW_ARB);
	if ( ::glGetError() != GL_NO_ERROR ) {
		::glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		ClearVBO();
		AfxMessageBox(IDS_ERR_OUTOFVRAM, MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}
	::glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

	// 頂点ｲﾝﾃﾞｯｸｽをGPUﾒﾓﾘに転送
	if ( m_pGenBuf ) {
		::glDeleteBuffersARB(m_vVertexWrk.size()+m_vVertexCut.size(), m_pGenBuf);
		delete	m_pGenBuf;
	}

	m_vVertexWrk.clear();
	m_vVertexCut.clear();

	try {
		m_pGenBuf = new GLuint[vElementWrk.size()+vElementCut.size()];
		::glGenBuffersARB(vElementWrk.size()+vElementCut.size(), m_pGenBuf);

		size_t	ii, jj;		// 警告防止
#ifdef _DEBUG
		int		dbgTriangleWrk = 0, dbgTriangleCut = 0;
#endif
		// ﾜｰｸ矩形用
		m_vVertexWrk.reserve(vElementWrk.size());
		for ( ii=jj=0; ii<vElementWrk.size(); ii++ ) {
			::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m_pGenBuf[jj++]);
			::glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,
				vElementWrk[ii].size()*sizeof(GLuint), &vElementWrk[ii][0],
				GL_STATIC_DRAW_ARB);
			if ( ::glGetError() != GL_NO_ERROR ) {
				::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
				ClearVBO();
				AfxMessageBox(IDS_ERR_OUTOFVRAM, MB_OK|MB_ICONEXCLAMATION);
				return FALSE;
			}
			m_vVertexWrk.push_back(vElementWrk[ii].size());
#ifdef _DEBUG
			dbgTriangleWrk += vElementWrk[ii].size();
#endif
		}
		// 切削面用
		m_vVertexCut.reserve(vElementCut.size());
		for ( ii=0; ii<vElementCut.size(); ii++ ) {
			::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m_pGenBuf[jj++]);
			::glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,
				vElementCut[ii].size()*sizeof(GLuint), &vElementCut[ii][0],
				GL_STATIC_DRAW_ARB);
			if ( ::glGetError() != GL_NO_ERROR ) {
				::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
				ClearVBO();
				AfxMessageBox(IDS_ERR_OUTOFVRAM, MB_OK|MB_ICONEXCLAMATION);
				return FALSE;
			}
			m_vVertexCut.push_back(vElementCut[ii].size());
#ifdef _DEBUG
			dbgTriangleCut += vElementCut[ii].size();
#endif
		}
		::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);

#ifdef _DEBUG
		dbg.printf("VertexCount(/3)=%d size(*2)=%d",
			m_icx*m_icy*2, m_icx*m_icy*NCXYZ*2*sizeof(GLfloat));
		dbg.printf("Work IndexCount=%d Triangle=%d",
			vElementWrk.size(), dbgTriangleWrk/3);
		dbg.printf("Cut  IndexCount=%d Triangle=%d",
			vElementCut.size(), dbgTriangleCut/3);
#endif
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		return FALSE;
	}

	return TRUE;
}

DWORD WINAPI CreateElement_WrkThread(LPVOID pVoid)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CreateElement_WrkThread()\nStart", DBG_BLUE);
	DWORD	t1 = ::timeGetTime();
#endif

	LPCREATEELEMENTPARAM pParam = reinterpret_cast<LPCREATEELEMENTPARAM>(pVoid);
	int			i, j, cxcy = pParam->cx * pParam->cy;
	BOOL		bReverse, bSingle;
	GLuint		n0, n1,	nn;
	UINT		k0, k1;
	CVElement	vElement;

	vElement.reserve(pParam->cx * 2);

	try {
		// ﾜｰｸ上面
		// 切削面以外は m_rcDraw.high を代入しているので
		// 等しいか否かの判断でOK
		for ( j=0; j<pParam->cy-1; j++ ) {
			bReverse = bSingle = FALSE;
			vElement.clear();
			for ( i=0; i<pParam->cx; i++ ) {
				n0 =  j   *pParam->cx+i;
				n1 = (j+1)*pParam->cx+i;
				k0 = n0*NCXYZ + NCA_Z;
				k1 = n1*NCXYZ + NCA_Z;
				if ( pParam->pfXYZ[k0] != pParam->h ) {
					if ( pParam->pfXYZ[k1] != pParam->h ) {
						// k0:×, k1:×
						if ( vElement.size() > 3 )
							pParam->pElement->push_back(vElement);
						bReverse = bSingle = FALSE;
						vElement.clear();
					}
					else {
						// k0:×, k1:○
						if ( bSingle ) {
							// 片方だけが連続するならそこでbreak
							if ( vElement.size() > 3 )
								pParam->pElement->push_back(vElement);
							bReverse = bSingle = FALSE;
							vElement.clear();
						}
						else {
							vElement.push_back(n1);
							bReverse = FALSE;	// 次はn0から登録
							bSingle  = TRUE;
						}
					}
				}
				else if ( pParam->pfXYZ[k1] != pParam->h ) {
					// k0:○, k1:×
					if ( bSingle ) {
						if ( vElement.size() > 3 )
							pParam->pElement->push_back(vElement);
						bReverse = bSingle = FALSE;
						vElement.clear();
					}
					else {
						vElement.push_back(n0);
						bReverse = TRUE;		// 次はn1から登録
						bSingle  = TRUE;
					}
				}
				else {
					// k0:○, k1:○
					if ( bSingle ) {	// 前回が片方だけ
						// 凹形状防止
						if ( vElement.size() > 3 ) {
							pParam->pElement->push_back(vElement);
							// 前回の座標を再登録
							nn = vElement.back();
							vElement.clear();
							vElement.push_back(nn);
						}
					}
					bSingle = FALSE;
					if ( bReverse ) {
						vElement.push_back(n1);
						vElement.push_back(n0);
					}
					else {
						vElement.push_back(n0);
						vElement.push_back(n1);
					}
				}
			}
			// Ｘ方向のﾙｰﾌﾟが終了
			if ( vElement.size() > 3 )
				pParam->pElement->push_back(vElement);
		}

		// ﾜｰｸ底面
		for ( j=0; j<pParam->cy-1; j++ ) {
			bReverse = bSingle = FALSE;
			vElement.clear();
			for ( i=0; i<pParam->cx; i++ ) {
				n0 =  j   *pParam->cx+i;
				n1 = (j+1)*pParam->cx+i;
				k0 = n0*NCXYZ + NCA_Z;	// ﾜｰｸ上面のZ値で描画判断
				k1 = n1*NCXYZ + NCA_Z;
				n0 += cxcy;				// 底面座標番号
				n1 += cxcy;
				if ( pParam->pfXYZ[k0] < pParam->l ) {
					if ( pParam->pfXYZ[k1] < pParam->l ) {
						// k0:×, k1:×
						if ( vElement.size() > 3 )
							pParam->pElement->push_back(vElement);
						bReverse = bSingle = FALSE;
						vElement.clear();
					}
					else {
						// k0:×, k1:○
						if ( bSingle ) {
							// 片方だけが連続するならそこでbreak
							if ( vElement.size() > 3 )
								pParam->pElement->push_back(vElement);
							bReverse = bSingle = FALSE;
							vElement.clear();
						}
						else {
							vElement.push_back(n1);
							bReverse = FALSE;	// 次はn0から登録
							bSingle  = TRUE;
						}
					}
				}
				else if ( pParam->pfXYZ[k1] < pParam->l ) {
					// k0:○, k1:×
					if ( bSingle ) {
						if ( vElement.size() > 3 )
							pParam->pElement->push_back(vElement);
						bReverse = bSingle = FALSE;
						vElement.clear();
					}
					else {
						vElement.push_back(n0);
						bReverse = TRUE;		// 次はn1から登録
						bSingle  = TRUE;
					}
				}
				else {
					// k0:○, k1:○
					if ( bSingle ) {	// 前回が片方だけ
						// 凹形状防止
						if ( vElement.size() > 3 ) {
							pParam->pElement->push_back(vElement);
							// 前回の座標を再登録
							nn = vElement.back();
							vElement.clear();
							vElement.push_back(nn);
						}
					}
					bSingle = FALSE;
					if ( bReverse ) {
						vElement.push_back(n1);
						vElement.push_back(n0);
					}
					else {
						vElement.push_back(n0);
						vElement.push_back(n1);
					}
				}
			}
			// Ｘ方向のﾙｰﾌﾟが終了
			if ( vElement.size() > 3 )
				pParam->pElement->push_back(vElement);
		}
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		pParam->bResult = FALSE;
	}
#ifdef _DEBUG
	DWORD	t2 = ::timeGetTime();
	dbg.printf("end %d[ms]", t2 - t1);
#endif

	return 0;
}

DWORD WINAPI CreateElement_CutThread(LPVOID pVoid)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CreateElement_CutThread()\nStart", DBG_BLUE);
	DWORD	t1 = ::timeGetTime();
#endif

	LPCREATEELEMENTPARAM pParam = reinterpret_cast<LPCREATEELEMENTPARAM>(pVoid);
	int			i, j;
	BOOL		bWorkrct,	// 前回××かどうかを判断するﾌﾗｸﾞ
				bThrough;	// 前回△△　　〃
	GLuint		n0, n1;
	UINT		k0, k1, kk0, kk1;
	GLfloat		k0z, k1z;
	float		q;
	CVElement	vElement;

	vElement.reserve(pParam->cx * 2);

	try {
		// 切削面（上面・側面兼用）
		// ○：切削面，△：貫通，×：ﾜｰｸ上面
		for ( j=0; j<pParam->cy-1; j++ ) {
			bWorkrct = bThrough = FALSE;
			vElement.clear();
			for ( i=0; i<pParam->cx; i++ ) {
				n0  =  j   *pParam->cx+i;
				n1  = (j+1)*pParam->cx+i;
				k0  = n0*NCXYZ;
				k1  = n1*NCXYZ;
				k0z = pParam->pfXYZ[k0+NCA_Z];
				k1z = pParam->pfXYZ[k1+NCA_Z];
				if ( k0z >= pParam->h ) {
					if ( k1z >= pParam->h ) {
						// k0:×, k1:×
						if ( bWorkrct ) {
							// 前回も k0:×, k1:×
							if ( vElement.size() > 3 ) {
								// break
								pParam->pElement->push_back(vElement);
								// 前回の法線を左向き(終点)に変更
								pParam->pfNOR[kk0+NCA_X] = -1.0;
								pParam->pfNOR[kk0+NCA_Z] = 0;
								pParam->pfNOR[kk1+NCA_X] = -1.0;
								pParam->pfNOR[kk1+NCA_Z] = 0;
							}
							vElement.clear();
						}
						bWorkrct = TRUE;
					}
					else {
						// k0:×, k1:△
						// k0:×, k1:○
						if ( bWorkrct ) {
							// 前回 k0:×, k1:× なら
							// 前回の法線を右向き(始点)に変更
							pParam->pfNOR[kk0+NCA_X] = 1.0;
							pParam->pfNOR[kk0+NCA_Z] = 0;
							pParam->pfNOR[kk1+NCA_X] = 1.0;
							pParam->pfNOR[kk1+NCA_Z] = 0;
						}
						// k0をY方向上向きの法線に
						pParam->pfNOR[k0+NCA_Y] = 1.0;
						pParam->pfNOR[k0+NCA_Z] = 0;
						if ( k1z < pParam->l ) {
							// k1（△：切れ目側）をY方向下向きの法線
							pParam->pfNOR[k1+NCA_Y] = -1.0;
							pParam->pfNOR[k1+NCA_Z] = 0;
						}
						bWorkrct = FALSE;
					}
					bThrough = FALSE;
				}
				else if ( k0z < pParam->l ) {
					// k0:△
					if ( bWorkrct ) {
						// 前回 k0:×, k1:×
						// 前回の法線を右向き(始点)に変更
						pParam->pfNOR[kk0+NCA_X] = 1.0;
						pParam->pfNOR[kk0+NCA_Z] = 0;
						pParam->pfNOR[kk1+NCA_X] = 1.0;
						pParam->pfNOR[kk1+NCA_Z] = 0;
					}
					bWorkrct = FALSE;
					if ( k1z < pParam->l ) {
						// k0:△, k1:△
						if ( bThrough ) {
							// 前回も k0:△, k1:△
							if ( vElement.size() > 3 ) {
								// break
								pParam->pElement->push_back(vElement);
								// 前回の法線を左向き(切れ目)に変更
								pParam->pfNOR[kk0+NCA_X] = -1.0;
								pParam->pfNOR[kk0+NCA_Z] = 0;
								pParam->pfNOR[kk1+NCA_X] = -1.0;
								pParam->pfNOR[kk1+NCA_Z] = 0;
							}
							vElement.clear();
						}
						bThrough = TRUE;
					}
					else {
						// k0:△, k1:×
						// k0:△, k1:○
						bThrough = FALSE;
						// k0を下向き, k1を上向きの法線
						pParam->pfNOR[k0+NCA_Y] = -1.0;
						pParam->pfNOR[k0+NCA_Z] = 0;
						pParam->pfNOR[k1+NCA_Y] = 1.0;
						pParam->pfNOR[k1+NCA_Z] = 0;
					}
				}
				else {
					// k0:○
					if ( bWorkrct ) {
						// 前回 k0:×, k1:×
						// 前回の法線を右向きに変更
						pParam->pfNOR[kk0+NCA_X] = 1.0;
						pParam->pfNOR[kk0+NCA_Z] = 0;
						pParam->pfNOR[kk1+NCA_X] = 1.0;
						pParam->pfNOR[kk1+NCA_Z] = 0;
					}
					if ( bThrough ) {
						// 前回 k0:△, k1:△
						// 前回の法線を左向きに変更
						pParam->pfNOR[kk0+NCA_X] = -1.0;
						pParam->pfNOR[kk0+NCA_Z] = 0;
						pParam->pfNOR[kk1+NCA_X] = -1.0;
						pParam->pfNOR[kk1+NCA_Z] = 0;
					}
					bWorkrct = bThrough = FALSE;
					if ( k1z >= pParam->h ) {
						// k0:○, k1:×
						pParam->pfNOR[k1+NCA_Y] = -1.0;		// 下向きの法線
						pParam->pfNOR[k1+NCA_Z] = 0;
					}
					else if ( k1z < pParam->l ) {
						// k0:○, k1:△
						pParam->pfNOR[k1+NCA_Y] = 1.0;		// 上向きの法線
						pParam->pfNOR[k1+NCA_Z] = 0;
					}
					else {
						// YZ平面での法線ﾍﾞｸﾄﾙ計算
						q = atan2f(k1z - k0z, 
							pParam->pfXYZ[k1+NCA_Y] - pParam->pfXYZ[k0+NCA_Y]);
					//	CPointD	pt(0, 1);
					//	pt.RoundPoint(q);
					//	pParam->pfNOR[k0+NCA_X] = pt.x;
					//	pParam->pfNOR[k0+NCA_Z] = pt.y;
						pParam->pfNOR[k1+NCA_Y] = -sinf(q);	// 式の簡略化
						pParam->pfNOR[k1+NCA_Z] =  cosf(q);
					}
					// k0に対する法線計算
					if ( i>0 && pParam->pfXYZ[kk0+NCA_Z] >= pParam->l ) {
						// 前回の座標との角度を求め
						// XZ平面での法線ﾍﾞｸﾄﾙを計算
						q = atan2f(k0z - pParam->pfXYZ[kk0+NCA_Z],
							pParam->pfXYZ[k0+NCA_X] - pParam->pfXYZ[kk0+NCA_X]);
						pParam->pfNOR[k0+NCA_X] = -sinf(q);
						pParam->pfNOR[k0+NCA_Z] =  cosf(q);
					}
				}
				// 全座標をつなぐ
				// ××|△△が連続する所だけclear
				vElement.push_back(n1);
				vElement.push_back(n0);
				// 
				kk0 = k0;
				kk1 = k1;
			}
			// Ｘ方向のﾙｰﾌﾟが終了
			if ( vElement.size() > 3 )
				pParam->pElement->push_back(vElement);
		}
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		pParam->bResult = FALSE;
	}

#ifdef _DEBUG
	DWORD	t2 = ::timeGetTime();
	dbg.printf("end %d[ms]", t2 - t1);
#endif

	return 0;
}

BOOL CreateElement_Side(LPCREATEELEMENTPARAM pParam)
{
	int			i, j;
	GLuint		n0, n1,
				nn = pParam->cy * pParam->cx;	// 底面座標へのｵﾌｾｯﾄ
	UINT		k0, k1;
	GLfloat		nor = -1.0;
	CVElement	vElement;

	vElement.reserve(pParam->cx * 2);

	try {
		// ﾜｰｸ矩形側面（X方向手前と奥）
		for ( j=0; j<pParam->cy; j+=pParam->cy-1 ) {	// 0とm_icy-1
			vElement.clear();
			for ( i=0; i<pParam->cx; i++ ) {
				n0 = j*pParam->cx+i;	// 上面座標
				n1 = nn + n0;			// 底面座標
				k0 = n0*NCXYZ;
				k1 = n1*NCXYZ;
				if ( pParam->l >= pParam->pfXYZ[k0+NCA_Z] ) {
					if ( vElement.size() > 3 )
						pParam->pElement->push_back(vElement);
					vElement.clear();
				}
				else {
					vElement.push_back(n0);
					vElement.push_back(n1);
				}
				// 法線ﾍﾞｸﾄﾙの修正
				pParam->pfNOR[k0+NCA_Y] = nor;	// 上or下向きの法線
				pParam->pfNOR[k0+NCA_Z] = 0;
				pParam->pfNOR[k1+NCA_Y] = nor;
				pParam->pfNOR[k1+NCA_Z] = 0;
			}
			if ( vElement.size() > 3 )
				pParam->pElement->push_back(vElement);
			nor *= -1.0;	// 符号反転
		}
		// ﾜｰｸ矩形側面（Y方向左と右）
		for ( i=0; i<pParam->cx; i+=pParam->cx-1 ) {
			vElement.clear();
			for ( j=0; j<pParam->cy; j++ ) {
				n0 = j*pParam->cx+i;
				n1 = nn + n0;
				k0 = n0*NCXYZ;
				k1 = n1*NCXYZ;
				if ( pParam->l >= pParam->pfXYZ[k0+NCA_Z] ) {
					if ( vElement.size() > 3 )
						pParam->pElement->push_back(vElement);
					vElement.clear();
				}
				else {
					vElement.push_back(n0);
					vElement.push_back(n1);
				}
				pParam->pfNOR[k0+NCA_X] = nor;	// 左or右向きの法線
				pParam->pfNOR[k0+NCA_Z] = 0;
				pParam->pfNOR[k1+NCA_X] = nor;
				pParam->pfNOR[k1+NCA_Z] = 0;
			}
			if ( vElement.size() > 3 )
				pParam->pElement->push_back(vElement);
			nor *= -1.0;
		}
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		return FALSE;
	}
	return TRUE;
}

BOOL CNCViewGL::CreateLathe(void)
{
	// ﾎﾞｸｾﾙ生成のための初期設定
	InitialBoxel();
	// 画面いっぱいに描画
	::glMatrixMode(GL_PROJECTION);
	::glPushMatrix();
	::glLoadIdentity();
	::glOrtho(m_rcDraw.left, m_rcDraw.right,
		m_rcView.low, m_rcView.high,	// top と bottom は使用不可
		m_rcView.low, m_rcView.high);	// m_rcDraw ではｷﾞﾘｷﾞﾘなので m_rcView を使う
	::glMatrixMode(GL_MODELVIEW);

	// 旋盤用ZXﾜｲﾔｰの描画
	::glPushAttrib( GL_LINE_BIT );
	::glLineWidth( 3.0 );	// 1Pixelではﾃﾞﾌﾟｽ値を拾えないかもしれないので
	for ( int i=0; i<GetDocument()->GetNCsize(); i++ )
		GetDocument()->GetNCdata(i)->DrawLatheWire();
	::glPopAttrib();
	::glFinish();

	// ﾃﾞﾌﾟｽ値の取得
	BOOL	bResult = GetClipDepthLathe();
	if ( !bResult )
		ClearVBO();

	// 終了処理
	FinalBoxel();

	return bResult;
}

BOOL CNCViewGL::GetClipDepthLathe(void)
{
#ifdef _DEBUG
	CMagaDbg	dbg("GetClipDepthLathe()");
#endif
	int			i, j, ii, jj;
	double		q;
	GLint		viewPort[4];
	GLdouble	mvMatrix[16], pjMatrix[16],
				wx1, wy1, wz1, wx2, wy2, wz2;
	GLfloat		fz, fx, fxb;
	boost::optional<GLfloat>	fzb;
	GLfloat*	pfDepth = NULL;		// ﾃﾞﾌﾟｽ値取得配列一時領域
	GLfloat*	pfCylinder = NULL;	// 円筒形座標
	GLfloat*	pfNOR = NULL;		// 法線ﾍﾞｸﾄﾙ

	::glGetIntegerv(GL_VIEWPORT, viewPort);
	::glGetDoublev (GL_MODELVIEW_MATRIX,  mvMatrix);
	::glGetDoublev (GL_PROJECTION_MATRIX, pjMatrix);

	// 矩形領域(left/right)をﾋﾟｸｾﾙ座標に変換
	::gluProject(m_rcDraw.left,  0.0, 0.0, mvMatrix, pjMatrix, viewPort,
		&wx1, &wy1, &wz1);
	::gluProject(m_rcDraw.right, 0.0, 0.0, mvMatrix, pjMatrix, viewPort,
		&wx2, &wy2, &wz2);

	m_icx = (int)(wx2 - wx1);
	m_icy = 0;
	if ( m_icx <= 0 )
		return FALSE;

#ifdef _DEBUG
	dbg.printf("left  -> wx1 = %f -> %f", m_rcDraw.left, wx1);
	dbg.printf("right -> wx2 = %f -> %f", m_rcDraw.right, wx2);
	dbg.printf("wy1 = %f", wy1);
	dbg.printf("m_icx=%d", m_icx);
#endif

	try {
		// 領域確保
		pfDepth    = new GLfloat[m_icx];
		pfCylinder = new GLfloat[m_icx*(ARCCOUNT+1)*NCXYZ];
		pfNOR      = new GLfloat[m_icx*(ARCCOUNT+1)*NCXYZ];
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		if ( pfDepth )
			delete	pfDepth;
		if ( pfCylinder )
			delete	pfCylinder;
		if ( pfNOR )
			delete	pfNOR;
		return FALSE;
	}

	// ｸﾗｲｱﾝﾄ領域のﾃﾞﾌﾟｽ値を取得（ﾋﾟｸｾﾙ単位）
#ifdef _DEBUG
	DWORD	t1 = ::timeGetTime();
#endif
	::glPixelStorei(GL_PACK_ALIGNMENT, 1);
	::glReadPixels((GLint)wx1, (GLint)wy1, m_icx, 1,
					GL_DEPTH_COMPONENT, GL_FLOAT, pfDepth);
//	ASSERT( ::glGetError() == GL_NO_ERROR );
//	GLenum glError = ::glGetError();
#ifdef _DEBUG
	DWORD	t2 = ::timeGetTime();
	dbg.printf( "glReadPixels()=%d[ms]", t2 - t1 );
#endif

	// 外径切削面の座標登録
	for ( i=0; i<m_icx; i++ ) {
		::gluUnProject(i+wx1, wy1, pfDepth[i],
				mvMatrix, pjMatrix, viewPort,
				&wx2, &wy2, &wz2);	// それほど遅くないので自作変換は中止
		fz = (GLfloat)fabs( (pfDepth[i]==0.0) ?	// ﾃﾞﾌﾟｽ値が初期値(矩形範囲内で切削面でない)なら
				m_rcDraw.high :				// ﾜｰｸ半径値
				min(wz2, m_rcDraw.high) );	// 変換座標(==半径)かﾜｰｸ半径の小さい方
		ii = i * (ARCCOUNT+1);
		// ﾃｰﾊﾟｰ状の法線ﾍﾞｸﾄﾙを計算
		if ( fzb && fz != *fzb ) {
			q = fz - *fzb;
			fx = cos( atan2(q, wx2 - fxb) + 90.0*RAD );
		}
		else
			fx = 0;
		// fz を半径に円筒形の座標を生成
		for ( j=0, q=0; j<=ARCCOUNT; j++, q+=ARCSTEP ) {
			jj = (ii + j) * NCXYZ;
			// ﾃﾞﾌｫﾙﾄの法線ﾍﾞｸﾄﾙ
			pfNOR[jj+NCA_X] = fx;
			pfNOR[jj+NCA_Y] = cos(q);
			pfNOR[jj+NCA_Z] = sin(q);
			// ﾜｰﾙﾄﾞ座標
			pfCylinder[jj+NCA_X] = wx2;
			pfCylinder[jj+NCA_Y] = fz * pfNOR[jj+NCA_Y];
			pfCylinder[jj+NCA_Z] = fz * pfNOR[jj+NCA_Z];
		}
		// 前回値を保存
		fzb = fz;
		fxb = wx2;
	}
#ifdef _DEBUG
	DWORD	t3 = ::timeGetTime();
	dbg.printf( "AddMatrix1=%d[ms]", t3 - t2 );
#endif

	// 頂点配列ﾊﾞｯﾌｧｵﾌﾞｼﾞｪｸﾄ生成
	BOOL	bResult = CreateVBOLathe(pfDepth, pfCylinder, pfNOR);

	delete	pfDepth;
	delete	pfCylinder;
	delete	pfNOR;

	return bResult;
}

BOOL CNCViewGL::CreateVBOLathe
	(const GLfloat* pfDepth, const GLfloat* pfCylinder, const GLfloat* pfNOR)
{
#ifdef _DEBUG
	CMagaDbg	dbg("CreateVBOLathe()", DBG_BLUE);
#endif
	int		i, j;
	GLuint	n0, n1;
	std::vector<CVElement>	vElementWrk,	// 頂点配列ｲﾝﾃﾞｯｸｽ(可変長２次元配列)
							vElementCut;
	CVElement	vElement;

	vElement.reserve((ARCCOUNT+1)*2);
	vElementWrk.reserve(m_icx);
	vElementCut.reserve(m_icx);

	// iとi+1の座標を円筒形につなげる
	for ( i=0; i<m_icx-1; i++ ) {
		vElement.clear();
		for ( j=0; j<=ARCCOUNT; j++ ) {
			n0 =  i    * (ARCCOUNT+1) + j;
			n1 = (i+1) * (ARCCOUNT+1) + j;
			vElement.push_back(n0);
			vElement.push_back(n1);
		}
		if ( pfDepth[i+1] == 0.0 )	// 切削面かﾜｰｸ面か
			vElementWrk.push_back(vElement);
		else
			vElementCut.push_back(vElement);
	}

	::glGetError();		// ｴﾗｰをﾌﾗｯｼｭ

	// 頂点配列をGPUﾒﾓﾘに転送
	if ( m_nVertexID > 0 )
		::glDeleteBuffersARB(1, &m_nVertexID);
	::glGenBuffersARB(1, &m_nVertexID);
	::glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nVertexID);
	::glBufferDataARB(GL_ARRAY_BUFFER_ARB,
			m_icx*(ARCCOUNT+1)*NCXYZ*sizeof(GLfloat), pfCylinder,
			GL_STATIC_DRAW_ARB);
	if ( ::glGetError() != GL_NO_ERROR ) {	// GL_OUT_OF_MEMORY
		::glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		ClearVBO();
		AfxMessageBox(IDS_ERR_OUTOFVRAM, MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	// 法線ﾍﾞｸﾄﾙをGPUﾒﾓﾘに転送
	if ( m_nNormalID > 0 )
		::glDeleteBuffersARB(1, &m_nNormalID);
	::glGenBuffersARB(1, &m_nNormalID);
	::glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nNormalID);
	::glBufferDataARB(GL_ARRAY_BUFFER_ARB,
			m_icx*(ARCCOUNT+1)*NCXYZ*sizeof(GLfloat), pfNOR,
			GL_STATIC_DRAW_ARB);
	if ( ::glGetError() != GL_NO_ERROR ) {
		::glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		ClearVBO();
		AfxMessageBox(IDS_ERR_OUTOFVRAM, MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}
	::glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

	// 頂点ｲﾝﾃﾞｯｸｽをGPUﾒﾓﾘに転送
	if ( m_pGenBuf ) {
		::glDeleteBuffersARB(m_vVertexWrk.size()+m_vVertexCut.size(), m_pGenBuf);
		delete	m_pGenBuf;
	}

	m_vVertexWrk.clear();
	m_vVertexCut.clear();

	try {
		m_pGenBuf = new GLuint[vElementWrk.size()+vElementCut.size()];
		::glGenBuffersARB(vElementWrk.size()+vElementCut.size(), m_pGenBuf);

		size_t	ii, jj;		// 警告防止
#ifdef _DEBUG
		int		dbgTriangleWrk = 0, dbgTriangleCut = 0;
#endif
		// ﾜｰｸ矩形用
		m_vVertexWrk.reserve(vElementWrk.size());
		for ( ii=jj=0; ii<vElementWrk.size(); ii++ ) {
			::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m_pGenBuf[jj++]);
			::glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,
				vElementWrk[ii].size()*sizeof(GLuint), &vElementWrk[ii][0],
				GL_STATIC_DRAW_ARB);
			if ( ::glGetError() != GL_NO_ERROR ) {
				::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
				ClearVBO();
				AfxMessageBox(IDS_ERR_OUTOFVRAM, MB_OK|MB_ICONEXCLAMATION);
				return FALSE;
			}
			m_vVertexWrk.push_back(vElementWrk[ii].size());
#ifdef _DEBUG
			dbgTriangleWrk += vElementWrk[ii].size();
#endif
		}
		// 切削面用
		m_vVertexCut.reserve(vElementCut.size());
		for ( ii=0; ii<vElementCut.size(); ii++ ) {
			::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m_pGenBuf[jj++]);
			::glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,
				vElementCut[ii].size()*sizeof(GLuint), &vElementCut[ii][0],
				GL_STATIC_DRAW_ARB);
			if ( ::glGetError() != GL_NO_ERROR ) {
				::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
				ClearVBO();
				AfxMessageBox(IDS_ERR_OUTOFVRAM, MB_OK|MB_ICONEXCLAMATION);
				return FALSE;
			}
			m_vVertexCut.push_back(vElementCut[ii].size());
#ifdef _DEBUG
			dbgTriangleCut += vElementCut[ii].size();
#endif
		}
		::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);

#ifdef _DEBUG
		dbg.printf("VertexCount(/3)=%d size=%d",
			m_icx*(ARCCOUNT+1), m_icx*(ARCCOUNT+1)*NCXYZ*sizeof(GLfloat));
		dbg.printf("Work IndexCount=%d Triangle=%d",
			vElementWrk.size(), dbgTriangleWrk/3);
		dbg.printf("Cut  IndexCount=%d Triangle=%d",
			vElementCut.size(), dbgTriangleCut/3);
#endif
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		return FALSE;
	}

	return TRUE;
}

BOOL CNCViewGL::ReadTexture(LPCTSTR szFileName)
{
	// ﾌｧｲﾙ名のUNICODE変換
	WCHAR	wszFileName[_MAX_PATH];
	::MultiByteToWideChar(CP_ACP, 0, szFileName, -1, wszFileName, _MAX_PATH);

	// 画像の読み込み
	Gdiplus::Bitmap	bmp(wszFileName);
	if ( bmp.GetLastStatus() != Gdiplus::Ok ) {
		ClearTexture();
		AfxMessageBox(IDS_ERR_TEXTURE, MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}
	// 画像のﾋﾟｸｾﾙを取得
	Gdiplus::BitmapData	bmpdata;
	bmp.LockBits(NULL, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bmpdata);

	// ﾃｸｽﾁｬの生成
	::glGenTextures(1, &m_nPictureID);
	::glBindTexture(GL_TEXTURE_2D, m_nPictureID);
	::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	// ﾃｸｽﾁｬにﾋﾟｸｾﾙを書き込む(2^n サイズ以外の対応)
	::gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, bmpdata.Width, bmpdata.Height,
		GL_BGRA_EXT, GL_UNSIGNED_BYTE, bmpdata.Scan0);

	// 後始末
	::glBindTexture(GL_TEXTURE_2D, 0);
	bmp.UnlockBits(&bmpdata);

	return TRUE;
}

void CNCViewGL::CreateTextureMill(void)
{
	int			i, j, n;
	GLfloat		ft;
	GLfloat*	pfTEX;

	if ( m_icx<=0 || m_icy<=0 )
		return;

	try {
		pfTEX = new GLfloat[m_icx*m_icy*2*2];	// (X,Y) 上面と底面
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		ClearTexture();
		return;
	}

	// 上面用ﾃｸｽﾁｬ座標
	for ( j=0, n=0; j<m_icy; j++ ) {
		ft = (GLfloat)(m_icy-j)/(m_icy-1);
		for ( i=0; i<m_icx; i++, n+=2 ) {
			// ﾃｸｽﾁｬ座標(0.0～1.0)
			pfTEX[n]   = (GLfloat)i/(m_icx-1);
			pfTEX[n+1] = ft;
		}
	}

	// 底面用ﾃｸｽﾁｬ座標
	for ( j=0; j<m_icy; j++ ) {
		ft = (GLfloat)j/(m_icy-1);
		for ( i=0; i<m_icx; i++, n+=2 ) {
			pfTEX[n]   = (GLfloat)i/(m_icx-1);
			pfTEX[n+1] = ft;
		}
	}

	// ﾃｸｽﾁｬ座標をGPUﾒﾓﾘに転送
	CreateTexture(m_icx*m_icy*2*2, pfTEX);

	delete	pfTEX;
}

void CNCViewGL::CreateTextureLathe(void)
{
	int			i, j, n;
	GLfloat		ft;
	GLfloat*	pfTEX;

	if ( m_icx<=0 )
		return;

	try {
		pfTEX = new GLfloat[m_icx*(ARCCOUNT+1)*2];	// Xと円筒座標分
	}
	catch (CMemoryException* e) {
		AfxMessageBox(IDS_ERR_OUTOFMEM, MB_OK|MB_ICONSTOP);
		e->Delete();
		ClearTexture();
		return;
	}

	// ﾃｸｽﾁｬ座標の割り当て
	for ( i=0, n=0; i<m_icx; i++ ) {
		ft = (GLfloat)i/(m_icx-1);
		for ( j=0; j<=ARCCOUNT; j++, n+=2 ) {
			// ﾃｸｽﾁｬ座標(0.0～1.0)
			pfTEX[n]   = (GLfloat)j/ARCCOUNT;
			pfTEX[n+1] = ft;
		}
	}

	// ﾃｸｽﾁｬ座標をGPUﾒﾓﾘに転送
	CreateTexture(m_icx*(ARCCOUNT+1)*2, pfTEX);

	delete	pfTEX;
}

void  CNCViewGL::CreateTexture(GLsizeiptr n, const GLfloat* pfTEX)
{
	// ﾃｸｽﾁｬ座標をGPUﾒﾓﾘに転送
	if ( m_nTextureID > 0 )
		::glDeleteBuffersARB(1, &m_nTextureID);
	::glGenBuffersARB(1, &m_nTextureID);
	::glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nTextureID);
	::glBufferDataARB(GL_ARRAY_BUFFER_ARB,
			n*sizeof(GLfloat), pfTEX,
			GL_STATIC_DRAW_ARB);
	if ( ::glGetError() != GL_NO_ERROR ) {	// GL_OUT_OF_MEMORY
		ClearTexture();
		AfxMessageBox(IDS_ERR_OUTOFVRAM, MB_OK|MB_ICONEXCLAMATION);
	}
	::glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
}

void CNCViewGL::ClearVBO(void)
{
	if ( m_nVertexID > 0 )
		::glDeleteBuffersARB(1, &m_nVertexID);
	if ( m_nNormalID > 0 )
		::glDeleteBuffersARB(1, &m_nNormalID);
	m_nVertexID = m_nNormalID = 0;
	if ( m_pGenBuf ) {
		::glDeleteBuffersARB(m_vVertexWrk.size()+m_vVertexCut.size(), m_pGenBuf);
		delete	m_pGenBuf;
		m_pGenBuf = NULL;
	}
	m_vVertexWrk.clear();
	m_vVertexCut.clear();
}

void CNCViewGL::ClearTexture(void)
{
	if ( m_nPictureID > 0 )
		::glDeleteTextures(1, &m_nPictureID);
	if ( m_nTextureID > 0 )
		::glDeleteBuffersARB(1, &m_nTextureID);
	m_nPictureID = m_nTextureID = 0;
}

void CNCViewGL::InitialBoxel(void)
{
	// ﾎﾞｸｾﾙ生成のための初期設定
	::glEnable(GL_DEPTH_TEST);
	::glClearDepth(0.0);		// 遠い方を優先させるためのﾃﾞﾌﾟｽ初期値
	::glDepthFunc(GL_GREATER);	// 遠い方を優先
	::glClear(GL_DEPTH_BUFFER_BIT);		// ﾃﾞﾌﾟｽﾊﾞｯﾌｧのみｸﾘｱ
	::glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);	// ｶﾗｰﾏｽｸOFF
}

void CNCViewGL::FinalBoxel(void)
{
	::glMatrixMode(GL_PROJECTION);
	::glPopMatrix();
	::glMatrixMode(GL_MODELVIEW);

	// 通常設定に戻す
	::glClearDepth(1.0);
	::glDepthFunc(GL_LESS);		// 近い方を優先(通常描画)
	::glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
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
	dVertex[2] = m_rcView.low;		// ﾃﾞﾌﾟｽﾃｽﾄ無効で描画
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

	::glPushAttrib( GL_LINE_BIT );	// 線情報
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

	::glPopAttrib();	//	::glDisable(GL_LINE_STIPPLE)は popにより不要
}

void CNCViewGL::RenderCode(void)
{
	::glEnable( GL_LINE_STIPPLE );
	// NCﾃﾞｰﾀ描画
	for ( int i=0; i<GetDocument()->GetNCsize(); i++ )
		GetDocument()->GetNCdata(i)->DrawWire();
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
	Invalidate(FALSE);
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
	static_cast<CNCChild *>(GetParentFrame())->SetFactorInfo(m_dRate/LOGPIXEL, m_strGuide);
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
#ifdef _DEBUG
	CMagaDbg	dbg("OnDraw()");
#endif
	ASSERT_VALID(GetDocument());
	const CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();

	// ｶﾚﾝﾄｺﾝﾃｷｽﾄの割り当て
	::wglMakeCurrent( pDC->GetSafeHdc(), m_hRC );

	::glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	::glDisable(GL_DEPTH_TEST);

	// 背景の描画
	RenderBack();
	// 軸の描画
	::glEnable(GL_DEPTH_TEST);
	RenderAxis();

#ifdef _DEBUG_DRAWTEST_
	for ( int i=0; i<GetDocument()->GetNCsize(); i++ )
		GetDocument()->GetNCdata(i)->DrawBottomFace();
//		GetDocument()->GetNCdata(1)->DrawBottomFace();
	if ( m_nPictureID > 0 ) {
//		COLORREF col = pOpt->GetNcDrawColor(NCCOL_GL_WRK);
		::glEnable(GL_LIGHTING);
		::glEnable (GL_LIGHT0);
		::glEnable (GL_LIGHT1);
		::glDisable(GL_LIGHT2);
		::glDisable(GL_LIGHT3);
		::glEnable(GL_TEXTURE_2D);
		::glBindTexture(GL_TEXTURE_2D, m_nPictureID);
		::glBegin(GL_QUADS);
//		::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
//		::glColor3ub( 255, 255, 255 );
		::glTexCoord2d(0, 0);
		::glVertex3d(m_rcDraw.left,  m_rcDraw.bottom, m_rcDraw.low);
		::glTexCoord2d(0, 1);
		::glVertex3d(m_rcDraw.left,  m_rcDraw.top,    m_rcDraw.low);
		::glTexCoord2d(1, 1);
		::glVertex3d(m_rcDraw.right, m_rcDraw.top,    m_rcDraw.low);
		::glTexCoord2d(1, 0);
		::glVertex3d(m_rcDraw.right, m_rcDraw.bottom, m_rcDraw.low);
		::glEnd();
		::glBindTexture(GL_TEXTURE_2D, 0);
		::glDisable(GL_TEXTURE_2D);
		::glDisable(GL_LIGHTING);
	}
#else
	if ( pOpt->GetNCViewFlg(NCVIEWFLG_SOLIDVIEW) && m_nVertexID > 0 &&
		(pOpt->GetNCViewFlg(NCVIEWFLG_DRAGRENDER) || m_enTrackingMode==TM_NONE) ) {
		::glEnable(GL_LIGHTING);
		// 線画が正しく表示されるためにﾎﾟﾘｺﾞﾝｵﾌｾｯﾄ
		::glEnable(GL_POLYGON_OFFSET_FILL);
		::glPolygonOffset(1.0f, 1.0f);
		// 頂点ﾊﾞｯﾌｧｵﾌﾞｼﾞｪｸﾄによるﾎﾞｸｾﾙ描画
		::glEnableClientState(GL_VERTEX_ARRAY);
		::glEnableClientState(GL_NORMAL_ARRAY);
		::glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nVertexID);
		::glVertexPointer(NCXYZ, GL_FLOAT, 0, NULL);
		::glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nNormalID);
		::glNormalPointer(GL_FLOAT, 0, NULL);
		if ( pOpt->GetNCViewFlg(NCVIEWFLG_TEXTURE) && m_nTextureID > 0 ) {
			::glBindTexture(GL_TEXTURE_2D, m_nPictureID);
			::glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			::glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nTextureID);
			::glTexCoordPointer(2, GL_FLOAT, 0, NULL);
			::glEnable(GL_TEXTURE_2D);
		}
		// ﾜｰｸ矩形
		::glEnable (GL_LIGHT0);
		::glEnable (GL_LIGHT1);
		::glDisable(GL_LIGHT2);
		::glDisable(GL_LIGHT3);
		size_t	i, j;
		for ( i=j=0; i<m_vVertexWrk.size(); i++, j++ ) {
			::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m_pGenBuf[j]);
			::glDrawElements(GL_TRIANGLE_STRIP, m_vVertexWrk[i], GL_UNSIGNED_INT, NULL);
		}
		// 切削面
		::glDisable(GL_LIGHT0);
		::glDisable(GL_LIGHT1);
		::glEnable (GL_LIGHT2);
		::glEnable (GL_LIGHT3);
		for ( i=0; i<m_vVertexCut.size(); i++, j++ ) {
			::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m_pGenBuf[j]);
			::glDrawElements(GL_TRIANGLE_STRIP, m_vVertexCut[i], GL_UNSIGNED_INT, NULL);
		}
		::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
		::glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		if ( pOpt->GetNCViewFlg(NCVIEWFLG_TEXTURE) && m_nTextureID > 0 ) {
			::glBindTexture(GL_TEXTURE_2D, 0);
			::glDisable(GL_TEXTURE_2D);
			::glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}
		::glDisableClientState(GL_NORMAL_ARRAY);
		::glDisableClientState(GL_VERTEX_ARRAY);
		::glDisable(GL_POLYGON_OFFSET_FILL);
		::glDisable(GL_LIGHTING);
	}
#endif

	if ( !pOpt->GetNCViewFlg(NCVIEWFLG_SOLIDVIEW) ||
			(pOpt->GetNCViewFlg(NCVIEWFLG_SOLIDVIEW) && pOpt->GetNCViewFlg(NCVIEWFLG_G00VIEW)) ||
			(!pOpt->GetNCViewFlg(NCVIEWFLG_DRAGRENDER) && m_enTrackingMode!=TM_NONE) ) {
		// 線画
		if ( m_glCode > 0 )
			::glCallList( m_glCode );
		else
			RenderCode();
	}

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
	HDC	hDC = dc.GetSafeHdc();

	// OpenGL pixel format の設定
    if( !SetupPixelFormat(&dc) ) {
		TRACE0("SetupPixelFormat failed\n");
		return -1;
	}

	// ﾚﾝﾀﾞﾘﾝｸﾞｺﾝﾃｷｽﾄの作成
	if( !(m_hRC = ::wglCreateContext(hDC)) ) {
		TRACE0("wglCreateContext failed\n");
		return -1;
	}

	// ﾚﾝﾀﾞﾘﾝｸﾞｺﾝﾃｷｽﾄをｶﾚﾝﾄのﾃﾞﾊﾞｲｽｺﾝﾃｷｽﾄに設定
	if( !::wglMakeCurrent(hDC, m_hRC) ) {
		TRACE0("wglMakeCurrent failed\n");
		return -1;
	}

	// OpenGL Extention 使用準備
	GLenum glewResult = ::glewInit();
	if ( glewResult != GLEW_OK ) {
		TRACE1("glewInit() failed code=%s\n", ::glewGetErrorString(glewResult));
		return -1;
	}

	::glEnable(GL_NORMALIZE);
	::glClearColor( 0, 0, 0, 0 );

#ifdef _DEBUG
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

	// 全体の切削パスを示すﾃﾞｨｽﾌﾟﾚｲﾘｽﾄ消去
	if ( m_glCode > 0 )
		::glDeleteLists(m_glCode, 1);

	// ﾊﾞｯﾌｧｵﾌﾞｼﾞｪｸﾄ関連の削除
	ClearVBO();
	ClearTexture();

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
		static_cast<CNCChild *>(GetParentFrame())->SetFactorInfo(m_dRate/LOGPIXEL, m_strGuide);
	}
	CView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}

LRESULT CNCViewGL::OnUserActivatePage(WPARAM, LPARAM lParam)
{
	if ( !m_bActive ) {
		CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
		pOpt->m_dwUpdateFlg = VIEWUPDATE_ALL;
		// 描画初期設定
		UpdateViewOption();
		//
		pOpt->m_dwUpdateFlg = 0;
	}

	// OpenGLｺﾏﾝﾄﾞを発行するにあたり、
	// 一度でもｱｸﾃｨﾌﾞになっていないと危険なため
	m_bActive = TRUE;

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

	m_rcView  = GetDocument()->GetMaxRect();
//	m_rcView |= GetDocument()->GetWorkRect();

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
	// !!! Homeキー処理で GL_INVALID_OPERATION が発生する !!!
	// !!! 当面無視で... !!!
	::glOrtho(m_rcView.left, m_rcView.right, m_rcView.top, m_rcView.bottom,
		m_rcView.low, m_rcView.high);
//	ASSERT( ::glGetError() == GL_NO_ERROR );
//	GLenum glError = ::glGetError();
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
		static_cast<CNCChild *>(GetParentFrame())->SetFactorInfo(m_dRate/LOGPIXEL, m_strGuide);
		break;
	case ID_VIEW_LENSP:
		DoScale(-1);
		break;
	case ID_VIEW_LENSN:
		DoScale(1);
		break;
	}
}

void CNCViewGL::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	CMenu	menu;
	menu.LoadMenu(IDR_NCPOPUP1);
	CMenu*	pMenu = menu.GetSubMenu(0);
	pMenu->TrackPopupMenu(TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_RIGHTBUTTON,
		point.x, point.y, AfxGetMainWnd());
}

void CNCViewGL::OnLButtonDown(UINT nFlags, CPoint point)
{
	BeginTracking( point, TM_SPIN );
}

void CNCViewGL::OnLButtonUp(UINT nFlags, CPoint point)
{
	EndTracking();
}

void CNCViewGL::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	CViewOption* pOpt = AfxGetNCVCApp()->GetViewOption();
	if ( pOpt->GetNCViewFlg(NCVIEWFLG_SOLIDVIEW) ) {
		pOpt->m_bG00View = !pOpt->m_bG00View;
		Invalidate(FALSE);
	}
}

void CNCViewGL::OnRButtonDown(UINT nFlags, CPoint point)
{
	m_ptDownClick = point;
	BeginTracking( point, TM_PAN );
}

void CNCViewGL::OnRButtonUp(UINT nFlags, CPoint point)
{
	EndTracking();
	if ( m_ptDownClick == point )
		CView::OnRButtonUp(nFlags, point);	// ｺﾝﾃｷｽﾄﾒﾆｭｰの表示
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
